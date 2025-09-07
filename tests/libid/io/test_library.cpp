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

using namespace id;
using namespace id::io;
using namespace id::misc;
using namespace id::test::library;

namespace
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
    id::io::clear_read_library_path();
    id::io::clear_save_library();
}

void TestLibrary::TearDown()
{
    id::io::clear_read_library_path();
    id::io::clear_save_library();
    Test::TearDown();
}

} // namespace

TEST_F(TestLibrary, findConfigInLibraryDirectory)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);

    const Path path{id::io::find_file(id::io::ReadFile::ID_CONFIG, "root.cfg")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.cfg"}, path.filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR1}, path.parent_path()) << path;
}

TEST_F(TestLibrary, findConfigInLibrarySubDirectory)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::find_file(id::io::ReadFile::ID_CONFIG, "id.cfg")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"id.cfg"}, path.filename()) << path;
    EXPECT_EQ(Path{"config"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFormulaInLibraryDirectory)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);

    const Path path{id::io::find_file(id::io::ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR1}, path.parent_path()) << path;
}

TEST_F(TestLibrary, findFormulaInLibrarySubDirectory)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::find_file(id::io::ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, preferFormulaSubDirectory)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::find_file(id::io::ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFormulaSearchMultiplePaths)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);

    const Path path{id::io::find_file(id::io::ReadFile::FORMULA, ID_TEST_FRM_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_FRM_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR2}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFileFromSaveLibrary)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);
    id::io::set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::find_file(id::io::ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findImageInLibraryDirectory)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);

    const Path path{id::io::find_file(id::io::ReadFile::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR2}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, setSaveLibrary)
{
    id::io::set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::get_save_path(id::io::WriteFile::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, saveLibraryDefaultsToSaveDir)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_LIBRARY_DIR3};

    const Path path{id::io::get_save_path(id::io::WriteFile::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, saveFileAddsExtension)
{
    id::io::set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::get_save_path(id::io::WriteFile::MAP, "foo")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"foo.map"}, path.filename()) << path;
    EXPECT_EQ(Path{"map"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, saveOrbit)
{
    id::io::set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::get_save_path(id::io::WriteFile::ORBIT, "foo")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"foo.raw"}, path.filename()) << path;
    EXPECT_EQ(Path{"orbit"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, saveSound)
{
    id::io::set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::get_save_path(id::io::WriteFile::SOUND, "foo")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"foo.txt"}, path.filename()) << path;
    EXPECT_EQ(Path{"sound"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, saveRaytrace)
{
    id::io::set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::get_save_path(id::io::WriteFile::RAYTRACE, "foo")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"foo.ray"}, path.filename()) << path;
    EXPECT_EQ(Path{"raytrace"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFileCheckSearchDir1Root)
{
    ValueSaver saved_search_dir{g_fractal_search_dir1, ID_TEST_SEARCH_DIR1};

    const Path path{id::io::find_file(id::io::ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SEARCH_DIR1}, path.parent_path()) << path;
}

TEST_F(TestLibrary, findFilePreferSearchDir1SubDir)
{
    ValueSaver saved_search_dir{g_fractal_search_dir1, ID_TEST_SEARCH_DIR1};

    const Path path{id::io::find_file(id::io::ReadFile::IFS, ID_TEST_IFS_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IFS_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"ifs"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SEARCH_DIR1}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFileCheckSearchDir2Root)
{
    ValueSaver saved_search_dir{g_fractal_search_dir2, ID_TEST_SEARCH_DIR1};

    const Path path{id::io::find_file(id::io::ReadFile::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SEARCH_DIR1}, path.parent_path()) << path;
}

TEST_F(TestLibrary, findFilePreferSearchDir2SubDir)
{
    ValueSaver saved_search_dir{g_fractal_search_dir2, ID_TEST_SEARCH_DIR1};

    const Path path{id::io::find_file(id::io::ReadFile::IFS, ID_TEST_IFS_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_IFS_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"ifs"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SEARCH_DIR1}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findFileAbsolutePath)
{
    auto file_path{Path{ID_TEST_LIBRARY_DIR} / ID_TEST_FRM_FILE};

    const Path path{id::io::find_file(id::io::ReadFile::FORMULA, file_path)};

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
    ValueSaver saved_search_dir1{g_fractal_search_dir1, id::test::data::ID_TEST_DATA_SUBDIR};
    ValueSaver saved_search_dir2{g_fractal_search_dir2, id::test::data::ID_TEST_DATA_SUBDIR};
    ValueSaver saved_save_dir{g_save_dir, id::test::data::ID_TEST_DATA_SUBDIR};
    id::io::set_save_library(id::test::data::ID_TEST_DATA_SUBDIR);

    const Path path{id::io::find_file(id::io::ReadFile::FORMULA, ID_TEST_FORMULA_FILE2)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_FORMULA_FILE2}, path.filename()) << path;
    EXPECT_EQ(id::test::data::ID_TEST_FRM_SUBDIR, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{"formula"} / ID_TEST_FORMULA_FILE2, path) << path;
}

TEST_F(TestLibrary, findWildcardNoMatches)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::find_wildcard_first(id::io::ReadFile::IMAGE, ID_TEST_NO_SUCH_IMAGE_FILE)};

    ASSERT_TRUE(path.empty()) << path;
}

