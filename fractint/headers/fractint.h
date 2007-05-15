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

#define COLOR_CHANNEL_MAX 63

/* symmetry values */
enum SymmetryType
{
	SYMMETRY_X_AXIS_NO_IMAGINARY = -6,
	SYMMETRY_PI_NO_PARAMETER = -5,
	SYMMETRY_ORIGIN_NO_PARAMETER = -4,
	SYMMETRY_XY_AXIS_NO_PARAMETER = -3,
	SYMMETRY_Y_AXIS_NO_PARAMETER = -2,
	SYMMETRY_X_AXIS_NO_PARAMETER = -1,
	SYMMETRY_NONE = 0,
	SYMMETRY_X_AXIS = 1,
	SYMMETRY_Y_AXIS = 2,
	SYMMETRY_XY_AXIS = 3,
	SYMMETRY_ORIGIN = 4,
	SYMMETRY_PI = 5,
	SYMMETRY_X_AXIS_NO_REAL = 6,
	SYMMETRY_NO_PLOT = 99,
	SYMMETRY_SETUP = 100
};

/* g_file_type values */
enum FileType
{
	FILETYPE_GIF = 0
};

/* g_display_3d values */
enum Display3DType
{
	DISPLAY3D_GENERATED	= -1,
	DISPLAY3D_NONE		= 0,
	DISPLAY3D_YES		= 1,
	DISPLAY3D_OVERLAY	= 2
};

/* g_ifs_type values */
enum IFSType
{
	IFSTYPE_2D = 0,
	IFSTYPE_3D = 1
};

/* g_log_dynamic_calculate values */
enum LogDynamicType
{
	LOGDYNAMIC_NONE = 0,
	LOGDYNAMIC_DYNAMIC = 1,
	LOGDYNAMIC_TABLE = 2
};

/* g_log_palette_flag special values */
enum LogPaletteType
{
	LOGPALETTE_NONE = 0,
	LOGPALETTE_STANDARD = 1,
	LOGPALETTE_OLD = -1
};

/* g_color_state values */
enum ColorStateType
{
	COLORSTATE_DEFAULT	= 0,
	COLORSTATE_UNKNOWN	= 1,
	COLORSTATE_MAP		= 2
};

/* g_force_symmetry values */
enum ForceSymmetryType
{
	FORCESYMMETRY_NONE		= 999,
	FORCESYMMETRY_SEARCH	= 1000
};

/* g_save_dac values */
enum SaveDACType
{
	SAVEDAC_NO		= 0,
	SAVEDAC_YES		= 1,
	SAVEDAC_NEXT	= 2
};

/* g_orbit_draw_mode values */
enum OrbitDrawType
{
	ORBITDRAW_RECTANGLE	= 0,
	ORBITDRAW_LINE		= 1,
	ORBITDRAW_FUNCTION	= 2
};

/* ant types */
enum AntType
{
	ANTTYPE_MOVE_COLOR	= 0,
	ANTTYPE_MOVE_RULE	= 1
};

/* g_true_mode values */
enum TrueModeType
{
	TRUEMODE_DEFAULT	= 0,
	TRUEMODE_ITERATES	= 1
};

/* timer type values */
enum TimerType
{
	TIMER_ENGINE	= 0,
	TIMER_DECODER	= 1,
	TIMER_ENCODER	= 2
};

