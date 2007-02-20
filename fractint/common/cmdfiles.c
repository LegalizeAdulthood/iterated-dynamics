/*
        Command-line / Command-File Parser Routines
*/

#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#if !defined(XFRACT) && !defined(_WIN32)
#include <bios.h>
#endif
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "drivers.h"

#ifdef XFRACT
#define DEFAULT_PRINTER 5       /* Assume a Postscript printer */
#define PRT_RESOLUTION  100     /* Assume medium resolution    */
#define INIT_GIF87      0       /* Turn on GIF 89a processing  */
#else
#define DEFAULT_PRINTER 2       /* Assume an IBM/Epson printer */
#define PRT_RESOLUTION  60      /* Assume low resolution       */
#define INIT_GIF87      0       /* Turn on GIF 89a processing  */
#endif

static int  cmdfile(FILE *,int);
static int  next_command(char *,int,FILE *,char *,int *,int);
static int  next_line(FILE *,char *,int);
int  cmdarg(char *,int);
static void argerror(const char *);
static void initvars_run(void);
static void initvars_restart(void);
static void initvars_fractal(void);
static void initvars_3d(void);
static void reset_ifs_defn(void);
static void parse_textcolors(char *value);
static int  parse_colors(char *value);
//static int  parse_printer(char *value);
static int  get_bf(bf_t, char *);
static int isabigfloat(char *str);

/* variables defined by the command line/files processor */
int     stoppass=0;             /* stop at this guessing pass early */
int     pseudox=0;              /* xdots to use for video independence */
int     pseudoy=0;              /* ydots to use for video independence */
int     bfdigits=0;             /* digits to use (force) for bf_math */
int     showdot=-1;             /* color to show crawling graphics cursor */
int     sizedot;                /* size of dot crawling cursor */
char    recordcolors;           /* default PAR color-writing method */
char    autoshowdot=0;          /* dark, medium, bright */
char    start_showorbit=0;      /* show orbits on at start of fractal */
char    temp1[256];             /* temporary strings        */
char    readname[FILE_MAX_PATH];/* name of fractal input file */
char    tempdir[FILE_MAX_DIR] = {""}; /* name of temporary directory */
char    workdir[FILE_MAX_DIR] = {""}; /* name of directory for misc files */
char    orgfrmdir[FILE_MAX_DIR] = {""};/*name of directory for orgfrm files*/
char    gifmask[FILE_MAX_PATH] = {""};
char    PrintName[FILE_MAX_PATH]={"fract001.prn"}; /* Name for print-to-file */
char    savename[FILE_MAX_PATH]={"fract001"};  /* save files using this name */
char    autoname[FILE_MAX_PATH]={"auto.key"}; /* record auto keystrokes here */
int     potflag=0;              /* continuous potential enabled? */
int     pot16bit;               /* store 16 bit continuous potential values */
int     gif87a_flag;            /* 1 if GIF87a format, 0 otherwise */
int     dither_flag;            /* 1 if want to dither GIFs */
int     askvideo;               /* flag for video prompting */
char    floatflag;
int     biomorph;               /* flag for biomorph */
int     usr_biomorph;
int     forcesymmetry;          /* force symmetry */
int     showfile;               /* zero if file display pending */
int     rflag, rseed;           /* Random number seeding flag and value */
int     decomp[2];              /* Decomposition coloring */
long    distest;
int     distestwidth;
char    fract_overwrite = 0;	/* 0 if file overwrite not allowed */
int     soundflag;              /* sound control bitfield... see sound.c for useage*/
int     basehertz;              /* sound=x/y/x hertz value */
int     debugflag;              /* internal use only - you didn't see this */
int     timerflag;              /* you didn't see this, either */
int     cyclelimit;             /* color-rotator upper limit */
int     inside;                 /* inside color: 1=blue     */
int     fillcolor;              /* fillcolor: -1=normal     */
int     outside;                /* outside color    */
int     finattract;             /* finite attractor logic */
int     display3d;              /* 3D display flag: 0 = OFF */
int     overlay3d;              /* 3D overlay flag: 0 = OFF */
int     init3d[20];             /* '3d=nn/nn/nn/...' values */
int     checkcurdir;            /* flag to check current dir for files */
int     initbatch;              /* 1 if batch run (no kbd)  */
int     initsavetime;           /* autosave minutes         */
_CMPLX  initorbit;              /* initial orbitvalue */
char    useinitorbit;           /* flag for initorbit */
int     g_init_mode;               /* initial video mode       */
int     initcyclelimit;         /* initial cycle limit      */
BYTE    usemag;                 /* use center-mag corners   */
long    bailout;                /* user input bailout value */
enum bailouts bailoutest;       /* test used for determining bailout */
double  inversion[3];           /* radius, xcenter, ycenter */
int     rotate_lo,rotate_hi;    /* cycling color range      */
int *ranges;                /* iter->color ranges mapping */
int     rangeslen = 0;          /* size of ranges array     */
BYTE *mapdacbox = NULL;     /* map= (default colors)    */
int     colorstate;             /* 0, g_dac_box matches default (bios or map=) */
                                /* 1, g_dac_box matches no known defined map   */
                                /* 2, g_dac_box matches the colorfile map      */
int     colorpreloaded;         /* if g_dac_box preloaded for next mode select */
int     save_release;           /* release creating PAR file*/
char    dontreadcolor=0;        /* flag for reading color from GIF */
double  math_tol[2]={.05,.05};  /* For math transition */
int Targa_Out = 0;              /* 3D fullcolor flag */
int truecolor = 0;              /* escape time truecolor flag */
int truemode = 0;               /* truecolor coloring scheme */
char    colorfile[FILE_MAX_PATH];/* from last <l> <s> or colors=@filename */
int functionpreloaded; /* if function loaded for new bifs, JCO 7/5/92 */
float   screenaspect = DEFAULTASPECT;   /* aspect ratio of the screen */
float   aspectdrift = DEFAULTASPECTDRIFT;  /* how much drift is allowed and */
                                           /* still forced to screenaspect  */
int fastrestore = 0;          /* 1 - reset viewwindows prior to a restore
                                     and do not display warnings when video
                                     mode changes during restore */

int orgfrmsearch = 0;            /* 1 - user has specified a directory for
                                     Orgform formula compilation files */

int     orbitsave = 0;          /* for IFS and LORENZ to output acrospin file */
int orbit_delay;                /* clock ticks delating orbit release */
int     transparent[2];         /* transparency min/max values */
long    LogFlag;                /* Logarithmic palette flag: 0 = no */

BYTE exitmode = 3;      /* video mode on exit */

int     Log_Fly_Calc = 0;   /* calculate logmap on-the-fly */
int     Log_Auto_Calc = 0;  /* auto calculate logmap */
int     nobof = 0; /* Flag to make inside=bof options not duplicate bof images */

int        escape_exit;         /* set to 1 to avoid the "are you sure?" screen */
int first_init=1;               /* first time into cmdfiles? */
static int init_rseed;
static char initcorners,initparams;
struct fractalspecificstuff *curfractalspecific = NULL;

char FormFileName[FILE_MAX_PATH];/* file to find (type=)formulas in */
char FormName[ITEMNAMELEN+1];    /* Name of the Formula (if not null) */
char LFileName[FILE_MAX_PATH];   /* file to find (type=)L-System's in */
char LName[ITEMNAMELEN+1];       /* Name of L-System */
char CommandFile[FILE_MAX_PATH]; /* file to find command sets in */
char CommandName[ITEMNAMELEN+1]; /* Name of Command set */
char CommandComment[4][MAXCMT];    /* comments for command set */
char IFSFileName[FILE_MAX_PATH];/* file to find (type=)IFS in */
char IFSName[ITEMNAMELEN+1];    /* Name of the IFS def'n (if not null) */
struct SearchPath searchfor;
float *ifs_defn = NULL;     /* ifs parameters */
int  ifs_type;                  /* 0=2d, 1=3d */
int  g_slides = SLIDES_OFF;                /* 1 autokey=play, 2 autokey=record */

BYTE txtcolor[]={
      BLUE*16+L_WHITE,    /* C_TITLE           title background */
      BLUE*16+L_GREEN,    /* C_TITLE_DEV       development vsn foreground */
      GREEN*16+YELLOW,    /* C_HELP_HDG        help page title line */
      WHITE*16+BLACK,     /* C_HELP_BODY       help page body */
      GREEN*16+GRAY,      /* C_HELP_INSTR      help page instr at bottom */
      WHITE*16+BLUE,      /* C_HELP_LINK       help page links */
      CYAN*16+BLUE,       /* C_HELP_CURLINK    help page current link */
      WHITE*16+GRAY,      /* C_PROMPT_BKGRD    prompt/choice background */
      WHITE*16+BLACK,     /* C_PROMPT_TEXT     prompt/choice extra info */
      BLUE*16+WHITE,      /* C_PROMPT_LO       prompt/choice text */
      BLUE*16+L_WHITE,    /* C_PROMPT_MED      prompt/choice hdg2/... */
      BLUE*16+YELLOW,     /* C_PROMPT_HI       prompt/choice hdg/cur/... */
      GREEN*16+L_WHITE,   /* C_PROMPT_INPUT    fullscreen_prompt input */
      CYAN*16+L_WHITE,    /* C_PROMPT_CHOOSE   fullscreen_prompt choice */
      MAGENTA*16+L_WHITE, /* C_CHOICE_CURRENT  fullscreen_choice input */
      BLACK*16+WHITE,     /* C_CHOICE_SP_INSTR speed key bar & instr */
      BLACK*16+L_MAGENTA, /* C_CHOICE_SP_KEYIN speed key value */
      WHITE*16+BLUE,      /* C_GENERAL_HI      tab, thinking, IFS */
      WHITE*16+BLACK,     /* C_GENERAL_MED */
      WHITE*16+GRAY,      /* C_GENERAL_LO */
      BLACK*16+L_WHITE,   /* C_GENERAL_INPUT */
      WHITE*16+BLACK,     /* C_DVID_BKGRD      disk video */
      BLACK*16+YELLOW,    /* C_DVID_HI */
      BLACK*16+L_WHITE,   /* C_DVID_LO */
      RED*16+L_WHITE,     /* C_STOP_ERR        stop message, error */
      GREEN*16+BLACK,     /* C_STOP_INFO       stop message, info */
      BLUE*16+WHITE,      /* C_TITLE_LOW       bottom lines of title screen */
      GREEN*16+BLACK,     /* C_AUTHDIV1        title screen dividers */
      GREEN*16+GRAY,      /* C_AUTHDIV2        title screen dividers */
      BLACK*16+L_WHITE,   /* C_PRIMARY         primary authors */
      BLACK*16+WHITE      /* C_CONTRIB         contributing authors */
      };

char s_makepar[] =          "makepar";

int lzw[2];

/*
        cmdfiles(argc,argv) process the command-line arguments
                it also processes the 'sstools.ini' file and any
                indirect files ('fractint @myfile')
*/

/* This probably ought to go somewhere else, but it's used here.        */
/* getpower10(x) returns the magnitude of x.  This rounds               */
/* a little so 9.95 rounds to 10, but we're using a binary base anyway, */
/* so there's nothing magic about changing to the next power of 10.     */
int getpower10(LDBL x)
{
    char string[11]; /* space for "+x.xe-xxxx" */
    int p;

#ifdef USE_LONG_DOUBLE
    sprintf(string,"%+.1Le", x);
#else
    sprintf(string,"%+.1le", x);
#endif
    p = atoi(string+5);
    return p;
}



