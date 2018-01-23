// FRACTINT.H - common structures and values for the FRACTINT routines
#ifndef FRACTINT_H
#define FRACTINT_H
#include <vector>

#include "big.h"

/* Returns the number of items in an array declared of fixed size, i.e:
    int stuff[100];
    NUM_OF(stuff) returns 100.
*/
#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))
#ifndef XFRACT
#define clock_ticks() clock()
#endif
#define NUM_BOX_POINTS 4096

enum class display_3d_modes
{
    MINUS_ONE = -1,
    NONE = 0,
    YES = 1,
    B_COMMAND = 2
};

// for init_batch
enum class batch_modes
{
    FINISH_CALC_BEFORE_SAVE = -1,
    NONE,
    NORMAL,
    SAVE,
    BAILOUT_ERROR_NO_SAVE,
    BAILOUT_INTERRUPTED_TRY_SAVE,
    BAILOUT_INTERRUPTED_SAVE
};

enum class buzzer_codes
{
    COMPLETE = 0,
    INTERRUPT = 1,
    PROBLEM = 2
};

// stopmsg() flags
enum stopmsg_flags
{
    STOPMSG_NONE        = 0,
    STOPMSG_NO_STACK    = 1,
    STOPMSG_CANCEL      = 2,
    STOPMSG_NO_BUZZER   = 4,
    STOPMSG_FIXED_FONT  = 8,
    STOPMSG_INFO_ONLY   = 16
};

// for gotos in former FRACTINT.C pieces
enum class main_state
{
    NOTHING = 0,
    RESTART,
    IMAGE_START,
    RESTORE_START,
    CONTINUE
};

enum class slides_mode
{
    OFF = 0,
    PLAY = 1,
    RECORD = 2
};

// fullscreen_choice options
enum choice_flags
{
    CHOICE_RETURN_KEY   = 1,
    CHOICE_MENU         = 2,
    CHOICE_HELP         = 4,
    CHOICE_INSTRUCTIONS = 8,
    CHOICE_CRUNCH       = 16,
    CHOICE_NOT_SORTED   = 32
};

// g_calc_status values
enum class calc_status_value
{
    NO_FRACTAL = -1,
    PARAMS_CHANGED = 0,
    IN_PROGRESS = 1,
    RESUMABLE = 2,
    NON_RESUMABLE = 3,
    COMPLETED = 4
};

enum cmdarg_flags
{
    CMDARG_ERROR            = -1,
    CMDARG_NONE             = 0,
    CMDARG_FRACTAL_PARAM    = 1,
    CMDARG_3D_PARAM         = 2,
    CMDARG_3D_YES           = 4,
    CMDARG_RESET            = 8
};

enum class cmd_file
{
    AT_CMD_LINE = 0,
    SSTOOLS_INI = 1,
    AT_AFTER_STARTUP = 2,
    AT_CMD_LINE_SET_NAME = 3
};

enum input_field_flags
{
    INPUTFIELD_NUMERIC  = 1,
    INPUTFIELD_INTEGER  = 2,
    INPUTFIELD_DOUBLE   = 4
};

enum sound_flags
{
    SOUNDFLAG_OFF       = 0,
    SOUNDFLAG_BEEP      = 1,
    SOUNDFLAG_X         = 2,
    SOUNDFLAG_Y         = 3,
    SOUNDFLAG_Z         = 4,
    SOUNDFLAG_ORBITMASK = 0x07,
    SOUNDFLAG_SPEAKER   = 8,
    SOUNDFLAG_OPL3_FM   = 16,
    SOUNDFLAG_MIDI      = 32,
    SOUNDFLAG_QUANTIZED = 64,
    SOUNDFLAG_MASK      = 0x7F
};

enum class stereo_images
{
    NONE,
    RED,
    BLUE
};

