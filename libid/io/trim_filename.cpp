// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/trim_filename.h"

#include <cstddef>
#include <filesystem>
#include <numeric>
#include <vector>

namespace fs = std::filesystem;

namespace id::io
{

std::string trim_filename(const std::string &filename, const int length)
{
    if (length <= 0)
    {
        return {};
    }
    if (static_cast<int>(filename.length()) <= length)
    {
        return filename;
    }

    fs::path path{filename, fs::path::format::generic_format};
    std::vector<fs::path> parts;
    while (path.has_parent_path() && path.parent_path() != path)
    {
        parts.push_back(path.filename());
        path = path.parent_path();
    }
    if (parts.empty())
    {
        constexpr char ellipsis[]{"..."};
        constexpr std::size_t ellipsis_len{sizeof(ellipsis) - 1};
        const std::size_t result_len{static_cast<std::size_t>(length)};
        if (result_len <= ellipsis_len)
        {
            return std::string{ellipsis, result_len};
        }
        const std::size_t keep_len{result_len - ellipsis_len};
        const std::size_t prefix_len{keep_len / 2};
        const std::size_t suffix_len{keep_len - prefix_len};
        return filename.substr(0, prefix_len) + ellipsis + filename.substr(filename.length() - suffix_len);
    }
    const fs::path start{path / parts.back() / "..."};
    parts.pop_back();
    const auto path_length = [](const int len, const fs::path &item)
    { return static_cast<int>(item.filename().string().size()) + len + 1; };
    const int remaining{length - static_cast<int>(start.string().size())};
    while (std::accumulate(parts.begin(), parts.end(), 0, path_length) > remaining)
    {
        parts.pop_back();
    }
    const auto join_paths = [](const fs::path &result, const fs::path &item) { return result / item; };
    return std::accumulate(parts.rbegin(), parts.rend(), start, join_paths).make_preferred().string();
}

} // namespace id::io
