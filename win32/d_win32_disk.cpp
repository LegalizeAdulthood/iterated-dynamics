/* d_win32_disk.c
 *
 * Routines for a Win32 disk video mode driver for fractint.
 */
#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "id_data.h"
#include "os.h"
#include "plot3d.h"
#include "put_color_a.h"
#include "rotate.h"
#include "spindac.h"
#include "video_mode.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <Windows.h>

#include <cassert>

#include "WinText.h"
#include "frame.h"
#include "d_win32.h"
#include "ods.h"

#include <cstdio>
#include <get_color.h>

// read/write-a-dot/line routines
using t_dotwriter = void(int, int, int);
using t_dotreader = int(int, int);
using t_linewriter = void(int y, int x, int lastx, BYTE *pixels);
using t_linereader = void(int y, int x, int lastx, BYTE *pixels);

extern HINSTANCE g_instance;

#define DRAW_INTERVAL 6
#define TIMER_ID 1

#define DI(name_) Win32DiskDriver *name_ = (Win32DiskDriver *) drv

struct Win32DiskDriver
{
    Win32BaseDriver base;
    int width;
    int height;
    unsigned char clut[256][3];
};

// VIDEOINFO:
//         char    name[26];       Adapter name (IBM EGA, etc)
//         char    comment[26];    Comments (UNTESTED, etc)
//         int     keynum;         key number used to invoked this mode
//                                 2-10 = F2-10, 11-40 = S,C,A{F1-F10}
//         int     videomodeax;    begin with INT 10H, AX=(this)
//         int     videomodebx;                 ...and BX=(this)
//         int     videomodecx;                 ...and CX=(this)
//         int     videomodedx;                 ...and DX=(this)
//                                 NOTE:  IF AX==BX==CX==0, SEE BELOW
//         int     dotmode;        video access method used by asm code
//                                      1 == BIOS 10H, AH=12,13 (SLOW)
//                                      2 == access like EGA/VGA
//                                      3 == access like MCGA
//                                      4 == Tseng-like  SuperVGA*256
//                                      5 == P'dise-like SuperVGA*256
//                                      6 == Vega-like   SuperVGA*256
//                                      7 == "Tweaked" IBM-VGA ...*256
//                                      8 == "Tweaked" SuperVGA ...*256
//                                      9 == Targa Format
//                                      10 = Hercules
//                                      11 = "disk video" (no screen)
//                                      12 = 8514/A
//                                      13 = CGA 320x200x4, 640x200x2
//                                      14 = Tandy 1000
//                                      15 = TRIDENT  SuperVGA*256
//                                      16 = Chips&Tech SuperVGA*256
//         int     xdots;          number of dots across the screen
//         int     ydots;          number of dots down the screen
//         int     colors;         number of colors available

#define DRIVER_MODE(name_, comment_, key_, width_, height_, mode_) \
    { name_, comment_, key_, 0, 0, 0, 0, mode_, width_, height_, 256, nullptr }
#define MODE19(n_, c_, k_, w_, h_) DRIVER_MODE(n_, c_, k_, w_, h_, 19)
static VIDEOINFO modes[] =
{
    MODE19("Win32 Disk Video         ", "                        ", 0,  320,  240),
    MODE19("Win32 Disk Video         ", "                        ", 0,  400,  300),
    MODE19("Win32 Disk Video         ", "                        ", 0,  480,  360),
    MODE19("Win32 Disk Video         ", "                        ", 0,  600,  450),
    MODE19("Win32 Disk Video         ", "                        ", 0,  640,  480),
    MODE19("Win32 Disk Video         ", "                        ", 0,  800,  600),
    MODE19("Win32 Disk Video         ", "                        ", 0, 1024,  768),
    MODE19("Win32 Disk Video         ", "                        ", 0, 1200,  900),
    MODE19("Win32 Disk Video         ", "                        ", 0, 1280,  960),
    MODE19("Win32 Disk Video         ", "                        ", 0, 1400, 1050),
    MODE19("Win32 Disk Video         ", "                        ", 0, 1500, 1125),
    MODE19("Win32 Disk Video         ", "                        ", 0, 1600, 1200)
};
#undef MODE19
#undef DRIVER_MODE

