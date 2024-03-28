/*
   This file contains two 3 dimensional orbit-type fractal
   generators - IFS and LORENZ3D, along with code to generate
   red/blue 3D images.
*/
#include "port.h"
#include "prototyp.h"

#include "lorenz.h"

#include "3d.h"
#include "calcfrac.h"
#include "check_key.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "drivers.h"
#include "encoder.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "get_color.h"
#include "id_data.h"
#include "ifs.h"
#include "jiim.h"
#include "line3d.h"
#include "mpmath.h"
#include "not_disk_msg.h"
#include "plot3d.h"
#include "resume.h"
#include "sign.h"
#include "sound.h"
#include "stop_msg.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

// orbitcalc is declared with no arguments so jump through hoops here
#define LORBIT(x, y, z) \
   (*(int(*)(long *, long *, long *))g_cur_fractal_specific->orbitcalc)(x, y, z)
#define FORBIT(x, y, z) \
   (*(int(*)(double*, double*, double*))g_cur_fractal_specific->orbitcalc)(x, y, z)

#define RANDOM(x)  (rand()%(x))
/* BAD_PIXEL is used to cutoff orbits that are diverging. It might be better
to test the actual floating point orbit values, but this seems safe for now.
A higher value cannot be used - to test, turn off math coprocessor and
use +2.24 for type ICONS. If BAD_PIXEL is set to 20000, this will abort
Fractint with a math error. Note that this approach precludes zooming in very
far to an orbit type. */

#define BAD_PIXEL  10000L    // pixels can't get this big

struct l_affine
{
    // weird order so a,b,e and c,d,f are vectors
    long a;
    long b;
    long e;
    long c;
    long d;
    long f;
};

struct long3dvtinf // data used by 3d view transform subroutine
{
    long orbit[3];       // interated function orbit value
    long iview[3];       // perspective viewer's coordinates
    long viewvect[3];    // orbit transformed for viewing
    long viewvect1[3];   // orbit transformed for viewing
    long maxvals[3];
    long minvals[3];
    MATRIX doublemat;    // transformation matrix
    MATRIX doublemat1;   // transformation matrix
    long longmat[4][4];  // long version of matrix
    long longmat1[4][4]; // long version of matrix
    int row, col;         // results
    int row1, col1;
    l_affine cvt;
};

struct float3dvtinf // data used by 3d view transform subroutine
{
    double orbit[3];                // interated function orbit value
    double viewvect[3];        // orbit transformed for viewing
    double viewvect1[3];        // orbit transformed for viewing
    double maxvals[3];
    double minvals[3];
    MATRIX doublemat;    // transformation matrix
    MATRIX doublemat1;   // transformation matrix
    int row, col;         // results
    int row1, col1;
    affine cvt;
};

// Routines in this module

static int  ifs2d();
static int  ifs3d();
static int  ifs3dlong();
static int  ifs3dfloat();
static bool l_setup_convert_to_screen(l_affine *);
static void setupmatrix(MATRIX);
static bool long3dviewtransf(long3dvtinf *inf);
static bool float3dviewtransf(float3dvtinf *inf);
static std::FILE *open_orbitsave();
static void plothist(int x, int y, int color);
static bool realtime = false;

long g_max_count;
static int t;
static long l_dx;
static long l_dy;
static long l_dz;
static long l_dt;
static long l_a;
static long l_b;
static long l_c;
static long l_d;
static long l_adt;
static long l_bdt;
static long l_cdt;
static long l_xdt;
static long l_ydt;
static long initorbitlong[3];

static double dx;
static double dy;
static double dz;
static double dt;
static double a;
static double b;
static double c;
static double d;
static double adt;
static double bdt;
static double cdt;
static double xdt;
static double ydt;
static double zdt;
static double initorbitfp[3];

// The following declarations used for Inverse Julia.

static char NoQueue[] =
    "Not enough memory: switching to random walk.\n";

static int      mxhits;
static int      run_length;
Major           g_major_method;
Minor           g_inverse_julia_minor_method;
static affine   s_cvt;
static l_affine lcvt;

static double Cx;
static double Cy;
static long CxLong;
static long CyLong;

/*
 * end of Inverse Julia declarations;
 */

// these are potential user parameters
static bool connect = true;     // flag to connect points with a line
static bool euler = false;      // use implicit euler approximation for dynamic system
static int waste = 100;    // waste this many points before plotting
static int projection = 2; // projection plane - default is to plot x-y

//****************************************************************
//                 zoom box conversion functions
//****************************************************************

/*
   Conversion of complex plane to screen coordinates for rotating zoom box.
   Assume there is an affine transformation mapping complex zoom parallelogram
   to rectangular screen. We know this map must map parallelogram corners to
   screen corners, so we have following equations:

      a*xxmin+b*yymax+e == 0        (upper left)
      c*xxmin+d*yymax+f == 0

      a*xx3rd+b*yy3rd+e == 0        (lower left)
      c*xx3rd+d*yy3rd+f == ydots-1

      a*xxmax+b*yymin+e == xdots-1  (lower right)
      c*xxmax+d*yymin+f == ydots-1

      First we must solve for a,b,c,d,e,f - (which we do once per image),
      then we just apply the transformation to each orbit value.
*/

/*
   Thanks to Sylvie Gallet for the following. The original code for
   setup_convert_to_screen() solved for coefficients of the
   complex-plane-to-screen transformation using a very straight-forward
   application of determinants to solve a set of simulataneous
   equations. The procedure was simple and general, but inefficient.
   The inefficiecy wasn't hurting anything because the routine was called
   only once per image, but it seemed positively sinful to use it
   because the code that follows is SO much more compact, at the
   expense of being less general. Here are Sylvie's notes. I have further
   optimized the code a slight bit.
                                               Tim Wegner
                                               July, 1996
  Sylvie's notes, slightly edited follow:

  You don't need 3x3 determinants to solve these sets of equations because
  the unknowns e and f have the same coefficient: 1.

  First set of 3 equations:
     a*xxmin+b*yymax+e == 0
     a*xx3rd+b*yy3rd+e == 0
     a*xxmax+b*yymin+e == xdots-1
  To make things easy to read, I just replace xxmin, xxmax, xx3rd by x1,
  x2, x3 (ditto for yy...) and xdots-1 by xd.

     a*x1 + b*y2 + e == 0    (1)
     a*x3 + b*y3 + e == 0    (2)
     a*x2 + b*y1 + e == xd   (3)

  I subtract (1) to (2) and (3):
     a*x1      + b*y2      + e == 0   (1)
     a*(x3-x1) + b*(y3-y2)     == 0   (2)-(1)
     a*(x2-x1) + b*(y1-y2)     == xd  (3)-(1)

  I just have to calculate a 2x2 determinant:
     det == (x3-x1)*(y1-y2) - (y3-y2)*(x2-x1)

  And the solution is:
     a = -xd*(y3-y2)/det
     b =  xd*(x3-x1)/det
     e = - a*x1 - b*y2

The same technique can be applied to the second set of equations:

   c*xxmin+d*yymax+f == 0
   c*xx3rd+d*yy3rd+f == ydots-1
   c*xxmax+d*yymin+f == ydots-1

   c*x1 + d*y2 + f == 0    (1)
   c*x3 + d*y3 + f == yd   (2)
   c*x2 + d*y1 + f == yd   (3)

   c*x1      + d*y2      + f == 0    (1)
   c*(x3-x2) + d*(y3-y1)     == 0    (2)-(3)
   c*(x2-x1) + d*(y1-y2)     == yd   (3)-(1)

   det == (x3-x2)*(y1-y2) - (y3-y1)*(x2-x1)

   c = -yd*(y3-y1)/det
   d =  yd*(x3-x2))det
   f = - c*x1 - d*y2

        -  Sylvie
*/

bool setup_convert_to_screen(affine *scrn_cnvt)
{
    double det, xd, yd;

    det = (g_x_3rd-g_x_min)*(g_y_min-g_y_max) + (g_y_max-g_y_3rd)*(g_x_max-g_x_min);
    if (det == 0)
    {
        return true;
    }
    xd = g_logical_screen_x_size_dots/det;
    scrn_cnvt->a =  xd*(g_y_max-g_y_3rd);
    scrn_cnvt->b =  xd*(g_x_3rd-g_x_min);
    scrn_cnvt->e = -scrn_cnvt->a*g_x_min - scrn_cnvt->b*g_y_max;

    det = (g_x_3rd-g_x_max)*(g_y_min-g_y_max) + (g_y_min-g_y_3rd)*(g_x_max-g_x_min);
    if (det == 0)
    {
        return true;
    }
    yd = g_logical_screen_y_size_dots/det;
    scrn_cnvt->c =  yd*(g_y_min-g_y_3rd);
    scrn_cnvt->d =  yd*(g_x_3rd-g_x_max);
    scrn_cnvt->f = -scrn_cnvt->c*g_x_min - scrn_cnvt->d*g_y_max;
    return false;
}

static bool l_setup_convert_to_screen(l_affine *l_cvt)
{
    affine cvt;

    // This function should return a something!
    if (setup_convert_to_screen(&cvt))
    {
        return true;
    }
    l_cvt->a = (long)(cvt.a*g_fudge_factor);
    l_cvt->b = (long)(cvt.b*g_fudge_factor);
    l_cvt->c = (long)(cvt.c*g_fudge_factor);
    l_cvt->d = (long)(cvt.d*g_fudge_factor);
    l_cvt->e = (long)(cvt.e*g_fudge_factor);
    l_cvt->f = (long)(cvt.f*g_fudge_factor);

    return false;
}

//****************************************************************
//   setup functions - put in fractalspecific[fractype].per_image
//****************************************************************

static double orbit;
static long l_orbit;
static long l_sinx;
static long l_cosx;

