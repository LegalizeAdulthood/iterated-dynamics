#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "id_data.h"
#include "rotate.h"
#include "spindac.h"
#include "zoom.h"

#include <cstdio>
#include <cstring>

/*
 * This file contains Unix versions of the routines in video.asm
 * Copyright 1992 Ken Shirriff
 */

extern unsigned char *xgetfont();
extern int startdisk();
extern int waitkeypressed(int);

//WINDOW *curwin;

bool g_fake_lut = false;
int dacnorm = 0;
int ShadowColors;
void (*dotwrite)(int, int, int);
// write-a-dot routine
int (*dotread)(int, int);   // read-a-dot routine
void (*linewrite)(int y, int x, int lastx, BYTE const *pixels);        // write-a-line routine
void (*lineread)(int y, int x, int lastx, BYTE *pixels);         // read-a-line routine

int videoflag = 0;      // special "your-own-video" flag

void (*swapsetup)() = nullptr;     // setfortext/graphics setup routine
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
    {   "xfractint mode           ", "                         ",
        999, 0, 0, 0, 0, 19, 640, 480, 256
    },
};

void setforgraphics();

void
nullwrite(int a, int b, int c)
{
}

int
nullread(int a, int b)
{
    return 0;
}

void
setnullvideo()
{
    dotwrite = nullwrite;
    dotread = nullread;
}

#if 0
void
putprompt()
{
    wclear(curwin);       // ????
    putstring(0, 0, 0, "Press operation key, or <Esc> to return to Main Menu");
    wrefresh(curwin);
    return;
}
#endif

void
loaddac()
{
    readvideopalette();
}

/*
; **************** Function setvideomode(ax, bx, cx, dx) ****************
;       This function sets the (alphanumeric or graphic) video mode
;       of the monitor.   Called with the proper values of AX thru DX.
;       No returned values, as there is no particular standard to
;       adhere to in this case.

;       (SPECIAL "TWEAKED" VGA VALUES:  if AX==BX==CX==0, assume we have a
;       genuine VGA or register compatable adapter and program the registers
;       directly using the coded value in DX)

; Unix: We ignore ax,bx,cx,dx.  dotmode is the "mode" field in the video
; table.  We use mode 19 for the X window.
*/

#if 0
void
setvideomode(int ax, int bx, int cx, int dx)
{
    if (g_disk_flag)
    {
        enddisk();
    }
    if (videoflag)
    {
        endvideo();
        videoflag = 0;
    }
    g_good_mode = true;
    switch (g_dot_mode)
    {
    case 0:         // text
        clear();
        /*
           touchwin(curwin);
         */
        wrefresh(curwin);
        break;
    case 11:
        startdisk();
        dotwrite = writedisk;
        dotread = readdisk;
        lineread = normalineread;
        linewrite = normaline;
        break;
    case 19:            // X window
        putprompt();
        dotwrite = writevideo;
        dotread = readvideo;
        lineread = readvideoline;
        linewrite = writevideoline;
        videoflag = 1;
        startvideo();
        setforgraphics();
        break;
    default:
        std::printf("Bad mode %d\n", g_dot_mode);
        exit(-1);
    }
    if (g_dot_mode != 0)
    {
        loaddac();
        g_and_color = g_colors - 1;
        g_box_count = 0;
    }
    g_vesa_x_res = g_screen_x_dots;
    g_vesa_y_res = g_screen_y_dots;
}
#endif


/*
; **************** Function getcolor(xdot, ydot) *******************

;       Return the color on the screen at the (xdot,ydot) point
*/
int
getcolor(int xdot, int ydot)
{
    int x1, y1;
    x1 = xdot + g_logical_screen_x_offset;
    y1 = ydot + g_logical_screen_y_offset;
    if (x1 < 0 || y1 < 0 || x1 >= g_screen_x_dots || y1 >= g_screen_y_dots)
        return 0;
    return dotread(x1, y1);
}

/*
; ************** Function putcolor_a(xdot, ydot, color) *******************

;       write the color on the screen at the (xdot,ydot) point
*/
void
putcolor_a(int xdot, int ydot, int color)
{
    dotwrite(xdot + g_logical_screen_x_offset, ydot + g_logical_screen_y_offset, color & g_and_color);
}

