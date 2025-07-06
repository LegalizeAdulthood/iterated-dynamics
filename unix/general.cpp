// SPDX-License-Identifier: GPL-3.0-only
//
/* generalasm.c
 * This file contains routines to replace general.asm.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */

#include "engine/id_data.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "io/loadfile.h"
#include "misc/Driver.h"
#include "engine/cmdfiles.h"
#include "ui/read_ticker.h"
#include "ui/slideshw.h"
#include "ui/tab_display.h"

#include <config/port.h>

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
