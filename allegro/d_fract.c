#include "drivers.h"

/*
; ********************** Function setvideotext() ************************

;       Sets video to text mode, using setvideomode to do the work.
*/
void
setvideotext()
{
  dotmode = 0;
  driver_set_video_mode(3, 0, 0, 0);
}

/*
; ---- Help (Video) Support
; ********* Functions setfortext() and setforgraphics() ************

;       setfortext() resets the video for text mode and saves graphics data
;       setforgraphics() restores the graphics mode and data
;       setclear() clears the screen after setfortext()
*/
static void
setfortext()
{
}

static void
setforgraphics()
{
  driver_start_video();
  spindac(0,1);
}

static void
fractint_init(int *argc, char *argv)
{
  load_videotable(1); /* load fractint.cfg, no message yet if bad */
}

static void
fractint_terminate(void)
{


}

static void
fractint_hide_text_cursor(void)
{
  fractint_move_cursor(25, 80);
}

/* new driver		old fractint
   -------------------  ------------
   start_video		startvideo
   end_video		endvideo
   read_palette		readvideopalette
   write_palette	writevideopalette
   read_pixel		readvideo
   write_pixel		writevideo
   read_span		readvideoline
   write_span		writevideoline
   set_line_mode	setlinemode
   draw_line		drawline
   get_key		getkey
   shell		shell_to_dos
   set_video_mode	setvideomode
   set_for_text		setfortext
   set_for_graphics	setforgraphics
   set_clear		setclear
   find_font		findfont
   move_cursor		movecursor
   set_attr		setattr
   scroll_up		scrollup
   stack_screen		stackscreen
   unstack_screen	unstackscreen
   discard_screen	discardscreen
   init_fm		initfm
   delay                delay
   buzzer		buzzer
   sound_on		sound_on
   sound_off		sound_off
*/
Driver fractint_driver = STD_DRIVER_STRUCT(fractint);
