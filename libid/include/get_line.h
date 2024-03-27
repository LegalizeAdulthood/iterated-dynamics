#pragma once

#include "port.h"

void put_line(int row, int startcol, int stopcol, BYTE const *pixels);
void get_line(int row, int startcol, int stopcol, BYTE *pixels);
