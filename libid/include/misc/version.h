// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

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
