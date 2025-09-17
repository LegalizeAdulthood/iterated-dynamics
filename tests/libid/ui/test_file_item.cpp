// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/library.h"

#include <ui/file_item.h>

#include "test_data.h"
#include "test_library.h"

#include <engine/cmdfiles.h>
#include <io/CurrentPathSaver.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::test;
using namespace id::ui;

namespace id::test
{

class TestFindFileItem : public testing::Test
{
public:
    ~TestFindFileItem() override = default;

protected:
    void TearDown() override;

    std::FILE *m_file{};
    fs::path m_path;
    ValueSaver<bool> saved_check_cur_dir{g_check_cur_dir, false};
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

TEST_F(TestFindFileItem, formula)
{
    m_path = data::ID_TEST_FRM_DIR;
    m_path /= data::ID_TEST_FRM_FILE;

    const bool result{find_file_item(m_path, "Fractint", &m_file, ItemType::FORMULA)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(fs::path{data::ID_TEST_FRM_DIR} / data::ID_TEST_FRM_FILE, m_path);
}

TEST_F(TestFindFileItem, ifs)
{
    m_path = data::ID_TEST_IFS_DIR;
    m_path /= data::ID_TEST_IFS_FILE;

    const bool result{find_file_item(m_path, data::ID_TEST_FIRST_IFS_NAME, &m_file, ItemType::IFS)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(fs::path{data::ID_TEST_IFS_DIR} / data::ID_TEST_IFS_FILE, m_path);
}

TEST_F(TestFindFileItem, lindenmayerSystem)
{
    m_path = data::ID_TEST_LSYSTEM_DIR;
    m_path /= data::ID_TEST_LSYSTEM_FILE;

    const bool result{find_file_item(m_path, "Koch1", &m_file, ItemType::L_SYSTEM)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(fs::path{data::ID_TEST_LSYSTEM_DIR} / data::ID_TEST_LSYSTEM_FILE, m_path);
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

TEST_F(TestFindFileItemParFile, formula)
{
    m_path = "missing.frm";

    const bool result{find_file_item(m_path, "Fractint", &m_file, ItemType::FORMULA)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(g_parameter_file, m_path);
}

TEST_F(TestFindFileItemParFile, ifs)
{
    m_path = "missing.ifs";

    const bool result{find_file_item(m_path, data::ID_TEST_FIRST_IFS_NAME, &m_file, ItemType::IFS)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(g_parameter_file, m_path);
}

TEST_F(TestFindFileItemParFile, lindenmayerSystem)
{
    m_path = "missing.l";

    const bool result{find_file_item(m_path, "Koch1", &m_file, ItemType::L_SYSTEM)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(g_parameter_file, m_path);
}

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

    ValueSaver<bool> saved_check_cur_dir{g_check_cur_dir, true};
};

} // namespace

TEST_F(TestFindFileItemCurrentDir, formula)
{
    CurrentPathSaver cur_dir{data::ID_TEST_FRM_DIR};
    m_path /= data::ID_TEST_FRM_FILE;

    const bool result{find_file_item(m_path, "Fractint", &m_file, ItemType::FORMULA)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(data::ID_TEST_FRM_FILE, m_path.filename());
    EXPECT_EQ(fs::current_path() / data::ID_TEST_FRM_FILE, fs::absolute(m_path));
}

TEST_F(TestFindFileItemCurrentDir, ifs)
{
    CurrentPathSaver cur_dir{data::ID_TEST_IFS_DIR};
    m_path /= data::ID_TEST_IFS_FILE;

    const bool result{find_file_item(m_path, data::ID_TEST_FIRST_IFS_NAME, &m_file, ItemType::IFS)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(data::ID_TEST_IFS_FILE, m_path.filename());
    EXPECT_EQ(fs::current_path() / data::ID_TEST_IFS_FILE, fs::absolute(m_path));
}

TEST_F(TestFindFileItemCurrentDir, lindenmayerSystem)
{
    CurrentPathSaver cur_dir{data::ID_TEST_LSYSTEM_DIR};
    m_path /= data::ID_TEST_LSYSTEM_FILE;

    const bool result{find_file_item(m_path, "Koch1", &m_file, ItemType::L_SYSTEM)};

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

    const bool result{find_file_item(m_path, "Fractint", &m_file, ItemType::FORMULA)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(file(data::ID_TEST_FRM_SUBDIR, library::ID_TEST_FORMULA_FILE2), m_path);
}

TEST_F(TestFindFileItemLibrary, ifs)
{
    m_path = library::ID_TEST_LIBRARY_DIR;
    m_path /= library::ID_TEST_IFS_FILE2;

    const bool result{find_file_item(m_path, data::ID_TEST_FIRST_IFS_NAME, &m_file, ItemType::IFS)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(file(data::ID_TEST_IFS_SUBDIR, library::ID_TEST_IFS_FILE2), m_path);
}

TEST_F(TestFindFileItemLibrary, lindenmayerSystem)
{
    m_path = library::ID_TEST_LIBRARY_DIR;
    m_path /= library::ID_TEST_LSYSTEM_FILE2;

    const bool result{find_file_item(m_path, "Koch1", &m_file, ItemType::L_SYSTEM)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
    EXPECT_EQ(file(data::ID_TEST_LSYSTEM_SUBDIR, library::ID_TEST_LSYSTEM_FILE2), m_path);
}

} // namespace id::test
