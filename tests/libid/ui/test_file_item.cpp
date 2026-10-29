// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "io/library.h"

#include "test_data.h"
#include "test_library.h"
#include "MockDriver.h"

#include <engine/cmdfiles.h>
#include <io/CurrentPathSaver.h>
#include <io/file_item.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cctype>
#include <ostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::test;

namespace id::test
{

#if !defined(_WIN32)
static std::string to_upper_copy(std::string text)
{
    for (char &ch : text)
    {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return text;
}
#endif

class TestFindFileItem : public testing::Test
{
public:
    ~TestFindFileItem() override = default;

protected:
    void TearDown() override;

    std::FILE *m_file{};
    fs::path m_path;
    std::string m_diagnostic;
    ValueSaver<bool> saved_check_cur_dir_false{g_check_cur_dir, false};
};

void TestFindFileItem::TearDown()
{
    if (m_file != nullptr)
    {
        std::fclose(m_file);
        m_file = nullptr;
    }
    Test::TearDown();
}

TEST_F(TestFindFileItem, entryLimitReturnsWarningWithoutUi)
{
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(nullptr)};
    m_file = std::tmpfile();
    ASSERT_NE(nullptr, m_file);
    for (int entry = 0; entry < 2001; ++entry)
    {
        const std::string text{"entry" + std::to_string(entry) + " { }\n"};
        std::fputs(text.c_str(), m_file);
    }
    std::rewind(m_file);
    std::vector<FileEntry> choices(2001);

    EXPECT_EQ(2000, scan_entries(m_file, choices.data(), m_diagnostic));

    EXPECT_EQ("Too many entries in file, first 2000 used", m_diagnostic);
    EXPECT_STREQ("entry1999", choices[1999].name);
    EXPECT_EQ(0, choices[2000].name[0]);
}

struct FileItemInput
{
    const char *name;
    ItemType type;
    const char *directory;
    const char *filename;
    const char *entry;
};

void PrintTo(const FileItemInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestFindFileItemInputs : public TestFindFileItem, public testing::WithParamInterface<FileItemInput>
{
};

TEST_P(TestFindFileItemInputs, findsEntryWithoutChangingTheFilePath)
{
    m_path = fs::path{GetParam().directory} / GetParam().filename;

    const bool result{find_file_item(m_path, GetParam().entry, &m_file, GetParam().type, m_diagnostic)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(fs::path{GetParam().directory} / GetParam().filename, m_path);
}

INSTANTIATE_TEST_SUITE_P(Entry, TestFindFileItemInputs,
    testing::Values(
        FileItemInput{"Formula", ItemType::FORMULA, data::ID_TEST_FRM_DIR, data::ID_TEST_FRM_FILE, "Fractint"},
        FileItemInput{
            "FormulaCaseVariant", ItemType::FORMULA, data::ID_TEST_FRM_DIR, data::ID_TEST_FRM_FILE, "fractINT"},
        FileItemInput{
            "Ifs", ItemType::IFS, data::ID_TEST_IFS_DIR, data::ID_TEST_IFS_FILE, data::ID_TEST_FIRST_IFS_NAME},
        FileItemInput{"IfsCaseVariant", ItemType::IFS, data::ID_TEST_IFS_DIR, data::ID_TEST_IFS_FILE, "BINARY"},
        FileItemInput{"LSystem", ItemType::L_SYSTEM, data::ID_TEST_LSYSTEM_DIR, data::ID_TEST_LSYSTEM_FILE, "Koch1"},
        FileItemInput{
            "LSystemCaseVariant", ItemType::L_SYSTEM, data::ID_TEST_LSYSTEM_DIR, data::ID_TEST_LSYSTEM_FILE, "koch1"}),
    [](const testing::TestParamInfo<FileItemInput> &info) { return info.param.name; });

TEST_F(TestFindFileItem, diagnosticLookupRejectsEmptyNameWithoutPolling)
{
    id::misc::test::MockDriver driver;
    ValueSaver saved_driver{g_driver, &driver};
    EXPECT_CALL(driver, key_pressed()).Times(0);
    EXPECT_CALL(driver, get_key()).Times(0);
    std::string diagnostic;
    m_path = fs::path{data::ID_TEST_IFS_DIR} / data::ID_TEST_IFS_FILE;

    EXPECT_TRUE(find_file_item(m_path, "", &m_file, ItemType::IFS, diagnostic));

    EXPECT_EQ("File entry item name is empty", diagnostic);
    EXPECT_EQ(nullptr, m_file);
}

TEST_F(TestFindFileItem, diagnosticLookupReportsMissingEntryWithoutPolling)
{
    id::misc::test::MockDriver driver;
    ValueSaver saved_driver{g_driver, &driver};
    EXPECT_CALL(driver, key_pressed()).Times(0);
    EXPECT_CALL(driver, get_key()).Times(0);
    std::string diagnostic;
    m_path = fs::path{data::ID_TEST_IFS_DIR} / data::ID_TEST_IFS_FILE;

    EXPECT_TRUE(find_file_item(m_path, "missing-test-entry", &m_file, ItemType::IFS, diagnostic));

    EXPECT_EQ("'missing-test-entry' file entry item not found", diagnostic);
    EXPECT_EQ(nullptr, m_file);
}

TEST_F(TestFindFileItem, diagnosticLookupClearsOldErrorAndReturnsPositionedFile)
{
    std::string diagnostic{"previous failure"};
    m_path = fs::path{data::ID_TEST_IFS_DIR} / data::ID_TEST_IFS_FILE;

    EXPECT_FALSE(find_file_item(m_path, data::ID_TEST_FIRST_IFS_NAME, &m_file, ItemType::IFS, diagnostic));

    EXPECT_TRUE(diagnostic.empty());
    ASSERT_NE(nullptr, m_file);
    EXPECT_EQ(data::ID_TEST_FIRST_IFS_NAME[0], std::fgetc(m_file));
}

TEST_F(TestFindFileItem, emptyItemName)
{
    ValueSaver saved_first_init{g_first_init, false};
    ValueSaver saved_init_batch{g_init_batch, BatchMode::NORMAL};
    m_path = data::ID_TEST_LSYSTEM_DIR;
    m_path /= data::ID_TEST_LSYSTEM_FILE;

    const bool result{find_file_item(m_path, "", &m_file, ItemType::L_SYSTEM, m_diagnostic)};

    EXPECT_TRUE(result);
    EXPECT_EQ(nullptr, m_file);
}

namespace
{

class TestFindFileItemParFile : public TestFindFileItem
{
public:
    ~TestFindFileItemParFile() override = default;

protected:
    fs::path m_par{fs::path{data::ID_TEST_PAR_DIR} / data::ID_TEST_PAR_FILE};
    ValueSaver<fs::path> m_saved_parameter_file{g_parameter_file, m_par};
};

} // namespace

class TestFindParameterFileItem : public TestFindFileItemParFile, public testing::WithParamInterface<FileItemInput>
{
};

TEST_P(TestFindParameterFileItem, fallsBackToTheParameterFile)
{
    m_path = GetParam().filename;

    const bool result{find_file_item(m_path, GetParam().entry, &m_file, GetParam().type, m_diagnostic)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(g_parameter_file, m_path);
}

INSTANTIATE_TEST_SUITE_P(Type, TestFindParameterFileItem,
    testing::Values(FileItemInput{"Formula", ItemType::FORMULA, "", "missing.frm", "Fractint"},
        FileItemInput{"Ifs", ItemType::IFS, "", "missing.ifs", data::ID_TEST_FIRST_IFS_NAME},
        FileItemInput{"LSystem", ItemType::L_SYSTEM, "", "missing.l", "Koch1"}),
    [](const testing::TestParamInfo<FileItemInput> &info) { return info.param.name; });

struct FormulaPrecedenceInput
{
    const char *name;
    fs::path parameter_file;
};

void PrintTo(const FormulaPrecedenceInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestFormulaFilePrecedence : public TestFindFileItem, public testing::WithParamInterface<FormulaPrecedenceInput>
{
protected:
    void SetUp() override;

    ValueSaver<fs::path> m_saved_parameter_file{g_parameter_file};
};

void TestFormulaFilePrecedence::SetUp()
{
    TestFindFileItem::SetUp();
    g_parameter_file = GetParam().parameter_file;
    m_path = fs::path{data::ID_TEST_FRM_DIR} / data::ID_TEST_FRM_FILE;
}

TEST_P(TestFormulaFilePrecedence, existingFormulaPrecedesTheParameterFile)
{
    const bool result{find_file_item(m_path, "Fractint", &m_file, ItemType::FORMULA, m_diagnostic)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(fs::path{data::ID_TEST_FRM_DIR} / data::ID_TEST_FRM_FILE, m_path);
}

std::vector<FormulaPrecedenceInput> formula_precedence_inputs()
{
    std::vector<FormulaPrecedenceInput> inputs{
        {"SeparateFile", fs::path{data::ID_TEST_PAR_DIR} / data::ID_TEST_PAR_FILE}};
#if !defined(_WIN32)
    inputs.push_back({"CaseVariant", fs::path{data::ID_TEST_FRM_DIR} / to_upper_copy(data::ID_TEST_FRM_FILE)});
#endif
    return inputs;
}

INSTANTIATE_TEST_SUITE_P(ParameterFile, TestFormulaFilePrecedence, testing::ValuesIn(formula_precedence_inputs()),
    [](const testing::TestParamInfo<FormulaPrecedenceInput> &info) { return info.param.name; });

namespace
{

class TestFindFileItemCurrentDir : public TestFindFileItem
{
public:
    ~TestFindFileItemCurrentDir() override = default;

protected:
    void SetUp() override
    {
        TestFindFileItem::SetUp();
        m_path = "non-existent";
    }

    ValueSaver<bool> saved_check_cur_dir_true{g_check_cur_dir, true};
};

} // namespace

TEST_F(TestFindFileItemCurrentDir, formula)
{
    CurrentPathSaver cur_dir{data::ID_TEST_FRM_DIR};
    m_path /= data::ID_TEST_FRM_FILE;

    const bool result{find_file_item(m_path, "Fractint", &m_file, ItemType::FORMULA, m_diagnostic)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(data::ID_TEST_FRM_FILE, m_path.filename());
    EXPECT_EQ(fs::current_path() / data::ID_TEST_FRM_FILE, fs::absolute(m_path));
}

TEST_F(TestFindFileItemCurrentDir, ifs)
{
    CurrentPathSaver cur_dir{data::ID_TEST_IFS_DIR};
    m_path /= data::ID_TEST_IFS_FILE;

    const bool result{find_file_item(m_path, data::ID_TEST_FIRST_IFS_NAME, &m_file, ItemType::IFS, m_diagnostic)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(data::ID_TEST_IFS_FILE, m_path.filename());
    EXPECT_EQ(fs::current_path() / data::ID_TEST_IFS_FILE, fs::absolute(m_path));
}

TEST_F(TestFindFileItemCurrentDir, lindenmayerSystem)
{
    CurrentPathSaver cur_dir{data::ID_TEST_LSYSTEM_DIR};
    m_path /= data::ID_TEST_LSYSTEM_FILE;

    const bool result{find_file_item(m_path, "Koch1", &m_file, ItemType::L_SYSTEM, m_diagnostic)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(data::ID_TEST_LSYSTEM_FILE, m_path.filename());
    EXPECT_EQ(fs::current_path() / data::ID_TEST_LSYSTEM_FILE, fs::absolute(m_path));
}

namespace
{

class TestFindFileItemLibrary : public TestFindFileItem
{
public:
    ~TestFindFileItemLibrary() override = default;

protected:
    void SetUp() override
    {
        TestFindFileItem::SetUp();
        clear_read_library_path();
        add_read_library(library::ID_TEST_SEARCH_DIR2);
    }

    static fs::path file(const char *subdir, const char *filename)
    {
        return fs::path{library::ID_TEST_SEARCH_DIR2} / subdir / filename;
    }
};

} // namespace

TEST_F(TestFindFileItemLibrary, formula)
{
    m_path = library::ID_TEST_LIBRARY_DIR;
    m_path /= library::ID_TEST_FORMULA_FILE2;

    const bool result{find_file_item(m_path, "Fractint", &m_file, ItemType::FORMULA, m_diagnostic)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(file(data::ID_TEST_FRM_SUBDIR, library::ID_TEST_FORMULA_FILE2), m_path);
}

TEST_F(TestFindFileItemLibrary, ifs)
{
    m_path = library::ID_TEST_LIBRARY_DIR;
    m_path /= library::ID_TEST_IFS_FILE2;

    const bool result{find_file_item(m_path, data::ID_TEST_FIRST_IFS_NAME, &m_file, ItemType::IFS, m_diagnostic)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(file(data::ID_TEST_IFS_SUBDIR, library::ID_TEST_IFS_FILE2), m_path);
}

TEST_F(TestFindFileItemLibrary, lindenmayerSystem)
{
    m_path = library::ID_TEST_LIBRARY_DIR;
    m_path /= library::ID_TEST_LSYSTEM_FILE2;

    const bool result{find_file_item(m_path, "Koch1", &m_file, ItemType::L_SYSTEM, m_diagnostic)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(file(data::ID_TEST_LSYSTEM_SUBDIR, library::ID_TEST_LSYSTEM_FILE2), m_path);
}

} // namespace id::test
