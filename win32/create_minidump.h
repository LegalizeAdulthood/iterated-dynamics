// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "win_defines.h"
#include <Windows.h>

namespace id::misc
{

void create_minidump(EXCEPTION_POINTERS *ep);

} // namespace id::misc
