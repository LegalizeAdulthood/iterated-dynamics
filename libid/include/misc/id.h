// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::misc
{

enum
{
    MSG_LEN = 80,        // handy buffer size for messages
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

} // namespace id::misc