int cmdfiles(int argc,char **argv)
{
   int     i;
   char    curarg[141];
   char    tempstring[101];
   char    *sptr;
   FILE    *initfile;

   if (first_init) initvars_run();      /* once per run initialization */
   initvars_restart();                  /* <ins> key initialization */
   initvars_fractal();                  /* image initialization */

   strcpy(curarg, "sstools.ini");
   findpath(curarg, tempstring); /* look for SSTOOLS.INI */
   if (tempstring[0] != 0)              /* found it! */
      if ((initfile = fopen(tempstring,"r")) != NULL)
         cmdfile(initfile, CMDFILE_SSTOOLS_INI);           /* process it */

   for (i = 1; i < argc; i++) {         /* cycle through args */
#ifdef XFRACT
      /* Let the xfract code take a look at the argument */
      if (unixarg(argc,argv,&i)) continue;
#endif
      strcpy(curarg,argv[i]);
      if (curarg[0] == ';')             /* start of comments? */
         break;
      if (curarg[0] != '@') {           /* simple command? */
         if (strchr(curarg,'=') == NULL) { /* not xxx=yyy, so check for gif */
            strcpy(tempstring,curarg);
            if (has_ext(curarg) == NULL)
               strcat(tempstring,".gif");
            if ((initfile = fopen(tempstring,"rb")) != NULL) {
               fread(tempstring,6,1,initfile);
               if ( tempstring[0] == 'G'
                 && tempstring[1] == 'I'
                 && tempstring[2] == 'F'
                 && tempstring[3] >= '8' && tempstring[3] <= '9'
                 && tempstring[4] >= '0' && tempstring[4] <= '9') {
                  strcpy(readname,curarg);
                  extract_filename(browsename,readname);
                  curarg[0] = (char)(showfile = 0);
                  }
               fclose(initfile);
               }
            }
         if (curarg[0])
            cmdarg(curarg, CMDFILE_AT_CMDLINE);           /* process simple command */
         }
      else if ((sptr = strchr(curarg,'/')) != NULL) { /* @filename/setname? */
         *sptr = 0;
         if (merge_pathnames(CommandFile, &curarg[1], 0) < 0)
            init_msg("",CommandFile,0);
         strcpy(CommandName,sptr+1);
         if (find_file_item(CommandFile,CommandName,&initfile, 0)<0 || initfile==NULL)
            argerror(curarg);
         cmdfile(initfile, CMDFILE_AT_CMDLINE_SETNAME);
         }
      else {                            /* @filename */
         initfile = fopen(&curarg[1],"r");
		 if (initfile == NULL)
            argerror(curarg);
         cmdfile(initfile, CMDFILE_AT_CMDLINE);
         }
      }

   if (first_init == 0) {
      g_init_mode = -1; /* don't set video when <ins> key used */
      showfile = 1;  /* nor startup image file              */
      }

   init_msg("",NULL,0);  /* this causes driver_get_key if init_msg called on runup */

   if (debugflag != 110)
       first_init = 0;
/*       {
            char msg[MSGLEN];
            sprintf(msg,"cmdfiles colorpreloaded %d showfile %d savedac %d",
                colorpreloaded, showfile, savedac);
            stopmsg(0,msg);
         }
*/
   if (colorpreloaded && showfile==0) /* PAR reads a file and sets color */
      dontreadcolor = 1;   /* don't read colors from GIF */
   else
      dontreadcolor = 0;   /* read colors from GIF */

     /*set structure of search directories*/
   strcpy(searchfor.par, CommandFile);
   strcpy(searchfor.frm, FormFileName);
   strcpy(searchfor.lsys, LFileName);
   strcpy(searchfor.ifs, IFSFileName);
   return 0;
}


int load_commands(FILE *infile)
{
   /* when called, file is open in binary mode, positioned at the */
   /* '(' or '{' following the desired parameter set's name       */
   int ret;
   initcorners = initparams = 0; /* reset flags for type= */
   ret = cmdfile(infile, CMDFILE_AT_AFTER_STARTUP);
/*
         {
            char msg[MSGLEN];
            sprintf(msg,"load commands colorpreloaded %d showfile %d savedac %d",
                colorpreloaded, showfile, savedac);
            stopmsg(0,msg);
         }
*/

   if (colorpreloaded && showfile==0) /* PAR reads a file and sets color */
      dontreadcolor = 1;   /* don't read colors from GIF */
   else
      dontreadcolor = 0;   /* read colors from GIF */
   return ret;
}


static void initvars_run()              /* once per run init */
{
   char *p;
   init_rseed = (int)time(NULL);
   init_comments();
   p = getenv("TMP");
   if (p == NULL)
      p = getenv("TEMP");
   if (p != NULL)
   {
      if (isadirectory(p) != 0)
      {
         strcpy(tempdir,p);
         fix_dirname(tempdir);
      }
   }
   else
      *tempdir = 0;
}

static void initvars_restart()          /* <ins> key init */
{
   int i;
   recordcolors = 'a';                  /* don't use mapfiles in PARs */
   save_release = g_release;            /* this release number */
   gif87a_flag = INIT_GIF87;            /* turn on GIF89a processing */
   dither_flag = 0;                     /* no dithering */
   askvideo = 1;                        /* turn on video-prompt flag */
   fract_overwrite = 0;                 /* don't overwrite           */
   soundflag = SOUNDFLAG_SPEAKER | SOUNDFLAG_BEEP; /* sound is on to PC speaker */
   initbatch = 0;                       /* not in batch mode         */
   checkcurdir = 0;                     /* flag to check current dire for files */
   initsavetime = 0;                    /* no auto-save              */
   g_init_mode = -1;                       /* no initial video mode     */
   viewwindow = 0;                      /* no view window            */
   viewreduction = (float)4.2;
   viewcrop = 1;
   g_virtual_screens = 1;                 /* virtual screen modes on   */
   finalaspectratio = screenaspect;
   viewxdots = viewydots = 0;
   video_cutboth = 1;                   /* keep virtual aspect */
   zscroll = 1;                         /* relaxed screen scrolling */
   orbit_delay = 0;                     /* full speed orbits */
   orbit_interval = 1;                  /* plot all orbits */
   debugflag = 0;                       /* debugging flag(s) are off */
   timerflag = 0;                       /* timer flags are off       */
   strcpy(FormFileName, "fractint.frm"); /* default formula file      */
   FormName[0] = 0;
   strcpy(LFileName, "fractint.l");
   LName[0] = 0;
   strcpy(CommandFile, "fractint.par");
   CommandName[0] = 0;
   for (i=0; i<4; i++)
      CommandComment[i][0] = 0;
   strcpy(IFSFileName, "fractint.ifs");
   IFSName[0] = 0;
   reset_ifs_defn();
   rflag = 0;                           /* not a fixed srand() seed */
   rseed = init_rseed;
   strcpy(readname,DOTSLASH);           /* initially current directory */
   showfile = 1;
   /* next should perhaps be fractal re-init, not just <ins> ? */
   initcyclelimit=55;                   /* spin-DAC default speed limit */
   mapset = 0;                          /* no map= name active */
   if (mapdacbox) {
      free(mapdacbox);
      mapdacbox = NULL;
      }

   major_method = breadth_first;        /* default inverse julia methods */
   minor_method = left_first;   /* default inverse julia methods */
   truecolor = 0;              /* truecolor output flag */
   truemode = 0;               /* set to default color scheme */
}

static void initvars_fractal()          /* init vars affecting calculation */
{
   int i;
   escape_exit = 0;                     /* don't disable the "are you sure?" screen */
   usr_periodicitycheck = 1;            /* turn on periodicity    */
   inside = 1;                          /* inside color = blue    */
   fillcolor = -1;                      /* no special fill color */
   usr_biomorph = -1;                   /* turn off biomorph flag */
   outside = -1;                        /* outside color = -1 (not used) */
   maxit = 150;                         /* initial maxiter        */
   usr_stdcalcmode = 'g';               /* initial solid-guessing */
   stoppass = 0;                        /* initial guessing stoppass */
   quick_calc = 0;
   closeprox = 0.01;
   ismand = 1;                          /* default formula mand/jul toggle */
#ifndef XFRACT
   usr_floatflag = 0;                   /* turn off the float flag */
#else
   usr_floatflag = 1;                   /* turn on the float flag */
#endif
   finattract = 0;                      /* disable finite attractor logic */
   fractype = 0;                        /* initial type Set flag  */
   curfractalspecific = &fractalspecific[0];
   initcorners = initparams = 0;
   bailout = 0;                         /* no user-entered bailout */
   nobof = 0;  /* use normal bof initialization to make bof images */
   useinitorbit = 0;
   for (i = 0; i < MAXPARAMS; i++) param[i] = 0.0;     /* initial parameter values */
   for (i = 0; i < 3; i++) potparam[i]  = 0.0; /* initial potential values */
   for (i = 0; i < 3; i++) inversion[i] = 0.0;  /* initial invert values */
   initorbit.x = initorbit.y = 0.0;     /* initial orbit values */
   invert = 0;
   decomp[0] = decomp[1] = 0;
   usr_distest = 0;
   pseudox = 0;
   pseudoy = 0;
   distestwidth = 71;
   forcesymmetry = 999;                 /* symmetry not forced */
   xx3rd = xxmin = -2.5; xxmax = 1.5;   /* initial corner values  */
   yy3rd = yymin = -1.5; yymax = 1.5;   /* initial corner values  */
   bf_math = 0;
   pot16bit = potflag = 0;
   LogFlag = 0;                         /* no logarithmic palette */
   set_trig_array(0, "sin");             /* trigfn defaults */
   set_trig_array(1, "sqr");
   set_trig_array(2, "sinh");
   set_trig_array(3, "cosh");
   if (rangeslen) {
      free((char *)ranges);
      rangeslen = 0;
      }
   usemag = 1;                          /* use center-mag, not corners */

   colorstate = colorpreloaded = 0;
   rotate_lo = 1; rotate_hi = 255;      /* color cycling default range */
   orbit_delay = 0;                     /* full speed orbits */
   orbit_interval = 1;                  /* plot all orbits */
   keep_scrn_coords = 0;
   drawmode = 'r';                      /* passes=orbits draw mode */
   set_orbit_corners = 0;
   oxmin = curfractalspecific->xmin;
   oxmax = curfractalspecific->xmax;
   ox3rd = curfractalspecific->xmin;
   oymin = curfractalspecific->ymin;
   oymax = curfractalspecific->ymax;
   oy3rd = curfractalspecific->ymin;

   math_tol[0] = 0.05;
   math_tol[1] = 0.05;

   display3d = 0;                       /* 3D display is off        */
   overlay3d = 0;                       /* 3D overlay is off        */

   old_demm_colors = 0;
   bailoutest    = Mod;
   floatbailout  = (int ( *)(void))fpMODbailout;
   longbailout   = (int ( *)(void))asmlMODbailout;
   bignumbailout = (int ( *)(void))bnMODbailout;
   bigfltbailout = (int ( *)(void))bfMODbailout;

   functionpreloaded = 0; /* for old bifs  JCO 7/5/92 */
   mxminfp = -.83;
   myminfp = -.25;
   mxmaxfp = -.83;
   mymaxfp =  .25;
   originfp = 8;
   heightfp = 7;
   widthfp = 10;
   distfp = 24;
   eyesfp = (float)2.5;
   depthfp = 8;
   neworbittype = JULIA;
   zdots = 128;
   initvars_3d();
   basehertz = 440;                     /* basic hertz rate          */
#ifndef XFRACT
   fm_vol = 63;                         /* full volume on soundcard o/p */
   hi_atten = 0;                        /* no attenuation of hi notes */
   fm_attack = 5;                       /* fast attack     */
   fm_decay = 10;                        /* long decay      */
   fm_sustain = 13;                      /* fairly high sustain level   */
   fm_release = 5;                      /* short release   */
   fm_wavetype = 0;                     /* sin wave */
   polyphony = 0;                       /* no polyphony    */
   for (i=0; i<=11; i++) scale_map[i]=i+1;    /* straight mapping of notes in octave */
#endif
}

static void initvars_3d()               /* init vars affecting 3d */
{
   RAY     = 0;
   BRIEF   = 0;
   SPHERE = FALSE;
   preview = 0;
   showbox = 0;
   xadjust = 0;
   yadjust = 0;
   g_eye_separation = 0;
   g_glasses_type = 0;
   previewfactor = 20;
   red_crop_left   = 4;
   red_crop_right  = 0;
   blue_crop_left  = 0;
   blue_crop_right = 4;
   red_bright     = 80;
   blue_bright   = 100;
   transparent[0] = transparent[1] = 0; /* no min/max transparency */
   set_3d_defaults();
}

static void reset_ifs_defn()
{
   if (ifs_defn) {
      free((char *)ifs_defn);
      ifs_defn = NULL;
      }
}


static int cmdfile(FILE *handle,int mode)
   /* mode = 0 command line @filename         */
   /*        1 sstools.ini                    */
   /*        2 <@> command after startup      */
   /*        3 command line @filename/setname */
{
   /* note that cmdfile could be open as text OR as binary */
   /* binary is used in @ command processing for reasonable speed note/point */
   int i;
   int lineoffset = 0;
   int changeflag = 0; /* &1 fractal stuff chgd, &2 3d stuff chgd */
   char linebuf[513];
   char cmdbuf[10000] = { 0 };

   if (mode == CMDFILE_AT_AFTER_STARTUP || mode == CMDFILE_AT_CMDLINE_SETNAME) {
      while ((i = getc(handle)) != '{' && i != EOF) { }
      for (i=0; i<4; i++)
          CommandComment[i][0] = 0;
      }
   linebuf[0] = 0;
   while (next_command(cmdbuf,10000,handle,linebuf,&lineoffset,mode) > 0) {
      if ((mode == CMDFILE_AT_AFTER_STARTUP || mode == CMDFILE_AT_CMDLINE_SETNAME) && strcmp(cmdbuf,"}") == 0) break;
      if ((i = cmdarg(cmdbuf,mode)) < 0) break;
      changeflag |= i;
      }
   fclose(handle);
#ifdef XFRACT
   g_init_mode = 0;                /* Skip credits if @file is used. */
#endif
   if (changeflag & CMDARG_FRACTAL_PARAM)
   {
      backwards_v18();
      backwards_v19();
      backwards_v20();
   }
   return changeflag;
}

