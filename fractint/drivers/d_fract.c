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

static int
load_fractint_cfg(int options)
{
   /* Reads fractint.cfg, loading videoinfo entries into extraseg. */
   /* Sets vidtbl pointing to the loaded table, and returns the    */
   /* number of entries (also sets vidtbllen to this).             */
   /* Past vidtbl, cfglinenums are stored for update_fractint_cfg. */
   /* If fractint.cfg is not found or invalid, issues a message    */
   /* (first time the problem occurs only, and only if options is  */
   /* zero) and uses the hard-coded table.                         */

   FILE *cfgfile;
   VIDEOINFO *vident;
   int far *cfglinenums;
   int linenum;
   long xdots, ydots;
   int i, j, keynum, ax, bx, cx, dx, file_dotmode, colors;
   int commas[10];
   int textsafe2;
   char tempstring[150];
   int truecolorbits; 

   vidtbl = MK_FP(extraseg,0);
   cfglinenums = (int far *)(&vidtbl[MAXVIDEOMODES]);

   if (badconfig)  /* fractint.cfg already known to be missing or bad */
      goto use_resident_table;

   findpath("fractint.cfg",tempstring);
   if (tempstring[0] == 0                            /* can't find the file */
     || (cfgfile = fopen(tempstring,"r")) == NULL)   /* can't open it */
      goto bad_fractint_cfg;

   vidtbllen = 0;
   linenum = 0;
   vident = vidtbl;
   while (vidtbllen < MAXVIDEOMODES
     && fgets(tempstring, 120, cfgfile)) {
      ++linenum;
      if (tempstring[0] == ';') continue;   /* comment line */
      tempstring[120] = 0;
      tempstring[strlen(tempstring)-1] = 0; /* zap trailing \n */
      memset(commas,0,20);
      i = j = -1;
      for(;;) {
         if (tempstring[++i] < ' ') {
            if (tempstring[i] == 0) break;
            tempstring[i] = ' '; /* convert tab (or whatever) to blank */
            }
         else if (tempstring[i] == ',' && ++j < 10) {
            commas[j] = i + 1;   /* remember start of next field */
            tempstring[i] = 0;   /* make field a separate string */
            }
         }
      keynum = check_vidmode_keyname(tempstring);
      sscanf(&tempstring[commas[1]],"%x",&ax);
      sscanf(&tempstring[commas[2]],"%x",&bx);
      sscanf(&tempstring[commas[3]],"%x",&cx);
      sscanf(&tempstring[commas[4]],"%x",&dx);
      file_dotmode     = atoi(&tempstring[commas[5]]);
      xdots       = atol(&tempstring[commas[6]]);
      ydots       = atol(&tempstring[commas[7]]);
      colors      = atoi(&tempstring[commas[8]]);
      if(colors == 16 && strchr(strlwr(&tempstring[commas[8]]),'m'))
      {
         colors = 256;
         truecolorbits = 3; /* 48 bits */
      }
      else if(colors == 64 && strchr(&tempstring[commas[8]],'k'))
      {
         colors = 256;
         truecolorbits = 2; /* 16 bits */
      }
      else if(colors == 32 && strchr(&tempstring[commas[8]],'k'))
      {
         colors = 256;
         truecolorbits = 1; /* 15 bits */
      }
      else
         truecolorbits = 0;

      textsafe2   = file_dotmode / 100;
      file_dotmode    %= 100;
      if (j != 9 ||
            keynum < 0 ||
            file_dotmode < 0 || file_dotmode > 30 ||
            textsafe2 < 0 || textsafe2 > 4 ||
            xdots < MINPIXELS || xdots > MAXPIXELS ||
            ydots < MINPIXELS || ydots > MAXPIXELS ||
            (colors != 0 && colors != 2 && colors != 4 && colors != 16 &&
             colors != 256)
           )
         goto bad_fractint_cfg;
      cfglinenums[vidtbllen] = linenum; /* for update_fractint_cfg */
      far_memcpy(vident->name,   (char far *)&tempstring[commas[0]],25);
      far_memcpy(vident->comment,(char far *)&tempstring[commas[9]],25);
      vident->name[25] = vident->comment[25] = 0;
      vident->keynum      = keynum;
      vident->videomodeax = ax;
      vident->videomodebx = bx;
      vident->videomodecx = cx;
      vident->videomodedx = dx;
      vident->dotmode =
	truecolorbits * 1000 + textsafe2 * 100 + file_dotmode;
      vident->xdots       = (short)xdots;
      vident->ydots       = (short)ydots;
      vident->colors      = colors;
      ++vident;
      ++vidtbllen;
      }
   fclose(cfgfile);
   return (vidtbllen);

bad_fractint_cfg:
   badconfig = -1; /* bad, no message issued yet */
   if (options == 0)
      bad_fractint_cfg_msg();

use_resident_table:
   vidtbllen = 0;
   vident = vidtbl;
   for (i = 0; i < MAXVIDEOTABLE; ++i) {
      if (videotable[i].xdots) {
         far_memcpy((char far *)vident,(char far *)videotable[i],
                    sizeof(*vident));
         ++vident;
         ++vidtbllen;
         }
      }
   return (vidtbllen);

}

static void
bad_fractint_cfg_msg(void)
{
static char far badcfgmsg[]={"\
File FRACTINT.CFG is missing or invalid.\n\
See Hardware Support and Video Modes in the full documentation for help.\n\
I will continue with only the built-in video modes available."};
   stopmsg(0,badcfgmsg);
   badconfig = 1; /* bad, message issued */
}

static void
load_videotable(int options)
{
   /* Loads fractint.cfg and copies the video modes which are */
   /* assigned to function keys into videotable.              */
   int keyents,i;
   load_fractint_cfg(options); /* load fractint.cfg to extraseg */
   keyents = 0;
   far_memset((char far *)videotable,0,sizeof(*vidtbl)*MAXVIDEOTABLE);
   for (i = 0; i < vidtbllen; ++i) {
      if (vidtbl[i].keynum > 0) {
         far_memcpy((char far *)&videotable[keyents],(char far *)&vidtbl[i],
                    sizeof(*vidtbl));
         if (++keyents >= MAXVIDEOTABLE)
            break;
         }
      }
}

static void
fractint_init(int *argc, char *argv)
{
  load_videotable(1); /* load fractint.cfg, no message yet if bad */
}

static void
fractint_terminate(void)
{
  if (*s_makepar != 0) {
    r.h.al = (char)((mode7text == 0) ? exitmode : 7);
    r.h.ah = 0;
    int86(0x10, &r, &r);
  }
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
   buzzer		buzzer
   sound_on		sound_on
   sound_off		sound_off
*/
Driver fractint_driver = STD_DRIVER_STRUCT(fractint);
