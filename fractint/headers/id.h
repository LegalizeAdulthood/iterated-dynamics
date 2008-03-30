#ifndef FRACTINT_H
#define FRACTINT_H

#include "port.h"

// FRACTINT.H - common structures and values for the FRACTINT routines

/* Returns the number of items in an array declared of fixed size, i.e:
	int stuff[100];
	NUM_OF(stuff) returns 100.
*/
#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

#ifndef XFRACT
#include <time.h>

inline int clock_ticks()
{
	return clock();
}
#endif

enum StereogramCalibrateType
{
	CALIBRATE_NONE = 0,
	CALIBRATE_MIDDLE,
	CALIBRATE_TOP
};

enum ShowFileType
{
	SHOWFILE_PENDING = 0,
	SHOWFILE_DONE = 1,
	SHOWFILE_CANCELLED = -1
};

enum InitialZType
{
	INITIALZ_NONE = 0,
	INITIALZ_ORBIT = 1,
	INITIALZ_PIXEL = 2
};

// symmetry values
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

// g_file_type values
enum FileType
{
	FILETYPE_GIF = 0
};

// g_display_3d values
enum Display3DType
{
	DISPLAY3D_GENERATED	= -1,
	DISPLAY3D_NONE		= 0,
	DISPLAY3D_YES		= 1,
	DISPLAY3D_OVERLAY	= 2
};

// g_ifs_type values
enum IFSType
{
	IFSTYPE_2D = 0,
	IFSTYPE_3D = 1
};

// g_log_dynamic_calculate values
enum LogDynamicType
{
	LOGDYNAMIC_NONE = 0,
	LOGDYNAMIC_DYNAMIC = 1,
	LOGDYNAMIC_TABLE = 2
};

// g_log_palette_mode special values
enum LogPaletteType
{
	LOGPALETTE_NONE = 0,
	LOGPALETTE_STANDARD = 1,
	LOGPALETTE_OLD = -1
};

// g_.ColorState values
enum ColorStateType
{
	COLORSTATE_DEFAULT	= 0,
	COLORSTATE_UNKNOWN	= 1,
	COLORSTATE_MAP		= 2
};

// g_force_symmetry values
enum ForceSymmetryType
{
	FORCESYMMETRY_NONE		= 999,
	FORCESYMMETRY_SEARCH	= 1000
};

// g_save_dac values
enum SaveDACType
{
	SAVEDAC_NO		= 0,
	SAVEDAC_YES		= 1,
	SAVEDAC_NEXT	= 2
};

// g_orbit_draw_mode values
enum OrbitDrawType
{
	ORBITDRAW_RECTANGLE	= 0,
	ORBITDRAW_LINE		= 1,
	ORBITDRAW_FUNCTION	= 2
};

// g_true_mode values
enum TrueModeType
{
	TRUEMODE_DEFAULT	= 0,
	TRUEMODE_ITERATES	= 1
};

// g_debug_mode values
enum DebugFlagType
{
	DEBUGMODE_NONE				= 0,
	DEBUGMODE_LORENZ_FLOAT		= 22,
	DEBUGMODE_COMPARE_RESTORED	= 50,
	DEBUGMODE_NO_FPU			= 70,
	DEBUGMODE_NO_ASM_MANDEL		= 90,
	DEBUGMODE_OLD_POWER			= 94,
	DEBUGMODE_DISK_MESSAGES		= 96,
	DEBUGMODE_REAL_POPCORN		= 98,
	DEBUGMODE_NO_FIRST_INIT		= 110,
	DEBUGMODE_TIME_ENCODER		= 200,
	DEBUGMODE_NO_MIIM_QUEUE		= 300,
	DEBUGMODE_SKIP_OPTIMIZER	= 322,
	DEBUGMODE_NO_HELP_F1_ESC	= 324,
	DEBUGMODE_USE_DISK			= 420,
	DEBUGMODE_USE_MEMORY		= 422,
	DEBUGMODE_ABORT_SAVENAME	= 450,
	DEBUGMODE_BNDTRACE_NONZERO	= 470,
	DEBUGMODE_SOLID_GUESS_BR	= 472,
	DEBUGMODE_SET_DIGITS_MIN	= 700,
	DEBUGMODE_SET_DIGITS_MAX	= 720,
	DEBUGMODE_MORE_DIGITS		= 750,
	DEBUGMODE_NO_COLORS_FIX		= 910,
	DEBUGMODE_COLORS_LOSSLESS	= 920,
	DEBUGMODE_FORCE_FP_NEWTON	= 1010,
	DEBUGMODE_SWAP_SIGN			= 1012,
	DEBUGMODE_FORCE_BITSHIFT	= 1234,
	DEBUGMODE_2222				= 2222,
	DEBUGMODE_2224				= 2224,
	DEBUGMODE_EDIT_TEXT_COLORS	= 3000,
	DEBUGMODE_NO_DEV_HEADING	= 3002,
	DEBUGMODE_NO_BIG_TO_FLOAT	= 3200,
	DEBUGMODE_NO_INT_TO_FLOAT	= 3400,
	DEBUGMODE_SOI_LONG_DOUBLE	= 3444,
	DEBUGMODE_PIN_CORNERS_ONE	= 3600,
	DEBUGMODE_NO_PIXEL_GRID		= 3800,
	DEBUGMODE_SHOW_MATH_ERRORS	= 4000,
	DEBUGMODE_PRE193_CENTERMAG	= 4010,
	DEBUGMODE_OLD_TIMER			= 4020,
	DEBUGMODE_OLD_ORBIT_SOUND	= 4030,
	DEBUGMODE_MIN_DISKVID_CACHE	= 4200,
	DEBUGMODE_UNOPT_POWER		= 6000,
	DEBUGMODE_REDUCE_VIDEO_MIN	= 9002,
	DEBUGMODE_REDUCE_VIDEO_MAX	= 9100,
	DEBUGMODE_MEMORY			= 10000
};

