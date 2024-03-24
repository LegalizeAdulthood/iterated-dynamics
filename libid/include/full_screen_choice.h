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
    int numchoices,
    char const **choices,
    int *attributes,
    int boxwidth,
    int boxdepth,
    int colwidth,
    int current,
    void (*formatitem)(int, char*),
    char *speedstring,
    int (*speedprompt)(int row, int col, int vid, char const *speedstring, int speed_match),
    int (*checkkey)(int, int)
);
