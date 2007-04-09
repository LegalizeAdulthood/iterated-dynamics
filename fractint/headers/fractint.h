/* FRACTINT.H - common structures and values for the FRACTINT routines */

#ifndef FRACTINT_H
#define FRACTINT_H

typedef BYTE BOOLEAN;

#ifndef C6
#ifndef _fastcall
#define _fastcall       /* _fastcall is a Microsoft C6.00 extension */
#endif
#endif

/* Returns the number of items in an array declared of fixed size, i.e:
	int stuff[100];
	NUM_OF(stuff) returns 100.
*/
#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

#ifndef XFRACT
#define clock_ticks() clock()
#endif

#ifdef XFRACT
#define difftime(now,then) ((now)-(then))
#endif

#define TRIG_LIMIT_16 (8L << 16)		/* domain limit of fast trig functions */
#define NUM_BOXES 4096

/* g_note_attenuation values */
#define ATTENUATE_NONE		0
#define ATTENUATE_LOW		1
#define ATTENUATE_MIDDLE	2
#define ATTENUATE_HIGH		3

/* g_ifs_type values */
#define IFSTYPE_2D 0
#define IFSTYPE_3D 1

/* g_log_dynamic_calculate values */
#define LOGDYNAMIC_NONE 0
#define LOGDYNAMIC_DYNAMIC 1
#define LOGDYNAMIC_TABLE 2

/* g_log_palette_flag special values */
#define LOGPALETTE_NONE 0
#define LOGPALETTE_STANDARD 1
#define LOGPALETTE_OLD -1

/* g_color_state values */
#define COLORSTATE_DEFAULT	0
#define COLORSTATE_UNKNOWN	1
#define COLORSTATE_MAP		2

/* g_force_symmetry values */
#define FORCESYMMETRY_NONE		999
#define FORCESYMMETRY_SEARCH	1000

/* g_save_dac values */
#define SAVEDAC_NO		0
#define SAVEDAC_YES		1
#define SAVEDAC_NEXT	2

/* g_raytrace_output values */
#define RAYTRACE_NONE		0
#define RAYTRACE_POVRAY		1
#define RAYTRACE_VIVID		2
#define RAYTRACE_RAW		3
#define RAYTRACE_MTV		4
#define RAYTRACE_RAYSHADE	5
#define RAYTRACE_ACROSPIN	6
#define RAYTRACE_DXF		7

/* g_orbit_draw_mode values */
#define ORBITDRAW_RECTANGLE	0
#define ORBITDRAW_LINE		1
#define ORBITDRAW_FUNCTION	2

/* ant types */
#define ANTTYPE_MOVE_COLOR	0
#define ANTTYPE_MOVE_RULE	1

/* g_true_mode values */
#define TRUEMODE_DEFAULT	0
#define TRUEMODE_ITERATES	1

/* timer type values */
#define TIMER_ENGINE	0
#define TIMER_DECODER	1
#define TIMER_ENCODER	2

/* g_debug_flag values */
#define DEBUGFLAG_NONE				0
#define DEBUGFLAG_LORENZ_FLOAT		22
#define DEBUGFLAG_COMPARE_RESTORED	50
#define DEBUGFLAG_NO_FPU			70
#define DEBUGFLAG_FAST_287_MATH		72
#define DEBUGFLAG_NO_ASM_MANDEL		90
#define DEBUGFLAG_OLD_POWER			94
#define DEBUGFLAG_DISK_MESSAGES		96
#define DEBUGFLAG_REAL_POPCORN		98
#define DEBUGFLAG_NO_FIRST_INIT		110
#define DEBUGFLAG_TIME_ENCODER		200
#define DEBUGFLAG_NO_MIIM_QUEUE		300
#define DEBUGFLAG_SKIP_OPTIMIZER	322
#define DEBUGFLAG_NO_HELP_F1_ESC	324
#define DEBUGFLAG_USE_DISK			420
#define DEBUGFLAG_USE_MEMORY		422
#define DEBUGFLAG_ABORT_SAVENAME	450
#define DEBUGFLAG_BNDTRACE_NONZERO	470
#define DEBUGFLAG_SOLID_GUESS_BR	472
#define DEBUGFLAG_SET_DIGITS_MIN	700
#define DEBUGFLAG_SET_DIGITS_MAX	720
#define DEBUGFLAG_MORE_DIGITS		750
#define DEBUGFLAG_FPU_87			870
#define DEBUGFLAG_NO_COLORS_FIX		910
#define DEBUGFLAG_COLORS_LOSSLESS	920
#define DEBUGFLAG_FORCE_FP_NEWTON	1010
#define DEBUGFLAG_SWAP_SIGN			1012
#define DEBUGFLAG_FORCE_BITSHIFT	1234
#define DEBUGFLAG_2222				2222
#define DEBUGFLAG_2224				2224
#define DEBUGFLAG_X_FPU_287			2870
#define DEBUGFLAG_EDIT_TEXT_COLORS	3000
#define DEBUGFLAG_NO_DEV_HEADING	3002
#define DEBUGFLAG_NO_BIG_TO_FLOAT	3200
#define DEBUGFLAG_NO_INT_TO_FLOAT	3400
#define DEBUGFLAG_SOI_LONG_DOUBLE	3444
#define DEBUGFLAG_PIN_CORNERS_ONE	3600
#define DEBUGFLAG_NO_PIXEL_GRID		3800
#define DEBUGFLAG_SHOW_MATH_ERRORS	4000
#define DEBUGFLAG_PRE193_CENTERMAG	4010
#define DEBUGFLAG_OLD_TIMER			4020
#define DEBUGFLAG_OLD_ORBIT_SOUND	4030
#define DEBUGFLAG_MIN_DISKVID_CACHE	4200
#define DEBUGFLAG_UNOPT_POWER		6000
#define DEBUGFLAG_CPU_8088			8088
#define DEBUGFLAG_REDUCE_VIDEO_MIN	9002
#define DEBUGFLAG_REDUCE_VIDEO_MAX	9100
#define DEBUGFLAG_MEMORY			10000

/* projection values */
#define PROJECTION_ZX	0
#define PROJECTION_XZ	1
#define PROJECTION_XY	2

/* random_dir values */
#define DIRECTION_LEFT		0
#define DIRECTION_RIGHT		1
#define DIRECTION_RANDOM	2