// projection values
enum ProjectionType
{
	PROJECTION_ZX	= 0,
	PROJECTION_XZ	= 1,
	PROJECTION_XY	= 2
};

// random_dir values
enum DirectionType
{
	DIRECTION_LEFT		= 0,
	DIRECTION_RIGHT		= 1,
	DIRECTION_RANDOM	= 2
};

// g_juli_3d_mode values
enum Juli3DModeType
{
	JULI3DMODE_MONOCULAR	= 0,
	JULI3DMODE_LEFT_EYE		= 1,
	JULI3DMODE_RIGHT_EYE	= 2,
	JULI3DMODE_RED_BLUE		= 3
};

// g_which_image values
enum WhichImageType
{
	WHICHIMAGE_NONE = 0,
	WHICHIMAGE_RED	= 1,
	WHICHIMAGE_BLUE	= 2
};

// g_orbit_save values
enum OrbitSaveType
{
	ORBITSAVE_NONE	= 0,
	ORBITSAVE_RAW	= 1,
	ORBITSAVE_SOUND = 2
};

// find_file_item itemtypes
enum FindFileItemType
{
	ITEMTYPE_PARAMETER	= 0,
	ITEMTYPE_FORMULA	= 1,
	ITEMTYPE_L_SYSTEM	= 2,
	ITEMTYPE_IFS		= 3
};

// getfileentry and find_file_item itemtype values
enum GetFileEntryType
{
	GETFILE_FORMULA		= 0,
	GETFILE_L_SYSTEM	= 1,
	GETFILE_IFS			= 2,
	GETFILE_PARAMETER	= 3
};

// tab status values
enum TabStatusType
{
	TAB_STATUS_NONE = -1,
	TAB_STATUS_12PASS = 0,
	TAB_STATUS_GUESSING = 1,
	TAB_STATUS_BOUNDARY_TRACE = 2,
	TAB_STATUS_3D = 3,
	TAB_STATUS_TESSERAL = 4,
	TAB_STATUS_DIFFUSION = 5,
	TAB_STATUS_ORBITS = 6
};

// g_resave_mode values
enum ResaveType
{
	RESAVE_NO = 0,
	RESAVE_YES = 1,
	RESAVE_DONE = 2
};

// g_look_at_mouse values
enum LookMouseType
{
	LOOK_MOUSE_NONE		= 0,
	LOOK_MOUSE_TEXT		= 2,
	LOOK_MOUSE_ZOOM_BOX	= 3
};

// pause_error() values
enum PauseErrorType
{
	PAUSE_ERROR_NO_BATCH = 0,
	PAUSE_ERROR_ANY = 1,
	PAUSE_ERROR_GOODBYE = 2
};

enum TimedSaveType
{
	TIMEDSAVE_DONE = 0,
	TIMEDSAVE_START = 1,
	TIMEDSAVE_PENDING = 2
};

