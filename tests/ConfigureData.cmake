# SPDX-License-Identifier: GPL-3.0-only
#
set(ID_TEST_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_data")
set(ID_TEST_HOME_DIR "${ID_TEST_DATA_DIR}/home")
set(ID_TEST_MAP_SUBDIR "maps")
set(ID_TEST_MAP_DIR "${ID_TEST_HOME_DIR}/${ID_TEST_MAP_SUBDIR}")

# id.cfg test data
set(ID_TEST_GDI_FN "F6")
set(ID_TEST_GDI_FN_KEY "ID_KEY_${ID_TEST_GDI_FN}")
set(ID_TEST_GDI_WIDTH 800)
set(ID_TEST_GDI_HEIGHT 600)
set(ID_TEST_GDI_COLORS 256)
set(ID_TEST_GDI_COMMENT "fancy GDI mode")
set(ID_TEST_GDI_DRIVER "gdi")

set(ID_TEST_DISK_FN "SF8")
set(ID_TEST_DISK_FN_KEY "ID_KEY_${ID_TEST_DISK_FN}")
set(ID_TEST_DISK_WIDTH 1024)
set(ID_TEST_DISK_HEIGHT 768)
set(ID_TEST_DISK_COLORS 256)
set(ID_TEST_DISK_COMMENT "fancier disk mode")
set(ID_TEST_DISK_DRIVER "disk")

set(ID_TEST_CONFIG_FILE "${ID_TEST_HOME_DIR}/test.cfg")
configure_file("test.cfg.in" "${ID_TEST_CONFIG_FILE}" @ONLY)
configure_file("test_config_data.h.in" "include/test_config_data.h" @ONLY)

# GIF file extensions test data
set(ID_TEST_GIF_DIR "${ID_TEST_DATA_DIR}/gif")
macro(configure_gif_extension_file num)
    set("EXT${num}_FILE" "extension${num}.gif")
    set("ID_TEST_GIF_EXT${num}_FILE" "${ID_TEST_GIF_DIR}/${EXT${num}_FILE}")
    configure_file("gif/${EXT${num}_FILE}" "${ID_TEST_GIF_EXT${num}_FILE}" COPYONLY)
    set("ID_TEST_GIF_WRITE${num}_FILE" "${ID_TEST_GIF_DIR}/output${num}.gif")
endmacro()
configure_gif_extension_file(1)
configure_gif_extension_file(2)
configure_gif_extension_file(3)
configure_gif_extension_file(4)
configure_gif_extension_file(5)
configure_gif_extension_file(6)
configure_gif_extension_file(7)

# IFS test data
set(ID_TEST_FIRST_IFS_NAME "binary")
set(ID_TEST_FIRST_IFS_PARAM1 ".5")
set(ID_TEST_SECOND_IFS_NAME "3dfern")
set(ID_TEST_IFS_FILE "test.ifs")
configure_file(test.ifs.in "${ID_TEST_DATA_DIR}/${ID_TEST_IFS_FILE}")

set(ID_TEST_DATA_SUBDIR_NAME    "subdir")
set(ID_TEST_DATA_SUBDIR         "${ID_TEST_DATA_DIR}/${ID_TEST_DATA_SUBDIR_NAME}")
set(ID_TEST_IFS_FILE2           "test2.ifs")
configure_file(test.ifs.in      "${ID_TEST_DATA_SUBDIR}/${ID_TEST_IFS_FILE2}")

# find_file test data
set(ID_TEST_FIND_FILE_SUBDIR            "test_data/find_file")
set(ID_TEST_FIND_FILE_CASEDIR           "casedir")
set(ID_TEST_DATA_FIND_FILE_DIR          "${CMAKE_CURRENT_BINARY_DIR}/${ID_TEST_FIND_FILE_SUBDIR}")
set(ID_TEST_FIND_FILE1                  "find_file1.txt")
set(ID_TEST_FIND_FILE2                  "find_file2.txt")
set(ID_TEST_FIND_FILE3                  "find_file3.txt")
set(ID_TEST_FIND_FILE_CASE_FILENAME     "FIND_FILE4")
set(ID_TEST_FIND_FILE_CASE              "${ID_TEST_FIND_FILE_CASE_FILENAME}.TXT")
configure_file(test_find_file.txt.in    "${ID_TEST_FIND_FILE_SUBDIR}/${ID_TEST_FIND_FILE1}")
configure_file(test_find_file.txt.in    "${ID_TEST_FIND_FILE_SUBDIR}/${ID_TEST_FIND_FILE2}")
configure_file(test_find_file.txt.in    "${ID_TEST_FIND_FILE_SUBDIR}/${ID_TEST_DATA_SUBDIR_NAME}/${ID_TEST_FIND_FILE3}")
configure_file(test_find_file.txt.in    "${ID_TEST_FIND_FILE_SUBDIR}/${ID_TEST_FIND_FILE_CASEDIR}/${ID_TEST_FIND_FILE_CASE}")

# check_writefile test data
set(ID_TEST_CHECK_WRITE_FILE_DATA_DIR   "${ID_TEST_DATA_DIR}/check_writefile")
set(ID_TEST_CHECK_WRITE_FILE_NEW        "${ID_TEST_CHECK_WRITE_FILE_DATA_DIR}/new_file.gif")
set(ID_TEST_CHECK_WRITE_FILE_BASE1      "${ID_TEST_CHECK_WRITE_FILE_DATA_DIR}/fract001")
set(ID_TEST_CHECK_WRITE_FILE_EXT        ".gif")
set(ID_TEST_CHECK_WRITE_FILE_OTHER_EXT  ".foo")
set(ID_TEST_CHECK_WRITE_FILE_EXISTS1    "${ID_TEST_CHECK_WRITE_FILE_DATA_DIR}/fract001.gif")
set(ID_TEST_CHECK_WRITE_FILE_EXISTS2    "${ID_TEST_CHECK_WRITE_FILE_DATA_DIR}/fract002.gif")
set(ID_TEST_CHECK_WRITE_FILE3           "${ID_TEST_CHECK_WRITE_FILE_DATA_DIR}/fract003.gif")
configure_file(test_check_write_file_data.h.in "include/test_check_write_file_data.h" @ONLY)
configure_file("gif/extension1.gif"     "${ID_TEST_CHECK_WRITE_FILE_DATA_DIR}/fract001.gif" COPYONLY)
configure_file("gif/extension1.gif"     "${ID_TEST_CHECK_WRITE_FILE_DATA_DIR}/fract002.gif" COPYONLY)
file(REMOVE "${ID_TEST_CHECK_WRITE_FILE_NEW}")
file(REMOVE "${ID_TEST_CHECK_WRITE_FILE3}")

# color map file test data
set(ID_TEST_MAP_FILE "test.map")
configure_file(test.map.in "${ID_TEST_MAP_DIR}/test.map" @ONLY)

# Test data header
configure_file(test_data.h.in include/test_data.h @ONLY)