/*
; **************** Function movecursor(row, col)  **********************

;       Move the cursor (called before printfs)
*/
#if 0
void
movecursor(int row, int col)
{
    if (row == -1)
    {
        row = g_text_row;
    }
    else
    {
        g_text_row = row;
    }
    if (col == -1)
    {
        col = g_text_col;
    }
    else
    {
        g_text_col = col;
    }
    wmove(curwin, row, col);
}
#endif

/*
; **************** Function keycursor(row, col)  **********************

;       Subroutine to wait cx ticks, or till keystroke pending
*/
#if 0
int
keycursor(int row, int col)
{
    movecursor(row, col);
    wrefresh(curwin);
    waitkeypressed(0);
    return getakey();
}
#endif

/*
; PUTSTR.asm puts a string directly to video display memory. Called from C by:
;    putstring(row, col, attr, string) where
;         row, col = row and column to start printing.
;         attr = color attribute.
;         string = far pointer to the null terminated string to print.
*/
#if 0
void
putstring(int row, int col, int attr, char const *msg)
{
    int so = 0;

    if (row != -1)
        g_text_row = row;
    if (col != -1)
        g_text_col = col;

    if (attr & INVERSE || attr & BRIGHT)
    {
        wstandout(curwin);
        so = 1;
    }
    wmove(curwin, g_text_row + g_text_rbase, g_text_col + g_text_cbase);
    while (1)
    {
        if (*msg == '\0')
            break;
        if (*msg == '\n')
        {
            g_text_col = 0;
            g_text_row++;
            wmove(curwin, g_text_row + g_text_rbase, g_text_col + g_text_cbase);
        }
        else
        {
            char const *ptr;
            ptr = std::strchr(msg, '\n');
            if (ptr == nullptr)
            {
                waddstr(curwin, msg);
                break;
            }
            else
            {
                waddch(curwin, *msg);
            }
        }
        msg++;
    }
    if (so)
    {
        wstandend(curwin);
    }

    wrefresh(curwin);
    fflush(stdout);
    getyx(curwin, g_text_row, g_text_col);
    g_text_row -= g_text_rbase;
    g_text_col -= g_text_cbase;
}
#endif

/*
; ************* Function scrollup(toprow, botrow) ******************

;       Scroll the screen up (from toprow to botrow)
*/
#if 0
void
scrollup(int top, int bot)
{
    wmove(curwin, top, 0);
    wdeleteln(curwin);
    wmove(curwin, bot, 0);
    winsertln(curwin);
    wrefresh(curwin);
}
#endif

/*
; ---- Help (Video) Support
; ********* Functions setfortext() and setforgraphics() ************

;       setfortext() resets the video for text mode and saves graphics data
;       setforgraphics() restores the graphics mode and data
;       setclear() clears the screen after setfortext()
*/
void
setfortext()
{
}

#if 0
void
setclear()
{
    wclear(curwin);
    wrefresh(curwin);
}
#endif

void
setforgraphics()
{
    startvideo();
    spindac(0, 1);
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
; ******************** Zoombox functions **************************
*/

/*
 * The IBM method is that g_box_x[],g_box_y[] is a set of locations, and g_box_values
 * is the values in these locations.
 * Instead of using this box save/restore technique, we'll put the corners
 * in g_box_x[0],g_box_y[0],1,2,3 and then use xor.
 */

void
dispbox()
{
    if (g_box_count)
    {
        setlinemode(1);
        drawline(g_box_x[0], g_box_y[0], g_box_x[1], g_box_y[1]);
        drawline(g_box_x[1], g_box_y[1], g_box_x[2], g_box_y[2]);
        drawline(g_box_x[2], g_box_y[2], g_box_x[3], g_box_y[3]);
        drawline(g_box_x[3], g_box_y[3], g_box_x[0], g_box_y[0]);
        setlinemode(0);
        xsync();
    }
}

void
clearbox()
{
    dispbox();
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
    return (char) getakey();
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
    lineread(row + g_logical_screen_y_offset, startcol + g_logical_screen_x_offset, stopcol + g_logical_screen_x_offset, pixels);
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
    linewrite(row + g_logical_screen_y_offset, startcol + g_logical_screen_x_offset, stopcol + g_logical_screen_x_offset, pixels);
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
    linewrite(g_row_count + g_logical_screen_y_offset, g_logical_screen_x_offset, linelen + g_logical_screen_x_offset - 1, pixels);
    g_row_count++;
    return 0;
}
