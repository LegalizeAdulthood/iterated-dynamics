// SPDX-License-Identifier: GPL-3.0-only
//
// Command-line / Command-File Parser Routines
//
#include "engine/text_color.h"

#include <iterator>

namespace id::engine
{

Byte g_text_color[31] = {
    BLUE * 16 + LT_WHITE,    // C_TITLE           title background
    BLUE * 16 + LT_GREEN,    // C_TITLE_DEV       development vsn foreground
    GREEN * 16 + YELLOW,     // C_HELP_HDG        help page title line
    WHITE * 16 + BLACK,      // C_HELP_BODY       help page body
    GREEN * 16 + GRAY,       // C_HELP_INSTR      help page instr at bottom
    WHITE * 16 + BLUE,       // C_HELP_LINK       help page links
    CYAN * 16 + BLUE,        // C_HELP_CURLINK    help page current link
    WHITE * 16 + GRAY,       // C_PROMPT_BKGRD    prompt/choice background
    WHITE * 16 + BLACK,      // C_PROMPT_TEXT     prompt/choice extra info
    BLUE * 16 + WHITE,       // C_PROMPT_LO       prompt/choice text
    BLUE * 16 + LT_WHITE,    // C_PROMPT_MED      prompt/choice hdg2/...
    BLUE * 16 + YELLOW,      // C_PROMPT_HI       prompt/choice hdg/cur/...
    GREEN * 16 + LT_WHITE,   // C_PROMPT_INPUT    fullscreen_prompt input
    CYAN * 16 + LT_WHITE,    // C_PROMPT_CHOOSE   fullscreen_prompt choice
    MAGENTA * 16 + LT_WHITE, // C_CHOICE_CURRENT  fullscreen_choice input
    BLACK * 16 + WHITE,      // C_CHOICE_SP_INSTR speed key bar & instr
    BLACK * 16 + LT_MAGENTA, // C_CHOICE_SP_KEYIN speed key value
    WHITE * 16 + BLUE,       // C_GENERAL_HI      tab, thinking, IFS
    WHITE * 16 + BLACK,      // C_GENERAL_MED
    WHITE * 16 + GRAY,       // C_GENERAL_LO
    BLACK * 16 + LT_WHITE,   // C_GENERAL_INPUT
    WHITE * 16 + BLACK,      // C_DVID_BKGRD      disk video
    BLACK * 16 + YELLOW,     // C_DVID_HI
    BLACK * 16 + LT_WHITE,   // C_DVID_LO
    RED * 16 + LT_WHITE,     // C_STOP_ERR        stop message, error
    GREEN * 16 + BLACK,      // C_STOP_INFO       stop message, info
    BLUE * 16 + WHITE,       // C_TITLE_LOW       bottom lines of title screen
    GREEN * 16 + BLACK,      // C_AUTH_DIV1       title screen dividers
    GREEN * 16 + GRAY,       // C_AUTH_DIV2       title screen dividers
    BLACK * 16 + LT_WHITE,   // C_PRIMARY         primary authors
    BLACK * 16 + WHITE       // C_CONTRIB         contributing authors
};
static_assert(std::size(g_text_color) == 31);

} // namespace id::engine