// these are used to declare arrays for file names
#if defined(_WIN32)
#define FILE_MAX_PATH _MAX_PATH
#define FILE_MAX_DIR _MAX_DIR
#else
#ifdef XFRACT
#define FILE_MAX_PATH  256       // max length of path+filename
#define FILE_MAX_DIR   256       // max length of directory name
#else
#define FILE_MAX_PATH  80       // max length of path+filename
#define FILE_MAX_DIR   80       // max length of directory name
#endif
#endif
#define FILE_MAX_DRIVE  3       // max length of drive letter
/*
The filename limits were increased in Xfract 3.02. But alas,
in this poor program that was originally developed on the
nearly-brain-dead DOS operating system, quite a few things
in the UI would break if file names were bigger than DOS 8-3
names. So for now humor us and let's keep the names short.
*/
#define FILE_MAX_FNAME  64          // max length of filename
#define FILE_MAX_EXT    64          // max length of extension
#define MAX_MAX_LINE_LENGTH  128    // upper limit for g_max_line_length for PARs
#define MIN_MAX_LINE_LENGTH  40     // lower limit for g_max_line_length for PARs
#define MSG_LEN 80                  // handy buffer size for messages
#define MAX_COMMENT_LEN 57          // length of par comments
#define MAX_PARAMS 10               // maximum number of parameters
#define MAX_PIXELS   32767          // Maximum pixel count across/down the screen
#define OLD_MAX_PIXELS 2048         // Limit of some old fixed arrays
#define MIN_PIXELS 10               // Minimum pixel count across/down the screen
#define DEFAULT_ASPECT 1.0F         // Assumed overall screen dimensions, y/x
#define DEFAULT_ASPECT_DRIFT 0.02F  // drift of < 2% is forced to 0%

enum debug_flags
{
    none                                = 0,
    benchmark_timer                     = 1,
    force_float_perspective             = 22,
    force_disk_restore_not_save         = 50,
    prevent_287_math                    = 72,
    force_standard_fractal              = 90,
    force_ld_check                      = 94,
    force_real_popcorn                  = 96,
    write_formula_debug_information     = 98,
    allow_init_commands_anytime         = 110,
    benchmark_encoder                   = 200,
    prevent_miim                        = 300,
    prevent_formula_optimizer           = 322,
    show_formula_info_after_compile     = 324,
    force_memory_from_disk              = 420,
    force_memory_from_memory            = 422,
    prevent_overwrite_savename          = 450,
    force_boundary_trace_error          = 470,
    force_solid_guess_error             = 472,
    force_precision_0_digits            = 700,
    force_precision_20_digits           = 720,
    force_long_double_param_output      = 750,
    allow_large_colormap_changes        = 910,
    force_lossless_colormap             = 920,
    allow_mp_newton_type                = 1010,
    mandelbrot_mix4_flip_sign           = 1012,
    force_smaller_bitshift              = 1234,
    show_float_flag                     = 2224,
    force_arbitrary_precision_math      = 3200,
    prevent_arbitrary_precision_math    = 3400,
    use_soi_long_double                 = 3444,
    prevent_plasma_random               = 3600,
    prevent_coordinate_grid             = 3800,
    allow_negative_cross_product        = 4010,
    force_old_sleep                     = 4020,
    force_scaled_sound_formula          = 4030,
    force_disk_min_cache                = 4200,
    force_complex_power                 = 6000,
    prevent_386_math                    = 8088,
    display_memory_statistics           = 10000
};

struct Driver;
struct VIDEOINFO
{                           // All we need to know about a Video Adapter
    char    name[26];       // Adapter name (IBM EGA, etc)
    char    comment[26];    // Comments (UNTESTED, etc)
    int     keynum;         // key number used to invoked this mode
    // 2-10 = F2-10, 11-40 = S,C,A{F1-F10}
    int     videomodeax;    // begin with INT 10H, AX=(this)
    int     videomodebx;    // ...and BX=(this)
    int     videomodecx;    // ...and CX=(this)
    int     videomodedx;    // ...and DX=(this)
    // NOTE:  IF AX==BX==CX==0, SEE BELOW
    int     dotmode;        // video access method used by asm code
    // 1 == BIOS 10H, AH=12,13 (SLOW)
    // 2 == access like EGA/VGA
    // 3 == access like MCGA
    // 4 == Tseng-like  SuperVGA*256
    // 5 == P'dise-like SuperVGA*256
    // 6 == Vega-like   SuperVGA*256
    // 7 == "Tweaked" IBM-VGA ...*256
    // 8 == "Tweaked" SuperVGA ...*256
    // 9 == Targa Format
    // 10 = Hercules
    // 11 = "disk video" (no screen)
    // 12 = 8514/A
    // 13 = CGA 320x200x4, 640x200x2
    // 14 = Tandy 1000
    // 15 = TRIDENT  SuperVGA*256
    // 16 = Chips&Tech SuperVGA*256
    int     xdots;          // number of dots across the screen
    int     ydots;          // number of dots down the screen
    int     colors;         // number of colors available
    Driver *driver;
};
#define INFO_ID         "Fractal"
/*
 * Note: because non-MSDOS machines store structures differently, we have
 * to do special processing of the fractal_info structure in loadfile.c.
 * Make sure changes to the structure here get reflected there.
 */