/* g_juli_3d_mode values */
#define JULI3DMODE_MONOCULAR	0
#define JULI3DMODE_LEFT_EYE		1
#define JULI3DMODE_RIGHT_EYE	2
#define JULI3DMODE_RED_BLUE		3

/* g_which_image values */
#define WHICHIMAGE_NONE 0
#define WHICHIMAGE_RED	1
#define WHICHIMAGE_BLUE	2

/* FILLTYPE values */
#define FILLTYPE_SURFACE_GRID	-1
#define FILLTYPE_POINTS			0
#define FILLTYPE_WIRE_FRAME		1
#define FILLTYPE_FILL_GOURAUD	2
#define FILLTYPE_FILL_FLAT		3
#define FILLTYPE_FILL_BARS		4
#define FILLTYPE_LIGHT_BEFORE	5
#define FILLTYPE_LIGHT_AFTER	6

/* g_orbit_save values */
#define ORBITSAVE_NONE	0
#define ORBITSAVE_RAW	1
#define ORBITSAVE_SOUND 2

/* g_glasses_type values */
#define STEREO_NONE			0
#define STEREO_ALTERNATE	1
#define STEREO_SUPERIMPOSE	2
#define STEREO_PHOTO		3
#define STEREO_PAIR			4

/* find_file_item itemtypes */
#define ITEMTYPE_PARAMETER	0
#define ITEMTYPE_FORMULA	1
#define ITEMTYPE_L_SYSTEM	2
#define ITEMTYPE_IFS		3

/* getfileentry and find_file_item itemtype values */
#define GETFILE_FORMULA		0
#define GETFILE_L_SYSTEM	1
#define GETFILE_IFS			2
#define GETFILE_PARAMETER	3

/* g_got_status values */
#define GOT_STATUS_NONE -1
#define GOT_STATUS_12PASS 0
#define GOT_STATUS_GUESSING 1
#define GOT_STATUS_BOUNDARY_TRACE 2
#define GOT_STATUS_3D 3
#define GOT_STATUS_TESSERAL 4
#define GOT_STATUS_DIFFUSION 5
#define GOT_STATUS_ORBITS 6

/* g_resave_flag values */
#define RESAVE_NO 0
#define RESAVE_YES 1
#define RESAVE_DONE 2

/* g_look_at_mouse values */
#define LOOK_MOUSE_NONE		0
#define LOOK_MOUSE_TEXT		2
#define LOOK_MOUSE_ZOOM_BOX	3

/* pause_error() values */
#define PAUSE_ERROR_NO_BATCH 0
#define PAUSE_ERROR_ANY 1
#define PAUSE_ERROR_GOODBYE 2

/* g_initialize_batch values */
#define INITBATCH_FINISH_CALC -1
#define INITBATCH_NONE 0
#define INITBATCH_NORMAL 1
#define INITBATCH_SAVE 2
#define INITBATCH_BAILOUT_ERROR 3
#define INITBATCH_BAILOUT_INTERRUPTED 4
#define INITBATCH_BAILOUT_SAVE 5

/* driver_buzzer() codes */
#define BUZZER_COMPLETE 0
#define BUZZER_INTERRUPT 1
#define BUZZER_ERROR 2

/* stopmsg() flags */
#define STOPMSG_NO_STACK	1
#define STOPMSG_CANCEL		2
#define STOPMSG_NO_BUZZER	4
#define STOPMSG_FIXED_FONT	8
#define STOPMSG_INFO_ONLY	16

/* g_video_type video types */
#define VIDEO_TYPE_HGC		1
#define VIDEO_TYPE_EGA		3
#define VIDEO_TYPE_CGA		2
#define VIDEO_TYPE_MCGA		4
#define VIDEO_TYPE_VGA		5

/* for gotos in former FRACTINT.C pieces */
#define RESTART           1
#define IMAGESTART        2
#define RESTORESTART      3
#define CONTINUE          4

#define SLIDES_OFF		0
#define SLIDES_PLAY		1
#define SLIDES_RECORD	2

/* fullscreen_choice options */
#define CHOICE_RETURN_KEY	1
#define CHOICE_MENU			2
#define CHOICE_HELP			4
#define CHOICE_INSTRUCTIONS	8
#define CHOICE_CRUNCH		16
#define CHOICE_NOT_SORTED	32

/* g_calculation_status values */
#define CALCSTAT_NO_FRACTAL		-1
#define CALCSTAT_PARAMS_CHANGED	0
#define CALCSTAT_IN_PROGRESS	1
#define CALCSTAT_RESUMABLE		2
#define CALCSTAT_NON_RESUMABLE	3
#define CALCSTAT_COMPLETED		4

/* process_command() return values */
#define COMMAND_ERROR			-1
#define COMMAND_OK				0
#define COMMAND_FRACTAL_PARAM	1
#define COMMAND_3D_PARAM			2
#define COMMAND_3D_YES			4
#define COMMAND_RESET			8

#define CMDFILE_AT_CMDLINE 0
#define CMDFILE_SSTOOLS_INI 1
#define CMDFILE_AT_AFTER_STARTUP 2
#define CMDFILE_AT_CMDLINE_SETNAME 3

#define INPUTFIELD_NUMERIC 1
#define INPUTFIELD_INTEGER 2
#define INPUTFIELD_DOUBLE 4

#define SOUNDFLAG_OFF		0
#define SOUNDFLAG_BEEP		1
#define SOUNDFLAG_X			2
#define SOUNDFLAG_Y			3
#define SOUNDFLAG_Z			4
#define SOUNDFLAG_ORBITMASK 0x07
#define SOUNDFLAG_SPEAKER	0x08
#define SOUNDFLAG_OPL3_FM	0x10
#define SOUNDFLAG_MIDI		0x20
#define SOUNDFLAG_QUANTIZED 0x40
#define SOUNDFLAG_MASK		0x7F

/* these are used to declare arrays for file names */
#if defined(_WIN32)
#define FILE_MAX_PATH _MAX_PATH
#define FILE_MAX_DIR _MAX_DIR
#else
#ifdef XFRACT
#define FILE_MAX_PATH  256       /* max length of path+filename  */
#define FILE_MAX_DIR   256       /* max length of directory name */
#else
#define FILE_MAX_PATH  80       /* max length of path+filename  */
#define FILE_MAX_DIR   80       /* max length of directory name */
#endif
#endif
#define FILE_MAX_DRIVE  3       /* max length of drive letter   */