bool orbit3dlongsetup()
{
    g_max_count = 0L;
    connect = true;
    waste = 100;
    projection = 2;
    if (g_fractal_type == fractal_type::LHENON
        || g_fractal_type == fractal_type::KAM
        || g_fractal_type == fractal_type::KAM3D
        || g_fractal_type == fractal_type::INVERSEJULIA)
    {
        connect = false;
    }
    if (g_fractal_type == fractal_type::LROSSLER)
    {
        waste = 500;
    }
    if (g_fractal_type == fractal_type::LLORENZ)
    {
        projection = 1;
    }

    initorbitlong[0] = g_fudge_factor;  // initial conditions
    initorbitlong[1] = g_fudge_factor;
    initorbitlong[2] = g_fudge_factor;

    if (g_fractal_type == fractal_type::LHENON)
    {
        l_a = (long)(g_params[0]*g_fudge_factor);
        l_b = (long)(g_params[1]*g_fudge_factor);
        l_c = (long)(g_params[2]*g_fudge_factor);
        l_d = (long)(g_params[3]*g_fudge_factor);
    }
    else if (g_fractal_type == fractal_type::KAM || g_fractal_type == fractal_type::KAM3D)
    {
        g_max_count = 1L;
        a   = g_params[0];           // angle
        if (g_params[1] <= 0.0)
        {
            g_params[1] = .01;
        }
        l_b = (long)(g_params[1]*g_fudge_factor);     // stepsize
        l_c = (long)(g_params[2]*g_fudge_factor);     // stop
        l_d = (long) g_params[3];
        t = (int) l_d;      // points per orbit

        l_sinx = (long)(std::sin(a)*g_fudge_factor);
        l_cosx = (long)(std::cos(a)*g_fudge_factor);
        l_orbit = 0;
        initorbitlong[2] = 0;
        initorbitlong[1] = initorbitlong[2];
        initorbitlong[0] = initorbitlong[1];
    }
    else if (g_fractal_type == fractal_type::INVERSEJULIA)
    {
        LComplex Sqrt;

        CxLong = (long)(g_params[0] * g_fudge_factor);
        CyLong = (long)(g_params[1] * g_fudge_factor);

        mxhits    = (int) g_params[2];
        run_length = (int) g_params[3];
        if (mxhits <= 0)
        {
            mxhits = 1;
        }
        else if (mxhits >= g_colors)
        {
            mxhits = g_colors - 1;
        }
        g_params[2] = mxhits;

        setup_convert_to_screen(&s_cvt);
        // Note: using bitshift of 21 for affine, 24 otherwise

        lcvt.a = (long)(s_cvt.a * (1L << 21));
        lcvt.b = (long)(s_cvt.b * (1L << 21));
        lcvt.c = (long)(s_cvt.c * (1L << 21));
        lcvt.d = (long)(s_cvt.d * (1L << 21));
        lcvt.e = (long)(s_cvt.e * (1L << 21));
        lcvt.f = (long)(s_cvt.f * (1L << 21));

        Sqrt = ComplexSqrtLong(g_fudge_factor - 4 * CxLong, -4 * CyLong);

        switch (g_major_method)
        {
        case Major::breadth_first:
            if (!Init_Queue(32*1024UL))
            {
                // can't get queue memory: fall back to random walk
                stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, NoQueue);
                g_major_method = Major::random_walk;
                goto lrwalk;
            }
            EnQueueLong((g_fudge_factor + Sqrt.x) / 2,  Sqrt.y / 2);
            EnQueueLong((g_fudge_factor - Sqrt.x) / 2, -Sqrt.y / 2);
            break;

        case Major::depth_first:
            if (!Init_Queue(32*1024UL))
            {
                // can't get queue memory: fall back to random walk
                stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, NoQueue);
                g_major_method = Major::random_walk;
                goto lrwalk;
            }
            switch (g_inverse_julia_minor_method)
            {
            case Minor::left_first:
                PushLong((g_fudge_factor + Sqrt.x) / 2,  Sqrt.y / 2);
                PushLong((g_fudge_factor - Sqrt.x) / 2, -Sqrt.y / 2);
                break;
            case Minor::right_first:
                PushLong((g_fudge_factor - Sqrt.x) / 2, -Sqrt.y / 2);
                PushLong((g_fudge_factor + Sqrt.x) / 2,  Sqrt.y / 2);
                break;
            }
            break;
        case Major::random_walk:
lrwalk:
            initorbitlong[0] = g_fudge_factor + Sqrt.x / 2;
            g_l_new_z.x = initorbitlong[0];
            initorbitlong[1] =         Sqrt.y / 2;
            g_l_new_z.y = initorbitlong[1];
            break;
        case Major::random_run:
            initorbitlong[0] = g_fudge_factor + Sqrt.x / 2;
            g_l_new_z.x = initorbitlong[0];
            initorbitlong[1] =         Sqrt.y / 2;
            g_l_new_z.y = initorbitlong[1];
            break;
        }
    }
    else
    {
        l_dt = (long)(g_params[0]*g_fudge_factor);
        l_a = (long)(g_params[1]*g_fudge_factor);
        l_b = (long)(g_params[2]*g_fudge_factor);
        l_c = (long)(g_params[3]*g_fudge_factor);
    }

    // precalculations for speed
    l_adt = multiply(l_a, l_dt, g_bit_shift);
    l_bdt = multiply(l_b, l_dt, g_bit_shift);
    l_cdt = multiply(l_c, l_dt, g_bit_shift);
    return true;
}

#define COSB   dx
#define SINABC dy

bool orbit3dfloatsetup()
{
    g_max_count = 0L;
    connect = true;
    waste = 100;
    projection = 2;

    if (g_fractal_type == fractal_type::FPHENON
        || g_fractal_type == fractal_type::FPPICKOVER
        || g_fractal_type == fractal_type::FPGINGERBREAD
        || g_fractal_type == fractal_type::KAMFP
        || g_fractal_type == fractal_type::KAM3DFP
        || g_fractal_type == fractal_type::FPHOPALONG
        || g_fractal_type == fractal_type::INVERSEJULIAFP)
    {
        connect = false;
    }
    if (g_fractal_type == fractal_type::FPLORENZ3D1
        || g_fractal_type == fractal_type::FPLORENZ3D3
        || g_fractal_type == fractal_type::FPLORENZ3D4)
    {
        waste = 750;
    }
    if (g_fractal_type == fractal_type::FPROSSLER)
    {
        waste = 500;
    }
    if (g_fractal_type == fractal_type::FPLORENZ)
    {
        projection = 1; // plot x and z
    }

    initorbitfp[0] = 1;  // initial conditions
    initorbitfp[1] = 1;
    initorbitfp[2] = 1;
    if (g_fractal_type == fractal_type::FPGINGERBREAD)
    {
        initorbitfp[0] = g_params[0];        // initial conditions
        initorbitfp[1] = g_params[1];
    }

    if (g_fractal_type == fractal_type::ICON || g_fractal_type == fractal_type::ICON3D)
    {
        initorbitfp[0] = 0.01;  // initial conditions
        initorbitfp[1] = 0.003;
        connect = false;
        waste = 2000;
    }

    if (g_fractal_type == fractal_type::LATOO)
    {
        connect = false;
    }

    if (g_fractal_type == fractal_type::FPHENON || g_fractal_type == fractal_type::FPPICKOVER)
    {
        a =  g_params[0];
        b =  g_params[1];
        c =  g_params[2];
        d =  g_params[3];
    }
    else if (g_fractal_type == fractal_type::ICON || g_fractal_type == fractal_type::ICON3D)
    {
        initorbitfp[0] = 0.01;  // initial conditions
        initorbitfp[1] = 0.003;
        connect = false;
        waste = 2000;
        // Initialize parameters
        a  =   g_params[0];
        b  =   g_params[1];
        c  =   g_params[2];
        d  =   g_params[3];
    }
    else if (g_fractal_type == fractal_type::KAMFP || g_fractal_type == fractal_type::KAM3DFP)
    {
        g_max_count = 1L;
        a = g_params[0];           // angle
        if (g_params[1] <= 0.0)
        {
            g_params[1] = .01;
        }
        b =  g_params[1];    // stepsize
        c =  g_params[2];    // stop
        l_d = (long) g_params[3];
        t = (int) l_d;      // points per orbit
        g_sin_x = std::sin(a);
        g_cos_x = std::cos(a);
        orbit = 0;
        initorbitfp[2] = 0;
        initorbitfp[1] = initorbitfp[2];
        initorbitfp[0] = initorbitfp[1];
    }
    else if (g_fractal_type == fractal_type::FPHOPALONG
        || g_fractal_type == fractal_type::FPMARTIN
        || g_fractal_type == fractal_type::CHIP
        || g_fractal_type == fractal_type::QUADRUPTWO
        || g_fractal_type == fractal_type::THREEPLY)
    {
        initorbitfp[0] = 0;  // initial conditions
        initorbitfp[1] = 0;
        initorbitfp[2] = 0;
        connect = false;
        a =  g_params[0];
        b =  g_params[1];
        c =  g_params[2];
        d =  g_params[3];
        if (g_fractal_type == fractal_type::THREEPLY)
        {
            COSB   = std::cos(b);
            SINABC = std::sin(a+b+c);
        }
    }
    else if (g_fractal_type == fractal_type::INVERSEJULIAFP)
    {
        DComplex Sqrt;

        Cx = g_params[0];
        Cy = g_params[1];

        mxhits    = (int) g_params[2];
        run_length = (int) g_params[3];
        if (mxhits <= 0)
        {
            mxhits = 1;
        }
        else if (mxhits >= g_colors)
        {
            mxhits = g_colors - 1;
        }
        g_params[2] = mxhits;

        setup_convert_to_screen(&s_cvt);

        // find fixed points: guaranteed to be in the set
        Sqrt = ComplexSqrtFloat(1 - 4 * Cx, -4 * Cy);
        switch (g_major_method)
        {
        case Major::breadth_first:
            if (!Init_Queue(32*1024UL))
            {
                // can't get queue memory: fall back to random walk
                stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, NoQueue);
                g_major_method = Major::random_walk;
                goto rwalk;
            }
            EnQueueFloat((float)((1 + Sqrt.x) / 2), (float)(Sqrt.y / 2));
            EnQueueFloat((float)((1 - Sqrt.x) / 2), (float)(-Sqrt.y / 2));
            break;
        case Major::depth_first:                      // depth first (choose direction)
            if (!Init_Queue(32*1024UL))
            {
                // can't get queue memory: fall back to random walk
                stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, NoQueue);
                g_major_method = Major::random_walk;
                goto rwalk;
            }
            switch (g_inverse_julia_minor_method)
            {
            case Minor::left_first:
                PushFloat((float)((1 + Sqrt.x) / 2), (float)(Sqrt.y / 2));
                PushFloat((float)((1 - Sqrt.x) / 2), (float)(-Sqrt.y / 2));
                break;
            case Minor::right_first:
                PushFloat((float)((1 - Sqrt.x) / 2), (float)(-Sqrt.y / 2));
                PushFloat((float)((1 + Sqrt.x) / 2), (float)(Sqrt.y / 2));
                break;
            }
            break;
        case Major::random_walk:
rwalk:
            initorbitfp[0] = 1 + Sqrt.x / 2;
            g_new_z.x = initorbitfp[0];
            initorbitfp[1] = Sqrt.y / 2;
            g_new_z.y = initorbitfp[1];
            break;
        case Major::random_run:       // random run, choose intervals
            g_major_method = Major::random_run;
            initorbitfp[0] = 1 + Sqrt.x / 2;
            g_new_z.x = initorbitfp[0];
            initorbitfp[1] = Sqrt.y / 2;
            g_new_z.y = initorbitfp[1];
            break;
        }
    }
    else
    {
        dt = g_params[0];
        a =  g_params[1];
        b =  g_params[2];
        c =  g_params[3];

    }

    // precalculations for speed
    adt = a*dt;
    bdt = b*dt;
    cdt = c*dt;

    return true;
}

//****************************************************************
//   orbit functions - put in fractalspecific[fractype].orbitcalc
//****************************************************************

int Minverse_julia_orbit()
{
    static int   random_dir = 0, random_len = 0;
    int    newrow, newcol;
    int    color,  leftright;

    /*
     * First, compute new point
     */
    switch (g_major_method)
    {
    case Major::breadth_first:
        if (QueueEmpty())
        {
            return -1;
        }
        g_new_z = DeQueueFloat();
        break;
    case Major::depth_first:
        if (QueueEmpty())
        {
            return -1;
        }
        g_new_z = PopFloat();
        break;
    case Major::random_walk:
        break;
    case Major::random_run:
        break;
    }

    /*
     * Next, find its pixel position
     */
    newcol = (int)(s_cvt.a * g_new_z.x + s_cvt.b * g_new_z.y + s_cvt.e);
    newrow = (int)(s_cvt.c * g_new_z.x + s_cvt.d * g_new_z.y + s_cvt.f);

    /*
     * Now find the next point(s), and flip a coin to choose one.
     */

    g_new_z       = ComplexSqrtFloat(g_new_z.x - Cx, g_new_z.y - Cy);
    leftright = (RANDOM(2)) ? 1 : -1;

    if (newcol < 1 || newcol >= g_logical_screen_x_dots || newrow < 1 || newrow >= g_logical_screen_y_dots)
    {
        /*
         * MIIM must skip points that are off the screen boundary,
         * since it cannot read their color.
         */
        switch (g_major_method)
        {
        case Major::breadth_first:
            EnQueueFloat((float)(leftright * g_new_z.x), (float)(leftright * g_new_z.y));
            return 1;
        case Major::depth_first:
            PushFloat((float)(leftright * g_new_z.x), (float)(leftright * g_new_z.y));
            return 1;
        case Major::random_run:
        case Major::random_walk:
            break;
        }
    }

    /*
     * Read the pixel's color:
     * For MIIM, if color >= mxhits, discard the point
     *           else put the point's children onto the queue
     */
    color  = getcolor(newcol, newrow);
    switch (g_major_method)
    {
    case Major::breadth_first:
        if (color < mxhits)
        {
            g_put_color(newcol, newrow, color+1);
            EnQueueFloat((float)g_new_z.x, (float)g_new_z.y);
            EnQueueFloat((float)-g_new_z.x, (float)-g_new_z.y);
        }
        break;
    case Major::depth_first:
        if (color < mxhits)
        {
            g_put_color(newcol, newrow, color+1);
            if (g_inverse_julia_minor_method == Minor::left_first)
            {
                if (QueueFullAlmost())
                {
                    PushFloat((float)-g_new_z.x, (float)-g_new_z.y);
                }
                else
                {
                    PushFloat((float)g_new_z.x, (float)g_new_z.y);
                    PushFloat((float)-g_new_z.x, (float)-g_new_z.y);
                }
            }
            else
            {
                if (QueueFullAlmost())
                {
                    PushFloat((float)g_new_z.x, (float)g_new_z.y);
                }
                else
                {
                    PushFloat((float)-g_new_z.x, (float)-g_new_z.y);
                    PushFloat((float)g_new_z.x, (float)g_new_z.y);
                }
            }
        }
        break;
    case Major::random_run:
        if (random_len-- == 0)
        {
            random_len = RANDOM(run_length);
            random_dir = RANDOM(3);
        }
        switch (random_dir)
        {
        case 0:     // left
            break;
        case 1:     // right
            g_new_z.x = -g_new_z.x;
            g_new_z.y = -g_new_z.y;
            break;
        case 2:     // random direction
            g_new_z.x = leftright * g_new_z.x;
            g_new_z.y = leftright * g_new_z.y;
            break;
        }
        if (color < g_colors-1)
        {
            g_put_color(newcol, newrow, color+1);
        }
        break;
    case Major::random_walk:
        if (color < g_colors-1)
        {
            g_put_color(newcol, newrow, color+1);
        }
        g_new_z.x = leftright * g_new_z.x;
        g_new_z.y = leftright * g_new_z.y;
        break;
    }
    return 1;

}

