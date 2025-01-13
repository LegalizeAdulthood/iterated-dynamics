// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

//
// The filename limits were increased in Xfract 3.02. But alas,
// in this poor program that was originally developed on the
// nearly-brain-dead DOS operating system, quite a few things
// in the UI would break if file names were bigger than DOS 8-3
// names. So for now humor us and let's keep the names short.
//
enum
{
    MSG_LEN = 80,        // handy buffer size for messages
    MAX_PARAMS = 10      // maximum number of parameters
};

#define DEFAULT_ASPECT 0.75F         // Assumed overall screen dimensions, y/x

enum
{
    ITEM_NAME_LEN = 18, // max length of names in .frm/.l/.ifs/.fc
};

#define DEFAULT_FRACTAL_TYPE      ".gif"

#ifdef PI
#undef PI
#endif
#define PI 3.14159265358979323846
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
