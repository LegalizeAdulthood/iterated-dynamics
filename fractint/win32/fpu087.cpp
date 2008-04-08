#include <string>

#include "port.h"
#include "prototyp.h"
#include "fpu.h"

/*
 *----------------------------------------------------------------------
 *
 * xxx086 --
 *
 *	Simulate integer math routines using floating point.
 *	This will of course slow things down, so this should all be
 *	changed at some point.
 *
 *----------------------------------------------------------------------
 */

int DivideOverflow = 0;

// Integer Routines



