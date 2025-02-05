// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

// major.minor.patch.tweak
// legacy is true when FRACTINT version read from a file or parameter set
struct Version
{
    int major;
    int minor;
    int patch;
    int tweak;
    bool legacy;
};

extern const int             g_patch_level;
extern int                   g_release;
extern Version               g_version;

// String representation suitable for writing out as argument to reset parameter
std::string to_par_string(const Version &value);

// Parse legacy FRACTINT style version from file.
Version parse_legacy_version(int version);

inline bool operator<(const Version &lhs, const Version &rhs)
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
inline bool operator>(const Version &lhs, const Version &rhs)
{
    return rhs < lhs;
}
