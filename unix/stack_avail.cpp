// SPDX-License-Identifier: GPL-3.0-only
//
#include "stack_avail.h"

/*
 *----------------------------------------------------------------------
 *
 * stack_avail --
 *
 *      Returns amout of stack available.
 *
 * Results:
 *      Available stack.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
long stack_avail()
{
    return 8192;
}
