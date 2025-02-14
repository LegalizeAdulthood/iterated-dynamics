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

std::string to_display_string(const Version &value)
{
    if (value.legacy)
    {
        return std::string{"FRACTINT v"} + std::to_string(value.major) + '.' +
            std::to_string(value.minor + 100).substr(1);
    }
    std::string result{"Id v"};
    result.append(std::to_string(value.major) + '.' + std::to_string(value.minor));
    if (value.patch)
    {
        result.append('.' + std::to_string(value.patch));
        if (value.tweak)
        {
            result.append('.' + std::to_string(value.tweak));
        }
    }
    else if (value.tweak)
    {
        result.append(".0." + std::to_string(value.tweak));
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

bool operator<(const Version &lhs, const Version &rhs)
{
    if (lhs.legacy)
    {
        if (!rhs.legacy)
        {
            return true;
        }
    }
    else if (rhs.legacy)
    {
        return false;
    }
    // now both lhs and rhs are either both legacy or both modern
    if (lhs.major < rhs.major)
    {
        return true;
    }
    if (lhs.major == rhs.major)
    {
        if (lhs.minor < rhs.minor)
        {
            return true;
        }
        if (lhs.minor == rhs.minor)
        {
            if (lhs.patch < rhs.patch)
            {
                return true;
            }
            if (lhs.patch == rhs.patch && lhs.tweak < rhs.tweak)
            {
                return true;
            }
        }
    }

    return false;
}
