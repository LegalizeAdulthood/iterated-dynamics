#pragma once

#include <filesystem>
#include <string>

inline std::string extract_filename(char const *source)
{
    return std::filesystem::path(source).filename().string();
}
