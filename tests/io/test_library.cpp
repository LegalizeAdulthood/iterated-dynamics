#include <io/library.h>

#include "test_library.h"

#include <gtest/gtest.h>

using Path = std::filesystem::path;

TEST(TestLibrary, findFormulaInLibraryDirectory)
{
    id::io::clear_search_path();
    id::io::add_library(ID_TEST_LIBRARY_DIR1);

    const Path path{id::io::find_file(id::io::FileType::FORMULA, "root.frm")};

    ASSERT_TRUE(!path.empty());
    EXPECT_EQ(Path{"root.frm"}, path.filename());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR1}, path.parent_path());
}

TEST(TestLibrary, findFormulaInLibrarySubDirectory)
{
    id::io::clear_search_path();
    id::io::add_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::find_file(id::io::FileType::FORMULA, "root.frm")};

    ASSERT_TRUE(!path.empty());
    EXPECT_EQ(Path{"root.frm"}, path.filename());
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path());
}

TEST(TestLibrary, preferFormulaSubDirectory)
{
    id::io::clear_search_path();
    id::io::add_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_library(ID_TEST_LIBRARY_DIR3);

    const Path path{id::io::find_file(id::io::FileType::FORMULA, "root.frm")};

    ASSERT_TRUE(!path.empty());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path());
}

TEST(TestLibrary, findFormulaSearchMultiplePaths)
{
    id::io::clear_search_path();
    id::io::add_library(ID_TEST_LIBRARY_DIR1);
    id::io::add_library(ID_TEST_LIBRARY_DIR3);
    id::io::add_library(ID_TEST_LIBRARY_DIR2);

    const Path path{id::io::find_file(id::io::FileType::FORMULA, ID_TEST_FRM_FILE)};

    ASSERT_TRUE(!path.empty());
    EXPECT_EQ(Path{ID_TEST_FRM_FILE}, path.filename());
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR2}, path.parent_path().parent_path());
}

TEST(TestLibrary, findImageInLibraryDirectory)
{
    id::io::clear_search_path();
    id::io::add_library(ID_TEST_LIBRARY_DIR2);

    const Path path{id::io::find_file(id::io::FileType::IMAGE, ID_TEST_IMAGE_FILE)};

    ASSERT_TRUE(!path.empty());
    EXPECT_EQ(Path{ID_TEST_IMAGE_FILE}, path.filename());
    EXPECT_EQ(Path{"image"}, path.parent_path().filename());
    EXPECT_EQ(Path{ID_TEST_LIBRARY_DIR2}, path.parent_path().parent_path());
}
