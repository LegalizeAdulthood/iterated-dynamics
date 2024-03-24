#pragma once

#include "miscres.h"

#include <string>

extern int                   g_patch_level;
extern int                   g_release;

void helptitle();
int putstringcenter(int row, int col, int width, int attr, char const *msg);
bool thinking(int options, char const *msg);
void discardgraphics();