#ifndef XFRACT
#define FRACTAL_INFO_SIZE sizeof(FRACTAL_INFO)
#else
// This value should be the MSDOS size, not the Unix size.
#define FRACTAL_INFO_SIZE 504
#endif
#define FRACTAL_INFO_VERSION 17  // file version, independent of system
// increment this EVERY time the fractal_info structure changes
// TODO: instead of hacking the padding here, adjust the code that reads this structure
#if defined(_WIN32)
#pragma pack(push, 1)
#endif
struct FRACTAL_INFO         // for saving data in GIF file
{
    char  info_id[8];       // Unique identifier for info block
    short iterationsold;    // Pre version 18.24
    short fractal_type;     // 0=Mandelbrot 1=Julia 2= ...
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    double creal;
    double cimag;
    short videomodeax;
    short videomodebx;
    short videomodecx;
    short videomodedx;
    short dotmode;
    short xdots;
    short ydots;
    short colors;
    short version;          // used to be 'future[0]'
    float parm3;
    float parm4;
    float potential[3];
    short rseed;
    short rflag;
    short biomorph;
    short inside;
    short logmapold;
    float invert[3];
    short decomp[2];
    short symmetry;
    // version 2 stuff
    short init3d[16];
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
    double x3rd;          // 3rd corner
    double y3rd;
    char stdcalcmode;     // 1/2/g/b
    char useinitorbit;    // init Mandelbrot orbit flag
    short calc_status;    // resumable, finished, etc
    long tot_extend_len;  // total length of extension blocks in .gif file
    short distestold;
    short floatflag;
    short bailoutold;
    long calctime;
    BYTE trigndx[4];      // which trig functions selected
    short finattract;
    double initorbit[2];  // init Mandelbrot orbit values
    short periodicity;    // periodicity checking
    // version 5 stuff, release 15
    short pot16bit;       // save 16 bit continuous potential info
    float faspectratio;   // finalaspectratio, y/x
    short system;         // 0 for dos, 1 for windows
    short release;        // release number, with 2 decimals implied
    short display_3d;     // stored only for now, for future use
    short transparent[2];
    short ambient;
    short haze;
    short randomize;
    // version 6 stuff, release 15.x
    short rotate_lo;
    short rotate_hi;
    short distestwidth;
    // version 7 stuff, release 16
    double dparm3;
    double dparm4;
    // version 8 stuff, release 17
    short fillcolor;
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
    float distfp;
    float eyesfp;
    short orbittype;
    short juli3Dmode;
    short maxfn;
    short inversejulia;
    double dparm5;
    double dparm6;
    double dparm7;
    double dparm8;
    double dparm9;
    double dparm10;
    // version 10 stuff, release 19
    long bailout;
    short bailoutest;
    long iterations;
    short bf_math;
    short bflength;
    short yadjust;        // yikes! we left this out ages ago!
    short old_demm_colors;
    long logmap;
    long distest;
    double dinvert[3];
    short logcalc;
    short stoppass;
    short quick_calc;
    double closeprox;
    short nobof;
    long orbit_interval;
    short orbit_delay;
    double math_tol[2];
    short future[7];     // for stuff we haven't thought of yet
};
#if defined(_WIN32)
#pragma pack(pop)
#endif
#define ITEM_NAME_LEN 18   // max length of names in .frm/.l/.ifs/.fc
struct HISTORY
{
    short fractal_type;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    double creal;
    double cimag;
    double potential[3];
    short rseed;
    short rflag;
    short biomorph;
    short inside;
    long logmap;
    double invert[3];
    short decomp;
    short symmetry;
    short init3d[16];
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
    double x3rd;
    double y3rd;
    long distest;
    short bailoutold;
    BYTE trigndx[4];
    short finattract;
    double initorbit[2];
    short periodicity;
    short pot16bit;
    short release;
    short save_release;
    display_3d_modes display_3d;
    short transparent[2];
    short ambient;
    short haze;
    short randomize;
    short rotate_lo;
    short rotate_hi;
    short distestwidth;
    double dparm3;
    double dparm4;
    short fillcolor;
    double mxmaxfp;
    double mxminfp;
    double mymaxfp;
    double myminfp;
    short zdots;
    float originfp;
    float depthfp;
    float heightfp;
    float widthfp;
    float distfp;
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
    long bailout;
    short bailoutest;
    long iterations;
    short bf_math;
    short bflength;
    short yadjust;
    short old_demm_colors;
    char filename[FILE_MAX_PATH];
    char itemname[ITEM_NAME_LEN+1];
    unsigned char dac[256][3];
    char  maxfn;
    char stdcalcmode;
    char three_pass;
    char useinitorbit;
    short logcalc;
    short stoppass;
    short ismand;
    double closeprox;
    short nobof;
    double math_tol[2];
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

struct formula_info         // for saving formula data in GIF file
{
    char  form_name[40];
    short uses_p1;
    short uses_p2;
    short uses_p3;
    short uses_ismand;
    short ismand;
    short uses_p4;
    short uses_p5;
    short future[6];       // for stuff we haven't thought of, yet
};

enum stored_at_values
{
    NOWHERE,
    MEMORY,
    DISK
};
#define NUM_GENES 21
/*
 * Note: because non-MSDOS machines store structures differently, we have
 * to do special processing of the evolution_info structure in loadfile.c and
 * encoder.c.  See decode_evolver_info() in general.c.
 * Make sure changes to the structure here get reflected there.
 */
#ifndef XFRACT
#define EVOLVER_INFO_SIZE sizeof(evolution_info)
#else
// This value should be the MSDOS size, not the Unix size.
#define EVOLVER_INFO_SIZE 200
#endif
struct EVOLUTION_INFO      // for saving evolution data in a GIF file
{
    short evolving;
    short image_grid_size;
    unsigned short this_generation_random_seed;
    double max_random_mutation;
    double x_parameter_range;
    double y_parameter_range;
    double x_parameter_offset;
    double y_parameter_offset;
    short discrete_x_parameter_offset;
    short discrete_y_paramter_offset;
    short px;
    short py;
    short sxoffs;
    short syoffs;
    short xdots;
    short ydots;
    short mutate[NUM_GENES];
    short ecount; // count of how many images have been calc'ed so far
    short future[68 - NUM_GENES];      // total of 200 bytes
};
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
struct ORBITS_INFO      // for saving orbits data in a GIF file
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
#define MAX_VIDEO_MODES 300       // maximum entries in fractint.cfg
#define AUTO_INVERT -123456.789
#define MAX_NUM_ATTRACTORS 8
extern  long     g_l_at_rad;      // finite attractor radius
extern  double   g_f_at_rad;      // finite attractor radius
#define NUM_IFS_FUNCTIONS 64
#define NUM_IFS_PARAMS    7
#define NUM_IFS_3D_PARAMS 13

enum class fractal_type;

struct MOREPARAMS
{
    fractal_type type;                      // index in fractalname of the fractal
    char const *param[MAX_PARAMS-4];     // name of the parameters
    double   paramvalue[MAX_PARAMS-4];   // default parameter values
};

enum class symmetry_type
{
    NONE                = 0,
    X_AXIS_NO_PARAM     = -1,
    X_AXIS              = 1,
    Y_AXIS_NO_PARAM     = -2,
    Y_AXIS              = 2,
    XY_AXIS_NO_PARAM    = -3,
    XY_AXIS             = 3,
    ORIGIN_NO_PARAM     = -4,
    ORIGIN              = 4,
    PI_SYM_NO_PARAM     = -5,
    PI_SYM              = 5,
    X_AXIS_NO_IMAG      = -6,
    X_AXIS_NO_REAL      = 6,
    NO_PLOT             = 99,
    SETUP               = 100,
    NOT_FORCED          = 999
};

enum class fractal_type;

struct fractalspecificstuff
{
    char const  *name;                  // name of the fractal
                                        // (leading "*" supresses name display)
    char const  *param[4];              // name of the parameters
    double paramvalue[4];               // default parameter values
    int   helptext;                     // helpdefs.h HT_xxxx, -1 for none
    int   helpformula;                  // helpdefs.h HF_xxxx, -1 for none
    unsigned flags;                     // constraints, bits defined below
    float xmin;                         // default XMIN corner
    float xmax;                         // default XMAX corner
    float ymin;                         // default YMIN corner
    float ymax;                         // default YMAX corner
    int   isinteger;                    // 1 if integerfractal, 0 otherwise
    fractal_type tojulia;               // mandel-to-julia switch
    fractal_type tomandel;              // julia-to-mandel switch
    fractal_type tofloat;               // integer-to-floating switch
    symmetry_type symmetry;             // applicable symmetry logic
                                        //  0 = no symmetry
                                        // -1 = y-axis symmetry (If No Params)
                                        //  1 = y-axis symmetry
                                        // -2 = x-axis symmetry (No Parms)
                                        //  2 = x-axis symmetry
                                        // -3 = y-axis AND x-axis (No Parms)
                                        //  3 = y-axis AND x-axis symmetry
                                        // -4 = polar symmetry (No Parms)
                                        //  4 = polar symmetry
                                        //  5 = PI (sin/cos) symmetry
                                        //  6 = NEWTON (power) symmetry
                                        //
    int (*orbitcalc)();                 // function that calculates one orbit
    int (*per_pixel)();                 // once-per-pixel init
    bool (*per_image)();                // once-per-image setup
    int (*calctype)();                  // name of main fractal function
    int orbit_bailout;                  // usual bailout value for orbit calc
};

enum class fractal_type;

struct AlternateMath
{
    fractal_type type;                  // index in fractalname of the fractal
    bf_math_type math;                  // kind of math used
    int (*orbitcalc)();                 // function that calculates one orbit
    int (*per_pixel)();                 // once-per-pixel init
    bool (*per_image)();                // once-per-image setup
};

// defines for inside/outside
#define COLOR_BLACK 0
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

// for bailoutest
enum class bailouts
{
    Mod,
    Real,
    Imag,
    Or,
    And,
    Manh,
    Manr
};

enum class Major
{
    breadth_first,
    depth_first,
    random_walk,
    random_run
};

enum class Minor
{
    left_first,
    right_first
};

enum orbit_save_flags
{
    osf_raw = 1,
    osf_midi = 2
};

enum class raytrace_formats
{
    none = 0,
    povray = 1,
    vivid = 2,
    raw = 3,
    mtv = 4,
    rayshade = 5,
    acrospin = 6,
    dxf = 7
};

enum class record_colors_mode
{
    none = 0,
    automatic = 'a',
    comment = 'c',
    yes = 'y'
};

enum class true_color_mode
{
    default_color = 0,
    iterate = 1
};

enum class init_orbit_mode
{
    normal = 0,
    value = 1,
    pixel = 2
};

// bitmask defines for fractalspecific flags
#define  NOZOOM         1    // zoombox not allowed at all
#define  NOGUESS        2    // solid guessing not allowed
#define  NOTRACE        4    // boundary tracing not allowed
#define  NOROTATE       8    // zoombox rotate/stretch not allowed
#define  NORESUME      16    // can't interrupt and resume
#define  INFCALC       32    // this type calculates forever
#define  TRIG1         64    // number of trig functions in formula
#define  TRIG2        128
#define  TRIG3        192
#define  TRIG4        256
#define  PARMS3D     1024    // uses 3d parameters
#define  OKJB        2048    // works with Julibrot
#define  MORE        4096    // more than 4 parms
#define  BAILTEST    8192    // can use different bailout tests
#define  BF_MATH    16384    // supports arbitrary precision
#define  LD_MATH    32768    // supports long double

// more bitmasks for evolution mode flag
#define FIELDMAP        1    // steady field varyiations across screen
#define RANDWALK        2    // newparm = lastparm +- rand()
#define RANDPARAM       4    // newparm = constant +- rand()
#define NOGROUT         8    // no gaps between images

#define DEFAULT_FRACTAL_TYPE      ".gif"
#define ALTERNATE_FRACTAL_TYPE    ".fra"

inline int sqr(int x)
{
    return x*x;
}

inline float sqr(float x)
{
    return x*x;
}

inline double sqr(double x)
{
    return x*x;
}

inline long lsqr(long x)
{
    extern int g_bit_shift;
    extern long multiply(long x, long y, int n);
    return multiply(x, x, g_bit_shift);
}

#define CMPLXmod(z)     (sqr((z).x)+sqr((z).y))
#define CMPLXconj(z)    ((z).y =  -((z).y))
#define LCMPLXmod(z)    (lsqr((z).x)+lsqr((z).y))
#define LCMPLXconj(z)   ((z).y =  -((z).y))
#define PER_IMAGE   (g_fractal_specific[static_cast<int>(g_fractal_type)].per_image)
#define PER_PIXEL   (g_fractal_specific[static_cast<int>(g_fractal_type)].per_pixel)
#define ORBIT_CALC   (g_fractal_specific[static_cast<int>(g_fractal_type)].orbitcalc)

// 3D stuff - formerly in 3d.h
#define    CMAX    4   // maximum column (4 x 4 matrix)
#define    RMAX    4   // maximum row    (4 x 4 matrix)
typedef double MATRIX [RMAX] [CMAX];  // matrix of doubles
typedef int   IMATRIX [RMAX] [CMAX];  // matrix of ints
typedef long  LMATRIX [RMAX] [CMAX];  // matrix of longs
/* A MATRIX is used to describe a transformation from one coordinate
system to another.  Multiple transformations may be concatenated by
multiplying their transformation matrices. */
typedef double VECTOR [3];  // vector of doubles
typedef int   IVECTOR [3];  // vector of ints
typedef long  LVECTOR [3];  // vector of longs
/* A VECTOR is an array of three coordinates [x,y,z] representing magnitude
and direction. A fourth dimension is assumed to always have the value 1, but
is not in the data structure */
inline double dot_product(VECTOR v1, VECTOR v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}
#ifdef PI
#undef PI
#endif
#define PI 3.14159265358979323846
#define SPHERE    g_init_3d[0]     // sphere? 1 = yes, 0 = no
#define ILLUMINE  (FILLTYPE>4)  // illumination model
// regular 3D
#define XROT      g_init_3d[1]     // rotate x-axis 60 degrees
#define YROT      g_init_3d[2]     // rotate y-axis 90 degrees
#define ZROT      g_init_3d[3]     // rotate x-axis  0 degrees
#define XSCALE    g_init_3d[4]     // scale x-axis, 90 percent
#define YSCALE    g_init_3d[5]     // scale y-axis, 90 percent
// sphere 3D
#define PHI1      g_init_3d[1]     // longitude start, 180
#define PHI2      g_init_3d[2]     // longitude end ,   0
#define THETA1    g_init_3d[3]     // latitude start,-90 degrees
#define THETA2    g_init_3d[4]     // latitude stop,  90 degrees
#define RADIUS    g_init_3d[5]     // should be user input
// common parameters
#define ROUGH     g_init_3d[6]     // scale z-axis, 30 percent
#define WATERLINE g_init_3d[7]     // water level
#define FILLTYPE  g_init_3d[8]     // fill type
#define ZVIEWER   g_init_3d[9]     // perspective view point
#define XSHIFT    g_init_3d[10]    // x shift
#define YSHIFT    g_init_3d[11]    // y shift
#define XLIGHT    g_init_3d[12]    // x light vector coordinate
#define YLIGHT    g_init_3d[13]    // y light vector coordinate
#define ZLIGHT    g_init_3d[14]    // z light vector coordinate
#define LIGHTAVG  g_init_3d[15]    // number of points to average
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
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
// TODO: make sure X Window System font can render these chars
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

enum class jiim_types
{
    JIIM = 0,
    ORBIT
};

struct WORKLIST     // work list entry for std escape time engines
{
    int xxstart;    // screen window for this entry
    int xxstop;
    int yystart;
    int yystop;
    int yybegin;    // start row within window, for 2pass/ssg resume
    int sym;        // if symmetry in window, prevents bad combines
    int pass;       // for 2pass and solid guessing
    int xxbegin;    // start col within window, =0 except on resume
};

#define MAXCALCWORK 12
struct coords
{
    int x, y;
};

struct dblcoords
{
    double x, y;
};

extern void (*ltrig0)(), (*ltrig1)(), (*ltrig2)(), (*ltrig3)();
extern void (*dtrig0)(), (*dtrig1)(), (*dtrig2)(), (*dtrig3)();

struct trig_funct_lst
{
    char const *name;
    void (*lfunct)();
    void (*dfunct)();
    void (*mfunct)();
};

// for overlay return stack
#define BIG 100000.0
#define CTL(x) ((x)&0x1f)
// nonalpha tests if we have a control character
inline bool nonalpha(int c)
{
    return c < 32 || c > 127;
}
/* keys; FIK = "FractInt Key"
 * Use this prefix to disambiguate key name symbols used in the fractint source
 * from symbols defined by the external environment, i.e. "DELETE" on Win32
 */
#define FIK_ALT_A           1030
#define FIK_ALT_S           1031
#define FIK_ALT_F1          1104
#define FIK_ALT_F2          1105
#define FIK_ALT_F3          1106
#define FIK_ALT_F4          1107
#define FIK_ALT_F5          1108
#define FIK_ALT_F6          1109
#define FIK_ALT_F7          1110
#define FIK_ALT_F8          1111
#define FIK_ALT_F9          1112
#define FIK_ALT_F10         1113
#define FIK_CTL_A           1
#define FIK_CTL_B           2
#define FIK_CTL_E           5
#define FIK_CTL_F           6
#define FIK_CTL_G           7
#define FIK_CTL_H           8
#define FIK_CTL_O           15
#define FIK_CTL_P           16
#define FIK_CTL_S           19
#define FIK_CTL_U           21
#define FIK_CTL_X           24
#define FIK_CTL_Y           25
#define FIK_CTL_Z           26
#define FIK_CTL_BACKSLASH   28
#define FIK_CTL_DEL         1147
#define FIK_CTL_DOWN_ARROW  1145
#define FIK_CTL_END         1117
#define FIK_CTL_ENTER       10
#define FIK_CTL_ENTER_2     1010
#define FIK_CTL_F1          1094
#define FIK_CTL_F2          1095
#define FIK_CTL_F3          1096
#define FIK_CTL_F4          1097
#define FIK_CTL_F5          1098
#define FIK_CTL_F6          1099
#define FIK_CTL_F7          1100
#define FIK_CTL_F8          1101
#define FIK_CTL_F9          1102
#define FIK_CTL_F10         1103
#define FIK_CTL_HOME        1119
#define FIK_CTL_INSERT      1146
#define FIK_CTL_LEFT_ARROW  1115
#define FIK_CTL_MINUS       1142
#define FIK_CTL_PAGE_DOWN   1118
#define FIK_CTL_PAGE_UP     1132
#define FIK_CTL_PLUS        1144
#define FIK_CTL_RIGHT_ARROW 1116
#define FIK_CTL_TAB         1148
#define FIK_CTL_UP_ARROW    1141
#define FIK_SHF_TAB         1015  // shift tab aka BACKTAB
#define FIK_BACKSPACE       8
#define FIK_DELETE          1083
#define FIK_DOWN_ARROW      1080
#define FIK_END             1079
#define FIK_ENTER           13
#define FIK_ENTER_2         1013
#define FIK_ESC             27
#define FIK_F1              1059
#define FIK_F2              1060
#define FIK_F3              1061
#define FIK_F4              1062
#define FIK_F5              1063
#define FIK_F6              1064
#define FIK_F7              1065
#define FIK_F8              1066
#define FIK_F9              1067
#define FIK_F10             1068
#define FIK_HOME            1071
#define FIK_INSERT          1082
#define FIK_LEFT_ARROW      1075
#define FIK_PAGE_DOWN       1081
#define FIK_PAGE_UP         1073
#define FIK_RIGHT_ARROW     1077
#define FIK_SPACE           32
#define FIK_SF1             1084
#define FIK_SF2             1085
#define FIK_SF3             1086
#define FIK_SF4             1087
#define FIK_SF5             1088
#define FIK_SF6             1089
#define FIK_SF7             1090
#define FIK_SF8             1091
#define FIK_SF9             1092
#define FIK_SF10            1093
#define FIK_TAB             9
#define FIK_UP_ARROW        1072
#define FIK_ALT_1           1120
#define FIK_ALT_2           1121
#define FIK_ALT_3           1122
#define FIK_ALT_4           1123
#define FIK_ALT_5           1124
#define FIK_ALT_6           1125
#define FIK_ALT_7           1126
#define FIK_CTL_KEYPAD_5    1143
#define FIK_KEYPAD_5        1076

enum text_colors
{
    BLACK = 0,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN, // dirty yellow on cga
    WHITE,

