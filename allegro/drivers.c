#include <string.h>

#include "port.h"
#include "prototyp.h"
#include "externs.h"
#include "cmplx.h"

extern Driver *disk_driver;
extern Driver *win32_driver;
extern Driver *x11_driver;
extern Driver *algro_driver;

/* list of drivers that are supported by source code in fractint */
/* default driver is first one in the list that initializes. */
#define MAX_DRIVERS 3
static int num_drivers = 0;
static Driver *available[MAX_DRIVERS];
static Driver *mode_drivers[MAXVIDEOMODES];

Driver *display = NULL;

#define NUM_OF(array_) (sizeof(array_)/sizeof(array_[0]))

static int
load_driver(Driver *drv, int *argc, char **argv)
{
  if (drv && drv->init) {
    const int num = (*drv->init)(drv, argc, argv);
    if (num > 0) {
      if (! display)
	display = drv;
      available[num_drivers++] = drv;
      return 1;
    }
  }

  return 0;
}

/*------------------------------------------------------------
 * init_drivers
 *
 * Go through the static list of drivers defined and try to initialize
 * them one at a time.  Returns the number of drivers initialized.
 */
#define LOAD_DRIVER(name_) init_single_driver(name_##_driver)
int
init_drivers(int *argc, char **argv)
{
  int i;
  FILE *cfgfile;
  char tempstring[150];

  vidtbl = (VIDEOINFO *) malloc(sizeof(VIDEOINFO)*MAXVIDEOMODES);
  vidtbllen = 0;

  findpath("fractint.cfg",tempstring);
  if (tempstring[0] == 0                            /* can't find the file */
    || (cfgfile = fopen(tempstring,"r")) == NULL)   /* can't open it */
      badconfig = -1;
  if (cfgfile != NULL) {
    fclose(cfgfile);
#if 0
    load_fractint_cfg(0); /* change this once we can successfully load
                             fractint.cfg FIXME JCO */
#else
    badconfig = -1;
#endif
  }

#if HAVE_X11_DRIVER
  load_driver(x11_driver, argc, argv);
#endif
#if HAVE_DISK_DRIVER
  load_driver(disk_driver, argc, argv);
#endif
#if HAVE_ALLEGRO_DRIVER
  load_driver(algro_driver, argc, argv);
#endif

  return num_drivers;		/* number of drivers supported at runtime */
}

/* add_video_mode
 *
 * a driver uses this to inform the system of an available video mode
 */
void
add_video_mode(Driver *drv, VIDEOINFO *mode)
{
  /* stash away driver pointer so we can init driver for selected mode */
  mode_drivers[vidtbllen] = drv;
  memcpy(&videotable[vidtbllen], mode, sizeof(videotable[0]));
  vidtbllen++;
}

void
close_drivers(void)
{
  int i;

  for (i = 0; i < num_drivers; i++)
    if (available[i]) {
      (*available[i]->terminate)(available[i]);
      available[i] = NULL;
    }

  if (vidtbl)
    free(vidtbl);
  vidtbl = NULL;
  display = NULL;
}

int
load_fractint_cfg(int options)
{
   /* Reads fractint.cfg, loading videoinfo entries into memory.   */
   /* Sets vidtbl pointing to the loaded table, and returns the    */
   /* number of entries (also sets vidtbllen to this).             */
   /* Past vidtbl, cfglinenums are stored for update_fractint_cfg. */
   /* If fractint.cfg is not found or invalid, issues a message    */
   /* (first time the problem occurs only, and only if options is  */
   /* zero) and generates a new fractint.cfg file.                 */

   FILE *cfgfile;
   VIDEOINFO *vident;
   int far *cfglinenums;
   int linenum;
   int xdots, ydots;
   int i, j, keynum, R, G, B, A, file_dotmode, colors;
   int commas[10];
   int textsafe2;
   char tempstring[150];
   int truecolorbits; 

   cfglinenums = (int far *)(&vidtbl[MAXVIDEOMODES]);

   if (badconfig)  /* fractint.cfg already known to be missing or bad */
      goto generate_table;

   findpath("fractint.cfg",tempstring);
   cfgfile = fopen(tempstring,"r");  /* file tested and known to be present earlier */

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
      xdots       = atoi(&tempstring[commas[1]]);
      ydots       = atoi(&tempstring[commas[2]]);
      colors      = atoi(&tempstring[commas[3]]);
      if(colors == 4 && strchr(strlwr(&tempstring[commas[3]]),'g'))
      {
         colors = 16777216;
         truecolorbits = 32; /* 32 bits */
      }
      else if(colors == 16 && strchr(&tempstring[commas[3]],'m'))
      {
         colors = 16777216;
         truecolorbits = 24; /* 24 bits */
      }
      else if(colors == 64 && strchr(&tempstring[commas[3]],'k'))
      {
         colors = 65536;
         truecolorbits = 16; /* 16 bits */
      }
      else if(colors == 32 && strchr(&tempstring[commas[3]],'k'))
      {
         colors = 32768;
         truecolorbits = 15; /* 15 bits */
      }
      else
      {
         colors = 256;
         truecolorbits = 8; /* 256 colors */
      }

if(colors > 256)
   colors = 256;

      sscanf(&tempstring[commas[4]],"%u",&R);
      sscanf(&tempstring[commas[5]],"%u",&G);
      sscanf(&tempstring[commas[6]],"%u",&B);
      sscanf(&tempstring[commas[7]],"%u",&A);
      if (j < 9 /* FIXME JCO */
          ||  xdots < MINPIXELS || xdots > MAXPIXELS
          ||  ydots < MINPIXELS || ydots > MAXPIXELS
          ||  (colors != 0 && colors != 2 && colors != 4 && colors != 16
               && colors != 256 && colors != 32768 && colors != 65536
               && colors != 16777216)
           )
         goto bad_fractint_cfg;
      cfglinenums[vidtbllen] = linenum; /* for update_fractint_cfg */
      far_memcpy(vident->name,   (char far *)&tempstring[commas[0]],15);
      far_memcpy(vident->comment,(char far *)&tempstring[commas[9]],25); /* FIXME */
      vident->name[15] = vident->comment[25] = 0;
      vident->videomodeR = R;
      vident->videomodeG = G;
      vident->videomodeB = B;
      vident->videomodeA = A;
      if (A == 8)  /* then 32 bits */
         vident->truecolorbits = 32;
      else
         vident->truecolorbits = truecolorbits;
      vident->xdots       = xdots;
      vident->ydots       = ydots;
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

generate_table:
   vidtbllen = 0;
   vident = vidtbl;
   for (i = 0; i < MAXVIDEOTABLE; ++i) {
      if (videotable[i].xdots) {
         memcpy((char *)vident,(char *)&videotable[i],
                sizeof(*vident));
         ++vident;
         ++vidtbllen;
         }
      }
   update_fractint_cfg();
   return (vidtbllen);

}

