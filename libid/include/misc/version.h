// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

// major.minor.patch.tweak
// legacy is true when version read from a file or parameter set as a single integer
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

// String representation suitable for writing out as argument to reset parameter.
std::string to_par_string(const Version &value);

// String for displaying to the user.
std::string to_display_string(const Version &value);

// Parse legacy g_release style version.
Version parse_legacy_version(int version);

bool operator==(const Version &lhs, const Version &rhs);
inline bool operator!=(const Version &lhs, const Version &rhs)
{
    return !(lhs == rhs);
}
bool operator<(const Version &lhs, const Version &rhs);

inline bool operator>(const Version &lhs, const Version &rhs)
{
    return rhs < lhs;
}

inline bool operator<(const Version &lhs, int legacy_version)
{
    return lhs < parse_legacy_version(legacy_version);
}

inline bool operator<=(const Version &lhs, const Version &rhs)
{
    return lhs < rhs || lhs == rhs;
}
inline bool operator<=(const Version &lhs, int legacy_version)
{
    return lhs <= parse_legacy_version(legacy_version);
}
