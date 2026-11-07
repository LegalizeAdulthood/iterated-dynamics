// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/library.h>

#include <EnvVarSaver.h>
#include <MockSpecialDirectories.h>
#include <test_data.h>
#include <test_library.h>

#include <io/CurrentPathSaver.h>
#include <io/special_dirs.h>
#include <misc/ValueSaver.h>

#include <config/port_config.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

using Path = std::filesystem::path;

using id::io::test::MockSpecialDirectories;

using namespace id::io;
using namespace id::misc;
using namespace id::test::library;
using namespace testing;

namespace id::test
{

class TestLibrary : public testing::Test
{
public:
    ~TestLibrary() override = default;

protected:
    void SetUp() override;
    void TearDown() override;
};

void TestLibrary::SetUp()
{
    Test::SetUp();
    clear_read_library_path();
    clear_save_library();
}

void TestLibrary::TearDown()
{
    clear_read_library_path();
    clear_save_library();
    Test::TearDown();
}

class TestLibraryInitialization : public TestLibrary
{
protected:
    void prepare_directories(
        const Path &current_dir, const Path &program_dir, const Path &documents_dir, const Path &config_path);
    void TearDown() override;

private:
    EnvVarSaver m_fract_dir{"FRACTDIR", nullptr};
    ValueSaver<std::shared_ptr<SpecialDirectories>> m_special_dirs{g_special_dirs};
    std::unique_ptr<CurrentPathSaver> m_current_directory;
    Path m_config_path;
};

void TestLibraryInitialization::prepare_directories(
    const Path &current_dir, const Path &program_dir, const Path &documents_dir, const Path &config_path)
{
    m_config_path = config_path;
    std::filesystem::create_directories(current_dir);
    std::filesystem::create_directories(config_path.parent_path());
    std::ofstream{config_path} << "; test config\n";
    m_current_directory = std::make_unique<CurrentPathSaver>(current_dir);
    const std::shared_ptr<NiceMock<MockSpecialDirectories>> directories{
        std::make_shared<NiceMock<MockSpecialDirectories>>()};
    ON_CALL(*directories, program_dir()).WillByDefault(Return(program_dir));
    ON_CALL(*directories, documents_dir()).WillByDefault(Return(documents_dir));
    g_special_dirs = directories;
}

void TestLibraryInitialization::TearDown()
{
    m_current_directory.reset();
    std::error_code error;
    std::filesystem::remove(m_config_path, error);
    TestLibrary::TearDown();
}

TEST_F(TestLibraryInitialization, initLibrariesFindsConfigAwayFromCurrentDirectory)
{
    const Path current_dir{Path{data::ID_TEST_DATA_DIR} / "other-current-dir"};
    const Path documents_dir{Path{data::ID_TEST_DATA_DIR} / "documents"};
    const Path config_path{documents_dir / ID_PROGRAM_NAME / "config" / "id.cfg"};
    prepare_directories(current_dir, "missing-program-dir", documents_dir, config_path);

    init_libraries();

    EXPECT_EQ(config_path, find_file(ReadFile::ID_CONFIG, "id.cfg"));
}

TEST_F(TestLibraryInitialization, defaultReadLibrariesPreferProgramDirectoryToCompiledHomeDirectory)
{
    const Path current_dir{Path{data::ID_TEST_DATA_DIR} / "program-dir-current-dir"};
    const Path program_dir{Path{data::ID_TEST_DATA_DIR} / "program-dir"};
    const Path config_path{program_dir / "config" / "id.cfg"};
    prepare_directories(current_dir, program_dir, "missing-documents-dir", config_path);

    init_default_read_libraries();

    EXPECT_EQ(config_path, find_file(ReadFile::ID_CONFIG, "id.cfg"));
}

struct LibraryLookupInput
{
    const char *name;
    std::vector<Path> roots;
    ReadFile category;
    const char *filename;
    Path expected;
};

void PrintTo(const LibraryLookupInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestLibraryLookup : public TestLibrary, public testing::WithParamInterface<LibraryLookupInput>
{
protected:
    void SetUp() override;
};

void TestLibraryLookup::SetUp()
{
    TestLibrary::SetUp();
    for (const Path &root : GetParam().roots)
    {
        add_read_library(root);
    }
}

TEST_P(TestLibraryLookup, findsFileUsingOrderedLibraryRoots)
{
    const Path path{find_file(GetParam().category, GetParam().filename)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(GetParam().expected, path) << path;
}

INSTANTIATE_TEST_SUITE_P(LookupInputs, TestLibraryLookup,
    testing::Values(LibraryLookupInput{"ConfigRoot", {ID_TEST_LIBRARY_DIR1}, ReadFile::ID_CONFIG, "root.cfg",
                        Path{ID_TEST_LIBRARY_DIR1} / "root.cfg"},
        LibraryLookupInput{"ConfigSubdirectory", {ID_TEST_LIBRARY_DIR3}, ReadFile::ID_CONFIG, "id.cfg",
            Path{ID_TEST_LIBRARY_DIR3} / "config" / "id.cfg"},
        LibraryLookupInput{"FormulaRoot", {ID_TEST_LIBRARY_DIR1}, ReadFile::FORMULA, "root.frm",
            Path{ID_TEST_LIBRARY_DIR1} / "root.frm"},
        LibraryLookupInput{"FormulaSubdirectory", {ID_TEST_LIBRARY_DIR3}, ReadFile::FORMULA, "root.frm",
            Path{ID_TEST_LIBRARY_DIR3} / "formula" / "root.frm"},
        LibraryLookupInput{"PreferFormulaSubdirectory", {ID_TEST_LIBRARY_DIR1, ID_TEST_LIBRARY_DIR3}, ReadFile::FORMULA,
            "root.frm", Path{ID_TEST_LIBRARY_DIR3} / "formula" / "root.frm"},
        LibraryLookupInput{"MultipleFormulaRoots", {ID_TEST_LIBRARY_DIR1, ID_TEST_LIBRARY_DIR3, ID_TEST_LIBRARY_DIR2},
            ReadFile::FORMULA, ID_TEST_FRM_FILE, Path{ID_TEST_LIBRARY_DIR2} / "formula" / ID_TEST_FRM_FILE},
        LibraryLookupInput{"PluralFormula", {ID_TEST_LIBRARY_DIR2}, ReadFile::FORMULA, ID_TEST_FORMULA_FILE2,
            Path{ID_TEST_LIBRARY_DIR2} / "formulas" / ID_TEST_FORMULA_FILE2},
        LibraryLookupInput{"PluralMap", {ID_TEST_LIBRARY_DIR2}, ReadFile::MAP, ID_TEST_MAP_FILE2,
            Path{ID_TEST_LIBRARY_DIR2} / "maps" / ID_TEST_MAP_FILE2},
        LibraryLookupInput{"PluralParameter", {ID_TEST_LIBRARY_DIR2}, ReadFile::PARAMETER, ID_TEST_PAR_FILE2,
            Path{ID_TEST_LIBRARY_DIR2} / "pars" / ID_TEST_PAR_FILE2},
        LibraryLookupInput{"ImageDirectory", {ID_TEST_LIBRARY_DIR2}, ReadFile::IMAGE, ID_TEST_IMAGE_FILE,
            Path{ID_TEST_LIBRARY_DIR2} / "image" / ID_TEST_IMAGE_FILE}),
    [](const testing::TestParamInfo<LibraryLookupInput> &info) { return info.param.name; });

class TestLibraryFallback : public TestLibrary
{
private:
    EnvVarSaver m_fract_dir{"FRACTDIR", ID_TEST_SEARCH_DIR1};
};

TEST_F(TestLibraryFallback, findFileIgnoresPluralFallbackSubDirectory)
{
    init_default_read_libraries();

    const Path path{find_file(ReadFile::FORMULA, ID_TEST_FORMULA_FILE2)};

    EXPECT_TRUE(path.empty()) << path;
}

TEST_F(TestLibraryFallback, findFileReadLibraryPrecedesFallback)
{
    add_read_library(ID_TEST_LIBRARY_DIR2);
    init_default_read_libraries();

    const Path path{find_file(ReadFile::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR2}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFileFromSaveLibrary)
{
    add_read_library(ID_TEST_LIBRARY_DIR2);
    set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{find_file(ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

struct LibrarySaveInput
{
    const char *name;
    WriteFile category;
    const char *requested;
    const char *filename;
    const char *subdirectory;
};

void PrintTo(const LibrarySaveInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestLibrarySave : public TestLibrary, public testing::WithParamInterface<LibrarySaveInput>
{
protected:
    void SetUp() override;
};

void TestLibrarySave::SetUp()
{
    TestLibrary::SetUp();
    set_save_library(ID_TEST_LIBRARY_DIR3);
}

TEST_P(TestLibrarySave, usesCategoryExtensionAndSubdirectory)
{
    const LibrarySaveInput &input{GetParam()};

    const Path path{get_save_path(input.category, input.requested)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{input.filename}, path.filename()) << path;
    EXPECT_EQ(Path{input.subdirectory}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

INSTANTIATE_TEST_SUITE_P(SaveInputs, TestLibrarySave,
    testing::Values(LibrarySaveInput{"Image", WriteFile::IMAGE, ID_TEST_IMAGE_FILE, ID_TEST_IMAGE_FILE, "image"},
        LibrarySaveInput{"Map", WriteFile::MAP, "foo", "foo.map", "map"},
        LibrarySaveInput{"Formula", WriteFile::FORMULA, "foo", "foo.frm", "formula"},
        LibrarySaveInput{"Parameter", WriteFile::PARAMETER, "foo", "foo.par", "par"},
        LibrarySaveInput{"Orbit", WriteFile::ORBIT, "foo", "foo.raw", "orbit"},
        LibrarySaveInput{"Sound", WriteFile::SOUND, "foo", "foo.txt", "sound"},
        LibrarySaveInput{"Raytrace", WriteFile::RAYTRACE, "foo", "foo.ray", "raytrace"},
        LibrarySaveInput{"Debug", WriteFile::DEBUG, "trace", "trace.txt", "debug"},
        LibrarySaveInput{"DebugJson", WriteFile::DEBUG_JSON, "history", "history.json", "debug"}),
    [](const testing::TestParamInfo<LibrarySaveInput> &info) { return info.param.name; });

TEST_F(TestLibrary, saveLibraryDefaultsToSaveDir)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_LIBRARY_DIR3};

    const Path path{get_save_path(WriteFile::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

class TestLibraryFallbackLookup : public TestLibraryFallback, public testing::WithParamInterface<LibraryLookupInput>
{
};

TEST_P(TestLibraryFallbackLookup, findsFileInFallbackDirectory)
{
    init_default_read_libraries();

    const Path path{find_file(GetParam().category, GetParam().filename)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(GetParam().expected, path) << path;
}

INSTANTIATE_TEST_SUITE_P(FallbackInputs, TestLibraryFallbackLookup,
    testing::Values(
        LibraryLookupInput{"Root", {}, ReadFile::FORMULA, "root.frm", Path{ID_TEST_SEARCH_DIR1} / "root.frm"},
        LibraryLookupInput{
            "Subdirectory", {}, ReadFile::IFS, ID_TEST_IFS_FILE, Path{ID_TEST_SEARCH_DIR1} / "ifs" / ID_TEST_IFS_FILE}),
    [](const testing::TestParamInfo<LibraryLookupInput> &info) { return info.param.name; });

TEST_F(TestLibrary, findFileAbsolutePath)
{
    const auto file_path{Path{ID_TEST_LIBRARY_DIR} / ID_TEST_FRM_FILE};

    const Path path{find_file(ReadFile::FORMULA, file_path)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(ID_TEST_FRM_FILE, path.filename()) << path;
    EXPECT_EQ(ID_TEST_LIBRARY_DIR, path.parent_path()) << path;
    EXPECT_EQ(file_path, path) << path;
}

TEST_F(TestLibrary, findFileCurrentDirectory)
{
    // this is where we expect to find ig
    CurrentPathSaver saved_cur_dir{ID_TEST_SEARCH_DIR2};
    ValueSaver saved_check_cur_dir{g_check_cur_dir, true};
    // these are all the places it could look
    ValueSaver saved_save_dir{g_save_dir, data::ID_TEST_DATA_SUBDIR};
    add_read_library(data::ID_TEST_DATA_SUBDIR);
    set_save_library(data::ID_TEST_DATA_SUBDIR);

    const Path path{find_file(ReadFile::FORMULA, ID_TEST_FORMULA_FILE2)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_FORMULA_FILE2}, path.filename()) << path;
    EXPECT_EQ(id::test::data::ID_TEST_FRM_SUBDIR, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{"formula"} / ID_TEST_FORMULA_FILE2, path) << path;
}

class TestLibraryCurrentFallback : public TestLibrary
{
private:
    EnvVarSaver m_fract_dir{"FRACTDIR", nullptr};
    CurrentPathSaver m_current_directory{ID_TEST_SEARCH_DIR1};
};

TEST_F(TestLibraryCurrentFallback, defaultReadLibrariesUseCurrentDirectoryWhenFractDirUnset)
{
    init_default_read_libraries();

    const Path path{find_file(ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{"root.frm"}, path) << path;
}

struct LibraryWildcardInput
{
    const char *name;
    std::vector<Path> roots;
    Path current_directory;
    bool check_current_directory;
    Path save_library;
    ReadFile category;
    Path pattern;
    Path expected;
};

void PrintTo(const LibraryWildcardInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestLibraryWildcard : public TestLibrary, public testing::WithParamInterface<LibraryWildcardInput>
{
protected:
    void SetUp() override;
    void TearDown() override;

private:
    std::unique_ptr<CurrentPathSaver> m_current_directory;
    ValueSaver<bool> m_check_current_directory{g_check_cur_dir};
};

void TestLibraryWildcard::SetUp()
{
    TestLibrary::SetUp();
    const LibraryWildcardInput &input{GetParam()};
    if (!input.current_directory.empty())
    {
        m_current_directory = std::make_unique<CurrentPathSaver>(input.current_directory);
    }
    if (input.check_current_directory)
    {
        g_check_cur_dir = true;
    }
    for (const Path &root : input.roots)
    {
        add_read_library(root);
    }
    if (!input.save_library.empty())
    {
        set_save_library(input.save_library);
    }
}

void TestLibraryWildcard::TearDown()
{
    m_current_directory.reset();
    TestLibrary::TearDown();
}

TEST_P(TestLibraryWildcard, returnsFirstMatchUsingSearchPolicy)
{
    const LibraryWildcardInput &input{GetParam()};

    const Path path{find_wildcard_first(input.category, input.pattern.string())};

    ASSERT_EQ(input.expected.empty(), path.empty()) << path;
    EXPECT_EQ(input.expected, path) << path;
}

INSTANTIATE_TEST_SUITE_P(FirstMatchInputs, TestLibraryWildcard,
    testing::Values(
        LibraryWildcardInput{"NoMatches", {ID_TEST_LIBRARY_DIR1, ID_TEST_LIBRARY_DIR2, ID_TEST_LIBRARY_DIR3}, {}, false,
            {}, ReadFile::IMAGE, ID_TEST_NO_SUCH_IMAGE_FILE, {}},
        LibraryWildcardInput{"ReorderedSubdirectories",
            {ID_TEST_LIBRARY_DIR1, ID_TEST_LIBRARY_DIR3, ID_TEST_LIBRARY_DIR2}, {}, false, {}, ReadFile::IMAGE, "*.gif",
            Path{ID_TEST_LIBRARY_DIR3} / "image" / "root.gif"},
        LibraryWildcardInput{"RootFile", {ID_TEST_LIBRARY_DIR1}, {}, false, {}, ReadFile::IMAGE, "*.gif",
            Path{ID_TEST_LIBRARY_DIR1} / "root.gif"},
        LibraryWildcardInput{"CurrentDirectory", {ID_TEST_LIBRARY_DIR3}, ID_TEST_LIBRARY_DIR2, true, {},
            ReadFile::IMAGE, "*.gif", Path{"image"} / "test.gif"},
        LibraryWildcardInput{"PluralSubdirectory", {ID_TEST_LIBRARY_DIR2}, {}, false, {}, ReadFile::FORMULA,
            ID_TEST_FORMULA_FILE2, Path{ID_TEST_LIBRARY_DIR2} / "formulas" / ID_TEST_FORMULA_FILE2},
        LibraryWildcardInput{"ExplicitAbsoluteDirectory", {ID_TEST_LIBRARY_DIR3}, {}, false, {}, ReadFile::IMAGE,
            Path{ID_TEST_LIBRARY_DIR1} / "*.gif", Path{ID_TEST_LIBRARY_DIR1} / "root.gif"},
        LibraryWildcardInput{"SaveSubdirectory", {ID_TEST_LIBRARY_DIR1, ID_TEST_LIBRARY_DIR2, ID_TEST_LIBRARY_DIR3}, {},
            false, ID_TEST_SAVE_DIR, ReadFile::IMAGE, ID_TEST_SAVE_IMAGE_FILE,
            Path{ID_TEST_SAVE_DIR} / "image" / ID_TEST_SAVE_IMAGE_FILE},
        LibraryWildcardInput{"SaveRoot", {ID_TEST_LIBRARY_DIR1, ID_TEST_LIBRARY_DIR2, ID_TEST_LIBRARY_DIR3}, {}, false,
            ID_TEST_SAVE_DIR, ReadFile::IMAGE, ID_TEST_SAVE_IMAGE_ROOT_FILE,
            Path{ID_TEST_SAVE_DIR} / ID_TEST_SAVE_IMAGE_ROOT_FILE}),
    [](const testing::TestParamInfo<LibraryWildcardInput> &info) { return info.param.name; });

struct LibrarySequenceInput
{
    const char *name;
    std::vector<Path> roots;
    Path current_directory;
    Path pattern;
    std::vector<Path> expected;
};

void PrintTo(const LibrarySequenceInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestLibrarySequence : public TestLibrary, public testing::WithParamInterface<LibrarySequenceInput>
{
protected:
    void SetUp() override;
    void TearDown() override;

private:
    std::unique_ptr<CurrentPathSaver> m_current_directory;
};

void TestLibrarySequence::SetUp()
{
    TestLibrary::SetUp();
    const LibrarySequenceInput &input{GetParam()};
    if (!input.current_directory.empty())
    {
        m_current_directory = std::make_unique<CurrentPathSaver>(input.current_directory);
    }
    for (const Path &root : input.roots)
    {
        add_read_library(root);
    }
}

void TestLibrarySequence::TearDown()
{
    m_current_directory.reset();
    TestLibrary::TearDown();
}

TEST_P(TestLibrarySequence, enumeratesMatchesInSearchOrder)
{
    const LibrarySequenceInput &input{GetParam()};
    std::vector<Path> results;

    for (Path next{find_wildcard_first(ReadFile::IMAGE, input.pattern.string())}; !next.empty();
        next = find_wildcard_next())
    {
        results.emplace_back(next);
    }

    EXPECT_EQ(input.expected, results);
}

INSTANTIATE_TEST_SUITE_P(SequenceInputs, TestLibrarySequence,
    testing::Values(LibrarySequenceInput{"Subdirectories", {ID_TEST_LIBRARY_DIR2, ID_TEST_LIBRARY_DIR3}, {}, "*.gif",
                        {Path{ID_TEST_LIBRARY_DIR2} / "image/test.gif", Path{ID_TEST_LIBRARY_DIR3} / "image/root.gif"}},
        LibrarySequenceInput{"RootAfterSubdirectories",
            {ID_TEST_LIBRARY_DIR1, ID_TEST_LIBRARY_DIR2, ID_TEST_LIBRARY_DIR3}, {}, "*.gif",
            {Path{ID_TEST_LIBRARY_DIR2} / "image/test.gif", Path{ID_TEST_LIBRARY_DIR3} / "image/root.gif",
                Path{ID_TEST_LIBRARY_DIR1} / "root.gif"}},
        LibrarySequenceInput{"ExplicitRelativeDirectory", {ID_TEST_LIBRARY_DIR3}, ID_TEST_LIBRARY_DIR,
            Path{"lib1"} / "*.gif", {Path{"lib1"} / "root.gif"}}),
    [](const testing::TestParamInfo<LibrarySequenceInput> &info) { return info.param.name; });

} // namespace id::test
