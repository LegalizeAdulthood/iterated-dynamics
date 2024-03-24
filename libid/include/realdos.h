#pragma once

#include <string>

enum input_field_flags
{
    INPUTFIELD_NUMERIC  = 1,
    INPUTFIELD_INTEGER  = 2,
    INPUTFIELD_DOUBLE   = 4
};

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

extern int                   g_cfg_line_nums[];
extern int                   g_patch_level;
extern int                   g_release;
extern std::string const     g_speed_prompt;

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
void helptitle();
int putstringcenter(int row, int col, int width, int attr, char const *msg);
int main_menu(int);
int input_field(int options, int attr, char *fld, int len, int row, int col,
    int (*checkkey)(int curkey));
bool thinking(int options, char const *msg);
void discardgraphics();
void load_id_config();
