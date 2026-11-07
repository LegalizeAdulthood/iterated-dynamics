// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string_view>

namespace id::io
{

inline bool ends_with_slash(const std::string_view fl)
{
    const char sep{static_cast<char>(std::filesystem::path::preferred_separator)};
    return !fl.empty() && fl.back() == sep;
}

} // namespace id::io
