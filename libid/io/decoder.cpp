// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/decoder.h"

#include "ui/video.h"

using namespace id::ui;

namespace id::io
{

int (*g_out_line)(Byte *, int){out_line};

} // namespace id::io
