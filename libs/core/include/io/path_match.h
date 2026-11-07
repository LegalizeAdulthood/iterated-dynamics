// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <functional>

namespace id::io
{

using MatchFn = std::function<bool(const std::filesystem::path &)>;

MatchFn match_fn(const std::filesystem::path &pattern);

} // namespace id::io
