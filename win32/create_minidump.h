#pragma once

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <Windows.h>

void create_minidump(EXCEPTION_POINTERS *ep);
