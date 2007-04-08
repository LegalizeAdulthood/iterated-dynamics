/*
		Command-line / Command-File Parser Routines
*/
#include <assert.h>
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

#define INIT_GIF87      0       /* Turn on GIF 89a processing  */
#define NON_NUMERIC -32767

/* variables defined by the command line/files processor */
int     g_stop_pass = 0;             /* stop at this guessing pass early */
int     g_pseudo_x = 0;              /* xdots to use for video independence */
int     g_pseudo_y = 0;              /* ydots to use for video independence */
int     g_bf_digits = 0;             /* digits to use (force) for bf_math */
int     g_show_dot = -1;             /* color to show crawling graphics cursor */
int     g_size_dot;                /* size of dot crawling cursor */
char    g_record_colors;           /* default PAR color-writing method */
char    g_auto_show_dot = 0;          /* dark, medium, bright */
char    g_start_show_orbit = 0;      /* show orbits on at start of fractal */
char    readname[FILE_MAX_PATH]; /* name of fractal input file */
char    tempdir[FILE_MAX_DIR] = {""}; /* name of temporary directory */
char    workdir[FILE_MAX_DIR] = {""}; /* name of directory for misc files */
char    orgfrmdir[FILE_MAX_DIR] = {""}; /*name of directory for orgfrm files*/
char    gifmask[FILE_MAX_PATH] = {""};
char    PrintName[FILE_MAX_PATH] = {"fract001.prn"}; /* Name for print-to-file */
char    savename[FILE_MAX_PATH] = {"fract001"};  /* save files using this name */
char    autoname[FILE_MAX_PATH] = {"auto.key"}; /* record auto keystrokes here */
int     potflag = 0;              /* continuous potential enabled? */
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
int     initbatch = 0;			/* 1 if batch run (no kbd)  */
int     initsavetime;           /* autosave minutes         */
_CMPLX  initorbit;              /* initial orbitvalue */
char    useinitorbit;           /* flag for initorbit */
int     g_init_mode;               /* initial video mode       */
int     initcyclelimit;         /* initial cycle limit      */
BYTE    usemag;                 /* use center-mag corners   */
long    bailout;                /* user input bailout value */
enum bailouts g_bail_out_test;       /* test used for determining bailout */
double  inversion[3];           /* radius, xcenter, ycenter */
int     rotate_lo, rotate_hi;    /* cycling color range      */
int *ranges;                /* iter->color ranges mapping */
int     rangeslen = 0;          /* size of ranges array     */
BYTE *mapdacbox = NULL;     /* map= (default colors)    */
int     colorstate;             /* 0, g_dac_box matches default (bios or map=) */
								/* 1, g_dac_box matches no known defined map   */
								/* 2, g_dac_box matches the colorfile map      */
int     colorpreloaded;         /* if g_dac_box preloaded for next mode select */
int     save_release;           /* release creating PAR file*/
char    dontreadcolor = 0;        /* flag for reading color from GIF */
double  math_tol[2] = {.05, .05};  /* For math transition */
int Targa_Out = 0;              /* 3D fullcolor flag */
int truecolor = 0;              /* escape time truecolor flag */
int truemode = TRUEMODE_DEFAULT;               /* truecolor coloring scheme */
char    colorfile[FILE_MAX_PATH]; /* from last <l> <s> or colors=@filename */
int functionpreloaded; /* if function loaded for new bifs, JCO 7/5/92 */
float   screenaspect = DEFAULTASPECT;   /* aspect ratio of the screen */
float   aspectdrift = DEFAULTASPECTDRIFT;  /* how much drift is allowed and */
								/* still forced to screenaspect  */
int fastrestore = 0;          /* 1 - reset viewwindows prior to a restore
								and do not display warnings when video
								mode changes during restore */
int orgfrmsearch = 0;            /* 1 - user has specified a directory for
									Orgform formula compilation files */
int     orbitsave = ORBITSAVE_NONE;          /* for IFS and LORENZ to output acrospin file */
int orbit_delay;                /* clock ticks delating orbit release */
int     transparent[2];         /* transparency min/max values */
long    LogFlag;                /* Logarithmic palette flag: 0 = no */
BYTE exitmode = 3;      /* video mode on exit */
int     Log_Fly_Calc = 0;   /* calculate logmap on-the-fly */
int     Log_Auto_Calc = 0;  /* auto calculate logmap */
int     nobof = 0; /* Flag to make inside=bof options not duplicate bof images */
int        escape_exit;         /* set to 1 to avoid the "are you sure?" screen */
int first_init = 1;               /* first time into cmdfiles? */
struct fractalspecificstuff *curfractalspecific = NULL;
char FormFileName[FILE_MAX_PATH]; /* file to find (type=)formulas in */
char FormName[ITEMNAMELEN + 1];    /* Name of the Formula (if not null) */
char LFileName[FILE_MAX_PATH];   /* file to find (type=)L-System's in */
char LName[ITEMNAMELEN + 1];       /* Name of L-System */
char CommandFile[FILE_MAX_PATH]; /* file to find command sets in */
char CommandName[ITEMNAMELEN + 1]; /* Name of Command set */
char CommandComment[4][MAXCMT];    /* comments for command set */
char IFSFileName[FILE_MAX_PATH]; /* file to find (type=)IFS in */
char IFSName[ITEMNAMELEN + 1];    /* Name of the IFS def'n (if not null) */
struct SearchPath searchfor;
float *ifs_defn = NULL;     /* ifs parameters */
int  ifs_type;                  /* 0 = 2d, 1 = 3d */
int  g_slides = SLIDES_OFF;                /* 1 autokey=play, 2 autokey=record */
BYTE txtcolor[]=
{
		BLUE*16 + L_WHITE,    /* C_TITLE           title background */
		BLUE*16 + L_GREEN,    /* C_TITLE_DEV       development vsn foreground */
		GREEN*16 + YELLOW,    /* C_HELP_HDG        help page title line */
		WHITE*16 + BLACK,     /* C_HELP_BODY       help page body */
		GREEN*16 + GRAY,      /* C_HELP_INSTR      help page instr at bottom */
		WHITE*16 + BLUE,      /* C_HELP_LINK       help page links */
		CYAN*16 + BLUE,       /* C_HELP_CURLINK    help page current link */
		WHITE*16 + GRAY,      /* C_PROMPT_BKGRD    prompt/choice background */
		WHITE*16 + BLACK,     /* C_PROMPT_TEXT     prompt/choice extra info */
		BLUE*16 + WHITE,      /* C_PROMPT_LO       prompt/choice text */
		BLUE*16 + L_WHITE,    /* C_PROMPT_MED      prompt/choice hdg2/... */
		BLUE*16 + YELLOW,     /* C_PROMPT_HI       prompt/choice hdg/cur/... */
		GREEN*16 + L_WHITE,   /* C_PROMPT_INPUT    fullscreen_prompt input */
		CYAN*16 + L_WHITE,    /* C_PROMPT_CHOOSE   fullscreen_prompt choice */
		MAGENTA*16 + L_WHITE, /* C_CHOICE_CURRENT  fullscreen_choice input */
		BLACK*16 + WHITE,     /* C_CHOICE_SP_INSTR speed key bar & instr */
		BLACK*16 + L_MAGENTA, /* C_CHOICE_SP_KEYIN speed key value */
		WHITE*16 + BLUE,      /* C_GENERAL_HI      tab, thinking, IFS */
		WHITE*16 + BLACK,     /* C_GENERAL_MED */
		WHITE*16 + GRAY,      /* C_GENERAL_LO */
		BLACK*16 + L_WHITE,   /* C_GENERAL_INPUT */
		WHITE*16 + BLACK,     /* C_DVID_BKGRD      disk video */
		BLACK*16 + YELLOW,    /* C_DVID_HI */
		BLACK*16 + L_WHITE,   /* C_DVID_LO */
		RED*16 + L_WHITE,     /* C_STOP_ERR        stop message, error */
		GREEN*16 + BLACK,     /* C_STOP_INFO       stop message, info */
		BLUE*16 + WHITE,      /* C_TITLE_LOW       bottom lines of title screen */
		GREEN*16 + BLACK,     /* C_AUTHDIV1        title screen dividers */
		GREEN*16 + GRAY,      /* C_AUTHDIV2        title screen dividers */
		BLACK*16 + L_WHITE,   /* C_PRIMARY         primary authors */
		BLACK*16 + WHITE      /* C_CONTRIB         contributing authors */
	};
char s_makepar[] =          "makepar";
int lzw[2];

int  process_command(char *, int);

static int init_rseed;
static char initcorners, initparams;

static int  command_file(FILE *, int);
static int  next_command(char *, int, FILE *, char *, int *, int);
static int  next_line(FILE *, char *, int);
static void argerror(const char *);
static void initvars_run(void);
static void initvars_restart(void);
static void initvars_fractal(void);
static void initvars_3d(void);
static void reset_ifs_defn(void);
static void parse_textcolors(char *value);
static int  parse_colors(char *value);
static int  get_bf(bf_t, char *);
static int isabigfloat(char *str);

/*
		cmdfiles(argc, argv) process the command-line arguments
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
	sprintf(string, "%+.1Le", x);
#else
	sprintf(string, "%+.1le", x);
#endif
	p = atoi(string + 5);
	return p;
}



int cmdfiles(int argc, char **argv)
{
	int     i;
	char    curarg[141];
	char    tempstring[101];
	char    *sptr;
	FILE    *initfile;

	if (first_init) /* once per run initialization  */
	{
		initvars_run();
	}
	initvars_restart();                  /* <ins> key initialization */
	initvars_fractal();                  /* image initialization */

	strcpy(curarg, "sstools.ini");
	findpath(curarg, tempstring); /* look for SSTOOLS.INI */
	if (tempstring[0] != 0)              /* found it! */
	{
		initfile = fopen(tempstring, "r");
		if (initfile != NULL)
		{
			command_file(initfile, CMDFILE_SSTOOLS_INI);           /* process it */
		}
	}

	for (i = 1; i < argc; i++)  /* cycle through args */
	{
#ifdef XFRACT
		/* Let the xfract code take a look at the argument */
		if (unixarg(argc, argv, &i))
		{
			continue;
		}
#endif
		strcpy(curarg, argv[i]);
		if (curarg[0] == ';')             /* start of comments? */
		{
			break;
		}
		if (curarg[0] != '@')  /* simple command? */
		{
			if (strchr(curarg, '=') == NULL)  /* not xxx = yyy, so check for gif */
			{
				strcpy(tempstring, curarg);
				if (has_ext(curarg) == NULL)
				{
					strcat(tempstring, ".gif");
				}
				initfile = fopen(tempstring, "rb");
				if (initfile != NULL)
				{
					fread(tempstring, 6, 1, initfile);
					if (tempstring[0] == 'G'
						&& tempstring[1] == 'I'
						&& tempstring[2] == 'F'
						&& tempstring[3] >= '8' && tempstring[3] <= '9'
						&& tempstring[4] >= '0' && tempstring[4] <= '9')
					{
						strcpy(readname, curarg);
						extract_filename(browsename, readname);
						showfile = 0;
						curarg[0] = 0;
					}
					fclose(initfile);
				}
			}
			if (curarg[0])
			{
				process_command(curarg, CMDFILE_AT_CMDLINE);           /* process simple command */
			}
		}
		else
		{
			sptr = strchr(curarg, '/');
			if (sptr != NULL)  /* @filename/setname? */
			{
				*sptr = 0;
				if (merge_pathnames(CommandFile, &curarg[1], 0) < 0)
				{
					init_msg("", CommandFile, 0);
				}
				strcpy(CommandName, sptr + 1);
				if (find_file_item(CommandFile, CommandName, &initfile, ITEMTYPE_PARAMETER) < 0 || initfile == NULL)
				{
					argerror(curarg);
				}
				command_file(initfile, CMDFILE_AT_CMDLINE_SETNAME);
			}
			else  /* @filename */
			{
				initfile = fopen(&curarg[1], "r");
				if (initfile == NULL)
				{
					argerror(curarg);
				}
				command_file(initfile, CMDFILE_AT_CMDLINE);
			}
		}
	}

	if (first_init == 0)
	{
		g_init_mode = -1; /* don't set video when <ins> key used */
		showfile = 1;  /* nor startup image file              */
	}

	init_msg("", NULL, 0);  /* this causes driver_get_key if init_msg called on runup */

	if (debugflag != DEBUGFLAG_NO_FIRST_INIT)
	{
		first_init = 0;
	}
/*
{
				char msg[MSGLEN];
				sprintf(msg, "cmdfiles colorpreloaded %d showfile %d savedac %d",
				colorpreloaded, showfile, savedac);
				stopmsg(0, msg);
			}
*/
	/* PAR reads a file and sets color */
	dontreadcolor = (colorpreloaded && showfile == 0) ? 1 : 0;

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
	ret = command_file(infile, CMDFILE_AT_AFTER_STARTUP);
/*
			{
				char msg[MSGLEN];
				sprintf(msg, "load commands colorpreloaded %d showfile %d savedac %d",
				colorpreloaded, showfile, savedac);
				stopmsg(0, msg);
			}
*/

	/* PAR reads a file and sets color */
	dontreadcolor = (colorpreloaded && showfile == 0) ? 1 : 0;
	return ret;
}


static void initvars_run()              /* once per run init */
{
	char *p;
	init_rseed = (int)time(NULL);
	init_comments();
	p = getenv("TMP");
	if (p == NULL)
	{
		p = getenv("TEMP");
	}
	if (p != NULL)
	{
		if (isadirectory(p) != 0)
		{
			strcpy(tempdir, p);
			fix_dirname(tempdir);
		}
	}
	else
	{
		*tempdir = 0;
	}
}