static int next_command(char *cmdbuf,int maxlen,
                      FILE *handle,char *linebuf,int *lineoffset,int mode)
{
   int i;
   int cmdlen = 0;
   char *lineptr;
   lineptr = linebuf + *lineoffset;
   while (1) {
      while (*lineptr <= ' ' || *lineptr == ';') {
         if (cmdlen) {                  /* space or ; marks end of command */
            cmdbuf[cmdlen] = 0;
            *lineoffset = (int) (lineptr - linebuf);
            return cmdlen;
            }
         while (*lineptr && *lineptr <= ' ')
            ++lineptr;                  /* skip spaces and tabs */
         if (*lineptr == ';' || *lineptr == 0) {
            if (*lineptr == ';'
              && (mode == CMDFILE_AT_AFTER_STARTUP || mode == CMDFILE_AT_CMDLINE_SETNAME)
              && (CommandComment[0][0] == 0 || CommandComment[1][0] == 0 ||
                  CommandComment[2][0] == 0 || CommandComment[3][0] == 0)) {
               /* save comment */
               while (*(++lineptr)
                 && (*lineptr == ' ' || *lineptr == '\t')) { }
               if (*lineptr) {
                  if ((int)strlen(lineptr) >= MAXCMT)
                     *(lineptr+MAXCMT-1) = 0;
                  for (i=0; i<4; i++)
                     if (CommandComment[i][0] == 0)
                     {
                        strcpy(CommandComment[i],lineptr);
                        break;   
                     }
                  }
               }
            if (next_line(handle,linebuf,mode) != 0)
               return -1; /* eof */
            lineptr = linebuf; /* start new line */
            }
         }
      if (*lineptr == '\\'              /* continuation onto next line? */
        && *(lineptr+1) == 0) {
         if (next_line(handle,linebuf,mode) != 0) {
            argerror(cmdbuf);           /* missing continuation */
            return -1;
            }
         lineptr = linebuf;
         while (*lineptr && *lineptr <= ' ')
            ++lineptr;                  /* skip white space @ start next line */
         continue;                      /* loop to check end of line again */
         }
      cmdbuf[cmdlen] = *(lineptr++);    /* copy character to command buffer */
      if (++cmdlen >= maxlen) {         /* command too long? */
         argerror(cmdbuf);
         return -1;
         }
      }
}

static int next_line(FILE *handle,char *linebuf,int mode)
{
   int toolssection;
   char tmpbuf[11];
   toolssection = 0;
   while (file_gets(linebuf,512,handle) >= 0) {
      if (mode == CMDFILE_SSTOOLS_INI && linebuf[0] == '[') {     /* check for [fractint] */
#ifndef XFRACT
         strncpy(tmpbuf,&linebuf[1],9);
         tmpbuf[9] = 0;
         strlwr(tmpbuf);
         toolssection = strncmp(tmpbuf,"fractint]",9);
#else
         strncpy(tmpbuf,&linebuf[1],10);
         tmpbuf[10] = 0;
         strlwr(tmpbuf);
         toolssection = strncmp(tmpbuf,"xfractint]",10);
#endif
         continue;                              /* skip tools section heading */
         }
      if (toolssection == 0) return 0;
      }
   return -1;
}

/*
  cmdarg(string,mode) processes a single command-line/command-file argument
    return:
      -1 error, >= 0 ok
      if ok, return value:
        | 1 means fractal parm has been set
        | 2 means 3d parm has been set
        | 4 means 3d=yes specified
        | 8 means reset specified
*/

#define NONNUMERIC -32767