/*
The filename limits were increased in Xfract 3.02. But alas,
in this poor program that was originally developed on the
nearly-brain-dead DOS operating system, quite a few things
in the UI would break if file names were bigger than DOS 8-3
names. So for now humor us and let's keep the names short.
*/
#define FILE_MAX_FNAME  64       /* max length of filename       */
#define FILE_MAX_EXT    64       /* max length of extension      */

#define MAXMAXLINELENGTH  128   /* upper limit for g_max_line_length for PARs */
#define MINMAXLINELENGTH  40    /* lower limit for g_max_line_length for PARs */

#define MSGLEN 80               /* handy buffer size for messages */
#define MAXCMT 57               /* length of par comments       */
#define MAXPARAMS 10            /* maximum number of parameters */
#define MAXPIXELS   32767       /* Maximum pixel count across/down the screen */
#define OLDMAXPIXELS 2048       /* Limit of some old fixed arrays */
#define MINPIXELS 10            /* Minimum pixel count across/down the screen */
#define DEFAULTASPECT 0.75f		/* Assumed overall screen dimensions, y/x  */
#define DEFAULTASPECTDRIFT 0.02f /* drift of < 2% is forced to 0% */

typedef struct tagDriver Driver;

struct videoinfo
{              /* All we need to know about a Video Adapter */
    char    name[26];       /* Adapter name (IBM EGA, etc)          */
    char    comment[26];    /* Comments (UNTESTED, etc)             */
    int     keynum;         /* key number used to invoked this mode */
                            /* 2-10 = F2-10, 11-40 = S,C,A{F1-F10}  */
    int     videomodeax;    /* begin with INT 10H, AX=(this)        */
    int     videomodebx;    /*              ...and BX=(this)        */
    int     videomodecx;    /*              ...and CX=(this)        */
    int     videomodedx;    /*              ...and DX=(this)        */
                            /* NOTE:  IF AX==BX==CX==0, SEE BELOW   */
    int     dotmode;        /* video access method used by asm code */
                            /*      1 == BIOS 10H, AH=12,13 (SLOW)  */
                            /*      2 == access like EGA/VGA        */
                            /*      3 == access like MCGA           */
                            /*      4 == Tseng-like  SuperVGA*256   */
                            /*      5 == P'dise-like SuperVGA*256   */
                            /*      6 == Vega-like   SuperVGA*256   */
                            /*      7 == "Tweaked" IBM-VGA ...*256  */
                            /*      8 == "Tweaked" SuperVGA ...*256 */
                            /*      9 == Targa Format               */
                            /*      10 = Hercules                   */
                            /*      11 = "disk video" (no screen)   */
                            /*      12 = 8514/A                     */
                            /*      13 = CGA 320x200x4, 640x200x2   */
                            /*      14 = Tandy 1000                 */
                            /*      15 = TRIDENT  SuperVGA*256      */
                            /*      16 = Chips&Tech SuperVGA*256    */
    int     x_dots;          /* number of dots across the screen     */
    int     y_dots;          /* number of dots down the screen       */
    int     colors;         /* number of g_colors available           */
	Driver *driver;
};

typedef struct videoinfo VIDEOINFO;
#define INFO_ID         "Fractal"
typedef    struct fractal_info FRACTAL_INFO;

/*
 * Note: because non-MSDOS machines store structures differently, we have
 * to do special processing of the fractal_info structure in loadfile.c.
 * Make sure changes to the structure here get reflected there.
 */
#ifndef XFRACT
#define FRACTAL_INFO_SIZE sizeof(FRACTAL_INFO)
#else
/* This value should be the MSDOS size, not the Unix size. */
#define FRACTAL_INFO_SIZE 504
#endif

#define FRACTAL_INFO_VERSION 17  /* file version, independent of system */
   /* increment this EVERY time the fractal_info structure changes */

/* TODO: instead of hacking the padding here, adjust the code that reads
   this structure */
#if defined(_WIN32)
#pragma pack(push, 1)
#endif
struct fractal_info         /*  for saving data in GIF file     */
{
    char  info_id[8];       /* Unique identifier for info g_block */
    short iterationsold;    /* Pre version 18.24 */
    short fractal_type;     /* 0=Mandelbrot 1=Julia 2= ... */
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    double c_real;
    double c_imag;
    short videomodeax;
    short videomodebx;
    short videomodecx;
    short videomodedx;
    short dotmode;
    short x_dots;
    short y_dots;
    short colors;
    short version;          /* used to be 'future[0]' */
    float parm3;
    float parm4;
    float potential[3];
    short random_seed;
    short random_flag;
    short biomorph;
    short inside;
    short logmapold;
    float invert[3];
    short decomposition[2];
    short symmetry;
                        /* version 2 stuff */
    short init_3d[16];
    short previewfactor;
    short xtrans;
    short ytrans;
    short red_crop_left;
    short red_crop_right;
    short blue_crop_left;
    short blue_crop_right;
    short red_bright;
    short blue_bright;
    short xadjust;
    short eyeseparation;
    short glassestype;
                        /* version 3 stuff, release 13 */
    short outside;
                        /* version 4 stuff, release 14 */
    double x_3rd;          /* 3rd corner */
    double y_3rd;
    char stdcalcmode;     /* 1/2/g/b */
    char use_initial_orbit_z;    /* init Mandelbrot orbit flag */
    short calculation_status;    /* resumable, finished, etc */
    long tot_extend_len;  /* total length of extension blocks in .gif file */
    short distestold;
    short float_flag;
    short bailoutold;
    long calculation_time;
    BYTE trig_index[4];      /* which trig functions selected */
    short finattract;
    double initial_orbit_z[2];  /* init Mandelbrot orbit values */
    short periodicity;    /* periodicity checking */
                        /* version 5 stuff, release 15 */
    short potential_16bit;       /* save 16 bit continuous potential info */
    float faspectratio;   /* g_final_aspect_ratio, y/x */
    short system;         /* 0 for dos, 1 for windows */
    short release;        /* release number, with 2 decimals implied */
    short flag3d;         /* stored only for now, for future use */
    short transparent[2];
    short ambient;
    short haze;
    short randomize;
                        /* version 6 stuff, release 15.x */
    short rotate_lo;
    short rotate_hi;
    short distance_test_width;
                        /* version 7 stuff, release 16 */
    double dparm3;
    double dparm4;
                        /* version 8 stuff, release 17 */
    short fill_color;
                        /* version 9 stuff, release 18 */
    double mxmaxfp;
    double mxminfp;
    double mymaxfp;
    double myminfp;
    short zdots;
    float originfp;
    float depthfp;
    float heightfp;
    float widthfp;
    float screen_distance_fp;
    float eyesfp;
    short orbittype;
    short juli3Dmode;
    short max_fn;
    short inversejulia;
    double dparm5;
    double dparm6;
    double dparm7;
    double dparm8;
    double dparm9;
    double dparm10;
                        /* version 10 stuff, release 19 */
    long bail_out;
    short bailoutest;
    long iterations;
    short bf_math;
    short bflength;
    short yadjust;        /* yikes! we left this out ages ago! */
    short old_demm_colors;
    long logmap;
    long distance_test;
    double dinvert[3];
    short logcalc;
    short stop_pass;
    short quick_calculate;
    double proximity;
    short no_bof;
    long orbit_interval;
    short orbit_delay;
    double math_tolerance[2];
    short future[7];     /* for stuff we haven't thought of yet */
};
#if defined(_WIN32)
#pragma pack(pop)
#endif

