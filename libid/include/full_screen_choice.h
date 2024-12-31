// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

extern std::string const     g_speed_prompt;

// fullscreen_choice options
enum class ChoiceFlags
{
    NONE         = 0,
    MENU         = 2,
    HELP         = 4,
    INSTRUCTIONS = 8,
    CRUNCH       = 16,
    NOT_SORTED   = 32
};
inline int operator+(ChoiceFlags value)
{
    return static_cast<int>(value);
}
inline ChoiceFlags operator&(ChoiceFlags lhs, ChoiceFlags rhs)
{
    return static_cast<ChoiceFlags>(+lhs & +rhs);
}
inline ChoiceFlags &operator&=(ChoiceFlags &lhs, ChoiceFlags rhs)
{
    lhs = lhs & rhs;
    return lhs;
}
inline ChoiceFlags operator|(ChoiceFlags lhs, ChoiceFlags rhs)
{
    return static_cast<ChoiceFlags>(+lhs | +rhs);
}
inline ChoiceFlags &operator|=(ChoiceFlags &lhs, ChoiceFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}
inline bool bit_set(ChoiceFlags lhs, ChoiceFlags rhs)
{
    return (lhs & rhs) == rhs;
}

int full_screen_choice(ChoiceFlags flags,                   //
    char const *hdg, char const *hdg2, char const *instr,   //
    int num_choices, char const **choices, int *attributes, //
    int box_width, int box_depth, int col_width,            //
    int current, void (*format_item)(int, char *), char *speed_string,
    int (*speed_prompt)(int row, int col, int vid, char const *speedstring, int speed_match), //
    int (*check_key)(int, int));
