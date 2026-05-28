// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "io/library.h"

#include <filesystem>
#include <string>

namespace id::io
{

extern bool g_overwrite_file; // true if file overwrite allowed

void check_write_file(std::string &name, const char *ext);
std::filesystem::path get_checked_save_path(WriteFile kind, const std::filesystem::path &name);

} // namespace id::io
