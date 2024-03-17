#pragma once

#include <cstdlib>
#include <functional>
#include <string>

#ifdef WIN32
constexpr const char *const PATH_SEPARATOR{";"};
#else
constexpr const char *const PATH_SEPARATOR{":"};
#endif

std::string search_path(const char *filename, const char *path_var, std::function<const char *(const char *)> get_env);

inline std::string search_path(const char * filename, const char *path_var)
{
    return search_path(filename, path_var, [](const char *var) -> const char * { return std::getenv(var); });
}
