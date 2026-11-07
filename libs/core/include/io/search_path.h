// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdlib>
#include <functional>
#include <string>

namespace id::io
{

#ifdef WIN32
constexpr const char *const PATH_SEPARATOR{";"};
#else
constexpr const char *const PATH_SEPARATOR{":"};
#endif

using GetEnv = std::function<const char *(const char *)>;

std::string search_path(const char *filename, const char *path_var, const GetEnv &get_env);

inline std::string search_path(const char * filename, const char *path_var)
{
    return search_path(filename, path_var, [](const char *var) -> const char * { return std::getenv(var); });
}

} // namespace id::io
