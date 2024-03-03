#include <expand_dirname.h>

#include <port.h>
#include <fractint.h>

#include "current_path_saver.h"
#include "test_data.h"

#include <gtest/gtest.h>

#include <cstring>

namespace fs = std::filesystem;

TEST(TestExpandDirName, basic)
{
    char dirname[FILE_MAX_PATH]{};
    current_path_saver saver(ID_TEST_DATA_DIR);
    std::strcpy(dirname, ID_TEST_DATA_SUBDIR_NAME);
    EXPECT_STREQ("C:", fs::path{ID_TEST_DATA_DIR}.root_name().make_preferred().string().c_str());
    char drive[FILE_MAX_PATH];
    std::strcpy(drive, fs::path{ID_TEST_DATA_DIR}.root_name().string().c_str());

    expand_dirname(dirname, drive);

    EXPECT_EQ(drive, fs::path{ID_TEST_DATA_SUBDIR}.root_name().make_preferred().string());
    EXPECT_EQ(dirname, fs::path{ID_TEST_DATA_SUBDIR}.make_preferred().string().substr(2) + static_cast<char>(fs::path::preferred_separator));
}
