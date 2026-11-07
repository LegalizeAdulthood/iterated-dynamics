// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>

namespace id::io
{

inline bool has_ext(const std::filesystem::path &source)
{
    return source.has_extension();
}

} // namespace id::io
