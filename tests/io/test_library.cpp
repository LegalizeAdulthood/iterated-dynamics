#include <io/library.h>

#include "test_library.h"

#include "engine/id_data.h"
#include "io/special_dirs.h"
#include "misc/ValueSaver.h"

#include <gtest/gtest.h>

using Path = std::filesystem::path;

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
    auto file_path{std::filesystem::path{ID_TEST_LIBRARY_DIR} / ID_TEST_FRM_FILE};

    const Path path{id::io::find_file(id::io::ReadFile::FORMULA, file_path)};

    ASSERT_FALSE(path.empty()) << path;
    EXPECT_EQ(ID_TEST_FRM_FILE, path.filename()) << path;
    EXPECT_EQ(ID_TEST_LIBRARY_DIR, path.parent_path()) << path;
    EXPECT_EQ(file_path, path) << path;
}
