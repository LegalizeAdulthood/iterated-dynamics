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
/*#ifdef __TURBOC__
#include <dir.h>
#endif  */

#ifdef XFRACT
#define DEFAULT_PRINTER 5       /* Assume a Postscript printer */
#define PRT_RESOLUTION  100     /* Assume medium resolution    */
#define INIT_GIF87      0       /* Turn on GIF 89a processing  */
#else
#define DEFAULT_PRINTER 2       /* Assume an IBM/Epson printer */
#define PRT_RESOLUTION  60      /* Assume low resolution       */
#define INIT_GIF87      0       /* Turn on GIF 89a processing  */
#endif

#define far_strncmp far_strnicmp

static int  cmdfile(FILE *,int);
static int  next_command(char *,int,FILE *,char *,int *,int);
static int  next_line(FILE *,char *,int);
int  cmdarg(char *,int);
static void argerror(char *);
static void initvars_run(void);
static void initvars_restart(void);
static void initvars_fractal(void);
static void initvars_3d(void);
static void reset_ifs_defn(void);
static void parse_textcolors(char *value);
static int  parse_colors(char *value);
static int  parse_printer(char *value);
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
char    gifmask[13] = {""};
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
int     initmode;               /* initial video mode       */
int     initcyclelimit;         /* initial cycle limit      */
BYTE    usemag;                 /* use center-mag corners   */
long    bailout;                /* user input bailout value */
enum bailouts bailoutest;       /* test used for determining bailout */
double  inversion[3];           /* radius, xcenter, ycenter */
int     rotate_lo,rotate_hi;    /* cycling color range      */
int far *ranges;                /* iter->color ranges mapping */
int     rangeslen = 0;          /* size of ranges array     */
BYTE far *mapdacbox = NULL;     /* map= (default colors)    */
int     colorstate;             /* 0, dacbox matches default (bios or map=) */
                                /* 1, dacbox matches no known defined map   */
                                /* 2, dacbox matches the colorfile map      */
int     colorpreloaded;         /* if dacbox preloaded for next mode select */
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
/* TARGA+ variables */
int     TPlusFlag;              /* Use the TARGA+ if found  */
int     MaxColorRes;            /* Default Color Resolution if available */
int     PixelZoom;              /* TPlus Zoom Level */
int     NonInterlaced;          /* Non-Interlaced video flag */

int     orbitsave = 0;          /* for IFS and LORENZ to output acrospin file */
int orbit_delay;                /* clock ticks delating orbit release */
int     transparent[2];         /* transparency min/max values */
long    LogFlag;                /* Logarithmic palette flag: 0 = no */

BYTE exitmode = 3;      /* video mode on exit */

char    ai_8514;        /* Flag for using 8514a afi JCO 4/11/92 */
int     Log_Fly_Calc = 0;   /* calculate logmap on-the-fly */
int     Log_Auto_Calc = 0;  /* auto calculate logmap */
int     nobof = 0; /* Flag to make inside=bof options not duplicate bof images */

int        bios_palette;        /* set to 1 to force BIOS palette updates */
int        escape_exit;         /* set to 1 to avoid the "are you sure?" screen */
int first_init=1;               /* first time into cmdfiles? */
static int init_rseed;
static char initcorners,initparams;
struct fractalspecificstuff far *curfractalspecific;

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
float far *ifs_defn = NULL;     /* ifs parameters */
int  ifs_type;                  /* 0=2d, 1=3d */
int  slides = 0;                /* 1 autokey=play, 2 autokey=record */

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

/* start of string literals cleanup */
char s_atan[]    = "atan";
char s_iter[]    = "iter";
char s_real[]    = "real";
char s_mult[]     = "mult";
char s_sum[]     = "summ";
char s_imag[]    = "imag";
char s_zmag[]    = "zmag";
char s_bof60[]   = "bof60";
char s_bof61[]   = "bof61";
char s_maxiter[] =  "maxiter";
char s_epscross[] =  "epsiloncross";
char s_startrail[] =  "startrail";
char s_normal[] =  "normal";
char s_period[] = "period";
char s_fmod[] = "fmod";
char s_tdis[] = "tdis";
char s_or[]     = "or";
char s_and[]    = "and";
char s_mod[]    = "mod";
char s_16bit[] =            "16bit";
char s_387[] =              "387";
char s_3d[] =               "3d";
char s_3dmode[] =           "3dmode";
char s_adapter[] =          "adapter";
char s_afi[] =              "afi";
char s_ambient[] =          "ambient";
char s_askvideo[] =         "askvideo";
char s_aspectdrift[] =      "aspectdrift";
char s_attack[] =           "attack";
char s_atten[] =            "attenuate";
char s_autokey[] =          "autokey";
char s_autokeyname[] =      "autokeyname";
char s_background[] =       "background";
char s_bailout[] =          "bailout";
char s_bailoutest[] =       "bailoutest";
char s_batch[] =            "batch";
char s_beep[] =             "beep";
char s_biomorph[] =         "biomorph";
char s_biospalette[] =      "biospalette";
char s_brief[] =            "brief";
char s_bright[] =           "bright";
char s_centermag[] =        "center-mag";
char s_cga[] =              "cga";
char s_coarse[] =           "coarse";
char s_colorps[] =          "colorps";
char s_colors[] =           "colors";
char s_comment[] =          "comment";
char s_comport[] =          "comport";
char s_converge[] =         "converge";
char s_corners[] =          "corners";
char s_cr[] =               "cr";
char s_crlf[] =             "crlf";
char s_crop[] =             "crop";
char s_cyclelimit[] =       "cyclelimit";
char s_cyclerange[] =       "cyclerange";
char s_curdir[] =           "curdir";
char s_debug[] =            "debug";
char s_debugflag[] =        "debugflag";
char s_decay[] =            "decay";
char s_decomp[] =           "decomp";
char s_distest[] =          "distest";
char s_dither[] =           "dither";
char s_ega[] =              "ega";
char s_egamono[] =          "egamono";
char s_epsf[] =             "epsf";
char s_exitmode[] =         "exitmode";
char s_exitnoask[] =        "exitnoask";
char s_fastrestore[] =      "fastrestore";
char s_filename[] =         "filename";
char s_fillcolor[] =        "fillcolor";
char s_filltype[] =         "filltype";
char s_finattract[] =       "finattract";
char s_float[] =            "float";
char s_formulafile[] =      "formulafile";
char s_formulaname[] =      "formulaname";
char s_fpu[] =              "fpu";
char s_fract001prn[] =     "fract001.prn";
char s_fullcolor[] =        "fullcolor";
char s_function[] =         "function";
char s_gif87a[] =           "gif87a";
char s_halftone[] =         "halftone";
char s_haze[] =             "haze";
char s_hertz[] =            "hertz";
char s_hgc[] =              "hgc";
char s_high[] =             "high";
char s_ifs[] =              "ifs";
char s_ifs3d[] =            "ifs3d";
char s_ifsfile[] =          "ifsfile";
char s_initorbit[] =        "initorbit";
char s_inside[] =           "inside";
char s_interocular[] =      "interocular";
char s_invert[] =           "invert";
char s_iterincr[] =         "iterincr";
char s_julibrot3d[] =       "julibrot3d";
char s_julibroteyes[] =     "julibroteyes";
char s_julibrotfromto[] =   "julibrotfromto";
char s_latitude[] =         "latitude";
char s_lf[] =               "lf";
char s_lfile[] =            "lfile";
char s_lightname[] =        "lightname";
char s_lightsource[] =      "lightsource";
char s_linefeed[] =         "linefeed";
char s_lname[] =            "lname";
char s_logmap[] =           "logmap";
char s_logmode[] =          "logmode";
char s_longitude[] =        "longitude";
char s_low[] =              "low";
char s_makedoc[] =          "makedoc";
char s_makemig[] =          "makemig";
char s_makepar[] =          "makepar";
char s_manh[]    = "manh";
char s_manr[]    = "manr";
char s_map[] =              "map";
char s_maxcolorres[] =      "maxcolorres";
char s_mcga[] =             "mcga";
char s_mid[] =              "mid";
char s_miim[] =             "miim";
char s_nobof[] =            "nobof";
char s_mono[] =             "mono";
char s_none[] =             "none";
char s_noninterlaced[] =    "noninterlaced";
char s_off[] =              "off";
char s_olddemmcolors[] =    "olddemmcolors";
char s_orbitcorners[] =     "orbitcorners";
char s_orbitdelay[] =       "orbitdelay";
char s_orbitdrawmode[] =    "orbitdrawmode";
char s_orbitinterval[] =    "orbitinterval";
char s_orbitname[] =        "orbitname";
char s_orbitsave[] =        "orbitsave";
char s_orgfrmdir[] =        "orgfrmdir";
char s_origin[] =           "origin";
char s_outside[] =          "outside";
char s_overlay[] =          "overlay";
char s_overwrite[] =        "overwrite";
char s_params[] =           "params";
char s_parmfile[] =         "parmfile";
char s_passes[] =           "passes";
char s_periodicity[] =      "periodicity";
char s_perspective[] =      "perspective";
char s_pi[] =               "pi";
char s_pixel[] =            "pixel";
char s_pixelzoom[] =        "pixelzoom";
char s_play[] =             "play";
char s_plotstyle[] =        "plotstyle";
char s_polyphony[] =        "polyphony";
char s_potential[] =        "potential";
char s_preview[] =          "preview";
char s_printer[] =          "printer";
char s_printfile[] =        "printfile";
char s_prox[] =             "proximity";
char s_radius[] =           "radius";
char s_ramvideo[] =         "ramvideo";
char s_randomize[] =        "randomize";
char s_ranges[] =           "ranges";
char s_ray[] =              "ray";
char s_record[] =           "record";
char s_release[] =          "release";
char s_srelease[] =         "srelease";
char s_reset[] =            "reset";
char s_rleps[] =            "rleps";
char s_rotation[] =         "rotation";
char s_roughness[] =        "roughness";
char s_rseed[] =            "rseed";
char s_savename[] =         "savename";
char s_savetime[] =         "savetime";
char s_scalemap[] =         "scalemap";
char s_scalexyz[] =         "scalexyz";
char s_screencoords[] =     "screencoords";
char s_showbox[] =          "showbox";
char s_showdot[] =          "showdot";
char s_showorbit[] =        "showorbit";
char s_smoothing[] =        "smoothing";
char s_sound[] =            "sound";
char s_sphere[] =           "sphere";
char s_stereo[] =           "stereo";
char s_sustain[] =          "sustain";
char s_symmetry[] =         "symmetry";
char s_truecolor[] =        "truecolor";
char s_truemode[] =         "truemode";
char s_tempdir[] =          "tempdir";
char s_workdir[] =          "workdir";
char s_usegrayscale[] =     "usegrayscale";
char s_monitorwidth[] =     "monitorwidth";
char s_targa_overlay[] =    "targa_overlay";
char s_textcolors[] =       "textcolors";
char s_textsafe[] =         "textsafe";
char s_title[] =            "title";
char s_tplus[] =            "tplus";
char s_translate[] =        "translate";
char s_transparent[] =      "transparent";
char s_type[] =             "type";
char s_vesadetect[] =       "vesadetect";
char s_vga[] =              "vga";
char s_video[] =            "video";
char s_viewwindows[] =      "viewwindows";
char s_virtual[] =          "virtual";
char s_volume[] =           "volume";
char s_warn[] =             "warn";
char s_waterline[] =        "waterline";
char s_wavetype[]=          "wavetype";
char s_xaxis[] =            "xaxis";
char s_xyadjust[] =         "xyadjust";
char s_xyaxis[] =           "xyaxis";
char s_xyshift[] =          "xyshift";
char s_yaxis [] =           "yaxis";
char s_sin [] =             "sin";
char s_sinh [] =            "sinh";
char s_cos [] =             "cos";
char s_cosh [] =            "cosh";
char s_sqr [] =             "sqr";
char s_log [] =             "log";
char s_exp [] =             "exp";
char s_abs [] =             "abs";
char s_conj [] =            "conj";
char s_fn1 [] =             "fn1";
char s_fn2 [] =             "fn2";
char s_fn3 [] =             "fn3";
char s_fn4 [] =             "fn4";
char s_flip [] =            "flip";
char s_floor [] =           "floor";
char s_ceil [] =            "ceil";
char s_trunc [] =           "trunc";
char s_round [] =           "round";
char s_tan [] =             "tan";
char s_tanh [] =            "tanh";
char s_cotan [] =           "cotan";
char s_cotanh [] =          "cotanh";
char s_cosxx [] =           "cosxx";
char s_srand [] =           "srand";
char s_recip [] =           "recip";
char s_ident [] =           "ident";
char s_zero [] =            "zero";
char s_one  [] =            "one";
char s_asin [] =            "asin";
char s_asinh [] =           "asinh";
char s_acos [] =            "acos";
char s_acosh [] =           "acosh";
char s_atanh [] =           "atanh";
char s_cabs [] =            "cabs";
char s_sqrt [] =            "sqrt";
char s_ismand [] =          "ismand";
char s_mathtolerance[] =    "mathtolerance";
static FCODE s_bfdigits []     = "bfdigits";
static FCODE s_recordcolors [] = "recordcolors";
static FCODE s_maxlinelength []= "maxlinelength";
static FCODE s_minstack[]      = "minstack";
static FCODE s_lzw []          = "tweaklzw";
static FCODE s_sstoolsini []   = "sstools.ini";
static FCODE s_fractintfrm []  = "fractint.frm";
static FCODE s_fractintl []    = "fractint.l";
static FCODE s_fractintpar []  = "fractint.par";
static FCODE s_fractintifs []  = "fractint.ifs";
static FCODE s_commandline []  = "command line";
static FCODE s_at_cmd []       = "PAR file";

