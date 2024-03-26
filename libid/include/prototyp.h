#pragma once

#include "id.h"

#ifdef XFRACT
#include "unixprot.h"
#else
#endif
void put_line(int row, int startcol, int stopcol, BYTE const *pixels);
void get_line(int row, int startcol, int stopcol, BYTE *pixels);