int Linverse_julia_orbit()
{
    static int   random_dir = 0, random_len = 0;
    int    newrow, newcol;
    int    color;

    /*
     * First, compute new point
     */
    switch (g_major_method)
    {
    case Major::breadth_first:
        if (QueueEmpty())
        {
            return -1;
        }
        g_l_new_z = DeQueueLong();
        break;
    case Major::depth_first:
        if (QueueEmpty())
        {
            return -1;
        }
        g_l_new_z = PopLong();
        break;
    case Major::random_walk:
        g_l_new_z = ComplexSqrtLong(g_l_new_z.x - CxLong, g_l_new_z.y - CyLong);
        if (RANDOM(2))
        {
            g_l_new_z.x = -g_l_new_z.x;
            g_l_new_z.y = -g_l_new_z.y;
        }
        break;
    case Major::random_run:
        g_l_new_z = ComplexSqrtLong(g_l_new_z.x - CxLong, g_l_new_z.y - CyLong);
        if (random_len == 0)
        {
            random_len = RANDOM(run_length);
            random_dir = RANDOM(3);
        }
        switch (random_dir)
        {
        case 0:     // left
            break;
        case 1:     // right
            g_l_new_z.x = -g_l_new_z.x;
            g_l_new_z.y = -g_l_new_z.y;
            break;
        case 2:     // random direction
            if (RANDOM(2))
            {
                g_l_new_z.x = -g_l_new_z.x;
                g_l_new_z.y = -g_l_new_z.y;
            }
            break;
        }
    }

    /*
     * Next, find its pixel position
     *
     * Note: had to use a bitshift of 21 for this operation because
     * otherwise the values of lcvt were truncated.  Used bitshift
     * of 24 otherwise, for increased precision.
     */
    newcol = (int)((multiply(lcvt.a, g_l_new_z.x >> (g_bit_shift - 21), 21) +
                    multiply(lcvt.b, g_l_new_z.y >> (g_bit_shift - 21), 21) + lcvt.e) >> 21);
    newrow = (int)((multiply(lcvt.c, g_l_new_z.x >> (g_bit_shift - 21), 21) +
                    multiply(lcvt.d, g_l_new_z.y >> (g_bit_shift - 21), 21) + lcvt.f) >> 21);

    if (newcol < 1 || newcol >= g_logical_screen_x_dots || newrow < 1 || newrow >= g_logical_screen_y_dots)
    {
        /*
         * MIIM must skip points that are off the screen boundary,
         * since it cannot read their color.
         */
        if (RANDOM(2))
        {
            color =  1;
        }
        else
        {
            color = -1;
        }
        switch (g_major_method)
        {
        case Major::breadth_first:
            g_l_new_z = ComplexSqrtLong(g_l_new_z.x - CxLong, g_l_new_z.y - CyLong);
            EnQueueLong(color * g_l_new_z.x, color * g_l_new_z.y);
            break;
        case Major::depth_first:
            g_l_new_z = ComplexSqrtLong(g_l_new_z.x - CxLong, g_l_new_z.y - CyLong);
            PushLong(color * g_l_new_z.x, color * g_l_new_z.y);
            break;
        case Major::random_run:
            random_len--;
        case Major::random_walk:
            break;
        }
        return 1;
    }

    /*
     * Read the pixel's color:
     * For MIIM, if color >= mxhits, discard the point
     *           else put the point's children onto the queue
     */
    color  = getcolor(newcol, newrow);
    switch (g_major_method)
    {
    case Major::breadth_first:
        if (color < mxhits)
        {
            g_put_color(newcol, newrow, color+1);
            g_l_new_z = ComplexSqrtLong(g_l_new_z.x - CxLong, g_l_new_z.y - CyLong);
            EnQueueLong(g_l_new_z.x,  g_l_new_z.y);
            EnQueueLong(-g_l_new_z.x, -g_l_new_z.y);
        }
        break;
    case Major::depth_first:
        if (color < mxhits)
        {
            g_put_color(newcol, newrow, color+1);
            g_l_new_z = ComplexSqrtLong(g_l_new_z.x - CxLong, g_l_new_z.y - CyLong);
            if (g_inverse_julia_minor_method == Minor::left_first)
            {
                if (QueueFullAlmost())
                {
                    PushLong(-g_l_new_z.x, -g_l_new_z.y);
                }
                else
                {
                    PushLong(g_l_new_z.x,  g_l_new_z.y);
                    PushLong(-g_l_new_z.x, -g_l_new_z.y);
                }
            }
            else
            {
                if (QueueFullAlmost())
                {
                    PushLong(g_l_new_z.x,  g_l_new_z.y);
                }
                else
                {
                    PushLong(-g_l_new_z.x, -g_l_new_z.y);
                    PushLong(g_l_new_z.x,  g_l_new_z.y);
                }
            }
        }
        break;
    case Major::random_run:
        random_len--;
        // fall through
    case Major::random_walk:
        if (color < g_colors-1)
        {
            g_put_color(newcol, newrow, color+1);
        }
        break;
    }
    return 1;
}

int lorenz3dlongorbit(long *l_x, long *l_y, long *l_z)
{
    l_xdt = multiply(*l_x, l_dt, g_bit_shift);
    l_ydt = multiply(*l_y, l_dt, g_bit_shift);
    l_dx  = -multiply(l_adt, *l_x, g_bit_shift) + multiply(l_adt, *l_y, g_bit_shift);
    l_dy  =  multiply(l_bdt, *l_x, g_bit_shift) -l_ydt -multiply(*l_z, l_xdt, g_bit_shift);
    l_dz  = -multiply(l_cdt, *l_z, g_bit_shift) + multiply(*l_x, l_ydt, g_bit_shift);

    *l_x += l_dx;
    *l_y += l_dy;
    *l_z += l_dz;
    return 0;
}

int lorenz3d1floatorbit(double *x, double *y, double *z)
{
    double norm;

    xdt = (*x)*dt;
    ydt = (*y)*dt;
    zdt = (*z)*dt;

    // 1-lobe Lorenz
    norm = std::sqrt((*x)*(*x)+(*y)*(*y));
    dx   = (-adt-dt)*(*x) + (adt-bdt)*(*y) + (dt-adt)*norm + ydt*(*z);
    dy   = (bdt-adt)*(*x) - (adt+dt)*(*y) + (bdt+adt)*norm - xdt*(*z) -
           norm*zdt;
    dz   = (ydt/2) - cdt*(*z);

    *x += dx;
    *y += dy;
    *z += dz;
    return 0;
}

int lorenz3dfloatorbit(double *x, double *y, double *z)
{
    xdt = (*x)*dt;
    ydt = (*y)*dt;
    zdt = (*z)*dt;

    // 2-lobe Lorenz (the original)
    dx  = -adt*(*x) + adt*(*y);
    dy  =  bdt*(*x) - ydt - (*z)*xdt;
    dz  = -cdt*(*z) + (*x)*ydt;

    *x += dx;
    *y += dy;
    *z += dz;
    return 0;
}

int lorenz3d3floatorbit(double *x, double *y, double *z)
{
    double norm;

    xdt = (*x)*dt;
    ydt = (*y)*dt;
    zdt = (*z)*dt;

    // 3-lobe Lorenz
    norm = std::sqrt((*x)*(*x)+(*y)*(*y));
    dx   = (-(adt+dt)*(*x) + (adt-bdt+zdt)*(*y)) / 3 +
           ((dt-adt)*((*x)*(*x)-(*y)*(*y)) +
            2*(bdt+adt-zdt)*(*x)*(*y))/(3*norm);
    dy   = ((bdt-adt-zdt)*(*x) - (adt+dt)*(*y)) / 3 +
           (2*(adt-dt)*(*x)*(*y) +
            (bdt+adt-zdt)*((*x)*(*x)-(*y)*(*y)))/(3*norm);
    dz   = (3*xdt*(*x)*(*y)-ydt*(*y)*(*y))/2 - cdt*(*z);

    *x += dx;
    *y += dy;
    *z += dz;
    return 0;
}

int lorenz3d4floatorbit(double *x, double *y, double *z)
{
    xdt = (*x)*dt;
    ydt = (*y)*dt;
    zdt = (*z)*dt;

    // 4-lobe Lorenz
    dx   = (-adt*(*x)*(*x)*(*x) + (2*adt+bdt-zdt)*(*x)*(*x)*(*y) +
            (adt-2*dt)*(*x)*(*y)*(*y) + (zdt-bdt)*(*y)*(*y)*(*y)) /
           (2 * ((*x)*(*x)+(*y)*(*y)));
    dy   = ((bdt-zdt)*(*x)*(*x)*(*x) + (adt-2*dt)*(*x)*(*x)*(*y) +
            (-2*adt-bdt+zdt)*(*x)*(*y)*(*y) - adt*(*y)*(*y)*(*y)) /
           (2 * ((*x)*(*x)+(*y)*(*y)));
    dz   = (2*xdt*(*x)*(*x)*(*y) - 2*xdt*(*y)*(*y)*(*y) - cdt*(*z));

    *x += dx;
    *y += dy;
    *z += dz;
    return 0;
}

int henonfloatorbit(double *x, double *y, double * /*z*/)
{
    double newx, newy;
    newx  = 1 + *y - a*(*x)*(*x);
    newy  = b*(*x);
    *x = newx;
    *y = newy;
    return 0;
}

int henonlongorbit(long *l_x, long *l_y, long * /*l_z*/)
{
    long newx, newy;
    newx = multiply(*l_x, *l_x, g_bit_shift);
    newx = multiply(newx, l_a, g_bit_shift);
    newx  = g_fudge_factor + *l_y - newx;
    newy  = multiply(l_b, *l_x, g_bit_shift);
    *l_x = newx;
    *l_y = newy;
    return 0;
}

int rosslerfloatorbit(double *x, double *y, double *z)
{
    xdt = (*x)*dt;
    ydt = (*y)*dt;

    dx = -ydt - (*z)*dt;
    dy = xdt + (*y)*adt;
    dz = bdt + (*z)*xdt - (*z)*cdt;

    *x += dx;
    *y += dy;
    *z += dz;
    return 0;
}

