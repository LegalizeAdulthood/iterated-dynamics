// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

extern int                 (*g_out_line)(Byte *, int);

short decoder(short line_width);
void set_byte_buff(Byte *ptr);
