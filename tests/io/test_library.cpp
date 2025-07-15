#include <io/library.h>

#include "test_library.h"

#include "io/special_dirs.h"
#include "misc/ValueSaver.h"

#include <gtest/gtest.h>

using Path = std::filesystem::path;

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

TEST_F(TestLibrary, findFormulaInLibraryDirectory)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);

    const Path path{id::io::find_file(id::io::FileType::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty());
    EXPECT_EQ(Path{"root.frm"}, path.filename());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR1}, path.parent_path());
}

TEST_F(TestLibrary, findFormulaInLibrarySubDirectory)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::find_file(id::io::FileType::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty());
    EXPECT_EQ(Path{"root.frm"}, path.filename());
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path());
}

TEST_F(TestLibrary, preferFormulaSubDirectory)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::find_file(id::io::FileType::FORMULA, "root.frm")};

    ASSERT_FALSE(path.empty());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path());
}

TEST_F(TestLibrary, findFormulaSearchMultiplePaths)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR3);
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);

    const Path path{id::io::find_file(id::io::FileType::FORMULA, ID_TEST_FRM_FILE)};

    ASSERT_FALSE(path.empty());
    EXPECT_EQ(Path{ID_TEST_FRM_FILE}, path.filename());
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR2}, path.parent_path().parent_path());
}

TEST_F(TestLibrary, findImageInLibraryDirectory)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR2);

    const Path path{id::io::find_file(id::io::FileType::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_FALSE(path.empty());
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename());
    EXPECT_EQ(Path{"image"}, path.parent_path().filename());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR2}, path.parent_path().parent_path());
}

TEST_F(TestLibrary, setSaveLibrary)
{
    id::io::set_save_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::get_save_path(id::io::FileType::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_FALSE(path.empty());
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename());
    EXPECT_EQ(Path{"image"}, path.parent_path().filename());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path());
}

TEST_F(TestLibrary, saveLibraryDefaultsToSaveDir)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_LIBRARY_DIR3};

    const Path path{id::io::get_save_path(id::io::FileType::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_FALSE(path.empty());
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename());
    EXPECT_EQ(Path{"image"}, path.parent_path().filename());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path());
}

TEST_F(TestLibrary, findFileWithRootNotAllowed)
{
    id::io::add_read_library(ID_TEST_LIBRARY_DIR1);

    const Path path{id::io::find_file(id::io::FileType::ROOT, "root.gif")};

    ASSERT_TRUE(path.empty());
}