int pickoverfloatorbit(double *x, double *y, double *z)
{
    double newx, newy, newz;
    newx = std::sin(a*(*y)) - (*z)*std::cos(b*(*x));
    newy = (*z)*std::sin(c*(*x)) - std::cos(d*(*y));
    newz = std::sin(*x);
    *x = newx;
    *y = newy;
    *z = newz;
    return 0;
}

// page 149 "Science of Fractal Images"
int gingerbreadfloatorbit(double *x, double *y, double * /*z*/)
{
    double newx;
    newx = 1 - (*y) + std::fabs(*x);
    *y = *x;
    *x = newx;
    return 0;
}

int rosslerlongorbit(long *l_x, long *l_y, long *l_z)
{
    l_xdt = multiply(*l_x, l_dt, g_bit_shift);
    l_ydt = multiply(*l_y, l_dt, g_bit_shift);

    l_dx  = -l_ydt - multiply(*l_z, l_dt, g_bit_shift);
    l_dy  =  l_xdt + multiply(*l_y, l_adt, g_bit_shift);
    l_dz  =  l_bdt + multiply(*l_z, l_xdt, g_bit_shift)
             - multiply(*l_z, l_cdt, g_bit_shift);

    *l_x += l_dx;
    *l_y += l_dy;
    *l_z += l_dz;

    return 0;
}

// OSTEP  = Orbit Step (and inner orbit value)
// NTURNS = Outside Orbit
// TURN2  = Points per orbit
// a      = Angle
int kamtorusfloatorbit(double *r, double *s, double *z)
{
    double srr;
    if (t++ >= l_d)
    {
        orbit += b;
        (*s) = orbit/3;
        (*r) = (*s);
        t = 0;
        *z = orbit;
        if (orbit > c)
        {
            return 1;
        }
    }
    srr = (*s)-(*r)*(*r);
    (*s) = (*r)*g_sin_x+srr*g_cos_x;
    (*r) = (*r)*g_cos_x-srr*g_sin_x;
    return 0;
}

int kamtoruslongorbit(long *r, long *s, long *z)
{
    long srr;
    if (t++ >= l_d)
    {
        l_orbit += l_b;
        (*s) = l_orbit/3;
        (*r) = (*s);
        t = 0;
        *z = l_orbit;
        if (l_orbit > l_c)
        {
            return 1;
        }
    }
    srr = (*s)-multiply((*r), (*r), g_bit_shift);
    (*s) = multiply((*r), l_sinx, g_bit_shift)+multiply(srr, l_cosx, g_bit_shift);
    (*r) = multiply((*r), l_cosx, g_bit_shift)-multiply(srr, l_sinx, g_bit_shift);
    return 0;
}

int hopalong2dfloatorbit(double *x, double *y, double * /*z*/)
{
    double tmp;
    tmp = *y - sign(*x)*std::sqrt(std::fabs(b*(*x)-c));
    *y = a - *x;
    *x = tmp;
    return 0;
}

int chip2dfloatorbit(double *x, double *y, double * /*z*/)
{
    double tmp;
    tmp = *y - sign(*x) * std::cos(sqr(std::log(std::fabs(b*(*x)-c))))
          * std::atan(sqr(std::log(std::fabs(c*(*x)-b))));
    *y = a - *x;
    *x = tmp;
    return 0;
}

int quadruptwo2dfloatorbit(double *x, double *y, double * /*z*/)
{
    double tmp;
    tmp = *y - sign(*x) * std::sin(std::log(std::fabs(b*(*x)-c)))
          * std::atan(sqr(std::log(std::fabs(c*(*x)-b))));
    *y = a - *x;
    *x = tmp;
    return 0;
}

int threeply2dfloatorbit(double *x, double *y, double * /*z*/)
{
    double tmp;
    tmp = *y - sign(*x)*(std::fabs(std::sin(*x)*COSB+c-(*x)*SINABC));
    *y = a - *x;
    *x = tmp;
    return 0;
}

int martin2dfloatorbit(double *x, double *y, double * /*z*/)
{
    double tmp;
    tmp = *y - std::sin(*x);
    *y = a - *x;
    *x = tmp;
    return 0;
}

int mandelcloudfloat(double *x, double *y, double * /*z*/)
{
    double newx, newy, x2, y2;
    x2 = (*x)*(*x);
    y2 = (*y)*(*y);
    if (x2+y2 > 2)
    {
        return 1;
    }
    newx = x2-y2+a;
    newy = 2*(*x)*(*y)+b;
    *x = newx;
    *y = newy;
    return 0;
}

int dynamfloat(double *x, double *y, double * /*z*/)
{
    DComplex cp, tmp;
    double newx, newy;
    cp.x = b* *x;
    cp.y = 0;
    CMPLXtrig0(cp, tmp);
    newy = *y + dt*std::sin(*x + a*tmp.x);
    if (euler)
    {
        *y = newy;
    }

    cp.x = b* *y;
    cp.y = 0;
    CMPLXtrig0(cp, tmp);
    newx = *x - dt*std::sin(*y + a*tmp.x);
    *x = newx;
    *y = newy;
    return 0;
}

#undef  LAMBDA
#define LAMBDA  g_params[0]
#define ALPHA   g_params[1]
#define BETA    g_params[2]
#define GAMMA   g_params[3]
#define OMEGA   g_params[4]
#define DEGREE  g_params[5]

int iconfloatorbit(double *x, double *y, double *z)
{
    double oldx, oldy, zzbar, zreal, zimag, za, zb, zn, p;

    oldx = *x;
    oldy = *y;

    zzbar = oldx * oldx + oldy * oldy;
    zreal = oldx;
    zimag = oldy;

    for (int i = 1; i <= DEGREE-2; i++)
    {
        za = zreal * oldx - zimag * oldy;
        zb = zimag * oldx + zreal * oldy;
        zreal = za;
        zimag = zb;
    }
    zn = oldx * zreal - oldy * zimag;
    p = LAMBDA + ALPHA * zzbar + BETA * zn;
    *x = p * oldx + GAMMA * zreal - OMEGA * oldy;
    *y = p * oldy - GAMMA * zimag + OMEGA * oldx;

    *z = zzbar;
    return 0;
}
#ifdef LAMBDA
#undef LAMBDA
#undef ALPHA
#undef BETA
#undef GAMMA
#endif

#define PAR_A   g_params[0]
#define PAR_B   g_params[1]
#define PAR_C   g_params[2]
#define PAR_D   g_params[3]

int latoofloatorbit(double *x, double *y, double * /*z*/)
{
    double xold, yold, tmp;

    xold = *x;
    yold = *y;

    //    *x = sin(yold * PAR_B) + PAR_C * sin(xold * PAR_B);
    g_old_z.x = yold * PAR_B;
    g_old_z.y = 0;          // old = (y * B) + 0i (in the complex)
    CMPLXtrig0(g_old_z, g_new_z);
    tmp = (double) g_new_z.x;
    g_old_z.x = xold * PAR_B;
    g_old_z.y = 0;          // old = (x * B) + 0i
    CMPLXtrig1(g_old_z, g_new_z);
    *x  = PAR_C * g_new_z.x + tmp;

    //    *y = sin(xold * PAR_A) + PAR_D * sin(yold * PAR_A);
    g_old_z.x = xold * PAR_A;
    g_old_z.y = 0;          // old = (y * A) + 0i (in the complex)
    CMPLXtrig2(g_old_z, g_new_z);
    tmp = (double) g_new_z.x;
    g_old_z.x = yold * PAR_A;
    g_old_z.y = 0;          // old = (x * B) + 0i
    CMPLXtrig3(g_old_z, g_new_z);
    *y  = PAR_D * g_new_z.x + tmp;

    return 0;
}

#undef PAR_A
#undef PAR_B
#undef PAR_C
#undef PAR_D

//********************************************************************
//   Main fractal engines - put in fractalspecific[fractype].calctype
//********************************************************************

int inverse_julia_per_image()
{
    int color = 0;

    if (g_resuming)              // can't resume
    {
        return -1;
    }

    while (color >= 0)       // generate points
    {
        if (check_key())
        {
            Free_Queue();
            return -1;
        }
        color = g_cur_fractal_specific->orbitcalc();
        g_old_z = g_new_z;
    }
    Free_Queue();
    return 0;
}

