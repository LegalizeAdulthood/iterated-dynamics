/* FRACTINT.H - common structures and values for the FRACTINT routines */

#ifndef FRACTINT_H
#define FRACTINT_H

typedef BYTE BOOLEAN;

#ifndef C6
#ifndef _fastcall
#define _fastcall       /* _fastcall is a Microsoft C6.00 extension */
#endif
#endif

#ifndef XFRACT
#define ftimex ftime
typedef int SEGTYPE;
typedef unsigned USEGTYPE;
#ifdef __TURBOC__
#   define _bios_printer(a,b,c)   biosprint((a),(c),(b))
#   define _bios_serialcom(a,b,c) bioscom((a),(c),(b))
#else
#ifndef __WATCOMC__
#ifndef MK_FP
#   define MK_FP(seg,off) (VOIDFARPTR )( (((long)(seg))<<16) | \
                                          ((unsigned)(off)) )
#endif
#endif
#endif
#else
typedef char * SEGTYPE;
typedef char * USEGTYPE;
#   define MK_FP(seg,off) (VOIDFARPTR )(seg+off)
#include <sys/types.h> /* need size_t */
#endif

#ifndef XFRACT
#define clock_ticks() clock()
#endif

#ifdef XFRACT
#define difftime(now,then) ((now)-(then))
#endif

/* for gotos in former FRACTINT.C pieces */
#define RESTART           1
#define IMAGESTART        2
#define RESTORESTART      3
#define CONTINUE          4

#define MAXMAXLINELENGTH  128   /* upper limit for maxlinelength for PARs */
#define MINMAXLINELENGTH  40    /* lower limit for maxlinelength for PARs */

#define MSGLEN 80               /* handy buffer size for messages */
#define MAXCMT 57               /* length of par comments       */
#define MAXPARAMS 10            /* maximum number of parameters */
#define MAXPIXELS   32767       /* Maximum pixel count across/down the screen */
#define OLDMAXPIXELS 2048       /* Limit of some old fixed arrays */
#define MINPIXELS 10            /* Minimum pixel count across/down the screen */
#define DEFAULTASPECT ((float)0.75)/* Assumed overall screen dimensions, y/x  */
#define DEFAULTASPECTDRIFT ((float)0.02) /* drift of < 2% is forced to 0% */

struct videoinfo {              /* All we need to know about a Video Adapter */
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
        int     xdots;          /* number of dots across the screen     */
        int     ydots;          /* number of dots down the screen       */
        int     colors;         /* number of colors available           */
        };

typedef struct videoinfo far        VIDEOINFO;
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

#define VERSION 17  /* file version, independent of system */
   /* increment this EVERY time the fractal_info structure changes */
   
struct fractal_info         /*  for saving data in GIF file     */
{
    char  info_id[8];       /* Unique identifier for info block */
    short iterationsold;    /* Pre version 18.24 */
    short fractal_type;     /* 0=Mandelbrot 1=Julia 2= ... */
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
    short version;          /* used to be 'future[0]' */
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
                        /* version 2 stuff */
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
                        /* version 3 stuff, release 13 */
    short outside;
                        /* version 4 stuff, release 14 */
    double x3rd;          /* 3rd corner */
    double y3rd;
    char stdcalcmode;     /* 1/2/g/b */
    char useinitorbit;    /* init Mandelbrot orbit flag */
    short calc_status;    /* resumable, finished, etc */
    long tot_extend_len;  /* total length of extension blocks in .gif file */
    short distestold;
    short floatflag;
    short bailoutold;
    long calctime;
    BYTE trigndx[4];      /* which trig functions selected */
    short finattract;
    double initorbit[2];  /* init Mandelbrot orbit values */
    short periodicity;    /* periodicity checking */
                        /* version 5 stuff, release 15 */
    short pot16bit;       /* save 16 bit continuous potential info */
    float faspectratio;   /* finalaspectratio, y/x */
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
    short distestwidth;
                        /* version 7 stuff, release 16 */
    double dparm3;
    double dparm4;
                        /* version 8 stuff, release 17 */
    short fillcolor;
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
                        /* version 10 stuff, release 19 */
    long bailout;
    short bailoutest;
    long iterations;
    short bf_math;
    short bflength;
    short yadjust;        /* yikes! we left this out ages ago! */
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
    short future[7];     /* for stuff we haven't thought of yet */
};