#define ITEMNAMELEN 18   /* max length of names in .frm/.l/.ifs/.fc */
struct history_info
{
    short fractal_type;
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    double c_real;
    double c_imag;
    double potential[3];
    short random_seed;
    short random_flag;
    short biomorph;
    short inside;
    long logmap;
    double invert[3];
    short decomposition;
    short symmetry;
    short init_3d[16];
    short previewfactor;
    short xtrans;
    short ytrans;
    short red_crop_left;
    short red_crop_right;
    short blue_crop_left;
    short blue_crop_right;
    short red_bright;
    short blue_bright;
    short xadjust;
    short eyeseparation;
    short glassestype;
    short outside;
    double x_3rd;
    double y_3rd;
    long distance_test;
    short bailoutold;
    BYTE trig_index[4];
    short finattract;
    double initial_orbit_z[2];
    short periodicity;
    short potential_16bit;
    short release;
    short save_release;
    short flag3d;
    short transparent[2];
    short ambient;
    short haze;
    short randomize;
    short rotate_lo;
    short rotate_hi;
    short distance_test_width;
    double dparm3;
    double dparm4;
    short fill_color;
    double mxmaxfp;
    double mxminfp;
    double mymaxfp;
    double myminfp;
    short zdots;
    float originfp;
    float depthfp;
    float heightfp;
    float widthfp;
    float screen_distance_fp;
    float eyesfp;
    short orbittype;
    short juli3Dmode;
    short major_method;
    short minor_method;
    double dparm5;
    double dparm6;
    double dparm7;
    double dparm8;
    double dparm9;
    double dparm10;
    long bail_out;
    short bailoutest;
    long iterations;
    short bf_math;
    short bflength;
    short yadjust;
    short old_demm_colors;
    char filename[FILE_MAX_PATH];
    char itemname[ITEMNAMELEN+1];
    unsigned char dac[256][3];
    char  max_fn;
    char stdcalcmode;
    char three_pass;
    char use_initial_orbit_z;
    short logcalc;
    short stop_pass;
    short ismand;
    double proximity;
    short no_bof;
    double math_tolerance[2];
    short orbit_delay;
    long orbit_interval;
    double oxmin;
    double oxmax;
    double oymin;
    double oymax;
    double ox3rd;
    double oy3rd;
    short keep_scrn_coords;
    char drawmode;
};

typedef struct history_info HISTORY;

struct formula_info         /*  for saving formula data in GIF file     */
{
    char  form_name[40];
    short uses_p1;
    short uses_p2;
    short uses_p3;
    short uses_is_mand;
    short ismand;
    short uses_p4;
    short uses_p5;
    short future[6];       /* for stuff we haven't thought of, yet */
};

enum stored_at_values
{
   NOWHERE,
   MEMORY,
   DISK
};

#define NUMGENES 21

typedef    struct evolution_info EVOLUTION_INFO;
/*
 * Note: because non-MSDOS machines store structures differently, we have
 * to do special processing of the evolution_info structure in loadfile.c and
 * encoder.c.  See decode_evolver_info() in general.c.
 * Make sure changes to the structure here get reflected there.
 */
#ifndef XFRACT
#define EVOLVER_INFO_SIZE sizeof(evolution_info)
#else
/* This value should be the MSDOS size, not the Unix size. */
#define EVOLVER_INFO_SIZE 200
#endif

struct evolution_info      /* for saving evolution data in a GIF file */
{
   short evolving;
   short gridsz;
   unsigned short this_generation_random_seed;
   double fiddle_factor;
   double parameter_range_x;
   double parameter_range_y;
   double opx;
   double opy;
   short odpx;
   short odpy;
   short px;
   short py;
   short sxoffs;
   short syoffs;
   short x_dots;
   short y_dots;
   short mutate[NUMGENES];
   short ecount; /* count of how many images have been calc'ed so far */
   short future[68 - NUMGENES];      /* total of 200 bytes */
};


typedef    struct orbits_info ORBITS_INFO;
/*
 * Note: because non-MSDOS machines store structures differently, we have
 * to do special processing of the orbits_info structure in loadfile.c and
 * encoder.c.  See decode_orbits_info() in general.c.
 * Make sure changes to the structure here get reflected there.
 */
#ifndef XFRACT
#define ORBITS_INFO_SIZE sizeof(orbits_info)
#else
/* This value should be the MSDOS size, not the Unix size. */
#define ORBITS_INFO_SIZE 200
#endif

struct orbits_info      /* for saving orbits data in a GIF file */
{
   double oxmin;
   double oxmax;
   double oymin;
   double oymax;
   double ox3rd;
   double oy3rd;
   short keep_scrn_coords;
   char drawmode;
   char dummy; /* need an even number of bytes */
   short future[74];      /* total of 200 bytes */
};

#define MAXVIDEOMODES 300       /* maximum entries in fractint.cfg        */