int lzw[2];
static FCODE s_escapetoabort[] = "Press Escape to abort, any other key to continue";
char far s_pressanykeytocontinue[] = "press any key to continue";

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

   strcpy(curarg,s_sstoolsini);
   findpath(curarg, tempstring); /* look for SSTOOLS.INI */
   if (tempstring[0] != 0)              /* found it! */
      if ((initfile = fopen(tempstring,"r")) != NULL)
         cmdfile(initfile,1);           /* process it */

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
            cmdarg(curarg,0);           /* process simple command */
         }
      else if ((sptr = strchr(curarg,'/')) != NULL) { /* @filename/setname? */
         *sptr = 0;
         if(merge_pathnames(CommandFile, &curarg[1], 0) < 0)
            init_msg(0,"",CommandFile,0);
         strcpy(CommandName,sptr+1);
         if(find_file_item(CommandFile,CommandName,&initfile, 0)<0 || initfile==NULL)
            argerror(curarg);
         cmdfile(initfile,3);
         }
      else {                            /* @filename */
         if ((initfile = fopen(&curarg[1],"r")) == NULL)
            argerror(curarg);
         cmdfile(initfile,0);
         }
      }

   if (first_init == 0) {
      initmode = -1; /* don't set video when <ins> key used */
      showfile = 1;  /* nor startup image file              */
      }

   init_msg(0,"",NULL,0);  /* this causes getakey if init_msg called on runup */

   if(debugflag != 110)
       first_init = 0;
/*       {
            char msg[MSGLEN];
            sprintf(msg,"cmdfiles colorpreloaded %d showfile %d savedac %d",
                colorpreloaded, showfile, savedac);
            stopmsg(0,msg);
         }
*/
   if(colorpreloaded && showfile==0) /* PAR reads a file and sets color */
      dontreadcolor = 1;   /* don't read colors from GIF */
   else
      dontreadcolor = 0;   /* read colors from GIF */

     /*set structure of search directories*/
   strcpy(searchfor.par, CommandFile);
   strcpy(searchfor.frm, FormFileName);
   strcpy(searchfor.lsys, LFileName);
   strcpy(searchfor.ifs, IFSFileName);
   return(0);
}


int load_commands(FILE *infile)
{
   /* when called, file is open in binary mode, positioned at the */
   /* '(' or '{' following the desired parameter set's name       */
   int ret;
   initcorners = initparams = 0; /* reset flags for type= */
   ret = cmdfile(infile,2);
/*
         {
            char msg[MSGLEN];
            sprintf(msg,"load commands colorpreloaded %d showfile %d savedac %d",
                colorpreloaded, showfile, savedac);
            stopmsg(0,msg);
         }
*/

   if(colorpreloaded && showfile==0) /* PAR reads a file and sets color */
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
   if((p = getenv("TMP")) == NULL)
      p = getenv("TEMP");
   if(p != NULL)
   {
      if(isadirectory(p) != 0)
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
   save_release = release;              /* this release number */
   gif87a_flag = INIT_GIF87;            /* turn on GIF89a processing */
   dither_flag = 0;                     /* no dithering */
   askvideo = 1;                        /* turn on video-prompt flag */
   fract_overwrite = 0;                 /* don't overwrite           */
   soundflag = 9;                       /* sound is on to PC speaker */
   initbatch = 0;                       /* not in batch mode         */
   checkcurdir = 0;                     /* flag to check current dire for files */
   initsavetime = 0;                    /* no auto-save              */
   initmode = -1;                       /* no initial video mode     */
   viewwindow = 0;                      /* no view window            */
   viewreduction = (float)4.2;
   viewcrop = 1;
   virtual_screens = 1;                 /* virtual screen modes on   */
   ai_8514 = 0;                         /* no need for the 8514 API  */
   finalaspectratio = screenaspect;
   viewxdots = viewydots = 0;
   video_cutboth = 1;                   /* keep virtual aspect */
   zscroll = 1;                         /* relaxed screen scrolling */
   orbit_delay = 0;                     /* full speed orbits */
   orbit_interval = 1;                  /* plot all orbits */
   debugflag = 0;                       /* debugging flag(s) are off */
   timerflag = 0;                       /* timer flags are off       */
   strcpy(FormFileName,s_fractintfrm); /* default formula file      */
   FormName[0] = 0;
   strcpy(LFileName,s_fractintl);
   LName[0] = 0;
   strcpy(CommandFile,s_fractintpar);
   CommandName[0] = 0;
   for(i=0;i<4; i++)
      CommandComment[i][0] = 0;
   strcpy(IFSFileName,s_fractintifs);
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
      farmemfree(mapdacbox);
      mapdacbox = NULL;
      }
   TPlusFlag = 1;
   MaxColorRes = 8;
   PixelZoom = 0;
   NonInterlaced = 0;

   Printer_Type = DEFAULT_PRINTER;      /* assume an IBM/EPSON    */
   Printer_Resolution = PRT_RESOLUTION; /* assume low resolution  */
   Printer_Titleblock = 0;              /* assume no title block  */
   Printer_ColorXlat = 0;               /* assume positive image  */
   Printer_SetScreen = 0;               /* assume default screen  */
   Printer_SFrequency = 45;             /* New screen frequency K */
   Printer_SAngle = 45;                 /* New screen angle     K */
   Printer_SStyle = 1;                  /* New screen style     K */
   Printer_RFrequency = 45;             /* New screen frequency R */
   Printer_RAngle = 75;                 /* New screen angle     R */
   Printer_RStyle = 1;                  /* New screen style     R */
   Printer_GFrequency = 45;             /* New screen frequency G */
   Printer_GAngle = 15;                 /* New screen angle     G */
   Printer_GStyle = 1;                  /* New screen style     G */
   Printer_BFrequency = 45;             /* New screen frequency B */
   Printer_BAngle = 0;                  /* New screen angle     B */
   Printer_BStyle = 1;                  /* New screen style     B */
#ifndef XFRACT
   Print_To_File = 0;                   /* No print-to-file       */
   Printer_CRLF = 0;                    /* Assume CR+LF           */
#else
   Print_To_File = 1;                   /* Print-to-file          */
   Printer_CRLF = 2;                    /* Assume LF              */
   Printer_Compress = 0;                /* Assume NO PostScript compression */
#endif
   EPSFileType = 0;                     /* Assume no save to .EPS */
   LPTNumber = 1;                       /* assume LPT1 */
   ColorPS = 0;                         /* Assume NO Color PostScr*/
   major_method = breadth_first;        /* default inverse julia methods */
   minor_method = left_first;   /* default inverse julia methods */
   truecolor = 0;              /* truecolor output flag */
   truemode = 0;               /* set to default color scheme */
}

static void initvars_fractal()          /* init vars affecting calculation */
{
   int i;
   bios_palette = 0;                    /* don't force use of a BIOS palette */
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
   set_trig_array(0,s_sin);             /* trigfn defaults */
   set_trig_array(1,s_sqr);
   set_trig_array(2,s_sinh);
   set_trig_array(3,s_cosh);
   if (rangeslen) {
      farmemfree((char far *)ranges);
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
   floatbailout  = (int (near *)(void))fpMODbailout;
   longbailout   = (int (near *)(void))asmlMODbailout;
   bignumbailout = (int (near *)(void))bnMODbailout;
   bigfltbailout = (int (near *)(void))bfMODbailout;

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
   for(i=0;i<=11;i++) scale_map[i]=i+1;    /* straight mapping of notes in octave */
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
   eyeseparation = 0;
   glassestype = 0;
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
      farmemfree((char far *)ifs_defn);
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
   char linebuf[513],*cmdbuf;
   char far *savesuffix;
   /* use near array suffix for large argument buffer, but save existing
      contents to extraseg */
   cmdbuf = (char *)suffix;
   savesuffix = MK_FP(extraseg,0);
   far_memcpy(savesuffix,suffix,10000);
   far_memset(suffix,0,10000);

   if (mode == 2 || mode == 3) {
      while ((i = getc(handle)) != '{' && i != EOF) { }
      for(i=0;i<4; i++)
          CommandComment[i][0] = 0;
      }
   linebuf[0] = 0;
   while (next_command(cmdbuf,10000,handle,linebuf,&lineoffset,mode) > 0) {
      if ((mode == 2 || mode == 3) && strcmp(cmdbuf,"}") == 0) break;
      if ((i = cmdarg(cmdbuf,mode)) < 0) break;
      changeflag |= i;
      }
   fclose(handle);
#ifdef XFRACT
   initmode = 0;                /* Skip credits if @file is used. */
#endif
   far_memcpy(suffix,savesuffix,10000);
   if(changeflag&1)
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
   for(;;) {
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
              && (mode == 2 || mode == 3)
              && (CommandComment[0][0] == 0 || CommandComment[1][0] == 0 ||
                  CommandComment[2][0] == 0 || CommandComment[3][0] == 0)) {
               /* save comment */
               while (*(++lineptr)
                 && (*lineptr == ' ' || *lineptr == '\t')) { }
               if (*lineptr) {
                  if ((int)strlen(lineptr) >= MAXCMT)
                     *(lineptr+MAXCMT-1) = 0;
                  for(i=0;i<4; i++)
                     if (CommandComment[i][0] == 0)
                     {
                        strcpy(CommandComment[i],lineptr);
                        break;   
                     }
                  }
               }
            if (next_line(handle,linebuf,mode) != 0)
               return(-1); /* eof */
            lineptr = linebuf; /* start new line */
            }
         }
      if (*lineptr == '\\'              /* continuation onto next line? */
        && *(lineptr+1) == 0) {
         if (next_line(handle,linebuf,mode) != 0) {
            argerror(cmdbuf);           /* missing continuation */
            return(-1);
            }
         lineptr = linebuf;
         while (*lineptr && *lineptr <= ' ')
            ++lineptr;                  /* skip white space @ start next line */
         continue;                      /* loop to check end of line again */
         }
      cmdbuf[cmdlen] = *(lineptr++);    /* copy character to command buffer */
      if (++cmdlen >= maxlen) {         /* command too long? */
         argerror(cmdbuf);
         return(-1);
         }
      }
}