static void initvars_restart()          /* <ins> key init */
{
	int i;
	g_record_colors = 'a';                  /* don't use mapfiles in PARs */
	save_release = g_release;            /* this release number */
	gif87a_flag = INIT_GIF87;            /* turn on GIF89a processing */
	dither_flag = 0;                     /* no dithering */
	askvideo = 1;                        /* turn on video-prompt flag */
	fract_overwrite = 0;                 /* don't overwrite           */
	soundflag = SOUNDFLAG_SPEAKER | SOUNDFLAG_BEEP; /* sound is on to PC speaker */
	initbatch = INIT_BATCH_NONE;			/* not in batch mode         */
	checkcurdir = 0;                     /* flag to check current dire for files */
	initsavetime = 0;                    /* no auto-save              */
	g_init_mode = -1;                       /* no initial video mode     */
	viewwindow = 0;                      /* no view window            */
	viewreduction = 4.2f;
	viewcrop = 1;
	finalaspectratio = screenaspect;
	viewxdots = viewydots = 0;
	orbit_delay = 0;                     /* full speed orbits */
	g_orbit_interval = 1;                  /* plot all orbits */
	debugflag = DEBUGFLAG_NONE;				/* debugging flag(s) are off */
	timerflag = 0;                       /* timer flags are off       */
	strcpy(FormFileName, "fractint.frm"); /* default formula file      */
	FormName[0] = 0;
	strcpy(LFileName, "fractint.l");
	LName[0] = 0;
	strcpy(CommandFile, "fractint.par");
	CommandName[0] = 0;
	for (i = 0; i < 4; i++)
	{
		CommandComment[i][0] = 0;
	}
	strcpy(IFSFileName, "fractint.ifs");
	IFSName[0] = 0;
	reset_ifs_defn();
	rflag = 0;                           /* not a fixed srand() seed */
	rseed = init_rseed;
	strcpy(readname, DOTSLASH);           /* initially current directory */
	showfile = 1;
	/* next should perhaps be fractal re-init, not just <ins> ? */
	initcyclelimit = 55;                   /* spin-DAC default speed limit */
	mapset = 0;                          /* no map= name active */
	if (mapdacbox)
	{
		free(mapdacbox);
		mapdacbox = NULL;
	}

	g_major_method = breadth_first;        /* default inverse julia methods */
	g_minor_method = left_first;   /* default inverse julia methods */
	truecolor = 0;              /* truecolor output flag */
	truemode = TRUEMODE_DEFAULT;               /* set to default color scheme */
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
	g_stop_pass = 0;                        /* initial guessing g_stop_pass */
	g_quick_calculate = FALSE;
	g_proximity = 0.01;
	g_is_mand = 1;                          /* default formula mand/jul toggle */
#ifndef XFRACT
	usr_floatflag = 0;                   /* turn off the float flag */
#else
	usr_floatflag = 1;                   /* turn on the float flag */
#endif
	finattract = 0;                      /* disable finite attractor logic */
	fractype = 0;                        /* initial type Set flag  */
	curfractalspecific = &fractalspecific[fractype];
	initcorners = initparams = 0;
	bailout = 0;                         /* no user-entered bailout */
	nobof = 0;  /* use normal bof initialization to make bof images */
	useinitorbit = 0;
	for (i = 0; i < MAXPARAMS; i++)
	{
		param[i] = 0.0;     /* initial parameter values */
	}
	for (i = 0; i < 3; i++)
	{
		potparam[i]  = 0.0; /* initial potential values */
		inversion[i] = 0.0;  /* initial invert values */
	}
	initorbit.x = initorbit.y = 0.0;     /* initial orbit values */
	g_invert = 0;
	decomp[0] = decomp[1] = 0;
	usr_distest = 0;
	g_pseudo_x = 0;
	g_pseudo_y = 0;
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
	if (rangeslen)
	{
		free((char *)ranges);
		rangeslen = 0;
	}
	usemag = 1;                          /* use center-mag, not corners */

	colorstate = colorpreloaded = 0;
	rotate_lo = 1; rotate_hi = 255;      /* color cycling default range */
	orbit_delay = 0;                     /* full speed orbits */
	g_orbit_interval = 1;                  /* plot all orbits */
	g_keep_screen_coords = 0;
	g_orbit_draw_mode = ORBITDRAW_RECTANGLE; /* passes=orbits draw mode */
	g_set_orbit_corners = 0;
	g_orbit_x_min = curfractalspecific->xmin;
	g_orbit_x_max = curfractalspecific->xmax;
	g_orbit_x_3rd = curfractalspecific->xmin;
	g_orbit_y_min = curfractalspecific->ymin;
	g_orbit_y_max = curfractalspecific->ymax;
	g_orbit_y_3rd = curfractalspecific->ymin;

	math_tol[0] = 0.05;
	math_tol[1] = 0.05;

	display3d = 0;                       /* 3D display is off        */
	overlay3d = 0;                       /* 3D overlay is off        */

	g_old_demm_colors = 0;
	g_bail_out_test    = Mod;
	g_bail_out_fp  = (int (*)(void))bail_out_mod_fp;
	g_bail_out_l   = (int (*)(void))asmlMODbailout;
	g_bail_out_bn = (int (*)(void))bail_out_mod_bn;
	g_bail_out_bf = (int (*)(void))bail_out_mod_bf;

	functionpreloaded = 0; /* for old bifs  JCO 7/5/92 */
	g_m_x_min_fp = -.83;
	g_m_y_min_fp = -.25;
	g_m_x_max_fp = -.83;
	g_m_y_max_fp =  .25;
	g_origin_fp = 8;
	g_height_fp = 7;
	g_width_fp = 10;
	g_dist_fp = 24;
	g_eyes_fp = 2.5f;
	g_depth_fp = 8;
	g_new_orbit_type = JULIA;
	g_z_dots = 128;
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
	for (i = 0; i <= 11; i++)
	{
		scale_map[i] = i + 1;    /* straight mapping of notes in octave */
	}
#endif
}

static void initvars_3d()               /* init vars affecting 3d */
{
	g_raytrace_output = RAYTRACE_NONE;
	g_raytrace_brief   = 0;
	SPHERE = FALSE;
	g_preview = 0;
	g_show_box = 0;
	g_x_adjust = 0;
	g_y_adjust = 0;
	g_eye_separation = 0;
	g_glasses_type = STEREO_NONE;
	g_preview_factor = 20;
	g_red_crop_left   = 4;
	g_red_crop_right  = 0;
	g_blue_crop_left  = 0;
	g_blue_crop_right = 4;
	g_red_bright     = 80;
	g_blue_bright   = 100;
	transparent[0] = transparent[1] = 0; /* no min/max transparency */
	set_3d_defaults();
}

static void reset_ifs_defn()
{
	if (ifs_defn)
	{
		free((char *)ifs_defn);
		ifs_defn = NULL;
	}
}


static int command_file(FILE *handle, int mode)
	/* mode = 0 command line @filename         */
	/*        1 sstools.ini                    */
	/*        2 <@> command after startup      */
	/*        3 command line @filename/setname */
{
	/* note that command_file could be open as text OR as binary */
	/* binary is used in @ command processing for reasonable speed note/point */
	int i;
	int lineoffset = 0;
	int changeflag = 0; /* &1 fractal stuff chgd, &2 3d stuff chgd */
	char linebuf[513];
	char cmdbuf[10000] = { 0 };

	if (mode == CMDFILE_AT_AFTER_STARTUP || mode == CMDFILE_AT_CMDLINE_SETNAME)
	{
		do
		{
			i = getc(handle);
		}
		while (i != '{' && i != EOF);
		for (i = 0; i < 4; i++)
		{
			CommandComment[i][0] = 0;
		}
	}
	linebuf[0] = 0;
	while (next_command(cmdbuf, 10000, handle, linebuf, &lineoffset, mode) > 0)
	{
		if ((mode == CMDFILE_AT_AFTER_STARTUP || mode == CMDFILE_AT_CMDLINE_SETNAME) && strcmp(cmdbuf, "}") == 0)
		{
			break;
		}
		if ((i = process_command(cmdbuf, mode)) < 0)
		{
			break;
		}
		changeflag |= i;
	}
	fclose(handle);
#ifdef XFRACT
	g_init_mode = 0;                /* Skip credits if @file is used. */
#endif
	if (changeflag & COMMAND_FRACTAL_PARAM)
	{
		backwards_v18();
		backwards_v19();
		backwards_v20();
	}
	return changeflag;
}

static int next_command(char *cmdbuf, int maxlen,
	FILE *handle, char *linebuf, int *lineoffset, int mode)
{
	int i;
	int cmdlen = 0;
	char *lineptr;
	lineptr = linebuf + *lineoffset;
	while (1)
	{
		while (*lineptr <= ' ' || *lineptr == ';')
		{
			if (cmdlen)  /* space or ; marks end of command */
			{
				cmdbuf[cmdlen] = 0;
				*lineoffset = (int) (lineptr - linebuf);
				return cmdlen;
			}
			while (*lineptr && *lineptr <= ' ')
			{
				++lineptr;                  /* skip spaces and tabs */
			}
			if (*lineptr == ';' || *lineptr == 0)
			{
				if (*lineptr == ';'
					&& (mode == CMDFILE_AT_AFTER_STARTUP || mode == CMDFILE_AT_CMDLINE_SETNAME)
					&& (CommandComment[0][0] == 0 || CommandComment[1][0] == 0 ||
						CommandComment[2][0] == 0 || CommandComment[3][0] == 0))
				{
					/* save comment */
					while (*(++lineptr)
						&& (*lineptr == ' ' || *lineptr == '\t'))
					{
					}
					if (*lineptr)
					{
						if ((int)strlen(lineptr) >= MAXCMT)
						{
							*(lineptr + MAXCMT-1) = 0;
						}
						for (i = 0; i < 4; i++)
						{
							if (CommandComment[i][0] == 0)
							{
								strcpy(CommandComment[i], lineptr);
								break;
							}
						}
					}
				}
				if (next_line(handle, linebuf, mode) != 0)
				{
					return -1; /* eof */
				}
				lineptr = linebuf; /* start new line */
			}
		}
		if (*lineptr == '\\'              /* continuation onto next line? */
			&& *(lineptr + 1) == 0)
		{
			if (next_line(handle, linebuf, mode) != 0)
			{
				argerror(cmdbuf);           /* missing continuation */
				return -1;
			}
			lineptr = linebuf;
			while (*lineptr && *lineptr <= ' ')
			{
				++lineptr;                  /* skip white space @ start next line */
			}
			continue;                      /* loop to check end of line again */
		}
		cmdbuf[cmdlen] = *(lineptr++);    /* copy character to command buffer */
		if (++cmdlen >= maxlen)  /* command too long? */
		{
			argerror(cmdbuf);
			return -1;
		}
	}
}

static int next_line(FILE *handle, char *linebuf, int mode)
{
	int toolssection;
	char tmpbuf[11];
	toolssection = 0;
	while (file_gets(linebuf, 512, handle) >= 0)
	{
		if (mode == CMDFILE_SSTOOLS_INI && linebuf[0] == '[')  /* check for [fractint] */
		{
#ifndef XFRACT
			strncpy(tmpbuf, &linebuf[1], 9);
			tmpbuf[9] = 0;
			strlwr(tmpbuf);
			toolssection = strncmp(tmpbuf, "fractint]", 9);
#else
			strncpy(tmpbuf, &linebuf[1], 10);
			tmpbuf[10] = 0;
			strlwr(tmpbuf);
			toolssection = strncmp(tmpbuf, "xfractint]", 10);
#endif
			continue;                              /* skip tools section heading */
		}
		if (toolssection == 0)
		{
			return 0;
		}
	}
	return -1;
}

static int badarg(const char *curarg)
{
	argerror(curarg);
	return COMMAND_ERROR;
}

struct tag_cmd_context
{
	const char *curarg;
	int     yesnoval[16];                /* 0 if 'n', 1 if 'y', -1 if not */
	int     numval;                      /* numeric value of arg      */
	char    *value;                      /* pointer to variable value */
	char    charval[16];                 /* first character of arg    */
	int     totparms;                    /* # of / delimited parms    */
	int     valuelen;                    /* length of value           */
	int mode;
	const char *variable;
	int     intval[64];                  /* pre-parsed integer parms  */
	double  floatval[16];                /* pre-parsed floating parms */
	char    *floatvalstr[16];            /* pointers to float vals */
	int     intparms;                    /* # of / delimited ints     */
	int     floatparms;                  /* # of / delimited floats   */
};
typedef struct tag_cmd_context cmd_context;

struct tag_named_int
{
	const char *name;
	int value;
};
typedef struct tag_named_int named_int;

static int named_value(const named_int *args, int num_args, const char *name, int *value)
{
	int ii;
	for (ii = 0; ii < NUM_OF(args); ii++)
	{
		if (strcmp(name, args[ii].name) == 0)
		{
			*value = args[ii].value;
			return TRUE;
		}
	}

	return FALSE;
}

