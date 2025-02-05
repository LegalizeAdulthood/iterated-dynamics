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
