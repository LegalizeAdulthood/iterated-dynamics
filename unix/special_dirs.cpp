// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/special_dirs.h"

#include <unistd.h>

#include <array>
#include <cstdlib>
#include <stdexcept>

enum
{
    BUFFER_SIZE = 1024
};

std::filesystem::path get_executable_dir()
{
    char buffer[BUFFER_SIZE]{};
    int bytes_read = readlink("/proc/self/exe", buffer, std::size(buffer));
    if (bytes_read == -1)
    {
        throw std::runtime_error("Couldn't get exe path: " + std::to_string(errno));
    }
    return std::filesystem::path{buffer}.parent_path();
}

std::filesystem::path get_documents_dir()
{
    char buffer[BUFFER_SIZE]{};
    const char *home = std::getenv("HOME");
    return home != nullptr ? home : getcwd(buffer, std::size(buffer));
}
