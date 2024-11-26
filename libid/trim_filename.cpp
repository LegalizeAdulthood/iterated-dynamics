// SPDX-License-Identifier: GPL-3.0-only
//
#include "trim_filename.h"

#include <filesystem>
#include <numeric>
#include <vector>

namespace fs = std::filesystem;

std::string trim_file_name(const std::string &file_name, int length)
{
    if (static_cast<int>(file_name.length()) <= length)
    {
        return file_name;
    }

    fs::path path{file_name, fs::path::format::generic_format};
    std::vector<fs::path> parts;
    while (path.has_parent_path() && path.parent_path() != path)
    {
        parts.push_back(path.filename());
        path = path.parent_path();
    }
    fs::path start{path / parts.back() / "..."};
    parts.pop_back();
    const auto path_length = [](int len, const fs::path &item) { return static_cast<int>(item.filename().string().size()) + len + 1; };
    const int remaining{length - static_cast<int>(start.string().size())};
    while (std::accumulate(parts.begin(), parts.end(), 0, path_length) > remaining)
    {
        parts.pop_back();
    }
    const auto join_paths = [](const fs::path &result, const fs::path &item) { return result / item; };
    return std::accumulate(parts.rbegin(), parts.rend(), start, join_paths).make_preferred().string();
}
