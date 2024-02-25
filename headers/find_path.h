#pragma once

#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

std::string find_path(const char *filename, const std::function<const char *(const char *)> &get_env);

inline std::string find_path(const char *filename)
{
    return find_path(filename, [](const char *var) -> const char * { return std::getenv(var); });
}

inline void findpath(char const *filename, char *fullpathname)
{
    const std::string result = find_path(filename);
    std::strcpy(fullpathname, result.c_str());
}
