/* d_win32.c -- Win32 driver for FractInt */
#include <string.h>

#include "port.h"
#include "cmplx.h"
#include "fractint.h"
#include "drivers.h"
#include "externs.h"
#include "prototyp.h"

typedef struct tagDriverWin32 DriverWin32;
struct tagDriverWin32
{
	Driver pub;
};

static int win32_init(Driver *drv, int *argc, char **argv)
{
  return TRUE;
}

static void win32_terminate(Driver *drv)
{
}

static void win32_flush(Driver *drv)
{
}

static void win32_schedule_alarm(Driver *drv, int secs)
{
}

static int win32_start_video(Driver *drv)
{
    return 0;
}

static int win32_end_video(Driver *drv)
{
    return 0;
}

static void win32_window(Driver *drv)
{
}

static int win32_resize(Driver *drv)
{
    return 0;
}

static void win32_redraw(Driver *drv)
{
}

static int win32_read_palette(Driver *drv)
{
    return 0;
}

static int win32_write_palette(Driver *drv)
{
    return 0;
}

static int win32_read_pixel(Driver *drv, int x, int y)
{
    return 0;
}

static void win32_write_pixel(Driver *drv, int x, int y, int color)
{
}

static void win32_read_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
}

static void win32_write_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
}

static void win32_set_line_mode(Driver *drv, int mode)
{
}

static void win32_draw_line(Driver *drv, int x1, int y1, int x2, int y2, int color)
{
	draw_line(x1, y1, x2, y2, color);
}

static int win32_get_key(Driver *drv)
{
    return 0;
}

static void win32_shell(Driver *drv)
{
}

static void win32_set_video_mode(Driver *drv, int ax, int bx, int cx, int dx)
{
}

static void win32_put_string(Driver *drv, int row, int col, int attr, const char *msg)
{
}

static void win32_set_for_text(Driver *drv)
{
}

static void win32_set_for_graphics(Driver *drv)
{
}

static void win32_set_clear(Driver *drv)
{
}

static BYTE *win32_find_font(Driver *drv, int parm)
{
    return NULL;
}

static void win32_move_cursor(Driver *drv, int row, int col)
{
}

static void win32_hide_text_cursor(Driver *drv)
{
}

static void win32_set_attr(Driver *drv, int row, int col, int attr, int count)
{
}

static void win32_scroll_up(Driver *drv, int top, int bot)
{
}

static void win32_stack_screen(Driver *drv)
{
}

static void win32_unstack_screen(Driver *drv)
{
}

static void win32_discard_screen(Driver *drv)
{
}

static int win32_init_fm(Driver *drv)
{
    return 0;
}

static void win32_buzzer(Driver *drv, int kind)
{
}

static int win32_sound_on(Driver *drv, int frequency)
{
	return 0;
}

static void win32_sound_off(Driver *drv)
{
}

static void win32_mute(Driver *drv)
{
}

static int win32_diskp(Driver *drv)
{
    return 0;
}

static int win32_key_pressed(Driver *drv)
{
	return 0;
}

static int win32_key_cursor(Driver *drv, int row, int col)
{
	return 0;
}

static int win32_wait_key_pressed(Driver *drv, int timeout)
{
	return 0;
}

static int win32_get_char_attr(Driver *drv)
{
	return 0;
}

static void win32_put_char_attr(Driver *drv, int char_attr)
{
}

static int win32_validate_mode(Driver *drv, VIDEOINFO *mode)
{
	return 0;
}

static void win32_unget_key(Driver *drv, int key)
{
}

/* new driver		    old fractint
   -------------------  ------------
   start_video		    startvideo
   end_video		    endvideo
   read_palette		    readvideopalette
   write_palette	    writevideopalette
   read_pixel		    readvideo
   write_pixel		    writevideo
   read_span		    readvideoline
   write_span		    writevideoline
   set_line_mode	    setlinemode
   draw_line		    drawline
   get_key		        getkey
   key_pressed			keypressed
   wait_key_pressed		waitkeypressed
   shell		        shell_to_dos
   set_video_mode	    setvideomode
   set_for_text		    setfortext
   set_for_graphics	    setforgraphics
   set_clear		    setclear
   find_font		    findfont
   move_cursor		    movecursor
   set_attr		        setattr
   scroll_up		    scrollup
   stack_screen		    stackscreen
   unstack_screen	    unstackscreen
   discard_screen	    discardscreen
   init_fm		        initfm
   buzzer		        buzzer
   sound_on		        soundon
   sound_off		    soundoff
   mute					mute
   diskp                diskp
   delay				delay
*/
static DriverWin32 win32_driver_info =
{
	STD_DRIVER_STRUCT(win32, "A GDI driver for 32-bit Windows."),
};

Driver *win32_driver = &win32_driver_info.pub;
