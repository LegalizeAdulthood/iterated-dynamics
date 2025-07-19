// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/library.h"

#include "engine/id_data.h"
#include "io/special_dirs.h"

#include <string>
#include <utility>
#include <vector>

namespace id::io
{

static std::vector<std::filesystem::path> s_search_path;
static std::filesystem::path s_save_path;

static std::string_view subdir(ReadFile kind)
{
    switch (kind)
    {
    case ReadFile::FORMULA:
        return "formula";
    case ReadFile::IFS:
        return "ifs";
    case ReadFile::IMAGE:
        return "image";
    case ReadFile::KEY:
        return "key";
    case ReadFile::LSYSTEM:
        return "lsystem";
    case ReadFile::MAP:
        return "map";
    case ReadFile::PARAMETER:
        return "par";
    }
    throw std::runtime_error("Unknown ReadFile type " + std::to_string(static_cast<int>(kind)));
}

void clear_read_library_path()
{
    s_search_path.clear();
}

void add_read_library(std::filesystem::path path)
{
    s_search_path.emplace_back(std::move(path));
}

std::filesystem::path find_file(ReadFile kind, const std::filesystem::path &file_path)
{
    if (file_path.is_absolute() && std::filesystem::exists(file_path))
    {
        return file_path;
    }

    const std::filesystem::path filename{file_path.filename()};
    for (const std::filesystem::path &dir : s_search_path)
    {
        if (const std::filesystem::path path = dir / subdir(kind) / filename; std::filesystem::exists(path))
        {
            return path;
        }
    }

    for (const std::filesystem::path &dir : s_search_path)
    {
        if (const std::filesystem::path path = dir / filename; std::filesystem::exists(path))
        {
            return path;
        }
    }

    auto check_dir = [&](const std::filesystem::path &dir) -> std::filesystem::path
    {
        if (const std::filesystem::path path = dir / subdir(kind) / filename; std::filesystem::exists(path))
        {
            return path;
        }

        if (const std::filesystem::path path = dir / filename; std::filesystem::exists(path))
        {
            return path;
        }

        return {};
    };

    if (const std::filesystem::path path = check_dir(g_fractal_search_dir1); !path.empty())
    {
        return path;
    }

    if (const std::filesystem::path path = check_dir(g_fractal_search_dir2); !path.empty())
    {
        return path;
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

static std::string_view subdir(WriteFile kind)
{
    switch (kind)
    {
    case WriteFile::FORMULA:
        return "formula";
    case WriteFile::IFS:
        return "ifs";
    case WriteFile::IMAGE:
        return "image";
    case WriteFile::KEY:
        return "key";
    case WriteFile::LSYSTEM:
        return "lsystem";
    case WriteFile::MAP:
        return "map";
    case WriteFile::PARAMETER:
        return "par";
    case WriteFile::RAYTRACE: // no special subdir for raytrace output
    case WriteFile::ROOT:
        return {};
    }
    throw std::runtime_error("Unknown WriteFile type " + std::to_string(static_cast<int>(kind)));
}

std::filesystem::path get_save_path(WriteFile file, const std::string &filename)
{
    std::filesystem::path dir = (s_save_path.empty() ? g_save_dir : s_save_path) / subdir(file);
    if (!std::filesystem::exists(dir))
    {
        std::error_code ec;
        if (create_directories(dir, ec); ec)
        {
            return {};
        }
    }
    return dir / filename;
}

} // namespace id::io
