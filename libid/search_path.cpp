// SPDX-License-Identifier: GPL-3.0-only
//
#include "search_path.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

std::string search_path(const char *filename, const char *path_var, std::function<const char *(const char *)> get_env)
{
    if (filename == nullptr || path_var == nullptr)
    {
        return {};
    }

    const char *path = get_env(path_var);
    if (path == nullptr)
    {
        return {};
    }

    std::vector<std::string> parts;
    split(parts, path, boost::algorithm::is_any_of(PATH_SEPARATOR));
    for (const fs::path dir : parts)
    {
        fs::path candidate{dir / filename};
        if (exists(candidate))
        {
            return candidate.make_preferred().string();
        }
    }

    return {};
}
