#include "text_screen.h"

int g_text_row{};   // for putstring(-1,...)
int g_text_col{};   // for putstring(..,-1,...)
int g_text_rbase{}; // g_text_row is relative to this
int g_text_cbase{}; // g_text_col is relative to this
