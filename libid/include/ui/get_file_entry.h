// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "io/file_item.h"

#include <filesystem>
#include <string>

namespace id::ui
{

long get_file_entry(io::ItemType type, std::filesystem::path &path, std::string &entry_name);

} // namespace id::ui