void
bad_fractint_cfg_msg(void)
{
static char badcfgmsg[]={"\
File FRACTINT.CFG is missing or invalid.\n\
I will generate a new one after determining the video modes available."};
   stopmsg(0,badcfgmsg);
   badconfig = 1; /* bad, message issued */
}

#if 0
void
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
#endif

#if defined(USE_DRIVER_FUNCTIONS)
void
driver_terminate(void)
{
  (*display->terminate)(display);
}

#define METHOD_VOID(name_) \
void driver_##name_(void) { (*display->name_)(display); }
#define METHOD(type_,name_) \
type_ driver_##name_(void) { return (*display->name_)(display); }
#define METHOD_INT(name_) METHOD(int,name_)

METHOD_VOID(flush)

void
driver_schedule_alarm(int soon)
{
  (*display->schedule_alarm)(display, soon);
}

METHOD_INT(start_video)
METHOD_INT(end_video)
METHOD_VOID(window)
METHOD_INT(resize)
METHOD_VOID(redraw)
METHOD_INT(read_palette)
METHOD_INT(write_palette)

int
driver_read_pixel(int x, int y)
{
  return (*display->read_pixel)(display, x, y);
}

void
driver_write_pixel(int x, int y, int color)
{
  (*display->write_pixel)(display, x, y, color);
}

void
driver_read_span(int y, int x, int lastx, BYTE *pixels)
{
  (*display->read_span)(display, y, x, lastx, pixels);
}

void
driver_write_span(int y, int x, int lastx, BYTE *pixels)
{
  (*display->write_span)(display, y, x, lastx, pixels);
}

void
driver_set_line_mode(int mode)
{
  (*display->set_line_mode)(display, mode);
}

void
driver_draw_line(int x1, int y1, int x2, int y2)
{
  (*display->draw_line)(display, x1, y1, x2, y2);
}

int
driver_get_key(int block)
{
  return (*display->get_key)(display, block);
}

METHOD_VOID(shell)

void
driver_set_video_mode(int ax, int bx, int cx, int dx)
{
  (*display->set_video_mode)(display, ax, bx, cx, dx);
}

void
driver_put_string(int row, int col, int attr, const char *msg)
{
  (*display->put_string)(display, row, col, attr, msg);
}

METHOD_VOID(set_for_text)
METHOD_VOID(set_for_graphics)
METHOD_VOID(set_clear)

BYTE *
driver_find_font(int parm)
{
  return (*display->find_font)(display, parm);
}

void
driver_move_cursor(int row, int col)
{
  (*display->move_cursor)(display, row, col);
}

METHOD_VOID(hide_text_cursor)

void
driver_set_attr(int row, int col, int attr, int count)
{
  (*display->set_attr)(display, row, col, attr, count);
}

void
driver_scroll_up(int top, int bot)
{
  (*display->scroll_up)(display, top, bot);
}

METHOD_VOID(stack_screen)
METHOD_VOID(unstack_screen)
METHOD_VOID(discard_screen)

METHOD_INT(init_fm)

void
driver_delay(long time)
{
  (*display->delay)(display, time);
}

void
driver_buzzer(int kind)
{
  (*display->buzzer)(display, kind);
}

int
driver_sound_on(int freq)
{
  return (*display->sound_on)(display, freq);
}

METHOD_VOID(sound_off)
METHOD_INT(diskp)

#endif