// g_initialize_batch values
enum InitializeBatchType
{
	INITBATCH_FINISH_CALC = -1,
	INITBATCH_NONE = 0,
	INITBATCH_NORMAL = 1,
	INITBATCH_SAVE = 2,
	INITBATCH_BAILOUT_ERROR = 3,
	INITBATCH_BAILOUT_INTERRUPTED = 4,
	INITBATCH_BAILOUT_SAVE = 5
};

// driver_buzzer() codes
enum BuzzerType
{
	BUZZER_COMPLETE = 0,
	BUZZER_INTERRUPT = 1,
	BUZZER_ERROR = 2
};

enum ApplicationStateType
{
	APPSTATE_NO_CHANGE = 0,
	APPSTATE_RESTART = 1,
	APPSTATE_IMAGE_START = 2,
	APPSTATE_RESTORE_START = 3,
	APPSTATE_CONTINUE = 4,
	APPSTATE_RESUME_LOOP = 5
};

// full_screen_choice options
enum FullScreenChoiceFlag
{
	CHOICE_RETURN_KEY	= 1,
	CHOICE_MENU			= 2,
	CHOICE_HELP			= 4,
	CHOICE_INSTRUCTIONS	= 8,
	CHOICE_CRUNCH		= 16,
	CHOICE_NOT_SORTED	= 32
};

// calculation status values
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

enum MiscellaneousConstants
{
	COLOR_CHANNEL_MAX = 63,

	// these are used to declare arrays for file names
#if defined(_WIN32)
	FILE_MAX_PATH = _MAX_PATH,
	FILE_MAX_DIR = _MAX_DIR,
#else
#ifdef XFRACT
	FILE_MAX_PATH = 256,		// max length of path+filename
	FILE_MAX_DIR = 256,			// max length of directory name
#else
	FILE_MAX_PATH = 80,			// max length of path+filename
	FILE_MAX_DIR = 80,			// max length of directory name
#endif
#endif
	FILE_MAX_DRIVE = 3,			// max length of drive letter

	/*
	The filename limits were increased in Xfract 3.02. But alas,
	in this poor program that was originally developed on the
	nearly-brain-dead DOS operating system, quite a few things
	in the UI would break if file names were bigger than DOS 8-3
	names. So for now humor us and let's keep the names short.
	*/
	FILE_MAX_FNAME = 64,		// max length of filename
	FILE_MAX_EXT = 64,			// max length of extension

	MAX_MAX_LINE_LENGTH = 128,	// upper limit for g_max_line_length for PARs
	MIN_MAX_LINE_LENGTH = 40,	// lower limit for g_max_line_length for PARs

	MESSAGE_LEN = 80,			// handy buffer size for messages
	MAX_COMMENT = 57,			// length of par comments
	MAX_PARAMETERS = 10,		// maximum number of parameters
	MAX_PIXELS = 32767,			// Maximum pixel count across/down the screen
	OLD_MAX_PIXELS = 2048,		// Limit of some old fixed arrays
	MIN_PIXELS = 10				// Minimum pixel count across/down the screen
};

extern const float DEFAULT_ASPECT_RATIO;
extern const float DEFAULT_ASPECT_DRIFT;

class AbstractDriver;

struct VIDEOINFO
{              // All we need to know about a Video Adapter
	char    name[26];       // Adapter name (IBM EGA, etc)
	char    comment[26];    // Comments (UNTESTED, etc)
	int     keynum;         // key number used to invoked this mode
							// 2-10 = F2-10, 11-40 = S,C,A{F1-F10}
	int     x_dots;          // number of dots across the screen
	int     y_dots;          // number of dots down the screen
	int     colors;         // number of colors available
	AbstractDriver *driver;
};

extern bool operator==(const VIDEOINFO &lhs, const VIDEOINFO &rhs);

extern char const *INFO_ID;

enum
{
	FRACTAL_INFO_VERSION  = 17
	// file version, independent of system
	// increment this EVERY time the fractal_info structure changes
};

/* TODO: instead of hacking the padding here, adjust the code that reads
   this structure */
