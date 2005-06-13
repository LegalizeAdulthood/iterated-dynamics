/*
        FRACTINT - The Ultimate Fractal Generator
                        Main Routine
*/

#include <string.h>
#include <time.h>
#include <signal.h>

#ifndef XFRACT
#include <io.h>
#endif

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <ctype.h>

  /* #include hierarchy for fractint is a follows:
        Each module should include port.h as the first fractint specific
            include. port.h includes <stdlib.h>, <stdio.h>, <math.h>,
            <float.h>; and, ifndef XFRACT, <dos.h>.
        Most modules should include prototyp.h, which incorporates by
            direct or indirect reference the following header files:
                mpmath.h
                cmplx.h
                fractint.h
                big.h
                biginit.h
                helpcom.h
                externs.h
        Other modules may need the following, which must be included
            separately:
                fractype.h
                helpdefs.h
                lsys.y
                targa.h
                targa_lc.h
                tplus.h
        If included separately from prototyp.h, big.h includes cmplx.h
           and biginit.h; and mpmath.h includes cmplx.h
   */

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

struct videoinfo videoentry;
int helpmode;

long timer_start,timer_interval;        /* timer(...) start & total */
int     adapter;                /* Video Adapter chosen from list in ...h */
char *fract_dir1="", *fract_dir2="";

#ifdef __TURBOC__

/* yes, I *know* it's supposed to be compatible with Microsoft C,
   but some of the routines need to know if the "C" code
   has been compiled with Turbo-C.  This flag is a 1 if FRACTINT.C
   (and presumably the other routines as well) has been compiled
   with Turbo-C. */
int compiled_by_turboc = 1;

/* set size to be used for overlays, a bit bigger than largest (help) */
unsigned _ovrbuffer = 54 * 64; /* that's 54k for overlays, counted in paragraphs */

#else

int compiled_by_turboc = 0;

#endif

/*
   the following variables are out here only so
   that the calcfract() and assembler routines can get at them easily
*/
        int     active_system = 0;      /* 0 for DOS, WINFRAC for Windows */
        int     dotmode;                /* video access method      */
        int     textsafe2;              /* textsafe override from videotable */
        int     oktoprint;              /* 0 if printf() won't work */
        int     sxdots,sydots;          /* # of dots on the physical screen    */
        int     sxoffs,syoffs;          /* physical top left of logical screen */
        int     xdots, ydots;           /* # of dots on the logical screen     */
        double  dxsize, dysize;         /* xdots-1, ydots-1         */
        int     colors = 256;           /* maximum colors available */
        long    maxit;                  /* try this many iterations */
        int     boxcount;               /* 0 if no zoom-box yet     */
        int     zrotate;                /* zoombox rotation         */
        double  zbx,zby;                /* topleft of zoombox       */
        double  zwidth,zdepth,zskew;    /* zoombox size & shape     */

        int     fractype;               /* if == 0, use Mandelbrot  */
        char    stdcalcmode;            /* '1', '2', 'g', 'b'       */
        long    creal, cimag;           /* real, imag'ry parts of C */
        long    delx, dely;             /* screen pixel increments  */
        long    delx2, dely2;           /* screen pixel increments  */
        LDBL    delxx, delyy;           /* screen pixel increments  */
        LDBL    delxx2, delyy2;         /* screen pixel increments  */
        long    delmin;                 /* for calcfrac/calcmand    */
        double  ddelmin;                /* same as a double         */
        double  param[MAXPARAMS];       /* parameters               */
        double  potparam[3];            /* three potential parameters*/
        long    fudge;                  /* 2**fudgefactor           */
        long    l_at_rad;               /* finite attractor radius  */
        double  f_at_rad;               /* finite attractor radius  */
        int     bitshift;               /* fudgefactor              */

        int     badconfig = 0;          /* 'fractint.cfg' ok?       */
        int     diskisactive;           /* disk-video drivers flag  */
        int     diskvideo;              /* disk-video access flag   */
        int hasinverse = 0;
        /* note that integer grid is set when integerfractal && !invert;    */
        /* otherwise the floating point grid is set; never both at once     */
        long    far *lx0, far *ly0;     /* x, y grid                */
        long    far *lx1, far *ly1;     /* adjustment for rotate    */
        /* note that lx1 & ly1 values can overflow into sign bit; since     */
        /* they're used only to add to lx0/ly0, 2s comp straightens it out  */
        double far *dx0, far *dy0;      /* floating pt equivs */
        double far *dx1, far *dy1;
        int     integerfractal;         /* TRUE if fractal uses integer math */

        /* usr_xxx is what the user wants, vs what we may be forced to do */
        char    usr_stdcalcmode;
        int     usr_periodicitycheck;
        long    usr_distest;
        char    usr_floatflag;

        int     viewwindow;             /* 0 for full screen, 1 for window */
        float   viewreduction;          /* window auto-sizing */
        int     viewcrop;               /* nonzero to crop default coords */
        float   finalaspectratio;       /* for view shape and rotation */
        int     viewxdots,viewydots;    /* explicit view sizing */
        int     video_cutboth;          /* nonzero to keep virtual aspect */
        int     zscroll;                /* screen/zoombox 0 fixed, 1 relaxed */