#define AUTOINVERT -123456.789
#define ENDVID 22400   /* video table uses extra seg up to here */

#define N_ATTR 8                        /* max number of attractors     */

extern  long     g_attractor_radius_l;      /* finite attractor radius  */
extern  double   g_attractor_radius_fp;      /* finite attractor radius  */

#define NUMIFS    64     /* number of ifs functions in ifs array */
#define IFSPARM    7     /* number of ifs parameters */
#define IFS3DPARM 13     /* number of ifs 3D parameters */

struct tag_more_parameters
{
   int      type;                       /* index in fractalname of the fractal */
   char     *parameters[MAXPARAMS-4];    /* name of the parameters */
   double   paramvalue[MAXPARAMS-4];    /* default parameter values */
};
typedef struct tag_more_parameters more_parameters;

struct fractalspecificstuff
{
   char  *name;                         /* name of the fractal */
                                        /* (leading "*" supresses name display) */
   char  *parameters[4];                 /* name of the parameters */
   double paramvalue[4];                /* default parameter values */
   int   helptext;                      /* helpdefs.h HT_xxxx, -1 for none */
   int   helpformula;                   /* helpdefs.h HF_xxxx, -1 for none */
   unsigned flags;                      /* constraints, bits defined below */
   float x_min;                          /* default XMIN corner */
   float x_max;                          /* default XMAX corner */
   float y_min;                          /* default YMIN corner */
   float y_max;                          /* default YMAX corner */
   int   isinteger;                     /* 1 if g_integer_fractal, 0 otherwise */
   int   tojulia;                       /* mandel-to-julia switch */
   int   tomandel;                      /* julia-to-mandel switch */
   int   tofloat;                       /* integer-to-floating switch */
   int   symmetry;                      /* applicable symmetry logic
                                           0 = no symmetry
                                          -1 = y-axis symmetry (If No Params)
                                           1 = y-axis symmetry
                                          -2 = x-axis symmetry (No Parms)
                                           2 = x-axis symmetry
                                          -3 = y-axis AND x-axis (No Parms)
                                           3 = y-axis AND x-axis symmetry
                                          -4 = polar symmetry (No Parms)
                                           4 = polar symmetry
                                           5 = PI (sin/cos) symmetry
                                           6 = NEWTON (power) symmetry
                                                                */
   int (*orbitcalc)(void);      /* function that calculates one orbit */
   int (*per_pixel)(void);      /* once-per-pixel init */
   int (*per_image)(void);      /* once-per-image setup */
   int (*calculate_type)(void);       /* name of main fractal function */
   int orbit_bailout;           /* usual bailout value for orbit calc */
};

struct tag_alternate_math_info
{
   int type;                    /* index in fractalname of the fractal */
   int math;                    /* kind of math used */
   int (*orbitcalc)(void);      /* function that calculates one orbit */
   int (*per_pixel)(void);      /* once-per-pixel init */
   int (*per_image)(void);      /* once-per-image setup */
};

typedef struct tag_alternate_math_info alternate_math;

/* defines for symmetry */
#define  NOSYM          0
#define  XAXIS_NOPARM  -1
#define  XAXIS          1
#define  YAXIS_NOPARM  -2
#define  YAXIS          2
#define  XYAXIS_NOPARM -3
#define  XYAXIS         3
#define  ORIGIN_NOPARM -4
#define  ORIGIN         4
#define  PI_SYM_NOPARM -5
#define  PI_SYM         5
#define  XAXIS_NOIMAG  -6
#define  XAXIS_NOREAL   6
#define  NOPLOT        99
#define  SETUP_SYM    100

/* defines for inside/outside */
#define ITER        -1
#define REAL        -2
#define IMAG        -3
#define MULT        -4
#define SUM         -5
#define ATAN        -6
#define FMOD        -7
#define TDIS        -8
#define ZMAG       -59
#define BOF60      -60
#define BOF61      -61
#define EPSCROSS  -100
#define STARTRAIL -101
#define PERIOD    -102
#define FMODI     -103
#define ATANI     -104

/* defines for bailoutest */
enum bailouts { Mod, Real, Imag, Or, And, Manh, Manr };
enum Major  {breadth_first, depth_first, random_walk, random_run};
enum Minor  {left_first, right_first};

/* bitmask defines for g_fractal_specific flags */
#define  NOZOOM         1    /* zoombox not allowed at all          */
#define  NOGUESS        2    /* solid guessing not allowed          */
#define  NOTRACE        4    /* boundary tracing not allowed        */
#define  NOROTATE       8    /* zoombox rotate/stretch not allowed  */
#define  NORESUME    0x10    /* can't interrupt and resume          */
#define  INFCALC     0x20    /* this type calculates forever        */
#define  TRIG1       0x40    /* number of trig functions in formula */
#define  TRIG2       0x80
#define  TRIG3       0xC0
#define  TRIG4      0x100
#define  WINFRAC    0x200    /* supported in WinFrac                */
#define  PARMS3D    0x400    /* uses 3d parameters                  */
#define  OKJB       0x800    /* works with Julibrot                 */
#define  MORE      0x1000    /* more than 4 parms                   */
#define  BAILTEST  0x2000    /* can use different bailout tests     */
#define  BF_MATH   0x4000    /* supports arbitrary precision        */
#define  LD_MATH   0x8000    /* supports long double                */


/* more bitmasks for evolution mode flag */
#define EVOLVE_NONE			0	/* no evolution */
#define EVOLVE_FIELD_MAP	1	/*steady field varyiations across screen */
#define EVOLVE_RAND_WALK	2	/* newparm = lastparm +- rand()                   */
#define EVOLVE_RAND_PARAM	4	/* newparm = constant +- rand()                   */
#define EVOLVE_NO_GROUT		8	/* no gaps between images                                   */
#define EVOLVE_PARM_BOX		128


extern struct fractalspecificstuff g_fractal_specific[];
extern struct fractalspecificstuff *g_current_fractal_specific;

#define DEFAULTFRACTALTYPE      ".gif"
#define ALTERNATEFRACTALTYPE    ".fra"


#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#ifndef lsqr
#define lsqr(x) (multiply((x), (x), g_bit_shift))
#endif