#if defined(_WIN32)
#pragma pack(push, 1)
#endif
struct fractal_info         // for saving data in GIF file
{
	char  info_id[8];       // Unique identifier for info g_block
	short iterationsold;    // Pre version 18.24
	short fractal_type;     // 0=Mandelbrot 1=Julia 2= ...
	double x_min;
	double x_max;
	double y_min;
	double y_max;
	double c_real;
	double c_imag;
	short deprecated_video_mode_ax;
	short deprecated_video_mode_bx;
	short deprecated_video_mode_cx;
	short deprecated_video_mode_dx;
	short deprecated_dotmode;
	short x_dots;
	short y_dots;
	short colors;
	short version;          // used to be 'future[0]'
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
						// version 2 stuff
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
						// version 3 stuff, release 13
	short outside;
						// version 4 stuff, release 14
	double x_3rd;          // 3rd corner
	double y_3rd;
	char stdcalcmode;     // 1/2/g/b
	char use_initial_orbit_z;    // init Mandelbrot orbit flag
	short calculation_status;    // resumable, finished, etc
	long tot_extend_len;  // total length of extension blocks in .gif file
	short distestold;
	short float_flag;
	short bailoutold;
	long calculation_time;
	BYTE function_index[4];      // which trig functions selected
	short finattract;
	double initial_orbit_z[2];  // init Mandelbrot orbit values
	short periodicity;    // periodicity checking
						// version 5 stuff, release 15
	short potential_16bit;       // save 16 bit continuous potential info
	float aspect_ratio;   // g_viewWindow.AspectRatio(), y/x
	short system;         // DEPRECATED: 0 for dos, 1 for windows
	short release;        // release number, with 2 decimals implied
	short flag3d;         // stored only for now, for future use
	short transparent[2];
	short ambient;
	short haze;
	short randomize;
						// version 6 stuff, release 15.x
	short rotate_lo;
	short rotate_hi;
	short distance_test_width;
						// version 7 stuff, release 16
	double dparm3;
	double dparm4;
						// version 8 stuff, release 17
	short fill_color;
						// version 9 stuff, release 18
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
						// version 10 stuff, release 19
	long bail_out;
	short bailoutest;
	long iterations;
	short bf_math;
	short bflength;
	short yadjust;        // yikes! we left this out ages ago!
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
	short future[7];     // for stuff we haven't thought of yet
};
#if defined(_WIN32)
#pragma pack(pop)
#endif

/*
 * Note: because non-MSDOS machines store structures differently, we have
 * to do special processing of the fractal_info structure in loadfile.c.
 * Make sure changes to the structure here get reflected there.
 */
#ifndef XFRACT
enum
{
	FRACTAL_INFO_SIZE = sizeof(fractal_info)
};
#else
// This value should be the MSDOS size, not the Unix size.
enum
{
	FRACTAL_INFO_SIZE = 504
};
#endif

extern char CheckFractalInfoSize[sizeof(fractal_info) == 504];

enum
{
	ITEMNAMELEN = 18   // max length of names in .frm/.l/.ifs/.fc
};

#if defined(_WIN32)
#pragma pack(push, 1)
#endif
struct formula_info         // for saving formula data in GIF file
{
	char  form_name[40];
	short uses_p1;
	short uses_p2;
	short uses_p3;
	short uses_is_mand;
	short ismand;
	short uses_p4;
	short uses_p5;
	short future[6];       // for stuff we haven't thought of, yet
};
#if defined(_WIN32)
#pragma pack(pop)
#endif

enum stored_at_values
{
	NOWHERE,
	MEMORY,
	DISK
};

enum
{
	NUM_GENES = 21
};

#if defined(_WIN32)
#pragma pack(push, 1)
#endif
struct evolution_info      // for saving evolution data in a GIF file
{
	short evolving;
	short grid_size;
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
	short screen_x_offset;
	short screen_y_offset;
	short x_dots;
	short y_dots;
	short mutate[NUM_GENES];
	short ecount; // count of how many images have been calc'ed so far
	short future[68 - NUM_GENES];      // total of 200 bytes
};
#if defined(_WIN32)
#pragma pack(pop)
#endif

/*
 * Note: because non-MSDOS machines store structures differently, we have
 * to do special processing of the evolution_info structure in loadfile.c and
 * encoder.c.  See decode_evolver_info() in general.c.
 * Make sure changes to the structure here get reflected there.
 */
#ifndef XFRACT
enum
{
	EVOLVER_INFO_SIZE = sizeof(evolution_info)
};
#else
// This value should be the MSDOS size, not the Unix size.
enum
{
	EVOLVER_INFO_SIZE = 200
};
#endif
extern char CheckEvolverInfoSize[200 == sizeof(evolution_info)];