/*      HISTORY  far *history = NULL; */
        U16 history = 0;
        int maxhistory = 10;

/* variables defined by the command line/files processor */
int     comparegif=0;                   /* compare two gif files flag */
int     timedsave=0;                    /* when doing a timed save */
int     resave_flag=0;                  /* tells encoder not to incr filename */
int     started_resaves=0;              /* but incr on first resave */
int     save_system;                    /* from and for save files */
int     tabmode = 1;                    /* tab display enabled */

/* for historical reasons (before rotation):         */
/*    top    left  corner of screen is (xxmin,yymax) */
/*    bottom left  corner of screen is (xx3rd,yy3rd) */
/*    bottom right corner of screen is (xxmax,yymin) */
double  xxmin,xxmax,yymin,yymax,xx3rd,yy3rd; /* selected screen corners  */
long    xmin, xmax, ymin, ymax, x3rd, y3rd;  /* integer equivs           */
double  sxmin,sxmax,symin,symax,sx3rd,sy3rd; /* displayed screen corners */
double  plotmx1,plotmx2,plotmy1,plotmy2;     /* real->screen multipliers */

int calc_status = -1; /* -1 no fractal                   */
                      /*  0 parms changed, recalc reqd   */
                      /*  1 actively calculating         */
                      /*  2 interrupted, resumable       */
                      /*  3 interrupted, not resumable   */
                      /*  4 completed                    */
long calctime;

int max_colors;                         /* maximum palette size */
int        zoomoff;                     /* = 0 when zoom is disabled    */
int        savedac;                     /* save-the-Video DAC flag      */
int browsing;                 /* browse mode flag */
char file_name_stack[16][13]; /* array of file names used while browsing */
int name_stack_ptr ;
double toosmall;
int  minbox;
int no_sub_images;
int autobrowse,doublecaution;
char brwscheckparms,brwschecktype;
char browsemask[13];
int scale_map[12] = {1,2,3,4,5,6,7,8,9,10,11,12}; /*RB, array for mapping notes to a (user defined) scale */


#define RESTART           1
#define IMAGESTART        2
#define RESTORESTART      3
#define CONTINUE          4

static void check_samename(void)
   {
      char drive[FILE_MAX_DRIVE];
      char dir[FILE_MAX_DIR];
      char fname[FILE_MAX_FNAME];
      char ext[FILE_MAX_EXT];
      char path[FILE_MAX_PATH];
      splitpath(savename,drive,dir,fname,ext);
      if(strcmp(fname,"fract001"))
      {
         makepath(path,drive,dir,fname,"gif");
         if(access(path,0)==0)
         exit(0);
      }
   }

/* Do nothing if math error */
static void my_floating_point_err(int sig)
{
   if(sig != 0)
      overflow = 1;
}

