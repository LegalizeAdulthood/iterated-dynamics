// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

enum class CmdFile;

// copies the proposed new filename to the fullpath variable
// does not copy directories for PAR files
// (modes AT_AFTER_STARTUP and AT_CMD_LINE_SET_NAME)
// attempts to extract directory and test for existence
// (modes AT_CMD_LINE and SSTOOLS_INI)
int merge_path_names(char *old_full_path, char const *new_filename, CmdFile mode);
int merge_path_names(std::string &old_full_path, char const *new_filename, CmdFile mode);
