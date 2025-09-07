// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/get_disk_space.h"

#include <filesystem>
#include <limits>

namespace fs = std::filesystem;

namespace id::io
{

unsigned long get_disk_space()
{
    const fs::space_info info{fs::space(fs::current_path())};
    const std::uintmax_t available{info.available};
    if (available > std::numeric_limits<unsigned long>::max())
    {
        return std::numeric_limits<unsigned long>::max();
    }
    return static_cast<unsigned long>(available);
}

} // namespace id::io
