#pragma once

#include <filesystem>

inline bool isadirectory(const char *s)
{
    return std::filesystem::is_directory(s);
}
