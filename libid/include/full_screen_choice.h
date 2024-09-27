// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

extern std::string const     g_speed_prompt;

// fullscreen_choice options
enum choice_flags
{
    CHOICE_RETURN_KEY   = 1,
    CHOICE_MENU         = 2,
    CHOICE_HELP         = 4,
    CHOICE_INSTRUCTIONS = 8,
    CHOICE_CRUNCH       = 16,
    CHOICE_NOT_SORTED   = 32
};

int fullscreen_choice(
    int options,
    char const *hdg,
    char const *hdg2,
    char const *instr,
    int num_choices,
    char const **choices,
    int *attributes,
    int box_width,
    int box_depth,
    int col_width,
    int current,
    void (*format_item)(int, char*),
    char *speed_string,
    int (*speed_prompt)(int row, int col, int vid, char const *speedstring, int speed_match),
    int (*check_key)(int, int)
    );