#if defined(_WIN32)
#pragma pack(push, 1)
#endif
struct orbits_info      // for saving orbits data in a GIF file
{
	double oxmin;
	double oxmax;
	double oymin;
	double oymax;
	double ox3rd;
	double oy3rd;
	short keep_scrn_coords;
	char drawmode;
	char dummy; // need an even number of bytes
	short future[74];      // total of 200 bytes
};
#if defined(_WIN32)
#pragma pack(pop)
#endif

/*
 * Note: because non-MSDOS machines store structures differently, we have
 * to do special processing of the orbits_info structure in loadfile.c and
 * encoder.c.  See decode_orbits_info() in general.c.
 * Make sure changes to the structure here get reflected there.
 */
#ifndef XFRACT
#define ORBITS_INFO_SIZE sizeof(orbits_info)
#else
// This value should be the MSDOS size, not the Unix size.
#define ORBITS_INFO_SIZE 200
#endif

extern char CheckOrbitsInfoSize[sizeof(orbits_info) == 200];

enum
{
	MAXVIDEOMODES  = 300       // maximum entries in fractint.cfg
};

extern double const AUTO_INVERT;

enum
{
	MAX_NUM_ATTRACTORS = 8                        // max number of attractors
};

extern  long     g_attractor_radius_l;      // finite attractor radius
extern  double   g_attractor_radius_fp;      // finite attractor radius

enum
{
	NUMIFS = 64,			// number of ifs functions in ifs array
	IFSPARM = 7,		// number of ifs parameters
	IFS3DPARM = 13		// number of ifs 3D parameters
};

struct more_parameters
{
	int type;											// index in fractalname of the fractal
	const char *parameters[MAX_PARAMETERS-4];			// name of the parameters
	double paramvalue[MAX_PARAMETERS-4];				// default parameter values
};

enum FractalFlags
{
	// bitmask defines for g_fractal_specific flags
	FRACTALFLAG_NO_ZOOM					= 1,			// zoombox not allowed at all
	FRACTALFLAG_NO_SOLID_GUESSING		= 2,			// solid guessing not allowed
	FRACTALFLAG_NO_BOUNDARY_TRACING		= 4,			// boundary tracing not allowed
	FRACTALFLAG_NO_ZOOM_BOX_ROTATE		= 8,			// zoombox rotate/stretch not allowed
	FRACTALFLAG_NOT_RESUMABLE			= 0x10,			// can't interrupt and resume
	FRACTALFLAG_INFINITE_CALCULATION	= 0x20,			// this type calculates forever
	FRACTALFLAG_FUNCTION_SHIFT			= 6,			// number of trig functions in formula
	FRACTALFLAG_FUNCTION_MASK			= 0x7,
	FRACTALFLAG_1_FUNCTION				= (1 << FRACTALFLAG_FUNCTION_SHIFT),
	FRACTALFLAG_2_FUNCTIONS				= (2 << FRACTALFLAG_FUNCTION_SHIFT),
	FRACTALFLAG_4_FUNCTIONS				= (4 << FRACTALFLAG_FUNCTION_SHIFT),
	FRACTALFLAG_3D_PARAMETERS			= 0x400,		// uses 3d parameters
	FRACTALFLAG_JULIBROT				= 0x800,		// works with Julibrot
	FRACTALFLAG_MORE_PARAMETERS			= 0x1000,		// more than 4 parms
	FRACTALFLAG_BAIL_OUT_TESTS			= 0x2000,		// can use different bailout tests
	FRACTALFLAG_ARBITRARY_PRECISION		= 0x4000,		// supports arbitrary precision
	FRACTALFLAG_DONT_DISPLAY			= 0x10000		// don't display in fractal type list
};

enum SpecialHelpFormulaType
{
	SPECIALHF_IFS = -4,
	SPECIALHF_L_SYSTEM = -3,
	SPECIALHF_FORMULA = -2,
	SPECIALHF_JULIBROT = -1
};

struct FractalTypeSpecificData
{
	int fractal_type;
	const char *name;					// name of the fractal
										// (leading "*" supresses name display)
	const char  *parameters[4];			// name of the parameters
	double paramvalue[4];				// default parameter values
	int   helptext;						// helpdefs.h HT_xxxx, -1 for none
	int   helpformula;					// helpdefs.h HF_xxxx, -1 for none
	int flags;							// constraints, bits defined below
	float x_min;						// default XMIN corner
	float x_max;						// default XMAX corner
	float y_min;						// default YMIN corner
	float y_max;						// default YMAX corner
	int   isinteger;					// >= 1 if integer fractal, 0 otherwise
	int   tojulia;						// index of corresponding julia type
	int   tomandel;						// index of corresponding mandelbrot type
	int   tofloat;						// index of corresponding floating-point type
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
	int (*orbitcalc)();					// function that calculates one orbit
	int (*per_pixel)();					// once-per-pixel init
	bool (*per_image)();				// once-per-image setup
	int (*calculate_type)();			// name of main fractal function
	int orbit_bailout;					// usual bailout value for orbit calc

