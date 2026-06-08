// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

namespace id::io
{

extern int                 (*g_out_line)(Byte *, int);

int decoder(int line_width);
void set_byte_buff(Byte *ptr);

} // namespace id::io