/* g_debug_flag values */
enum DebugFlagType
{
	DEBUGFLAG_NONE				= 0,
	DEBUGFLAG_LORENZ_FLOAT		= 22,
	DEBUGFLAG_COMPARE_RESTORED	= 50,
	DEBUGFLAG_NO_FPU			= 70,
	DEBUGFLAG_NO_ASM_MANDEL		= 90,
	DEBUGFLAG_OLD_POWER			= 94,
	DEBUGFLAG_DISK_MESSAGES		= 96,
	DEBUGFLAG_REAL_POPCORN		= 98,
	DEBUGFLAG_NO_FIRST_INIT		= 110,
	DEBUGFLAG_TIME_ENCODER		= 200,
	DEBUGFLAG_NO_MIIM_QUEUE		= 300,
	DEBUGFLAG_SKIP_OPTIMIZER	= 322,
	DEBUGFLAG_NO_HELP_F1_ESC	= 324,
	DEBUGFLAG_USE_DISK			= 420,
	DEBUGFLAG_USE_MEMORY		= 422,
	DEBUGFLAG_ABORT_SAVENAME	= 450,
	DEBUGFLAG_BNDTRACE_NONZERO	= 470,
	DEBUGFLAG_SOLID_GUESS_BR	= 472,
	DEBUGFLAG_SET_DIGITS_MIN	= 700,
	DEBUGFLAG_SET_DIGITS_MAX	= 720,
	DEBUGFLAG_MORE_DIGITS		= 750,
	DEBUGFLAG_NO_COLORS_FIX		= 910,
	DEBUGFLAG_COLORS_LOSSLESS	= 920,
	DEBUGFLAG_FORCE_FP_NEWTON	= 1010,
	DEBUGFLAG_SWAP_SIGN			= 1012,
	DEBUGFLAG_FORCE_BITSHIFT	= 1234,
	DEBUGFLAG_2222				= 2222,
	DEBUGFLAG_2224				= 2224,
	DEBUGFLAG_EDIT_TEXT_COLORS	= 3000,
	DEBUGFLAG_NO_DEV_HEADING	= 3002,
	DEBUGFLAG_NO_BIG_TO_FLOAT	= 3200,
	DEBUGFLAG_NO_INT_TO_FLOAT	= 3400,
	DEBUGFLAG_SOI_LONG_DOUBLE	= 3444,
	DEBUGFLAG_PIN_CORNERS_ONE	= 3600,
	DEBUGFLAG_NO_PIXEL_GRID		= 3800,
	DEBUGFLAG_SHOW_MATH_ERRORS	= 4000,
	DEBUGFLAG_PRE193_CENTERMAG	= 4010,
	DEBUGFLAG_OLD_TIMER			= 4020,
	DEBUGFLAG_OLD_ORBIT_SOUND	= 4030,
	DEBUGFLAG_MIN_DISKVID_CACHE	= 4200,
	DEBUGFLAG_UNOPT_POWER		= 6000,
	DEBUGFLAG_REDUCE_VIDEO_MIN	= 9002,
	DEBUGFLAG_REDUCE_VIDEO_MAX	= 9100,
	DEBUGFLAG_MEMORY			= 10000
};

/* projection values */
enum ProjectionType
{
	PROJECTION_ZX	= 0,
	PROJECTION_XZ	= 1,
	PROJECTION_XY	= 2
};

/* random_dir values */
enum DirectionType
{
	DIRECTION_LEFT		= 0,
	DIRECTION_RIGHT		= 1,
	DIRECTION_RANDOM	= 2
};

/* g_juli_3d_mode values */
enum Juli3DModeType
{
	JULI3DMODE_MONOCULAR	= 0,
	JULI3DMODE_LEFT_EYE		= 1,
	JULI3DMODE_RIGHT_EYE	= 2,
	JULI3DMODE_RED_BLUE		= 3
};

/* g_which_image values */
enum WhichImageType
{
	WHICHIMAGE_NONE = 0,
	WHICHIMAGE_RED	= 1,
	WHICHIMAGE_BLUE	= 2
};

/* g_orbit_save values */
enum OrbitSaveType
{
	ORBITSAVE_NONE	= 0,
	ORBITSAVE_RAW	= 1,
	ORBITSAVE_SOUND = 2
};

/* find_file_item itemtypes */
enum FindFileItemType
{
	ITEMTYPE_PARAMETER	= 0,
	ITEMTYPE_FORMULA	= 1,
	ITEMTYPE_L_SYSTEM	= 2,
	ITEMTYPE_IFS		= 3
};

/* getfileentry and find_file_item itemtype values */
enum GetFileEntryType
{
	GETFILE_FORMULA		= 0,
	GETFILE_L_SYSTEM	= 1,
	GETFILE_IFS			= 2,
	GETFILE_PARAMETER	= 3
};

