#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "get_color.h"
#include "get_line.h"
#include "id_data.h"
#include "out_line.h"
#include "put_color_a.h"
#include "rotate.h"
#include "spindac.h"
#include "text_screen.h"
#include "unixscr.h"
#include "video_mode.h"
#include "zoom.h"

#include <cstdio>
#include <cstring>

/*
 * This file contains Unix versions of the routines in video.asm
 * Copyright 1992 Ken Shirriff
 */

extern int startdisk();

//WINDOW *curwin;

bool g_fake_lut = false;
int dacnorm = 0;
int ShadowColors;

int videoflag = 0;      // special "your-own-video" flag

int g_row_count = 0;        // row-counter for decoder and out_line
int video_type = 0;     /* actual video adapter type:
                   0  = type not yet determined
                   1  = Hercules
                   2  = CGA (assumed if nothing else)
                   3  = EGA
                   4  = MCGA
                   5  = VGA
                   6  = VESA (not yet checked)
                   11  = 8514/A (not yet checked)
                   12  = TIGA   (not yet checked)
                   13  = TARGA  (not yet checked)
                   100 = x monochrome
                   101 = x 256 colors
                 */
int svga_type = 0;      /*  (forced) SVGA type
                   1 = ahead "A" type
                   2 = ATI
                   3 = C&T
                   4 = Everex
                   5 = Genoa
                   6 = Ncr
                   7 = Oak-Tech
                   8 = Paradise
                   9 = Trident
                   10 = Tseng 3000
                   11 = Tseng 4000
                   12 = Video-7
                   13 = ahead "B" type
                   14 = "null" type (for testing only) */
int mode7text = 0;      // nonzero for egamono and hgc
int textaddr = 0xb800;      // b800 for mode 3, b000 for mode 7
int textsafe = 0;       /* 0 = default, runup chgs to 1
                   1 = yes
                   2 = no, use 640x200
                   3 = bios, yes plus use int 10h-1Ch
                   4 = save, save entire image */
int g_vesa_detect = 1;      // set to 0 to disable VESA-detection
int g_video_start_x = 0;
int g_video_start_y = 0;
int g_vesa_x_res;
int g_vesa_y_res;
int chkd_vvs = 0;
int video_vram = 0;

void putstring(int row, int col, int attr, char const *msg);

/*

;       |--Adapter/Mode-Name------|-------Comments-----------|

;       |------INT 10H------|Dot-|--Resolution---|
;       |key|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|
*/

VIDEOINFO x11_video_table[] = {
    { "id        mode           ", 999, 640, 480, 256, nullptr, "                         " },
};

int
nullread(int a, int b)
{
    return 0;
}

void
setnullvideo()
{
}

void
loaddac()
{
    readvideopalette();
}

/*
; **************** Function getcolor(xdot, ydot) *******************

;       Return the color on the screen at the (xdot,ydot) point
*/
int
getcolor(int xdot, int ydot)
{
        return 0;
}

/*
; ************** Function putcolor_a(xdot, ydot, color) *******************

;       write the color on the screen at the (xdot,ydot) point
*/
void
putcolor_a(int xdot, int ydot, int color)
{
}

unsigned char *fontTab = nullptr;

/*
; ************** Function findfont(n) ******************************

;       findfont(0) returns pointer to 8x8 font table if it can
;                   find it, nullptr otherwise;
;                   nonzero parameter reserved for future use
*/
BYTE *
findfont(int fontparm)
{
    if (fontTab == nullptr)
    {
        fontTab = xgetfont();
    }
    return (BYTE *) fontTab;
}

/*
; Passing this routine 0 turns off shadow, nonzero turns it on.
*/
int
ShadowVideo(int on)
{
    return 0;
}

int
SetupShadowVideo()
{
    return 0;
}

/*
; *************** Functions get_a_char, put_a_char ********************

;       Get and put character and attribute at cursor
;       Hi nybble=character, low nybble attribute. Text mode only
*/
char
get_a_char()
{
    return 0;
}

void
put_a_char(int ch)
{
}

/*
; ***Function get_line(int row,int startcol,int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'getcolor()', and gets a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/

void get_line(int row, int startcol, int stopcol, BYTE *pixels)
{
    if (startcol + g_logical_screen_x_offset >= g_screen_x_dots || row + g_logical_screen_y_offset >= g_screen_y_dots)
        return;
}

/*
; ***Function put_line(int row,int startcol,int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'putcolor()', and puts a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/

void
put_line(int row, int startcol, int stopcol, BYTE const *pixels)
{
    if (startcol + g_logical_screen_x_offset >= g_screen_x_dots || row + g_logical_screen_y_offset > g_screen_y_dots)
        return;
}

/*
; ***************Function out_line(pixels,linelen) *********************

;       This routine is a 'line' analog of 'putcolor()', and sends an
;       entire line of pixels to the screen (0 <= xdot < xdots) at a clip
;       Called by the GIF decoder
*/
int
out_line(BYTE *pixels, int linelen)
{
    if (g_row_count + g_logical_screen_y_offset >= g_screen_y_dots)
        return 0;
    g_row_count++;
    return 0;
}