TEST_F(TestLibrary, findWildcardFirstMatchSubDirTestGif)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::find_wildcard_first(id::io::ReadFile::IMAGE, "*.gif")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"test.gif"}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR2}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findWildcardFirstMatchSubDirRootGif)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);

    const Path path{id::io::find_wildcard_first(id::io::ReadFile::IMAGE, "*.gif")};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{"root.gif"}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findWildcardSeqence)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);
    std::vector expected{                              //
        Path{ID_TEST_LIBRARY_DIR2} / "image/test.gif", //
        Path{ID_TEST_LIBRARY_DIR3} / "image/root.gif"};
    std::vector<Path> results;

    for (Path next{id::io::find_wildcard_first(id::io::ReadFile::IMAGE, "*.gif")}; !next.empty();
        next = id::io::find_wildcard_next())
    {
        results.emplace_back(next);
    }

    EXPECT_EQ(expected, results);
}

TEST_F(TestLibrary, findWildcardRootFilesAfterSubdirFiles)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);

    const Path first{id::io::find_wildcard_first(id::io::ReadFile::IMAGE, "*.gif")};

    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR1}/"root.gif", first);
}

TEST_F(TestLibrary, findWildcardSequenceRootFilesAfterSubdirFiles)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);
    std::vector expected{                              //
        Path{ID_TEST_LIBRARY_DIR2} / "image/test.gif", //
        Path{ID_TEST_LIBRARY_DIR3} / "image/root.gif", //
        Path{ID_TEST_LIBRARY_DIR1} / "root.gif"};
    std::vector<Path> results;

    for (Path next{id::io::find_wildcard_first(id::io::ReadFile::IMAGE, "*.gif")}; !next.empty();
        next = id::io::find_wildcard_next())
    {
        results.emplace_back(next);
    }

    EXPECT_EQ(expected, results);
}

TEST_F(TestLibrary, findWildcardSaveLibrarySubDir)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);
    id::io::set_save_library(ID_TEST_SAVE_DIR);

    const Path path{id::io::find_wildcard_first(id::io::ReadFile::IMAGE, ID_TEST_SAVE_IMAGE_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_SAVE_IMAGE_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"image"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SAVE_DIR}, path.parent_path().parent_path()) << path;
}

TEST_F(TestLibrary, findWildcardSaveLibraryRootDir)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);
    id::io::set_save_library(ID_TEST_SAVE_DIR);

    const Path path{id::io::find_wildcard_first(id::io::ReadFile::IMAGE, ID_TEST_SAVE_IMAGE_ROOT_FILE)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(Path{ID_TEST_SAVE_IMAGE_ROOT_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{ID_TEST_SAVE_DIR}, path.parent_path()) << path;
}