/* g_got_status values */
enum GotStatusType
{
	GOT_STATUS_NONE = -1,
	GOT_STATUS_12PASS = 0,
	GOT_STATUS_GUESSING = 1,
	GOT_STATUS_BOUNDARY_TRACE = 2,
	GOT_STATUS_3D = 3,
	GOT_STATUS_TESSERAL = 4,
	GOT_STATUS_DIFFUSION = 5,
	GOT_STATUS_ORBITS = 6
};

/* g_resave_flag values */
enum ResaveType
{
	RESAVE_NO = 0,
	RESAVE_YES = 1,
	RESAVE_DONE = 2
};

/* g_look_at_mouse values */
enum LookMouseType
{
	LOOK_MOUSE_NONE		= 0,
	LOOK_MOUSE_TEXT		= 2,
	LOOK_MOUSE_ZOOM_BOX	= 3
};

/* pause_error() values */
enum PauseErrorType
{
	PAUSE_ERROR_NO_BATCH = 0,
	PAUSE_ERROR_ANY = 1,
	PAUSE_ERROR_GOODBYE = 2
};

/* g_initialize_batch values */
enum InitBatchType
{
	INITBATCH_FINISH_CALC = -1,
	INITBATCH_NONE = 0,
	INITBATCH_NORMAL = 1,
	INITBATCH_SAVE = 2,
	INITBATCH_BAILOUT_ERROR = 3,
	INITBATCH_BAILOUT_INTERRUPTED = 4,
	INITBATCH_BAILOUT_SAVE = 5
};

/* driver_buzzer() codes */
enum BuzzerType
{
	BUZZER_COMPLETE = 0,
	BUZZER_INTERRUPT = 1,
	BUZZER_ERROR = 2
};

/* stop_message() flags */
enum StopMessageFlag
{
	STOPMSG_NO_STACK	= 1,
	STOPMSG_CANCEL		= 2,
	STOPMSG_NO_BUZZER	= 4,
	STOPMSG_FIXED_FONT	= 8,
	STOPMSG_INFO_ONLY	= 16
};

/* for gotos in former FRACTINT.C pieces */
enum MainLoopType
{
	RESTART           = 1,
	IMAGESTART        = 2,
	RESTORESTART      = 3,
	CONTINUE          = 4
};

enum SlideType
{
	SLIDES_OFF		= 0,
	SLIDES_PLAY		= 1,
	SLIDES_RECORD	= 2
};

/* full_screen_choice options */
enum FullScreenChoiceFlag
{
	CHOICE_RETURN_KEY	= 1,
	CHOICE_MENU			= 2,
	CHOICE_HELP			= 4,
	CHOICE_INSTRUCTIONS	= 8,
	CHOICE_CRUNCH		= 16,
	CHOICE_NOT_SORTED	= 32
};

/* g_calculation_status values */
enum CalculationStatusType
{
	CALCSTAT_NO_FRACTAL		= -1,
	CALCSTAT_PARAMS_CHANGED	= 0,
	CALCSTAT_IN_PROGRESS	= 1,
	CALCSTAT_RESUMABLE		= 2,
	CALCSTAT_NON_RESUMABLE	= 3,
	CALCSTAT_COMPLETED		= 4
};

enum CommandFileType
{
	CMDFILE_AT_CMDLINE = 0,
	CMDFILE_SSTOOLS_INI = 1,
	CMDFILE_AT_AFTER_STARTUP = 2,
	CMDFILE_AT_CMDLINE_SETNAME = 3
};

enum InputFieldType
{
	INPUTFIELD_NUMERIC = 1,
	INPUTFIELD_INTEGER = 2,
	INPUTFIELD_DOUBLE = 4
};

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

#define MAX_MAX_LINE_LENGTH  128   /* upper limit for g_max_line_length for PARs */
#define MIN_MAX_LINE_LENGTH  40    /* lower limit for g_max_line_length for PARs */

#define MESSAGE_LEN 80               /* handy buffer size for messages */
#define MAX_COMMENT 57               /* length of par comments       */
#define MAX_PARAMETERS 10            /* maximum number of parameters */
#define MAX_PIXELS   32767       /* Maximum pixel count across/down the screen */
#define OLD_MAX_PIXELS 2048       /* Limit of some old fixed arrays */
#define MIN_PIXELS 10            /* Minimum pixel count across/down the screen */
#define DEFAULT_ASPECT_RATIO 0.75f		/* Assumed overall screen dimensions, y/x  */
#define DEFAULT_ASPECT_DRIFT 0.02f /* drift of < 2% is forced to 0% */

