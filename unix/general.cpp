// SPDX-License-Identifier: GPL-3.0-only
//
/* generalasm.c
 * This file contains routines to replace general.asm.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */
#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "id_data.h"
#include "loadfile.h"
#include "read_ticker.h"
#include "slideshw.h"
#include "tab_display.h"

#include <fcntl.h>
#include <unistd.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

// ********************** Mouse Support Variables **************************


bool g_inside_help = false;

/*
; ************** Function tone(int frequency,int delaytime) **************
;
;       buzzes the speaker with this frequency for this amount of time
*/
void
tone(int frequency, int delaytime)
{
}

/*
; ************** Function snd(int hertz) and nosnd() **************
;
;       turn the speaker on with this frequency (snd) or off (nosnd)
;
; *****************************************************************
*/
void
snd(int hertz)
{
}

void
nosnd()
{}

/*
; long readticker() returns current bios ticker value
*/
long readticker()
{
    return std::clock();
}

