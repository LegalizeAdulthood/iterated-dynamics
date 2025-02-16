// SPDX-License-Identifier: GPL-3.0-only
//
#include "tos.h"

// Global variables (yuck!)
char *g_top_of_stack{};

enum
{
    WIN32_STACK_SIZE = 1024 * 1024
};

// Return available stack space ... shouldn't be needed in Win32, should it?
long stack_avail()
{
    char junk{};
    return WIN32_STACK_SIZE - (long)(g_top_of_stack - &junk);
}
