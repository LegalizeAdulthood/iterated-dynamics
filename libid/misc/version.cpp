// SPDX-License-Identifier: GPL-3.0-only
//
#include "misc/version.h"

#include <config/port_config.h>

// g_release is pushed/popped on the history stack to provide backward compatibility with previous
// behavior, so it can't be const.
int g_release{ID_VERSION_MAJOR * 100 + ID_VERSION_MINOR};

// g_patch_level can't be modified.
const int g_patch_level{ID_VERSION_PATCH};

Version g_version{ID_VERSION_MAJOR, ID_VERSION_MINOR, ID_VERSION_PATCH, ID_VERSION_TWEAK, false};
Version g_file_version{};

std::string to_par_string(const Version &value)
{
    std::string result{std::to_string(value.major) + '/' + std::to_string(value.minor)};
    if (value.patch)
    {
        result.append('/' + std::to_string(value.patch));
        if (value.tweak)
        {
            result.append('/' + std::to_string(value.tweak));
        }
    }
    else if (value.tweak)
    {
        result.append("/0/" + std::to_string(value.tweak));
    }
    return result;
}

Version parse_legacy_version(int version)
{
    Version result{};
    result.major = version / 100;
    result.minor = version % 100;
    result.legacy = true;
    return result;
}