	int num_functions() const
	{
		return (flags >> FRACTALFLAG_FUNCTION_SHIFT) & FRACTALFLAG_FUNCTION_MASK;
	}

	bool no_boundary_tracing() const
	{
		return (flags & FRACTALFLAG_NO_BOUNDARY_TRACING) != 0;
	}

	bool no_solid_guessing() const
	{
		return (flags & FRACTALFLAG_NO_SOLID_GUESSING) != 0;
	}

	bool arbitrary_precision() const
	{
		return (flags & FRACTALFLAG_ARBITRARY_PRECISION) != 0;
	}

	bool no_zoom_box_rotate() const
	{
		return (flags & FRACTALFLAG_NO_ZOOM_BOX_ROTATE) != 0;
	}

	const char *get_type() const
	{
		return &name[is_hidden() ? 1 : 0];
	}
	bool is_hidden() const
	{
		return (name[0] == '*');
	}
};

// defines for inside/outside
enum ColoringModeType
{
	COLORMODE_INVERSE_TANGENT_INTEGER	= -104,
	COLORMODE_FLOAT_MODULUS_INTEGER		= -103,
	COLORMODE_PERIOD					= -102,
	COLORMODE_STAR_TRAIL				= -101,
	COLORMODE_EPSILON_CROSS				= -100,
	COLORMODE_BEAUTY_OF_FRACTALS_61		= -61,
	COLORMODE_BEAUTY_OF_FRACTALS_60		= -60,
	COLORMODE_Z_MAGNITUDE				= -59,
	COLORMODE_TOTAL_DISTANCE			= -8,
	COLORMODE_FLOAT_MODULUS				= -7,
	COLORMODE_INVERSE_TANGENT			= -6,
	COLORMODE_SUM						= -5,
	COLORMODE_MULTIPLY					= -4,
	COLORMODE_IMAGINARY					= -3,
	COLORMODE_REAL						= -2,
	COLORMODE_ITERATION					= -1
};

enum BiomorphType
{
	BIOMORPH_NONE = -1
};

// defines for bailoutest
enum BailOutType
{
	BAILOUT_MODULUS,
	BAILOUT_REAL,
	BAILOUT_IMAGINARY,
	BAILOUT_OR,
	BAILOUT_AND,
	BAILOUT_MANHATTAN,
	BAILOUT_MANHATTAN_R
};

enum MajorMethodType
{
	MAJORMETHOD_BREADTH_FIRST,
	MAJORMETHOD_DEPTH_FIRST,
	MAJORMETHOD_RANDOM_WALK,
	MAJORMETHOD_RANDOM_RUN
};

enum MinorMethodType
{
	MINORMETHOD_LEFT_FIRST,
	MINORMETHOD_RIGHT_FIRST
};

// more bitmasks for evolution mode flag
enum EvolveFlags
{
	EVOLVE_NONE				= 0,	// no evolution
	EVOLVE_FIELD_MAP		= 1,	// steady field varyiations across screen
	EVOLVE_RANDOM_WALK		= 2,	// newparm = lastparm +- rand()
	EVOLVE_RANDOM_PARAMETER	= 4,	// newparm = constant +- rand()
	EVOLVE_NO_GROUT			= 8,	// no gaps between images
	EVOLVE_PARAMETER_BOX	= 128
};

extern FractalTypeSpecificData g_fractal_specific[];
extern FractalTypeSpecificData *g_current_fractal_specific;

extern char const *GIF_EXTENSION;
extern char const *FRA_EXTENSION;

#ifdef sqr
#undef sqr
#endif
template <typename T>
T sqr(T x)
{
	return x*x;
}

inline long lsqr(long x)
{
	extern int g_bit_shift;
	extern long multiply(long, long, int);
	return multiply(x, x, g_bit_shift);
}

enum
{
	CMAX = 4,				// maximum column (4 x 4 matrix)
	RMAX = 4,				// maximum row    (4 x 4 matrix)
	DIM = 3					// number of dimensions
};

