#pragma once

#include "miscres.h"

#include <string>

extern int                   g_patch_level;
extern int                   g_release;
extern std::string const     g_speed_prompt;

void footer_msg(int *i, int options, char const *speedstring);
void show_speedstring(
    int speedrow,
    char const *speedstring,
    int (*speedprompt)(int row, int col, int vid, char const *speedstring, int speed_match));
void helptitle();
int putstringcenter(int row, int col, int width, int attr, char const *msg);
bool thinking(int options, char const *msg);
void discardgraphics();
void process_speedstring(char *speedstring, //
    char const **choices,                   // array of choice strings
    int curkey,                             //
    int *pcurrent,                          //
    int numchoices,                         //
    int is_unsorted);