#define ITEMNAMELEN 18   /* max length of names in .frm/.l/.ifs/.fc */
struct history_info
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
    short flag3d;
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
    char itemname[ITEMNAMELEN+1];
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

typedef struct history_info HISTORY;

struct formula_info         /*  for saving formula data in GIF file     */
{
    char  form_name[40];
    short uses_p1;
    short uses_p2;
    short uses_p3;
    short uses_ismand;
    short ismand;
    short uses_p4;
    short uses_p5;
    short future[6];       /* for stuff we haven't thought of, yet */
};

#ifndef XFRACT
enum stored_at_values
   {
   NOWHERE,
   EXTRA,
   FARMEM,
   EXPANDED,
   EXTENDED,
   DISK
   };
#endif

#ifdef XFRACT
enum stored_at_values
   {
   NOWHERE,
   FARMEM,
   DISK
   };
#endif

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
   unsigned short this_gen_rseed;
   double fiddlefactor;
   double paramrangex;
   double paramrangey;
   double opx;
   double opy;
   short odpx;
   short odpy;
   short px;
   short py;
   short sxoffs;
   short syoffs;
   short xdots;
   short ydots;
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
#ifndef XFRACT
#define MAXVIDEOTABLE 40        /* size of the resident video modes table */
#else
#define MAXVIDEOTABLE 2         /* size of the resident video modes table */
#endif

#define AUTOINVERT -123456.789
#define ENDVID 22400   /* video table uses extra seg up to here */

#define N_ATTR 8                        /* max number of attractors     */

extern  long     l_at_rad;      /* finite attractor radius  */
extern  double   f_at_rad;      /* finite attractor radius  */

#define NUMIFS    64     /* number of ifs functions in ifs array */
#define IFSPARM    7     /* number of ifs parameters */
#define IFS3DPARM 13     /* number of ifs 3D parameters */

struct moreparams
{
   int      type;                       /* index in fractalname of the fractal */
   char     far *param[MAXPARAMS-4];    /* name of the parameters */
   double   paramvalue[MAXPARAMS-4];    /* default parameter values */
};

typedef struct moreparams far       MOREPARAMS;

struct fractalspecificstuff
{
   char  *name;                         /* name of the fractal */
                                        /* (leading "*" supresses name display) */
   char  far *param[4];                 /* name of the parameters */
   double paramvalue[4];                /* default parameter values */
   int   helptext;                      /* helpdefs.h HT_xxxx, -1 for none */
   int   helpformula;                   /* helpdefs.h HF_xxxx, -1 for none */
   unsigned flags;                      /* constraints, bits defined below */
   float xmin;                          /* default XMIN corner */
   float xmax;                          /* default XMAX corner */
   float ymin;                          /* default YMIN corner */
   float ymax;                          /* default YMAX corner */
   int   isinteger;                     /* 1 if integerfractal, 0 otherwise */
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
#ifdef XFRACT
   int (*orbitcalc)();  /* function that calculates one orbit */
#else
   int (*orbitcalc)(void);      /* function that calculates one orbit */
#endif
   int (*per_pixel)(void);      /* once-per-pixel init */
   int (*per_image)(void);      /* once-per-image setup */
   int (*calctype)(void);       /* name of main fractal function */
   long orbit_bailout;          /* usual bailout value for orbit calc */
};

struct alternatemathstuff
{
   int type;                    /* index in fractalname of the fractal */
   int math;                    /* kind of math used */
#ifdef XFRACT
   int (*orbitcalc)();  /* function that calculates one orbit */
#else
   int (*orbitcalc)(void);      /* function that calculates one orbit */
#endif
   int (*per_pixel)(void);      /* once-per-pixel init */
   int (*per_image)(void);      /* once-per-image setup */
};

typedef struct alternatemathstuff ALTERNATE;

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

