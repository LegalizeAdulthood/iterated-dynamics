#pragma once

#include "port.h"

#include "fractint.h"

#include "cmdfiles.h"

// copies the proposed new filename to the fullpath variable
// does not copy directories for PAR files
// (modes AT_AFTER_STARTUP and AT_CMD_LINE_SET_NAME)
// attempts to extract directory and test for existence
// (modes AT_CMD_LINE and SSTOOLS_INI)
int merge_pathnames(char *oldfullpath, char const *new_filename, cmd_file mode);
int merge_pathnames(std::string &oldfullpath, char const *new_filename, cmd_file mode);