int cmdarg(char *curarg, int mode) /* process a single argument */
{
	char    variable[21];                /* variable name goes here   */
	char    *value;                      /* pointer to variable value */
	int     valuelen;                    /* length of value           */
	int     numval;                      /* numeric value of arg      */
	char    charval[16];                 /* first character of arg    */
	int     yesnoval[16];                /* 0 if 'n', 1 if 'y', -1 if not */
	double  ftemp;
	int     i, j, k;
	char    *argptr,*argptr2;
	int     totparms;                    /* # of / delimited parms    */
	int     intparms;                    /* # of / delimited ints     */
	int     floatparms;                  /* # of / delimited floats   */
	int     intval[64];                  /* pre-parsed integer parms  */
	double  floatval[16];                /* pre-parsed floating parms */
	char    *floatvalstr[16];            /* pointers to float vals */
	char    tmpc;
	int     lastarg;
	double Xctr, Yctr, Xmagfactor, Rotation, Skew;
	LDBL Magnification;
	bf_t bXctr, bYctr;


	argptr = curarg;
	while (*argptr)
	{                    /* convert to lower case */
		if (*argptr >= 'A' && *argptr <= 'Z')
		{
			*argptr += 'a' - 'A';
		}
		else if (*argptr == '=')
		{
			/* don't convert colors=value or comment=value */
			if ((strncmp(curarg, "colors=", 7) == 0) || (strncmp(curarg, "comment", 7) == 0))
			{
				break;
			}
		}
		++argptr;
    }

	value = strchr(&curarg[1], '=');
	if (value != NULL)
	{
		j = (int) ((value++) - curarg);
		if (j > 1 && curarg[j-1] == ':')
		{
			--j;                           /* treat := same as =     */
		}
	}
	else
	{
		j = (int) strlen(curarg);
		value = curarg + j;
	}
	if (j > 20)
	{
		goto badarg;             /* keyword too long */
	}
	strncpy(variable, curarg, j);          /* get the variable name  */
	variable[j] = 0;                     /* truncate variable name */
	valuelen = (int) strlen(value);            /* note value's length    */
	charval[0] = value[0];               /* first letter of value  */
	yesnoval[0] = -1;                    /* note yes|no value      */
	if (charval[0] == 'n')
	{
		yesnoval[0] = 0;
	}
	if (charval[0] == 'y')
	{
		yesnoval[0] = 1;
	}

	argptr = value;
	numval = totparms = intparms = floatparms = 0;
	while (*argptr)                    /* count and pre-parse parms */
	{
		long ll;
		lastarg = 0;
		argptr2 = strchr(argptr,'/');
		if (argptr2 == NULL)     /* find next '/' */
		{
			argptr2 = argptr + strlen(argptr);
			*argptr2 = '/';
			lastarg = 1;
		}
		if (totparms == 0)
		{
			numval = NONNUMERIC;
		}
		i = -1;
		if ( totparms < 16)
		{
			charval[totparms] = *argptr;                      /* first letter of value  */
			if (charval[totparms] == 'n')
			{
				yesnoval[totparms] = 0;
			}
			if (charval[totparms] == 'y')
			{
				yesnoval[totparms] = 1;
			}
		}
		j = 0;
		if (sscanf(argptr, "%c%c", (char *) &j, &tmpc) > 0    /* NULL entry */
			&& ((char) j == '/' || (char) j == '=') && tmpc == '/')
		{
			j = 0;
			++floatparms;
			++intparms;
			if (totparms < 16)
			{
				floatval[totparms] = j;
				floatvalstr[totparms] = "0";
			}
			if (totparms < 64)
			{
				intval[totparms] = j;
			}
			if (totparms == 0)
			{
				numval = j;
			}
		}
		else if (sscanf(argptr, "%ld%c", &ll, &tmpc) > 0       /* got an integer */
			&& tmpc == '/')        /* needs a long int, ll, here for lyapunov */
		{
			++floatparms;
			++intparms;
			if (totparms < 16)
			{
				floatval[totparms] = ll;
				floatvalstr[totparms] = argptr;
			}
			if (totparms < 64)
			{
				intval[totparms] = (int) ll;
			}
			if (totparms == 0)
			{
				numval = (int) ll;
			}
		}
#ifndef XFRACT
		else if (sscanf(argptr, "%lg%c", &ftemp, &tmpc) > 0  /* got a float */
#else
		else if (sscanf(argptr, "%lf%c", &ftemp, &tmpc) > 0  /* got a float */
#endif
				&& tmpc == '/')
		{
			++floatparms;
			if (totparms < 16)
			{
				floatval[totparms] = ftemp;
				floatvalstr[totparms] = argptr;
			}
		}
		/* using arbitrary precision and above failed */
		else if (((int) strlen(argptr) > 513)  /* very long command */
					|| (totparms > 0 && floatval[totparms-1] == FLT_MAX
						&& totparms < 6)
					|| isabigfloat(argptr))
		{
			++floatparms;
			floatval[totparms] = FLT_MAX;
			floatvalstr[totparms] = argptr;
		}
		++totparms;
		argptr = argptr2;                                 /* on to the next */
		if (lastarg)
		{
			*argptr = 0;
		}
		else
		{
			++argptr;
		}
    }

	if (mode != CMDFILE_AT_AFTER_STARTUP || debugflag == 110)
	{
		/* these commands are allowed only at startup */
		if (strcmp(variable, "batch") == 0)     /* batch=?      */
		{
			if (yesnoval[0] < 0)
			{
				goto badarg;
			}
#ifdef XFRACT
			g_init_mode = yesnoval[0] ? 0 : -1; /* skip credits for batch mode */
#endif
			initbatch = yesnoval[0];
			return 3;
        }
		if (strcmp(variable, "maxhistory") == 0)       /* maxhistory=? */
		{
			if (numval == NONNUMERIC)
			{
				goto badarg;
			}
			else if (numval < 0 /* || numval > 1000 */)
			{
				goto badarg;
			}
			else
			{
				maxhistory = numval;
			}
			return 3;
		}

		/* adapter= no longer used */
		if (strcmp(variable, "adapter") == 0)    /* adapter==?     */
		{
			/* adapter parameter no longer used; check for bad argument anyway */
			if ((strcmp(value, "egamono") != 0)	&& (strcmp(value, "hgc") != 0) &&
				(strcmp(value, "ega") != 0)		&& (strcmp(value, "cga") != 0) &&
				(strcmp(value, "mcga") != 0)	&& (strcmp(value, "vga") != 0))
			{
				goto badarg;
			}
			return 3;
		}

		/* 8514 API no longer used; silently gobble any argument */
		if (strcmp(variable, "afi") == 0)
		{
			return 3;
		}

		if (strcmp(variable,"textsafe") == 0 )  /* textsafe==? */
		{
			/* textsafe no longer used, do validity checking, but gobble argument */
			if (first_init)
			{
				if (!((charval[0] == 'n')	/* no */
					|| (charval[0] == 'y')	/* yes */
					|| (charval[0] == 'b')	/* bios */
					|| (charval[0] == 's'))) /* save */
				{
					goto badarg;
				}
			}
			return 3;
		}

		if (strcmp(variable, "vesadetect") == 0)
		{
			/* vesadetect no longer used, do validity checks, but gobble argument */
			if (yesnoval[0] < 0)
			{
				goto badarg;
			}
			return 3;
		}

		/* biospalette no longer used, do validity checks, but gobble argument */
		if (strcmp(variable, "biospalette") == 0)
		{
			if (yesnoval[0] < 0)
			{
				goto badarg;
			}
			return 3;
		}

		if (strcmp(variable, "fpu") == 0)
		{
			if (strcmp(value, "387") == 0)
			{
#ifndef XFRACT
				fpu = 387;
#else
				fpu = -1;
#endif
				return 0;
			}
			goto badarg;
		}

		if (strcmp(variable, "exitnoask") == 0)
		{
			if (yesnoval[0] < 0)
			{
				goto badarg;
			}
			escape_exit = yesnoval[0];
			return 3;
         }

		if (strcmp(variable, "makedoc") == 0)
		{
			print_document(*value ? value : "fractint.doc", makedoc_msg_func, 0);
#ifndef WINFRACT
			goodbye();
#endif
        }

		if (strcmp(variable,s_makepar) == 0)
		{
			char *slash, *next = NULL;
			if (totparms < 1 || totparms > 2)
			{
				goto badarg;
			}
			slash = strchr(value, '/');
			if (slash != NULL)
			{
				*slash = 0;
				next = slash+1;
			}

			strcpy(CommandFile, value);
			if (strchr(CommandFile, '.') == NULL)
			{
				strcat(CommandFile, ".par");
			}
			if (strcmp(readname, DOTSLASH)==0)
			{
				*readname = 0;
			}
			if (next == NULL)
			{
				if (*readname != 0)
				{
					extract_filename(CommandName, readname);
				}
				else if (*MAP_name != 0)   
				{
					extract_filename(CommandName, MAP_name);
				}
				else
				{
					goto badarg;
				}
			}   
			else
			{
				strncpy(CommandName, next, ITEMNAMELEN);
				CommandName[ITEMNAMELEN] = 0;
			}
			*s_makepar = 0; /* used as a flag for makepar case */
			if (*readname != 0)
			{
				if (read_overlay() != 0)
				{
					goodbye();
				}
			}
			else if (*MAP_name != 0)
			{
				s_makepar[1] = 0; /* second char is flag for map */
			}
			xdots = filexdots;
			ydots = fileydots;
			dxsize = xdots - 1;
			dysize = ydots - 1;
			calcfracinit();
			make_batch_file();
#ifndef WINFRACT
#if !defined(XFRACT)
#if defined(_WIN32)
			ABORT(0, "Don't call standard I/O without a console on Windows");
			_ASSERTE(0 && "Don't call standard I/O without a console on Windows");
#else
			if (*readname != 0)
			{
				printf("copying fractal info in GIF %s to PAR %s/%s\n",
					readname, CommandFile, CommandName);
			}
			else if (*MAP_name != 0)
			{
				printf("copying color info in map %s to PAR %s/%s\n",
					MAP_name, CommandFile, CommandName);
			}
#endif
#endif
			goodbye();
#endif
		}
	} /* end of commands allowed only at startup */

	if (strcmp(variable, "reset") == 0)
	{
		initvars_fractal();

		/* PAR release unknown unless specified */
		if (numval >= 0)
		{
			save_release = numval;
		}
		else
		{
			goto badarg;
		}
		if (save_release == 0)
		{
			save_release = 1730; /* before start of lyapunov wierdness */
		}
		return 9;
	}

	if (strcmp(variable, "filename") == 0)      /* filename=?     */
	{
		int existdir;
		if (charval[0] == '.' && value[1] != SLASHC)
		{
			if (valuelen > 4)
			{
				goto badarg;
			}
			gifmask[0] = '*';
			gifmask[1] = 0;
			strcat(gifmask, value);
			return 0;
		}
		if (valuelen > (FILE_MAX_PATH-1))
		{
			goto badarg;
		}
		if (mode == CMDFILE_AT_AFTER_STARTUP && display3d == 0) /* can't do this in @ command */
		{
			goto badarg;
		}

		existdir = merge_pathnames(readname, value, mode);
		if (existdir == 0)
		{
			showfile = 0;
		}
		else if (existdir < 0)
		{
			init_msg(variable, value, mode);
		}
		else
		{
			extract_filename(browsename, readname);
		}
		return 3;
	}

	if (strcmp(variable, "video") == 0)         /* video=? */
	{
			k = check_vidmode_keyname(value);
			if (k == 0)
			{
				goto badarg;
			}
			g_init_mode = -1;
			for (i = 0; i < MAXVIDEOMODES; ++i)
			{
				if (g_video_table[i].keynum == k)
				{
					g_init_mode = i;
					break;
				}
			}
			if (g_init_mode == -1)
			{
				goto badarg;
			}
		return 3;
	}

	if (strcmp(variable, "map") == 0)         /* map=, set default colors */
	{
		int existdir;
		if (valuelen > (FILE_MAX_PATH-1))
		{
			goto badarg;
		}
		existdir = merge_pathnames(MAP_name, value, mode);
		if (existdir > 0)
		{
			return 0;    /* got a directory */
		}
		else if (existdir < 0)
		{
			init_msg(variable, value, mode);
			return 0;
		}
		SetColorPaletteName(MAP_name);
		return 0;
	}

	if (strcmp(variable, "colors") == 0)       /* colors=, set current colors */
	{
		if (parse_colors(value) < 0)
		{
			goto badarg;
		}
		return 0;
	}

	if (strcmp(variable, "recordcolors") == 0)       /* recordcolors= */
	{
		if (*value != 'y' && *value != 'c' && *value != 'a')
		{
			goto badarg;
		}
		recordcolors = *value;
		return 0;
	}

	if (strcmp(variable, "maxlinelength") == 0)  /* maxlinelength= */
	{
		if (numval < MINMAXLINELENGTH || numval > MAXMAXLINELENGTH)
		{
			goto badarg;
		}
		maxlinelength = numval;
		return 0;
	}

	if (strcmp(variable, "comment") == 0)       /* comment= */
	{
		parse_comments(value);
		return 0;
	}

	/* tplus no longer used, validate value and gobble argument */
	if (strcmp(variable, "tplus") == 0)
	{
		if (yesnoval[0] < 0)
		{
			goto badarg;
		}
		return 0;
	}

	/* noninterlaced no longer used, validate value and gobble argument */
	if (strcmp(variable, "noninterlaced") == 0)
	{
		if (yesnoval[0] < 0)
		{
			goto badarg;
		}
		return 0;
	}

	/* maxcolorres no longer used, validate value and gobble argument */
	if (strcmp(variable, "maxcolorres") == 0) /* Change default color resolution */
	{
		if (numval == 1 || numval == 4 || numval == 8 ||
							numval == 16 || numval == 24)
		{
			return 0;
		}
		goto badarg;
	}

	/* pixelzoom no longer used, validate value and gobble argument */
	if (strcmp(variable, "pixelzoom") == 0)
	{
		if (numval >= 5)
		{
			goto badarg;
		}
		return 0;
	}

	/* keep this for backward compatibility */
	if (strcmp(variable, "warn") == 0)         /* warn=? */
	{
		if (yesnoval[0] < 0)
		{
			goto badarg;
		}
		fract_overwrite = (char) (yesnoval[0] ^ 1);
		return 0;
	}
	if (strcmp(variable, "overwrite") == 0)    /* overwrite=? */
	{
		if (yesnoval[0] < 0)
		{
			goto badarg;
		}
		fract_overwrite = (char) yesnoval[0];
		return 0;
	}

	if (strcmp(variable, "gif87a") == 0)       /* gif87a=? */
	{
		if (yesnoval[0] < 0)
		{
			goto badarg;
		}
		gif87a_flag = yesnoval[0];
		return 0;
	}

	if (strcmp(variable, "dither") == 0) /* dither=? */
	{       
		if (yesnoval[0] < 0)
		{
			goto badarg;
		}
		dither_flag = yesnoval[0];
		return 0;
	}

	if (strcmp(variable, "savetime") == 0)      /* savetime=? */
	{
		initsavetime = numval;
		return 0;
	}

	if (strcmp(variable, "autokey") == 0)       /* autokey=? */
	{
		if (strcmp(value, "record") == 0)
		{
			g_slides = SLIDES_RECORD;
		}
		else if (strcmp(value, "play") == 0)
		{
			g_slides = SLIDES_PLAY;
		}
		else
		{
			goto badarg;
		}
		return 0;
	}

	if (strcmp(variable, "autokeyname") == 0)   /* autokeyname=? */
	{
		if (merge_pathnames(autoname, value, mode) < 0)
		{
			init_msg(variable, value, mode);
		}
		return 0;
	}

	if (strcmp(variable, "type") == 0)         /* type=? */
	{
		if (value[valuelen-1] == '*')
		{
			value[--valuelen] = 0;
		}
		/* kludge because type ifs3d has an asterisk in front */
		if (strcmp(value, "ifs3d") == 0)
		{
			value[3] = 0;
		}
		for (k = 0; fractalspecific[k].name != NULL; k++)
		{
			if (strcmp(value, fractalspecific[k].name) == 0)
			{
				break;
			}
		}
		if (fractalspecific[k].name == NULL)
		{
			goto badarg;
		}
		fractype = k;
		curfractalspecific = &fractalspecific[fractype];
		if (initcorners == 0)
		{
			xx3rd = xxmin = curfractalspecific->xmin;
			xxmax         = curfractalspecific->xmax;
			yy3rd = yymin = curfractalspecific->ymin;
			yymax         = curfractalspecific->ymax;
		}
		if (initparams == 0)
		{
			load_params(fractype);
		}
		return 1;
	}

	if (strcmp(variable, "inside") == 0)       /* inside=? */
	{
		struct
		{
			const char *arg;
			int inside;
		} args[] =
		{
			{ "zmag", ZMAG },
			{ "bof60", BOF60 },
			{ "bof61", BOF61 },
			{ "epsiloncross", EPSCROSS },
			{ "startrail", STARTRAIL },
			{ "period", PERIOD },
			{ "fmod", FMODI },
			{ "atan", ATANI },
			{ "maxiter", -1 }
		};
		int ii;
		for (ii = 0; ii < NUM_OF(args); ii++)
		{
			if (strcmp(value, args[ii].arg) == 0)
			{
				inside = args[ii].inside;
				return 1;
			}
		}
		if (numval == NONNUMERIC)
		{
			goto badarg;
		}
		else
		{
			inside = numval;
		}
		return 1;
	}

	if (strcmp(variable, "proximity") == 0)       /* proximity=? */
	{
		closeprox = floatval[0];
		return 1;
	}

	if (strcmp(variable, "fillcolor") == 0)       /* fillcolor */
	{
		if (strcmp(value, "normal")==0)
		{
			fillcolor = -1;
		}
		else if (numval == NONNUMERIC)
		{
			goto badarg;
		}
		else
		{
			fillcolor = numval;
		}
		return 1;
	}

	if (strcmp(variable, "finattract") == 0)   /* finattract=? */
	{
		if (yesnoval[0] < 0)
		{
			goto badarg;
		}
		finattract = yesnoval[0];
		return 1;
	}

	if (strcmp(variable, "nobof") == 0)   /* nobof=? */
	{
		if (yesnoval[0] < 0)
		{
			goto badarg;
		}
		nobof = yesnoval[0];
		return 1;
	}

	if (strcmp(variable, "function") == 0)      /* function=?,? */
	{
		k = 0;
		while (*value && k < 4)
		{
			if (set_trig_array(k++, value))
			{
				goto badarg;
			}
			value = strchr(value, '/');
			if (value == NULL)
			{
				break;
			}
			++value;
		}
		functionpreloaded = 1; /* for old bifs  JCO 7/5/92 */
		return 1;
	}

	if (strcmp(variable, "outside") == 0)      /* outside=? */
	{
		int ii;
		struct
		{
			const char *arg;
			int outside;
		}
		args[] =
		{
			{ "iter", ITER },
			{ "real", REAL },
			{ "imag", IMAG },
			{ "mult", MULT },
			{ "summ", SUM },
			{ "atan", ATAN },
			{ "fmod", FMOD },
			{ "tdis", TDIS }
		};
		for (ii = 0; ii < NUM_OF(args); ii++)
		{
			if (strcmp(value, args[ii].arg) == 0)
			{
				outside = args[ii].outside;
				return 1;
			}
		}
		if ((numval == NONNUMERIC) || (numval < TDIS || numval > 255))
		{
			goto badarg;
		}
		outside = numval;
		return 1;
	}

	if (strcmp(variable, "bfdigits") == 0)      /* bfdigits=? */
	{
		if ((numval == NONNUMERIC) || (numval < 0 || numval > 2000))
		{
			goto badarg;
		}
		bfdigits = numval;
		return 1;
	}

	if (strcmp(variable, "maxiter") == 0)       /* maxiter=? */
	{
		if (floatval[0] < 2)
		{
			goto badarg;
		}
		maxit = (long) floatval[0];
		return 1;
	}

	if (strcmp(variable, "iterincr") == 0)        /* iterincr=? */
	{
		return 0;
	}

	if (strcmp(variable, "passes") == 0)        /* passes=? */
	{
		if (charval[0] != '1' && charval[0] != '2' && charval[0] != '3'
			&& charval[0] != 'g' && charval[0] != 'b'
			&& charval[0] != 't' && charval[0] != 's'
			&& charval[0] != 'd' && charval[0] != 'o')
		{
			goto badarg;
		}
		usr_stdcalcmode = charval[0];
		if (charval[0] == 'g')
		{
			stoppass = ((int)value[1] - (int)'0');
			if (stoppass < 0 || stoppass > 6)
			{
				stoppass = 0;
			}
		}
		return 1;
	}

	if (strcmp(variable, "ismand") == 0)        /* ismand=? */
	{
		if (yesnoval[0] < 0)
		{
			goto badarg;
		}
		ismand = (short int)yesnoval[0];
		return 1;
	}

	if (strcmp(variable, "cyclelimit") == 0)   /* cyclelimit=? */
	{
		if (numval <= 1 || numval > 256)
		{
			goto badarg;
		}
		initcyclelimit = numval;
		return 0;
	}

	if (strcmp(variable, "makemig") == 0)
	{
		int xmult, ymult;
		if (totparms < 2)
		{
			goto badarg;
		}
		xmult = intval[0];
		ymult = intval[1];
		make_mig(xmult, ymult);
#ifndef WINFRACT
		exit(0);
#endif
	}

	if (strcmp(variable, "cyclerange") == 0)
	{
		if (totparms < 2)
		{
			intval[1] = 255;
		}
		if (totparms < 1)
		{
			intval[0] = 1;
		}
		if (totparms != intparms
			|| intval[0] < 0 || intval[1] > 255 || intval[0] > intval[1])
		{
			goto badarg;
		}
		rotate_lo = intval[0];
		rotate_hi = intval[1];
		return 0;
	}

	if (strcmp(variable, "ranges") == 0)
	{
		int i, j, entries, prev;
		int tmpranges[128];

		if (totparms != intparms)
		{
			goto badarg;
		}
		entries = prev = i = 0;
		LogFlag = 0; /* ranges overrides logmap */
		while (i < totparms)
		{
			if ((j = intval[i++]) < 0) /* striping */
			{
				if ((j = 0-j) < 1 || j >= 16384 || i >= totparms)
				{
					goto badarg;
				}
				tmpranges[entries++] = -1; /* {-1,width,limit} for striping */
				tmpranges[entries++] = j;
				j = intval[i++];
			}
			if (j < prev)
			{
				goto badarg;
			}
			tmpranges[entries++] = prev = j;
		}
		if (prev == 0)
		{
			goto badarg;
		}
		ranges = (int *)malloc(sizeof(int)*entries);
		if (ranges == NULL)
		{
			stopmsg(STOPMSG_NO_STACK, "Insufficient memory for ranges=");
			return -1;
		}
		rangeslen = entries;
		for (i = 0; i < rangeslen; ++i)
		{
			ranges[i] = tmpranges[i];
		}
		return 1;
	}

	if (strcmp(variable, "savename") == 0)      /* savename=? */
	{
		if (valuelen > (FILE_MAX_PATH-1))
		{
			goto badarg;
		}
		if (first_init || mode == CMDFILE_AT_AFTER_STARTUP)
		{
			if (merge_pathnames(savename, value, mode) < 0)
			{
				init_msg(variable, value, mode);
			}
		}
		return 0;
	}

	if (strcmp(variable, "tweaklzw") == 0)      /* tweaklzw=? */
	{
		if (totparms >= 1)
		{
			lzw[0] = intval[0];
		}
		if (totparms >= 2)
		{
			lzw[1] = intval[1];
		}
		return 0;
	}

	if (strcmp(variable, "minstack") == 0)      /* minstack=? */
	{
		if (totparms != 1)
		{
			goto badarg;
		}
		minstack = intval[0];
		return 0;
	}

	if (strcmp(variable, "mathtolerance") == 0)      /* mathtolerance=? */
	{
		if (charval[0] == '/')
		{
			; /* leave math_tol[0] at the default value */
		}
		else if (totparms >= 1)
		{
			math_tol[0] = floatval[0];
		}
		if (totparms >= 2)
		{
			math_tol[1] = floatval[1];
		}
		return 0;
	}

	if (strcmp(variable, "tempdir") == 0)      /* tempdir=? */
	{
		if (valuelen > (FILE_MAX_DIR-1))
		{
			goto badarg;
		}
		if (isadirectory(value) == 0)
		{
			goto badarg;
		}
		strcpy(tempdir, value);
		fix_dirname(tempdir);
		return 0;
	}

	if (strcmp(variable, "workdir") == 0)      /* workdir=? */
	{
		if (valuelen > (FILE_MAX_DIR-1))
		{
			goto badarg;
		}
		if (isadirectory(value) == 0)
		{
			goto badarg;
		}
		strcpy(workdir, value);
		fix_dirname(workdir);
		return 0;
	}

	if (strcmp(variable, "exitmode") == 0)      /* exitmode=? */
	{
		sscanf(value, "%x", &numval);
		exitmode = (BYTE)numval;
		return 0;
	}

	if (strcmp(variable, "textcolors") == 0)
	{
		parse_textcolors(value);
		return 0;
	}

	if (strcmp(variable, "potential") == 0)     /* potential=? */
	{
		k = 0;
		while (k < 3 && *value)
		{
			if (k==1)
			{
				potparam[k] = atof(value);
			}
			else
			{
				potparam[k] = atoi(value);
			}
			k++;
			value = strchr(value, '/');
			if (value == NULL)
			{
				k = 99;
			}
			++value;
		}
		pot16bit = 0;
		if (k < 99)
		{
			if (strcmp(value, "16bit"))
			{
				goto badarg;
			}
			pot16bit = 1;
		}
		return 1;
	}

	if (strcmp(variable, "params") == 0)        /* params=?,? */
	{
		if (totparms != floatparms || totparms > MAXPARAMS)
		{
			goto badarg;
		}
		initparams = 1;
		for (k = 0; k < MAXPARAMS; ++k)
		{
			param[k] = (k < totparms) ? floatval[k] : 0.0;
		}
		if (bf_math)
		{
			for (k = 0; k < MAXPARAMS; k++)
			{
				floattobf(bfparms[k], param[k]);
			}
		}
		return 1;
	}

	if (strcmp(variable, "miim") == 0)          /* miim=?[/?[/?[/?]]] */
	{
		if (totparms > 6)
		{
			goto badarg;
		}
		if (charval[0] == 'b')
		{
			major_method = breadth_first;
		}
		else if (charval[0] == 'd')
		{
			major_method = depth_first;
		}
		else if (charval[0] == 'w')
		{
			major_method = random_walk;
		}
#ifdef RANDOM_RUN
		else if (charval[0] == 'r')
		{
			major_method = random_run;
		}
#endif
		else
		{
			goto badarg;
		}

		if (charval[1] == 'l')
		{
			minor_method = left_first;
		}
		else if (charval[1] == 'r')
		{
			minor_method = right_first;
		}
		else
		{
			goto badarg;
		}

		/* keep this next part in for backwards compatibility with old PARs ??? */

		if (totparms > 2)
		{
			for (k = 2; k < 6; ++k)
			{
				param[k-2] = (k < totparms) ? floatval[k] : 0.0;
			}
		}

		return 1;
	}

	if (strcmp(variable, "initorbit") == 0)     /* initorbit=?,? */
	{
		if (strcmp(value, "pixel")==0)
		{
			useinitorbit = 2;
		}
		else
		{
			if (totparms != 2 || floatparms != 2)
			{
				goto badarg;
			}
			initorbit.x = floatval[0];
			initorbit.y = floatval[1];
			useinitorbit = 1;
		}
		return 1;
	}

	if (strcmp(variable, "orbitname") == 0)         /* orbitname=? */
	{
		if (check_orbit_name(value))
		{
			goto badarg;
		}
		return 1;
	}

	if (strcmp(variable, "3dmode") == 0)         /* orbitname=? */
	{
		int i, j;
		j = -1;
		for (i = 0; i < 4; i++)
		{
			if (strcmp(value, juli3Doptions[i]) == 0)
			{
				j = i;
			}
		}
		if (j < 0)
		{
			goto badarg;
		}
		else
		{
			juli3Dmode = j;
		}
		return 1;
	}

	if (strcmp(variable, "julibrot3d") == 0)       /* julibrot3d=?,?,?,? */
	{
		if (floatparms != totparms)
		{
			goto badarg;
		}
		if (totparms > 0)
		{
			zdots = (int)floatval[0];
		}
		if (totparms > 1)
		{
			originfp = (float)floatval[1];
		}
		if (totparms > 2)
		{
			depthfp = (float)floatval[2];
		}
		if (totparms > 3)
		{
			heightfp = (float)floatval[3];
		}
		if (totparms > 4)
		{
			widthfp = (float)floatval[4];
		}
		if (totparms > 5)
		{
			distfp = (float)floatval[5];
		}
		return 1;
	}

	if (strcmp(variable, "julibroteyes") == 0)       /* julibroteyes=?,?,?,? */
	{
		if (floatparms != totparms || totparms != 1)
		{
			goto badarg;
		}
		eyesfp =  (float)floatval[0];
		return 1;
	}

	if (strcmp(variable, "julibrotfromto") == 0)       /* julibrotfromto=?,?,?,? */
	{
		if (floatparms != totparms || totparms != 4)
		{
			goto badarg;
		}
		mxmaxfp = floatval[0];
		mxminfp = floatval[1];
		mymaxfp = floatval[2];
		myminfp = floatval[3];
		return 1;
	}

	if (strcmp(variable, "corners") == 0)       /* corners=?,?,?,? */
	{
		int dec;
		if (fractype == CELLULAR)
		{
			return 1; /* skip setting the corners */
		}
#if 0
		/* use a debugger and OutputDebugString instead of standard I/O on Windows */
		printf("totparms %d floatparms %d\n", totparms, floatparms);
		getch();
#endif
		if (floatparms != totparms
			|| (totparms != 0 && totparms != 4 && totparms != 6))
		{
			goto badarg;
		}
		usemag = 0;
		if (totparms == 0)
		{
			return 0; /* turns corners mode on */
		}
		initcorners = 1;
		/* good first approx, but dec could be too big */
		dec = get_max_curarg_len(floatvalstr, totparms) + 1;
		if ((dec > DBL_DIG+1 || debugflag == 3200) && debugflag != 3400)
		{
			int old_bf_math;

			old_bf_math = bf_math;
			if (!bf_math || dec > decimals)
			{
				init_bf_dec(dec);
			}
			if (old_bf_math == 0)
			{
				int k;
				for (k = 0; k < MAXPARAMS; k++)
				{
					floattobf(bfparms[k], param[k]);
				}
			}

			/* xx3rd = xxmin = floatval[0]; */
			get_bf(bfxmin, floatvalstr[0]);
			get_bf(bfx3rd, floatvalstr[0]);

			/* xxmax = floatval[1]; */
			get_bf(bfxmax, floatvalstr[1]);

			/* yy3rd = yymin = floatval[2]; */
			get_bf(bfymin, floatvalstr[2]);
			get_bf(bfy3rd, floatvalstr[2]);

			/* yymax = floatval[3]; */
			get_bf(bfymax, floatvalstr[3]);

			if (totparms == 6)
			{
				/* xx3rd = floatval[4]; */
				get_bf(bfx3rd, floatvalstr[4]);

				/* yy3rd = floatval[5]; */
				get_bf(bfy3rd, floatvalstr[5]);
			}

			/* now that all the corners have been read in, get a more */
			/* accurate value for dec and do it all again             */

			dec = getprecbf_mag();
			if (dec < 0)
			{
				goto badarg;     /* ie: Magnification is +-1.#INF */
			}

			if (dec > decimals)  /* get corners again if need more precision */
			{
				init_bf_dec(dec);

				/* now get parameters and corners all over again at new
				decimal setting */
				for (k = 0; k < MAXPARAMS; k++)
				{
					floattobf(bfparms[k], param[k]);
				}

				/* xx3rd = xxmin = floatval[0]; */
				get_bf(bfxmin, floatvalstr[0]);
				get_bf(bfx3rd, floatvalstr[0]);

				/* xxmax = floatval[1]; */
				get_bf(bfxmax, floatvalstr[1]);

				/* yy3rd = yymin = floatval[2]; */
				get_bf(bfymin, floatvalstr[2]);
				get_bf(bfy3rd, floatvalstr[2]);

				/* yymax = floatval[3]; */
				get_bf(bfymax, floatvalstr[3]);

				if (totparms == 6)
				{
					/* xx3rd = floatval[4]; */
					get_bf(bfx3rd, floatvalstr[4]);

					/* yy3rd = floatval[5]; */
					get_bf(bfy3rd, floatvalstr[5]);
				}
			}
		}
		xx3rd = xxmin = floatval[0];
		xxmax =         floatval[1];
		yy3rd = yymin = floatval[2];
		yymax =         floatval[3];

		if (totparms == 6)
		{
			xx3rd =      floatval[4];
			yy3rd =      floatval[5];
		}
		return 1;
	}

	if (strcmp(variable, "orbitcorners") == 0)  /* orbit corners=?,?,?,? */
	{
		set_orbit_corners = 0;
		if (floatparms != totparms
			|| (totparms != 0 && totparms != 4 && totparms != 6))
		{
			goto badarg;
		}
		ox3rd = oxmin = floatval[0];
		oxmax =         floatval[1];
		oy3rd = oymin = floatval[2];
		oymax =         floatval[3];

		if (totparms == 6)
		{
			ox3rd =      floatval[4];
			oy3rd =      floatval[5];
		}
		set_orbit_corners = 1;
		keep_scrn_coords = 1;
		return 1;
	}

	if (strcmp(variable, "screencoords") == 0)     /* screencoords=?   */
	{
		if (yesnoval[0] < 0)
		{
			goto badarg;
		}
		keep_scrn_coords = yesnoval[0];
		return 1;
	}

	if (strcmp(variable, "orbitdrawmode") == 0)     /* orbitdrawmode=? */
	{
		if (charval[0] != 'l' && charval[0] != 'r' && charval[0] != 'f')
		{
			goto badarg;
		}
		drawmode = charval[0];
		return 1;
	}

   if (strcmp(variable, "viewwindows") == 0) {  /* viewwindows=?,?,?,?,? */
      if (totparms > 5 || floatparms-intparms > 2 || intparms > 4)
         goto badarg;
      viewwindow = 1;
      viewreduction = 4.2f;  /* reset default values */
      finalaspectratio = screenaspect;
      viewcrop = 1; /* yes */
      viewxdots = viewydots = 0;

      if ((totparms > 0) && (floatval[0] > 0.001))
        viewreduction = (float)floatval[0];
      if ((totparms > 1) && (floatval[1] > 0.001))
        finalaspectratio = (float)floatval[1];
      if ((totparms > 2) && (yesnoval[2] == 0))
        viewcrop = yesnoval[2];
      if ((totparms > 3) && (intval[3] > 0))
        viewxdots = intval[3];
      if ((totparms == 5) && (intval[4] > 0))
        viewydots = intval[4];
      return 1;
      }

   if (strcmp(variable, "center-mag") == 0) {    /* center-mag=?,?,?[,?,?,?] */
      int dec;

      if ( (totparms != floatparms)
        || (totparms != 0 && totparms < 3)
        || (totparms >= 3 && floatval[2] == 0.0))
         goto badarg;
      if (fractype == CELLULAR)
          return 1; /* skip setting the corners */
      usemag = 1;
      if (totparms == 0) return 0; /* turns center-mag mode on */
      initcorners = 1;
      /* dec = get_max_curarg_len(floatvalstr, totparms); */
#ifdef USE_LONG_DOUBLE
      sscanf(floatvalstr[2], "%Lf", &Magnification);
#else
      sscanf(floatvalstr[2], "%lf", &Magnification);
#endif

      /* I don't know if this is portable, but something needs to */
      /* be used in case compiler's LDBL_MAX is not big enough    */
      if (Magnification > LDBL_MAX || Magnification < -LDBL_MAX)
         goto badarg;     /* ie: Magnification is +-1.#INF */

      dec = getpower10(Magnification) + 4; /* 4 digits of padding sounds good */

      if ((dec <= DBL_DIG+1 && debugflag != 3200) || debugflag == 3400) { /* rough estimate that double is OK */
         Xctr = floatval[0];
         Yctr = floatval[1];
         /* Magnification = floatval[2]; */  /* already done above */
         Xmagfactor = 1;
         Rotation = 0;
         Skew = 0;
         if (floatparms > 3)
            Xmagfactor = floatval[3];
         if (Xmagfactor == 0)
            Xmagfactor = 1;
         if (floatparms > 4)
            Rotation = floatval[4];
         if (floatparms > 5)
            Skew = floatval[5];
         /* calculate bounds */
         cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
         return 1;
      }
      else { /* use arbitrary precision */
         int old_bf_math;
         int saved;
         initcorners = 1;
         old_bf_math = bf_math;
         if (!bf_math || dec > decimals)
            init_bf_dec(dec);
         if (old_bf_math == 0) {
            int k;
            for (k = 0; k < MAXPARAMS; k++)
               floattobf(bfparms[k], param[k]);
         }
         usemag = 1;
         saved = save_stack();
         bXctr            = alloc_stack(bflength+2);
         bYctr            = alloc_stack(bflength+2);
         /* Xctr = floatval[0]; */
         get_bf(bXctr, floatvalstr[0]);
         /* Yctr = floatval[1]; */
         get_bf(bYctr, floatvalstr[1]);
         /* Magnification = floatval[2]; */  /* already done above */
         Xmagfactor = 1;
         Rotation = 0;
         Skew = 0;
         if (floatparms > 3)
            Xmagfactor = floatval[3];
         if (Xmagfactor == 0)
            Xmagfactor = 1;
         if (floatparms > 4)
            Rotation = floatval[4];
         if (floatparms > 5)
            Skew = floatval[5];
         /* calculate bounds */
         cvtcornersbf(bXctr, bYctr, Magnification, Xmagfactor, Rotation, Skew);
         bfcornerstofloat();
         restore_stack(saved);
         return 1;
      }
   }

   if (strcmp(variable, "aspectdrift") == 0 ) {  /* aspectdrift=? */
      if (floatparms != 1 || floatval[0] < 0)
         goto badarg;
      aspectdrift = (float)floatval[0];
      return 1;
      }

   if (strcmp(variable, "invert") == 0) {        /* invert=?,?,? */
      if (totparms != floatparms || (totparms != 1 && totparms != 3))
         goto badarg;
      invert = ((inversion[0] = floatval[0]) != 0.0) ? totparms : 0;
      if (totparms == 3) {
         inversion[1] = floatval[1];
         inversion[2] = floatval[2];
         }
      return 1;
      }

   if (strcmp(variable, "olddemmcolors") == 0 ) {     /* olddemmcolors=?   */
      if (yesnoval[0] < 0) goto badarg;
      old_demm_colors = yesnoval[0];
      return 0;
      }

   if (strcmp(variable, "askvideo") == 0 ) {     /* askvideo=?   */
      if (yesnoval[0] < 0) goto badarg;
      askvideo = yesnoval[0];
      return 0;
      }

   if (strcmp(variable, "ramvideo") == 0 )       /* ramvideo=?   */
      return 0; /* just ignore and return, for old time's sake */

   if (strcmp(variable, "float") == 0 ) {        /* float=? */
      if (yesnoval[0] < 0) goto badarg;
#ifndef XFRACT
      usr_floatflag = (char)yesnoval[0];
#else
      usr_floatflag = 1; /* must use floating point */
#endif
      return 3;
      }

   if (strcmp(variable, "fastrestore") == 0 ) {   /* fastrestore=? */
      if (yesnoval[0] < 0) goto badarg;
      fastrestore = (char)yesnoval[0];
      return 0;
      }

   if (strcmp(variable, "orgfrmdir") == 0 ) {   /* orgfrmdir=? */
      if (valuelen > (FILE_MAX_DIR-1)) goto badarg;
      if (isadirectory(value) == 0) goto badarg;
      orgfrmsearch = 1;
      strcpy(orgfrmdir, value);
      fix_dirname(orgfrmdir);
      return 0;
   }

   if (strcmp(variable, "biomorph") == 0 ) {     /* biomorph=? */
      usr_biomorph = numval;
      return 1;
      }

   if (strcmp(variable, "orbitsave") == 0 ) {     /* orbitsave=? */
      if (charval[0] == 's')
         orbitsave |= 2;
      else if (yesnoval[0] < 0) goto badarg;
      orbitsave |= yesnoval[0];
      return 1;
      }

   if (strcmp(variable, "bailout") == 0 ) {      /* bailout=? */
      if (floatval[0] < 1 || floatval[0] > 2100000000L) goto badarg;
      bailout = (long)floatval[0];
      return 1;
      }

   if (strcmp(variable, "bailoutest") == 0 ) {   /* bailoutest=? */
      if     (strcmp(value, "mod" )==0) bailoutest = Mod;
      else if (strcmp(value, "real")==0) bailoutest = Real;
      else if (strcmp(value, "imag")==0) bailoutest = Imag;
      else if (strcmp(value, "or"  )==0) bailoutest = Or;
      else if (strcmp(value, "and" )==0) bailoutest = And;
      else if (strcmp(value, "manh")==0) bailoutest = Manh;
      else if (strcmp(value, "manr")==0) bailoutest = Manr;
      else goto badarg;
      setbailoutformula(bailoutest);
      return 1;
      }

   if (strcmp(variable, "symmetry") == 0 ) {     /* symmetry=? */
      if     (strcmp(value, "xaxis" )==0) forcesymmetry = XAXIS;
      else if (strcmp(value, "yaxis" )==0) forcesymmetry = YAXIS;
      else if (strcmp(value, "xyaxis")==0) forcesymmetry = XYAXIS;
      else if (strcmp(value, "origin")==0) forcesymmetry = ORIGIN;
      else if (strcmp(value, "pi"    )==0) forcesymmetry = PI_SYM;
      else if (strcmp(value, "none"  )==0) forcesymmetry = NOSYM;
      else goto badarg;
      return 1;
      }

	/* deprecated print parameters */
	if ((strcmp(variable, "printer") == 0)
		|| (strcmp(variable, "printfile") == 0)
		|| (strcmp(variable, "rleps") == 0)
		|| (strcmp(variable, "colorps") == 0)
		|| (strcmp(variable, "epsf") == 0)
   		|| (strcmp(variable, "title") == 0)
   		|| (strcmp(variable, "translate") == 0)
   		|| (strcmp(variable, "plotstyle") == 0)
   		|| (strcmp(variable, "halftone") == 0)
   		|| (strcmp(variable, "linefeed") == 0)
   		|| (strcmp(variable, "comport") == 0))
	{
		return 0;
	}

   if (strcmp(variable, "sound") == 0 ) {        /* sound=?,?,? */
      if (totparms > 5)
         goto badarg;
      soundflag = SOUNDFLAG_OFF; /* start with a clean slate, add bits as we go */
      if (totparms == 1)
         soundflag = SOUNDFLAG_SPEAKER; /* old command, default to PC speaker */

      /* soundflag is used as a bitfield... bit 0,1,2 used for whether sound
         is modified by an orbits x,y,or z component. and also to turn it on
         or off (0==off, 1==beep (or yes), 2==x, 3==y, 4==z),
         Bit 3 is used for flagging the PC speaker sound,
         Bit 4 for OPL3 FM soundcard output,
         Bit 5 will be for midi output (not yet),
         Bit 6 for whether the tone is quantised to the nearest 'proper' note
          (according to the western, even tempered system anyway) */

      if (charval[0] == 'n' || charval[0] == 'o')
         soundflag &= ~SOUNDFLAG_ORBITMASK; 
      else if ((strncmp(value, "ye", 2) == 0) || (charval[0] == 'b'))
         soundflag |= SOUNDFLAG_BEEP;
      else if (charval[0] == 'x')
         soundflag |= SOUNDFLAG_X;
      else if (charval[0] == 'y' && strncmp(value, "ye", 2) != 0)
         soundflag |= SOUNDFLAG_Y;
      else if (charval[0] == 'z')
         soundflag |= SOUNDFLAG_Z;
      else
         goto badarg;
#if !defined(XFRACT)
      if (totparms > 1) {
       int i;
         soundflag &= SOUNDFLAG_ORBITMASK; /* reset options */
         for (i = 1; i < totparms; i++) {
          /* this is for 2 or more options at the same time */
            if (charval[i] == 'f') { /* (try to)switch on opl3 fm synth */
               if (driver_init_fm())
                  soundflag |= SOUNDFLAG_OPL3_FM;
               else
				   soundflag &= ~SOUNDFLAG_OPL3_FM;
            }
            else if (charval[i] == 'p')
               soundflag |= SOUNDFLAG_SPEAKER;
            else if (charval[i] == 'm')
               soundflag |= SOUNDFLAG_MIDI;
            else if (charval[i] == 'q')
               soundflag |= SOUNDFLAG_QUANTIZED;
            else
               goto badarg;
         } /* end for */
      }    /* end totparms > 1 */
      return 0;
      }

   if (strcmp(variable, "hertz") == 0) {         /* Hertz=? */
      basehertz = numval;
      return 0;
      }

   if (strcmp(variable, "volume") == 0) {         /* Volume =? */
      fm_vol = numval & 0x3F; /* 63 */
      return 0;
      }

   if (strcmp(variable, "attenuate") == 0) {
      if (charval[0] == 'n')
         hi_atten = 0;
      else if (charval[0] == 'l')
         hi_atten = 1;
      else if (charval[0] == 'm')
         hi_atten = 2;
      else if (charval[0] == 'h')
         hi_atten = 3;
      else
         goto badarg;
      return 0;
      }

   if (strcmp(variable, "polyphony") == 0) {
      if (numval > 9)
         goto badarg;
      polyphony = abs(numval-1);
      return 0;
   } 

   if (strcmp(variable, "wavetype") == 0) { /* wavetype = ? */
      fm_wavetype = numval & 0x0F;
      return 0;
   }

   if (strcmp(variable, "attack") == 0) { /* attack = ? */
      fm_attack = numval & 0x0F;
      return 0;
   }

   if (strcmp(variable, "decay") == 0) { /* decay = ? */
      fm_decay = numval & 0x0F;
      return 0;
   }

   if (strcmp(variable, "sustain") == 0) { /* sustain = ? */
      fm_sustain = numval & 0x0F;
      return 0;
   }
   
   if (strcmp(variable, "srelease") == 0) { /* release = ? */
      fm_release = numval & 0x0F;
      return 0;
   }

   if (strcmp(variable, "scalemap") == 0) {      /* Scalemap=?,?,?,?,?,?,?,?,?,?,? */
      int counter;
      if (totparms != intparms) goto badarg;
      for (counter=0; counter <=11; counter++)
         if ((totparms > counter) && (intval[counter] > 0)
           && (intval[counter] < 13))
             scale_map[counter] = intval[counter];
#endif
      return 0;
   } 

   if (strcmp(variable, "periodicity") == 0 ) {  /* periodicity=? */
      usr_periodicitycheck=1;
      if ((charval[0] == 'n') || (numval == 0))
         usr_periodicitycheck=0;
      else if (charval[0] == 'y')
         usr_periodicitycheck=1;
      else if (charval[0] == 's')   /* 's' for 'show' */
         usr_periodicitycheck= -1;
      else if (numval == NONNUMERIC)
         goto badarg;
      else if (numval != 0)
         usr_periodicitycheck=numval;
      if (usr_periodicitycheck > 255) usr_periodicitycheck = 255;
      if (usr_periodicitycheck < -255) usr_periodicitycheck = -255;
      return 1;
      }

   if (strcmp(variable, "logmap") == 0 ) {       /* logmap=? */
      Log_Auto_Calc = 0;   /* turn this off if loading a PAR */
      if (charval[0] == 'y')
         LogFlag = 1;                           /* palette is logarithmic */
      else if (charval[0] == 'n')
         LogFlag = 0;
      else if (charval[0] == 'o')
         LogFlag = -1;                          /* old log palette */
      else
         LogFlag = (long)floatval[0];
      return 1;
      }

   if (strcmp(variable, "logmode") == 0 ) {       /* logmode=? */
      Log_Fly_Calc = 0;                         /* turn off if error */
      Log_Auto_Calc = 0;
      if (charval[0] == 'f')
         Log_Fly_Calc = 1;                      /* calculate on the fly */
      else if (charval[0] == 't')
         Log_Fly_Calc = 2;                      /* force use of LogTable */
      else if (charval[0] == 'a') {
         Log_Auto_Calc = 1;                     /* force auto calc of logmap */
      }
      else goto badarg;
      return 1;
      }

   if (strcmp(variable, "debugflag") == 0
     || strcmp(variable, "debug") == 0) {        /* internal use only */
      debugflag = numval;
      timerflag = debugflag & 1;                /* separate timer flag */
      debugflag -= timerflag;
      return 0;
      }

   if (strcmp(variable, "rseed") == 0) {
      rseed = numval;
      rflag = 1;
      return 1;
      }

   if (strcmp(variable, "orbitdelay") == 0) {
      orbit_delay = numval;
      return 0;
      }

   if (strcmp(variable, "orbitinterval") == 0) {
      orbit_interval = numval;
      if (orbit_interval < 1)
         orbit_interval = 1;
      if (orbit_interval > 255)
         orbit_interval = 255;
      return 0;
      }

   if (strcmp(variable, "showdot") == 0) {
      showdot = 15;
      if (totparms > 0)
      {
         autoshowdot = (char)0;
         if (isalpha(charval[0]))
         {
            if (strchr("abdm", (int)charval[0]) != NULL)
               autoshowdot = charval[0];
            else
               goto badarg;
         }
         else   
         {
            showdot=numval;
            if (showdot<0)
               showdot=-1;
         }
         if (totparms > 1 && intparms > 0)
            sizedot = intval[1];
         if (sizedot < 0)
            sizedot = 0;   
      }      
      return 0;
      }

   if (strcmp(variable, "showorbit") == 0) {  /* showorbit=yes|no */
      start_showorbit=(char)yesnoval[0];
      return 0;
      }

   if (strcmp(variable, "decomp") == 0) {
      if (totparms != intparms || totparms < 1) goto badarg;
      decomp[0] = intval[0];
      decomp[1] = 0;
      if (totparms > 1) /* backward compatibility */
         bailout = decomp[1] = intval[1];
      return 1;
      }

   if (strcmp(variable, "distest") == 0) {
      if (totparms != intparms || totparms < 1) goto badarg;
      usr_distest = (long)floatval[0];
      distestwidth = 71;
      if (totparms > 1)
         distestwidth = intval[1];
      if (totparms > 3 && intval[2] > 0 && intval[3] > 0) {
         pseudox = intval[2];
         pseudoy = intval[3];
      }
      else
        pseudox = pseudoy = 0;
      return 1;
      }

   if (strcmp(variable, "formulafile") == 0) {   /* formulafile=? */
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if (merge_pathnames(FormFileName, value, mode)<0)
         init_msg(variable, value, mode);
      return 1;
      }

   if (strcmp(variable, "formulaname") == 0) {   /* formulaname=? */
      if (valuelen > ITEMNAMELEN) goto badarg;
      strcpy(FormName, value);
      return 1;
      }

   if (strcmp(variable, "lfile") == 0) {    /* lfile=? */
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if (merge_pathnames(LFileName, value, mode)<0)
         init_msg(variable, value, mode);
      return 1;
      }

   if (strcmp(variable, "lname") == 0) {
      if (valuelen > ITEMNAMELEN) goto badarg;
      strcpy(LName, value);
      return 1;
      }

   if (strcmp(variable, "ifsfile") == 0) {    /* ifsfile=?? */
      int existdir;
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      existdir=merge_pathnames(IFSFileName, value, mode);
	  if (existdir==0)
         reset_ifs_defn();
      else if (existdir < 0)
         init_msg(variable, value, mode);
      return 1;
      }


   if (strcmp(variable, "ifs") == 0
     || strcmp(variable, "ifs3d") == 0) {        /* ifs3d for old time's sake */
      if (valuelen > ITEMNAMELEN) goto badarg;
      strcpy(IFSName, value);
      reset_ifs_defn();
      return 1;
      }

   if (strcmp(variable, "parmfile") == 0) {   /* parmfile=? */
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if (merge_pathnames(CommandFile, value, mode)<0)
         init_msg(variable, value, mode);
      return 1;
      }

   if (strcmp(variable, "stereo") == 0) {        /* stereo=? */
      if ((numval<0) || (numval>4)) goto badarg;
      g_glasses_type = numval;
      return 3;
      }

   if (strcmp(variable, "rotation") == 0) {      /* rotation=?/?/? */
      if (totparms != 3 || intparms != 3) goto badarg;
      XROT = intval[0];
      YROT = intval[1];
      ZROT = intval[2];
      return 3;
      }

   if (strcmp(variable, "perspective") == 0) {   /* perspective=? */
      if (numval == NONNUMERIC) goto badarg;
      ZVIEWER = numval;
      return 3;
      }

   if (strcmp(variable, "xyshift") == 0) {       /* xyshift=?/?  */
      if (totparms != 2 || intparms != 2) goto badarg;
      XSHIFT = intval[0];
      YSHIFT = intval[1];
      return 3;
      }

   if (strcmp(variable, "interocular") == 0) {   /* interocular=? */
      g_eye_separation = numval;
      return 3;
      }

   if (strcmp(variable, "converge") == 0) {      /* converg=? */
      xadjust = numval;
      return 3;
      }

   if (strcmp(variable, "crop") == 0) {          /* crop=? */
      if (totparms != 4 || intparms != 4
        || intval[0] < 0 || intval[0] > 100
        || intval[1] < 0 || intval[1] > 100
        || intval[2] < 0 || intval[2] > 100
        || intval[3] < 0 || intval[3] > 100)
          goto badarg;
      red_crop_left   = intval[0];
      red_crop_right  = intval[1];
      blue_crop_left  = intval[2];
      blue_crop_right = intval[3];
      return 3;
      }

   if (strcmp(variable, "bright") == 0) {        /* bright=? */
      if (totparms != 2 || intparms != 2) goto badarg;
      red_bright  = intval[0];
      blue_bright = intval[1];
      return 3;
      }

   if (strcmp(variable, "xyadjust") == 0) {      /* trans=? */
      if (totparms != 2 || intparms != 2) goto badarg;
      xtrans = intval[0];
      ytrans = intval[1];
      return 3;
      }

   if (strcmp(variable, "3d") == 0) {            /* 3d=?/?/..    */
      if (strcmp(value, "overlay")==0) {
         yesnoval[0]=1;
         if (calc_status > CALCSTAT_NO_FRACTAL) /* if no image, treat same as 3D=yes */
            overlay3d=1;
      }
      else if (yesnoval[0] < 0) goto badarg;
      display3d = yesnoval[0];
      initvars_3d();
      return (display3d) ? 6 : 2;
      }

   if (strcmp(variable, "sphere") == 0 ) {       /* sphere=? */
      if (yesnoval[0] < 0) goto badarg;
      SPHERE = yesnoval[0];
      return 2;
      }

   if (strcmp(variable, "scalexyz") == 0) {      /* scalexyz=?/?/? */
      if (totparms < 2 || intparms != totparms) goto badarg;
      XSCALE = intval[0];
      YSCALE = intval[1];
      if (totparms > 2) ROUGH = intval[2];
      return 2;
      }

   /* "rough" is really scale z, but we add it here for convenience */
   if (strcmp(variable, "roughness") == 0) {     /* roughness=?  */
      ROUGH = numval;
      return 2;
      }

   if (strcmp(variable, "waterline") == 0) {     /* waterline=?  */
      if (numval<0) goto badarg;
      WATERLINE = numval;
      return 2;
      }

   if (strcmp(variable, "filltype") == 0) {      /* filltype=?   */
      if (numval < -1 || numval > 6) goto badarg;
      FILLTYPE = numval;
      return 2;
      }

   if (strcmp(variable, "lightsource") == 0) {   /* lightsource=?/?/? */
      if (totparms != 3 || intparms != 3) goto badarg;
      XLIGHT = intval[0];
      YLIGHT = intval[1];
      ZLIGHT = intval[2];
      return 2;
      }

   if (strcmp(variable, "smoothing") == 0) {     /* smoothing=?  */
      if (numval<0) goto badarg;
      LIGHTAVG = numval;
      return 2;
      }

   if (strcmp(variable, "latitude") == 0) {      /* latitude=?/? */
      if (totparms != 2 || intparms != 2) goto badarg;
      THETA1 = intval[0];
      THETA2 = intval[1];
      return 2;
      }

   if (strcmp(variable, "longitude") == 0) {     /* longitude=?/? */
      if (totparms != 2 || intparms != 2) goto badarg;
      PHI1 = intval[0];
      PHI2 = intval[1];
      return 2;
      }

   if (strcmp(variable, "radius") == 0) {        /* radius=? */
      if (numval < 0) goto badarg;
      RADIUS = numval;
      return 2;
      }

   if (strcmp(variable, "transparent") == 0) {   /* transparent? */
      if (totparms != intparms || totparms < 1) goto badarg;
      transparent[1] = transparent[0] = intval[0];
      if (totparms > 1) transparent[1] = intval[1];
      return 2;
      }

   if (strcmp(variable, "preview") == 0) {       /* preview? */
      if (yesnoval[0] < 0) goto badarg;
      preview = (char)yesnoval[0];
      return 2;
      }

   if (strcmp(variable, "showbox") == 0) {       /* showbox? */
      if (yesnoval[0] < 0) goto badarg;
      showbox = (char)yesnoval[0];
      return 2;
      }

   if (strcmp(variable, "coarse") == 0) {        /* coarse=? */
      if (numval < 3 || numval > 2000) goto badarg;
      previewfactor = numval;
      return 2;
      }

   if (strcmp(variable, "randomize") == 0) {     /* RANDOMIZE=? */
      if (numval<0 || numval>7) goto badarg;
      RANDOMIZE = numval;
      return 2;
      }

   if (strcmp(variable, "ambient") == 0) {       /* ambient=? */
      if (numval<0||numval>100) goto badarg;
      Ambient = numval;
      return 2;
      }

   if (strcmp(variable, "haze") == 0) {          /* haze=? */
      if (numval<0||numval>100) goto badarg;
      haze = numval;
      return 2;
      }

   if (strcmp(variable, "fullcolor") == 0) {     /* fullcolor=? */
      if (yesnoval[0] < 0) goto badarg;
      Targa_Out = yesnoval[0];
      return 2;
      }

   if (strcmp(variable, "truecolor") == 0) {     /* truecolor=? */
      if (yesnoval[0] < 0) goto badarg;
      truecolor = yesnoval[0];
      return 3;
      }

   if (strcmp(variable, "truemode") == 0) {    /* truemode=? */
      truemode = 0;                               /* use default if error */
      if (charval[0] == 'd')
         truemode = 0;                            /* use default color output */
      if (charval[0] == 'i' || intval[0] == 1)
         truemode = 1;                            /* use iterates output */
      if (intval[0] == 2)
         truemode = 2;
      if (intval[0] == 3)
         truemode = 3;
      return 3;
      }

   if (strcmp(variable, "usegrayscale") == 0) {     /* usegrayscale? */
      if (yesnoval[0] < 0) goto badarg;
      grayflag = (char)yesnoval[0];
      return 2;
      }

   if (strcmp(variable, "monitorwidth") == 0) {     /* monitorwidth=? */
      if (totparms != 1 || floatparms != 1) goto badarg;
      AutoStereo_width  = floatval[0];
      return 2;
      }

   if (strcmp(variable, "targa_overlay") == 0) {         /* Targa Overlay? */
      if (yesnoval[0] < 0) goto badarg;
      Targa_Overlay = yesnoval[0];
      return 2;
      }

   if (strcmp(variable, "background") == 0) {     /* background=?/? */
      if (totparms != 3 || intparms != 3) goto badarg;
                for (i=0;i<3;i++)
                        if (intval[i] & ~0xff)
                                goto badarg;
      back_color[0] = (BYTE)intval[0];
      back_color[1] = (BYTE)intval[1];
      back_color[2] = (BYTE)intval[2];
      return 2;
      }

   if (strcmp(variable, "lightname") == 0) {     /* lightname=?   */
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if (first_init || mode == CMDFILE_AT_AFTER_STARTUP)
         strcpy(light_name, value);
      return 0;
      }

   if (strcmp(variable, "ray") == 0) {           /* RAY=? */
      if (numval < 0 || numval > 6) goto badarg;
      RAY = numval;
      return 2;
      }

   if (strcmp(variable, "brief") == 0) {         /* BRIEF? */
      if (yesnoval[0] < 0) goto badarg;
      BRIEF = yesnoval[0];
      return 2;
      }

   if (strcmp(variable, "release") == 0) {       /* release */
      if (numval < 0) goto badarg;

      save_release = numval;
      return 2;
      }

   if (strcmp(variable, "curdir") == 0) {         /* curdir= */
      if (yesnoval[0] < 0) goto badarg;
      checkcurdir = yesnoval[0];
      return 0;
      }

	if (strcmp(variable, "virtual") == 0)         /* virtual= */
	{
		if (yesnoval[0] < 0)
		{
			goto badarg;
		}
		g_virtual_screens = yesnoval[0];
		return 1;
	}

badarg:
	argerror(curarg);
	return -1;
}

#ifdef _MSC_VER
#if (_MSC_VER >= 600)
#pragma optimize( "el", on )
#endif
#endif

/* Some routines broken out of above so compiler doesn't run out of heap: */

static void parse_textcolors(char *value)
{
   int i,j,k,hexval;
   if (strcmp(value, "mono") == 0) {
      for (k = 0; k < sizeof(txtcolor); ++k)
         txtcolor[k] = BLACK*16+WHITE;
   /* C_HELP_CURLINK = C_PROMPT_INPUT = C_CHOICE_CURRENT = C_GENERAL_INPUT
                     = C_AUTHDIV1 = C_AUTHDIV2 = WHITE*16+BLACK; */
      txtcolor[6] = txtcolor[12] = txtcolor[13] = txtcolor[14] = txtcolor[20]
                  = txtcolor[27] = txtcolor[28] = WHITE*16+BLACK;
      /* C_TITLE = C_HELP_HDG = C_HELP_LINK = C_PROMPT_HI = C_CHOICE_SP_KEYIN
                 = C_GENERAL_HI = C_DVID_HI = C_STOP_ERR
                 = C_STOP_INFO = BLACK*16+L_WHITE; */
      txtcolor[0] = txtcolor[2] = txtcolor[5] = txtcolor[11] = txtcolor[16]
                  = txtcolor[17] = txtcolor[22] = txtcolor[24]
                  = txtcolor[25] = BLACK*16+L_WHITE;
      }
   else {
      k = 0;
      while ( k < sizeof(txtcolor)) {
         if (*value == 0) break;
         if (*value != '/') {
            sscanf(value,"%x",&hexval);
            i = (hexval / 16) & 7;
            j = hexval & 15;
            if (i == j || (i == 0 && j == 8)) /* force contrast */
               j = 15;
            txtcolor[k] = (BYTE)(i * 16 + j);
            value = strchr(value,'/');
			if (value == NULL) break;
            }
         ++value;
         ++k;
         }
      }
}

static int parse_colors(char *value)
{
   int i,j,k;
   if (*value == '@') {
      if (merge_pathnames(MAP_name,&value[1],3)<0)
         init_msg("",&value[1],3);
      if ((int)strlen(value) > FILE_MAX_PATH || ValidateLuts(MAP_name) != 0)
         goto badcolor;
      if (display3d) {
        mapset = 1;
        }
      else {
        if (merge_pathnames(colorfile,&value[1],3)<0)
          init_msg("",&value[1],3);
        colorstate = 2;
        }
      }
   else {
      int smooth;
      i = smooth = 0;
      while (*value) {
         if (i >= 256) goto badcolor;
         if (*value == '<') {
            if (i == 0 || smooth
              || (smooth = atoi(value+1)) < 2
              || (value = strchr(value,'>')) == NULL)
               goto badcolor;
            i += smooth;
            ++value;
            }
         else {
            for (j = 0; j < 3; ++j) {
               if ((k = *(value++)) < '0')  goto badcolor;
               else if (k <= '9')       k -= '0';
               else if (k < 'A')            goto badcolor;
               else if (k <= 'Z')       k -= ('A'-10);
               else if (k < '_' || k > 'z') goto badcolor;
               else                     k -= ('_'-36);
               g_dac_box[i][j] = (BYTE)k;
               if (smooth) {
                  int start,spread,cnum;
                  start = i - (spread = smooth + 1);
                  cnum = 0;
                  if ((k - (int)g_dac_box[start][j]) == 0) {
                     while (++cnum < spread)
                        g_dac_box[start+cnum][j] = (BYTE)k;
                     }
                  else {
                     while (++cnum < spread)
                        g_dac_box[start+cnum][j] =
            (BYTE)(( cnum *g_dac_box[i][j]
            + (i-(start+cnum))*g_dac_box[start][j]
            + spread/2 )
            / (BYTE) spread);
                     }
                  }
               }
            smooth = 0;
            ++i;
            }
         }
      if (smooth) goto badcolor;
      while (i < 256)  { /* zap unset entries */
         g_dac_box[i][0] = g_dac_box[i][1] = g_dac_box[i][2] = 40;
         ++i;
         }
      colorstate = 1;
      }
   colorpreloaded = 1;
   memcpy(olddacbox,g_dac_box,256*3);
   return 0;
badcolor:
   return -1;
}

static void argerror(const char *badarg)      /* oops. couldn't decode this */
{
	char msg[300];
	char spillover[71];
	if ((int) strlen(badarg) > 70)
	{
		strncpy(spillover, badarg, 70);
		spillover[70] = 0;
		badarg = spillover;
	}
    sprintf(msg, "Oops. I couldn't understand the argument:\n  %s", badarg);

	if (first_init)       /* this is 1st call to cmdfiles */
	{
		strcat(msg, "\n"
			"\n"
			"(see the Startup Help screens or documentation for a complete\n"
			" argument list with descriptions)");
	}
	stopmsg(0, msg);
	if (initbatch)
	{
		initbatch = 4;
		goodbye();
	}
}

void set_3d_defaults()
{
   ROUGH     = 30;
   WATERLINE = 0;
   ZVIEWER   = 0;
   XSHIFT    = 0;
   YSHIFT    = 0;
   xtrans    = 0;
   ytrans    = 0;
   LIGHTAVG  = 0;
   Ambient   = 20;
   RANDOMIZE = 0;
   haze      = 0;
   back_color[0] = 51; back_color[1] = 153; back_color[2] = 200;
   if (SPHERE) {
      PHI1      =  180;
      PHI2      =  0;
      THETA1    =  -90;
      THETA2    =  90;
      RADIUS    =  100;
      FILLTYPE  = 2;
      XLIGHT    = 1;
      YLIGHT    = 1;
      ZLIGHT    = 1;
      }
   else {
      XROT      = 60;
      YROT      = 30;
      ZROT      = 0;
      XSCALE    = 90;
      YSCALE    = 90;
      FILLTYPE  = 0;
      XLIGHT    = 1;
      YLIGHT    = -1;
      ZLIGHT    = 1;
      }
}

/* copy a big number from a string, up to slash */
static int get_bf(bf_t bf, char *curarg)
{
   char *s;
   s=strchr(curarg,'/');
   if (s)
      *s = 0;
   strtobf(bf,curarg);
   if (s)
      *s = '/';
   return 0;
}

/* Get length of current args */
int get_curarg_len(char *curarg)
{
   int len;
   char *s;
   s=strchr(curarg,'/');
   if (s)
      *s = 0;
   len = (int) strlen(curarg);
   if (s)
      *s = '/';
   return len;
}

/* Get max length of current args */
int get_max_curarg_len(char *floatvalstr[], int totparms)
{
   int i,tmp,max_str;
   max_str = 0;
   for (i=0; i<totparms; i++)
      if ((tmp=get_curarg_len(floatvalstr[i])) > max_str)
         max_str = tmp;
   return max_str;
}

/* mode = 0 command line @filename         */
/*        1 sstools.ini                    */
/*        2 <@> command after startup      */
/*        3 command line @filename/setname */
/* this is like stopmsg() but can be used in cmdfiles()      */
/* call with NULL for badfilename to get pause for driver_get_key() */
int init_msg(char *cmdstr,char *badfilename,int mode)
{
   char *modestr[4] =
       {"command line", "sstools.ini", "PAR file", "PAR file"};
   char msg[256];
   char cmd[80];
   static int row = 1;

   if (initbatch == 1) { /* in batch mode */
      if (badfilename)
         /* uncomment next if wish to cause abort in batch mode for
            errors in CMDFILES.C such as parsing SSTOOLS.INI */
         /* initbatch = 4; */ /* used to set errorlevel */
      return -1;
   }
   strncpy(cmd,cmdstr,30);
   cmd[29] = 0;

   if (*cmd)
      strcat(cmd,"=");
   if (badfilename)
      sprintf(msg,"Can't find %s%s, please check %s",cmd,badfilename,modestr[mode]);
   if (first_init) {     /* & cmdfiles hasn't finished 1st try */
      if (row == 1 && badfilename) {
	     driver_set_for_text();
         driver_put_string(0,0,15, "Fractint found the following problems when parsing commands: ");
      }
      if (badfilename)
         driver_put_string(row++,0,7,msg);
      else if (row > 1){
         driver_put_string(++row,0,15, "Press Escape to abort, any other key to continue");
         driver_move_cursor(row+1,0);
         /*
         if (getakeynohelp()==27)
            goodbye();
         */
         dopause(2);  /* defer getakeynohelp until after parseing */
      }
   }
   else if (badfilename)
      stopmsg(0,msg);
   return 0;
}

/* defer pause until after parsing so we know if in batch mode */
void dopause(int action)
{
   static unsigned char needpause = 0;
   switch (action)
   {
   case 0:
      if (initbatch == 0)
      {
         if (needpause == 1)
            driver_get_key();
         else if (needpause == 2)
            if (getakeynohelp() == FIK_ESC)
               goodbye();
      }
      needpause = 0;
      break;
   case 1:
   case 2:
      needpause = (char)action;
      break;
   default:
      break;
   }
}

/* 
   Crude function to detect a floating point number. Intended for
   use with arbitrary precision.
*/
static int isabigfloat(char *str)
{
   /* [+|-]numbers][.]numbers[+|-][e|g]numbers */
   int result=1;
   char *s = str;
   int numdot=0;
   int nume=0;
   int numsign=0;
   while (*s != 0 && *s != '/' && *s != ' ')
   {
      if (*s == '-' || *s == '+') numsign++;
      else if (*s == '.') numdot++;
      else if (*s == 'e' || *s == 'E' || *s == 'g' || *s == 'G') nume++;
      else if (!isdigit(*s)) {result=0; break;}
      s++;
   }
   if (numdot > 1 || numsign > 2 || nume > 1) result=0;
   return result;
}