class AbstractDriver;

struct video_info
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
	AbstractDriver *driver;
};

typedef struct video_info VIDEOINFO;
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

#define N_ATTR 8                        /* max number of attractors     */

extern  long     g_attractor_radius_l;      /* finite attractor radius  */
extern  double   g_attractor_radius_fp;      /* finite attractor radius  */

#define NUMIFS    64     /* number of ifs functions in ifs array */
#define IFSPARM    7     /* number of ifs parameters */
#define IFS3DPARM 13     /* number of ifs 3D parameters */

struct tag_more_parameters
{
	int      type;                       /* index in fractalname of the fractal */
	char     *parameters[MAX_PARAMETERS-4];    /* name of the parameters */
	double   paramvalue[MAX_PARAMETERS-4];    /* default parameter values */
};
typedef struct tag_more_parameters more_parameters;

/* bitmask defines for g_fractal_specific flags */
#define FRACTALFLAG_NO_ZOOM					1			/* zoombox not allowed at all          */
#define FRACTALFLAG_NO_SOLID_GUESSING		2			/* solid guessing not allowed          */
#define FRACTALFLAG_NO_BOUNDARY_TRACING		4			/* boundary tracing not allowed        */
#define FRACTALFLAG_NO_ZOOM_BOX_ROTATE		8			/* zoombox rotate/stretch not allowed  */
#define FRACTALFLAG_NOT_RESUMABLE			0x10		/* can't interrupt and resume          */
#define FRACTALFLAG_INFINITE_CALCULATION	0x20		/* this type calculates forever        */
#define FRACTALFLAG_FUNCTION_SHIFT			6			/* number of trig functions in formula */
#define FRACTALFLAG_FUNCTION_MASK			0x7
#define FRACTALFLAG_1_FUNCTION				(1 << FRACTALFLAG_FUNCTION_SHIFT)	
#define FRACTALFLAG_2_FUNCTIONS				(2 << FRACTALFLAG_FUNCTION_SHIFT)
#define FRACTALFLAG_4_FUNCTIONS				(4 << FRACTALFLAG_FUNCTION_SHIFT)
#define FRACTALFLAG_3D_PARAMETERS			0x400		/* uses 3d parameters                  */
#define FRACTALFLAG_JULIBROT				0x800		/* works with Julibrot                 */
#define FRACTALFLAG_MORE_PARAMETERS			0x1000		/* more than 4 parms                   */
#define FRACTALFLAG_BAIL_OUT_TESTS			0x2000		/* can use different bailout tests     */
#define FRACTALFLAG_ARBITRARY_PRECISION		0x4000		/* supports arbitrary precision        */
#define FRACTALFLAG_DONT_DISPLAY			0x10000		/* don't display in fractal type list */

struct FractalTypeSpecificData
{
	int fractal_type;
	char *name;							/* name of the fractal */
										/* (leading "*" supresses name display) */
	char  *parameters[4];				/* name of the parameters */
	double paramvalue[4];				/* default parameter values */
	int   helptext;						/* helpdefs.h HT_xxxx, -1 for none */
	int   helpformula;					/* helpdefs.h HF_xxxx, -1 for none */
	int flags;							/* constraints, bits defined below */
	float x_min;						/* default XMIN corner */
	float x_max;						/* default XMAX corner */
	float y_min;						/* default YMIN corner */
	float y_max;						/* default YMAX corner */
	int   isinteger;					/* >= 1 if integer fractal, 0 otherwise */
	int   tojulia;						/* index of corresponding julia type */
	int   tomandel;						/* index of corresponding mandelbrot type */
	int   tofloat;						/* index of corresponding floating-point type */
	SymmetryType symmetry;				/* applicable symmetry logic
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
	int (*orbitcalc)();					/* function that calculates one orbit */
	int (*per_pixel)();					/* once-per-pixel init */
	int (*per_image)();					/* once-per-image setup */
	int (*calculate_type)();			/* name of main fractal function */
	int orbit_bailout;					/* usual bailout value for orbit calc */
};