#define CMPLXmod(z)     (sqr((z).x)+sqr((z).y))
#define CMPLXconj(z)    ((z).y =  -((z).y))
#define LCMPLXmod(z)    (lsqr((z).x)+lsqr((z).y))
#define LCMPLXconj(z)   ((z).y =  -((z).y))

#define PER_IMAGE   (g_fractal_specific[g_fractal_type].per_image)
#define PER_PIXEL   (g_fractal_specific[g_fractal_type].per_pixel)
#define ORBITCALC   (g_fractal_specific[g_fractal_type].orbitcalc)

typedef  _LCMPLX LCMPLX;

/* 3D stuff - formerly in 3d.h */
#ifndef dot_product
#define dot_product(v1,v2)  ((v1)[0]*(v2)[0]+(v1)[1]*(v2)[1]+(v1)[2]*(v2)[2])  /* TW 7-09-89 */
#endif

#define    CMAX    4   /* maximum column (4 x 4 matrix) */
#define    RMAX    4   /* maximum row    (4 x 4 matrix) */
#define    DIM     3   /* number of dimensions */

typedef double MATRIX [RMAX] [CMAX];  /* matrix of doubles */
typedef int   IMATRIX [RMAX] [CMAX];  /* matrix of ints    */
typedef long  LMATRIX [RMAX] [CMAX];  /* matrix of longs   */

/* A MATRIX is used to describe a transformation from one coordinate
system to another.  Multiple transformations may be concatenated by
multiplying their transformation matrices. */

typedef double VECTOR [DIM];  /* vector of doubles */
typedef int   IVECTOR [DIM];  /* vector of ints    */
typedef long  LVECTOR [DIM];  /* vector of longs   */

/* A VECTOR is an array of three coordinates [x,y,z] representing magnitude
and direction. A fourth dimension is assumed to always have the value 1, but
is not in the data structure */

#ifdef PI
#undef PI
#endif
#define PI 3.14159265358979323846
#define SPHERE    g_init_3d[0]             /* sphere? 1 = yes, 0 = no  */
#define ILLUMINE  (FILLTYPE > 4)  /* illumination model       */

/* regular 3D */
#define XROT      g_init_3d[1]     /* rotate x-axis 60 degrees */
#define YROT      g_init_3d[2]     /* rotate y-axis 90 degrees */
#define ZROT      g_init_3d[3]     /* rotate x-axis  0 degrees */
#define XSCALE    g_init_3d[4]     /* scale x-axis, 90 percent */
#define YSCALE    g_init_3d[5]     /* scale y-axis, 90 percent */

/* sphere 3D */
#define PHI1      g_init_3d[1]     /* longitude start, 180     */
#define PHI2      g_init_3d[2]     /* longitude end ,   0      */
#define THETA1    g_init_3d[3]         /* latitude start,-90 degrees */
#define THETA2    g_init_3d[4]         /* latitude stop,  90 degrees */
#define RADIUS    g_init_3d[5]     /* should be user input */

/* common parameters */
#define ROUGH     g_init_3d[6]     /* scale z-axis, 30 percent */
#define WATERLINE g_init_3d[7]     /* water level              */
#define FILLTYPE  g_init_3d[8]     /* fill type                */
#define ZVIEWER   g_init_3d[9]     /* perspective view point   */
#define XSHIFT    g_init_3d[10]    /* x shift */
#define YSHIFT    g_init_3d[11]    /* y shift */
#define XLIGHT    g_init_3d[12]    /* x light vector coordinate */
#define YLIGHT    g_init_3d[13]    /* y light vector coordinate */
#define ZLIGHT    g_init_3d[14]    /* z light vector coordinate */
#define LIGHTAVG  g_init_3d[15]    /* number of points to average */

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* Math definitions (normally in float.h) that are missing on some systems. */
#ifndef FLT_MIN
#define FLT_MIN 1.17549435e-38
#endif
#ifndef FLT_MAX
#define FLT_MAX 3.40282347e+38
#endif
#ifndef DBL_EPSILON
#define DBL_EPSILON 2.2204460492503131e-16
#endif

#ifndef XFRACT
#define UPARR "\x18"
#define DNARR "\x19"
#define RTARR "\x1A"
#define LTARR "\x1B"
#define UPARR1 "\x18"
#define DNARR1 "\x19"
#define RTARR1 "\x1A"
#define LTARR1 "\x1B"
#define FK_F1  "F1"
#define FK_F2  "F2"
#define FK_F3  "F3"
#define FK_F4  "F4"
#define FK_F5  "F5"
#define FK_F6  "F6"
#define FK_F7  "F7"
#define FK_F8  "F8"
#define FK_F9  "F9"
#else
#define UPARR "K"
#define DNARR "J"
#define RTARR "L"
#define LTARR "H"
#define UPARR1 "up(K)"
#define DNARR1 "down(J)"
#define RTARR1 "left(L)"
#define LTARR1 "right(H)"
#define FK_F1  "Shift-1"
#define FK_F2  "Shift-2"
#define FK_F3  "Shift-3"
#define FK_F4  "Shift-4"
#define FK_F5  "Shift-5"
#define FK_F6  "Shift-6"
#define FK_F7  "Shift-7"
#define FK_F8  "Shift-8"
#define FK_F9  "Shift-9"
#endif

#ifndef XFRACT
#define Fractint  "Fractint"
#define FRACTINT  "FRACTINT"
#else
#define Fractint  "Xfractint"
#define FRACTINT  "XFRACTINT"
#endif

#define JIIM  0
#define ORBIT 1

struct workliststuff    /* work list entry for std escape time engines */
{
	int xx_start;    /* screen window for this entry */
	int xx_stop;
	int yy_start;
	int yy_stop;
	int yy_begin;    /* start row within window, for 2pass/ssg resume */
	int sym;        /* if symmetry in window, prevents bad combines */
	int pass;       /* for 2pass and solid guessing */
	int xx_begin;    /* start col within window, =0 except on resume */
};

typedef struct workliststuff        WORKLIST;


#define MAXCALCWORK 12

struct coords {
	int x, y;
};

struct dblcoords {
	double x, y;
};

extern BYTE g_trig_index[];
extern void (*ltrig0)(void), (*ltrig1)(void), (*ltrig2)(void), (*ltrig3)(void);
extern void (*dtrig0)(void), (*dtrig1)(void), (*dtrig2)(void), (*dtrig3)(void);

