// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// structure passed to fullscreen_prompts
struct FullScreenValues
{
    int type;   // 'd' for double, 'f' for float, 's' for string,
    // 'D' for integer in double, '*' for comment
    // 'i' for integer, 'y' for yes=1 no=0
    // 0x100+n for string of length n
    // 'l' for one of a list of strings
    // 'L' for long
    union
    {
        double dval;        // when type 'd' or 'f'
        int    ival;        // when type is 'i'
        long   Lval;        // when type is 'L'
        char   sval[16];    // when type is 's'
        char  *sbuf;        // when type is 0x100+n
        struct              // when type is 'l'
        {
            int  val;       // selected choice
            int  vlen;      // char len per choice
            char const **list;  // list of values
            int  llen;      // number of values
        } ch;
    } uval;
};

int full_screen_prompt(
    char const *hdg,
    int num_prompts,
    char const **prompts,
    FullScreenValues *values,
    int fn_key_mask,
    char *extra_info);

void full_screen_reset_scrolling();
