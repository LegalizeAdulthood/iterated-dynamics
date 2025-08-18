// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#ifdef WIN32
#include "win_defines.h"
#include <Windows.h>
#else
using HINSTANCE = void *; // Dummy definition for non-Windows platforms
#endif

extern HINSTANCE g_instance;