static int next_line(FILE *handle,char *linebuf,int mode)
{
   int toolssection;
   char tmpbuf[11];
   toolssection = 0;
   while (file_gets(linebuf,512,handle) >= 0) {
      if (mode == 1 && linebuf[0] == '[') {     /* check for [fractint] */
#ifndef XFRACT
         strncpy(tmpbuf,&linebuf[1],9);
         tmpbuf[9] = 0;
         strlwr(tmpbuf);
         toolssection = far_strncmp(tmpbuf,"fractint]",9);
#else
         strncpy(tmpbuf,&linebuf[1],10);
         tmpbuf[10] = 0;
         strlwr(tmpbuf);
         toolssection = far_strncmp(tmpbuf,"xfractint]",10);
#endif
         continue;                              /* skip tools section heading */
         }
      if (toolssection == 0) return(0);
      }
   return(-1);
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

/* following gets rid of "too big for optimization" warning */
#ifdef _MSC_VER
#if (_MSC_VER >= 600)
#pragma optimize( "el", off )
#endif
#endif

int cmdarg(char *curarg,int mode) /* process a single argument */
{
   char    variable[21];                /* variable name goes here   */
   char    *value;                      /* pointer to variable value */
   int     valuelen;                    /* length of value           */
   int     numval;                      /* numeric value of arg      */
#define NONNUMERIC -32767
   char    charval[16];                 /* first character of arg    */
   int     yesnoval[16];                /* 0 if 'n', 1 if 'y', -1 if not */
   double  ftemp;
   int     i, j, k, l;
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
#ifndef XFRACT
   while (*argptr) {                    /* convert to lower case */
      if (*argptr >= 'A' && *argptr <= 'Z')
         *argptr += 'a' - 'A';
      if (*argptr == '=' && far_strncmp(curarg,"colors=",7) == 0)
         break;                         /* don't convert colors=value */
      if (*argptr == '=' && far_strncmp(curarg,s_comment,7) == 0)
         break;                         /* don't convert comment=value */
      ++argptr;
      }
#endif

   if ((value = strchr(&curarg[1],'=')) != NULL) {
      if ((j = (int) ((value++) - curarg)) > 1 && curarg[j-1] == ':')
         --j;                           /* treat := same as =     */
      }
   else
      value = curarg + (j = (int) strlen(curarg));
   if (j > 20) goto badarg;             /* keyword too long */
   strncpy(variable,curarg,j);          /* get the variable name  */
   variable[j] = 0;                     /* truncate variable name */
   valuelen = (int) strlen(value);            /* note value's length    */
   charval[0] = value[0];               /* first letter of value  */
   yesnoval[0] = -1;                    /* note yes|no value      */
   if (charval[0] == 'n') yesnoval[0] = 0;
   if (charval[0] == 'y') yesnoval[0] = 1;

   argptr = value;
   numval = totparms = intparms = floatparms = 0;
   while (*argptr) {                    /* count and pre-parse parms */
      long ll;
      lastarg = 0;
      if ((argptr2 = strchr(argptr,'/')) == NULL) {     /* find next '/' */
         argptr2 = argptr + strlen(argptr);
         *argptr2 = '/';
         lastarg = 1;
         }
      if (totparms == 0) numval = NONNUMERIC;
      i = -1;
      if(totparms < 16)
      {
         charval[totparms] = *argptr;                      /* first letter of value  */
         if (charval[totparms] == 'n') yesnoval[totparms] = 0;
         if (charval[totparms] == 'y') yesnoval[totparms] = 1;
      }
      j=0;
      if (sscanf(argptr,"%c%c",(char *)&j,&tmpc) > 0    /* NULL entry */
      && ((char)j == '/' || (char)j == '=') && tmpc == '/') {
         j = 0;
         ++floatparms; ++intparms;
         if (totparms < 16) {floatval[totparms] = j; floatvalstr[totparms]="0";}
         if (totparms < 64) intval[totparms] = j;
         if (totparms == 0) numval = j;
         }
      else if (sscanf(argptr,"%ld%c",&ll,&tmpc) > 0       /* got an integer */
        && tmpc == '/') {        /* needs a long int, ll, here for lyapunov */
         ++floatparms; ++intparms;
         if (totparms < 16) {floatval[totparms] = ll; floatvalstr[totparms]=argptr;}
         if (totparms < 64) intval[totparms] = (int)ll;
         if (totparms == 0) numval = (int)ll;
         }
#ifndef XFRACT
      else if (sscanf(argptr,"%lg%c",&ftemp,&tmpc) > 0  /* got a float */
#else
      else if (sscanf(argptr,"%lf%c",&ftemp,&tmpc) > 0  /* got a float */
#endif
             && tmpc == '/') {
         ++floatparms;
         if (totparms < 16) {floatval[totparms] = ftemp;floatvalstr[totparms]=argptr;}
         }
      /* using arbitrary precision and above failed */
      else if (((int)strlen(argptr) > 513)  /* very long command */
                 || (totparms > 0 && floatval[totparms-1] == FLT_MAX
                     && totparms < 6)
                 || isabigfloat(argptr)) {
         ++floatparms;
         floatval[totparms] = FLT_MAX;
         floatvalstr[totparms]=argptr;
      }
      ++totparms;
      argptr = argptr2;                                 /* on to the next */
      if (lastarg)
         *argptr = 0;
      else
         ++argptr;
      }

   if (mode != 2 || debugflag==110) {
      /* these commands are allowed only at startup */

      if (strcmp(variable,s_batch) == 0 ) {     /* batch=?      */
         if (yesnoval[0] < 0) goto badarg;
#ifdef XFRACT
         initmode = yesnoval[0]?0:-1; /* skip credits for batch mode */
#endif
         initbatch = yesnoval[0];
         return 3;
         }
   if (strcmp(variable,"maxhistory") == 0) {       /* maxhistory=? */
      if(numval == NONNUMERIC)
         goto badarg;
      else if(numval < 0 /* || numval > 1000 */) goto badarg;
      else maxhistory = numval;
      return 3;
      }

#ifndef XFRACT
      if (strcmp(variable,s_adapter) == 0 ) {   /* adapter==?     */
         int i, j;
         char adapter_name[8];          /* entry lenth from VIDEO.ASM */
         char *adapter_ptr;

         adapter_ptr = &supervga_list;

         for(i = 0 ; ; i++) {           /* find the SuperVGA entry */
             memcpy(adapter_name , adapter_ptr, 8);
             adapter_name[6] = ' ';
             for (j = 0; j < 8; j++)
                 if(adapter_name[j] == ' ')
                     adapter_name[j] = 0;
             if (adapter_name[0] == 0) break;  /* end-of-the-list */
             if (far_strncmp(value,adapter_name,(int) strlen(adapter_name)) == 0) {
                svga_type = i+1;
                adapter_ptr[6] = 1;
                break;
                }
             adapter_ptr += 8;
             }
         if (svga_type != 0) return 3;

         video_type = 5;                        /* assume video=vga */
         if (strcmp(value,s_egamono) == 0) {
            video_type = 3;
            mode7text = 1;
            }
         else if (strcmp(value,s_hgc) == 0) {   /* video = hgc */
            video_type = 1;
            mode7text = 1;
            }
         else if (strcmp(value,s_ega) == 0)     /* video = ega */
            video_type = 3;
         else if (strcmp(value,s_cga) == 0)     /* video = cga */
            video_type = 2;
         else if (strcmp(value,s_mcga) == 0)    /* video = mcga */
            video_type = 4;
         else if (strcmp(value,s_vga) == 0)     /* video = vga */
            video_type = 5;
         else
            goto badarg;
         return 3;
         }

      if (strcmp(variable,s_afi) == 0) {
       if (far_strncmp(value,"8514"  ,4) == 0
           || charval[0] == 'y') ai_8514 = 1; /* set afi flag JCO 4/11/92 */
       return 3;
        }

      if (strcmp(variable,s_textsafe) == 0 ) {  /* textsafe==? */
         if (first_init) {
            if (charval[0] == 'n') /* no */
               textsafe = 2;
            else if (charval[0] == 'y') /* yes */
               textsafe = 1;
            else if (charval[0] == 'b') /* bios */
               textsafe = 3;
            else if (charval[0] == 's') /* save */
               textsafe = 4;
            else
               goto badarg;
            }
         return 3;
         }

      if (strcmp(variable,s_vesadetect) == 0) {
         if (yesnoval[0] < 0) goto badarg;
         vesa_detect = yesnoval[0];
         return 3;
         }

      if (strcmp(variable,s_biospalette) == 0) {
         if (yesnoval[0] < 0) goto badarg;
         bios_palette = yesnoval[0];
         return 3;
         }
#endif

      if (strcmp(variable,s_fpu) == 0) {
         if (strcmp(value,s_387) == 0) {
#ifndef XFRACT
            fpu = 387;
#else
            fpu = -1;
#endif
            return 0;
            }
         goto badarg;
         }

      if (strcmp(variable,s_exitnoask) == 0) {
         if (yesnoval[0] < 0) goto badarg;
         escape_exit = yesnoval[0];
         return 3;
         }

      if (strcmp(variable,s_makedoc) == 0) {
         print_document(*value ? value : "fractint.doc", makedoc_msg_func, 0);
#ifndef WINFRACT
         goodbye();
#endif
         }
      if (strcmp(variable,s_makepar) == 0) {
         char *slash, *next=NULL;
         if(totparms < 1 || totparms > 2)
            goto badarg;
         if((slash = strchr(value,'/')) != NULL)
         {
            *slash = 0;
            next = slash+1;
         }

         strcpy(CommandFile,value);
         if(strchr(CommandFile,'.') == NULL)
            strcat(CommandFile,".par");
         if(strcmp(readname,DOTSLASH)==0)
            *readname = 0;
         if(next == NULL)
         {
            if(*readname != 0)
               extract_filename(CommandName,readname);
            else if(*MAP_name != 0)   
               extract_filename(CommandName,MAP_name);
            else
               goto badarg;
         }   
         else
         {
            strncpy(CommandName,next,ITEMNAMELEN);
            CommandName[ITEMNAMELEN] = 0;
         }
         *s_makepar = 0; /* used as a flag for makepar case */
         if(*readname != 0)
         {
            if(read_overlay() != 0)
               goodbye();
         }
         else if(*MAP_name != 0)
         {
            s_makepar[1] = 0; /* second char is flag for map */
         }
         xdots = filexdots;
         ydots = fileydots;
         dxsize = xdots-1;
         dysize = ydots-1;
         calcfracinit();
         make_batch_file();
#ifndef WINFRACT
#ifndef XFRACT
         if(*readname != 0)
            printf("copying fractal info in GIF %s to PAR %s/%s\n",
                   readname,CommandFile,CommandName);
         else if (*MAP_name != 0)
            printf("copying color info in map %s to PAR %s/%s\n",
                MAP_name,CommandFile,CommandName);
#endif
         goodbye();
#endif
         }

      } /* end of commands allowed only at startup */

   if (strcmp(variable,s_reset) == 0) {
      initvars_fractal();

      /* PAR release unknown unless specified */
      if (numval>=0) save_release = numval;
      else goto badarg;
      if (save_release == 0)
         save_release = 1730; /* before start of lyapunov wierdness */
      return 9;
      }

   if (strcmp(variable,s_filename) == 0) {      /* filename=?     */
      int existdir;
      if (charval[0] == '.' && value[1] != SLASHC) {
         if (valuelen > 4) goto badarg;
         gifmask[0] = '*';
         gifmask[1] = 0;
         strcat(gifmask,value);
         return 0;
         }
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if (mode == 2 && display3d == 0) /* can't do this in @ command */
         goto badarg;

      if((existdir=merge_pathnames(readname, value, mode))==0)
         showfile = 0;
      else if(existdir < 0)
         init_msg(0,variable,value,mode);
      else
         extract_filename(browsename,readname);
      return 3;
      }

   if (strcmp(variable,s_video) == 0) {         /* video=? */
      if (active_system == 0) {
         if ((k = check_vidmode_keyname(value)) == 0) goto badarg;
         initmode = -1;
         for (i = 0; i < MAXVIDEOTABLE; ++i) {
            if (videotable[i].keynum == k) {
               initmode = i;
               break;
               }
            }
         if (initmode == -1) goto badarg;
         }
      return 3;
      }

   if (strcmp(variable,s_map) == 0 ) {         /* map=, set default colors */
      int existdir;
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if((existdir=merge_pathnames(MAP_name,value,mode))>0)
         return 0;    /* got a directory */
      else if (existdir < 0) {
         init_msg(0,variable,value,mode);
         return (0);
      }
      SetColorPaletteName(MAP_name);
      return 0;
      }

   if (strcmp(variable,s_colors) == 0) {       /* colors=, set current colors */
      if (parse_colors(value) < 0) goto badarg;
      return 0;
      }

   if (strcmp(variable,s_recordcolors) == 0) {       /* recordcolors= */
      if(*value != 'y' && *value != 'c' && *value != 'a')
         goto badarg;
      recordcolors = *value;
      return 0;
      }

   if (strcmp(variable,s_maxlinelength) == 0) {  /* maxlinelength= */
      if(numval < MINMAXLINELENGTH || numval > MAXMAXLINELENGTH)
         goto badarg;
      maxlinelength = numval;
      return 0;
      }

   if (strcmp(variable,s_comment) == 0) {       /* comment= */
      parse_comments(value);
      return 0;
      }

   if (strcmp(variable,s_tplus) == 0) {       /* Use the TARGA+ if found? */
      if (yesnoval[0] < 0) goto badarg;
      TPlusFlag = yesnoval[0];
      return 0;
      }

   if (strcmp(variable,s_noninterlaced) == 0) {
      if (yesnoval[0] < 0) goto badarg;
      NonInterlaced = yesnoval[0];
      return 0;
      }

   if (strcmp(variable,s_maxcolorres) == 0) { /* Change default color resolution */
      if (numval == 1 || numval == 4 || numval == 8 ||
                        numval == 16 || numval == 24) {
         MaxColorRes = numval;
         return 0;
         }
      goto badarg;
      }

   if (strcmp(variable,s_pixelzoom) == 0) {
      if (numval < 5)
         PixelZoom = numval;
      return 0;
      }

   /* keep this for backward compatibility */
   if (strcmp(variable,s_warn) == 0 ) {         /* warn=? */
      if (yesnoval[0] < 0) goto badarg;
      fract_overwrite = (char)(yesnoval[0] ^ 1);
      return 0;
      }
   if (strcmp(variable,s_overwrite) == 0 ) {    /* overwrite=? */
      if (yesnoval[0] < 0) goto badarg;
      fract_overwrite = (char)yesnoval[0];
      return 0;
      }

   if (strcmp(variable,s_gif87a) == 0 ) {       /* gif87a=? */
      if (yesnoval[0] < 0) goto badarg;
      gif87a_flag = yesnoval[0];
      return 0;
      }

   if (strcmp(variable,s_dither) == 0 ) {       /* dither=? */
      if (yesnoval[0] < 0) goto badarg;
      dither_flag = yesnoval[0];
      return 0;
      }

   if (strcmp(variable,s_savetime) == 0) {      /* savetime=? */
      initsavetime = numval;
      return 0;
      }

   if (strcmp(variable,s_autokey) == 0) {       /* autokey=? */
      if (strcmp(value,s_record)==0)
         slides=2;
      else if (strcmp(value,s_play)==0)
         slides=1;
      else
         goto badarg;
      return 0;
      }

   if (strcmp(variable,s_autokeyname) == 0) {   /* autokeyname=? */
      if(merge_pathnames(autoname, value,mode) < 0)
         init_msg(0,variable,value,mode);
      return 0;
      }

   if (strcmp(variable,s_type) == 0 ) {         /* type=? */
      if (value[valuelen-1] == '*')
         value[--valuelen] = 0;
      /* kludge because type ifs3d has an asterisk in front */
      if(strcmp(value,s_ifs3d)==0)
         value[3]=0;
      for (k = 0; fractalspecific[k].name != NULL; k++)
         if (strcmp(value,fractalspecific[k].name) == 0)
            break;
      if (fractalspecific[k].name == NULL) goto badarg;
      curfractalspecific = &fractalspecific[fractype = k];
      if (initcorners == 0) {
         xx3rd = xxmin = curfractalspecific->xmin;
         xxmax         = curfractalspecific->xmax;
         yy3rd = yymin = curfractalspecific->ymin;
         yymax         = curfractalspecific->ymax;
      }
      if (initparams == 0)
         load_params(fractype);
      return 1;
      }
   if (strcmp(variable,s_inside) == 0 ) {       /* inside=? */
      if(strcmp(value,s_zmag)==0)
         inside = ZMAG;
      else if(strcmp(value,s_bof60)==0)
         inside = BOF60;
      else if(strcmp(value,s_bof61)==0)
         inside = BOF61;
      else if(far_strncmp(value,s_epscross,3)==0)
         inside = EPSCROSS;
      else if(far_strncmp(value,s_startrail,4)==0)
         inside = STARTRAIL;
      else if(far_strncmp(value,s_period,3)==0)
         inside = PERIOD;
      else if(far_strncmp(value,s_fmod,3)==0)
         inside = FMODI;
      else if(far_strncmp(value,s_atan,3)==0)
         inside = ATANI;
      else if(strcmp(value,s_maxiter)==0)
         inside = -1;
      else if(numval == NONNUMERIC)
         goto badarg;
      else
         inside = numval;
      return 1;
      }
   if (strcmp(variable,s_prox) == 0 ) {       /* proximity=? */
      closeprox = floatval[0];
      return 1;
      }
   if (strcmp(variable,s_fillcolor) == 0 ) {       /* fillcolor */
      if(strcmp(value,s_normal)==0)
         fillcolor = -1;
      else if(numval == NONNUMERIC)
         goto badarg;
      else
         fillcolor = numval;
      return 1;
      }

   if (strcmp(variable,s_finattract) == 0 ) {   /* finattract=? */
      if (yesnoval[0] < 0) goto badarg;
      finattract = yesnoval[0];
      return 1;
      }

   if (strcmp(variable,s_nobof) == 0 ) {   /* nobof=? */
      if (yesnoval[0] < 0) goto badarg;
      nobof = yesnoval[0];
      return 1;
      }

   if (strcmp(variable,s_function) == 0) {      /* function=?,? */
      k = 0;
      while (*value && k < 4) {
         if(set_trig_array(k++,value)) goto badarg;
         if ((value = strchr(value,'/')) == NULL) break;
         ++value;
         }
       functionpreloaded = 1; /* for old bifs  JCO 7/5/92 */
      return 1;
      }

   if (strcmp(variable,s_outside) == 0 ) {      /* outside=? */
      if(strcmp(value,s_iter)==0)
         outside = ITER;
      else if(strcmp(value,s_real)==0)
         outside = REAL;
      else if(strcmp(value,s_imag)==0)
         outside = IMAG;
      else if(strcmp(value,s_mult)==0)
         outside = MULT;
      else if(strcmp(value,s_sum)==0)
         outside = SUM;
      else if(strcmp(value,s_atan)==0)
         outside = ATAN;
      else if(strcmp(value,s_fmod)==0)
         outside = FMOD;
      else if(strcmp(value,s_tdis)==0)
         outside = TDIS;

      else if(numval == NONNUMERIC)
         goto badarg;
      else if(numval < TDIS || numval > 255) goto badarg;
      else outside = numval;
      return 1;
      }

   if (strcmp(variable,s_bfdigits) == 0 ) {      /* bfdigits=? */
      if(numval == NONNUMERIC)
         goto badarg;
      else if(numval < 0 || numval > 2000) goto badarg;
      else bfdigits = numval;
      return 1;
      }

   if (strcmp(variable,s_maxiter) == 0) {       /* maxiter=? */
      if (floatval[0] < 2) goto badarg;
      maxit = (long)floatval[0];
      return 1;
      }

   if (strcmp(variable,s_iterincr) == 0)        /* iterincr=? */
      return 0;

   if (strcmp(variable,s_passes) == 0) {        /* passes=? */
      if ( charval[0] != '1' && charval[0] != '2' && charval[0] != '3'
        && charval[0] != 'g' && charval[0] != 'b'
	&& charval[0] != 't' && charval[0] != 's'
	&& charval[0] != 'd' && charval[0] != 'o')
         goto badarg;
      usr_stdcalcmode = charval[0];
      if(charval[0] == 'g')
      {
         stoppass = ((int)value[1] - (int)'0');
         if(stoppass < 0 || stoppass > 6)
            stoppass = 0;
      }
      return 1;
      }

   if (strcmp(variable,s_ismand) == 0 ) {        /* ismand=? */
      if (yesnoval[0] < 0) goto badarg;
      ismand = (short int)yesnoval[0];
      return 1;
      }

   if (strcmp(variable,s_cyclelimit) == 0 ) {   /* cyclelimit=? */
      if (numval <= 1 || numval > 256) goto badarg;
      initcyclelimit = numval;
      return 0;
      }

   if (strcmp(variable,s_makemig) == 0) {
       int xmult, ymult;
       if (totparms < 2) goto badarg;
       xmult = intval[0];
       ymult = intval[1];
       make_mig(xmult, ymult);
#ifndef WINFRACT
       exit(0);
#endif
       }

   if (strcmp(variable,s_cyclerange) == 0) {
      if (totparms < 2) intval[1] = 255;
      if (totparms < 1) intval[0] = 1;
      if (totparms != intparms
        || intval[0] < 0 || intval[1] > 255 || intval[0] > intval[1])
         goto badarg;
      rotate_lo = intval[0];
      rotate_hi = intval[1];
      return 0;
      }

   if (strcmp(variable,s_ranges) == 0) {
      int i,j,entries,prev;
      int tmpranges[128];
      if (totparms != intparms) goto badarg;
      entries = prev = i = 0;
      LogFlag = 0; /* ranges overrides logmap */
      while (i < totparms) {
         if ((j = intval[i++]) < 0) { /* striping */
            if ((j = 0-j) < 1 || j >= 16384 || i >= totparms) goto badarg;
            tmpranges[entries++] = -1; /* {-1,width,limit} for striping */
            tmpranges[entries++] = j;
            j = intval[i++];
            }
         if (j < prev) goto badarg;
         tmpranges[entries++] = prev = j;
         }
      if (prev == 0) goto badarg;
      if ((ranges = (int far *)farmemalloc(sizeof(int)*entries)) == NULL) {
         static FCODE msg[] = {"Insufficient memory for ranges="};
         stopmsg(1,msg);
         return(-1);
         }
      rangeslen = entries;
      for (i = 0; i < rangeslen; ++i)
         ranges[i] = tmpranges[i];
      return 1;
      }

   if (strcmp(variable,s_savename) == 0) {      /* savename=? */
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if (first_init || mode == 2) {
         if(merge_pathnames(savename, value, mode) < 0)
            init_msg(0,variable,value,mode);
      }
      return 0;
      }

   if (strcmp(variable,s_lzw) == 0) {      /* tweaklzw=? */
      if (totparms >= 1) lzw[0] = intval[0];
      if (totparms >= 2) lzw[1] = intval[1];
      return 0;
      }

   if (strcmp(variable,s_minstack) == 0) {      /* minstack=? */
      if (totparms != 1)
         goto badarg;
      minstack = intval[0];
      return 0;
      }

   if (strcmp(variable,s_mathtolerance) == 0) {      /* mathtolerance=? */
      if(charval[0] == '/')
          ; /* leave math_tol[0] at the default value */
      else if (totparms >= 1) math_tol[0] = floatval[0];
      if (totparms >= 2) math_tol[1] = floatval[1];
      return 0;
      }

   if (strcmp(variable,s_tempdir) == 0) {      /* tempdir=? */
      if (valuelen > (FILE_MAX_DIR-1)) goto badarg;
      if(isadirectory(value) == 0) goto badarg;
      strcpy(tempdir,value);
      fix_dirname(tempdir);
      return 0;
      }

   if (strcmp(variable,s_workdir) == 0) {      /* workdir=? */
      if (valuelen > (FILE_MAX_DIR-1)) goto badarg;
      if(isadirectory(value) == 0) goto badarg;
      strcpy(workdir,value);
      fix_dirname(workdir);
      return 0;
      }

   if (strcmp(variable,s_exitmode) == 0) {      /* exitmode=? */
      sscanf(value,"%x",&numval);
      exitmode = (BYTE)numval;
      return 0;
      }

   if (strcmp(variable,s_textcolors) == 0) {
      parse_textcolors(value);
      return 0;
      }

   if (strcmp(variable,s_potential) == 0) {     /* potential=? */
      k = 0;
      while (k < 3 && *value) {
         if(k==1)
            potparam[k] = atof(value);
         else
            potparam[k] = atoi(value);
         k++;
       if ((value = strchr(value,'/')) == NULL) k = 99;
         ++value;
         }
      pot16bit = 0;
      if (k < 99) {
         if (strcmp(value,s_16bit)) goto badarg;
         pot16bit = 1;
         }
      return 1;
      }

   if (strcmp(variable,s_params) == 0) {        /* params=?,? */
      if (totparms != floatparms || totparms > MAXPARAMS)
         goto badarg;
      initparams = 1;
      for (k = 0; k < MAXPARAMS; ++k)
         param[k] = (k < totparms) ? floatval[k] : 0.0;
      if(bf_math)
         for (k = 0; k < MAXPARAMS; k++)
            floattobf(bfparms[k],param[k]);
      return 1;
      }

   if (strcmp(variable,s_miim) == 0) {          /* miim=?[/?[/?[/?]]] */
      if (totparms > 6) goto badarg;
      if (charval[0] == 'b')
         major_method = breadth_first;
      else if (charval[0] == 'd')
         major_method = depth_first;
      else if (charval[0] == 'w')
         major_method = random_walk;
#ifdef RANDOM_RUN
      else if (charval[0] == 'r')
         major_method = random_run;
#endif
      else goto badarg;

      if (charval[1] == 'l')
         minor_method = left_first;
      else if (charval[1] == 'r')
         minor_method = right_first;
      else goto badarg;

/* keep this next part in for backwards compatibility with old PARs ??? */

      if (totparms > 2)
        for (k = 2; k < 6; ++k)
           param[k-2] = (k < totparms) ? floatval[k] : 0.0;

      return 1;
   }

   if (strcmp(variable,s_initorbit) == 0) {     /* initorbit=?,? */
      if(strcmp(value,s_pixel)==0)
         useinitorbit = 2;
      else {
         if (totparms != 2 || floatparms != 2) goto badarg;
         initorbit.x = floatval[0];
         initorbit.y = floatval[1];
         useinitorbit = 1;
         }
      return 1;
      }

   if (strcmp(variable,s_orbitname) == 0 ) {         /* orbitname=? */
      if(check_orbit_name(value))
         goto badarg;
      return 1;
      }
   if (strcmp(variable,s_3dmode) == 0 ) {         /* orbitname=? */
      int i,j;
      j = -1;
      for(i=0;i<4;i++)
         if(strcmp(value,juli3Doptions[i])==0)
            j = i;
      if(j < 0)
         goto badarg;
      else
         juli3Dmode = j;
      return 1;
      }

   if (strcmp(variable,s_julibrot3d) == 0) {       /* julibrot3d=?,?,?,? */
      if (floatparms != totparms)
         goto badarg;
      if(totparms > 0)
         zdots = (int)floatval[0];
      if (totparms > 1)
         originfp = (float)floatval[1];
      if (totparms > 2)
         depthfp = (float)floatval[2];
      if (totparms > 3)
         heightfp = (float)floatval[3];
      if (totparms > 4)
         widthfp = (float)floatval[4];
      if (totparms > 5)
         distfp = (float)floatval[5];
      return 1;
      }

   if (strcmp(variable,s_julibroteyes) == 0) {       /* julibroteyes=?,?,?,? */
      if (floatparms != totparms || totparms != 1)
         goto badarg;
      eyesfp =  (float)floatval[0];
      return 1;
      }

   if (strcmp(variable,s_julibrotfromto) == 0) {       /* julibrotfromto=?,?,?,? */
      if (floatparms != totparms || totparms != 4)
         goto badarg;
      mxmaxfp = floatval[0];
      mxminfp = floatval[1];
      mymaxfp = floatval[2];
      myminfp = floatval[3];
      return 1;
      }

   if (strcmp(variable,s_corners) == 0) {       /* corners=?,?,?,? */
      int dec;
      if (fractype == CELLULAR)
          return 1; /* skip setting the corners */
#if 0
      printf("totparms %d floatparms %d\n",totparms, floatparms);
      getch();
#endif
      if (  floatparms != totparms
            || (totparms != 0 && totparms != 4 && totparms != 6))
         goto badarg;
      usemag = 0;
      if (totparms == 0) return 0; /* turns corners mode on */
      initcorners = 1;
      /* good first approx, but dec could be too big */
      dec = get_max_curarg_len(floatvalstr,totparms) + 1;
      if((dec > DBL_DIG+1 || debugflag == 3200) && debugflag != 3400) {
         int old_bf_math;

         old_bf_math = bf_math;
         if(!bf_math || dec > decimals)
            init_bf_dec(dec);
         if(old_bf_math == 0) {
            int k;
            for (k = 0; k < MAXPARAMS; k++)
               floattobf(bfparms[k],param[k]);
         }

         /* xx3rd = xxmin = floatval[0]; */
         get_bf(bfxmin,floatvalstr[0]);
         get_bf(bfx3rd,floatvalstr[0]);

         /* xxmax = floatval[1]; */
         get_bf(bfxmax,floatvalstr[1]);

         /* yy3rd = yymin = floatval[2]; */
         get_bf(bfymin,floatvalstr[2]);
         get_bf(bfy3rd,floatvalstr[2]);

         /* yymax = floatval[3]; */
         get_bf(bfymax,floatvalstr[3]);

         if (totparms == 6) {
            /* xx3rd = floatval[4]; */
            get_bf(bfx3rd,floatvalstr[4]);

            /* yy3rd = floatval[5]; */
            get_bf(bfy3rd,floatvalstr[5]);
         }

         /* now that all the corners have been read in, get a more */
         /* accurate value for dec and do it all again             */

         dec = getprecbf_mag();
         if (dec < 0)
            goto badarg;     /* ie: Magnification is +-1.#INF */

         if(dec > decimals)  /* get corners again if need more precision */
         {
            init_bf_dec(dec);

            /* now get parameters and corners all over again at new
               decimal setting */
            for (k = 0; k < MAXPARAMS; k++)
               floattobf(bfparms[k],param[k]);

            /* xx3rd = xxmin = floatval[0]; */
            get_bf(bfxmin,floatvalstr[0]);
            get_bf(bfx3rd,floatvalstr[0]);

            /* xxmax = floatval[1]; */
            get_bf(bfxmax,floatvalstr[1]);

            /* yy3rd = yymin = floatval[2]; */
            get_bf(bfymin,floatvalstr[2]);
            get_bf(bfy3rd,floatvalstr[2]);

            /* yymax = floatval[3]; */
            get_bf(bfymax,floatvalstr[3]);

            if (totparms == 6) {
            /* xx3rd = floatval[4]; */
               get_bf(bfx3rd,floatvalstr[4]);

            /* yy3rd = floatval[5]; */
               get_bf(bfy3rd,floatvalstr[5]);
            }
         }
      }
      xx3rd = xxmin = floatval[0];
      xxmax =         floatval[1];
      yy3rd = yymin = floatval[2];
      yymax =         floatval[3];

      if (totparms == 6) {
         xx3rd =      floatval[4];
         yy3rd =      floatval[5];
         }
      return 1;
      }

   if (strcmp(variable,s_orbitcorners) == 0) {  /* orbit corners=?,?,?,? */
      set_orbit_corners = 0;
      if (  floatparms != totparms
            || (totparms != 0 && totparms != 4 && totparms != 6))
         goto badarg;
      ox3rd = oxmin = floatval[0];
      oxmax =         floatval[1];
      oy3rd = oymin = floatval[2];
      oymax =         floatval[3];

      if (totparms == 6) {
         ox3rd =      floatval[4];
         oy3rd =      floatval[5];
         }
      set_orbit_corners = 1;
      keep_scrn_coords = 1;
      return 1;
      }

   if (strcmp(variable,s_screencoords) == 0 ) {     /* screencoords=?   */
      if (yesnoval[0] < 0) goto badarg;
      keep_scrn_coords = yesnoval[0];
      return 1;
      }

   if (strcmp(variable,s_orbitdrawmode) == 0) {     /* orbitdrawmode=? */
      if ( charval[0] != 'l' && charval[0] != 'r' && charval[0] != 'f')
         goto badarg;
      drawmode = charval[0];
      return 1;
      }

   if (strcmp(variable,s_viewwindows) == 0) {  /* viewwindows=?,?,?,?,? */
      if (totparms > 5 || floatparms-intparms > 2 || intparms > 4)
         goto badarg;
      viewwindow = 1;
      viewreduction = (float)4.2;  /* reset default values */
      finalaspectratio = screenaspect;
      viewcrop = 1; /* yes */
      viewxdots = viewydots = 0;

      if((totparms > 0) && (floatval[0] > 0.001))
        viewreduction = (float)floatval[0];
      if((totparms > 1) && (floatval[1] > 0.001))
        finalaspectratio = (float)floatval[1];
      if((totparms > 2) && (yesnoval[2] == 0))
        viewcrop = yesnoval[2];
      if((totparms > 3) && (intval[3] > 0))
        viewxdots = intval[3];
      if((totparms == 5) && (intval[4] > 0))
        viewydots = intval[4];
      return 1;
      }

   if (strcmp(variable,s_centermag) == 0) {    /* center-mag=?,?,?[,?,?,?] */
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
      /* dec = get_max_curarg_len(floatvalstr,totparms); */
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

      if((dec <= DBL_DIG+1 && debugflag != 3200) || debugflag == 3400) { /* rough estimate that double is OK */
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
         if(!bf_math || dec > decimals)
            init_bf_dec(dec);
         if(old_bf_math == 0) {
            int k;
            for (k = 0; k < MAXPARAMS; k++)
               floattobf(bfparms[k],param[k]);
         }
         usemag = 1;
         saved = save_stack();
         bXctr            = alloc_stack(bflength+2);
         bYctr            = alloc_stack(bflength+2);
         /* Xctr = floatval[0]; */
         get_bf(bXctr,floatvalstr[0]);
         /* Yctr = floatval[1]; */
         get_bf(bYctr,floatvalstr[1]);
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

   if (strcmp(variable,s_aspectdrift) == 0 ) {  /* aspectdrift=? */
      if(floatparms != 1 || floatval[0] < 0)
         goto badarg;
      aspectdrift = (float)floatval[0];
      return 1;
      }

   if (strcmp(variable,s_invert) == 0) {        /* invert=?,?,? */
      if (totparms != floatparms || (totparms != 1 && totparms != 3))
         goto badarg;
      invert = ((inversion[0] = floatval[0]) != 0.0) ? totparms : 0;
      if (totparms == 3) {
         inversion[1] = floatval[1];
         inversion[2] = floatval[2];
         }
      return 1;
      }

   if (strcmp(variable,s_olddemmcolors) == 0 ) {     /* olddemmcolors=?   */
      if (yesnoval[0] < 0) goto badarg;
      old_demm_colors = yesnoval[0];
      return 0;
      }

   if (strcmp(variable,s_askvideo) == 0 ) {     /* askvideo=?   */
      if (yesnoval[0] < 0) goto badarg;
      askvideo = yesnoval[0];
      return 0;
      }

   if (strcmp(variable,s_ramvideo) == 0 )       /* ramvideo=?   */
      return 0; /* just ignore and return, for old time's sake */

   if (strcmp(variable,s_float) == 0 ) {        /* float=? */
      if (yesnoval[0] < 0) goto badarg;
#ifndef XFRACT
      usr_floatflag = (char)yesnoval[0];
#else
      usr_floatflag = 1; /* must use floating point */
#endif
      return 3;
      }

   if (strcmp(variable,s_fastrestore) == 0 ) {   /* fastrestore=? */
      if (yesnoval[0] < 0) goto badarg;
      fastrestore = (char)yesnoval[0];
      return 0;
      }

   if (strcmp(variable,s_orgfrmdir) == 0 ) {   /* orgfrmdir=? */
      if (valuelen > (FILE_MAX_DIR-1)) goto badarg;
      if(isadirectory(value) == 0) goto badarg;
      orgfrmsearch = 1;
      strcpy(orgfrmdir,value);
      fix_dirname(orgfrmdir);
      return 0;
   }

   if (strcmp(variable,s_biomorph) == 0 ) {     /* biomorph=? */
      usr_biomorph = numval;
      return 1;
      }

   if (strcmp(variable,s_orbitsave) == 0 ) {     /* orbitsave=? */
      if(charval[0] == 's')
         orbitsave |= 2;
      else if (yesnoval[0] < 0) goto badarg;
      orbitsave |= yesnoval[0];
      return 1;
      }

   if (strcmp(variable,s_bailout) == 0 ) {      /* bailout=? */
      if (floatval[0] < 1 || floatval[0] > 2100000000L) goto badarg;
      bailout = (long)floatval[0];
      return 1;
      }

   if (strcmp(variable,s_bailoutest) == 0 ) {   /* bailoutest=? */
      if     (strcmp(value,s_mod )==0) bailoutest = Mod;
      else if(strcmp(value,s_real)==0) bailoutest = Real;
      else if(strcmp(value,s_imag)==0) bailoutest = Imag;
      else if(strcmp(value,s_or  )==0) bailoutest = Or;
      else if(strcmp(value,s_and )==0) bailoutest = And;
      else if(strcmp(value,s_manh)==0) bailoutest = Manh;
      else if(strcmp(value,s_manr)==0) bailoutest = Manr;
      else goto badarg;
      setbailoutformula(bailoutest);
      return 1;
      }

   if (strcmp(variable,s_symmetry) == 0 ) {     /* symmetry=? */
      if     (strcmp(value,s_xaxis )==0) forcesymmetry = XAXIS;
      else if(strcmp(value,s_yaxis )==0) forcesymmetry = YAXIS;
      else if(strcmp(value,s_xyaxis)==0) forcesymmetry = XYAXIS;
      else if(strcmp(value,s_origin)==0) forcesymmetry = ORIGIN;
      else if(strcmp(value,s_pi    )==0) forcesymmetry = PI_SYM;
      else if(strcmp(value,s_none  )==0) forcesymmetry = NOSYM;
      else goto badarg;
      return 1;
      }

   if (strcmp(variable,s_printer) == 0 ) {      /* printer=? */
      if (parse_printer(value) < 0) goto badarg;
      return 0;
      }

   if (strcmp(variable,s_printfile) == 0) {     /* printfile=? */
      int existdir;
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if((existdir=merge_pathnames(PrintName, value, mode))==0)
         Print_To_File = 1;
      else if (existdir < 0)
         init_msg(0,variable,value,mode);
      return 0;
      }
   if(strcmp(variable,s_rleps) == 0) {
      Printer_Compress = yesnoval[0];
      return(0);
      }
   if(strcmp(variable,s_colorps) == 0) {
      ColorPS = yesnoval[0];
      return(0);
      }

   if (strcmp(variable,s_epsf) == 0) {          /* EPS type? SWT */
      Print_To_File = 1;
      EPSFileType = numval;
      Printer_Type = 5;
      if (strcmp(PrintName,s_fract001prn)==0)
         strcpy(PrintName,"fract001.eps");
      return 0;
      }

   if (strcmp(variable,s_title) == 0) {         /* Printer title block? SWT */
      if (yesnoval[0] < 0) goto badarg;
      Printer_Titleblock = yesnoval[0];
      return 0;
      }

   if (strcmp(variable,s_translate) == 0) {     /* Translate color? SWT */
      Printer_ColorXlat=0;
      if (charval[0] == 'y')
         Printer_ColorXlat=1;
      else if (numval > 1 || numval < -1)
         Printer_ColorXlat=numval;
      return 0;
      }

   if (strcmp(variable,s_plotstyle) == 0) {     /* plot style? SWT */
      Printer_SStyle = numval;
      return 0;
      }

   if (strcmp(variable,s_halftone) == 0) {      /* New halftoning? SWT */
      if (totparms != intparms) goto badarg;
      Printer_SetScreen=1;
      if ((totparms >  0) && ( intval[ 0] >= 0))
                                          Printer_SFrequency = intval[ 0];
      if ((totparms >  1) && ( intval[ 1] >= 0))
                                          Printer_SAngle     = intval[ 1];
      if ((totparms >  2) && ( intval[ 2] >= 0))
                                          Printer_SStyle     = intval[ 2];
      if ((totparms >  3) && ( intval[ 3] >= 0))
                                          Printer_RFrequency = intval[ 3];
      if ((totparms >  4) && ( intval[ 4] >= 0))
                                          Printer_RAngle     = intval[ 4];
      if ((totparms >  5) && ( intval[ 5] >= 0))
                                          Printer_RStyle     = intval[ 5];
      if ((totparms >  6) && ( intval[ 6] >= 0))
                                          Printer_GFrequency = intval[ 6];
      if ((totparms >  7) && ( intval[ 7] >= 0))
                                          Printer_GAngle     = intval[ 7];
      if ((totparms >  8) && ( intval[ 8] >= 0))
                                          Printer_GStyle     = intval[ 8];
      if ((totparms >  9) && ( intval[ 9] >= 0))
                                          Printer_BFrequency = intval[ 9];
      if ((totparms > 10) && ( intval[10] >= 0))
                                          Printer_BAngle     = intval[10];
      if ((totparms > 11) && ( intval[11] >= 0))
                                          Printer_BStyle     = intval[11];
      return 0;
      }

   if (strcmp(variable,s_linefeed) == 0) {      /* Use LF for printer */
      if      (strcmp(value,s_cr)   == 0) Printer_CRLF = 1;
      else if (strcmp(value,s_lf)   == 0) Printer_CRLF = 2;
      else if (strcmp(value,s_crlf) == 0) Printer_CRLF = 0;
      else goto badarg;
      return 0;
      }

   if (strcmp(variable,s_comport) == 0 ) {      /* Set the COM parameters */
      if ((value=strchr(value,'/')) == NULL) goto badarg;
      switch (atoi(++value)) {
         case 110:  l = 0;   break;
         case 150:  l = 32;  break;
         case 300:  l = 64;  break;
         case 600:  l = 96;  break;
         case 1200: l = 128; break;
         case 2400: l = 160; break;
         case 4800: l = 192; break;
         case 9600:
         default:   l = 224; break;
         }
      if ((value=strchr(value,'/')) == NULL) goto badarg;
      for (k=0; k < (int)strlen(value); k++) {
         switch (value[k]) {
            case '7':  l |= 2;  break;
            case '8':  l |= 3;  break;
            case 'o':  l |= 8;  break;
            case 'e':  l |= 24; break;
            case '2':  l |= 4;  break;
            }
         }
#if !defined(XFRACT) && !defined(_WIN32)
#ifndef WINFRACT
      _bios_serialcom(0,numval-1,l);
#endif
#endif
      return 0;
      }

   if (strcmp(variable,s_sound) == 0 ) {        /* sound=?,?,? */
      if (totparms > 5)
         goto badarg;
      soundflag = 0; /* start with a clean slate, add bits as we go */
      if (totparms == 1)
         soundflag = 8; /* old command, default to PC speaker */

      /* soundflag is used as a bitfield... bit 0,1,2 used for whether sound
         is modified by an orbits x,y,or z component. and also to turn it on
         or off (0==off, 1==beep (or yes), 2==x, 3==y, 4==z),
         Bit 3 is used for flagging the PC speaker sound,
         Bit 4 for OPL3 FM soundcard output,
         Bit 5 will be for midi output (not yet),
         Bit 6 for whether the tone is quantised to the nearest 'proper' note
          (according to the western, even tempered system anyway) */

      if (charval[0] == 'n' || charval[0] == 'o')
         soundflag = soundflag & 0xF8; 
      else if ((far_strncmp(value,"ye",2) == 0) || (charval[0] == 'b'))
         soundflag = soundflag | 1;
      else if (charval[0] == 'x')
         soundflag = soundflag | 2;
      else if (charval[0] == 'y' && far_strncmp(value,"ye",2) != 0)
         soundflag = soundflag | 3;
      else if (charval[0] == 'z')
         soundflag = soundflag | 4;
      else
         goto badarg;
#if !defined(XFRACT)
      if (totparms > 1) {
       int i;
         soundflag = soundflag & 7; /* reset options */
         for (i = 1; i < totparms; i++) {
          /* this is for 2 or more options at the same time */
            if (charval[i] == 'f') { /* (try to)switch on opl3 fm synth */
               if (driver_init_fm())
                  soundflag = soundflag | 16;
               else soundflag = (soundflag & 0xEF);
            }
            else if (charval[i] == 'p')
               soundflag = soundflag | 8;
            else if (charval[i] == 'm')
               soundflag = soundflag | 32;
            else if (charval[i] == 'q')
               soundflag = soundflag | 64;
            else
               goto badarg;
         } /* end for */
      }    /* end totparms > 1 */
      return 0;
      }

   if (strcmp(variable,s_hertz) == 0) {         /* Hertz=? */
      basehertz = numval;
      return 0;
      }

   if (strcmp(variable,s_volume) == 0) {         /* Volume =? */
      fm_vol = numval & 0x3F; /* 63 */
      return 0;
      }

   if (strcmp(variable,s_atten) == 0) {
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

   if(strcmp(variable,s_polyphony) == 0) {
      if (numval > 9)
         goto badarg;
      polyphony = abs(numval-1);
      return(0);
   } 

   if(strcmp(variable,s_wavetype) == 0) { /* wavetype = ? */
      fm_wavetype = numval & 0x0F;
      return(0);
   }

   if(strcmp(variable,s_attack) == 0) { /* attack = ? */
      fm_attack = numval & 0x0F;
      return(0);
   }

   if(strcmp(variable,s_decay) == 0) { /* decay = ? */
      fm_decay = numval & 0x0F;
      return(0);
   }

   if(strcmp(variable,s_sustain) == 0) { /* sustain = ? */
      fm_sustain = numval & 0x0F;
      return(0);
   }
   
   if(strcmp(variable,s_srelease) == 0) { /* release = ? */
      fm_release = numval & 0x0F;
      return(0);
   }

   if (strcmp(variable,s_scalemap) == 0) {      /* Scalemap=?,?,?,?,?,?,?,?,?,?,? */
      int counter;
      if (totparms != intparms) goto badarg;
      for(counter=0;counter <=11;counter++)
         if ((totparms > counter) && (intval[counter] > 0)
           && (intval[counter] < 13))
             scale_map[counter] = intval[counter];
#endif
      return(0);
   } 

   if (strcmp(variable,s_periodicity) == 0 ) {  /* periodicity=? */
      usr_periodicitycheck=1;
      if ((charval[0] == 'n') || (numval == 0))
         usr_periodicitycheck=0;
      else if (charval[0] == 'y')
         usr_periodicitycheck=1;
      else if (charval[0] == 's')   /* 's' for 'show' */
         usr_periodicitycheck= -1;
      else if(numval == NONNUMERIC)
         goto badarg;
      else if(numval != 0)
         usr_periodicitycheck=numval;
      if (usr_periodicitycheck > 255) usr_periodicitycheck = 255;
      if (usr_periodicitycheck < -255) usr_periodicitycheck = -255;
      return 1;
      }

   if (strcmp(variable,s_logmap) == 0 ) {       /* logmap=? */
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

   if (strcmp(variable,s_logmode) == 0 ) {       /* logmode=? */
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

   if (strcmp(variable,s_debugflag) == 0
     || strcmp(variable,s_debug) == 0) {        /* internal use only */
      debugflag = numval;
      timerflag = debugflag & 1;                /* separate timer flag */
      debugflag -= timerflag;
      return 0;
      }

   if (strcmp(variable,s_rseed) == 0) {
      rseed = numval;
      rflag = 1;
      return 1;
      }

   if (strcmp(variable,s_orbitdelay) == 0) {
      orbit_delay = numval;
      return 0;
      }

   if (strcmp(variable,s_orbitinterval) == 0) {
      orbit_interval = numval;
      if (orbit_interval < 1)
         orbit_interval = 1;
      if (orbit_interval > 255)
         orbit_interval = 255;
      return 0;
      }

   if (strcmp(variable,s_showdot) == 0) {
      showdot = 15;
      if(totparms > 0)
      {
         autoshowdot = (char)0;
         if(isalpha(charval[0]))
         {
            if(strchr("abdm",(int)charval[0]) != NULL)
               autoshowdot = charval[0];
            else
               goto badarg;
         }
         else   
         {
            showdot=numval;
            if(showdot<0)
               showdot=-1;
         }
         if(totparms > 1 && intparms > 0)
            sizedot = intval[1];
         if(sizedot < 0)
            sizedot = 0;   
      }      
      return 0;
      }

   if (strcmp(variable,s_showorbit) == 0) {  /* showorbit=yes|no */
      start_showorbit=(char)yesnoval[0];
      return 0;
      }

   if (strcmp(variable,s_decomp) == 0) {
      if (totparms != intparms || totparms < 1) goto badarg;
      decomp[0] = intval[0];
      decomp[1] = 0;
      if (totparms > 1) /* backward compatibility */
         bailout = decomp[1] = intval[1];
      return 1;
      }

   if (strcmp(variable,s_distest) == 0) {
      if (totparms != intparms || totparms < 1) goto badarg;
      usr_distest = (long)floatval[0];
      distestwidth = 71;
      if (totparms > 1)
         distestwidth = intval[1];
      if(totparms > 3 && intval[2] > 0 && intval[3] > 0) {
         pseudox = intval[2];
         pseudoy = intval[3];
      }
      else
        pseudox = pseudoy = 0;
      return 1;
      }

   if (strcmp(variable,s_formulafile) == 0) {   /* formulafile=? */
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if(merge_pathnames(FormFileName, value, mode)<0)
         init_msg(0,variable,value,mode);
      return 1;
      }

   if (strcmp(variable,s_formulaname) == 0) {   /* formulaname=? */
      if (valuelen > ITEMNAMELEN) goto badarg;
      strcpy(FormName,value);
      return 1;
      }

   if (strcmp(variable,s_lfile) == 0) {    /* lfile=? */
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if(merge_pathnames(LFileName, value, mode)<0)
         init_msg(0,variable,value,mode);
      return 1;
      }

   if (strcmp(variable,s_lname) == 0) {
      if (valuelen > ITEMNAMELEN) goto badarg;
      strcpy(LName,value);
      return 1;
      }

   if (strcmp(variable,s_ifsfile) == 0) {    /* ifsfile=?? */
      int existdir;
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if((existdir=merge_pathnames(IFSFileName, value, mode))==0)
         reset_ifs_defn();
      else if(existdir < 0)
         init_msg(0,variable,value,mode);
      return 1;
      }


   if (strcmp(variable,s_ifs) == 0
     || strcmp(variable,s_ifs3d) == 0) {        /* ifs3d for old time's sake */
      if (valuelen > ITEMNAMELEN) goto badarg;
      strcpy(IFSName,value);
      reset_ifs_defn();
      return 1;
      }

   if (strcmp(variable,s_parmfile) == 0) {   /* parmfile=? */
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if(merge_pathnames(CommandFile, value, mode)<0)
         init_msg(0,variable,value,mode);
      return 1;
      }

   if (strcmp(variable,s_stereo) == 0) {        /* stereo=? */
      if ((numval<0) || (numval>4)) goto badarg;
      glassestype = numval;
      return 3;
      }

   if (strcmp(variable,s_rotation) == 0) {      /* rotation=?/?/? */
      if (totparms != 3 || intparms != 3) goto badarg;
      XROT = intval[0];
      YROT = intval[1];
      ZROT = intval[2];
      return 3;
      }

   if (strcmp(variable,s_perspective) == 0) {   /* perspective=? */
      if (numval == NONNUMERIC) goto badarg;
      ZVIEWER = numval;
      return 3;
      }

   if (strcmp(variable,s_xyshift) == 0) {       /* xyshift=?/?  */
      if (totparms != 2 || intparms != 2) goto badarg;
      XSHIFT = intval[0];
      YSHIFT = intval[1];
      return 3;
      }

   if (strcmp(variable,s_interocular) == 0) {   /* interocular=? */
      eyeseparation = numval;
      return 3;
      }

   if (strcmp(variable,s_converge) == 0) {      /* converg=? */
      xadjust = numval;
      return 3;
      }

   if (strcmp(variable,s_crop) == 0) {          /* crop=? */
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

   if (strcmp(variable,s_bright) == 0) {        /* bright=? */
      if (totparms != 2 || intparms != 2) goto badarg;
      red_bright  = intval[0];
      blue_bright = intval[1];
      return 3;
      }

   if (strcmp(variable,s_xyadjust) == 0) {      /* trans=? */
      if (totparms != 2 || intparms != 2) goto badarg;
      xtrans = intval[0];
      ytrans = intval[1];
      return 3;
      }

   if (strcmp(variable,s_3d) == 0) {            /* 3d=?/?/..    */
      if(strcmp(value,s_overlay)==0) {
         yesnoval[0]=1;
         if(calc_status > -1) /* if no image, treat same as 3D=yes */
            overlay3d=1;
      }
      else if (yesnoval[0] < 0) goto badarg;
      display3d = yesnoval[0];
      initvars_3d();
      return (display3d) ? 6 : 2;
      }

   if (strcmp(variable,s_sphere) == 0 ) {       /* sphere=? */
      if (yesnoval[0] < 0) goto badarg;
      SPHERE = yesnoval[0];
      return 2;
      }

   if (strcmp(variable,s_scalexyz) == 0) {      /* scalexyz=?/?/? */
      if (totparms < 2 || intparms != totparms) goto badarg;
      XSCALE = intval[0];
      YSCALE = intval[1];
      if (totparms > 2) ROUGH = intval[2];
      return 2;
      }

   /* "rough" is really scale z, but we add it here for convenience */
   if (strcmp(variable,s_roughness) == 0) {     /* roughness=?  */
      ROUGH = numval;
      return 2;
      }

   if (strcmp(variable,s_waterline) == 0) {     /* waterline=?  */
      if (numval<0) goto badarg;
      WATERLINE = numval;
      return 2;
      }

   if (strcmp(variable,s_filltype) == 0) {      /* filltype=?   */
      if (numval < -1 || numval > 6) goto badarg;
      FILLTYPE = numval;
      return 2;
      }

   if (strcmp(variable,s_lightsource) == 0) {   /* lightsource=?/?/? */
      if (totparms != 3 || intparms != 3) goto badarg;
      XLIGHT = intval[0];
      YLIGHT = intval[1];
      ZLIGHT = intval[2];
      return 2;
      }

   if (strcmp(variable,s_smoothing) == 0) {     /* smoothing=?  */
      if (numval<0) goto badarg;
      LIGHTAVG = numval;
      return 2;
      }

   if (strcmp(variable,s_latitude) == 0) {      /* latitude=?/? */
      if (totparms != 2 || intparms != 2) goto badarg;
      THETA1 = intval[0];
      THETA2 = intval[1];
      return 2;
      }

   if (strcmp(variable,s_longitude) == 0) {     /* longitude=?/? */
      if (totparms != 2 || intparms != 2) goto badarg;
      PHI1 = intval[0];
      PHI2 = intval[1];
      return 2;
      }

   if (strcmp(variable,s_radius) == 0) {        /* radius=? */
      if (numval < 0) goto badarg;
      RADIUS = numval;
      return 2;
      }

   if (strcmp(variable,s_transparent) == 0) {   /* transparent? */
      if (totparms != intparms || totparms < 1) goto badarg;
      transparent[1] = transparent[0] = intval[0];
      if (totparms > 1) transparent[1] = intval[1];
      return 2;
      }

   if (strcmp(variable,s_preview) == 0) {       /* preview? */
      if (yesnoval[0] < 0) goto badarg;
      preview = (char)yesnoval[0];
      return 2;
      }

   if (strcmp(variable,s_showbox) == 0) {       /* showbox? */
      if (yesnoval[0] < 0) goto badarg;
      showbox = (char)yesnoval[0];
      return 2;
      }

   if (strcmp(variable,s_coarse) == 0) {        /* coarse=? */
      if (numval < 3 || numval > 2000) goto badarg;
      previewfactor = numval;
      return 2;
      }

   if (strcmp(variable,s_randomize) == 0) {     /* RANDOMIZE=? */
      if (numval<0 || numval>7) goto badarg;
      RANDOMIZE = numval;
      return 2;
      }

   if (strcmp(variable,s_ambient) == 0) {       /* ambient=? */
      if (numval<0||numval>100) goto badarg;
      Ambient = numval;
      return 2;
      }

   if (strcmp(variable,s_haze) == 0) {          /* haze=? */
      if (numval<0||numval>100) goto badarg;
      haze = numval;
      return 2;
      }

   if (strcmp(variable,s_fullcolor) == 0) {     /* fullcolor=? */
      if (yesnoval[0] < 0) goto badarg;
      Targa_Out = yesnoval[0];
      return 2;
      }

   if (strcmp(variable,s_truecolor) == 0) {     /* truecolor=? */
      if (yesnoval[0] < 0) goto badarg;
      truecolor = yesnoval[0];
      return 3;
      }

   if (strcmp(variable,s_truemode) == 0) {    /* truemode=? */
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

   if (strcmp(variable,s_usegrayscale) == 0) {     /* usegrayscale? */
      if (yesnoval[0] < 0) goto badarg;
      grayflag = (char)yesnoval[0];
      return 2;
      }

   if (strcmp(variable,s_monitorwidth) == 0) {     /* monitorwidth=? */
      if (totparms != 1 || floatparms != 1) goto badarg;
      AutoStereo_width  = floatval[0];
      return 2;
      }

   if (strcmp(variable,s_targa_overlay) == 0) {         /* Targa Overlay? */
      if (yesnoval[0] < 0) goto badarg;
      Targa_Overlay = yesnoval[0];
      return 2;
      }

   if (strcmp(variable,s_background) == 0) {     /* background=?/? */
      if (totparms != 3 || intparms != 3) goto badarg;
                for (i=0;i<3;i++)
                        if (intval[i] & ~0xff)
                                goto badarg;
      back_color[0] = (BYTE)intval[0];
      back_color[1] = (BYTE)intval[1];
      back_color[2] = (BYTE)intval[2];
      return 2;
      }

   if (strcmp(variable,s_lightname) == 0) {     /* lightname=?   */
      if (valuelen > (FILE_MAX_PATH-1)) goto badarg;
      if (first_init || mode == 2)
         strcpy(light_name,value);
      return 0;
      }

   if (strcmp(variable,s_ray) == 0) {           /* RAY=? */
      if (numval < 0 || numval > 6) goto badarg;
      RAY = numval;
      return 2;
      }

   if (strcmp(variable,s_brief) == 0) {         /* BRIEF? */
      if (yesnoval[0] < 0) goto badarg;
      BRIEF = yesnoval[0];
      return 2;
      }

   if (strcmp(variable,s_release) == 0) {       /* release */
      if (numval < 0) goto badarg;

      save_release = numval;
      return 2;
      }

   if (strcmp(variable,s_curdir) == 0) {         /* curdir= */
      if (yesnoval[0] < 0) goto badarg;
      checkcurdir = yesnoval[0];
      return 0;
      }

   if (strcmp(variable,s_virtual) == 0) {         /* virtual= */
      if (yesnoval[0] < 0) goto badarg;
      virtual_screens = yesnoval[0];
      return 1;
      }

badarg:
   argerror(curarg);
   return(-1);

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
   if (strcmp(value,s_mono) == 0) {
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
            if ((value = strchr(value,'/')) == NULL) break;
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
      if(merge_pathnames(MAP_name,&value[1],3)<0)
         init_msg(0,"",&value[1],3);
      if ((int)strlen(value) > FILE_MAX_PATH || ValidateLuts(MAP_name) != 0)
         goto badcolor;
      if (display3d) {
        mapset = 1;
        }
      else {
        if(merge_pathnames(colorfile,&value[1],3)<0)
          init_msg(0,"",&value[1],3);
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
               dacbox[i][j] = (BYTE)k;
               if (smooth) {
                  int start,spread,cnum;
                  start = i - (spread = smooth + 1);
                  cnum = 0;
                  if ((k - (int)dacbox[start][j]) == 0) {
                     while (++cnum < spread)
                        dacbox[start+cnum][j] = (BYTE)k;
                     }
                  else {
                     while (++cnum < spread)
                        dacbox[start+cnum][j] =
            (BYTE)(( cnum *dacbox[i][j]
            + (i-(start+cnum))*dacbox[start][j]
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
         dacbox[i][0] = dacbox[i][1] = dacbox[i][2] = 40;
         ++i;
         }
      colorstate = 1;
      }
   colorpreloaded = 1;
   memcpy(olddacbox,dacbox,256*3);
   return(0);
badcolor:
   return(-1);
}

static int parse_printer(char *value)
{
   int k;
   if (value[0]=='h' && value[1]=='p')
      Printer_Type=1;                        /* HP LaserJet            */
   if (value[0]=='i' && value[1]=='b')
      Printer_Type=2;                        /* IBM Graphics           */
   if (value[0]=='e' && value[1]=='p')
      Printer_Type=2;                        /* Epson (model?)         */
   if (value[0]=='c' && value[1]=='o')
      Printer_Type=3;                        /* Star (Epson-Comp?) color */
   if (value[0]=='p') {
      if (value[1]=='a')
         Printer_Type=4;                     /* HP Paintjet (color)    */
      if ((value[1]=='o' || value[1]=='s')) {
         Printer_Type=5;                     /* PostScript  SWT */
         if (value[2]=='h' || value[2]=='l')
            Printer_Type=6;
         }
      if (value[1]=='l')
         Printer_Type=7;                     /* HP Plotter (semi-color) */
      }
   if (Printer_Type == 1)                    /* assume low resolution */
      Printer_Resolution = 75;
   else
      Printer_Resolution = 60;
   if (EPSFileType > 0)                      /* EPS save - force type 5 */
      Printer_Type = 5;
   if ((Printer_Type == 5) || (Printer_Type == 6))
      Printer_Resolution = 150;              /* PostScript def. res. */
   if ((value=strchr(value,'/')) != NULL) {
      if ((k=atoi(++value)) >= 0) Printer_Resolution=k;
      if ((value=strchr(value,'/')) != NULL) {
         if ((k=atoi(++value))> 0) LPTNumber = k;
         if (k < 0) {
            Print_To_File = 1;
            LPTNumber = 1;
            }
         }
      }
   return(0);
}



static void argerror(char *badarg)      /* oops. couldn't decode this */
{
   static FCODE argerrmsg1[]={"\
Oops. I couldn't understand the argument:\n  "};
   static FCODE argerrmsg2[]={"\n\n\
(see the Startup Help screens or documentation for a complete\n\
 argument list with descriptions)"};
   char msg[300];
   if ((int)strlen(badarg) > 70) badarg[70] = 0;
   if (active_system == 0 /* DOS */
     && first_init)       /* & this is 1st call to cmdfiles */
#ifndef XFRACT
      sprintf(msg,"%Fs%s%Fs",(char far *)argerrmsg1,badarg,(char far *)argerrmsg2);
   else
      sprintf(msg,"%Fs%s",(char far *)argerrmsg1,badarg);
#else
      sprintf(msg,"%s%s%s",argerrmsg1,badarg,argerrmsg2);
   else
      sprintf(msg,"%s%s",argerrmsg1,badarg);
#endif
   stopmsg(0,msg);
   if (initbatch) {
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
   if(SPHERE) {
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
      if (active_system != 0)
         FILLTYPE = 2;
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
   if(s)
      *s = 0;
   strtobf(bf,curarg);
   if(s)
      *s = '/';
   return(0);
}

/* Get length of current args */
int get_curarg_len(char *curarg)
{
   int len;
   char *s;
   s=strchr(curarg,'/');
   if(s)
      *s = 0;
   len = (int) strlen(curarg);
   if(s)
      *s = '/';
   return(len);
}

/* Get max length of current args */
int get_max_curarg_len(char *floatvalstr[], int totparms)
{
   int i,tmp,max_str;
   max_str = 0;
   for(i=0;i<totparms;i++)
      if((tmp=get_curarg_len(floatvalstr[i])) > max_str)
         max_str = tmp;
   return(max_str);
}

/* mode = 0 command line @filename         */
/*        1 sstools.ini                    */
/*        2 <@> command after startup      */
/*        3 command line @filename/setname */
/* this is like stopmsg() but can be used in cmdfiles()      */
/* call with NULL for badfilename to get pause for getakey() */
int init_msg(int flags,char *cmdstr,char far *badfilename,int mode)
{
   char far *modestr[4] =
       {s_commandline,s_sstoolsini,s_at_cmd,s_at_cmd};
   static FCODE diags[] =
       {"Fractint found the following problems when parsing commands: "};
   char msg[256];
   char cmd[80];
   static int row = 1;

   if (initbatch == 1) { /* in batch mode */
      if(badfilename)
         /* uncomment next if wish to cause abort in batch mode for
            errors in CMDFILES.C such as parsing SSTOOLS.INI */
         /* initbatch = 4; */ /* used to set errorlevel */
      return (-1);
   }
   strncpy(cmd,cmdstr,30);
   cmd[29] = 0;

   if(*cmd)
      strcat(cmd,"=");
   if(badfilename)
#ifndef XFRACT
      sprintf(msg,"Can't find %s%Fs, please check %Fs",cmd,badfilename,modestr[mode]);
#else
      sprintf(msg,"Can't find %s%s, please check %s",cmd,badfilename,modestr[mode]);
#endif
   if (active_system == 0 /* DOS */
     && first_init) {     /* & cmdfiles hasn't finished 1st try */
      if(row == 1 && badfilename) {
	     driver_set_for_text();
         driver_put_string(0,0,15,diags);
      }
      if(badfilename)
         driver_put_string(row++,0,7,msg);
      else if(row > 1){
         driver_put_string(++row,0,15,s_escapetoabort);
         driver_move_cursor(row+1,0);
         /*
         if(getakeynohelp()==27)
            goodbye();
         */
         dopause(2);  /* defer getakeynohelp until after parseing */
      }
   }
   else if(badfilename)
      stopmsg(flags,msg);
   return(0);
}

/* defer pause until after parsing so we know if in batch mode */
void dopause(int action)
{
   static unsigned char needpause = 0;
   switch(action)
   {
   case 0:
      if(initbatch == 0)
      {
         if(needpause == 1)
            getakey();
         else if (needpause == 2)
            if(getakeynohelp() == ESC)
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
   while(*s != 0 && *s != '/' && *s != ' ')
   {
      if(*s == '-' || *s == '+') numsign++;
      else if(*s == '.') numdot++;
      else if(*s == 'e' || *s == 'E' || *s == 'g' || *s == 'G') nume++;
      else if(!isdigit(*s)) {result=0; break;}
      s++;
   }
   if(numdot > 1 || numsign > 2 || nume > 1) result=0;
   return(result);
}

