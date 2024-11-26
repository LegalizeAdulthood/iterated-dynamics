// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "cmdfiles.h"

// copies the proposed new filename to the fullpath variable
// does not copy directories for PAR files
// (modes AT_AFTER_STARTUP and AT_CMD_LINE_SET_NAME)
// attempts to extract directory and test for existence
// (modes AT_CMD_LINE and SSTOOLS_INI)
int merge_path_names(char *oldfullpath, char const *new_filename, cmd_file mode);
int merge_path_names(std::string &oldfullpath, char const *new_filename, cmd_file mode);
