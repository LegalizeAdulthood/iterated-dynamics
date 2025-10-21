// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

namespace id::io
{

extern bool g_overwrite_file; // true if file overwrite allowed

void check_write_file(std::string &name, const char *ext);

} // namespace id::io