// TODO: use a template type for different kinds of matrices
typedef double MATRIX[RMAX][CMAX];  // matrix of doubles
typedef long  MATRIX_L[RMAX][CMAX];  // matrix of longs

/* A MATRIX is used to describe a transformation from one coordinate
system to another.  Multiple transformations may be concatenated by
multiplying their transformation matrices. */

// TODO: use a template type for different kinds of vectors
typedef double VECTOR[DIM];  // vector of doubles
typedef long  VECTOR_L[DIM];  // vector of longs

/* A VECTOR is an array of three coordinates [x,y,z] representing magnitude
and direction. A fourth dimension is assumed to always have the value 1, but
is not in the data structure */

inline double DOT_PRODUCT(VECTOR const v1,VECTOR const v2)
{
	return ((v1)[0]*(v2)[0]+(v1)[1]*(v2)[1]+(v1)[2]*(v2)[2]);
}

// Math definitions (normally in float.h) that are missing on some systems.
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

template <typename T>
struct CoordinateT
{
	T x;
	T y;
};

typedef CoordinateT<int> Coordinate;
typedef CoordinateT<double> CoordinateD;

struct FunctionListItem
{
	const char *name;
	void (*lfunct)();
	void (*dfunct)();
};

#define BIG 100000.0

// nonalpha tests if we have a control character
inline bool nonalpha(int c)
{
	return (c < 32) || (c > 127);
}

/* keys; IDK = "Iterated Dynamics Key"
 * Use this prefix to disambiguate key name symbols used in the fractint source
 * from symbols defined by the external environment, i.e. "DELETE" on Win32
 */
enum IdKey
{
	IDK_ALT_A = 1030,
	IDK_ALT_S = 1031,
	IDK_ALT_F1 = 1104,
	IDK_ALT_F2 = 1105,
	IDK_ALT_F3 = 1106,
	IDK_ALT_F4 = 1107,
	IDK_ALT_F5 = 1108,
	IDK_ALT_F6 = 1109,
	IDK_ALT_F7 = 1110,
	IDK_ALT_F8 = 1111,
	IDK_ALT_F9 = 1112,
	IDK_ALT_F10 = 1113,
	IDK_ALT_1 = 1120,
	IDK_ALT_2 = 1121,
	IDK_ALT_3 = 1122,
	IDK_ALT_4 = 1123,
	IDK_ALT_5 = 1124,
	IDK_ALT_6 = 1125,
	IDK_ALT_7 = 1126,

	IDK_CTL_A = 1,
	IDK_CTL_B = 2,
	IDK_CTL_E = 5,
	IDK_CTL_F = 6,
	IDK_CTL_G = 7,
	IDK_CTL_O = 15,
	IDK_CTL_S = 19,
	IDK_CTL_U = 21,
	IDK_CTL_X = 24,
	IDK_CTL_Y = 25,
	IDK_CTL_Z = 26,
	IDK_CTL_BACKSLASH = 28,
	IDK_CTL_DEL = 1147,
	IDK_CTL_DOWN_ARROW = 1145,
	IDK_CTL_END = 1117,
	IDK_CTL_ENTER = 10,
	IDK_CTL_ENTER_2 = 1010,
	IDK_CTL_F1 = 1094,
	IDK_CTL_F2 = 1095,
	IDK_CTL_F3 = 1096,
	IDK_CTL_F4 = 1097,
	IDK_CTL_F5 = 1098,
	IDK_CTL_F6 = 1099,
	IDK_CTL_F7 = 1100,
	IDK_CTL_F8 = 1101,
	IDK_CTL_F9 = 1102,
	IDK_CTL_F10 = 1103,
	IDK_CTL_HOME = 1119,
	IDK_CTL_INSERT = 1146,
	IDK_CTL_LEFT_ARROW = 1115,
	IDK_CTL_MINUS = 1142,
	IDK_CTL_PAGE_DOWN = 1118,
	IDK_CTL_PAGE_UP = 1132,
	IDK_CTL_PLUS = 1144,
	IDK_CTL_RIGHT_ARROW = 1116,
	IDK_CTL_TAB = 1148,
	IDK_CTL_UP_ARROW = 1141,

	IDK_SHF_TAB = 1015,  // shift tab aka BACKTAB

