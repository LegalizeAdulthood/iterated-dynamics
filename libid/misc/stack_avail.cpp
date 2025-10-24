// SPDX-License-Identifier: GPL-3.0-only
//
#include "misc/stack_avail.h"

namespace id::misc
{

enum
{
    WIN32_STACK_SIZE = 1024 * 1024
};

char *g_top_of_stack{};

// Return available stack space ... shouldn't be needed in Win32, should it?
long stack_avail()
{
    char junk{};
    return WIN32_STACK_SIZE - static_cast<long>(g_top_of_stack - &junk);
}

} // namespace id::misc
