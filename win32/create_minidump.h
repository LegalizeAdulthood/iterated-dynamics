#pragma once

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <Windows.h>

void CreateMiniDump(EXCEPTION_POINTERS *ep);