/* bitmask defines for fractalspecific flags */
#define  NOZOOM         1    /* zoombox not allowed at all          */
#define  NOGUESS        2    /* solid guessing not allowed          */
#define  NOTRACE        4    /* boundary tracing not allowed        */
#define  NOROTATE       8    /* zoombox rotate/stretch not allowed  */
#define  NORESUME      16    /* can't interrupt and resume          */
#define  INFCALC       32    /* this type calculates forever        */
#define  TRIG1         64    /* number of trig functions in formula */
#define  TRIG2        128
#define  TRIG3        192
#define  TRIG4        256
#define  WINFRAC      512    /* supported in WinFrac                */
#define  PARMS3D     1024    /* uses 3d parameters                  */
#define  OKJB        2048    /* works with Julibrot                 */
#define  MORE        4096    /* more than 4 parms                   */
#define  BAILTEST    8192    /* can use different bailout tests     */
#define  BF_MATH    16384    /* supports arbitrary precision        */
#define  LD_MATH    32768    /* supports long double                */


/* more bitmasks for evolution mode flag */
#define FIELDMAP        1    /*steady field varyiations across screen */
#define RANDWALK        2    /* newparm = lastparm +- rand()                   */
#define RANDPARAM       4    /* newparm = constant +- rand()                   */
#define NOGROUT         8    /* no gaps between images                                   */


extern struct fractalspecificstuff far fractalspecific[];
extern struct fractalspecificstuff far *curfractalspecific;

#define DEFAULTFRACTALTYPE      ".gif"
#define ALTERNATEFRACTALTYPE    ".fra"


#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#ifndef lsqr
#define lsqr(x) (multiply((x),(x),bitshift))
#endif

#define CMPLXmod(z)     (sqr((z).x)+sqr((z).y))
#define CMPLXconj(z)    ((z).y =  -((z).y))
#define LCMPLXmod(z)    (lsqr((z).x)+lsqr((z).y))
#define LCMPLXconj(z)   ((z).y =  -((z).y))

#define PER_IMAGE   (fractalspecific[fractype].per_image)
#define PER_PIXEL   (fractalspecific[fractype].per_pixel)
#define ORBITCALC   (fractalspecific[fractype].orbitcalc)

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
#define SPHERE    init3d[0]             /* sphere? 1 = yes, 0 = no  */
#define ILLUMINE  (FILLTYPE>4)  /* illumination model       */

/* regular 3D */
#define XROT      init3d[1]     /* rotate x-axis 60 degrees */
#define YROT      init3d[2]     /* rotate y-axis 90 degrees */
#define ZROT      init3d[3]     /* rotate x-axis  0 degrees */
#define XSCALE    init3d[4]     /* scale x-axis, 90 percent */
#define YSCALE    init3d[5]     /* scale y-axis, 90 percent */

/* sphere 3D */
#define PHI1      init3d[1]     /* longitude start, 180     */
#define PHI2      init3d[2]     /* longitude end ,   0      */
#define THETA1    init3d[3]         /* latitude start,-90 degrees */
#define THETA2    init3d[4]         /* latitude stop,  90 degrees */
#define RADIUS    init3d[5]     /* should be user input */

/* common parameters */
#define ROUGH     init3d[6]     /* scale z-axis, 30 percent */
#define WATERLINE init3d[7]     /* water level              */
#define FILLTYPE  init3d[8]     /* fill type                */
#define ZVIEWER   init3d[9]     /* perspective view point   */
#define XSHIFT    init3d[10]    /* x shift */
#define YSHIFT    init3d[11]    /* y shift */
#define XLIGHT    init3d[12]    /* x light vector coordinate */
#define YLIGHT    init3d[13]    /* y light vector coordinate */
#define ZLIGHT    init3d[14]    /* z light vector coordinate */
#define LIGHTAVG  init3d[15]    /* number of points to average */

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
#define FK_F1  "F1"
#define FK_F2  "F2"
#define FK_F3  "F3"
#define FK_F4  "F4"
#define FK_F5  "F5"
#define FK_F6  "F6"
#define FK_F7  "F7"
#define FK_F8  "F8"
#define FK_F9  "F9"
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
        int xxstart;    /* screen window for this entry */
        int xxstop;
        int yystart;
        int yystop;
        int yybegin;    /* start row within window, for 2pass/ssg resume */
        int sym;        /* if symmetry in window, prevents bad combines */
        int pass;       /* for 2pass and solid guessing */
        int xxbegin;    /* start col within window, =0 except on resume */
};

