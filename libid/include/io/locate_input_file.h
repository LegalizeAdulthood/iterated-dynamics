// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

namespace id::io
{

// Return full path to filename, or empty string if not found.
std::string locate_input_file(const std::string &name);

} // namespace id::io
