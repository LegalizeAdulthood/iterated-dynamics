// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/library.h"

#include "io/special_dirs.h"

#include <string>
#include <utility>
#include <vector>

namespace id::io
{

static std::vector<std::filesystem::path> s_search_path;
static std::filesystem::path s_save_path;

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
    case FileType::ROOT:
        return ".";
    }
    throw std::runtime_error("Unknown FileType " + std::to_string(static_cast<int>(kind)));
}

void clear_read_library_path()
{
    s_search_path.clear();
}

void add_read_library(std::filesystem::path path)
{
    s_search_path.emplace_back(std::move(path));
}

std::filesystem::path find_file(FileType kind, const std::filesystem::path &filename)
{
    // ROOT is for special case output only, e.g. makedoc or makepar batch files.
    if (kind == FileType::ROOT)
    {
        return {};
    }

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

void clear_save_library()
{
    s_save_path.clear();
}

void set_save_library(std::filesystem::path path)
{
    s_save_path = std::move(path);
}

std::filesystem::path get_save_path(FileType file, const std::string &filename)
{
    return (s_save_path.empty() ? g_save_dir : s_save_path) / subdir(file) / filename;
}

} // namespace id::io