static int batch_arg(const cmd_context *context)
{
	if (context->yesnoval[0] < 0)
	{
		return badarg(context->curarg);
	}
#ifdef XFRACT
	g_init_mode = context->yesnoval[0] ? 0 : -1; /* skip credits for batch mode */
#endif
	initbatch = context->yesnoval[0];
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int max_history_arg(const cmd_context *context)
{
	if (context->numval == NON_NUMERIC)
	{
		return badarg(context->curarg);
	}
	else if (context->numval < 0 /* || context->numval > 1000 */)
	{
		return badarg(context->curarg);
	}
	else
	{
		maxhistory = context->numval;
	}
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int adapter_arg(const cmd_context *context)
{
	/* adapter parameter no longer used; check for bad argument anyway */
	named_int args[] =
	{
		{ "egamono", -1 },
		{ "hgc", -1 },
		{ "ega", -1 },
		{ "cga", -1 },
		{ "mcga", -1 },
		{ "vga", -1 }
	};
	int adapter = 0;
	if (named_value(args, NUM_OF(args), context->value, &adapter))
	{
		assert(adapter == -1);
		return badarg(context->curarg);
	}

	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int text_safe_arg(const cmd_context *context)
{
	/* textsafe no longer used, do validity checking, but gobble argument */
	if (first_init)
	{
		if (!((context->charval[0] == 'n')	/* no */
				|| (context->charval[0] == 'y')	/* yes */
				|| (context->charval[0] == 'b')	/* bios */
				|| (context->charval[0] == 's'))) /* save */
		{
			return badarg(context->curarg);
		}
	}
	return COMMAND_OK;
}

static int gobble_flag_arg(const cmd_context *context)
{
	if (context->yesnoval[0] < 0)
	{
		return badarg(context->curarg);
	}
	return COMMAND_OK;
}

static int flag_arg(const cmd_context *context, int *flag, int result)
{
	if (context->yesnoval[0] < 0)
	{
		return badarg(context->curarg);
	}
	*flag = context->yesnoval[0];
	return result;
}

static int exit_no_ask_arg(const cmd_context *context)
{
	return flag_arg(context, &escape_exit, COMMAND_FRACTAL_PARAM	| COMMAND_3D_PARAM);
}

static int fpu_arg(const cmd_context *context)
{
	if (strcmp(context->value, "387") == 0)
	{
#ifndef XFRACT
		fpu = 387;
#else
		fpu = -1;
#endif
		return COMMAND_OK;
	}
	return badarg(context->curarg);
}

static int make_doc_arg(const cmd_context *context)
{
	print_document(context->value ? context->value : "fractint.doc", makedoc_msg_func, 0);
#ifndef WINFRACT
	goodbye();
#endif
	return 0;
}

static int make_par_arg(const cmd_context *context)
{
	char *slash, *next = NULL;
	if (context->totparms < 1 || context->totparms > 2)
	{
		return badarg(context->curarg);
	}
	slash = strchr(context->value, '/');
	if (slash != NULL)
	{
		*slash = 0;
		next = slash + 1;
	}

	strcpy(CommandFile, context->value);
	if (strchr(CommandFile, '.') == NULL)
	{
		strcat(CommandFile, ".par");
	}
	if (strcmp(readname, DOTSLASH) == 0)
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
			return badarg(context->curarg);
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
	calculate_fractal_initialize();
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
	return 0;
}

static int reset_arg(const cmd_context *context)
{
	initvars_fractal();

	/* PAR release unknown unless specified */
	if (context->numval >= 0)
	{
		save_release = context->numval;
	}
	else
	{
		return badarg(context->curarg);
	}
	if (save_release == 0)
	{
		save_release = 1730; /* before start of lyapunov wierdness */
	}
	return COMMAND_RESET | COMMAND_FRACTAL_PARAM;
}

static int filename_arg(const cmd_context *context)
{
	int existdir;
	if (context->charval[0] == '.' && context->value[1] != SLASHC)
	{
		if (context->valuelen > 4)
		{
			return badarg(context->curarg);
		}
		gifmask[0] = '*';
		gifmask[1] = 0;
		strcat(gifmask, context->value);
		return COMMAND_OK;
	}
	if (context->valuelen > (FILE_MAX_PATH-1))
	{
		return badarg(context->curarg);
	}
	if (context->mode == CMDFILE_AT_AFTER_STARTUP && display3d == 0) /* can't do this in @ command */
	{
		return badarg(context->curarg);
	}

	existdir = merge_pathnames(readname, context->value, context->mode);
	if (existdir == 0)
	{
		showfile = 0;
	}
	else if (existdir < 0)
	{
		init_msg(context->variable, context->value, context->mode);
	}
	else
	{
		extract_filename(browsename, readname);
	}
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int video_arg(const cmd_context *context)
{
	int k = check_vidmode_keyname(context->value);
	int i;

	if (k == 0)
	{
		return badarg(context->curarg);
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
		return badarg(context->curarg);
	}
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int map_arg(const cmd_context *context)
{
	int existdir;
	if (context->valuelen > (FILE_MAX_PATH-1))
	{
		return badarg(context->curarg);
	}
	existdir = merge_pathnames(MAP_name, context->value, context->mode);
	if (existdir > 0)
	{
		return COMMAND_OK;    /* got a directory */
	}
	else if (existdir < 0)
	{
		init_msg(context->variable, context->value, context->mode);
		return COMMAND_OK;
	}
	SetColorPaletteName(MAP_name);
	return COMMAND_OK;
}

static int colors_arg(const cmd_context *context)
{
	if (parse_colors(context->value) < 0)
	{
		return badarg(context->curarg);
	}
	return COMMAND_OK;
}

static int record_colors_arg(const cmd_context *context)
{
	if (*context->value != 'y' && *context->value != 'c' && *context->value != 'a')
	{
		return badarg(context->curarg);
	}
	g_record_colors = *context->value;
	return COMMAND_OK;
}

static int max_line_length_arg(const cmd_context *context)
{
	if (context->numval < MINMAXLINELENGTH || context->numval > MAXMAXLINELENGTH)
	{
		return badarg(context->curarg);
	}
	maxlinelength = context->numval;
	return COMMAND_OK;
}

static int parse_arg(const cmd_context *context)
{
	parse_comments(context->value);
	return COMMAND_OK;
}

/* maxcolorres no longer used, validate value and gobble argument */
static int max_color_res_arg(const cmd_context *context)
{
	if (context->numval == 1
		|| context->numval == 4
		|| context->numval == 8
		|| context->numval == 16
		|| context->numval == 24)
	{
		return COMMAND_OK;
	}
	return badarg(context->curarg);
}

/* pixelzoom no longer used, validate value and gobble argument */
static int pixel_zoom_arg(const cmd_context *context)
{
	if (context->numval >= 5)
	{
		return badarg(context->curarg);
	}
	return COMMAND_OK;
}

/* keep this for backward compatibility */
static int warn_arg(const cmd_context *context)
{
	if (context->yesnoval[0] < 0)
	{
		return badarg(context->curarg);
	}
	fract_overwrite = (char) (context->yesnoval[0] ^ 1);
	return COMMAND_OK;
}

static int overwrite_arg(const cmd_context *context)
{
	if (context->yesnoval[0] < 0)
	{
		return badarg(context->curarg);
	}
	fract_overwrite = (char) context->yesnoval[0];
	return COMMAND_OK;
}

static int save_time_arg(const cmd_context *context)
{
	initsavetime = context->numval;
	return COMMAND_OK;
}

static int auto_key_arg(const cmd_context *context)
{
	named_int args[] =
	{
		{ "record", SLIDES_RECORD },
		{ "play", SLIDES_PLAY }
	};
	return named_value(args, NUM_OF(args), context->value, &g_slides)
		? COMMAND_OK : badarg(context->curarg);
}

static int auto_key_name_arg(const cmd_context *context)
{
	if (merge_pathnames(autoname, context->value, context->mode) < 0)
	{
		init_msg(context->variable, context->value, context->mode);
	}
	return COMMAND_OK;
}

static int type_arg(const cmd_context *context)
{
	char value[256];
	int valuelen = context->valuelen;
	int k;

	strcpy(value, context->value);
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
		return badarg(context->curarg);
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
	return COMMAND_FRACTAL_PARAM;
}

static int inside_arg(const cmd_context *context)
{
	named_int args[] =
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
	if (named_value(args, NUM_OF(args), context->value, &inside))
	{
		return COMMAND_FRACTAL_PARAM;
	}
	if (context->numval == NON_NUMERIC)
	{
		return badarg(context->curarg);
	}
	else
	{
		inside = context->numval;
	}
	return COMMAND_FRACTAL_PARAM;
}

static int proximity_arg(const cmd_context *context)
{
	g_proximity = context->floatval[0];
	return COMMAND_FRACTAL_PARAM;
}

static int fill_color_arg(const cmd_context *context)
{
	if (strcmp(context->value, "normal") == 0)
	{
		fillcolor = -1;
	}
	else if (context->numval == NON_NUMERIC)
	{
		return badarg(context->curarg);
	}
	else
	{
		fillcolor = context->numval;
	}
	return COMMAND_FRACTAL_PARAM;
}

static int function_arg(const cmd_context *context)
{
	int k = 0;
	const char *value = context->value;

	while (*value && k < 4)
	{
		if (set_trig_array(k++, value))
		{
			return badarg(context->curarg);
		}
		value = strchr(value, '/');
		if (value == NULL)
		{
			break;
		}
		++value;
	}
	functionpreloaded = 1; /* for old bifs  JCO 7/5/92 */
	return COMMAND_FRACTAL_PARAM;
}

static int outside_arg(const cmd_context *context)
{
	named_int args[] =
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
	if (named_value(args, NUM_OF(args), context->value, &outside))
	{
		return COMMAND_FRACTAL_PARAM;
	}
	if ((context->numval == NON_NUMERIC) || (context->numval < TDIS || context->numval > 255))
	{
		return badarg(context->curarg);
	}
	outside = context->numval;
	return COMMAND_FRACTAL_PARAM;
}

static int bf_digits_arg(const cmd_context *context)
{
	if ((context->numval == NON_NUMERIC) || (context->numval < 0 || context->numval > 2000))
	{
		return badarg(context->curarg);
	}
	g_bf_digits = context->numval;
	return COMMAND_FRACTAL_PARAM;
}

static int max_iter_arg(const cmd_context *context)
{
	if (context->floatval[0] < 2)
	{
		return badarg(context->curarg);
	}
	maxit = (long) context->floatval[0];
	return COMMAND_FRACTAL_PARAM;
}

static int passes_arg(const cmd_context *context)
{
	if (context->charval[0] != '1' && context->charval[0] != '2' && context->charval[0] != '3'
		&& context->charval[0] != 'g' && context->charval[0] != 'b'
		&& context->charval[0] != 't' && context->charval[0] != 's'
		&& context->charval[0] != 'd' && context->charval[0] != 'o')
	{
		return badarg(context->curarg);
	}
	usr_stdcalcmode = context->charval[0];
	if (context->charval[0] == 'g')
	{
		g_stop_pass = ((int)context->value[1] - (int)'0');
		if (g_stop_pass < 0 || g_stop_pass > 6)
		{
			g_stop_pass = 0;
		}
	}
	return COMMAND_FRACTAL_PARAM;
}

static int cycle_limit_arg(const cmd_context *context)
{
	if (context->numval <= 1 || context->numval > 256)
	{
		return badarg(context->curarg);
	}
	initcyclelimit = context->numval;
	return COMMAND_OK;
}

static int make_mig_arg(const cmd_context *context)
{
	int xmult, ymult;
	if (context->totparms < 2)
	{
		return badarg(context->curarg);
	}
	xmult = context->intval[0];
	ymult = context->intval[1];
	make_mig(xmult, ymult);
#ifndef WINFRACT
	exit(0);
#endif
	return COMMAND_OK;
}

static int cycle_range_arg(const cmd_context *context)
{
	int lo;
	int hi;
	if (context->totparms < 2)
	{
		hi = 255;
	}
	else
	{
		hi = context->intval[1];
	}
	if (context->totparms < 1)
	{
		lo = 1;
	}
	else
	{
		lo = context->intval[0];
	}
	if (context->totparms != context->intparms
		|| lo < 0 || hi > 255 || lo > hi)
	{
		return badarg(context->curarg);
	}
	rotate_lo = lo;
	rotate_hi = hi;
	return COMMAND_OK;
}

static int ranges_arg(const cmd_context *context)
{
	int i, j, entries, prev;
	int tmpranges[128];

	if (context->totparms != context->intparms)
	{
		return badarg(context->curarg);
	}
	entries = prev = i = 0;
	LogFlag = 0; /* ranges overrides logmap */
	while (i < context->totparms)
	{
		if ((j = context->intval[i++]) < 0) /* striping */
		{
			j = -j;
			if (j < 1 || j >= 16384 || i >= context->totparms)
			{
				return badarg(context->curarg);
			}
			tmpranges[entries++] = -1; /* {-1,width,limit} for striping */
			tmpranges[entries++] = j;
			j = context->intval[i++];
		}
		if (j < prev)
		{
			return badarg(context->curarg);
		}
		tmpranges[entries++] = prev = j;
	}
	if (prev == 0)
	{
		return badarg(context->curarg);
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
	return COMMAND_FRACTAL_PARAM;
}

static int save_name_arg(const cmd_context *context)
{
	if (context->valuelen > (FILE_MAX_PATH-1))
	{
		return badarg(context->curarg);
	}
	if (first_init || context->mode == CMDFILE_AT_AFTER_STARTUP)
	{
		if (merge_pathnames(savename, context->value, context->mode) < 0)
		{
			init_msg(context->variable, context->value, context->mode);
		}
	}
	return COMMAND_OK;
}

static int tweak_lzw_arg(const cmd_context *context)
{
	if (context->totparms >= 1)
	{
		lzw[0] = context->intval[0];
	}
	if (context->totparms >= 2)
	{
		lzw[1] = context->intval[1];
	}
	return COMMAND_OK;
}

static int min_stack_arg(const cmd_context *context)
{
	if (context->totparms != 1)
	{
		return badarg(context->curarg);
	}
	minstack = context->intval[0];
	return COMMAND_OK;
}

static int math_tolerance_arg(const cmd_context *context)
{
	if (context->charval[0] == '/')
	{
		; /* leave math_tol[0] at the default value */
	}
	else if (context->totparms >= 1)
	{
		math_tol[0] = context->floatval[0];
	}
	if (context->totparms >= 2)
	{
		math_tol[1] = context->floatval[1];
	}
	return COMMAND_OK;
}

static int temp_dir_arg(const cmd_context *context)
{
	if (context->valuelen > (FILE_MAX_DIR-1))
	{
		return badarg(context->curarg);
	}
	if (isadirectory(context->value) == 0)
	{
		return badarg(context->curarg);
	}
	strcpy(tempdir, context->value);
	fix_dirname(tempdir);
	return COMMAND_OK;
}

static int work_dir_arg(const cmd_context *context)
{
	if (context->valuelen > (FILE_MAX_DIR-1))
	{
		return badarg(context->curarg);
	}
	if (isadirectory(context->value) == 0)
	{
		return badarg(context->curarg);
	}
	strcpy(workdir, context->value);
	fix_dirname(workdir);
	return COMMAND_OK;
}

static int exit_mode_arg(const cmd_context *context)
{
	sscanf(context->value, "%x", &context->numval);
	exitmode = (BYTE)context->numval;
	return COMMAND_OK;
}

static int text_colors_arg(const cmd_context *context)
{
	parse_textcolors(context->value);
	return COMMAND_OK;
}

static int potential_arg(const cmd_context *context)
{
	int k = 0;
	char *value = context->value;
	while (k < 3 && *value)
	{
		potparam[k] = (k == 1) ? atof(value) : atoi(value);
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
			return badarg(context->curarg);
		}
		pot16bit = 1;
	}
	return COMMAND_FRACTAL_PARAM;
}

static int params_arg(const cmd_context *context)
{
	int k;

	if (context->totparms != context->floatparms || context->totparms > MAXPARAMS)
	{
		return badarg(context->curarg);
	}
	initparams = 1;
	for (k = 0; k < MAXPARAMS; ++k)
	{
		param[k] = (k < context->totparms) ? context->floatval[k] : 0.0;
	}
	if (bf_math)
	{
		for (k = 0; k < MAXPARAMS; k++)
		{
			floattobf(bfparms[k], param[k]);
		}
	}
	return COMMAND_FRACTAL_PARAM;
}

static int miim_arg(const cmd_context *context)
{
	if (context->totparms > 6)
	{
		return badarg(context->curarg);
	}
	if (context->charval[0] == 'b')
	{
		g_major_method = breadth_first;
	}
	else if (context->charval[0] == 'd')
	{
		g_major_method = depth_first;
	}
	else if (context->charval[0] == 'w')
	{
		g_major_method = random_walk;
	}
#ifdef RANDOM_RUN
	else if (context->charval[0] == 'r')
	{
		g_major_method = random_run;
	}
#endif
	else
	{
		return badarg(context->curarg);
	}

	if (context->charval[1] == 'l')
	{
		g_minor_method = left_first;
	}
	else if (context->charval[1] == 'r')
	{
		g_minor_method = right_first;
	}
	else
	{
		return badarg(context->curarg);
	}

	/* keep this next part in for backwards compatibility with old PARs ??? */

	if (context->totparms > 2)
	{
		int k;
		for (k = 2; k < 6; ++k)
		{
			param[k-2] = (k < context->totparms) ? context->floatval[k] : 0.0;
		}
	}

	return COMMAND_FRACTAL_PARAM;
}

static int init_orbit_arg(const cmd_context *context)
{
	if (strcmp(context->value, "pixel") == 0)
	{
		useinitorbit = 2;
	}
	else
	{
		if (context->totparms != 2 || context->floatparms != 2)
		{
			return badarg(context->curarg);
		}
		initorbit.x = context->floatval[0];
		initorbit.y = context->floatval[1];
		useinitorbit = 1;
	}
	return COMMAND_FRACTAL_PARAM;
}

static int orbit_name_arg(const cmd_context *context)
{
	if (check_orbit_name(context->value))
	{
		return badarg(context->curarg);
	}
	return COMMAND_FRACTAL_PARAM;
}

static int threed_mode_arg(const cmd_context *context)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		if (strcmp(context->value, juli3Doptions[i]) == 0)
		{
			g_juli_3D_mode = i;
			return COMMAND_FRACTAL_PARAM;
		}
	}
	return badarg(context->curarg);
}

static int julibrot_3d_arg(const cmd_context *context)
{
	if (context->floatparms != context->totparms)
	{
		return badarg(context->curarg);
	}
	if (context->totparms > 0)
	{
		g_z_dots = (int)context->floatval[0];
	}
	if (context->totparms > 1)
	{
		g_origin_fp = (float)context->floatval[1];
	}
	if (context->totparms > 2)
	{
		g_depth_fp = (float)context->floatval[2];
	}
	if (context->totparms > 3)
	{
		g_height_fp = (float)context->floatval[3];
	}
	if (context->totparms > 4)
	{
		g_width_fp = (float)context->floatval[4];
	}
	if (context->totparms > 5)
	{
		g_dist_fp = (float)context->floatval[5];
	}
	return COMMAND_FRACTAL_PARAM;
}

static int julibrot_eyes_arg(const cmd_context *context)
{
	if (context->floatparms != context->totparms || context->totparms != 1)
	{
		return badarg(context->curarg);
	}
	g_eyes_fp =  (float)context->floatval[0];
	return COMMAND_FRACTAL_PARAM;
}

static int julibrot_from_to_arg(const cmd_context *context)
{
	if (context->floatparms != context->totparms || context->totparms != 4)
	{
		return badarg(context->curarg);
	}
	g_m_x_max_fp = context->floatval[0];
	g_m_x_min_fp = context->floatval[1];
	g_m_y_max_fp = context->floatval[2];
	g_m_y_min_fp = context->floatval[3];
	return COMMAND_FRACTAL_PARAM;
}

static int corners_arg(const cmd_context *context)
{
	int dec;
	if (fractype == CELLULAR)
	{
		return COMMAND_FRACTAL_PARAM; /* skip setting the corners */
	}
#if 0
	/* use a debugger and OutputDebugString instead of standard I/O on Windows */
	printf("totparms %d floatparms %d\n", totparms, context->floatparms);
	getch();
#endif
	if (context->floatparms != context->totparms
		|| (context->totparms != 0 && context->totparms != 4 && context->totparms != 6))
	{
		return badarg(context->curarg);
	}
	usemag = 0;
	if (context->totparms == 0)
	{
		return COMMAND_OK; /* turns corners mode on */
	}
	initcorners = 1;
	/* good first approx, but dec could be too big */
	dec = get_max_curarg_len((char **) context->floatvalstr, context->totparms) + 1;
	if ((dec > DBL_DIG + 1 || DEBUGFLAG_NO_BIG_TO_FLOAT == debugflag) && debugflag != DEBUGFLAG_NO_INT_TO_FLOAT)
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
		get_bf(bfxmin, context->floatvalstr[0]);
		get_bf(bfx3rd, context->floatvalstr[0]);

		/* xxmax = floatval[1]; */
		get_bf(bfxmax, context->floatvalstr[1]);

		/* yy3rd = yymin = floatval[2]; */
		get_bf(bfymin, context->floatvalstr[2]);
		get_bf(bfy3rd, context->floatvalstr[2]);

		/* yymax = floatval[3]; */
		get_bf(bfymax, context->floatvalstr[3]);

		if (context->totparms == 6)
		{
			/* xx3rd = floatval[4]; */
			get_bf(bfx3rd, context->floatvalstr[4]);

			/* yy3rd = floatval[5]; */
			get_bf(bfy3rd, context->floatvalstr[5]);
		}

		/* now that all the corners have been read in, get a more */
		/* accurate value for dec and do it all again             */

		dec = getprecbf_mag();
		if (dec < 0)
		{
			return badarg(context->curarg);     /* ie: Magnification is +-1.#INF */
		}

		if (dec > decimals)  /* get corners again if need more precision */
		{
			int k;

			init_bf_dec(dec);

			/* now get parameters and corners all over again at new
			decimal setting */
			for (k = 0; k < MAXPARAMS; k++)
			{
				floattobf(bfparms[k], param[k]);
			}

			/* xx3rd = xxmin = floatval[0]; */
			get_bf(bfxmin, context->floatvalstr[0]);
			get_bf(bfx3rd, context->floatvalstr[0]);

			/* xxmax = floatval[1]; */
			get_bf(bfxmax, context->floatvalstr[1]);

			/* yy3rd = yymin = floatval[2]; */
			get_bf(bfymin, context->floatvalstr[2]);
			get_bf(bfy3rd, context->floatvalstr[2]);

			/* yymax = floatval[3]; */
			get_bf(bfymax, context->floatvalstr[3]);

			if (context->totparms == 6)
			{
				/* xx3rd = floatval[4]; */
				get_bf(bfx3rd, context->floatvalstr[4]);

				/* yy3rd = floatval[5]; */
				get_bf(bfy3rd, context->floatvalstr[5]);
			}
		}
	}
	xx3rd = xxmin = context->floatval[0];
	xxmax =         context->floatval[1];
	yy3rd = yymin = context->floatval[2];
	yymax =         context->floatval[3];

	if (context->totparms == 6)
	{
		xx3rd =      context->floatval[4];
		yy3rd =      context->floatval[5];
	}
	return COMMAND_FRACTAL_PARAM;
}

static int orbit_corners_arg(const cmd_context *context)
{
	g_set_orbit_corners = 0;
	if (context->floatparms != context->totparms
		|| (context->totparms != 0 && context->totparms != 4 && context->totparms != 6))
	{
		return badarg(context->curarg);
	}
	g_orbit_x_3rd = g_orbit_x_min = context->floatval[0];
	g_orbit_x_max =         context->floatval[1];
	g_orbit_y_3rd = g_orbit_y_min = context->floatval[2];
	g_orbit_y_max =         context->floatval[3];

	if (context->totparms == 6)
	{
		g_orbit_x_3rd =      context->floatval[4];
		g_orbit_y_3rd =      context->floatval[5];
	}
	g_set_orbit_corners = 1;
	g_keep_screen_coords = 1;
	return COMMAND_FRACTAL_PARAM;
}

static int orbit_draw_mode_arg(const cmd_context *context)
{
	switch (context->charval[0])
	{
	case 'l': g_orbit_draw_mode = ORBITDRAW_LINE;		break;
	case 'r': g_orbit_draw_mode = ORBITDRAW_RECTANGLE;	break;
	case 'f': g_orbit_draw_mode = ORBITDRAW_FUNCTION;	break;

	default:
		return badarg(context->curarg);
	}
	return COMMAND_FRACTAL_PARAM;
}

static int view_windows_arg(const cmd_context *context)
{
	if (context->totparms > 5 || context->floatparms-context->intparms > 2 || context->intparms > 4)
	{
		return badarg(context->curarg);
	}
	viewwindow = 1;
	viewreduction = 4.2f;  /* reset default values */
	finalaspectratio = screenaspect;
	viewcrop = 1; /* yes */
	viewxdots = viewydots = 0;

	if ((context->totparms > 0) && (context->floatval[0] > 0.001))
	{
		viewreduction = (float)context->floatval[0];
	}
	if ((context->totparms > 1) && (context->floatval[1] > 0.001))
	{
		finalaspectratio = (float)context->floatval[1];
	}
	if ((context->totparms > 2) && (context->yesnoval[2] == 0))
	{
		viewcrop = context->yesnoval[2];
	}
	if ((context->totparms > 3) && (context->intval[3] > 0))
	{
		viewxdots = context->intval[3];
	}
	if ((context->totparms == 5) && (context->intval[4] > 0))
	{
		viewydots = context->intval[4];
	}
	return COMMAND_FRACTAL_PARAM;
}

static int center_mag_arg(const cmd_context *context)
{
	int dec;
	double Xctr, Yctr, Xmagfactor, Rotation, Skew;
	LDBL Magnification;
	bf_t bXctr, bYctr;

	if ((context->totparms != context->floatparms)
		|| (context->totparms != 0 && context->totparms < 3)
		|| (context->totparms >= 3 && context->floatval[2] == 0.0))
	{
		return badarg(context->curarg);
	}
	if (fractype == CELLULAR)
	{
		return COMMAND_FRACTAL_PARAM; /* skip setting the corners */
	}
	usemag = 1;
	if (context->totparms == 0)
	{
		return COMMAND_OK; /* turns center-mag mode on */
	}
	initcorners = 1;
	/* dec = get_max_curarg_len(floatvalstr, context->totparms); */
#ifdef USE_LONG_DOUBLE
	sscanf(context->floatvalstr[2], "%Lf", &Magnification);
#else
	sscanf(context->floatvalstr[2], "%lf", &Magnification);
#endif

	/* I don't know if this is portable, but something needs to */
	/* be used in case compiler's LDBL_MAX is not big enough    */
	if (Magnification > LDBL_MAX || Magnification < -LDBL_MAX)
	{
		return badarg(context->curarg);     /* ie: Magnification is +-1.#INF */
	}

	dec = getpower10(Magnification) + 4; /* 4 digits of padding sounds good */

	if ((dec <= DBL_DIG + 1 && debugflag != DEBUGFLAG_NO_BIG_TO_FLOAT) || DEBUGFLAG_NO_INT_TO_FLOAT == debugflag)  /* rough estimate that double is OK */
	{
		Xctr = context->floatval[0];
		Yctr = context->floatval[1];
		/* Magnification = context->floatval[2]; */  /* already done above */
		Xmagfactor = 1;
		Rotation = 0;
		Skew = 0;
		if (context->floatparms > 3)
		{
			Xmagfactor = context->floatval[3];
		}
		if (Xmagfactor == 0)
		{
			Xmagfactor = 1;
		}
		if (context->floatparms > 4)
		{
			Rotation = context->floatval[4];
		}
		if (context->floatparms > 5)
		{
			Skew = context->floatval[5];
		}
		/* calculate bounds */
		cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
		return COMMAND_FRACTAL_PARAM;
	}
	else  /* use arbitrary precision */
	{
		int old_bf_math;
		int saved;
		initcorners = 1;
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
		usemag = 1;
		saved = save_stack();
		bXctr            = alloc_stack(bflength + 2);
		bYctr            = alloc_stack(bflength + 2);
		/* Xctr = context->floatval[0]; */
		get_bf(bXctr, context->floatvalstr[0]);
		/* Yctr = context->floatval[1]; */
		get_bf(bYctr, context->floatvalstr[1]);
		/* Magnification = context->floatval[2]; */  /* already done above */
		Xmagfactor = 1;
		Rotation = 0;
		Skew = 0;
		if (context->floatparms > 3)
		{
			Xmagfactor = context->floatval[3];
		}
		if (Xmagfactor == 0)
		{
			Xmagfactor = 1;
		}
		if (context->floatparms > 4)
		{
			Rotation = context->floatval[4];
		}
		if (context->floatparms > 5)
		{
			Skew = context->floatval[5];
		}
		/* calculate bounds */
		cvtcornersbf(bXctr, bYctr, Magnification, Xmagfactor, Rotation, Skew);
		corners_bf_to_float();
		restore_stack(saved);
		return COMMAND_FRACTAL_PARAM;
	}
}

static int aspect_drift_arg(const cmd_context *context)
{
	if (context->floatparms != 1 || context->floatval[0] < 0)
	{
		return badarg(context->curarg);
	}
	aspectdrift = (float)context->floatval[0];
	return COMMAND_FRACTAL_PARAM;
}

static int invert_arg(const cmd_context *context)
{
	if (context->totparms != context->floatparms || (context->totparms != 1 && context->totparms != 3))
	{
		return badarg(context->curarg);
	}
	inversion[0] = context->floatval[0];
	g_invert = (inversion[0] != 0.0) ? context->totparms : 0;
	if (context->totparms == 3)
	{
		inversion[1] = context->floatval[1];
		inversion[2] = context->floatval[2];
	}
	return COMMAND_FRACTAL_PARAM;
}

static int ignore_arg(const cmd_context *context)
{
	return COMMAND_OK; /* just ignore and return, for old time's sake */
}

static int float_arg(const cmd_context *context)
{
	if (context->yesnoval[0] < 0)
	{
		return badarg(context->curarg);
	}
#ifndef XFRACT
	usr_floatflag = (char)context->yesnoval[0];
#else
	usr_floatflag = 1; /* must use floating point */
#endif
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int fast_restore_arg(const cmd_context *context)
{
	if (context->yesnoval[0] < 0)
	{
		return badarg(context->curarg);
	}
	fastrestore = (char)context->yesnoval[0];
	return COMMAND_OK;
}

static int org_frm_dir_arg(const cmd_context *context)
{
	if ((context->valuelen > (FILE_MAX_DIR-1))
		|| (isadirectory(context->value) == 0))
	{
		return badarg(context->curarg);
	}
	orgfrmsearch = 1;
	strcpy(orgfrmdir, context->value);
	fix_dirname(orgfrmdir);
	return COMMAND_OK;
}

static int biomorph_arg(const cmd_context *context)
{
	usr_biomorph = context->numval;
	return COMMAND_FRACTAL_PARAM;
}

static int orbit_save_arg(const cmd_context *context)
{
	if (context->charval[0] == 's')
	{
		orbitsave |= ORBITSAVE_SOUND;
	}
	else if (context->yesnoval[0] < 0)
	{
		return badarg(context->curarg);
	}
	orbitsave |= context->yesnoval[0];
	return COMMAND_FRACTAL_PARAM;
}

static int bail_out_arg(const cmd_context *context)
{
	if (context->floatval[0] < 1 || context->floatval[0] > 2100000000L)
	{
		return badarg(context->curarg);
	}
	bailout = (long)context->floatval[0];
	return COMMAND_FRACTAL_PARAM;
}

static int bail_out_test_arg(const cmd_context *context)
{
	named_int args[] =
	{
		{ "mod", Mod },
		{ "real", Real },
		{ "imag", Imag },
		{ "or", Or },
		{ "and", And },
		{ "manh", Manh },
		{ "manr", Manr }
	};
	int value;
	if (named_value(args, NUM_OF(args), context->value, &value))
	{
		g_bail_out_test = (enum bailouts) value;
		setbailoutformula(g_bail_out_test);
		return COMMAND_FRACTAL_PARAM;
	}

	return badarg(context->curarg);
}

static int symmetry_arg(const cmd_context *context)
{
	named_int args[] =
	{
		{ "xaxis", XAXIS },
		{ "yaxis", YAXIS },
		{ "xyaxis", XYAXIS },
		{ "origin", ORIGIN },
		{ "pi", PI_SYM },
		{ "none", NOSYM }
	};
	if (named_value(args, NUM_OF(args), context->value, &forcesymmetry))
	{
		return COMMAND_FRACTAL_PARAM;
	}
	return badarg(context->curarg);
}

static int sound_arg(const cmd_context *context)
{
	if (context->totparms > 5)
	{
		return badarg(context->curarg);
	}
	soundflag = SOUNDFLAG_OFF; /* start with a clean slate, add bits as we go */
	if (context->totparms == 1)
	{
		soundflag = SOUNDFLAG_SPEAKER; /* old command, default to PC speaker */
	}

	/* soundflag is used as a bitfield... bit 0, 1, 2 used for whether sound
		is modified by an orbits x, y, or z component. and also to turn it on
		or off (0==off, 1==beep (or yes), 2==x, 3 == y, 4 == z),
		Bit 3 is used for flagging the PC speaker sound,
		Bit 4 for OPL3 FM soundcard output,
		Bit 5 will be for midi output (not yet),
		Bit 6 for whether the tone is quantised to the nearest 'proper' note
	(according to the western, even tempered system anyway) */

	if (context->charval[0] == 'n' || context->charval[0] == 'o')
	{
		soundflag &= ~SOUNDFLAG_ORBITMASK;
	}
	else if ((strncmp(context->value, "ye", 2) == 0) || (context->charval[0] == 'b'))
	{
		soundflag |= SOUNDFLAG_BEEP;
	}
	else if (context->charval[0] == 'x')
	{
		soundflag |= SOUNDFLAG_X;
	}
	else if (context->charval[0] == 'y' && strncmp(context->value, "ye", 2) != 0)
	{
		soundflag |= SOUNDFLAG_Y;
	}
	else if (context->charval[0] == 'z')
	{
		soundflag |= SOUNDFLAG_Z;
	}
	else
	{
		return badarg(context->curarg);
	}
#if !defined(XFRACT)
	if (context->totparms > 1)
	{
		int i;
		soundflag &= SOUNDFLAG_ORBITMASK; /* reset options */
		for (i = 1; i < context->totparms; i++)
		{
			/* this is for 2 or more options at the same time */
			if (context->charval[i] == 'f')  /* (try to)switch on opl3 fm synth */
			{
				if (driver_init_fm())
				{
					soundflag |= SOUNDFLAG_OPL3_FM;
				}
				else
				{
					soundflag &= ~SOUNDFLAG_OPL3_FM;
				}
			}
			else if (context->charval[i] == 'p')
			{
				soundflag |= SOUNDFLAG_SPEAKER;
			}
			else if (context->charval[i] == 'm')
			{
				soundflag |= SOUNDFLAG_MIDI;
			}
			else if (context->charval[i] == 'q')
			{
				soundflag |= SOUNDFLAG_QUANTIZED;
			}
			else
			{
				return badarg(context->curarg);
			}
		} /* end for */
	}    /* end context->totparms > 1 */
#endif
	return COMMAND_OK;
}

static int hertz_arg(const cmd_context *context)
{
	basehertz = context->numval;
	return COMMAND_OK;
}

static int volume_arg(const cmd_context *context)
{
	fm_vol = context->numval & 0x3F; /* 63 */
	return COMMAND_OK;
}

static int attenuate_arg(const cmd_context *context)
{
	if (context->charval[0] == 'n')
	{
		hi_atten = 0;
	}
	else if (context->charval[0] == 'l')
	{
		hi_atten = 1;
	}
	else if (context->charval[0] == 'm')
	{
		hi_atten = 2;
	}
	else if (context->charval[0] == 'h')
	{
		hi_atten = 3;
	}
	else
	{
		return badarg(context->curarg);
	}
	return COMMAND_OK;
}

static int polyphony_arg(const cmd_context *context)
{
	if (context->numval > 9)
	{
		return badarg(context->curarg);
	}
	polyphony = abs(context->numval-1);
	return COMMAND_OK;
}

static int wave_type_arg(const cmd_context *context)
{
	fm_wavetype = context->numval & 0x0F;
	return COMMAND_OK;
}

static int attack_arg(const cmd_context *context)
{
	fm_attack = context->numval & 0x0F;
	return COMMAND_OK;
}

static int decay_arg(const cmd_context *context)
{
	fm_decay = context->numval & 0x0F;
	return COMMAND_OK;
}

static int sustain_arg(const cmd_context *context)
{
	fm_sustain = context->numval & 0x0F;
	return COMMAND_OK;
}

static int sustain_release_arg(const cmd_context *context)
{
	fm_release = context->numval & 0x0F;
	return COMMAND_OK;
}

static int scale_map_arg(const cmd_context *context)
{
	int counter;
	if (context->totparms != context->intparms)
	{
		return badarg(context->curarg);
	}
	for (counter = 0; counter <= 11; counter++)
	{
		if ((context->totparms > counter) && (context->intval[counter] > 0)
			&& (context->intval[counter] < 13))
		{
			scale_map[counter] = context->intval[counter];
		}
	}
	return COMMAND_OK;
}

static int periodicity_arg(const cmd_context *context)
{
	usr_periodicitycheck = 1;
	if ((context->charval[0] == 'n') || (context->numval == 0))
	{
		usr_periodicitycheck = 0;
	}
	else if (context->charval[0] == 'y')
	{
		usr_periodicitycheck = 1;
	}
	else if (context->charval[0] == 's')   /* 's' for 'show' */
	{
		usr_periodicitycheck = -1;
	}
	else if (context->numval == NON_NUMERIC)
	{
		return badarg(context->curarg);
	}
	else if (context->numval != 0)
	{
		usr_periodicitycheck = context->numval;
	}
	if (usr_periodicitycheck > 255)
	{
		usr_periodicitycheck = 255;
	}
	if (usr_periodicitycheck < -255)
	{
		usr_periodicitycheck = -255;
	}
	return COMMAND_FRACTAL_PARAM;
}

static int log_map_arg(const cmd_context *context)
{
	Log_Auto_Calc = 0;   /* turn this off if loading a PAR */
	if (context->charval[0] == 'y')
	{
		LogFlag = 1;                           /* palette is logarithmic */
	}
	else if (context->charval[0] == 'n')
	{
		LogFlag = 0;
	}
	else if (context->charval[0] == 'o')
	{
		LogFlag = -1;                          /* old log palette */
	}
	else
	{
		LogFlag = (long)context->floatval[0];
	}
	return COMMAND_FRACTAL_PARAM;
}

static int log_mode_arg(const cmd_context *context)
{
	Log_Fly_Calc = 0;                         /* turn off if error */
	Log_Auto_Calc = 0;
	if (context->charval[0] == 'f')
	{
		Log_Fly_Calc = 1;                      /* calculate on the fly */
	}
	else if (context->charval[0] == 't')
	{
		Log_Fly_Calc = 2;                      /* force use of LogTable */
	}
	else if (context->charval[0] == 'a')
	{
		Log_Auto_Calc = 1;                     /* force auto calc of logmap */
	}
	else
	{
		return badarg(context->curarg);
	}
	return COMMAND_FRACTAL_PARAM;
}

static int debug_flag_arg(const cmd_context *context)
{
	debugflag = context->numval;
	timerflag = debugflag & 1;                /* separate timer flag */
	debugflag &= ~1;
	return COMMAND_OK;
}

static int r_seed_arg(const cmd_context *context)
{
	rseed = context->numval;
	rflag = 1;
	return COMMAND_FRACTAL_PARAM;
}

static int orbit_delay_arg(const cmd_context *context)
{
	orbit_delay = context->numval;
	return COMMAND_OK;
}

static int orbit_interval_arg(const cmd_context *context)
{
	g_orbit_interval = context->numval;
	if (g_orbit_interval < 1)
	{
		g_orbit_interval = 1;
	}
	if (g_orbit_interval > 255)
	{
		g_orbit_interval = 255;
	}
	return COMMAND_OK;
}

static int show_dot_arg(const cmd_context *context)
{
	g_show_dot = 15;
	if (context->totparms > 0)
	{
		g_auto_show_dot = (char)0;
		if (isalpha(context->charval[0]))
		{
			if (strchr("abdm", (int)context->charval[0]) != NULL)
			{
				g_auto_show_dot = context->charval[0];
			}
			else
			{
				return badarg(context->curarg);
			}
		}
		else
		{
			g_show_dot = context->numval;
			if (g_show_dot < 0)
			{
				g_show_dot = -1;
			}
		}
		if (context->totparms > 1 && context->intparms > 0)
		{
			g_size_dot = context->intval[1];
		}
		if (g_size_dot < 0)
		{
			g_size_dot = 0;
		}
	}
	return COMMAND_OK;
}

static int show_orbit_arg(const cmd_context *context)
{
	g_start_show_orbit = (char)context->yesnoval[0];
	return COMMAND_OK;
}

static int decomp_arg(const cmd_context *context)
{
	if (context->totparms != context->intparms || context->totparms < 1)
	{
		return badarg(context->curarg);
	}
	decomp[0] = context->intval[0];
	decomp[1] = 0;
	if (context->totparms > 1) /* backward compatibility */
	{
		bailout = decomp[1] = context->intval[1];
	}
	return COMMAND_FRACTAL_PARAM;
}

static int dis_test_arg(const cmd_context *context)
{
	if (context->totparms != context->intparms || context->totparms < 1)
	{
		return badarg(context->curarg);
	}
	usr_distest = (long)context->floatval[0];
	distestwidth = 71;
	if (context->totparms > 1)
	{
		distestwidth = context->intval[1];
	}
	if (context->totparms > 3 && context->intval[2] > 0 && context->intval[3] > 0)
	{
		g_pseudo_x = context->intval[2];
		g_pseudo_y = context->intval[3];
	}
	else
	{
		g_pseudo_x = g_pseudo_y = 0;
	}
	return COMMAND_FRACTAL_PARAM;
}

static int formula_file_arg(const cmd_context *context)
{
	if (context->valuelen > (FILE_MAX_PATH-1))
	{
		return badarg(context->curarg);
	}
	if (merge_pathnames(FormFileName, context->value, context->mode) < 0)
	{
		init_msg(context->variable, context->value, context->mode);
	}
	return COMMAND_FRACTAL_PARAM;
}

static int formula_name_arg(const cmd_context *context)
{
	if (context->valuelen > ITEMNAMELEN)
	{
		return badarg(context->curarg);
	}
	strcpy(FormName, context->value);
	return COMMAND_FRACTAL_PARAM;
}

static int l_file_arg(const cmd_context *context)
{
	if (context->valuelen > (FILE_MAX_PATH-1))
	{
		return badarg(context->curarg);
	}
	if (merge_pathnames(LFileName, context->value, context->mode) < 0)
	{
		init_msg(context->variable, context->value, context->mode);
	}
	return COMMAND_FRACTAL_PARAM;
}

static int l_name_arg(const cmd_context *context)
{
	if (context->valuelen > ITEMNAMELEN)
	{
		return badarg(context->curarg);
	}
	strcpy(LName, context->value);
	return COMMAND_FRACTAL_PARAM;
}

static int ifs_file_arg(const cmd_context *context)
{
	int existdir;
	if (context->valuelen > (FILE_MAX_PATH-1))
	{
		return badarg(context->curarg);
	}
	existdir = merge_pathnames(IFSFileName, context->value, context->mode);
	if (existdir == 0)
	{
		reset_ifs_defn();
	}
	else if (existdir < 0)
	{
		init_msg(context->variable, context->value, context->mode);
	}
	return COMMAND_FRACTAL_PARAM;
}

static int ifs_arg(const cmd_context *context)
{
	if (context->valuelen > ITEMNAMELEN)
	{
		return badarg(context->curarg);
	}
	strcpy(IFSName, context->value);
	reset_ifs_defn();
	return COMMAND_FRACTAL_PARAM;
}

static int parm_file_arg(const cmd_context *context)
{
	if (context->valuelen > (FILE_MAX_PATH-1))
	{
		return badarg(context->curarg);
	}
	if (merge_pathnames(CommandFile, context->value, context->mode) < 0)
	{
		init_msg(context->variable, context->value, context->mode);
	}
	return COMMAND_FRACTAL_PARAM;
}

static int stereo_arg(const cmd_context *context)
{
	if ((context->numval < 0) || (context->numval > 4))
	{
		return badarg(context->curarg);
	}
	g_glasses_type = context->numval;
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int rotation_arg(const cmd_context *context)
{
	if (context->totparms != 3 || context->intparms != 3)
	{
		return badarg(context->curarg);
	}
	XROT = context->intval[0];
	YROT = context->intval[1];
	ZROT = context->intval[2];
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int perspective_arg(const cmd_context *context)
{
	if (context->numval == NON_NUMERIC)
	{
		return badarg(context->curarg);
	}
	ZVIEWER = context->numval;
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int xy_shift_arg(const cmd_context *context)
{
	if (context->totparms != 2 || context->intparms != 2)
	{
		return badarg(context->curarg);
	}
	XSHIFT = context->intval[0];
	YSHIFT = context->intval[1];
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int inter_ocular_arg(const cmd_context *context)
{
	g_eye_separation = context->numval;
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int converge_arg(const cmd_context *context)
{
	g_x_adjust = context->numval;
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int crop_arg(const cmd_context *context)
{
	if (context->totparms != 4 || context->intparms != 4
		|| context->intval[0] < 0 || context->intval[0] > 100
		|| context->intval[1] < 0 || context->intval[1] > 100
		|| context->intval[2] < 0 || context->intval[2] > 100
		|| context->intval[3] < 0 || context->intval[3] > 100)
	{
		return badarg(context->curarg);
	}
	g_red_crop_left   = context->intval[0];
	g_red_crop_right  = context->intval[1];
	g_blue_crop_left  = context->intval[2];
	g_blue_crop_right = context->intval[3];
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int bright_arg(const cmd_context *context)
{
	if (context->totparms != 2 || context->intparms != 2)
	{
		return badarg(context->curarg);
	}
	g_red_bright  = context->intval[0];
	g_blue_bright = context->intval[1];
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int xy_adjust_arg(const cmd_context *context)
{
	if (context->totparms != 2 || context->intparms != 2)
	{
		return badarg(context->curarg);
	}
	g_x_trans = context->intval[0];
	g_y_trans = context->intval[1];
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int threed_arg(const cmd_context *context)
{
	int yesno = context->yesnoval[0];
	if (strcmp(context->value, "overlay") == 0)
	{
		yesno = 1;
		if (calc_status > CALCSTAT_NO_FRACTAL) /* if no image, treat same as 3D=yes */
		{
			overlay3d = 1;
		}
	}
	else if (yesno < 0)
	{
		return badarg(context->curarg);
	}
	display3d = yesno;
	initvars_3d();
	return (display3d) ? 6 : 2;
}

static int scale_xyz_arg(const cmd_context *context)
{
	if (context->totparms < 2 || context->intparms != context->totparms)
	{
		return badarg(context->curarg);
	}
	XSCALE = context->intval[0];
	YSCALE = context->intval[1];
	if (context->totparms > 2)
	{
		ROUGH = context->intval[2];
	}
	return COMMAND_3D_PARAM;
}

static int roughness_arg(const cmd_context *context)
{
	/* "rough" is really scale z, but we add it here for convenience */
	ROUGH = context->numval;
	return COMMAND_3D_PARAM;
}

static int water_line_arg(const cmd_context *context)
{
	if (context->numval < 0)
	{
		return badarg(context->curarg);
	}
	WATERLINE = context->numval;
	return COMMAND_3D_PARAM;
}

static int fill_type_arg(const cmd_context *context)
{
	if (context->numval < -1 || context->numval > 6)
	{
		return badarg(context->curarg);
	}
	FILLTYPE = context->numval;
	return COMMAND_3D_PARAM;
}

static int light_source_arg(const cmd_context *context)
{
	if (context->totparms != 3 || context->intparms != 3)
	{
		return badarg(context->curarg);
	}
	XLIGHT = context->intval[0];
	YLIGHT = context->intval[1];
	ZLIGHT = context->intval[2];
	return COMMAND_3D_PARAM;
}

static int smoothing_arg(const cmd_context *context)
{
	if (context->numval < 0)
	{
		return badarg(context->curarg);
	}
	LIGHTAVG = context->numval;
	return COMMAND_3D_PARAM;
}

static int latitude_arg(const cmd_context *context)
{
	if (context->totparms != 2 || context->intparms != 2)
	{
		return badarg(context->curarg);
	}
	THETA1 = context->intval[0];
	THETA2 = context->intval[1];
	return COMMAND_3D_PARAM;
}

static int longitude_arg(const cmd_context *context)
{
	if (context->totparms != 2 || context->intparms != 2)
	{
		return badarg(context->curarg);
	}
	PHI1 = context->intval[0];
	PHI2 = context->intval[1];
	return COMMAND_3D_PARAM;
}

static int radius_arg(const cmd_context *context)
{
	if (context->numval < 0)
	{
		return badarg(context->curarg);
	}
	RADIUS = context->numval;
	return COMMAND_3D_PARAM;
}

static int transparent_arg(const cmd_context *context)
{
	if (context->totparms != context->intparms || context->totparms < 1)
	{
		return badarg(context->curarg);
	}
	transparent[1] = transparent[0] = context->intval[0];
	if (context->totparms > 1)
	{
		transparent[1] = context->intval[1];
	}
	return COMMAND_3D_PARAM;
}

static int coarse_arg(const cmd_context *context)
{
	if (context->numval < 3 || context->numval > 2000)
	{
		return badarg(context->curarg);
	}
	g_preview_factor = context->numval;
	return COMMAND_3D_PARAM;
}

static int randomize_arg(const cmd_context *context)
{
	if (context->numval < 0 || context->numval > 7)
	{
		return badarg(context->curarg);
	}
	g_randomize = context->numval;
	return COMMAND_3D_PARAM;
}

static int ambient_arg(const cmd_context *context)
{
	if (context->numval < 0 || context->numval > 100)
	{
		return badarg(context->curarg);
	}
	g_ambient = context->numval;
	return COMMAND_3D_PARAM;
}

static int haze_arg(const cmd_context *context)
{
	if (context->numval < 0 || context->numval > 100)
	{
		return badarg(context->curarg);
	}
	g_haze = context->numval;
	return COMMAND_3D_PARAM;
}

static int true_mode_arg(const cmd_context *context)
{
	truemode = TRUEMODE_DEFAULT;				/* use default if error */
	if (context->charval[0] == 'd')
	{
		truemode = TRUEMODE_DEFAULT;			/* use default color output */
	}
	if (context->charval[0] == 'i' || context->intval[0] == 1)
	{
		truemode = TRUEMODE_ITERATES;			/* use iterates output */
	}
	return COMMAND_FRACTAL_PARAM | COMMAND_3D_PARAM;
}

static int monitor_width_arg(const cmd_context *context)
{
	if (context->totparms != 1 || context->floatparms != 1)
	{
		return badarg(context->curarg);
	}
	AutoStereo_width  = context->floatval[0];
	return COMMAND_3D_PARAM;
}

static int background_arg(const cmd_context *context)
{
	int i;

	if (context->totparms != 3 || context->intparms != 3)
	{
		return badarg(context->curarg);
	}
	for (i = 0; i < 3; i++)
	{
		if (context->intval[i] & ~0xff)
		{
			return badarg(context->curarg);
		}
	}
	g_back_color[0] = (BYTE)context->intval[0];
	g_back_color[1] = (BYTE)context->intval[1];
	g_back_color[2] = (BYTE)context->intval[2];
	return COMMAND_3D_PARAM;
}

static int light_name_arg(const cmd_context *context)
{
	if (context->valuelen > (FILE_MAX_PATH-1))
	{
		return badarg(context->curarg);
	}
	if (first_init || context->mode == CMDFILE_AT_AFTER_STARTUP)
	{
		strcpy(g_light_name, context->value);
	}
	return COMMAND_OK;
}

static int ray_arg(const cmd_context *context)
{
	if (context->numval < 0 || context->numval > 6)
	{
		return badarg(context->curarg);
	}
	g_raytrace_output = context->numval;
	return COMMAND_3D_PARAM;
}

static int release_arg(const cmd_context *context)
{
	if (context->numval < 0)
	{
		return badarg(context->curarg);
	}

	save_release = context->numval;
	return COMMAND_3D_PARAM;
}

struct tag_command_processor
{
	const char *command;
	int (*processor)(const cmd_context *context);
};
typedef struct tag_command_processor command_processor;

static int named_processor(const command_processor *processors,
						   int num_processors,
						   const cmd_context *context,
						   const char *command, int *result)
{
	int i;
	for (i = 0; i < num_processors; i++)
	{
		if (strcmp(processors[i].command, command) == 0)
		{
			*result = processors[i].processor(context);
			return TRUE;
		}
	}
	return FALSE;
}

static int gif87a_arg(const cmd_context *context)
{
	return flag_arg(context, &gif87a_flag, COMMAND_OK);
}

static int dither_arg(const cmd_context *context)
{
	return flag_arg(context, &dither_flag, COMMAND_OK);
}

static int fin_attract_arg(const cmd_context *context)
{
	return flag_arg(context, &finattract, COMMAND_FRACTAL_PARAM);
}

static int no_bof_arg(const cmd_context *context)
{
	return flag_arg(context, &nobof, COMMAND_FRACTAL_PARAM);
}

static int is_mand_arg(const cmd_context *context)
{
	return flag_arg(context, &g_is_mand, COMMAND_FRACTAL_PARAM);
}

static int sphere_arg(const cmd_context *context)
{
	return flag_arg(context, &SPHERE, COMMAND_3D_PARAM);
}

static int preview_arg(const cmd_context *context)
{
	return flag_arg(context, &g_preview, COMMAND_3D_PARAM);
}

static int showbox_arg(const cmd_context *context)
{
	return flag_arg(context, &g_show_box, COMMAND_3D_PARAM);
}

static int fullcolor_arg(const cmd_context *context)
{
	return flag_arg(context, &Targa_Out, COMMAND_3D_PARAM);
}

static int truecolor_arg(const cmd_context *context)
{
	return flag_arg(context, &truecolor, COMMAND_3D_PARAM | COMMAND_FRACTAL_PARAM);
}

static int use_grayscale_depth_arg(const cmd_context *context)
{
	return flag_arg(context, &g_grayscale_depth, COMMAND_3D_PARAM);
}

static int targa_overlay_arg(const cmd_context *context)
{
	return flag_arg(context, &g_targa_overlay, COMMAND_3D_PARAM);
}

static int brief_arg(const cmd_context *context)
{
	return flag_arg(context, &g_raytrace_brief, COMMAND_3D_PARAM);
}

static int screencoords_arg(const cmd_context *context)
{
	return flag_arg(context, &g_keep_screen_coords, COMMAND_FRACTAL_PARAM);
}

static int olddemmcolors_arg(const cmd_context *context)
{
	return flag_arg(context, &g_old_demm_colors, COMMAND_OK);
}

static int askvideo_arg(const cmd_context *context)
{
	return flag_arg(context, &askvideo, COMMAND_OK);
}

static int cur_dir_arg(const cmd_context *context)
{
	return flag_arg(context, &checkcurdir, COMMAND_OK);
}

/*
	process_command(string, mode) processes a single command-line/command-file argument
	return:
		-1 error, >= 0 ok
		if ok, return value:
		| 1 means fractal parm has been set
		| 2 means 3d parm has been set
		| 4 means 3d=yes specified
		| 8 means reset specified
*/

int process_command(char *curarg, int mode) /* process a single argument */
{
	cmd_context context;
	char    variable[21];                /* variable name goes here   */
	double  ftemp;
	int     i, j;
	char    *argptr, *argptr2;
	char    tmpc;
	int     lastarg;

	argptr = curarg;
	context.curarg = curarg;
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

	context.value = strchr(&curarg[1], '=');
	if (context.value != NULL)
	{
		j = (int) ((context.value++) - curarg);
		if (j > 1 && curarg[j-1] == ':')
		{
			--j;                           /* treat := same as =     */
		}
	}
	else
	{
		j = (int) strlen(curarg);
		context.value = curarg + j;
	}
	if (j > 20)
	{
		return badarg(context.curarg);             /* keyword too long */
	}
	strncpy(variable, curarg, j);          /* get the variable name  */
	variable[j] = 0;                     /* truncate variable name */
	context.valuelen = (int) strlen(context.value);            /* note value's length    */
	context.charval[0] = context.value[0];               /* first letter of value  */
	context.yesnoval[0] = -1;                    /* note yes|no value      */
	if (context.charval[0] == 'n')
	{
		context.yesnoval[0] = 0;
	}
	if (context.charval[0] == 'y')
	{
		context.yesnoval[0] = 1;
	}

	argptr = context.value;
	context.numval = context.totparms = context.intparms = context.floatparms = 0;
	while (*argptr)                    /* count and pre-parse parms */
	{
		long ll;
		lastarg = 0;
		argptr2 = strchr(argptr, '/');
		if (argptr2 == NULL)     /* find next '/' */
		{
			argptr2 = argptr + strlen(argptr);
			*argptr2 = '/';
			lastarg = 1;
		}
		if (context.totparms == 0)
		{
			context.numval = NON_NUMERIC;
		}
		i = -1;
		if (context.totparms < 16)
		{
			context.charval[context.totparms] = *argptr;                      /* first letter of value  */
			if (context.charval[context.totparms] == 'n')
			{
				context.yesnoval[context.totparms] = 0;
			}
			if (context.charval[context.totparms] == 'y')
			{
				context.yesnoval[context.totparms] = 1;
			}
		}
		j = 0;
		if (sscanf(argptr, "%c%c", (char *) &j, &tmpc) > 0    /* NULL entry */
			&& ((char) j == '/' || (char) j == '=') && tmpc == '/')
		{
			j = 0;
			++context.floatparms;
			++context.intparms;
			if (context.totparms < 16)
			{
				context.floatval[context.totparms] = j;
				context.floatvalstr[context.totparms] = "0";
			}
			if (context.totparms < 64)
			{
				context.intval[context.totparms] = j;
			}
			if (context.totparms == 0)
			{
				context.numval = j;
			}
		}
		else if (sscanf(argptr, "%ld%c", &ll, &tmpc) > 0       /* got an integer */
			&& tmpc == '/')        /* needs a long int, ll, here for lyapunov */
		{
			++context.floatparms;
			++context.intparms;
			if (context.totparms < 16)
			{
				context.floatval[context.totparms] = ll;
				context.floatvalstr[context.totparms] = argptr;
			}
			if (context.totparms < 64)
			{
				context.intval[context.totparms] = (int) ll;
			}
			if (context.totparms == 0)
			{
				context.numval = (int) ll;
			}
		}
#ifndef XFRACT
		else if (sscanf(argptr, "%lg%c", &ftemp, &tmpc) > 0  /* got a float */
#else
		else if (sscanf(argptr, "%lf%c", &ftemp, &tmpc) > 0  /* got a float */
#endif
				&& tmpc == '/')
		{
			++context.floatparms;
			if (context.totparms < 16)
			{
				context.floatval[context.totparms] = ftemp;
				context.floatvalstr[context.totparms] = argptr;
			}
		}
		/* using arbitrary precision and above failed */
		else if (((int) strlen(argptr) > 513)  /* very long command */
					|| (context.totparms > 0 && context.floatval[context.totparms-1] == FLT_MAX
						&& context.totparms < 6)
					|| isabigfloat(argptr))
		{
			++context.floatparms;
			context.floatval[context.totparms] = FLT_MAX;
			context.floatvalstr[context.totparms] = argptr;
		}
		++context.totparms;
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

	context.mode = mode;
	context.variable = variable;
	/* these commands are allowed only at startup */
	if (mode != CMDFILE_AT_AFTER_STARTUP || DEBUGFLAG_NO_FIRST_INIT == debugflag)
	{
		command_processor processors[] =
		{
			{ "batch",		batch_arg },
			{ "maxhistory", max_history_arg },
			{ "adapter",	adapter_arg },
			{ "afi",		ignore_arg }, /* 8514 API no longer used; silently gobble any argument */
			{ "textsafe",	text_safe_arg },
			{ "vesadetect", gobble_flag_arg },
			{ "biospalette", gobble_flag_arg },
			{ "fpu",		fpu_arg },
			{ "exitnoask",	exit_no_ask_arg },
			{ "makedoc",	make_doc_arg },
			{ "makepar",	make_par_arg }
		};
		int result = COMMAND_OK;
		if (named_processor(processors, NUM_OF(processors), &context, variable, &result))
		{
			return result;
		}
	}

	{
		command_processor processors[] =
		{
			{ "reset",			reset_arg },
			{ "filename",		filename_arg },			/* filename=?     */
			{ "video",			video_arg },			/* video=? */
 			{ "map",			map_arg },				/* map=, set default colors */
			{ "colors",			colors_arg },			/* colors=, set current colors */
			{ "recordcolors",	record_colors_arg },	/* recordcolors= */
			{ "maxlinelength",	max_line_length_arg },	/* maxlinelength= */
			{ "comment",		parse_arg },			/* comment= */
			{ "tplus",			gobble_flag_arg },		/* tplus no longer used */
			{ "noninterlaced",	gobble_flag_arg },		/* noninterlaced no longer used */
			{ "maxcolorres",	max_color_res_arg },	/* Change default color resolution */
			{ "pixelzoom",		pixel_zoom_arg },
			{ "warn",			warn_arg },				/* warn=? */
			{ "overwrite",		overwrite_arg },		/* overwrite=? */
			{ "gif87a",			gif87a_arg },
			{ "dither",			dither_arg },
			{ "savetime",		save_time_arg },		/* savetime=? */
			{ "autokey",		auto_key_arg },			/* autokey=? */
			{ "autokeyname",	auto_key_name_arg },	/* autokeyname=? */
			{ "type",			type_arg },				/* type=? */
			{ "inside",			inside_arg },			/* inside=? */
			{ "proximity",		proximity_arg },		/* proximity=? */
			{ "fillcolor",		fill_color_arg },		/* fillcolor */
			{ "finattract",		fin_attract_arg },
			{ "nobof",			no_bof_arg },
			{ "function",		function_arg },			/* function=?,? */
			{ "outside",		outside_arg },			/* outside=? */
			{ "bfdigits",		bf_digits_arg },		/* bfdigits=? */
			{ "maxiter",		max_iter_arg },			/* maxiter=? */
			{ "iterincr",		ignore_arg },			/* iterincr=? */
			{ "passes",			passes_arg },			/* passes=? */
			{ "ismand",			is_mand_arg },
			{ "cyclelimit",		cycle_limit_arg },		/* cyclelimit=? */
			{ "makemig",		make_mig_arg },
			{ "cyclerange",		cycle_range_arg },
			{ "ranges",			ranges_arg },
			{ "savename", 		save_name_arg },		/* savename=? */
			{ "tweaklzw", 		tweak_lzw_arg },		/* tweaklzw=? */
			{ "minstack", 		min_stack_arg },		/* minstack=? */
			{ "mathtolerance", 	math_tolerance_arg },	/* mathtolerance=? */
			{ "tempdir", 		temp_dir_arg },			/* tempdir=? */
			{ "workdir", 		work_dir_arg },			/* workdir=? */
			{ "exitmode", 		exit_mode_arg },		/* exitmode=? */
			{ "textcolors", 	text_colors_arg },
			{ "potential", 		potential_arg },		/* potential=? */
			{ "params", 		params_arg },			/* params=?,? */
			{ "miim", 			miim_arg },				/* miim=?[/?[/?[/?]]] */
			{ "initorbit", 		init_orbit_arg },		/* initorbit=?,? */
			{ "orbitname", 		orbit_name_arg },		/* orbitname=? */
			{ "3dmode", 		threed_mode_arg },		/* orbitname=? */
			{ "julibrot3d", 	julibrot_3d_arg },		/* julibrot3d=?,?,?,? */
			{ "julibroteyes", 	julibrot_eyes_arg },	/* julibroteyes=?,?,?,? */
			{ "julibrotfromto", julibrot_from_to_arg },	/* julibrotfromto=?,?,?,? */
			{ "corners", 		corners_arg },			/* corners=?,?,?,? */
			{ "orbitcorners", 	orbit_corners_arg },	/* orbit corners=?,?,?,? */
			{ "screencoords", 	screencoords_arg },
			{ "orbitdrawmode", 	orbit_draw_mode_arg },	/* orbitdrawmode=? */
			{ "viewwindows", 	view_windows_arg },		/* viewwindows=?,?,?,?,? */
			{ "center-mag", 	center_mag_arg },		/* center-mag=?,?,?[,?,?,?] */
			{ "aspectdrift", 	aspect_drift_arg },		/* aspectdrift=? */
			{ "invert", 		invert_arg },			/* invert=?,?,? */
			{ "olddemmcolors", 	olddemmcolors_arg },
			{ "askvideo", 		askvideo_arg },
			{ "ramvideo", 		ignore_arg },			/* ramvideo=?   */
			{ "float", 			float_arg },			/* float=? */
			{ "fastrestore", 	fast_restore_arg },		/* fastrestore=? */
			{ "orgfrmdir", 		org_frm_dir_arg },		/* orgfrmdir=? */
			{ "biomorph", 		biomorph_arg },			/* biomorph=? */
			{ "orbitsave", 		orbit_save_arg },		/* orbitsave=? */
			{ "bailout", 		bail_out_arg },			/* bailout=? */
			{ "bailoutest", 	bail_out_test_arg },	/* bailoutest=? */
			{ "symmetry", 		symmetry_arg },			/* symmetry=? */
			{ "printer", 		ignore_arg },			/* deprecated print parameters */
			{ "printfile", 		ignore_arg },
			{ "rleps", 			ignore_arg },
			{ "colorps", 		ignore_arg },
			{ "epsf", 			ignore_arg },
			{ "title", 			ignore_arg },
			{ "translate", 		ignore_arg },
			{ "plotstyle", 		ignore_arg },
			{ "halftone", 		ignore_arg },
			{ "linefeed", 		ignore_arg },
			{ "comport", 		ignore_arg },
			{ "sound", 			sound_arg },			/* sound=?,?,? */
			{ "hertz", 			hertz_arg },			/* Hertz=? */
			{ "volume", 		volume_arg },			/* Volume =? */
			{ "attenuate", 		attenuate_arg },
			{ "polyphony", 		polyphony_arg },
			{ "wavetype", 		wave_type_arg },		/* wavetype = ? */
			{ "attack", 		attack_arg },			/* attack = ? */
			{ "decay", 			decay_arg },			/* decay = ? */
			{ "sustain", 		sustain_arg },			/* sustain = ? */
			{ "srelease", 		sustain_release_arg },	/* srelease = ? */
			{ "scalemap", 		scale_map_arg },		/* Scalemap=?,?,?,?,?,?,?,?,?,?,? */
			{ "periodicity", 	periodicity_arg },		/* periodicity=? */
			{ "logmap", 		log_map_arg },			/* logmap=? */
			{ "logmode", 		log_mode_arg },			/* logmode=? */
			{ "debugflag", 		debug_flag_arg },
			{ "debug", 			debug_flag_arg },		/* internal use only */
			{ "rseed", 			r_seed_arg },
			{ "orbitdelay", 	orbit_delay_arg },
			{ "orbitinterval", 	orbit_interval_arg },
			{ "showdot", 		show_dot_arg },
			{ "showorbit", 		show_orbit_arg },		/* showorbit=yes|no */
			{ "decomp", 		decomp_arg },
			{ "distest", 		dis_test_arg },
			{ "formulafile", 	formula_file_arg },		/* formulafile=? */
			{ "formulaname", 	formula_name_arg },		/* formulaname=? */
			{ "lfile", 			l_file_arg },			/* lfile=? */
			{ "lname", 			l_name_arg },
			{ "ifsfile", 		ifs_file_arg },			/* ifsfile=?? */
			{ "ifs", 			ifs_arg },
			{ "ifs3d", 			ifs_arg },				/* ifs3d for old time's sake */
			{ "parmfile", 		parm_file_arg },		/* parmfile=? */
			{ "stereo", 		stereo_arg },			/* stereo=? */
			{ "rotation", 		rotation_arg },			/* rotation=?/?/? */
			{ "perspective", 	perspective_arg },		/* perspective=? */
			{ "xyshift", 		xy_shift_arg },			/* xyshift=?/?  */
			{ "interocular", 	inter_ocular_arg },		/* interocular=? */
			{ "converge", 		converge_arg },			/* converg=? */
			{ "crop", 			crop_arg },				/* crop=? */
			{ "bright", 		bright_arg },			/* bright=? */
			{ "xyadjust", 		xy_adjust_arg },		/* xyadjust=?/? */
			{ "3d", 			threed_arg },			/* 3d=?/?/..    */
			{ "sphere", 		sphere_arg },
			{ "scalexyz", 		scale_xyz_arg },		/* scalexyz=?/?/? */
			{ "roughness", 		roughness_arg },		/* roughness=?  */
			{ "waterline", 		water_line_arg },		/* waterline=?  */
			{ "filltype", 		fill_type_arg },		/* filltype=?   */
			{ "lightsource", 	light_source_arg },		/* lightsource=?/?/? */
			{ "smoothing", 		smoothing_arg },		/* smoothing=?  */
			{ "latitude", 		latitude_arg },			/* latitude=?/? */
			{ "longitude", 		longitude_arg },		/* longitude=?/? */
			{ "radius", 		radius_arg },			/* radius=? */
			{ "transparent", 	transparent_arg },		/* transparent? */
			{ "preview", 		preview_arg },
			{ "showbox", 		showbox_arg },
			{ "coarse", 		coarse_arg },			/* coarse=? */
			{ "randomize", 		randomize_arg },		/* randomize=? */
			{ "ambient", 		ambient_arg },			/* ambient=? */
			{ "haze", 			haze_arg },				/* haze=? */
			{ "fullcolor", 		fullcolor_arg },
			{ "truecolor", 		truecolor_arg },
			{ "truemode", 		true_mode_arg },		/* truemode=? */
			{ "usegrayscale", 	use_grayscale_depth_arg },
			{ "monitorwidth", 	monitor_width_arg },	/* monitorwidth=? */
			{ "targa_overlay", 	targa_overlay_arg },
			{ "background", 	background_arg },		/* background=?/? */
			{ "lightname", 		light_name_arg },		/* lightname=?   */
			{ "ray", 			ray_arg },				/* ray=? */
			{ "brief", 			brief_arg },
			{ "release", 		release_arg },			/* release */
			{ "curdir", 		cur_dir_arg },
			{ "virtual", 		gobble_flag_arg }
		};
		int result = COMMAND_OK;
		if (named_processor(processors, NUM_OF(processors), &context, variable, &result))
		{
			return result;
		}
	}

	return badarg(context.curarg);
}

#ifdef _MSC_VER
#if (_MSC_VER >= 600)
#pragma optimize("el", on)
#endif
#endif

static void parse_textcolors(char *value)
{
	int i, j, k, hexval;
	if (strcmp(value, "mono") == 0)
	{
		for (k = 0; k < sizeof(txtcolor); ++k)
		{
			txtcolor[k] = BLACK*16 + WHITE;
		}
	/* C_HELP_CURLINK = C_PROMPT_INPUT = C_CHOICE_CURRENT = C_GENERAL_INPUT
							= C_AUTHDIV1 = C_AUTHDIV2 = WHITE*16 + BLACK; */
		txtcolor[6] = txtcolor[12] = txtcolor[13] = txtcolor[14] = txtcolor[20]
						= txtcolor[27] = txtcolor[28] = WHITE*16 + BLACK;
		/* C_TITLE = C_HELP_HDG = C_HELP_LINK = C_PROMPT_HI = C_CHOICE_SP_KEYIN
					= C_GENERAL_HI = C_DVID_HI = C_STOP_ERR
					= C_STOP_INFO = BLACK*16 + L_WHITE; */
		txtcolor[0] = txtcolor[2] = txtcolor[5] = txtcolor[11] = txtcolor[16]
						= txtcolor[17] = txtcolor[22] = txtcolor[24]
						= txtcolor[25] = BLACK*16 + L_WHITE;
	}
	else
	{
		k = 0;
		while (k < sizeof(txtcolor))
		{
			if (*value == 0)
			{
				break;
			}
			if (*value != '/')
			{
				sscanf(value, "%x", &hexval);
				i = (hexval / 16) & 7;
				j = hexval & 15;
				if (i == j || (i == 0 && j == 8)) /* force contrast */
				{
					j = 15;
				}
				txtcolor[k] = (BYTE) (i*16 + j);
				value = strchr(value, '/');
				if (value == NULL)
				{
					break;
				}
			}
			++value;
			++k;
		}
	}
}

static int parse_colors(char *value)
{
	int i, j, k;
	if (*value == '@')
	{
		if (merge_pathnames(MAP_name, &value[1], 3) < 0)
		{
			init_msg("", &value[1], 3);
		}
		if ((int)strlen(value) > FILE_MAX_PATH || ValidateLuts(MAP_name) != 0)
		{
			goto badcolor;
		}
		if (display3d)
		{
			mapset = 1;
		}
		else
		{
			if (merge_pathnames(colorfile, &value[1], 3) < 0)
			{
				init_msg("", &value[1], 3);
			}
			colorstate = 2;
		}
	}
	else
	{
		int smooth;
		i = smooth = 0;
		while (*value)
		{
			if (i >= 256)
			{
				goto badcolor;
			}
			if (*value == '<')
			{
				if (i == 0 || smooth
					|| (smooth = atoi(value + 1)) < 2
					|| (value = strchr(value, '>')) == NULL)
				{
					goto badcolor;
				}
				i += smooth;
				++value;
			}
			else
			{
				for (j = 0; j < 3; ++j)
				{
					if ((k = *(value++)) < '0')
					{
						goto badcolor;
					}
					else if (k <= '9')
					{
						k -= '0';
					}
					else if (k < 'A')
					{
						goto badcolor;
					}
					else if (k <= 'Z')
					{
						k -= ('A'-10);
					}
					else if (k < '_' || k > 'z')
					{
						goto badcolor;
					}
					else
					{
						k -= ('_'-36);
					}
					g_dac_box[i][j] = (BYTE)k;
					if (smooth)
					{
						int spread = smooth + 1;
						int start = i - spread;
						int cnum = 0;
						if ((k - (int)g_dac_box[start][j]) == 0)
						{
							while (++cnum < spread)
							{
								g_dac_box[start + cnum][j] = (BYTE)k;
							}
						}
						else
						{
							while (++cnum < spread)
							{
								g_dac_box[start + cnum][j] =
									(BYTE)((cnum *g_dac_box[i][j]
									+ (i-(start + cnum))*g_dac_box[start][j]
									+ spread/2)
									/ (BYTE) spread);
							}
						}
					}
				}
				smooth = 0;
				++i;
			}
		}
		if (smooth)
		{
			goto badcolor;
		}
		while (i < 256)   /* zap unset entries */
		{
			g_dac_box[i][0] = g_dac_box[i][1] = g_dac_box[i][2] = 40;
			++i;
		}
		colorstate = 1;
	}
	colorpreloaded = 1;
	memcpy(olddacbox, g_dac_box, 256*3);
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
		initbatch = INIT_BATCH_BAILOUT_INTERRUPTED;
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
	g_x_trans    = 0;
	g_y_trans    = 0;
	LIGHTAVG  = 0;
	g_ambient   = 20;
	g_randomize = 0;
	g_haze      = 0;
	g_back_color[0] = 51;
	g_back_color[1] = 153;
	g_back_color[2] = 200;
	if (SPHERE)
	{
		PHI1      =  180;
		PHI2      =  0;
		THETA1    =  -90;
		THETA2    =  90;
		RADIUS    =  100;
		FILLTYPE  = FILLTYPE_FILL_GOURAUD;
		XLIGHT    = 1;
		YLIGHT    = 1;
		ZLIGHT    = 1;
	}
	else
	{
		XROT      = 60;
		YROT      = 30;
		ZROT      = 0;
		XSCALE    = 90;
		YSCALE    = 90;
		FILLTYPE  = FILLTYPE_POINTS;
		XLIGHT    = 1;
		YLIGHT    = -1;
		ZLIGHT    = 1;
	}
}

/* copy a big number from a string, up to slash */
static int get_bf(bf_t bf, char *curarg)
{
	char *s;
	s = strchr(curarg, '/');
	if (s)
	{
		*s = 0;
	}
	strtobf(bf, curarg);
	if (s)
	{
		*s = '/';
	}
	return 0;
}

/* Get length of current args */
/* TODO: this shouldn't modify the curarg! */
int get_curarg_len(char *curarg)
{
	int len;
	char *s;
	s = strchr(curarg, '/');
	if (s)
	{
		*s = 0;
	}
	len = (int) strlen(curarg);
	if (s)
	{
		*s = '/';
	}
	return len;
}

/* Get max length of current args */
int get_max_curarg_len(char *floatvalstr[], int totparms)
{
	int i, tmp, max_str;
	max_str = 0;
	for (i = 0; i < totparms; i++)
	{
		tmp = get_curarg_len(floatvalstr[i]);
		if (tmp > max_str)
		{
			max_str = tmp;
		}
	}
	return max_str;
}

/* mode = 0 command line @filename         */
/*        1 sstools.ini                    */
/*        2 <@> command after startup      */
/*        3 command line @filename/setname */
/* this is like stopmsg() but can be used in cmdfiles()      */
/* call with NULL for badfilename to get pause for driver_get_key() */
int init_msg(const char *cmdstr, char *badfilename, int mode)
{
	char *modestr[4] =
		{"command line", "sstools.ini", "PAR file", "PAR file"};
	char msg[256];
	char cmd[80];
	static int row = 1;

	if (initbatch == INIT_BATCH_NORMAL)  /* in batch mode */
	{
		if (badfilename)
		{
			/* uncomment next if wish to cause abort in batch mode for
			errors in CMDFILES.C such as parsing SSTOOLS.INI */
			/* initbatch = INIT_BATCH_BAILOUT_INTERRUPTED; */ /* used to set errorlevel */
			return -1;
		}
	}
	strncpy(cmd, cmdstr, 30);
	cmd[29] = 0;

	if (*cmd)
	{
		strcat(cmd, "=");
	}
	if (badfilename)
	{
		sprintf(msg, "Can't find %s%s, please check %s", cmd, badfilename, modestr[mode]);
	}
	if (first_init)  /* & cmdfiles hasn't finished 1st try */
	{
		if (row == 1 && badfilename)
		{
			driver_set_for_text();
			driver_put_string(0, 0, 15, "Fractint found the following problems when parsing commands: ");
		}
		if (badfilename)
		{
			driver_put_string(row++, 0, 7, msg);
		}
		else if (row > 1)
		{
			driver_put_string(++row, 0, 15, "Press Escape to abort, any other key to continue");
			driver_move_cursor(row + 1, 0);
			dopause(PAUSE_ERROR_GOODBYE);  /* defer getakeynohelp until after parsing */
		}
	}
	else if (badfilename)
	{
		stopmsg(0, msg);
	}
	return 0;
}

/* defer pause until after parsing so we know if in batch mode */
void dopause(int action)
{
	static int needpause = PAUSE_ERROR_NO_BATCH;
	switch (action)
	{
	case PAUSE_ERROR_NO_BATCH:
		if (initbatch == INIT_BATCH_NONE)
		{
			if (needpause == PAUSE_ERROR_ANY)
			{
				driver_get_key();
			}
			else if (needpause == PAUSE_ERROR_GOODBYE)
			{
				if (getakeynohelp() == FIK_ESC)
				{
					goodbye();
				}
			}
		}
		needpause = PAUSE_ERROR_NO_BATCH;
		break;

	case PAUSE_ERROR_ANY:
	case PAUSE_ERROR_GOODBYE:
		needpause = action;
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
	int result = 1;
	char *s = str;
	int numdot = 0;
	int nume = 0;
	int numsign = 0;
	while (*s != 0 && *s != '/' && *s != ' ')
	{
		if (*s == '-' || *s == '+')
		{
			numsign++;
		}
		else if (*s == '.')
		{
			numdot++;
		}
		else if (*s == 'e' || *s == 'E' || *s == 'g' || *s == 'G')
		{
			nume++;
		}
		else if (!isdigit(*s))
		{
			result = 0;
			break;
		}
		s++;
	}
	if (numdot > 1 || numsign > 2 || nume > 1)
	{
		result = 0;
	}
	return result;
}