struct trig_funct_lst
{
    char *name;
    void (*lfunct)(void);
    void (*dfunct)(void);
    void (*mfunct)(void);
} ;
extern struct trig_funct_lst trigfn[];

/* function prototypes */

extern  void   (_fastcall *plot)(int, int, int);

/* for overlay return stack */

#define BIG 100000.0

#define CTL(x) ((x)&0x1f)

/* nonalpha tests if we have a control character */
#define nonalpha(c) ((c)<32 || (c)>127)

/* keys; FIK = "FractInt Key"
 * Use this prefix to disambiguate key name symbols used in the fractint source
 * from symbols defined by the external environment, i.e. "DELETE" on Win32
 */
#define FIK_ALT_A			1030
#define FIK_ALT_S			1031
#define FIK_ALT_F1			1104
#define FIK_ALT_F2			1105
#define FIK_ALT_F3			1106
#define FIK_ALT_F4			1107
#define FIK_ALT_F5			1108
#define FIK_ALT_F6			1109
#define FIK_ALT_F7			1110
#define FIK_ALT_F8			1111
#define FIK_ALT_F9			1112
#define FIK_ALT_F10			1113
#define FIK_ALT_1			1120
#define FIK_ALT_2			1121
#define FIK_ALT_3			1122
#define FIK_ALT_4			1123
#define FIK_ALT_5			1124
#define FIK_ALT_6			1125
#define FIK_ALT_7			1126

#define FIK_CTL_A			1
#define FIK_CTL_B			2
#define FIK_CTL_E			5
#define FIK_CTL_F			6
#define FIK_CTL_G			7
#define FIK_CTL_O			15
#define FIK_CTL_P			16
#define FIK_CTL_S			19
#define FIK_CTL_U			21
#define FIK_CTL_X			24
#define FIK_CTL_Y			25
#define FIK_CTL_Z			26
#define FIK_CTL_BACKSLASH	28
#define FIK_CTL_DEL			1147
#define FIK_CTL_DOWN_ARROW	1145
#define FIK_CTL_END			1117
#define FIK_CTL_ENTER		10
#define FIK_CTL_ENTER_2		1010
#define FIK_CTL_F1			1094
#define FIK_CTL_F2			1095
#define FIK_CTL_F3			1096
#define FIK_CTL_F4			1097
#define FIK_CTL_F5			1098
#define FIK_CTL_F6			1099
#define FIK_CTL_F7			1100
#define FIK_CTL_F8			1101
#define FIK_CTL_F9			1102
#define FIK_CTL_F10			1103
#define FIK_CTL_HOME		1119
#define FIK_CTL_INSERT		1146
#define FIK_CTL_LEFT_ARROW	1115
#define FIK_CTL_MINUS		1142
#define FIK_CTL_PAGE_DOWN	1118
#define FIK_CTL_PAGE_UP		1132
#define FIK_CTL_PLUS		1144
#define FIK_CTL_RIGHT_ARROW	1116
#define FIK_CTL_TAB			1148
#define FIK_CTL_UP_ARROW	1141

#define FIK_SHF_TAB			1015  /* shift tab aka BACKTAB */

#define FIK_BACKSPACE		8
#define FIK_DELETE			1083
#define FIK_DOWN_ARROW		1080
#define FIK_END				1079
#define FIK_ENTER			13
#define FIK_ENTER_2			1013
#define FIK_ESC				27
#define FIK_F1				1059
#define FIK_F2				1060
#define FIK_F3				1061
#define FIK_F4				1062
#define FIK_F5				1063
#define FIK_F6				1064
#define FIK_F7				1065
#define FIK_F8				1066
#define FIK_F9				1067
#define FIK_F10				1068
#define FIK_HOME			1071
#define FIK_INSERT			1082
#define FIK_LEFT_ARROW		1075
#define FIK_PAGE_DOWN		1081
#define FIK_PAGE_UP			1073
#define FIK_RIGHT_ARROW		1077
#define FIK_SPACE			32
#define FIK_SF1				1084
#define FIK_SF2				1085
#define FIK_SF3				1086
#define FIK_SF4				1087
#define FIK_SF5				1088
#define FIK_SF6				1089
#define FIK_SF7				1090
#define FIK_SF8				1091
#define FIK_SF9				1092
#define FIK_SF10			1093
#define FIK_TAB				9
#define FIK_UP_ARROW		1072

/* not really a key, but a special trigger */
#define FIK_SAVE_TIME		9999

/* text g_colors */
#define BLACK      0
#define BLUE       1
#define GREEN      2
#define CYAN       3
#define RED        4
#define MAGENTA    5
#define BROWN      6 /* dirty yellow on cga */
#define WHITE      7
/* use values below this for foreground only, they don't work background */
#define GRAY       8 /* don't use this much - is black on cga */
#define L_BLUE     9
#define L_GREEN   10
#define L_CYAN    11
#define L_RED     12
#define L_MAGENTA 13
#define YELLOW    14
#define L_WHITE   15
#define INVERSE 0x8000 /* when 640x200x2 text or mode 7, inverse */
#define BRIGHT  0x4000 /* when mode 7, bright */
/* and their use: */
#define C_TITLE           g_text_colors[0]+BRIGHT
#define C_TITLE_DEV       g_text_colors[1]
#define C_HELP_HDG        g_text_colors[2]+BRIGHT
#define C_HELP_BODY       g_text_colors[3]
#define C_HELP_INSTR      g_text_colors[4]
#define C_HELP_LINK       g_text_colors[5]+BRIGHT
#define C_HELP_CURLINK    g_text_colors[6]+INVERSE
#define C_PROMPT_BKGRD    g_text_colors[7]
#define C_PROMPT_TEXT     g_text_colors[8]
#define C_PROMPT_LO       g_text_colors[9]
#define C_PROMPT_MED      g_text_colors[10]
#ifndef XFRACT
#define C_PROMPT_HI       g_text_colors[11]+BRIGHT
#else
#define C_PROMPT_HI       g_text_colors[11]
#endif
#define C_PROMPT_INPUT    g_text_colors[12]+INVERSE
#define C_PROMPT_CHOOSE   g_text_colors[13]+INVERSE
#define C_CHOICE_CURRENT  g_text_colors[14]+INVERSE
#define C_CHOICE_SP_INSTR g_text_colors[15]
#define C_CHOICE_SP_KEYIN g_text_colors[16]+BRIGHT
#define C_GENERAL_HI      g_text_colors[17]+BRIGHT
#define C_GENERAL_MED     g_text_colors[18]
#define C_GENERAL_LO      g_text_colors[19]
#define C_GENERAL_INPUT   g_text_colors[20]+INVERSE
#define C_DVID_BKGRD      g_text_colors[21]
#define C_DVID_HI         g_text_colors[22]+BRIGHT
#define C_DVID_LO         g_text_colors[23]
#define C_STOP_ERR        g_text_colors[24]+BRIGHT
#define C_STOP_INFO       g_text_colors[25]+BRIGHT
#define C_TITLE_LOW       g_text_colors[26]
#define C_AUTHDIV1        g_text_colors[27]+INVERSE
#define C_AUTHDIV2        g_text_colors[28]+INVERSE
#define C_PRIMARY         g_text_colors[29]
#define C_CONTRIB         g_text_colors[30]