int orbit2dfloat()
{
    std::FILE *fp;
    double *soundvar;
    double x, y, z;
    int color, col, row;
    int count;
    int oldrow, oldcol;
    double *p0, *p1, *p2;
    affine cvt;
    int ret;

    p2 = nullptr;
    p1 = p2;
    p0 = p1;
    soundvar = p0;

    fp = open_orbitsave();
    // setup affine screen coord conversion
    setup_convert_to_screen(&cvt);

    // set up projection scheme
    switch (projection)
    {
    case 0:
        p0 = &z;
        p1 = &x;
        p2 = &y;
        break;
    case 1:
        p0 = &x;
        p1 = &z;
        p2 = &y;
        break;
    case 2:
        p0 = &x;
        p1 = &y;
        p2 = &z;
        break;
    }
    switch (g_sound_flag & SOUNDFLAG_ORBITMASK)
    {
    case SOUNDFLAG_X:
        soundvar = &x;
        break;
    case SOUNDFLAG_Y:
        soundvar = &y;
        break;
    case SOUNDFLAG_Z:
        soundvar = &z;
        break;
    }

    if (g_inside_color > COLOR_BLACK)
    {
        color = g_inside_color;
    }
    else
    {
        color = 2;
    }

    oldrow = -1;
    oldcol = oldrow;
    x = initorbitfp[0];
    y = initorbitfp[1];
    z = initorbitfp[2];
    g_color_iter = 0L;
    ret = 0;
    count = ret;
    if (g_max_iterations > 0x1fffffL || g_max_count)
    {
        g_max_count = 0x7fffffffL;
    }
    else
    {
        g_max_count = g_max_iterations*1024L;
    }

    if (g_resuming)
    {
        start_resume();
        get_resume(sizeof(count), &count, sizeof(color), &color,
                   sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
                   sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(t), &t,
                   sizeof(orbit), &orbit, sizeof(g_color_iter), &g_color_iter,
                   0);
        end_resume();
    }

    while (g_color_iter++ <= g_max_count) // loop until keypress or maxit
    {
        if (driver_key_pressed())
        {
            driver_mute();
            alloc_resume(100, 1);
            put_resume(sizeof(count), &count, sizeof(color), &color,
                       sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
                       sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(t), &t,
                       sizeof(orbit), &orbit, sizeof(g_color_iter), &g_color_iter,
                       0);
            ret = -1;
            break;
        }
        if (++count > 1000)
        {
            // time to switch colors?
            count = 0;
            if (++color >= g_colors)   // another color to switch to?
            {
                color = 1;  // (don't use the background color)
            }
        }

        col = (int)(cvt.a*x + cvt.b*y + cvt.e);
        row = (int)(cvt.c*x + cvt.d*y + cvt.f);
        if (col >= 0 && col < g_logical_screen_x_dots && row >= 0 && row < g_logical_screen_y_dots)
        {
            if (soundvar && (g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
            {
                w_snd((int)(*soundvar*100 + g_base_hertz));
            }
            if ((g_fractal_type != fractal_type::ICON) && (g_fractal_type != fractal_type::LATOO))
            {
                if (oldcol != -1 && connect)
                {
                    driver_draw_line(col, row, oldcol, oldrow, color % g_colors);
                }
                else
                {
                    (*g_plot)(col, row, color % g_colors);
                }
            }
            else
            {
                // should this be using plothist()?
                color = getcolor(col, row)+1;
                if (color < g_colors) // color sticks on last value
                {
                    (*g_plot)(col, row, color);
                }
            }

            oldcol = col;
            oldrow = row;
        }
        else if ((long) std::abs(row) + (long) std::abs(col) > BAD_PIXEL) // sanity check
        {
            return ret;
        }
        else
        {
            oldcol = -1;
            oldrow = oldcol;
        }

        if (FORBIT(p0, p1, p2))
        {
            break;
        }
        if (fp)
        {
            std::fprintf(fp, "%g %g %g 15\n", *p0, *p1, 0.0);
        }
    }
    if (fp)
    {
        std::fclose(fp);
    }
    return ret;
}

int orbit2dlong()
{
    std::FILE *fp;
    long *soundvar;
    long x, y, z;
    int color, col, row;
    int count;
    int oldrow, oldcol;
    long *p0, *p1, *p2;
    l_affine cvt;
    int ret;

    bool start = true;
    p2 = nullptr;
    p1 = p2;
    p0 = p1;
    soundvar = p0;
    fp = open_orbitsave();

    // setup affine screen coord conversion
    l_setup_convert_to_screen(&cvt);

    // set up projection scheme
    switch (projection)
    {
    case 0:
        p0 = &z;
        p1 = &x;
        p2 = &y;
        break;
    case 1:
        p0 = &x;
        p1 = &z;
        p2 = &y;
        break;
    case 2:
        p0 = &x;
        p1 = &y;
        p2 = &z;
        break;
    }
    switch (g_sound_flag & SOUNDFLAG_ORBITMASK)
    {
    case SOUNDFLAG_X:
        soundvar = &x;
        break;
    case SOUNDFLAG_Y:
        soundvar = &y;
        break;
    case SOUNDFLAG_Z:
        soundvar = &z;
        break;
    }

    if (g_inside_color > COLOR_BLACK)
    {
        color = g_inside_color;
    }
    else
    {
        color = 2;
    }
    if (color >= g_colors)
    {
        color = 1;
    }
    oldrow = -1;
    oldcol = oldrow;
    x = initorbitlong[0];
    y = initorbitlong[1];
    z = initorbitlong[2];
    ret = 0;
    count = ret;
    if (g_max_iterations > 0x1fffffL || g_max_count)
    {
        g_max_count = 0x7fffffffL;
    }
    else
    {
        g_max_count = g_max_iterations*1024L;
    }
    g_color_iter = 0L;

    if (g_resuming)
    {
        start_resume();
        get_resume(sizeof(count), &count, sizeof(color), &color,
                   sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
                   sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(t), &t,
                   sizeof(l_orbit), &l_orbit, sizeof(g_color_iter), &g_color_iter,
                   0);
        end_resume();
    }

    while (g_color_iter++ <= g_max_count) // loop until keypress or maxit
    {
        if (driver_key_pressed())
        {
            driver_mute();
            alloc_resume(100, 1);
            put_resume(sizeof(count), &count, sizeof(color), &color,
                       sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
                       sizeof(x), &x, sizeof(y), &y, sizeof(z), &z, sizeof(t), &t,
                       sizeof(l_orbit), &l_orbit, sizeof(g_color_iter), &g_color_iter,
                       0);
            ret = -1;
            break;
        }
        if (++count > 1000)
        {
            // time to switch colors?
            count = 0;
            if (++color >= g_colors)   // another color to switch to?
            {
                color = 1;  // (don't use the background color)
            }
        }

        col = (int)((multiply(cvt.a, x, g_bit_shift) + multiply(cvt.b, y, g_bit_shift) + cvt.e) >> g_bit_shift);
        row = (int)((multiply(cvt.c, x, g_bit_shift) + multiply(cvt.d, y, g_bit_shift) + cvt.f) >> g_bit_shift);
        if (g_overflow)
        {
            g_overflow = false;
            return ret;
        }
        if (col >= 0 && col < g_logical_screen_x_dots && row >= 0 && row < g_logical_screen_y_dots)
        {
            if (soundvar && (g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
            {
                double yy;
                yy = *soundvar;
                yy = yy/g_fudge_factor;
                w_snd((int)(yy*100+g_base_hertz));
            }
            if (oldcol != -1 && connect)
            {
                driver_draw_line(col, row, oldcol, oldrow, color%g_colors);
            }
            else if (!start)
            {
                (*g_plot)(col, row, color%g_colors);
            }
            oldcol = col;
            oldrow = row;
            start = false;
        }
        else if ((long)std::abs(row) + (long)std::abs(col) > BAD_PIXEL) // sanity check
        {
            return ret;
        }
        else
        {
            oldcol = -1;
            oldrow = oldcol;
        }

        // Calculate the next point
        if (LORBIT(p0, p1, p2))
        {
            break;
        }
        if (fp)
        {
            std::fprintf(fp, "%g %g %g 15\n", (double)*p0/g_fudge_factor, (double)*p1/g_fudge_factor, 0.0);
        }
    }
    if (fp)
    {
        std::fclose(fp);
    }
    return ret;
}

static int orbit3dlongcalc()
{
    std::FILE *fp;
    unsigned long count;
    int oldcol, oldrow;
    int oldcol1, oldrow1;
    long3dvtinf inf;
    int color;
    int ret;

    // setup affine screen coord conversion
    l_setup_convert_to_screen(&inf.cvt);

    oldrow = -1;
    oldcol = oldrow;
    oldrow1 = oldcol;
    oldcol1 = oldrow1;
    color = 2;
    if (color >= g_colors)
    {
        color = 1;
    }

    inf.orbit[0] = initorbitlong[0];
    inf.orbit[1] = initorbitlong[1];
    inf.orbit[2] = initorbitlong[2];

    if (driver_diskp())                  // this would KILL a disk drive!
    {
        notdiskmsg();
    }

    fp = open_orbitsave();

    ret = 0;
    count = ret;
    if (g_max_iterations > 0x1fffffL || g_max_count)
    {
        g_max_count = 0x7fffffffL;
    }
    else
    {
        g_max_count = g_max_iterations*1024L;
    }
    g_color_iter = 0L;
    while (g_color_iter++ <= g_max_count) // loop until keypress or maxit
    {
        // calc goes here
        if (++count > 1000)
        {
            // time to switch colors?
            count = 0;
            if (++color >= g_colors)     // another color to switch to?
            {
                color = 1;        // (don't use the background color)
            }
        }
        if (driver_key_pressed())
        {
            driver_mute();
            ret = -1;
            break;
        }

        LORBIT(&inf.orbit[0], &inf.orbit[1], &inf.orbit[2]);
        if (fp)
        {
            std::fprintf(fp, "%g %g %g 15\n", (double)inf.orbit[0]/g_fudge_factor, (double)inf.orbit[1]/g_fudge_factor, (double)inf.orbit[2]/g_fudge_factor);
        }
        if (long3dviewtransf(&inf))
        {
            // plot if inside window
            if (inf.col >= 0)
            {
                if (realtime)
                {
                    g_which_image = stereo_images::RED;
                }
                if ((g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
                {
                    double yy;
                    yy = inf.viewvect[((g_sound_flag & SOUNDFLAG_ORBITMASK) - SOUNDFLAG_X)];
                    yy = yy/g_fudge_factor;
                    w_snd((int)(yy*100+g_base_hertz));
                }
                if (oldcol != -1 && connect)
                {
                    driver_draw_line(inf.col, inf.row, oldcol, oldrow, color%g_colors);
                }
                else
                {
                    (*g_plot)(inf.col, inf.row, color%g_colors);
                }
            }
            else if (inf.col == -2)
            {
                return ret;
            }
            oldcol = inf.col;
            oldrow = inf.row;
            if (realtime)
            {
                g_which_image = stereo_images::BLUE;
                // plot if inside window
                if (inf.col1 >= 0)
                {
                    if (oldcol1 != -1 && connect)
                    {
                        driver_draw_line(inf.col1, inf.row1, oldcol1, oldrow1, color%g_colors);
                    }
                    else
                    {
                        (*g_plot)(inf.col1, inf.row1, color%g_colors);
                    }
                }
                else if (inf.col1 == -2)
                {
                    return ret;
                }
                oldcol1 = inf.col1;
                oldrow1 = inf.row1;
            }
        }
    }
    if (fp)
    {
        std::fclose(fp);
    }
    return ret;
}


static int orbit3dfloatcalc()
{
    std::FILE *fp;
    unsigned long count;
    int oldcol, oldrow;
    int oldcol1, oldrow1;
    int color;
    int ret;
    float3dvtinf inf;

    // setup affine screen coord conversion
    setup_convert_to_screen(&inf.cvt);

    oldrow = -1;
    oldcol = oldrow;
    oldrow1 = -1;
    oldcol1 = oldrow1;
    color = 2;
    if (color >= g_colors)
    {
        color = 1;
    }
    inf.orbit[0] = initorbitfp[0];
    inf.orbit[1] = initorbitfp[1];
    inf.orbit[2] = initorbitfp[2];

    if (driver_diskp())                  // this would KILL a disk drive!
    {
        notdiskmsg();
    }

    fp = open_orbitsave();

    ret = 0;
    if (g_max_iterations > 0x1fffffL || g_max_count)
    {
        g_max_count = 0x7fffffffL;
    }
    else
    {
        g_max_count = g_max_iterations*1024L;
    }
    g_color_iter = 0L;
    count = g_color_iter;
    while (g_color_iter++ <= g_max_count) // loop until keypress or maxit
    {
        // calc goes here
        if (++count > 1000)
        {
            // time to switch colors?
            count = 0;
            if (++color >= g_colors)     // another color to switch to?
            {
                color = 1;        // (don't use the background color)
            }
        }

        if (driver_key_pressed())
        {
            driver_mute();
            ret = -1;
            break;
        }

        FORBIT(&inf.orbit[0], &inf.orbit[1], &inf.orbit[2]);
        if (fp)
        {
            std::fprintf(fp, "%g %g %g 15\n", inf.orbit[0], inf.orbit[1], inf.orbit[2]);
        }
        if (float3dviewtransf(&inf))
        {
            // plot if inside window
            if (inf.col >= 0)
            {
                if (realtime)
                {
                    g_which_image = stereo_images::RED;
                }
                if ((g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
                {
                    w_snd((int)(inf.viewvect[((g_sound_flag & SOUNDFLAG_ORBITMASK) - SOUNDFLAG_X)]*100+g_base_hertz));
                }
                if (oldcol != -1 && connect)
                {
                    driver_draw_line(inf.col, inf.row, oldcol, oldrow, color%g_colors);
                }
                else
                {
                    (*g_plot)(inf.col, inf.row, color%g_colors);
                }
            }
            else if (inf.col == -2)
            {
                return ret;
            }
            oldcol = inf.col;
            oldrow = inf.row;
            if (realtime)
            {
                g_which_image = stereo_images::BLUE;
                // plot if inside window
                if (inf.col1 >= 0)
                {
                    if (oldcol1 != -1 && connect)
                    {
                        driver_draw_line(inf.col1, inf.row1, oldcol1, oldrow1, color%g_colors);
                    }
                    else
                    {
                        (*g_plot)(inf.col1, inf.row1, color%g_colors);
                    }
                }
                else if (inf.col1 == -2)
                {
                    return ret;
                }
                oldcol1 = inf.col1;
                oldrow1 = inf.row1;
            }
        }
    }
    if (fp)
    {
        std::fclose(fp);
    }
    return ret;
}

bool dynam2dfloatsetup()
{
    connect = false;
    euler = false;
    d = g_params[0]; // number of intervals
    if (d < 0)
    {
        d = -d;
        connect = true;
    }
    else if (d == 0)
    {
        d = 1;
    }
    if (g_fractal_type == fractal_type::DYNAMICFP)
    {
        a = g_params[2]; // parameter
        b = g_params[3]; // parameter
        dt = g_params[1]; // step size
        if (dt < 0)
        {
            dt = -dt;
            euler = true;
        }
        if (dt == 0)
        {
            dt = 0.01;
        }
    }
    if (g_outside_color == SUM)
    {
        g_plot = plothist;
    }
    return true;
}

/*
 * This is the routine called to perform a time-discrete dynamical
 * system image.
 * The starting positions are taken by stepping across the image in steps
 * of parameter1 pixels.  maxit differential equation steps are taken, with
 * a step size of parameter2.
 */
int dynam2dfloat()
{
    std::FILE *fp = nullptr;
    double *soundvar = nullptr;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    int color = 0;
    int col = 0;
    int row = 0;
    int oldrow = 0;
    int oldcol = 0;
    double *p0 = nullptr;
    double *p1 = nullptr;
    affine cvt;
    int ret = 0;
    int xstep = 0;
    int ystep = 0; // The starting position step number
    double xpixel = 0.0;
    double ypixel = 0.0; // Our pixel position on the screen

    fp = open_orbitsave();
    // setup affine screen coord conversion
    setup_convert_to_screen(&cvt);

    p0 = &x;
    p1 = &y;

    if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)
    {
        soundvar = &x;
    }
    else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y)
    {
        soundvar = &y;
    }
    else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z)
    {
        soundvar = &z;
    }

    long count = 0;
    if (g_inside_color > COLOR_BLACK)
    {
        color = g_inside_color;
    }
    if (color >= g_colors)
    {
        color = 1;
    }
    oldrow = -1;
    oldcol = oldrow;

    xstep = -1;
    ystep = 0;

    if (g_resuming)
    {
        start_resume();
        get_resume(sizeof(count), &count, sizeof(color), &color,
                   sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
                   sizeof(x), &x, sizeof(y), &y, sizeof(xstep), &xstep,
                   sizeof(ystep), &ystep, 0);
        end_resume();
    }

    ret = 0;
    while (true)
    {
        if (driver_key_pressed())
        {
            driver_mute();
            alloc_resume(100, 1);
            put_resume(sizeof(count), &count, sizeof(color), &color,
                       sizeof(oldrow), &oldrow, sizeof(oldcol), &oldcol,
                       sizeof(x), &x, sizeof(y), &y, sizeof(xstep), &xstep,
                       sizeof(ystep), &ystep, 0);
            ret = -1;
            break;
        }

        xstep ++;
        if (xstep >= d)
        {
            xstep = 0;
            ystep ++;
            if (ystep > d)
            {
                driver_mute();
                ret = -1;
                break;
            }
        }

        xpixel = g_logical_screen_x_size_dots*(xstep+.5)/d;
        ypixel = g_logical_screen_y_size_dots*(ystep+.5)/d;
        x = (double)((g_x_min+g_delta_x*xpixel) + (g_delta_x2*ypixel));
        y = (double)((g_y_max-g_delta_y*ypixel) + (-g_delta_y2*xpixel));
        if (g_fractal_type == fractal_type::MANDELCLOUD)
        {
            a = x;
            b = y;
        }
        oldcol = -1;

        if (++color >= g_colors)     // another color to switch to?
        {
            color = 1;    // (don't use the background color)
        }

        for (count = 0; count < g_max_iterations; count++)
        {
            if (count % 2048L == 0)
            {
                if (driver_key_pressed())
                {
                    break;
                }
            }

            col = (int)(cvt.a*x + cvt.b*y + cvt.e);
            row = (int)(cvt.c*x + cvt.d*y + cvt.f);
            if (col >= 0 && col < g_logical_screen_x_dots && row >= 0 && row < g_logical_screen_y_dots)
            {
                if (soundvar && (g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
                {
                    w_snd((int)(*soundvar*100+g_base_hertz));
                }

                if (count >= g_orbit_delay)
                {
                    if (oldcol != -1 && connect)
                    {
                        driver_draw_line(col, row, oldcol, oldrow, color%g_colors);
                    }
                    else if (count > 0 || g_fractal_type != fractal_type::MANDELCLOUD)
                    {
                        (*g_plot)(col, row, color%g_colors);
                    }
                }
                oldcol = col;
                oldrow = row;
            }
            else if ((long)std::abs(row) + (long)std::abs(col) > BAD_PIXEL)   // sanity check
            {
                return ret;
            }
            else
            {
                oldcol = -1;
                oldrow = oldcol;
            }

            if (FORBIT(p0, p1, nullptr))
            {
                break;
            }
            if (fp)
            {
                std::fprintf(fp, "%g %g %g 15\n", *p0, *p1, 0.0);
            }
        }
    }
    if (fp)
    {
        std::fclose(fp);
    }
    return ret;
}

bool g_keep_screen_coords = false;
bool g_set_orbit_corners = false;
long g_orbit_interval;
double g_orbit_corner_min_x;
double g_orbit_corner_min_y;
double g_orbit_corner_max_x;
double g_orbit_corner_max_y;
double g_orbit_corner_3_x;
double g_orbit_corner_3_y;
static affine o_cvt;
static int o_color;

int setup_orbits_to_screen(affine *scrn_cnvt)
{
    double det, xd, yd;

    det = (g_orbit_corner_3_x-g_orbit_corner_min_x)*(g_orbit_corner_min_y-g_orbit_corner_max_y) + (g_orbit_corner_max_y-g_orbit_corner_3_y)*(g_orbit_corner_max_x-g_orbit_corner_min_x);
    if (det == 0)
    {
        return -1;
    }
    xd = g_logical_screen_x_size_dots/det;
    scrn_cnvt->a =  xd*(g_orbit_corner_max_y-g_orbit_corner_3_y);
    scrn_cnvt->b =  xd*(g_orbit_corner_3_x-g_orbit_corner_min_x);
    scrn_cnvt->e = -scrn_cnvt->a*g_orbit_corner_min_x - scrn_cnvt->b*g_orbit_corner_max_y;

    det = (g_orbit_corner_3_x-g_orbit_corner_max_x)*(g_orbit_corner_min_y-g_orbit_corner_max_y) + (g_orbit_corner_min_y-g_orbit_corner_3_y)*(g_orbit_corner_max_x-g_orbit_corner_min_x);
    if (det == 0)
    {
        return -1;
    }
    yd = g_logical_screen_y_size_dots/det;
    scrn_cnvt->c =  yd*(g_orbit_corner_min_y-g_orbit_corner_3_y);
    scrn_cnvt->d =  yd*(g_orbit_corner_3_x-g_orbit_corner_max_x);
    scrn_cnvt->f = -scrn_cnvt->c*g_orbit_corner_min_x - scrn_cnvt->d*g_orbit_corner_max_y;
    return 0;
}

int plotorbits2dsetup()
{
    if (g_cur_fractal_specific->isinteger != 0)
    {
        fractal_type tofloat = g_cur_fractal_specific->tofloat;
        if (tofloat == fractal_type::NOFRACTAL)
        {
            return -1;
        }
        g_float_flag = true;
        g_user_float_flag = true; // force floating point
        g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(tofloat)];
        g_fractal_type = tofloat;
    }

    per_image();

    // setup affine screen coord conversion
    if (g_keep_screen_coords)
    {
        if (setup_orbits_to_screen(&o_cvt))
        {
            return -1;
        }
    }
    else
    {
        if (setup_convert_to_screen(&o_cvt))
        {
            return -1;
        }
    }
    // set so truncation to int rounds to nearest
    o_cvt.e += 0.5;
    o_cvt.f += 0.5;

    if (g_orbit_delay >= g_max_iterations)   // make sure we get an image
    {
        g_orbit_delay = (int)(g_max_iterations - 1);
    }

    o_color = 1;

    if (g_outside_color == SUM)
    {
        g_plot = plothist;
    }
    return 1;
}

int plotorbits2dfloat()
{
    double *soundvar = nullptr;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    int col = 0;
    int row = 0;
    long count = 0;

    if (driver_key_pressed())
    {
        driver_mute();
        alloc_resume(100, 1);
        put_resume(sizeof(o_color), &o_color, 0);
        return -1;
    }

    if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)
    {
        soundvar = &x;
    }
    else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y)
    {
        soundvar = &y;
    }
    else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z)
    {
        soundvar = &z;
    }

    if (g_resuming)
    {
        start_resume();
        get_resume(sizeof(o_color), &o_color, 0);
        end_resume();
    }

    if (g_inside_color > COLOR_BLACK)
    {
        o_color = g_inside_color;
    }
    else
    {
        // inside <= 0
        o_color++;
        if (o_color >= g_colors)   // another color to switch to?
        {
            o_color = 1;    // (don't use the background color)
        }
    }

    per_pixel(); // initialize the calculations

    for (count = 0; count < g_max_iterations; count++)
    {
        if (orbit_calc() == 1 && g_periodicity_check)
        {
            continue;  // bailed out, don't plot
        }

        if (count < g_orbit_delay || count%g_orbit_interval)
        {
            continue;  // don't plot it
        }

        // else count >= orbit_delay and we want to plot it
        col = (int)(o_cvt.a*g_new_z.x + o_cvt.b*g_new_z.y + o_cvt.e);
        row = (int)(o_cvt.c*g_new_z.x + o_cvt.d*g_new_z.y + o_cvt.f);
        if (col > 0 && col < g_logical_screen_x_dots && row > 0 && row < g_logical_screen_y_dots)
        {
            // plot if on the screen
            if (soundvar && (g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)
            {
                w_snd((int)(*soundvar*100+g_base_hertz));
            }

            (*g_plot)(col, row, o_color%g_colors);
        }
        else
        {
            // off screen, don't continue unless periodicity=0
            if (g_periodicity_check)
            {
                return 0; // skip to next pixel
            }
        }
    }
    return 0;
}

// this function's only purpose is to manage funnyglasses related
// stuff so the code is not duplicated for ifs3d() and lorenz3d()
int funny_glasses_call(int (*calc)())
{
    g_which_image = g_glasses_type ? stereo_images::RED : stereo_images::NONE;
    plot_setup();
    g_plot = g_standard_plot;
    int status = calc();
    if (realtime && g_glasses_type < 3)
    {
        realtime = false;
        goto done;
    }
    if (g_glasses_type && status == 0 && g_display_3d != display_3d_modes::NONE)
    {
        if (g_glasses_type == 3)
        {
            // photographer's mode
            stopmsg(STOPMSG_INFO_ONLY,
                    "First image (left eye) is ready.  Hit any key to see it,\n"
                    "then hit <s> to save, hit any other key to create second image.");
            for (int i = driver_get_key(); i == 's' || i == 'S'; i = driver_get_key())
            {
                savetodisk(g_save_filename);
            }
            // is there a better way to clear the screen in graphics mode?
            driver_set_video_mode(&g_video_entry);
        }
        g_which_image = stereo_images::BLUE;
        if (g_cur_fractal_specific->flags & INFCALC)
        {
            g_cur_fractal_specific->per_image(); // reset for 2nd image
        }
        plot_setup();
        g_plot = g_standard_plot;
        // is there a better way to clear the graphics screen ?
        status = calc();
        if (status != 0)
        {
            goto done;
        }
        if (g_glasses_type == 3)   // photographer's mode
        {
            stopmsg(STOPMSG_INFO_ONLY, "Second image (right eye) is ready");
        }
    }
done:
    if (g_glasses_type == 4 && g_screen_x_dots >= 2*g_logical_screen_x_dots)
    {
        // turn off view windows so will save properly
        g_logical_screen_x_offset = 0;
        g_logical_screen_y_offset = 0;
        g_logical_screen_x_dots = g_screen_x_dots;
        g_logical_screen_y_dots = g_screen_y_dots;
        g_view_window = false;
    }
    return status;
}

// double version - mainly for testing
static int ifs3dfloat()
{
    int color_method;
    std::FILE *fp;
    int color;

    double newx, newy, newz, r, sum;

    int k;
    int ret;

    float3dvtinf inf;

    float *ffptr;

    // setup affine screen coord conversion
    setup_convert_to_screen(&inf.cvt);
    srand(1);
    color_method = (int)g_params[0];
    if (driver_diskp())                  // this would KILL a disk drive!
    {
        notdiskmsg();
    }

    inf.orbit[0] = 0;
    inf.orbit[1] = 0;
    inf.orbit[2] = 0;

    fp = open_orbitsave();

    ret = 0;
    if (g_max_iterations > 0x1fffffL)
    {
        g_max_count = 0x7fffffffL;
    }
    else
    {
        g_max_count = g_max_iterations*1024;
    }
    g_color_iter = 0L;
    while (g_color_iter++ <= g_max_count) // loop until keypress or maxit
    {
        if (driver_key_pressed())  // keypress bails out
        {
            ret = -1;
            break;
        }
        r = rand();      // generate a random number between 0 and 1
        r /= RAND_MAX;

        // pick which iterated function to execute, weighted by probability
        sum = g_ifs_definition[12]; // [0][12]
        k = 0;
        while (sum < r && ++k < g_num_affine_transforms*NUM_IFS_3D_PARAMS)
        {
            sum += g_ifs_definition[k*NUM_IFS_3D_PARAMS+12];
            if (g_ifs_definition[(k+1)*NUM_IFS_3D_PARAMS+12] == 0)
            {
                break; // for safety
            }
        }

        // calculate image of last point under selected iterated function
        ffptr = &g_ifs_definition[k*NUM_IFS_3D_PARAMS];     // point to first parm in row
        newx = *ffptr * inf.orbit[0] +
               *(ffptr+1) * inf.orbit[1] +
               *(ffptr+2) * inf.orbit[2] + *(ffptr+9);
        newy = *(ffptr+3) * inf.orbit[0] +
               *(ffptr+4) * inf.orbit[1] +
               *(ffptr+5) * inf.orbit[2] + *(ffptr+10);
        newz = *(ffptr+6) * inf.orbit[0] +
               *(ffptr+7) * inf.orbit[1] +
               *(ffptr+8) * inf.orbit[2] + *(ffptr+11);

        inf.orbit[0] = newx;
        inf.orbit[1] = newy;
        inf.orbit[2] = newz;
        if (fp)
        {
            std::fprintf(fp, "%g %g %g 15\n", newx, newy, newz);
        }
        if (float3dviewtransf(&inf))
        {
            // plot if inside window
            if (inf.col >= 0)
            {
                if (realtime)
                {
                    g_which_image = stereo_images::RED;
                }
                if (color_method)
                {
                    color = (k%g_colors)+1;
                }
                else
                {
                    color = getcolor(inf.col, inf.row)+1;
                }
                if (color < g_colors)     // color sticks on last value
                {
                    (*g_plot)(inf.col, inf.row, color);
                }
            }
            else if (inf.col == -2)
            {
                return ret;
            }
            if (realtime)
            {
                g_which_image = stereo_images::BLUE;
                // plot if inside window
                if (inf.col1 >= 0)
                {
                    if (color_method)
                    {
                        color = (k%g_colors)+1;
                    }
                    else
                    {
                        color = getcolor(inf.col1, inf.row1)+1;
                    }
                    if (color < g_colors)     // color sticks on last value
                    {
                        (*g_plot)(inf.col1, inf.row1, color);
                    }
                }
                else if (inf.col1 == -2)
                {
                    return ret;
                }
            }
        }
    } // end while
    if (fp)
    {
        std::fclose(fp);
    }
    return ret;
}

int ifs()                       // front-end for ifs2d and ifs3d
{
    if (g_ifs_definition.empty() && ifsload() < 0)
    {
        return -1;
    }
    if (driver_diskp())                  // this would KILL a disk drive!
    {
        notdiskmsg();
    }
    return !g_ifs_type ? ifs2d() : ifs3d();
}

static const char *const insufficient_ifs_mem{"Insufficient memory for IFS"};

// IFS logic shamelessly converted to integer math
static int ifs2d()
{
    int color_method;
    std::FILE *fp;
    int col;
    int row;
    int color;
    int ret;
    std::vector<long> localifs;
    long *lfptr;
    long x, y, newx, newy, r, sum, tempr;
    l_affine cvt;
    // setup affine screen coord conversion
    l_setup_convert_to_screen(&cvt);

    srand(1);
    color_method = (int)g_params[0];
    bool resized = false;
    try
    {
        localifs.resize(g_num_affine_transforms*NUM_IFS_PARAMS);
        resized = true;
    }
    catch (std::bad_alloc const&)
    {
    }
    if (!resized)
    {
        stopmsg(STOPMSG_NONE, insufficient_ifs_mem);
        return -1;
    }

    for (int i = 0; i < g_num_affine_transforms; i++)      // fill in the local IFS array
    {
        for (int j = 0; j < NUM_IFS_PARAMS; j++)
        {
            localifs[i*NUM_IFS_PARAMS+j] = (long)(g_ifs_definition[i*NUM_IFS_PARAMS+j] * g_fudge_factor);
        }
    }

    tempr = g_fudge_factor / 32767;        // find the proper rand() fudge

    fp = open_orbitsave();

    y = 0;
    x = y;
    ret = 0;
    if (g_max_iterations > 0x1fffffL)
    {
        g_max_count = 0x7fffffffL;
    }
    else
    {
        g_max_count = g_max_iterations*1024L;
    }
    g_color_iter = 0L;
    while (g_color_iter++ <= g_max_count) // loop until keypress or maxit
    {
        if (driver_key_pressed())  // keypress bails out
        {
            ret = -1;
            break;
        }
        r = rand15();      // generate fudged random number between 0 and 1
        r *= tempr;

        // pick which iterated function to execute, weighted by probability
        sum = localifs[6];  // [0][6]
        int k = 0;
        while (sum < r && k < g_num_affine_transforms-1)    // fixed bug of error if sum < 1
        {
            sum += localifs[++k*NUM_IFS_PARAMS+6];
        }
        // calculate image of last point under selected iterated function
        lfptr = &localifs[0] + k*NUM_IFS_PARAMS; // point to first parm in row
        newx = multiply(lfptr[0], x, g_bit_shift) +
               multiply(lfptr[1], y, g_bit_shift) + lfptr[4];
        newy = multiply(lfptr[2], x, g_bit_shift) +
               multiply(lfptr[3], y, g_bit_shift) + lfptr[5];
        x = newx;
        y = newy;
        if (fp)
        {
            std::fprintf(fp, "%g %g %g 15\n", (double)newx/g_fudge_factor, (double)newy/g_fudge_factor, 0.0);
        }

        // plot if inside window
        col = (int)((multiply(cvt.a, x, g_bit_shift) + multiply(cvt.b, y, g_bit_shift) + cvt.e) >> g_bit_shift);
        row = (int)((multiply(cvt.c, x, g_bit_shift) + multiply(cvt.d, y, g_bit_shift) + cvt.f) >> g_bit_shift);
        if (col >= 0 && col < g_logical_screen_x_dots && row >= 0 && row < g_logical_screen_y_dots)
        {
            // color is count of hits on this pixel
            if (color_method)
            {
                color = (k%g_colors)+1;
            }
            else
            {
                color = getcolor(col, row)+1;
            }
            if (color < g_colors)     // color sticks on last value
            {
                (*g_plot)(col, row, color);
            }
        }
        else if ((long)std::abs(row) + (long)std::abs(col) > BAD_PIXEL)   // sanity check
        {
            return ret;
        }
    }
    if (fp)
    {
        std::fclose(fp);
    }
    return ret;
}

static int ifs3dlong()
{
    int color_method;
    std::FILE *fp;
    int color;
    int ret;
    std::vector<long> localifs;
    long *lfptr;
    long newx, newy, newz, r, sum, tempr;
    long3dvtinf inf;

    srand(1);
    color_method = (int)g_params[0];
    try
    {
        localifs.resize(g_num_affine_transforms*NUM_IFS_3D_PARAMS);
    }
    catch (std::bad_alloc const &)
    {
        stopmsg(STOPMSG_NONE, insufficient_ifs_mem);
        return -1;
    }

    // setup affine screen coord conversion
    l_setup_convert_to_screen(&inf.cvt);

    for (int i = 0; i < g_num_affine_transforms; i++)      // fill in the local IFS array
    {
        for (int j = 0; j < NUM_IFS_3D_PARAMS; j++)
        {
            localifs[i*NUM_IFS_3D_PARAMS+j] = (long)(g_ifs_definition[i*NUM_IFS_3D_PARAMS+j] * g_fudge_factor);
        }
    }

    tempr = g_fudge_factor / 32767;        // find the proper rand() fudge

    inf.orbit[0] = 0;
    inf.orbit[1] = 0;
    inf.orbit[2] = 0;

    fp = open_orbitsave();

    ret = 0;
    if (g_max_iterations > 0x1fffffL)
    {
        g_max_count = 0x7fffffffL;
    }
    else
    {
        g_max_count = g_max_iterations*1024L;
    }
    g_color_iter = 0L;
    while (g_color_iter++ <= g_max_count) // loop until keypress or maxit
    {
        if (driver_key_pressed())  // keypress bails out
        {
            ret = -1;
            break;
        }
        r = rand15();      // generate fudged random number between 0 and 1
        r *= tempr;

        // pick which iterated function to execute, weighted by probability
        sum = localifs[12];  // [0][12]
        int k = 0;
        while (sum < r && ++k < g_num_affine_transforms*NUM_IFS_3D_PARAMS)
        {
            sum += localifs[k*NUM_IFS_3D_PARAMS+12];
            if (g_ifs_definition[(k+1)*NUM_IFS_3D_PARAMS+12] == 0)
            {
                break; // for safety
            }
        }

        // calculate image of last point under selected iterated function
        lfptr = &localifs[0] + k*NUM_IFS_3D_PARAMS; // point to first parm in row

        // calculate image of last point under selected iterated function
        newx = multiply(lfptr[0], inf.orbit[0], g_bit_shift) +
               multiply(lfptr[1], inf.orbit[1], g_bit_shift) +
               multiply(lfptr[2], inf.orbit[2], g_bit_shift) + lfptr[9];
        newy = multiply(lfptr[3], inf.orbit[0], g_bit_shift) +
               multiply(lfptr[4], inf.orbit[1], g_bit_shift) +
               multiply(lfptr[5], inf.orbit[2], g_bit_shift) + lfptr[10];
        newz = multiply(lfptr[6], inf.orbit[0], g_bit_shift) +
               multiply(lfptr[7], inf.orbit[1], g_bit_shift) +
               multiply(lfptr[8], inf.orbit[2], g_bit_shift) + lfptr[11];

        inf.orbit[0] = newx;
        inf.orbit[1] = newy;
        inf.orbit[2] = newz;
        if (fp)
        {
            std::fprintf(fp, "%g %g %g 15\n", (double)newx/g_fudge_factor, (double)newy/g_fudge_factor, (double)newz/g_fudge_factor);
        }

        if (long3dviewtransf(&inf))
        {
            if ((long)std::abs(inf.row) + (long)std::abs(inf.col) > BAD_PIXEL)   // sanity check
            {
                return ret;
            }
            // plot if inside window
            if (inf.col >= 0)
            {
                if (realtime)
                {
                    g_which_image = stereo_images::RED;
                }
                if (color_method)
                {
                    color = (k%g_colors)+1;
                }
                else
                {
                    color = getcolor(inf.col, inf.row)+1;
                }
                if (color < g_colors)     // color sticks on last value
                {
                    (*g_plot)(inf.col, inf.row, color);
                }
            }
            if (realtime)
            {
                g_which_image = stereo_images::BLUE;
                // plot if inside window
                if (inf.col1 >= 0)
                {
                    if (color_method)
                    {
                        color = (k%g_colors)+1;
                    }
                    else
                    {
                        color = getcolor(inf.col1, inf.row1)+1;
                    }
                    if (color < g_colors)     // color sticks on last value
                    {
                        (*g_plot)(inf.col1, inf.row1, color);
                    }
                }
            }
        }
    }
    if (fp)
    {
        std::fclose(fp);
    }
    return ret;
}

static void setupmatrix(MATRIX doublemat)
{
    // build transformation matrix
    identity(doublemat);

    // apply rotations - uses the same rotation variables as line3d.c
    xrot((double)XROT / 57.29577, doublemat);
    yrot((double)YROT / 57.29577, doublemat);
    zrot((double)ZROT / 57.29577, doublemat);

    // apply scale
    //   scale((double)XSCALE/100.0,(double)YSCALE/100.0,(double)ROUGH/100.0,doublemat);
}

int orbit3dfloat()
{
    g_display_3d = display_3d_modes::MINUS_ONE ;
    realtime = 0 < g_glasses_type && g_glasses_type < 3;
    return funny_glasses_call(orbit3dfloatcalc);
}

int orbit3dlong()
{
    g_display_3d = display_3d_modes::MINUS_ONE ;
    realtime = 0 < g_glasses_type && g_glasses_type < 3;
    return funny_glasses_call(orbit3dlongcalc);
}

static int ifs3d()
{
    g_display_3d = display_3d_modes::MINUS_ONE;

    realtime = 0 < g_glasses_type && g_glasses_type < 3;
    if (g_float_flag)
    {
        return funny_glasses_call(ifs3dfloat); // double version of ifs3d
    }
    else
    {
        return funny_glasses_call(ifs3dlong); // long version of ifs3d
    }
}

static bool long3dviewtransf(long3dvtinf *inf)
{
    if (g_color_iter == 1)  // initialize on first call
    {
        for (int i = 0; i < 3; i++)
        {
            inf->minvals[i] =  1L << 30;
            inf->maxvals[i] = -inf->minvals[i];
        }
        setupmatrix(inf->doublemat);
        if (realtime)
        {
            setupmatrix(inf->doublemat1);
        }
        // copy xform matrix to long for for fixed point math
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                inf->longmat[i][j] = (long)(inf->doublemat[i][j] * g_fudge_factor);
                if (realtime)
                {
                    inf->longmat1[i][j] = (long)(inf->doublemat1[i][j] * g_fudge_factor);
                }
            }
        }
    }

    // 3D VIEWING TRANSFORM
    longvmult(inf->orbit, inf->longmat, inf->viewvect, g_bit_shift);
    if (realtime)
    {
        longvmult(inf->orbit, inf->longmat1, inf->viewvect1, g_bit_shift);
    }

    if (g_color_iter <= waste) // waste this many points to find minz and maxz
    {
        // find minz and maxz
        for (int i = 0; i < 3; i++)
        {
            long const tmp = inf->viewvect[i];
            if (tmp < inf->minvals[i])
            {
                inf->minvals[i] = tmp;
            }
            else if (tmp > inf->maxvals[i])
            {
                inf->maxvals[i] = tmp;
            }
        }

        if (g_color_iter == waste) // time to work it out
        {
            inf->iview[1] = 0L;
            inf->iview[0] = inf->iview[1]; // center viewer on origin

            /* z value of user's eye - should be more negative than extreme
                           negative part of image */
            inf->iview[2] = (long)((inf->minvals[2]-inf->maxvals[2])*(double)ZVIEWER/100.0);

            // center image on origin
            double tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0*g_fudge_factor); // center x
            double tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0*g_fudge_factor); // center y

            // apply perspective shift
            tmpx += ((double)g_x_shift*(g_x_max-g_x_min))/(g_logical_screen_x_dots);
            tmpy += ((double)g_y_shift*(g_y_max-g_y_min))/(g_logical_screen_y_dots);
            double tmpz = -((double)inf->maxvals[2]) / g_fudge_factor;
            trans(tmpx, tmpy, tmpz, inf->doublemat);

            if (realtime)
            {
                // center image on origin
                tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0*g_fudge_factor); // center x
                tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0*g_fudge_factor); // center y

                tmpx += ((double)g_x_shift1*(g_x_max-g_x_min))/(g_logical_screen_x_dots);
                tmpy += ((double)g_y_shift1*(g_y_max-g_y_min))/(g_logical_screen_y_dots);
                tmpz = -((double)inf->maxvals[2]) / g_fudge_factor;
                trans(tmpx, tmpy, tmpz, inf->doublemat1);
            }
            for (int i = 0; i < 3; i++)
            {
                g_view[i] = (double)inf->iview[i] / g_fudge_factor;
            }

            // copy xform matrix to long for for fixed point math
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    inf->longmat[i][j] = (long)(inf->doublemat[i][j] * g_fudge_factor);
                    if (realtime)
                    {
                        inf->longmat1[i][j] = (long)(inf->doublemat1[i][j] * g_fudge_factor);
                    }
                }
            }
        }
        return false;
    }

    // apply perspective if requested
    if (ZVIEWER)
    {
        if (g_debug_flag == debug_flags::force_float_perspective || ZVIEWER < 100) // use float for small persp
        {
            // use float perspective calc
            VECTOR tmpv;
            for (int i = 0; i < 3; i++)
            {
                tmpv[i] = (double)inf->viewvect[i] / g_fudge_factor;
            }
            perspective(tmpv);
            for (int i = 0; i < 3; i++)
            {
                inf->viewvect[i] = (long)(tmpv[i]*g_fudge_factor);
            }
            if (realtime)
            {
                for (int i = 0; i < 3; i++)
                {
                    tmpv[i] = (double)inf->viewvect1[i] / g_fudge_factor;
                }
                perspective(tmpv);
                for (int i = 0; i < 3; i++)
                {
                    inf->viewvect1[i] = (long)(tmpv[i]*g_fudge_factor);
                }
            }
        }
        else
        {
            longpersp(inf->viewvect, inf->iview, g_bit_shift);
            if (realtime)
            {
                longpersp(inf->viewvect1, inf->iview, g_bit_shift);
            }
        }
    }

    // work out the screen positions
    inf->row = (int)(((multiply(inf->cvt.c, inf->viewvect[0], g_bit_shift) +
                       multiply(inf->cvt.d, inf->viewvect[1], g_bit_shift) + inf->cvt.f)
                      >> g_bit_shift)
                     + g_yy_adjust);
    inf->col = (int)(((multiply(inf->cvt.a, inf->viewvect[0], g_bit_shift) +
                       multiply(inf->cvt.b, inf->viewvect[1], g_bit_shift) + inf->cvt.e)
                      >> g_bit_shift)
                     + g_xx_adjust);
    if (inf->col < 0 || inf->col >= g_logical_screen_x_dots || inf->row < 0 || inf->row >= g_logical_screen_y_dots)
    {
        if ((long)std::abs(inf->col)+(long)std::abs(inf->row) > BAD_PIXEL)
        {
            inf->row = -2;
            inf->col = inf->row;
        }
        else
        {
            inf->row = -1;
            inf->col = inf->row;
        }
    }
    if (realtime)
    {
        inf->row1 = (int)(((multiply(inf->cvt.c, inf->viewvect1[0], g_bit_shift) +
                            multiply(inf->cvt.d, inf->viewvect1[1], g_bit_shift) +
                            inf->cvt.f) >> g_bit_shift)
                          + g_yy_adjust1);
        inf->col1 = (int)(((multiply(inf->cvt.a, inf->viewvect1[0], g_bit_shift) +
                            multiply(inf->cvt.b, inf->viewvect1[1], g_bit_shift) +
                            inf->cvt.e) >> g_bit_shift)
                          + g_xx_adjust1);
        if (inf->col1 < 0 || inf->col1 >= g_logical_screen_x_dots || inf->row1 < 0 || inf->row1 >= g_logical_screen_y_dots)
        {
            if ((long)std::abs(inf->col1)+(long)std::abs(inf->row1) > BAD_PIXEL)
            {
                inf->row1 = -2;
                inf->col1 = inf->row1;
            }
            else
            {
                inf->row1 = -1;
                inf->col1 = inf->row1;
            }
        }
    }
    return true;
}