	IDK_BACKSPACE = 8,
	IDK_DELETE = 1083,
	IDK_DOWN_ARROW = 1080,
	IDK_END = 1079,
	IDK_ENTER = 13,
	IDK_ENTER_2 = 1013,
	IDK_ESC = 27,
	IDK_F1 = 1059,
	IDK_F2 = 1060,
	IDK_F3 = 1061,
	IDK_F4 = 1062,
	IDK_F5 = 1063,
	IDK_F6 = 1064,
	IDK_F7 = 1065,
	IDK_F8 = 1066,
	IDK_F9 = 1067,
	IDK_F10 = 1068,
	IDK_HOME = 1071,
	IDK_INSERT = 1082,
	IDK_LEFT_ARROW = 1075,
	IDK_PAGE_DOWN = 1081,
	IDK_PAGE_UP = 1073,
	IDK_RIGHT_ARROW = 1077,
	IDK_SPACE = 32,
	IDK_SF1 = 1084,
	IDK_SF2 = 1085,
	IDK_SF3 = 1086,
	IDK_SF4 = 1087,
	IDK_SF5 = 1088,
	IDK_SF6 = 1089,
	IDK_SF7 = 1090,
	IDK_SF8 = 1091,
	IDK_SF9 = 1092,
	IDK_SF10 = 1093,
	IDK_TAB = 9,
	IDK_UP_ARROW = 1072,

	// not really a key, but a special trigger
	IDK_SAVE_TIME = 9999
};

// structure passed to fullscreen_prompts
struct full_screen_values
{
	int type;   // 'd' for double, 'f' for float, 's' for string,
			   // 'D' for integer in double, '*' for comment
			   // 'i' for integer, 'y' for yes=1 no=0
			   // 0x100+n for string of length n
			   // 'l' for one of a list of strings
			   // 'L' for long
	union
	{
		double dval;      // when type 'd' or 'f'
		int    ival;      // when type is 'i'
		long   Lval;      // when type is 'L'
		char   sval[FILE_MAX_PATH];  // when type is 's'
		char  *sbuf;  // when type is 0x100+n
		struct
		{          // when type is 'l'
			 int  val;      // selected choice
			 int  vlen;     // char len per choice
			 const char **list;   // list of values
			 int  llen;     // number of values
		} ch;
	} uval;
};

struct Palettetype
{
	BYTE red;
	BYTE green;
	BYTE blue;
};

#if defined(_WIN32)
#pragma pack(push, 1)
#endif
struct resume_info_extension_block
{
	char got_data;
	int length;
	char *resume_data;
};

struct formula_info_extension_block
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

struct ranges_info_extension_block
{
	char got_data;
	int length;
	short *range_data;
};

struct multiple_precision_info_extension_block
{
	char got_data;
	int length;
	char *apm_data;
};

// parameter evolution stuff
struct evolver_info_extension_block
{
	char got_data;
	int length;
	short evolving;
	short grid_size;
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
	short  mutate[NUM_GENES];
};

struct orbits_info_extension_block
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

struct affine
{
	/*
		x' = [ a b ]*x + [ e ]
			 [ c d ]     [ f ]
	*/
	// weird order so a,b,e and c,d,f are vectors
	double a;
	double b;
	double e;
	double c;
	double d;
	double f;
};

struct GENEBASE  // smallest part of a fractint 'gene'
{
	void *addr; // address of variable to be referenced
	void (*varyfunc)(GENEBASE *,int,int); // pointer to func used to vary it
							  // takes random number and pointer to var
	int mutate;  // flag to switch on variation of this variable
				  // 0 for no mutation, 1 for x axis, 2 for y axis
				  // in steady field maps, either x or y=yes in random modes
	char name[16]; // name of variable (for menu )
	char level;    // mutation level at which this should become active
};

template <typename T>
inline int sign(T x)
{
	return (((x) < 0) ? -1 : ((x) != 0)  ? 1 : 0);
}


struct entry_info
{
	char name[ITEMNAMELEN + 2];
	long point; // points to the (or the { following the name
};

template <typename T>
class ValueSaver
{
public:
	ValueSaver(T &variable)
		: m_variable(variable), m_original_value(variable)
	{
	}
	ValueSaver(T &variable, T new_value)
		: m_variable(variable), m_original_value(variable)
	{
		variable = new_value;
	}

	~ValueSaver()
	{
		m_variable = m_original_value;
	}

private:
	T &m_variable;
	T m_original_value;
};

extern int timer_engine(int (*engine)());
extern int timer_decoder(int line_width);
extern int timer_encoder();

#endif
