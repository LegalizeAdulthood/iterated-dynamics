// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// Include this before including any Windows headers to minimize the
// amount of stuff that gets brought in and increase safety.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef STRICT
#define STRICT
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