/* check_arg
 *
 *  See if we want to do something with the argument.
 *
 * Results:
 *  Returns 1 if we parsed the argument.
 *
 * Side effects:
 *  Increments i if we use more than 1 argument.
 */
static bool
check_arg(Win32DiskDriver *di, char *arg)
{
    return false;
}

/*----------------------------------------------------------------------
*
* initdacbox --
*
* Put something nice in the dac.
*
* The conditions are:
*   Colors 1 and 2 should be bright so ifs fractals show up.
*   Color 15 should be bright for lsystem.
*   Color 1 should be bright for bifurcation.
*   Colors 1, 2, 3 should be distinct for periodicity.
*   The color map should look good for mandelbrot.
*
* Results:
*   None.
*
* Side effects:
*   Loads the dac.
*
*----------------------------------------------------------------------
*/
static void
initdacbox()
{
    for (int i = 0; i < 256; i++)
    {
        g_dac_box[i][0] = (i >> 5)*8+7;
        g_dac_box[i][1] = (((i+16) & 28) >> 2)*8+7;
        g_dac_box[i][2] = (((i+2) & 3))*16+15;
    }
    g_dac_box[0][2] = 0;
    g_dac_box[0][1] = g_dac_box[0][2];
    g_dac_box[0][0] = g_dac_box[0][1];
    g_dac_box[1][2] = 255;
    g_dac_box[1][1] = g_dac_box[1][2];
    g_dac_box[1][0] = g_dac_box[1][1];
    g_dac_box[2][0] = 190;
    g_dac_box[2][2] = 255;
    g_dac_box[2][1] = g_dac_box[2][2];
}

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

/*----------------------------------------------------------------------
*
* disk_init --
*
*   Initialize the windows and stuff.
*
* Results:
*   None.
*
* Side effects:
*   Initializes windows.
*
*----------------------------------------------------------------------
*/
static bool disk_init(Driver *drv, int *argc, char **argv)
{
    LPCSTR title = "FractInt for Windows";
    DI(di);

    frame_init(g_instance, title);
    if (!wintext_initialize(&di->base.wintext, g_instance, nullptr, title))
    {
        return false;
    }

    initdacbox();

    // filter out driver arguments
    for (int i = 0; i < *argc; i++)
    {
        if (check_arg(di, argv[i]))
        {
            int j;
            for (j = i; j < *argc-1; j++)
            {
                argv[j] = argv[j+1];
            }
            argv[j] = nullptr;
            --*argc;
        }
    }

    // add default list of video modes
    for (VIDEOINFO &mode : modes)
    {
        add_video_mode(drv, &mode);
    }

    return true;
}

/* disk_resize
 *
 * Check if we need resizing.  If no, return 0.
 * If yes, resize internal buffers and return 1.
 */
static bool
disk_resize(Driver *drv)
{
    DI(di);

    frame_resize(di->base.wintext.max_width, di->base.wintext.max_height);
    if ((g_video_table[g_adapter].xdots == di->width)
        && (g_video_table[g_adapter].ydots == di->height))
    {
        return false;
    }

    if (g_disk_flag)
    {
        enddisk();
    }
    startdisk();

    return true;
}


/*----------------------------------------------------------------------
* disk_read_palette
*
*   Reads the current video palette into g_dac_box.
*
*
* Results:
*   None.
*
* Side effects:
*   Fills in g_dac_box.
*
*----------------------------------------------------------------------
*/
static int
disk_read_palette(Driver *drv)
{
    DI(di);

    ODS("disk_read_palette");
    if (!g_got_real_dac)
    {
        return -1;
    }
    for (int i = 0; i < 256; i++)
    {
        g_dac_box[i][0] = di->clut[i][0];
        g_dac_box[i][2] = di->clut[i][2];
    }
    return 0;
}

