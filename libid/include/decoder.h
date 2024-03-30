#pragma once

#include "port.h"

extern int                 (*g_out_line)(BYTE *, int);

short decoder(short);
void set_byte_buff(BYTE *ptr);