    // use values below this for foreground only, they don't work background
    GRAY, // don't use this much - is black on cga
    L_BLUE,
    L_GREEN,
    L_CYAN,
    L_RED,
    L_MAGENTA,
    YELLOW,
    L_WHITE,
    INVERSE = 0x8000, // when 640x200x2 text or mode 7, inverse
    BRIGHT = 0x4000 // when mode 7, bright
};
// and their use:
#define C_TITLE           g_text_color[0]+BRIGHT
#define C_TITLE_DEV       g_text_color[1]
#define C_HELP_HDG        g_text_color[2]+BRIGHT
#define C_HELP_BODY       g_text_color[3]
#define C_HELP_INSTR      g_text_color[4]
#define C_HELP_LINK       g_text_color[5]+BRIGHT
#define C_HELP_CURLINK    g_text_color[6]+INVERSE
#define C_PROMPT_BKGRD    g_text_color[7]
#define C_PROMPT_TEXT     g_text_color[8]
#define C_PROMPT_LO       g_text_color[9]
#define C_PROMPT_MED      g_text_color[10]
#ifndef XFRACT
#define C_PROMPT_HI       g_text_color[11]+BRIGHT
#else
#define C_PROMPT_HI       g_text_color[11]
#endif
#define C_PROMPT_INPUT    g_text_color[12]+INVERSE
#define C_PROMPT_CHOOSE   g_text_color[13]+INVERSE
#define C_CHOICE_CURRENT  g_text_color[14]+INVERSE
#define C_CHOICE_SP_INSTR g_text_color[15]
#define C_CHOICE_SP_KEYIN g_text_color[16]+BRIGHT
#define C_GENERAL_HI      g_text_color[17]+BRIGHT
#define C_GENERAL_MED     g_text_color[18]
#define C_GENERAL_LO      g_text_color[19]
#define C_GENERAL_INPUT   g_text_color[20]+INVERSE
#define C_DVID_BKGRD      g_text_color[21]
#define C_DVID_HI         g_text_color[22]+BRIGHT
#define C_DVID_LO         g_text_color[23]
#define C_STOP_ERR        g_text_color[24]+BRIGHT
#define C_STOP_INFO       g_text_color[25]+BRIGHT
#define C_TITLE_LOW       g_text_color[26]
#define C_AUTHDIV1        g_text_color[27]+INVERSE
#define C_AUTHDIV2        g_text_color[28]+INVERSE
#define C_PRIMARY         g_text_color[29]
#define C_CONTRIB         g_text_color[30]
// structure for xmmmoveextended parameter
struct XMM_Move
{
    unsigned long   Length;
    unsigned int    SourceHandle;
    unsigned long   SourceOffset;
    unsigned int    DestHandle;
    unsigned long   DestOffset;
};
// structure passed to fullscreen_prompts
struct fullscreenvalues
{
    int type;   // 'd' for double, 'f' for float, 's' for string,
    // 'D' for integer in double, '*' for comment
    // 'i' for integer, 'y' for yes=1 no=0
    // 0x100+n for string of length n
    // 'l' for one of a list of strings
    // 'L' for long
    union
    {
        double dval;        // when type 'd' or 'f'
        int    ival;        // when type is 'i'
        long   Lval;        // when type is 'L'
        char   sval[16];    // when type is 's'
        char  *sbuf;        // when type is 0x100+n
        struct              // when type is 'l'
        {
            int  val;       // selected choice
            int  vlen;      // char len per choice
            char const **list;  // list of values
            int  llen;      // number of values
        } ch;
    } uval;
};

template <typename T>
int sign(T x)
{
    return (T{} < x) - (x < T{});
}

#endif
