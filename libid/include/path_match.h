#pragma once

#include <filesystem>
#include <functional>

using MatchFn = std::function<bool(const std::filesystem::path &)>;

MatchFn match_fn(const std::filesystem::path &pattern);