class AbstractFractalType
{
public:
	virtual ~AbstractFractalType()
	{
	}

	virtual int Orbit() = 0;
	virtual int PerPixel() = 0;
	virtual int PerImage() = 0;
	virtual int CalculateType() = 0;
};

struct tag_alternate_math_info
{
	int type;                    /* index in fractalname of the fractal */
	int math;                    /* kind of math used */
	int (*orbitcalc)();      /* function that calculates one orbit */
	int (*per_pixel)();      /* once-per-pixel init */
	int (*per_image)();      /* once-per-image setup */
};

typedef struct tag_alternate_math_info alternate_math;

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

/* more bitmasks for evolution mode flag */
#define EVOLVE_NONE			0	/* no evolution */
#define EVOLVE_FIELD_MAP	1	/*steady field varyiations across screen */
#define EVOLVE_RAND_WALK	2	/* newparm = lastparm +- rand()                   */
#define EVOLVE_RAND_PARAM	4	/* newparm = constant +- rand()                   */
#define EVOLVE_NO_GROUT		8	/* no gaps between images                                   */
#define EVOLVE_PARM_BOX		128


extern FractalTypeSpecificData g_fractal_specific[];
extern FractalTypeSpecificData *g_current_fractal_specific;

#define DEFAULTFRACTALTYPE      ".gif"
#define ALTERNATEFRACTALTYPE    ".fra"


#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#ifndef lsqr
#define lsqr(x) (multiply((x), (x), g_bit_shift))
#endif

#define CMPLXconj(z)    ((z).y =  -((z).y))
#define LCMPLXmod(z)    (lsqr((z).x)+lsqr((z).y))
#define LCMPLXconj(z)   ((z).y =  -((z).y))

#define PER_IMAGE   (g_fractal_specific[g_fractal_type].per_image)
#define PER_PIXEL   (g_fractal_specific[g_fractal_type].per_pixel)
#define ORBITCALC   (g_fractal_specific[g_fractal_type].orbitcalc)

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

struct WorkListItem    /* work list entry for std escape time engines */
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

typedef struct WorkListItem WORKLIST;


#define MAXCALCWORK 12

struct coords
{
	int x, y;
};

struct dblcoords
{
	double x, y;
};

struct trig_funct_lst
{
	const char *name;
	void (*lfunct)();
	void (*dfunct)();
	void (*mfunct)();
};

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

/* structure passed to fullscreen_prompts */
struct full_screen_values
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
		char   sval[FILE_MAX_PATH];  /* when type is 's'      */
		char  *sbuf;  /* when type is 0x100+n  */
		struct
		{          /* when type is 'l'      */
			 int  val;      /*   selected choice     */
			 int  vlen;     /*   char len per choice */
			 const char **list;   /*   list of values      */
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

typedef struct palett
{
	BYTE red;
	BYTE green;
	BYTE blue;
}
Palettetype;

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

struct search_path
{
	char par[FILE_MAX_PATH];
	char frm[FILE_MAX_PATH];
	char ifs[FILE_MAX_PATH];
	char lsys[FILE_MAX_PATH];
};

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

struct baseunit  /* smallest part of a fractint 'gene' */
{
	void *addr; /* address of variable to be referenced */
	void (*varyfunc)(struct baseunit*,int,int); /* pointer to func used to vary it */
							  /* takes random number and pointer to var*/
	int mutate;  /* flag to switch on variation of this variable */
				  /* 0 for no mutation, 1 for x axis, 2 for y axis */
				  /* in steady field maps, either x or y=yes in random modes*/ 
	char name[16]; /* name of variable (for menu ) */
	char level;    /* mutation level at which this should become active */
};

typedef struct baseunit    GENEBASE;

#define sign(x) (((x) < 0) ? -1 : ((x) != 0)  ? 1 : 0)


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

struct entryinfo
{
	char name[ITEMNAMELEN + 2];
	long point; /* points to the (or the { following the name */
};

struct UserInterfaceState
{
	bool double_caution;			/* confirm for deleting */
	bool ask_video;					/* flag for video prompting */
};

#endif
