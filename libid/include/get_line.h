#pragma once

#include "port.h"

void write_span(int row, int startcol, int stopcol, BYTE const *pixels);
void read_span(int row, int startcol, int stopcol, BYTE *pixels);