#ifdef XFRACT
int
#else
void
#endif
main(int argc, char **argv)
{
   int     resumeflag;
   int     kbdchar;                     /* keyboard key-hit value       */
   int     kbdmore;                     /* continuation variable        */
   char stacked=0;                      /* flag to indicate screen stacked */

   /* this traps non-math library floating point errors */
   signal( SIGFPE, my_floating_point_err );

   initasmvars();                       /* initialize ASM stuff */
   InitMemory();
   checkfreemem(0);
   load_videotable(1); /* load fractint.cfg, no message yet if bad */
#ifdef XFRACT
   UnixInit();
#endif
   init_help();

restart:   /* insert key re-starts here */
   autobrowse     = FALSE;
   brwschecktype  = TRUE;
   brwscheckparms = TRUE;
   doublecaution  = TRUE;
   no_sub_images = FALSE;
   toosmall = 6;
   minbox   = 3;
   strcpy(browsemask,"*.gif");
   strcpy(browsename,"            ");
   name_stack_ptr= -1; /* init loaded files stack */
   
   evolving = FALSE;
   paramrangex = 4;
   opx = newopx = -2.0;
   paramrangey = 3;
   opy = newopy = -1.5;
   odpx = odpy = 0;
   gridsz = 9;
   fiddlefactor = 1;
   fiddle_reduction = 1.0;
   this_gen_rseed = (unsigned int)clock_ticks();
   srand(this_gen_rseed);
   initgene(); /*initialise pointers to lots of fractint variables for the evolution engine*/
   start_showorbit = 0;
   showdot = -1; /* turn off showdot if entered with <g> command */
   calc_status = -1;                    /* no active fractal image */

   fract_dir1 = getenv("FRACTDIR");
   if (fract_dir1==NULL) {
       fract_dir1 = ".";
   }
#ifdef SRCDIR
   fract_dir2 = SRCDIR;
#else
   fract_dir2 = ".";
#endif

   cmdfiles(argc,argv);         /* process the command-line */
   dopause(0);                  /* pause for error msg if not batch */
   init_msg(0,"",NULL,0);  /* this causes getakey if init_msg called on runup */
   checkfreemem(1);
   if(debugflag==450 && initbatch==1)   /* abort if savename already exists */
       check_samename();
#ifdef XFRACT
   initUnixWindow();
#endif
   memcpy(olddacbox,dacbox,256*3);      /* save in case colors= present */

   if (debugflag == 8088)                cpu =  86; /* for testing purposes */
   if (debugflag == 2870 && fpu >= 287 ) {
      fpu = 287; /* for testing purposes */
      cpu = 286;
   }
   if (debugflag ==  870 && fpu >=  87 ) {
      fpu =  87; /* for testing purposes */
      cpu =  86;
   }
   if (debugflag ==   70)                fpu =   0; /* for testing purposes */
   if (getenv("NO87")) fpu = 0;

   if (fpu >= 287 && debugflag != 72)   /* Fast 287 math */
      setup287code();
   adapter_detect();                    /* check what video is really present */
   if (debugflag >= 9002 && debugflag <= 9100) /* for testing purposes */
      if (video_type > (debugflag-9000)/2)     /* adjust the video value */
         video_type = (debugflag-9000)/2;

   diskisactive = 0;                    /* disk-video is inactive */
   diskvideo = 0;                       /* disk driver is not in use */
   setvideotext();                      /* switch to text mode */
   savedac = 0;                         /* don't save the VGA DAC */

#ifndef XFRACT
   if (debugflag == 10000)              /* check for free memory */
      showfreemem();

   if (badconfig < 0)                   /* fractint.cfg bad, no msg yet */
      bad_fractint_cfg_msg();
#endif

   max_colors = 256;                    /* the Windows version is lower */
   max_kbdcount=(cpu>=386) ? 80 : 30;   /* check the keyboard this often */

   if (showfile && initmode < 0) {
      intro();                          /* display the credits screen */
      if (keypressed() == ESC) {
         getakey();
         goodbye();
         }
      }

   browsing = FALSE;

   if (!functionpreloaded)
      set_if_old_bif();
   stacked = 0;
restorestart:
   if (colorpreloaded)
      memcpy(dacbox,olddacbox,256*3);   /* restore in case colors= present */

   lookatmouse = 0;                     /* ignore mouse */

   while (showfile <= 0) {              /* image is to be loaded */
      char *hdg;
      tabmode = 0;
      if (!browsing )     /*RB*/
      {
      if (overlay3d) {
         hdg = "Select File for 3D Overlay";
         helpmode = HELP3DOVLY;
         }
      else if (display3d) {
         hdg = "Select File for 3D Transform";
         helpmode = HELP3D;
         }
      else {
         hdg = "Select File to Restore";
         helpmode = HELPSAVEREST;
         }
      if (showfile < 0 && getafilename(hdg,gifmask,readname) < 0) {
         showfile = 1;               /* cancelled */
         initmode = -1;
         break;
         }

           name_stack_ptr = 0; /* 'r' reads first filename for browsing */
           strcpy(file_name_stack[name_stack_ptr],browsename);
     }

      evolving = viewwindow = 0;
      showfile = 0;
      helpmode = -1;
      tabmode = 1;
      if(stacked)
      {
         discardscreen();
         setvideotext();
         stacked = 0;
      }
      if (read_overlay() == 0)       /* read hdr, get video mode */
         break;                      /* got it, exit */
      if (browsing) /* break out of infinite loop, but lose your mind */
         showfile = 1;
      else
         showfile = -1;                 /* retry */
      }

   helpmode = HELPMENU;                 /* now use this help mode */
   tabmode = 1;
   lookatmouse = 0;                     /* ignore mouse */

   if (((overlay3d && !initbatch) || stacked) && initmode < 0) {        /* overlay command failed */
      unstackscreen();                  /* restore the graphics screen */
      stacked = 0;
      overlay3d = 0;                    /* forget overlays */
      display3d = 0;                    /* forget 3D */
      if (calc_status ==3)
         calc_status = 0;
      resumeflag = 1;
      goto resumeloop;                  /* ooh, this is ugly */
      }

   savedac = 0;                         /* don't save the VGA DAC */
imagestart:                             /* calc/display a new image */
   if(stacked)
   {
      discardscreen();
      stacked = 0;
   }
#ifdef XFRACT
   usr_floatflag = 1;
#endif
   got_status = -1;                     /* for tab_display */

   if (showfile)
      if (calc_status > 0)              /* goto imagestart implies re-calc */
         calc_status = 0;

   if (initbatch == 0)
      lookatmouse = -PAGE_UP;           /* just mouse left button, == pgup */

   cyclelimit = initcyclelimit;         /* default cycle limit   */


   adapter = initmode;                  /* set the video adapter up */
   initmode = -1;                       /* (once)                   */

   while (adapter < 0) {                /* cycle through instructions */
      if (initbatch) {                          /* batch, nothing to do */
         initbatch = 4;                 /* exit with error condition set */
         goodbye();
      }
      kbdchar = main_menu(0);
      if (kbdchar == INSERT) goto restart;      /* restart pgm on Insert Key */
      if (kbdchar == DELETE)                    /* select video mode list */
         kbdchar = select_video_mode(-1);
      if ((adapter = check_vidmode_key(0,kbdchar)) >= 0)
         break;                                 /* got a video mode now */
#ifndef XFRACT
      if ('A' <= kbdchar && kbdchar <= 'Z')
         kbdchar = tolower(kbdchar);
#endif
      if (kbdchar == 'd') {                     /* shell to DOS */
         setclear();
#ifndef XFRACT
         printf("\n\nShelling to DOS - type 'exit' to return\n\n");
#else
         printf("\n\nShelling to Linux/Unix - type 'exit' to return\n\n");
#endif
         shell_to_dos();
         goto imagestart;
         }

#ifndef XFRACT
      if (kbdchar == '@' || kbdchar == '2') {    /* execute commands */
#else
      if (kbdchar == F2 || kbdchar == '@') {     /* We mapped @ to F2 */
#endif
         if ((get_commands() & 4) == 0)
            goto imagestart;
         kbdchar = '3';                         /* 3d=y so fall thru '3' code */
         }
#ifndef XFRACT
      if (kbdchar == 'r' || kbdchar == '3' || kbdchar == '#') {
#else
      if (kbdchar == 'r' || kbdchar == '3' || kbdchar == F3) {
#endif
         display3d = 0;
         if (kbdchar == '3' || kbdchar == '#' || kbdchar == F3)
            display3d = 1;
         if(colorpreloaded)
            memcpy(olddacbox,dacbox,256*3);     /* save in case colors= present */
         setvideotext(); /* switch to text mode */
         showfile = -1;
         goto restorestart;
         }
      if (kbdchar == 't') {                     /* set fractal type */
         julibrot = 0;
         get_fracttype();
         goto imagestart;
         }
      if (kbdchar == 'x') {                     /* generic toggle switch */
         get_toggles();
         goto imagestart;
         }
      if (kbdchar == 'y') {                     /* generic toggle switch */
         get_toggles2();
         goto imagestart;
         }
      if (kbdchar == 'z') {                     /* type specific parms */
         get_fract_params(1);
         goto imagestart;
         }
      if (kbdchar == 'v') {                     /* view parameters */
         get_view_params();
         goto imagestart;
         }
      if (kbdchar == 2) {                       /* ctrl B = browse parms*/
         get_browse_params();
         goto imagestart;
         }
      if (kbdchar == 6) {                       /* ctrl f = sound parms*/
         get_sound_params();
         goto imagestart;
         }
      if (kbdchar == 'f') {                     /* floating pt toggle */
         if (usr_floatflag == 0)
            usr_floatflag = 1;
         else
            usr_floatflag = 0;
         goto imagestart;
         }
      if (kbdchar == 'i') {                     /* set 3d fractal parms */
         get_fract3d_params(); /* get the parameters */
         goto imagestart;
         }
      if (kbdchar == 'g') {
         get_cmd_string(); /* get command string */
         goto imagestart;
         }
      /* buzzer(2); */                          /* unrecognized key */
      }

   zoomoff = 1;                 /* zooming is enabled */
   helpmode = HELPMAIN;         /* now use this help mode */
   resumeflag = 0;  /* allows taking goto inside big_while_loop() */
resumeloop:
   param_history(0); /* save old history */
   /* this switch processes gotos that are now inside function */
   switch(big_while_loop(&kbdmore,&stacked,resumeflag))
   {
   case RESTART:
      goto restart;
   case IMAGESTART:
      goto imagestart;
   case RESTORESTART:
      goto restorestart;
   default:
      break;
   }
#ifdef XFRACT
   return(0);
#endif
}

int check_key()
{
   int key;
   if((key = keypressed()) != 0) {
      if (show_orbit)
         scrub_orbit();
      if(key != 'o' && key != 'O') {
         fflush(stdout);
         return(-1);
      }
      getakey();
      if (dotmode != 11)
         show_orbit = 1 - show_orbit;
   }
   return(0);
}

/* timer function:
     timer(0,(*fractal)())              fractal engine
     timer(1,NULL,int width)            decoder
     timer(2)                           encoder
  */
#ifndef USE_VARARGS
int timer(int timertype,int(*subrtn)(),...)
#else
int timer(va_alist)
va_dcl
#endif
{
   va_list arg_marker;  /* variable arg list */
   char *timestring;
   time_t ltime;
   FILE *fp = NULL;
   int out=0;
   int i;
   int do_bench;

#ifndef USE_VARARGS
   va_start(arg_marker,subrtn);
#else
   int timertype;
   int (*subrtn)();
   va_start(arg_marker);
   timertype = va_arg(arg_marker, int);
   subrtn = (int (*)())va_arg(arg_marker, int *);
#endif

   do_bench = timerflag; /* record time? */
   if (timertype == 2)   /* encoder, record time only if debug=200 */
      do_bench = (debugflag == 200);
   if(do_bench)
      fp=dir_fopen(workdir,"bench","a");
   timer_start = clock_ticks();
   switch(timertype) {
      case 0:
         out = (*(int(*)(void))subrtn)();
         break;
      case 1:
         i = va_arg(arg_marker,int);
         out = (int)decoder((short)i); /* not indirect, safer with overlays */
         break;
      case 2:
         out = encoder();            /* not indirect, safer with overlays */
         break;
      }
   /* next assumes CLK_TCK is 10^n, n>=2 */
   timer_interval = (clock_ticks() - timer_start) / (CLK_TCK/100);

   if(do_bench) {
      time(&ltime);
      timestring = ctime(&ltime);
      timestring[24] = 0; /*clobber newline in time string */
      switch(timertype) {
         case 1:
            fprintf(fp,"decode ");
            break;
         case 2:
            fprintf(fp,"encode ");
            break;
         }
      fprintf(fp,"%s type=%s resolution = %dx%d maxiter=%ld",
          timestring,
          curfractalspecific->name,
          xdots,
          ydots,
          maxit);
      fprintf(fp," time= %ld.%02ld secs\n",timer_interval/100,timer_interval%100);
      if(fp != NULL)
         fclose(fp);
      }
   return(out);
}