/*
*----------------------------------------------------------------------
*
* disk_write_palette --
*   Writes g_dac_box into the video palette.
*
*
* Results:
*   None.
*
* Side effects:
*   Changes the displayed colors.
*
*----------------------------------------------------------------------
*/
static int
disk_write_palette(Driver *drv)
{
    DI(di);

    ODS("disk_write_palette");
    for (int i = 0; i < 256; i++)
    {
        di->clut[i][0] = g_dac_box[i][0];
        di->clut[i][1] = g_dac_box[i][1];
        di->clut[i][2] = g_dac_box[i][2];
    }

    return 0;
}

/*
*----------------------------------------------------------------------
*
* disk_schedule_alarm --
*
*   Start the refresh alarm
*
* Results:
*   None.
*
* Side effects:
*   Starts the alarm.
*
*----------------------------------------------------------------------
*/
static void
disk_schedule_alarm(Driver *drv, int secs)
{
    DI(di);
    wintext_schedule_alarm(&di->base.wintext, (secs ? 1 : DRAW_INTERVAL)*1000);
}

/*
*----------------------------------------------------------------------
*
* disk_write_pixel --
*
*   Write a point to the screen
*
* Results:
*   None.
*
* Side effects:
*   Draws point.
*
*----------------------------------------------------------------------
*/
static void
disk_write_pixel(Driver *drv, int x, int y, int color)
{
    putcolor_a(x, y, color);
}

/*
*----------------------------------------------------------------------
*
* disk_read_pixel --
*
*   Read a point from the screen
*
* Results:
*   Value of point.
*
* Side effects:
*   None.
*
*----------------------------------------------------------------------
*/
static int
disk_read_pixel(Driver *drv, int x, int y)
{
    return getcolor(x, y);
}

/*
*----------------------------------------------------------------------
*
* disk_write_span --
*
*   Write a line of pixels to the screen.
*
* Results:
*   None.
*
* Side effects:
*   Draws pixels.
*
*----------------------------------------------------------------------
*/
static void
disk_write_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
    int width = lastx-x+1;
    ODS3("disk_write_span (%d,%d,%d)", y, x, lastx);

    for (int i = 0; i < width; i++)
    {
        disk_write_pixel(drv, x+i, y, pixels[i]);
    }
}

/*
*----------------------------------------------------------------------
*
* disk_read_span --
*
*   Reads a line of pixels from the screen.
*
* Results:
*   None.
*
* Side effects:
*   Gets pixels
*
*----------------------------------------------------------------------
*/
static void
disk_read_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
    ODS3("disk_read_span (%d,%d,%d)", y, x, lastx);
    int width = lastx-x+1;
    for (int i = 0; i < width; i++)
    {
        pixels[i] = disk_read_pixel(drv, x+i, y);
    }
}

static void
disk_set_line_mode(Driver *drv, int mode)
{
    ODS1("disk_set_line_mode %d", mode);
}

static void
disk_draw_line(Driver *drv, int x1, int y1, int x2, int y2, int color)
{
    ODS5("disk_draw_line (%d,%d) (%d,%d) %d", x1, y1, x2, y2, color);
    draw_line(x1, y1, x2, y2, color);
}

/*
*----------------------------------------------------------------------
*
* disk_redraw --
*
*   Refresh the screen.
*
* Results:
*   None.
*
* Side effects:
*   Redraws the screen.
*
*----------------------------------------------------------------------
*/
static void
disk_redraw(Driver *drv)
{
    DI(di);
    ODS("disk_redraw");
    wintext_paintscreen(&di->base.wintext, 0, 80, 0, 25);
}

/* disk_unget_key
 *
 * Unread a key!  The key buffer is only one character deep, so we
 * assert if its already full.  This should never happen in real life :-).
 */
void disk_unget_key(Driver *drv, int key)
{
    DI(di);
    _ASSERTE(0 == di->base.key_buffer);
    di->base.key_buffer = key;
}

