// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/library.h>

#include "test_data.h"
#include "test_library.h"

#include <engine/cmdfiles.h>
#include <engine/id_data.h>
#include <io/CurrentPathSaver.h>
#include <io/special_dirs.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

using Path = std::filesystem::path;

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::test::library;

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

TEST_F(TestLibrary, findConfigInLibraryDirectory)
{
    add_read_library(ID_TEST_LIBRARY_DIR1);

    const Path path{find_file(ReadFile::ID_CONFIG, "root.cfg")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.cfg"}, path.filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR1}, path.parent_path()) << path;
}

TEST_F(TestLibrary, findConfigInLibrarySubDirectory)
{
    add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{find_file(ReadFile::ID_CONFIG, "id.cfg")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"id.cfg"}, path.filename()) << path;
    EXPECT_EQ(Path{"config"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFormulaInLibraryDirectory)
{
    add_read_library(ID_TEST_LIBRARY_DIR1);

    const Path path{find_file(ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR1}, path.parent_path()) << path;
}

TEST_F(TestLibrary, findFormulaInLibrarySubDirectory)
{
    add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{find_file(ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, preferFormulaSubDirectory)
{
    add_read_library(ID_TEST_LIBRARY_DIR1);
    add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{find_file(ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFormulaSearchMultiplePaths)
{
    add_read_library(ID_TEST_LIBRARY_DIR1);
    add_read_library(ID_TEST_LIBRARY_DIR3);
    add_read_library(ID_TEST_LIBRARY_DIR2);

    const Path path{find_file(ReadFile::FORMULA, ID_TEST_FRM_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_FRM_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename()) << path;
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

TEST_F(TestLibrary, findImageInLibraryDirectory)
{
    add_read_library(ID_TEST_LIBRARY_DIR2);

    const Path path{find_file(ReadFile::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR2}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, setSaveLibrary)
{
    set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{get_save_path(WriteFile::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, saveLibraryDefaultsToSaveDir)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_LIBRARY_DIR3};

    const Path path{get_save_path(WriteFile::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, saveFileAddsExtension)
{
    set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{get_save_path(WriteFile::MAP, "foo")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"foo.map"}, path.filename()) << path;
    EXPECT_EQ(Path{"map"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, saveOrbit)
{
    set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{get_save_path(WriteFile::ORBIT, "foo")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"foo.raw"}, path.filename()) << path;
    EXPECT_EQ(Path{"orbit"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, saveSound)
{
    set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{get_save_path(WriteFile::SOUND, "foo")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"foo.txt"}, path.filename()) << path;
    EXPECT_EQ(Path{"sound"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, saveRaytrace)
{
    set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{get_save_path(WriteFile::RAYTRACE, "foo")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"foo.ray"}, path.filename()) << path;
    EXPECT_EQ(Path{"raytrace"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFileCheckSearchDir1Root)
{
    ValueSaver saved_search_dir{g_fractal_search_dir1, ID_TEST_SEARCH_DIR1};

    const Path path{find_file(ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SEARCH_DIR1}, path.parent_path()) << path;
}

TEST_F(TestLibrary, findFilePreferSearchDir1SubDir)
{
    ValueSaver saved_search_dir{g_fractal_search_dir1, ID_TEST_SEARCH_DIR1};

    const Path path{find_file(ReadFile::IFS, ID_TEST_IFS_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IFS_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"ifs"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SEARCH_DIR1}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFileCheckSearchDir2Root)
{
    ValueSaver saved_search_dir{g_fractal_search_dir2, ID_TEST_SEARCH_DIR1};

    const Path path{find_file(ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SEARCH_DIR1}, path.parent_path()) << path;
}

TEST_F(TestLibrary, findFilePreferSearchDir2SubDir)
{
    ValueSaver saved_search_dir{g_fractal_search_dir2, ID_TEST_SEARCH_DIR1};

    const Path path{find_file(ReadFile::IFS, ID_TEST_IFS_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IFS_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"ifs"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SEARCH_DIR1}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFileAbsolutePath)
{
    auto file_path{Path{ID_TEST_LIBRARY_DIR} / ID_TEST_FRM_FILE};

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
    ValueSaver saved_search_dir1{g_fractal_search_dir1, data::ID_TEST_DATA_SUBDIR};
    ValueSaver saved_search_dir2{g_fractal_search_dir2, data::ID_TEST_DATA_SUBDIR};
    ValueSaver saved_save_dir{g_save_dir, data::ID_TEST_DATA_SUBDIR};
    set_save_library(data::ID_TEST_DATA_SUBDIR);

    const Path path{find_file(ReadFile::FORMULA, ID_TEST_FORMULA_FILE2)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_FORMULA_FILE2}, path.filename()) << path;
    EXPECT_EQ(id::test::data::ID_TEST_FRM_SUBDIR, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{"formula"} / ID_TEST_FORMULA_FILE2, path) << path;
}

TEST_F(TestLibrary, findWildcardNoMatches)
{
    add_read_library(ID_TEST_LIBRARY_DIR1);
    add_read_library(ID_TEST_LIBRARY_DIR2);
    add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{find_wildcard_first(ReadFile::IMAGE, ID_TEST_NO_SUCH_IMAGE_FILE)};

    ASSERT_TRUE(path.empty()) << path;
}

TEST_F(TestLibrary, findWildcardFirstMatchSubDirTestGif)
{
    add_read_library(ID_TEST_LIBRARY_DIR1);
    add_read_library(ID_TEST_LIBRARY_DIR2);
    add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{find_wildcard_first(ReadFile::IMAGE, "*.gif")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"test.gif"}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR2}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findWildcardFirstMatchSubDirRootGif)
{
    add_read_library(ID_TEST_LIBRARY_DIR1);
    add_read_library(ID_TEST_LIBRARY_DIR3);
    add_read_library(ID_TEST_LIBRARY_DIR2);

    const Path path{find_wildcard_first(ReadFile::IMAGE, "*.gif")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.gif"}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findWildcardSeqence)
{
    add_read_library(ID_TEST_LIBRARY_DIR2);
    add_read_library(ID_TEST_LIBRARY_DIR3);
    std::vector expected{                              //
        Path{ID_TEST_LIBRARY_DIR2} / "image/test.gif", //
        Path{ID_TEST_LIBRARY_DIR3} / "image/root.gif"};
    std::vector<Path> results;

    for (Path next{find_wildcard_first(ReadFile::IMAGE, "*.gif")}; !next.empty();
        next = find_wildcard_next())
    {
        results.emplace_back(next);
    }

    EXPECT_EQ(expected, results);
}

TEST_F(TestLibrary, findWildcardRootFilesAfterSubdirFiles)
{
    add_read_library(ID_TEST_LIBRARY_DIR1);

    const Path first{find_wildcard_first(ReadFile::IMAGE, "*.gif")};

    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR1}/"root.gif", first);
}

TEST_F(TestLibrary, findWildcardSequenceRootFilesAfterSubdirFiles)
{
    add_read_library(ID_TEST_LIBRARY_DIR1);
    add_read_library(ID_TEST_LIBRARY_DIR2);
    add_read_library(ID_TEST_LIBRARY_DIR3);
    std::vector expected{                              //
        Path{ID_TEST_LIBRARY_DIR2} / "image/test.gif", //
        Path{ID_TEST_LIBRARY_DIR3} / "image/root.gif", //
        Path{ID_TEST_LIBRARY_DIR1} / "root.gif"};
    std::vector<Path> results;

    for (Path next{find_wildcard_first(ReadFile::IMAGE, "*.gif")}; !next.empty();
        next = find_wildcard_next())
    {
        results.emplace_back(next);
    }

    EXPECT_EQ(expected, results);
}

TEST_F(TestLibrary, findWildcardSaveLibrarySubDir)
{
    add_read_library(ID_TEST_LIBRARY_DIR1);
    add_read_library(ID_TEST_LIBRARY_DIR2);
    add_read_library(ID_TEST_LIBRARY_DIR3);
    set_save_library(ID_TEST_SAVE_DIR);

    const Path path{find_wildcard_first(ReadFile::IMAGE, ID_TEST_SAVE_IMAGE_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_SAVE_IMAGE_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SAVE_DIR}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findWildcardSaveLibraryRootDir)
{
    add_read_library(ID_TEST_LIBRARY_DIR1);
    add_read_library(ID_TEST_LIBRARY_DIR2);
    add_read_library(ID_TEST_LIBRARY_DIR3);
    set_save_library(ID_TEST_SAVE_DIR);

    const Path path{find_wildcard_first(ReadFile::IMAGE, ID_TEST_SAVE_IMAGE_ROOT_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_SAVE_IMAGE_ROOT_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SAVE_DIR}, path.parent_path()) << path;
}

} // namespace id::test
