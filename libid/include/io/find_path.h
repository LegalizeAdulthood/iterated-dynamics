// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdlib>
#include <functional>
#include <string>

std::string find_path(const char *filename, const std::function<const char *(const char *)> &get_env);

inline std::string find_path(const char *filename)
{
    return find_path(filename, [](const char *var) -> const char * { return std::getenv(var); });
}
