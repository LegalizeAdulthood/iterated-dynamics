// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

namespace id::engine
{

enum TextColors
{
    BLACK = 0,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN, // dirty yellow on cga
    WHITE,

    // use values below this for foreground only, they don't work background
    GRAY, // don't use this much - is black on cga
    LT_BLUE,
    LT_GREEN,
    LT_CYAN,
    LT_RED,
    LT_MAGENTA,
    YELLOW,
    LT_WHITE,
    INVERSE = 0x8000,
    BRIGHT = 0x4000
};
// and their use:
#define C_TITLE           (g_text_color[0] | BRIGHT)
#define C_TITLE_DEV       (g_text_color[1])
#define C_HELP_HDG        (g_text_color[2] | BRIGHT)
#define C_HELP_BODY       (g_text_color[3])
#define C_HELP_INSTR      (g_text_color[4])
#define C_HELP_LINK       (g_text_color[5] | BRIGHT)
#define C_HELP_CURLINK    (g_text_color[6] | INVERSE)
#define C_PROMPT_BKGRD    (g_text_color[7])
#define C_PROMPT_TEXT     (g_text_color[8])
#define C_PROMPT_LO       (g_text_color[9])
#define C_PROMPT_MED      (g_text_color[10])
#define C_PROMPT_HI       (g_text_color[11] | BRIGHT)
#define C_PROMPT_INPUT    (g_text_color[12] | INVERSE)
#define C_PROMPT_CHOOSE   (g_text_color[13] | INVERSE)
#define C_CHOICE_CURRENT  (g_text_color[14] | INVERSE)
#define C_CHOICE_SP_INSTR (g_text_color[15])
#define C_CHOICE_SP_KEYIN (g_text_color[16] | BRIGHT)
#define C_GENERAL_HI      (g_text_color[17] | BRIGHT)
#define C_GENERAL_MED     (g_text_color[18])
#define C_GENERAL_LO      (g_text_color[19])
#define C_GENERAL_INPUT   (g_text_color[20] | INVERSE)
#define C_DVID_BKGRD      (g_text_color[21])
#define C_DVID_HI         (g_text_color[22] | BRIGHT)
#define C_DVID_LO         (g_text_color[23])
#define C_STOP_ERR        (g_text_color[24] | BRIGHT)
#define C_STOP_INFO       (g_text_color[25] | BRIGHT)
#define C_TITLE_LOW       (g_text_color[26])
#define C_AUTH_DIV1       (g_text_color[27] | INVERSE)
#define C_AUTH_DIV2       (g_text_color[28] | INVERSE)
#define C_PRIMARY         (g_text_color[29])
#define C_CONTRIB         (g_text_color[30])

extern Byte                  g_text_color[31];

} // namespace id::engine
