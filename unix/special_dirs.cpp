// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/special_dirs.h"

#include <array>
#include <cstdlib>
#include <stdexcept>
#include <unistd.h>

namespace
{

enum
{
    BUFFER_SIZE = 1024
};

class PosixSpecialDirectories : public SpecialDirectories
{
public:
    PosixSpecialDirectories() = default;
    ~PosixSpecialDirectories() override = default;

    std::filesystem::path program_dir() const override;
    std::filesystem::path documents_dir() const override;
};

std::filesystem::path PosixSpecialDirectories::program_dir() const
{
    char buffer[BUFFER_SIZE]{};
    int bytes_read = readlink("/proc/self/exe", buffer, std::size(buffer));
    if (bytes_read == -1)
    {
        throw std::runtime_error("Couldn't get exe path: " + std::to_string(errno));
    }
    return std::filesystem::path{buffer}.parent_path();
}

std::filesystem::path PosixSpecialDirectories::documents_dir() const
{
    char buffer[BUFFER_SIZE]{};
    const char *home = std::getenv("HOME");
    return home != nullptr ? home : getcwd(buffer, std::size(buffer));
}

} // namespace

std::shared_ptr<SpecialDirectories> create_special_directories()
{
    return std::make_shared<PosixSpecialDirectories>();
}