/* structure for xmmmoveextended parameter */
struct XMM_Move
  {
    unsigned long   Length;
    unsigned int    SourceHandle;
    unsigned long   SourceOffset;
    unsigned int    DestHandle;
    unsigned long   DestOffset;
  };

/* structure passed to fullscreen_prompts */
struct fullscreenvalues
{
   int type;   /* 'd' for double, 'f' for float, 's' for string,   */
               /* 'D' for integer in double, '*' for comment */
               /* 'i' for integer, 'y' for yes=1 no=0              */
               /* 0x100+n for string of length n                   */
               /* 'l' for one of a list of strings                 */
               /* 'L' for long */
   union
   {
      double dval;      /* when type 'd' or 'f'  */
      int    ival;      /* when type is 'i'      */
      long   Lval;      /* when type is 'L'      */
      char   sval[16];  /* when type is 's'      */
      char  *sbuf;  /* when type is 0x100+n  */
      struct {          /* when type is 'l'      */
         int  val;      /*   selected choice     */
         int  vlen;     /*   char len per choice */
         char **list;   /*   list of values      */
         int  llen;     /*   number of values    */
      } ch;
   } uval;
};

#define   FILEATTR       0x37      /* File attributes; select all but volume labels */
#define   HIDDEN         2
#define   SYSTEM         4
#define   SUBDIR         16

struct DIR_SEARCH				/* Allocate	DTA	and	define structure */
{
	char path[FILE_MAX_PATH];		/* DOS path	and	filespec */
	char attribute;				/* File	attributes wanted */
	int	 ftime;					/* File	creation time */
	int	 fdate;					/* File	creation date */
	long size;					/* File	size in bytes */
	char filename[FILE_MAX_PATH];	/* Filename	and	extension */
};

extern struct DIR_SEARCH DTA;   /* Disk Transfer Area */

typedef struct palett
{
   BYTE red;
   BYTE green;
   BYTE blue;
}
Palettetype;

#define MAX_JUMPS 200  /* size of JUMP_CONTROL array */

typedef struct frm_jmpptrs_st {
   int      JumpOpPtr;
   int      JumpLodPtr;
   int      JumpStoPtr;
} JUMP_PTRS_ST;


typedef struct frm_jump_st {
   int      type;
   JUMP_PTRS_ST ptrs;
   int      DestJumpIndex;
} JUMP_CONTROL_ST;

#if defined(_WIN32)
#pragma pack(push, 1)
#endif
struct ext_blk_resume_info
{
	char got_data;
	int length;
	char *resume_data;
};

struct ext_blk_formula_info
{
	char got_data;
	int length;
	char form_name[40];
	short uses_p1;
	short uses_p2;
	short uses_p3;
	short uses_is_mand;
	short ismand;
	short uses_p4;
	short uses_p5;
};

struct ext_blk_ranges_info
{
	char got_data;
	int length;
	int *range_data;
};

struct ext_blk_mp_info {
	char got_data;
	int length;
	char *apm_data;
};

/* parameter evolution stuff */
struct ext_blk_evolver_info
{
	char got_data;
	int length;
	short evolving;
	short gridsz;
	unsigned short this_generation_random_seed;
	double fiddle_factor;
	double parameter_range_x;
	double parameter_range_y;
	double opx;
	double opy;
	short  odpx;
	short  odpy;
	short  px;
	short  py;
	short  sxoffs;
	short  syoffs;
	short  x_dots;
	short  y_dots;
	short  ecount;
	short  mutate[NUMGENES];
};

struct ext_blk_orbits_info
{
	char got_data;
	int length;
	double oxmin;
	double oxmax;
	double oymin;
	double oymax;
	double ox3rd;
	double oy3rd;
	short keep_scrn_coords;
	char drawmode;
};
#if defined(_WIN32)
#pragma pack(pop)
#endif

struct search_path {
   char par[FILE_MAX_PATH];
   char frm[FILE_MAX_PATH];
   char ifs[FILE_MAX_PATH];
   char lsys[FILE_MAX_PATH];
} ;

struct affine
{
   /* weird order so a,b,e and c,d,f are vectors */
   double a;
   double b;
   double e;
   double c;
   double d;
   double f;
};

struct baseunit { /* smallest part of a fractint 'gene' */
   void *addr               ; /* address of variable to be referenced */
   void (*varyfunc)(struct baseunit*,int,int); /* pointer to func used to vary it */
                              /* takes random number and pointer to var*/
   int mutate ;  /* flag to switch on variation of this variable */
                  /* 0 for no mutation, 1 for x axis, 2 for y axis */
                  /* in steady field maps, either x or y=yes in random modes*/ 
   char name[16]; /* name of variable (for menu ) */
   char level;    /* mutation level at which this should become active */
};

typedef struct baseunit    GENEBASE;

#define sign(x) (((x) < 0) ? -1 : ((x) != 0)  ? 1 : 0)

#endif


#if _MSC_VER == 800
#ifndef FIXTAN_DEFINED
/* !!!!! stupid MSVC tan(x) bug fix !!!!!!!!            */
/* tan(x) can return -tan(x) if -pi/2 < x < pi/2       */
/* if tan(x) has been called before outside this range. */
double fixtan( double x );
#define tan fixtan
#define FIXTAN_DEFINED
#endif
#endif