typedef struct workliststuff        WORKLIST;


#define MAXCALCWORK 12

struct coords {
    int x,y;
    };

struct dblcoords {
    double x,y;
    };

extern BYTE trigndx[];
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

/* keys */
#define   INSERT         1082
#define   DELETE         1083
#define   PAGE_UP        1073
#define   PAGE_DOWN      1081
#define   CTL_HOME       1119
#define   CTL_END        1117
#define   LEFT_ARROW     1075
#define   RIGHT_ARROW    1077
#define   UP_ARROW       1072
#define   DOWN_ARROW     1080
#define   LEFT_ARROW_2   1115
#define   RIGHT_ARROW_2  1116
#define   UP_ARROW_2     1141
#define   DOWN_ARROW_2   1145
#define   HOME           1071
#define   END            1079
#define   ENTER          13
#define   ENTER_2        1013
#define   CTL_ENTER      10
#define   CTL_ENTER_2    1010
#define   CTL_PAGE_UP    1132
#define   CTL_PAGE_DOWN  1118
#define   CTL_MINUS      1142
#define   CTL_PLUS       1144
#define   CTL_INSERT     1146
#define   CTL_DEL        1147
#define   CTL_BACKSLASH  28
#define   F1             1059
#define   F2             1060
#define   F3             1061
#define   F4             1062
#define   F5             1063
#define   F6             1064
#define   F7             1065
#define   F8             1066
#define   F9             1067
#define   F10            1068
#define   BACKSPACE      8
#define   TAB            9
#define   CTL_TAB        1148
#define   ALT_TAB        1165
#define   BACK_TAB       1015  /* shift tab */
#define   ESC            27
#define   SPACE          32
#define   SF1            1084
#define   SF2            1085
#define   SF3            1086
#define   SF4            1087
#define   SF5            1088
#define   SF6            1089
#define   SF7            1090
#define   SF8            1091
#define   SF9            1092
#define   SF10           1093
/* text colors */
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
extern BYTE txtcolor[];
#define C_TITLE           txtcolor[0]+BRIGHT
#define C_TITLE_DEV       txtcolor[1]
#define C_HELP_HDG        txtcolor[2]+BRIGHT
#define C_HELP_BODY       txtcolor[3]
#define C_HELP_INSTR      txtcolor[4]
#define C_HELP_LINK       txtcolor[5]+BRIGHT
#define C_HELP_CURLINK    txtcolor[6]+INVERSE
#define C_PROMPT_BKGRD    txtcolor[7]
#define C_PROMPT_TEXT     txtcolor[8]
#define C_PROMPT_LO       txtcolor[9]
#define C_PROMPT_MED      txtcolor[10]
#ifndef XFRACT
#define C_PROMPT_HI       txtcolor[11]+BRIGHT
#else
#define C_PROMPT_HI       txtcolor[11]
#endif
#define C_PROMPT_INPUT    txtcolor[12]+INVERSE
#define C_PROMPT_CHOOSE   txtcolor[13]+INVERSE
#define C_CHOICE_CURRENT  txtcolor[14]+INVERSE
#define C_CHOICE_SP_INSTR txtcolor[15]
#define C_CHOICE_SP_KEYIN txtcolor[16]+BRIGHT
#define C_GENERAL_HI      txtcolor[17]+BRIGHT
#define C_GENERAL_MED     txtcolor[18]
#define C_GENERAL_LO      txtcolor[19]
#define C_GENERAL_INPUT   txtcolor[20]+INVERSE
#define C_DVID_BKGRD      txtcolor[21]
#define C_DVID_HI         txtcolor[22]+BRIGHT
#define C_DVID_LO         txtcolor[23]
#define C_STOP_ERR        txtcolor[24]+BRIGHT
#define C_STOP_INFO       txtcolor[25]+BRIGHT
#define C_TITLE_LOW       txtcolor[26]
#define C_AUTHDIV1        txtcolor[27]+INVERSE
#define C_AUTHDIV2        txtcolor[28]+INVERSE
#define C_PRIMARY         txtcolor[29]
#define C_CONTRIB         txtcolor[30]

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
      char  far *sbuf;  /* when type is 0x100+n  */
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

