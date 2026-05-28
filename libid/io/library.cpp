// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/library.h"

#include "io/find_file.h"
#include "io/special_dirs.h"

#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace id::io
{

bool g_check_cur_dir{};
std::filesystem::path g_fractal_search_dir1;
std::filesystem::path g_fractal_search_dir2;

using PathList = std::vector<fs::path>;

struct WildcardSearch
{
    ReadFile kind;
    std::vector<fs::path> patterns;
    std::size_t pattern{};
    bool active{};
};

static PathList s_read_libraries;
static fs::path s_save_library;
static WildcardSearch s_wildcard;

static std::string_view subdir(ReadFile kind)
{
    switch (kind)
    {
    case ReadFile::ID_CONFIG:
        return "config";
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
    s_read_libraries.clear();
}

void add_read_library(fs::path path)
{
    s_read_libraries.emplace_back(std::move(path));
}

fs::path find_file(const ReadFile kind, const fs::path &file_path)
{
    if (file_path.is_absolute() && fs::exists(file_path))
    {
        return file_path;
    }

    const fs::path filename{file_path.filename()};
    const auto check_dir = [&](const fs::path &dir) -> fs::path
    {
        if (const fs::path path = dir / subdir(kind) / filename; fs::exists(path))
        {
            return path;
        }

        if (const fs::path path = dir / filename; fs::exists(path))
        {
            return path;
        }

        return {};
    };

    if (g_check_cur_dir)
    {
        if (const fs::path path{check_dir("")}; !path.empty())
        {
            return path;
        }
    }

    for (const fs::path &dir : s_read_libraries)
    {
        if (const fs::path path = dir / subdir(kind) / filename; fs::exists(path))
        {
            return path;
        }
    }

    for (const fs::path &dir : s_read_libraries)
    {
        if (const fs::path path = dir / filename; fs::exists(path))
        {
            return path;
        }
    }

    if (const fs::path path = check_dir(s_save_library); !path.empty())
    {
        return path;
    }

    if (const fs::path path = check_dir(g_fractal_search_dir1); !path.empty())
    {
        return path;
    }

    if (const fs::path path = check_dir(g_fractal_search_dir2); !path.empty())
    {
        return path;
    }

    return {};
}

void clear_save_library()
{
    s_save_library.clear();
}

void set_save_library(fs::path path)
{
    s_save_library = std::move(path);
}

static std::string_view subdir(WriteFile kind)
{
    switch (kind)
    {
    case WriteFile::ID_CONFIG:
        return "config";
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
    case WriteFile::ORBIT:
        return "orbit";
    case WriteFile::PARAMETER:
        return "par";
    case WriteFile::SOUND:
        return "sound";
    case WriteFile::RAYTRACE:
        return "raytrace";
    case WriteFile::ROOT:
        return {};
    }
    throw std::runtime_error("Unknown WriteFile type " + std::to_string(static_cast<int>(kind)));
}

static const char *file_extension(WriteFile kind)
{
    switch (kind)
    {
    case WriteFile::FORMULA:
        return ".frm";
    case WriteFile::IFS:
        return ".ifs";
    case WriteFile::IMAGE:
        return ".gif";
    case WriteFile::KEY:
        return ".key";
    case WriteFile::LSYSTEM:
        return ".l";
    case WriteFile::MAP:
        return ".map";
    case WriteFile::PARAMETER:
        return ".par";
    case WriteFile::ROOT:
        return "";
    case WriteFile::RAYTRACE:
        return ".ray";
    case WriteFile::ID_CONFIG:
        return ".cfg";
    case WriteFile::ORBIT:
        return ".raw";
    case WriteFile::SOUND:
        return ".txt";
    }

    throw std::runtime_error("Unknown WriteFile type " + std::to_string(static_cast<int>(kind)));
}

fs::path get_save_path(const WriteFile kind, const std::string &filename)
{
    fs::path result = (s_save_library.empty() ? g_save_dir : s_save_library) / subdir(kind);
    if (!fs::exists(result))
    {
        std::error_code ec;
        if (create_directories(result, ec); ec)
        {
            return {};
        }
    }
    result /= filename;
    if (!result.has_extension())
    {
        result.replace_extension(file_extension(kind));
    }
    return result;
}

static fs::path find_next_file_in_pattern()
{
    if (!fr_find_next())
    {
        return {};
    }
    while (g_dta.attribute == SUB_DIR)
    {
        if (!fr_find_next())
        {
            return {};
        }
    }
    return g_dta.path;
}

fs::path find_wildcard_next()
{
    if (s_wildcard.active)
    {
        if (const fs::path path{find_next_file_in_pattern()}; !path.empty())
        {
            return path;
        }
        s_wildcard.active = false;
        ++s_wildcard.pattern;
    }

    while (s_wildcard.pattern < s_wildcard.patterns.size())
    {
        const std::string pattern{s_wildcard.patterns[s_wildcard.pattern].string()};
        if (!fr_find_first(pattern.c_str()))
        {
            ++s_wildcard.pattern;
            continue;
        }
        s_wildcard.active = true;
        while (g_dta.attribute == SUB_DIR)
        {
            if (!fr_find_next())
            {
                break;
            }
        }
        if (g_dta.attribute != SUB_DIR)
        {
            return g_dta.path;
        }
        s_wildcard.active = false;
        ++s_wildcard.pattern;
    }

    return {};
}

fs::path find_wildcard_first(const ReadFile kind, const std::string &wildcard)
{
    s_wildcard.kind = kind;
    s_wildcard.patterns.clear();
    s_wildcard.pattern = 0;
    s_wildcard.active = false;

    const fs::path wildcard_path{wildcard};
    if (wildcard_path.has_parent_path())
    {
        s_wildcard.patterns.push_back(wildcard_path);
    }
    else
    {
        if (g_check_cur_dir)
        {
            s_wildcard.patterns.push_back(fs::path{subdir(kind)} / wildcard_path);
            s_wildcard.patterns.push_back(wildcard_path);
        }
        for (const fs::path &dir : s_read_libraries)
        {
            s_wildcard.patterns.push_back(dir / subdir(kind) / wildcard_path);
        }
        for (const fs::path &dir : s_read_libraries)
        {
            s_wildcard.patterns.push_back(dir / wildcard_path);
        }
        if (!s_save_library.empty())
        {
            s_wildcard.patterns.push_back(s_save_library / subdir(kind) / wildcard_path);
            s_wildcard.patterns.push_back(s_save_library / wildcard_path);
        }
    }

    while (s_wildcard.pattern < s_wildcard.patterns.size())
    {
        const std::string pattern{s_wildcard.patterns[s_wildcard.pattern].string()};
        if (!fr_find_first(pattern.c_str()))
        {
            ++s_wildcard.pattern;
            continue;
        }
        s_wildcard.active = true;
        while (g_dta.attribute == SUB_DIR)
        {
            if (!fr_find_next())
            {
                break;
            }
        }
        if (g_dta.attribute != SUB_DIR)
        {
            return g_dta.path;
        }
        s_wildcard.active = false;
        ++s_wildcard.pattern;
    }

    return {};
}

} // namespace id::io
