// SPDX-License-Identifier: GPL-3.0-only
//
#include "misc/stack_avail.h"

namespace id::misc
{

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

} // namespace id::misc