struct DIR_SEARCH               /* Allocate DTA and define structure */
{
     char path[21];             /* DOS path and filespec */
     char attribute;            /* File attributes wanted */
     int  ftime;                /* File creation time */
     int  fdate;                /* File creation date */
     long size;                 /* File size in bytes */
     char filename[13];         /* Filename and extension */
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


struct ext_blk_2 {
   char got_data;
   int length;
   int resume_data;
   };

struct ext_blk_3 {
   char got_data;
   int length;
   char form_name[40];
   short uses_p1;
   short uses_p2;
   short uses_p3;
   short uses_ismand;
   short ismand;
   short uses_p4;
   short uses_p5;
   };

struct ext_blk_4 {
   char got_data;
   int length;
   int far *range_data;
   };

struct ext_blk_5 {
   char got_data;
   int length;
   char far *apm_data;
   };

/* parameter evolution stuff */
struct ext_blk_6 {
   char got_data;
   int length;
   short evolving;
   short gridsz;
   unsigned short this_gen_rseed;
   double fiddlefactor;
   double paramrangex;
   double paramrangey;
   double opx;
   double opy;
   short  odpx;
   short  odpy;
   short  px;
   short  py;
   short  sxoffs;
   short  syoffs;
   short  xdots;
   short  ydots;
   short  ecount;
   short  mutate[NUMGENES];
   };

struct ext_blk_7 {
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

struct SearchPath {
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

/* 
 * The following typedefs allow declaring based data
 * types that are stored in the code segment under MSC,
 * and thus may be overlaid. Use only for constant data.
 * Be sure to use the data right away, since arrays thus
 * declared do not exist when the overlay they belong to
 * is swapped out.
 */

#if (_MSC_VER >= 700 && !defined(WINFRACT))
typedef char __based(__segname("_CODE")) FCODE;
#else
typedef char far FCODE;
#endif

/* pointer to FCODE */
#if (_MSC_VER >= 700 && !defined(WINFRACT))
typedef FCODE * __based(__segname("_CODE")) PFCODE;
#else
typedef FCODE * PFCODE;
#endif

#if (_MSC_VER >= 700 && !defined(WINFRACT))
typedef BYTE __based(__segname("_CODE")) BFCODE;
#else
typedef BYTE far BFCODE;
#endif

#if (_MSC_VER >= 700 && !defined(WINFRACT))
typedef short __based(__segname("_CODE")) SIFCODE;
#else
typedef short far SIFCODE;
#endif

#if (_MSC_VER >= 700 && !defined(WINFRACT))
typedef short __based(__segname("_CODE")) USFCODE;
#else
typedef short far USFCODE;
#endif

#if (_MSC_VER >= 700 && !defined(WINFRACT))
typedef int __based(__segname("_CODE")) IFCODE;
#else
typedef int far IFCODE;
#endif

#if (_MSC_VER >= 700 && !defined(WINFRACT))
typedef unsigned int __based(__segname("_CODE")) UIFCODE;
#else
typedef unsigned int far UIFCODE;
#endif

#if (_MSC_VER >= 700 && !defined(WINFRACT))
typedef long __based(__segname("_CODE")) LFCODE;
#else
typedef long far LFCODE;
#endif

#if (_MSC_VER >= 700 && !defined(WINFRACT))
typedef unsigned long __based(__segname("_CODE")) ULFCODE;
#else
typedef unsigned long far ULFCODE;
#endif

#if (_MSC_VER >= 700 && !defined(WINFRACT))
typedef double __based(__segname("_CODE")) DFCODE;
#else
typedef double far DFCODE;
#endif
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
