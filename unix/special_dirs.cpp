#include "special_dirs.h"

#include "id.h"

#include <unistd.h>

#include <array>
#include <cstdlib>
#include <stdexcept>

std::string get_executable_dir()
{
    char buffer[1024]{};
    int bytes_read = readlink("/proc/self/exe", buffer, std::size(buffer));
    if (bytes_read == -1)
    {
        throw std::runtime_error("Couldn't get exe path: " + std::to_string(errno));
    }
    return buffer;
}

std::string get_documents_dir()
{
    char buffer[MAX_PATH]{};
    const char *home = std::getenv("HOME");
    return home != nullptr ? home : getcwd(buffer, std::size(buffer));
}
