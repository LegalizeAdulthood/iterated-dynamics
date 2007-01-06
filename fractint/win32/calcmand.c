/* calcmand.c
 * This file contains routines to replace calcmand.asm.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */

#include "port.h"
#include "prototyp.h"

unsigned long savedmask;
long linitx, linity;

long cdecl
calcmandasm(void)
{
	static int been_here = 0;
	if (!been_here)
	{
		stopmsg(0, "This integer fractal type is unimplemented;\n"
			"Use float=yes to get a real image.");
		been_here = 1;
	}
	return 0;
}
