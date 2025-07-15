// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/library.h"

#include <string>
#include <utility>
#include <vector>

namespace id::io
{

static std::vector<std::filesystem::path> s_search_path;

static std::string_view subdir(FileType kind)
{
    switch (kind)
    {
    case FileType::FORMULA:
        return "formula";
    case FileType::IFS:
        return "ifs";
    case FileType::IMAGE:
        return "image";
    case FileType::KEY:
        return "key";
    case FileType::LSYSTEM:
        return "lsystem";
    case FileType::MAP:
        return "map";
    case FileType::PARAMETER:
        return "par";
    }
    throw std::runtime_error("Unknown FileType " + std::to_string(static_cast<int>(kind)));
}

void clear_search_path()
{
    s_search_path.clear();
}

void add_library(std::filesystem::path path)
{
    s_search_path.emplace_back(std::move(path));
}

std::filesystem::path find_file(FileType kind, const std::filesystem::path &filename)
{
    for (const std::filesystem::path &dir : s_search_path)
    {
        if (const std::filesystem::path path = dir / subdir(kind) / filename; exists(path))
        {
            return path;
        }
    }
    for (const std::filesystem::path &dir : s_search_path)
    {
        if (const std::filesystem::path path = dir / filename; exists(path))
        {
            return path;
        }
    }
    return {};
}

} // namespace id::io
