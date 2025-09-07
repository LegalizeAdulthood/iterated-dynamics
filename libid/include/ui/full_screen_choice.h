// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::ui
{

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
    const char *hdg, const char *hdg2, const char *instr,   //
    int num_choices, const char **choices, int *attributes, //
    int box_width, int box_depth, int col_width,            //
    int current, void (*format_item)(int choice, char *buf), char *speed_string,
    int (*speed_prompt)(int row, int col, int vid, const char *speed_string, int speed_match), //
    int (*check_key)(int key, int choice));

} // namespace id::ui
