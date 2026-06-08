// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/search_path.h"

#include <algos/string_algorithms.h>

#include <filesystem>

namespace fs = std::filesystem;

using namespace id::algos;

namespace id::io
{

std::string search_path(const char *filename, const char *path_var, const GetEnv &get_env)
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

    for (const fs::path dir : split_any(path, PATH_SEPARATOR))
    {
        fs::path candidate{dir / filename};
        if (fs::exists(candidate))
        {
            return candidate.make_preferred().string();
        }
    }

    return {};
}

} // namespace id::io
