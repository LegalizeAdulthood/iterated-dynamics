#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef STRICT
#define STRICT
#endif
#include <Windows.h>

void create_minidump(EXCEPTION_POINTERS *ep);
