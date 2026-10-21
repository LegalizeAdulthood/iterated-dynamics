// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>

namespace id::io
{

using OutLine = int (*)(Byte *, int);

extern OutLine g_out_line;
extern int g_row_count;

int out_line(Byte *pixels, int line_len);

} // namespace id::io