static void
disk_window(Driver *drv)
{
    DI(di);
    frame_window(di->base.wintext.max_width, di->base.wintext.max_height);
    di->base.wintext.hWndParent = g_frame.window;
    wintext_texton(&di->base.wintext);
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
extern void set_disk_dot();
extern void set_normal_line();
static void
disk_set_video_mode(Driver *drv, VIDEOINFO *mode)
{
    // initially, set the virtual line to be the scan line length
    g_is_true_color = false;            // assume not truecolor
    g_vesa_x_res = 0;                   // reset indicators used for
    g_vesa_y_res = 0;                   // virtual screen limits estimation
    g_good_mode = true;
    if (g_dot_mode != 0)
    {
        g_and_color = g_colors-1;
        g_box_count = 0;
        g_dac_learn = true;
        g_dac_count = g_cycle_limit;
        g_got_real_dac = true;

        driver_read_palette();
    }

    disk_resize(drv);

    set_disk_dot();
    set_normal_line();
}

static void
disk_set_clear(Driver *drv)
{
    DI(di);
    wintext_clear(&di->base.wintext);
}

static void
disk_display_string(Driver *drv, int x, int y, int fg, int bg, char const *text)
{
}

static void
disk_hide_text_cursor(Driver *drv)
{
    DI(di);
    if (di->base.cursor_shown)
    {
        di->base.cursor_shown = false;
        wintext_hide_cursor(&di->base.wintext);
    }
    ODS("disk_hide_text_cursor");
}

static void
disk_set_for_text(Driver *drv)
{
}

static void
disk_set_for_graphics(Driver *drv)
{
    disk_hide_text_cursor(drv);
}

static bool
disk_diskp(Driver *drv)
{
    return true;
}

static bool
disk_validate_mode(Driver *drv, VIDEOINFO *mode)
{
    /* allow modes of any size with 256 colors and dotmode=19
       ax/bx/cx/dx must be zero. */
    return (mode->colors == 256)
        && (mode->videomodeax == 0)
        && (mode->videomodebx == 0)
        && (mode->videomodecx == 0)
        && (mode->videomodedx == 0)
        && (mode->dotmode == 19);
}

static void
disk_pause(Driver *drv)
{
    DI(di);
    if (di->base.wintext.hWndCopy)
    {
        ShowWindow(di->base.wintext.hWndCopy, SW_HIDE);
    }
}

static void
disk_resume(Driver *drv)
{
    DI(di);
    if (!di->base.wintext.hWndCopy)
    {
        disk_window(drv);
    }

    if (di->base.wintext.hWndCopy)
    {
        ShowWindow(di->base.wintext.hWndCopy, SW_NORMAL);
    }
    wintext_resume(&di->base.wintext);
}

static void disk_save_graphics(Driver *drv)
{
}

static void disk_restore_graphics(Driver *drv)
{
}

static void disk_get_max_screen(Driver *drv, int *xmax, int *ymax)
{
    if (xmax != nullptr)
    {
        *xmax = -1;
    }
    if (ymax != nullptr)
    {
        *ymax = -1;
    }
}

static void disk_flush(Driver *drv)
{
}

static Win32DiskDriver disk_driver_info =
{
    {
        "disk", "A disk video driver for 32-bit Windows.",
        disk_init,
        disk_validate_mode,
        disk_get_max_screen,
        win32_terminate,
        disk_pause, disk_resume,
        disk_schedule_alarm,
        disk_window, disk_resize, disk_redraw,
        disk_read_palette, disk_write_palette,
        disk_read_pixel, disk_write_pixel,
        disk_read_span, disk_write_span,
        win32_get_truecolor, win32_put_truecolor,
        disk_set_line_mode, disk_draw_line,
        disk_display_string,
        disk_save_graphics, disk_restore_graphics,
        win32_get_key, win32_key_cursor, win32_key_pressed, win32_wait_key_pressed, win32_unget_key,
        win32_shell,
        disk_set_video_mode,
        win32_put_string,
        disk_set_for_text, disk_set_for_graphics,
        disk_set_clear,
        win32_move_cursor, win32_hide_text_cursor,
        win32_set_attr,
        win32_scroll_up,
        win32_stack_screen, win32_unstack_screen, win32_discard_screen,
        win32_init_fm, win32_buzzer, win32_sound_on, win32_sound_off, win32_mute,
        disk_diskp,
        win32_get_char_attr, win32_put_char_attr,
        win32_delay,
        win32_set_keyboard_timeout,
        disk_flush
    },
    0,
    0,
    { 0 }
};

Driver *disk_driver = &disk_driver_info.base.pub;