static bool float3dviewtransf(float3dvtinf *inf)
{
    if (g_color_iter == 1)  // initialize on first call
    {
        for (int i = 0; i < 3; i++)
        {
            inf->minvals[i] =  100000.0; // impossible value
            inf->maxvals[i] = -100000.0;
        }
        setupmatrix(inf->doublemat);
        if (realtime)
        {
            setupmatrix(inf->doublemat1);
        }
    }

    // 3D VIEWING TRANSFORM
    vmult(inf->orbit, inf->doublemat, inf->viewvect);
    if (realtime)
    {
        vmult(inf->orbit, inf->doublemat1, inf->viewvect1);
    }

    if (g_color_iter <= waste) // waste this many points to find minz and maxz
    {
        // find minz and maxz
        for (int i = 0; i < 3; i++)
        {
            double const tmp = inf->viewvect[i];
            if (tmp < inf->minvals[i])
            {
                inf->minvals[i] = tmp;
            }
            else if (tmp > inf->maxvals[i])
            {
                inf->maxvals[i] = tmp;
            }
        }
        if (g_color_iter == waste) // time to work it out
        {
            g_view[1] = 0;
            g_view[0] = g_view[1]; // center on origin
            /* z value of user's eye - should be more negative than extreme
                              negative part of image */
            g_view[2] = (inf->minvals[2]-inf->maxvals[2])*(double)ZVIEWER/100.0;

            // center image on origin
            double tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0); // center x
            double tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0); // center y

            // apply perspective shift
            tmpx += ((double)g_x_shift*(g_x_max-g_x_min))/(g_logical_screen_x_dots);
            tmpy += ((double)g_y_shift*(g_y_max-g_y_min))/(g_logical_screen_y_dots);
            double tmpz = -(inf->maxvals[2]);
            trans(tmpx, tmpy, tmpz, inf->doublemat);

            if (realtime)
            {
                // center image on origin
                tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0); // center x
                tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0); // center y

                tmpx += ((double)g_x_shift1*(g_x_max-g_x_min))/(g_logical_screen_x_dots);
                tmpy += ((double)g_y_shift1*(g_y_max-g_y_min))/(g_logical_screen_y_dots);
                tmpz = -(inf->maxvals[2]);
                trans(tmpx, tmpy, tmpz, inf->doublemat1);
            }
        }
        return false;
    }

    // apply perspective if requested
    if (ZVIEWER)
    {
        perspective(inf->viewvect);
        if (realtime)
        {
            perspective(inf->viewvect1);
        }
    }
    inf->row = (int)(inf->cvt.c*inf->viewvect[0] + inf->cvt.d*inf->viewvect[1]
                     + inf->cvt.f + g_yy_adjust);
    inf->col = (int)(inf->cvt.a*inf->viewvect[0] + inf->cvt.b*inf->viewvect[1]
                     + inf->cvt.e + g_xx_adjust);
    if (inf->col < 0 || inf->col >= g_logical_screen_x_dots || inf->row < 0 || inf->row >= g_logical_screen_y_dots)
    {
        if ((long)std::abs(inf->col)+(long)std::abs(inf->row) > BAD_PIXEL)
        {
            inf->row = -2;
            inf->col = inf->row;
        }
        else
        {
            inf->row = -1;
            inf->col = inf->row;
        }
    }
    if (realtime)
    {
        inf->row1 = (int)(inf->cvt.c*inf->viewvect1[0] + inf->cvt.d*inf->viewvect1[1]
                          + inf->cvt.f + g_yy_adjust1);
        inf->col1 = (int)(inf->cvt.a*inf->viewvect1[0] + inf->cvt.b*inf->viewvect1[1]
                          + inf->cvt.e + g_xx_adjust1);
        if (inf->col1 < 0 || inf->col1 >= g_logical_screen_x_dots || inf->row1 < 0 || inf->row1 >= g_logical_screen_y_dots)
        {
            if ((long)std::abs(inf->col1)+(long)std::abs(inf->row1) > BAD_PIXEL)
            {
                inf->row1 = -2;
                inf->col1 = inf->row1;
            }
            else
            {
                inf->row1 = -1;
                inf->col1 = inf->row1;
            }
        }
    }
    return true;
}

static std::FILE *open_orbitsave()
{
    std::FILE *fp;
    if ((g_orbit_save_flags & osf_raw) && (fp = std::fopen("orbits.raw", "w")) != nullptr)
    {
        std::fprintf(fp, "pointlist x y z color\n");
        return fp;
    }
    return nullptr;
}

// Plot a histogram by incrementing the pixel each time it it touched
static void plothist(int x, int y, int color)
{
    color = getcolor(x, y)+1;
    if (color >= g_colors)
    {
        color = 1;
    }
    g_put_color(x, y, color);
}
