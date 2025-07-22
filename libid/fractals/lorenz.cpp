// SPDX-License-Identifier: GPL-3.0-only
//
/*
   This file contains two 3-dimensional orbit-type fractal
   generators - IFS and LORENZ3D, along with code to generate
   red/blue 3D images.
*/
#include "fractals/lorenz.h"

#include "3d/3d.h"
#include "3d/line3d.h"
#include "3d/plot3d.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/jiim.h"
#include "engine/resume.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/ifs.h"
#include "io/check_write_file.h"
#include "io/encoder.h"
#include "io/library.h"
#include "math/arg.h"
#include "math/cmplx.h"
#include "math/rand15.h"
#include "math/sign.h"
#include "misc/Driver.h"
#include "ui/not_disk_msg.h"
#include "ui/sound.h"
#include "ui/stop_msg.h"
#include "ui/video.h"

#include <fmt/format.h>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using OrbitCalc = int (*)(double *x, double *y, double *z);

/* BAD_PIXEL is used to cutoff orbits that are diverging. It might be better
to test the actual floating point orbit values, but this seems safe for now.
A higher value cannot be used - to test, turn off math coprocessor and
use +2.24 for type ICONS. If BAD_PIXEL is set to 20000, this will abort
with a math error. Note that this approach precludes zooming in very
far to an orbit type. */

// pixels can't get this big
enum
{
    BAD_PIXEL = 10000L
};

namespace
{

// data used by 3d view transform subroutine
struct ViewTransform3D
{
    double orbit[3];      // iterated function orbit value
    double view_vect[3];  // orbit transformed for viewing
    double view_vect1[3]; // orbit transformed for viewing
    double max_vals[3];   //
    double min_vals[3];   //
    Matrix double_mat;    // transformation matrix
    Matrix double_mat1;   // transformation matrix
    int row;              //
    int col;              // results
    int row1;             //
    int col1;             //
    Affine cvt;           //
};

} // namespace

static int  ifs2d();
static int  ifs3d();
static int  ifs3d_float();
static void setup_matrix(Matrix double_mat);
static bool float_view_transf3d(ViewTransform3D *inf);
static std::FILE *open_orbit_save();
static void plot_hist(int x, int y, int color);

static bool s_real_time{};
static int s_t{};

static long s_l_d{};

static double s_dx{};
static double s_dy{};
static double s_dz{};
static double s_dt{};
static double s_a{};
static double s_b{};
static double s_c{};
static double s_d{};
static double s_adt{};
static double s_bdt{};
static double s_cdt{};
static double s_xdt{};
static double s_ydt{};
static double s_zdt{};
static double s_init_orbit[3]{};

// The following declarations used for Inverse Julia.
static int      s_max_hits{};
static int      s_run_length{};
static Affine   s_cvt{};

static double s_cx{};
static double s_cy{};

/*
 * end of Inverse Julia declarations;
 */

// these are potential user parameters
static bool s_connect{true}; // flag to connect points with a line
static bool s_euler{};       // use implicit euler approximation for dynamic system
static int s_waste{100};     // waste this many points before plotting
static int s_projection{2};  // projection plane - default is to plot x-y

static Affine s_o_cvt{};
static int s_o_color{};

static double s_orbit{};

static double &s_cos_b{s_dx};
static double &s_sin_sum_a_b_c{s_dy};

static const double &LAMBDA{g_params[0]};
static const double &ALPHA{g_params[1]};
static const double &BETA{g_params[2]};
static const double &GAMMA{g_params[3]};
static const double &OMEGA{g_params[4]};
static const double &DEGREE{g_params[5]};

static const double &PAR_A{g_params[0]};
static const double &PAR_B{g_params[1]};
static const double &PAR_C{g_params[2]};
static const double &PAR_D{g_params[3]};

long g_max_count;
Major g_major_method;
Minor g_inverse_julia_minor_method;

bool g_keep_screen_coords{};
bool g_set_orbit_corners{};
long g_orbit_interval{};
double g_orbit_corner_min_x{};
double g_orbit_corner_min_y{};
double g_orbit_corner_max_x{};
double g_orbit_corner_max_y{};
double g_orbit_corner_3rd_x{};
double g_orbit_corner_3rd_y{};

// OrbitCalc is declared with no arguments so jump through hoops here
static int orbit(double *x, double *y, double *z)
{
    return (*reinterpret_cast<OrbitCalc>(g_cur_fractal_specific->orbit_calc))(x, y, z);
}

static int orbit(double *x, double *y)
{
    return orbit(x, y, nullptr);
}

static int random(int x)
{
    return std::rand() % x;
}

static void fallback_to_random_walk()
{
    stop_msg(
        StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER, "Not enough memory: switching to random walk.\n");
    g_major_method = Major::RANDOM_WALK;
}

//****************************************************************
//                 zoom box conversion functions
//****************************************************************

/*
   Conversion of complex plane to screen coordinates for rotating zoom box.
   Assume there is an affine transformation mapping complex zoom parallelogram
   to rectangular screen. We know this map must map parallelogram corners to
   screen corners, so we have the following equations:

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

bool setup_convert_to_screen(Affine *scrn_cnvt)
{
    double det = (g_x_3rd - g_x_min) * (g_y_min - g_y_max) + (g_y_max - g_y_3rd) * (g_x_max - g_x_min);
    if (det == 0)
    {
        return true;
    }
    double xd = g_logical_screen_x_size_dots / det;
    scrn_cnvt->a =  xd*(g_y_max-g_y_3rd);
    scrn_cnvt->b =  xd*(g_x_3rd-g_x_min);
    scrn_cnvt->e = -scrn_cnvt->a*g_x_min - scrn_cnvt->b*g_y_max;

    det = (g_x_3rd-g_x_max)*(g_y_min-g_y_max) + (g_y_min-g_y_3rd)*(g_x_max-g_x_min);
    if (det == 0)
    {
        return true;
    }
    double yd = g_logical_screen_y_size_dots / det;
    scrn_cnvt->c =  yd*(g_y_min-g_y_3rd);
    scrn_cnvt->d =  yd*(g_x_3rd-g_x_max);
    scrn_cnvt->f = -scrn_cnvt->c*g_x_min - scrn_cnvt->d*g_y_max;
    return false;
}

//****************************************************************
//   setup functions - put in fractalspecific[fractype].per_image
//****************************************************************

bool orbit3d_per_image()
{
    g_max_count = 0L;
    s_connect = true;
    s_waste = 100;
    s_projection = 2;

    if (g_fractal_type == FractalType::HENON          //
        || g_fractal_type == FractalType::PICKOVER    //
        || g_fractal_type == FractalType::GINGERBREAD //
        || g_fractal_type == FractalType::KAM         //
        || g_fractal_type == FractalType::KAM_3D      //
        || g_fractal_type == FractalType::HOPALONG    //
        || g_fractal_type == FractalType::INVERSE_JULIA)
    {
        s_connect = false;
    }
    if (g_fractal_type == FractalType::LORENZ_3D1    //
        || g_fractal_type == FractalType::LORENZ_3D3 //
        || g_fractal_type == FractalType::LORENZ_3D4)
    {
        s_waste = 750;
    }
    if (g_fractal_type == FractalType::ROSSLER)
    {
        s_waste = 500;
    }
    if (g_fractal_type == FractalType::LORENZ)
    {
        s_projection = 1; // plot x and z
    }

    s_init_orbit[0] = 1;  // initial conditions
    s_init_orbit[1] = 1;
    s_init_orbit[2] = 1;
    if (g_fractal_type == FractalType::GINGERBREAD)
    {
        s_init_orbit[0] = g_params[0];        // initial conditions
        s_init_orbit[1] = g_params[1];
    }

    if (g_fractal_type == FractalType::ICON || g_fractal_type == FractalType::ICON_3D)
    {
        s_init_orbit[0] = 0.01;  // initial conditions
        s_init_orbit[1] = 0.003;
        s_connect = false;
        s_waste = 2000;
    }

    if (g_fractal_type == FractalType::LATOO)
    {
        s_connect = false;
    }

    if (g_fractal_type == FractalType::HENON || g_fractal_type == FractalType::PICKOVER)
    {
        s_a =  g_params[0];
        s_b =  g_params[1];
        s_c =  g_params[2];
        s_d =  g_params[3];
    }
    else if (g_fractal_type == FractalType::ICON || g_fractal_type == FractalType::ICON_3D)
    {
        s_init_orbit[0] = 0.01;  // initial conditions
        s_init_orbit[1] = 0.003;
        s_connect = false;
        s_waste = 2000;
        // Initialize parameters
        s_a  =   g_params[0];
        s_b  =   g_params[1];
        s_c  =   g_params[2];
        s_d  =   g_params[3];
    }
    else if (g_fractal_type == FractalType::KAM || g_fractal_type == FractalType::KAM_3D)
    {
        g_max_count = 1L;
        s_a = g_params[0];           // angle
        if (g_params[1] <= 0.0)
        {
            g_params[1] = .01;
        }
        s_b = g_params[1];          // stepsize
        s_c = g_params[2];          // stop
        s_l_d = (long) g_params[3]; //
        s_t = (int) g_params[3];    // points per orbit
        g_sin_x = std::sin(s_a);
        g_cos_x = std::cos(s_a);
        s_orbit = 0;
        s_init_orbit[0] = 0;
        s_init_orbit[1] = 0;
        s_init_orbit[2] = 0;
    }
    else if (g_fractal_type == FractalType::HOPALONG  //
        || g_fractal_type == FractalType::MARTIN      //
        || g_fractal_type == FractalType::CHIP        //
        || g_fractal_type == FractalType::QUADRUP_TWO //
        || g_fractal_type == FractalType::THREEPLY)
    {
        s_init_orbit[0] = 0;  // initial conditions
        s_init_orbit[1] = 0;
        s_init_orbit[2] = 0;
        s_connect = false;
        s_a =  g_params[0];
        s_b =  g_params[1];
        s_c =  g_params[2];
        s_d =  g_params[3];
        if (g_fractal_type == FractalType::THREEPLY)
        {
            s_cos_b = std::cos(s_b);
            s_sin_sum_a_b_c = std::sin(s_a + s_b + s_c);
        }
    }
    else if (g_fractal_type == FractalType::INVERSE_JULIA)
    {
        s_cx = g_params[0];
        s_cy = g_params[1];

        s_max_hits    = (int) g_params[2];
        s_run_length = (int) g_params[3];
        if (s_max_hits <= 0)
        {
            s_max_hits = 1;
        }
        else if (s_max_hits >= g_colors)
        {
            s_max_hits = g_colors - 1;
        }
        g_params[2] = s_max_hits;

        setup_convert_to_screen(&s_cvt);

        // find fixed points: guaranteed to be in the set
        DComplex sqrt = complex_sqrt_float(1 - 4 * s_cx, -4 * s_cy);
        switch (g_major_method)
        {
        case Major::BREADTH_FIRST:
            if (!init_queue(32*1024UL))
            {
                // can't get queue memory: fall back to random walk
                fallback_to_random_walk();
                goto random_walk;
            }
            enqueue_float((float)((1 + sqrt.x) / 2), (float)(sqrt.y / 2));
            enqueue_float((float)((1 - sqrt.x) / 2), (float)(-sqrt.y / 2));
            break;
        case Major::DEPTH_FIRST:                      // depth first (choose direction)
            if (!init_queue(32*1024UL))
            {
                // can't get queue memory: fall back to random walk
                fallback_to_random_walk();
                goto random_walk;
            }
            switch (g_inverse_julia_minor_method)
            {
            case Minor::LEFT_FIRST:
                push_float((float)((1 + sqrt.x) / 2), (float)(sqrt.y / 2));
                push_float((float)((1 - sqrt.x) / 2), (float)(-sqrt.y / 2));
                break;
            case Minor::RIGHT_FIRST:
                push_float((float)((1 - sqrt.x) / 2), (float)(-sqrt.y / 2));
                push_float((float)((1 + sqrt.x) / 2), (float)(sqrt.y / 2));
                break;
            }
            break;
        case Major::RANDOM_WALK:
random_walk:
            s_init_orbit[0] = 1 + sqrt.x / 2;
            g_new_z.x = s_init_orbit[0];
            s_init_orbit[1] = sqrt.y / 2;
            g_new_z.y = s_init_orbit[1];
            break;
        case Major::RANDOM_RUN:       // random run, choose intervals
            g_major_method = Major::RANDOM_RUN;
            s_init_orbit[0] = 1 + sqrt.x / 2;
            g_new_z.x = s_init_orbit[0];
            s_init_orbit[1] = sqrt.y / 2;
            g_new_z.y = s_init_orbit[1];
            break;
        }
    }
    else
    {
        s_dt = g_params[0];
        s_a =  g_params[1];
        s_b =  g_params[2];
        s_c =  g_params[3];
    }

    // precalculations for speed
    s_adt = s_a*s_dt;
    s_bdt = s_b*s_dt;
    s_cdt = s_c*s_dt;

    return true;
}

//****************************************************************
//   orbit functions - put in fractalspecific[fractype].orbitcalc
//****************************************************************

int inverse_julia_orbit()
{
    static int random_dir = 0;
    static int random_len = 0;

    /*
     * First, compute new point
     */
    switch (g_major_method)
    {
    case Major::BREADTH_FIRST:
        if (queue_empty())
        {
            return -1;
        }
        g_new_z = dequeue_float();
        break;
    case Major::DEPTH_FIRST:
        if (queue_empty())
        {
            return -1;
        }
        g_new_z = pop_float();
        break;
    case Major::RANDOM_WALK:
        break;
    case Major::RANDOM_RUN:
        break;
    }

    /*
     * Next, find its pixel position
     */
    int new_col = (int) (s_cvt.a * g_new_z.x + s_cvt.b * g_new_z.y + s_cvt.e);
    int new_row = (int) (s_cvt.c * g_new_z.x + s_cvt.d * g_new_z.y + s_cvt.f);

    /*
     * Now find the next point(s), and flip a coin to choose one.
     */

    g_new_z       = complex_sqrt_float(g_new_z.x - s_cx, g_new_z.y - s_cy);
    int left_right = (random(2)) ? 1 : -1;

    if (new_col < 1 || new_col >= g_logical_screen_x_dots || new_row < 1 || new_row >= g_logical_screen_y_dots)
    {
        /*
         * MIIM must skip points that are off the screen boundary,
         * since it cannot read their color.
         */
        switch (g_major_method)
        {
        case Major::BREADTH_FIRST:
            enqueue_float((float)(left_right * g_new_z.x), (float)(left_right * g_new_z.y));
            return 1;
        case Major::DEPTH_FIRST:
            push_float((float)(left_right * g_new_z.x), (float)(left_right * g_new_z.y));
            return 1;
        case Major::RANDOM_RUN:
        case Major::RANDOM_WALK:
            break;
        }
    }

    /*
     * Read the pixel's color:
     * For MIIM, if color >= mxhits, discard the point
     *           else put the point's children onto the queue
     */
    int color = get_color(new_col, new_row);
    switch (g_major_method)
    {
    case Major::BREADTH_FIRST:
        if (color < s_max_hits)
        {
            g_put_color(new_col, new_row, color+1);
            enqueue_float((float)g_new_z.x, (float)g_new_z.y);
            enqueue_float((float)-g_new_z.x, (float)-g_new_z.y);
        }
        break;
    case Major::DEPTH_FIRST:
        if (color < s_max_hits)
        {
            g_put_color(new_col, new_row, color+1);
            if (g_inverse_julia_minor_method == Minor::LEFT_FIRST)
            {
                if (queue_full_almost())
                {
                    push_float((float)-g_new_z.x, (float)-g_new_z.y);
                }
                else
                {
                    push_float((float)g_new_z.x, (float)g_new_z.y);
                    push_float((float)-g_new_z.x, (float)-g_new_z.y);
                }
            }
            else
            {
                if (queue_full_almost())
                {
                    push_float((float)g_new_z.x, (float)g_new_z.y);
                }
                else
                {
                    push_float((float)-g_new_z.x, (float)-g_new_z.y);
                    push_float((float)g_new_z.x, (float)g_new_z.y);
                }
            }
        }
        break;
    case Major::RANDOM_RUN:
        if (random_len-- == 0)
        {
            random_len = random(s_run_length);
            random_dir = random(3);
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
            g_new_z.x = left_right * g_new_z.x;
            g_new_z.y = left_right * g_new_z.y;
            break;
        }
        if (color < g_colors-1)
        {
            g_put_color(new_col, new_row, color+1);
        }
        break;
    case Major::RANDOM_WALK:
        if (color < g_colors-1)
        {
            g_put_color(new_col, new_row, color+1);
        }
        g_new_z.x = left_right * g_new_z.x;
        g_new_z.y = left_right * g_new_z.y;
        break;
    }
    return 1;
}

int lorenz3d1_orbit(double *x, double *y, double *z)
{
    s_xdt = (*x)*s_dt;
    s_ydt = (*y)*s_dt;
    s_zdt = (*z)*s_dt;

    // 1-lobe Lorenz
    double norm = std::sqrt((*x) * (*x) + (*y) * (*y));
    s_dx   = (-s_adt-s_dt)*(*x) + (s_adt-s_bdt)*(*y) + (s_dt-s_adt)*norm + s_ydt*(*z);
    s_dy   = (s_bdt-s_adt)*(*x) - (s_adt+s_dt)*(*y) + (s_bdt+s_adt)*norm - s_xdt*(*z) -
           norm*s_zdt;
    s_dz   = (s_ydt/2) - s_cdt*(*z);

    *x += s_dx;
    *y += s_dy;
    *z += s_dz;
    return 0;
}

int lorenz3d_orbit(double *x, double *y, double *z)
{
    s_xdt = (*x)*s_dt;
    s_ydt = (*y)*s_dt;
    s_zdt = (*z)*s_dt;

    // 2-lobe Lorenz (the original)
    s_dx  = -s_adt*(*x) + s_adt*(*y);
    s_dy  =  s_bdt*(*x) - s_ydt - (*z)*s_xdt;
    s_dz  = -s_cdt*(*z) + (*x)*s_ydt;

    *x += s_dx;
    *y += s_dy;
    *z += s_dz;
    return 0;
}

int lorenz3d3_orbit(double *x, double *y, double *z)
{
    s_xdt = (*x)*s_dt;
    s_ydt = (*y)*s_dt;
    s_zdt = (*z)*s_dt;

    // 3-lobe Lorenz
    double norm = std::sqrt((*x) * (*x) + (*y) * (*y));
    s_dx   = (-(s_adt+s_dt)*(*x) + (s_adt-s_bdt+s_zdt)*(*y)) / 3 +
           ((s_dt-s_adt)*((*x)*(*x)-(*y)*(*y)) +
            2*(s_bdt+s_adt-s_zdt)*(*x)*(*y))/(3*norm);
    s_dy   = ((s_bdt-s_adt-s_zdt)*(*x) - (s_adt+s_dt)*(*y)) / 3 +
           (2*(s_adt-s_dt)*(*x)*(*y) +
            (s_bdt+s_adt-s_zdt)*((*x)*(*x)-(*y)*(*y)))/(3*norm);
    s_dz   = (3*s_xdt*(*x)*(*y)-s_ydt*(*y)*(*y))/2 - s_cdt*(*z);

    *x += s_dx;
    *y += s_dy;
    *z += s_dz;
    return 0;
}

int lorenz3d4_orbit(double *x, double *y, double *z)
{
    s_xdt = (*x)*s_dt;
    s_ydt = (*y)*s_dt;
    s_zdt = (*z)*s_dt;

    // 4-lobe Lorenz
    s_dx   = (-s_adt*(*x)*(*x)*(*x) + (2*s_adt+s_bdt-s_zdt)*(*x)*(*x)*(*y) +
            (s_adt-2*s_dt)*(*x)*(*y)*(*y) + (s_zdt-s_bdt)*(*y)*(*y)*(*y)) /
           (2 * ((*x)*(*x)+(*y)*(*y)));
    s_dy   = ((s_bdt-s_zdt)*(*x)*(*x)*(*x) + (s_adt-2*s_dt)*(*x)*(*x)*(*y) +
            (-2*s_adt-s_bdt+s_zdt)*(*x)*(*y)*(*y) - s_adt*(*y)*(*y)*(*y)) /
           (2 * ((*x)*(*x)+(*y)*(*y)));
    s_dz   = (2*s_xdt*(*x)*(*x)*(*y) - 2*s_xdt*(*y)*(*y)*(*y) - s_cdt*(*z));

    *x += s_dx;
    *y += s_dy;
    *z += s_dz;
    return 0;
}

int henon_orbit(double *x, double *y, double * /*z*/)
{
    double new_x = 1 + *y - s_a * (*x) * (*x);
    double new_y = s_b * (*x);
    *x = new_x;
    *y = new_y;
    return 0;
}

int rossler_orbit(double *x, double *y, double *z)
{
    s_xdt = (*x)*s_dt;
    s_ydt = (*y)*s_dt;

    s_dx = -s_ydt - (*z)*s_dt;
    s_dy = s_xdt + (*y)*s_adt;
    s_dz = s_bdt + (*z)*s_xdt - (*z)*s_cdt;

    *x += s_dx;
    *y += s_dy;
    *z += s_dz;
    return 0;
}

int pickover_orbit(double *x, double *y, double *z)
{
    double new_x = std::sin(s_a * (*y)) - (*z) * std::cos(s_b * (*x));
    double new_y = (*z) * std::sin(s_c * (*x)) - std::cos(s_d * (*y));
    double new_z = std::sin(*x);
    *x = new_x;
    *y = new_y;
    *z = new_z;
    return 0;
}

// page 149 "Science of Fractal Images"
int ginger_bread_orbit(double *x, double *y, double * /*z*/)
{
    double new_x = 1 - (*y) + std::abs(*x);
    *y = *x;
    *x = new_x;
    return 0;
}

// OSTEP  = Orbit Step (and inner orbit value)
// NTURNS = Outside Orbit
// TURN2  = Points per orbit
// a      = Angle
int kam_torus_orbit(double *r, double *s, double *z)
{
    if (s_t++ >= s_l_d)
    {
        s_orbit += s_b;
        (*s) = s_orbit/3;
        (*r) = (*s);
        s_t = 0;
        *z = s_orbit;
        if (s_orbit > s_c)
        {
            return 1;
        }
    }
    double srr = (*s) - (*r) * (*r);
    (*s) = (*r)*g_sin_x+srr*g_cos_x;
    (*r) = (*r)*g_cos_x-srr*g_sin_x;
    return 0;
}

int hopalong2d_orbit(double *x, double *y, double * /*z*/)
{
    double tmp = *y - sign(*x) * std::sqrt(std::abs(s_b * (*x) - s_c));
    *y = s_a - *x;
    *x = tmp;
    return 0;
}

int chip2d_orbit(double *x, double *y, double * /*z*/)
{
    double tmp = *y -
        sign(*x) * std::cos(sqr(std::log(std::abs(s_b * (*x) - s_c)))) *
            std::atan(sqr(std::log(std::abs(s_c * (*x) - s_b))));
    *y = s_a - *x;
    *x = tmp;
    return 0;
}

int quadrup_two2d_orbit(double *x, double *y, double * /*z*/)
{
    double tmp = *y -
        sign(*x) * std::sin(std::log(std::abs(s_b * (*x) - s_c))) *
            std::atan(sqr(std::log(std::abs(s_c * (*x) - s_b))));
    *y = s_a - *x;
    *x = tmp;
    return 0;
}

int three_ply2d_orbit(double *x, double *y, double * /*z*/)
{
    double tmp = *y - sign(*x) * (std::abs(std::sin(*x) * s_cos_b + s_c - (*x) * s_sin_sum_a_b_c));
    *y = s_a - *x;
    *x = tmp;
    return 0;
}

int martin2d_orbit(double *x, double *y, double * /*z*/)
{
    double tmp = *y - std::sin(*x);
    *y = s_a - *x;
    *x = tmp;
    return 0;
}

int mandel_cloud_orbit(double *x, double *y, double * /*z*/)
{
    double x2 = (*x) * (*x);
    double y2 = (*y) * (*y);
    if (x2+y2 > 2)
    {
        return 1;
    }
    double new_x = x2 - y2 + s_a;
    double new_y = 2 * (*x) * (*y) + s_b;
    *x = new_x;
    *y = new_y;
    return 0;
}

int dynamic_orbit(double *x, double *y, double * /*z*/)
{
    DComplex cp;
    DComplex tmp;
    cp.x = s_b* *x;
    cp.y = 0;
    cmplx_trig0(cp, tmp);
    double new_y = *y + s_dt * std::sin(*x + s_a * tmp.x);
    if (s_euler)
    {
        *y = new_y;
    }

    cp.x = s_b* *y;
    cp.y = 0;
    cmplx_trig0(cp, tmp);
    double new_x = *x - s_dt * std::sin(*y + s_a * tmp.x);
    *x = new_x;
    *y = new_y;
    return 0;
}

int icon_orbit(double *x, double *y, double *z)
{
    double old_x = *x;
    double old_y = *y;

    double z_z_bar = old_x * old_x + old_y * old_y;
    double z_real = old_x;
    double z_imag = old_y;

    for (int i = 1; i <= DEGREE-2; i++)
    {
        double za = z_real * old_x - z_imag * old_y;
        double zb = z_imag * old_x + z_real * old_y;
        z_real = za;
        z_imag = zb;
    }
    double zn = old_x * z_real - old_y * z_imag;
    double p = LAMBDA + ALPHA * z_z_bar + BETA * zn;
    *x = p * old_x + GAMMA * z_real - OMEGA * old_y;
    *y = p * old_y - GAMMA * z_imag + OMEGA * old_x;

    *z = z_z_bar;
    return 0;
}

int latoo_orbit(double *x, double *y, double * /*z*/)
{
    double x_old = *x;
    double y_old = *y;

    //    *x = sin(yold * PAR_B) + PAR_C * sin(xold * PAR_B);
    g_old_z.x = y_old * PAR_B;
    g_old_z.y = 0;          // old = (y * B) + 0i (in the complex)
    cmplx_trig0(g_old_z, g_new_z);
    double tmp = g_new_z.x;
    g_old_z.x = x_old * PAR_B;
    g_old_z.y = 0;          // old = (x * B) + 0i
    cmplx_trig1(g_old_z, g_new_z);
    *x  = PAR_C * g_new_z.x + tmp;

    //    *y = sin(xold * PAR_A) + PAR_D * sin(yold * PAR_A);
    g_old_z.x = x_old * PAR_A;
    g_old_z.y = 0;          // old = (y * A) + 0i (in the complex)
    cmplx_trig2(g_old_z, g_new_z);
    tmp = g_new_z.x;
    g_old_z.x = y_old * PAR_A;
    g_old_z.y = 0;          // old = (x * B) + 0i
    cmplx_trig3(g_old_z, g_new_z);
    *y  = PAR_D * g_new_z.x + tmp;

    return 0;
}

//********************************************************************
//   Main fractal engines - put in fractalspecific[fractype].calctype
//********************************************************************

namespace id::fractals
{

Orbit2D::Orbit2D() :
    m_fp(open_orbit_save()),
    m_x(s_init_orbit[0]),
    m_y(s_init_orbit[1]),
    m_z(s_init_orbit[2])
{
    setup_convert_to_screen(&m_cvt); // setup affine screen coord conversion

    // set up projection scheme
    switch (s_projection)
    {
    case 0:
        m_p0 = &m_z;
        m_p1 = &m_x;
        m_p2 = &m_y;
        break;
    case 1:
        m_p0 = &m_x;
        m_p1 = &m_z;
        m_p2 = &m_y;
        break;
    case 2:
        m_p0 = &m_x;
        m_p1 = &m_y;
        m_p2 = &m_z;
        break;
    }

    switch (g_sound_flag & SOUNDFLAG_ORBIT_MASK)
    {
    case SOUNDFLAG_X:
        m_sound_var = &m_x;
        break;
    case SOUNDFLAG_Y:
        m_sound_var = &m_y;
        break;
    case SOUNDFLAG_Z:
        m_sound_var = &m_z;
        break;
    }

    if (g_inside_color > COLOR_BLACK)
    {
        m_color = g_inside_color;
    }
    else
    {
        m_color = 2;
    }

    g_color_iter = 0L;
    if (g_max_iterations > 0x1fffffL || g_max_count)
    {
        g_max_count = 0x7fffffffL;
    }
    else
    {
        g_max_count = g_max_iterations*1024L;
    }
}

Orbit2D::~Orbit2D()
{
    if (m_fp != nullptr)
    {
        std::fclose(m_fp);
        m_fp = nullptr;
    }
}

void Orbit2D::resume()
{
    start_resume();
    get_resume(m_count, m_color, m_old_row, m_old_col, m_x, m_y, m_z, s_t, s_orbit, g_color_iter);
    end_resume();
}

void Orbit2D::suspend()
{
    driver_mute();
    alloc_resume(100, 1);
    put_resume(m_count, m_color, m_old_row, m_old_col, m_x, m_y, m_z, s_t, s_orbit, g_color_iter);
}

bool Orbit2D::done() const
{
    // loop until orbit bails out, orbit unbounded or max iteration exceeded
    if (m_bailout || m_unbounded || g_color_iter++ > g_max_count)
    {
        driver_mute();
        return true;
    }
    return false;
}

void Orbit2D::iterate()
{
    if (++m_count > 1000)
    {
        // time to switch colors?
        m_count = 0;
        if (++m_color >= g_colors)   // another color to switch to?
        {
            m_color = 1;  // (don't use the background color)
        }
    }

    const int col = (int)(m_cvt.a * m_x + m_cvt.b * m_y + m_cvt.e);
    const int row = (int)(m_cvt.c * m_x + m_cvt.d * m_y + m_cvt.f);
    if (col >= 0 && col < g_logical_screen_x_dots && row >= 0 && row < g_logical_screen_y_dots)
    {
        if (m_sound_var && (g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP)
        {
            write_sound((int)(*m_sound_var*100 + g_base_hertz));
        }
        if ((g_fractal_type != FractalType::ICON) && (g_fractal_type != FractalType::LATOO))
        {
            if (m_old_col != -1 && s_connect)
            {
                driver_draw_line(col, row, m_old_col, m_old_row, m_color % g_colors);
            }
            else
            {
                g_plot(col, row, m_color % g_colors);
            }
        }
        else
        {
            // should this be using plothist()?
            m_color = get_color(col, row)+1;
            if (m_color < g_colors) // color sticks on last value
            {
                g_plot(col, row, m_color);
            }
        }

        m_old_col = col;
        m_old_row = row;
    }
    else if ((long) std::abs(row) + (long) std::abs(col) > BAD_PIXEL) // sanity check
    {
        m_unbounded = true;
        return;
    }
    else
    {
        m_old_col = -1;
        m_old_row = -1;
    }

    if (orbit(m_p0, m_p1, m_p2))
    {
        m_bailout = true;
        return;
    }
    if (m_fp)
    {
        fmt::print(m_fp, "{:g} {:g} {:g} 15\n", *m_p0, *m_p1, 0.0);
    }
}

} // namespace id::fractals

int orbit2d_type()
{
    id::fractals::Orbit2D o2d;

    if (g_resuming)
    {
        o2d.resume();
    }

    while (!o2d.done())
    {
        if (driver_key_pressed())
        {
            o2d.suspend();
            return -1;
        }

        o2d.iterate();
    }
    return 0;
}

static int orbit3d_float_calc()
{
    int color = 2;
    if (color >= g_colors)
    {
        color = 1;
    }

    ViewTransform3D inf;
    setup_convert_to_screen(&inf.cvt); // setup affine screen coord conversion
    inf.orbit[0] = s_init_orbit[0];
    inf.orbit[1] = s_init_orbit[1];
    inf.orbit[2] = s_init_orbit[2];

    if (driver_is_disk())                  // this would KILL a disk drive!
    {
        not_disk_msg();
    }

    if (g_max_iterations > 0x1fffffL || g_max_count)
    {
        g_max_count = 0x7fffffffL;
    }
    else
    {
        g_max_count = g_max_iterations*1024L;
    }
    g_color_iter = 0L;

    std::FILE *fp = open_orbit_save();
    int ret = 0;
    unsigned long count = 0;
    int old_row = -1;
    int old_col = -1;
    int old_row1 = -1;
    int old_col1 = -1;
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

        orbit(&inf.orbit[0], &inf.orbit[1], &inf.orbit[2]);
        if (fp)
        {
            fmt::print(fp, "{:g} {:g} {:g} 15\n", inf.orbit[0], inf.orbit[1], inf.orbit[2]);
        }
        if (float_view_transf3d(&inf))
        {
            // plot if inside window
            if (inf.col >= 0)
            {
                if (s_real_time)
                {
                    g_which_image = StereoImage::RED;
                }
                if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP)
                {
                    write_sound((int)(inf.view_vect[((g_sound_flag & SOUNDFLAG_ORBIT_MASK) - SOUNDFLAG_X)]*100+g_base_hertz));
                }
                if (old_col != -1 && s_connect)
                {
                    driver_draw_line(inf.col, inf.row, old_col, old_row, color%g_colors);
                }
                else
                {
                    g_plot(inf.col, inf.row, color%g_colors);
                }
            }
            else if (inf.col == -2)
            {
                return ret;
            }
            old_col = inf.col;
            old_row = inf.row;
            if (s_real_time)
            {
                g_which_image = StereoImage::BLUE;
                // plot if inside window
                if (inf.col1 >= 0)
                {
                    if (old_col1 != -1 && s_connect)
                    {
                        driver_draw_line(inf.col1, inf.row1, old_col1, old_row1, color%g_colors);
                    }
                    else
                    {
                        g_plot(inf.col1, inf.row1, color%g_colors);
                    }
                }
                else if (inf.col1 == -2)
                {
                    return ret;
                }
                old_col1 = inf.col1;
                old_row1 = inf.row1;
            }
        }
    }

    if (fp)
    {
        std::fclose(fp);
    }

    return ret;
}

bool dynamic2d_per_image()
{
    s_connect = false;
    s_euler = false;
    s_d = g_params[0]; // number of intervals
    if (s_d < 0)
    {
        s_d = -s_d;
        s_connect = true;
    }
    else if (s_d == 0)
    {
        s_d = 1;
    }
    if (g_fractal_type == FractalType::DYNAMIC)
    {
        s_a = g_params[2];  // parameter
        s_b = g_params[3];  // parameter
        s_dt = g_params[1]; // step size
        if (s_dt < 0)
        {
            s_dt = -s_dt;
            s_euler = true;
        }
        if (s_dt == 0)
        {
            s_dt = 0.01;
        }
    }
    if (g_outside_color == SUM)
    {
        g_plot = plot_hist;
    }
    return true;
}

namespace id::fractals
{

Dynamic2D::Dynamic2D() :
    m_fp(open_orbit_save())
{
    // setup affine screen coord conversion
    setup_convert_to_screen(&m_cvt);

    if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_X)
    {
        m_sound_var = &m_x;
    }
    else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Y)
    {
        m_sound_var = &m_y;
    }
    else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Z)
    {
        m_sound_var = &m_z;
    }

    if (g_inside_color > COLOR_BLACK)
    {
        m_color = g_inside_color;
    }
    if (m_color >= g_colors)
    {
        m_color = 1;
    }
}

Dynamic2D::~Dynamic2D()
{
    if (m_fp != nullptr)
    {
        std::fclose(m_fp);
    }
}

void Dynamic2D::resume()
{
    start_resume();
    get_resume(m_count, m_color, m_old_row, m_old_col, m_x, m_y, m_x_step, m_y_step);
    end_resume();
}

void Dynamic2D::suspend()
{
    driver_mute();
    alloc_resume(100, 1);
    put_resume(m_count, m_color, m_old_row, m_old_col, m_x, m_y, m_x_step, m_y_step);
}

bool Dynamic2D::done() const
{
    if (m_unbounded || m_y_step > s_d)
    {
        driver_mute();
        return true;
    }
    return false;
}

void Dynamic2D::iterate()
{
    if (m_count == -1)
    {
        ++m_x_step;
        if (m_x_step >= s_d)
        {
            m_x_step = 0;
            ++m_y_step;
            if (m_y_step > s_d)
            {
                return;
            }
        }

        // Our pixel position on the screen
        m_x_pixel = g_logical_screen_x_size_dots * (m_x_step + .5) / s_d;
        m_y_pixel = g_logical_screen_y_size_dots * (m_y_step + .5) / s_d;
        m_x = (double) ((g_x_min + g_delta_x * m_x_pixel) + (g_delta_x2 * m_y_pixel));
        m_y = (double) ((g_y_max - g_delta_y * m_y_pixel) + (-g_delta_y2 * m_x_pixel));
        if (g_fractal_type == FractalType::MANDEL_CLOUD)
        {
            s_a = m_x;
            s_b = m_y;
        }
        m_old_col = -1;

        if (++m_color >= g_colors) // another color to switch to?
        {
            m_color = 1;           // (don't use the background color)
        }

        m_count = 0;
        m_keep_going = false;
    }

    for (; m_count < g_max_iterations; m_count++)
    {
        if (!m_keep_going && m_count % 2048L == 0)
        {
            m_keep_going = true;
            return;
        }
        m_keep_going = false;

        const int col = (int)(m_cvt.a * m_x + m_cvt.b * m_y + m_cvt.e);
        const int row = (int)(m_cvt.c * m_x + m_cvt.d * m_y + m_cvt.f);
        if (col >= 0 && col < g_logical_screen_x_dots && row >= 0 && row < g_logical_screen_y_dots)
        {
            if (m_sound_var && (g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP)
            {
                write_sound((int)(*m_sound_var*100+g_base_hertz));
            }

            if (m_count >= g_orbit_delay)
            {
                if (m_old_col != -1 && s_connect)
                {
                    driver_draw_line(col, row, m_old_col, m_old_row, m_color%g_colors);
                }
                else if (m_count > 0 || g_fractal_type != FractalType::MANDEL_CLOUD)
                {
                    g_plot(col, row, m_color%g_colors);
                }
            }
            m_old_col = col;
            m_old_row = row;
        }
        else if ((long)std::abs(row) + (long)std::abs(col) > BAD_PIXEL)   // sanity check
        {
            m_unbounded = true;
            return;
        }
        else
        {
            m_old_col = -1;
            m_old_row = -1;
        }

        if (orbit(m_p0, m_p1))
        {
            break;
        }
        if (m_fp)
        {
            fmt::print(m_fp, "{:g} {:g} {:g} 15\n", *m_p0, *m_p1, 0.0);
        }
    }
    m_count = -1;
}

} // namespace id::fractals

static int setup_orbits_to_screen(Affine *scrn_cnvt)
{
    double det = //
        (g_orbit_corner_3rd_x - g_orbit_corner_min_x) * (g_orbit_corner_min_y - g_orbit_corner_max_y) +
        (g_orbit_corner_max_y - g_orbit_corner_3rd_y) * (g_orbit_corner_max_x - g_orbit_corner_min_x);
    if (det == 0)
    {
        return -1;
    }
    const double xd = g_logical_screen_x_size_dots / det;
    scrn_cnvt->a =  xd*(g_orbit_corner_max_y-g_orbit_corner_3rd_y);
    scrn_cnvt->b =  xd*(g_orbit_corner_3rd_x-g_orbit_corner_min_x);
    scrn_cnvt->e = -scrn_cnvt->a*g_orbit_corner_min_x - scrn_cnvt->b*g_orbit_corner_max_y;

    det = //
        (g_orbit_corner_3rd_x - g_orbit_corner_max_x) * (g_orbit_corner_min_y - g_orbit_corner_max_y) +
        (g_orbit_corner_min_y - g_orbit_corner_3rd_y) * (g_orbit_corner_max_x - g_orbit_corner_min_x);
    if (det == 0)
    {
        return -1;
    }
    const double yd = g_logical_screen_y_size_dots / det;
    scrn_cnvt->c =  yd*(g_orbit_corner_min_y-g_orbit_corner_3rd_y);
    scrn_cnvt->d =  yd*(g_orbit_corner_3rd_x-g_orbit_corner_max_x);
    scrn_cnvt->f = -scrn_cnvt->c*g_orbit_corner_min_x - scrn_cnvt->d*g_orbit_corner_max_y;

    return 0;
}

int plot_orbits2d_setup()
{
    per_image();

    // setup affine screen coord conversion
    if (g_keep_screen_coords)
    {
        if (setup_orbits_to_screen(&s_o_cvt))
        {
            return -1;
        }
    }
    else
    {
        if (setup_convert_to_screen(&s_o_cvt))
        {
            return -1;
        }
    }
    // set so truncation to int rounds to nearest
    s_o_cvt.e += 0.5;
    s_o_cvt.f += 0.5;

    if (g_orbit_delay >= g_max_iterations)   // make sure we get an image
    {
        g_orbit_delay = (int)(g_max_iterations - 1);
    }

    s_o_color = 1;

    if (g_outside_color == SUM)
    {
        g_plot = plot_hist;
    }

    return 1;
}

int plot_orbits2d_float()
{
    if (driver_key_pressed())
    {
        driver_mute();
        alloc_resume(100, 1);
        put_resume(s_o_color);
        return -1;
    }

    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    const double *sound_var = nullptr;
    if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_X)
    {
        sound_var = &x;
    }
    else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Y)
    {
        sound_var = &y;
    }
    else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Z)
    {
        sound_var = &z;
    }

    if (g_resuming)
    {
        start_resume();
        get_resume(s_o_color);
        end_resume();
    }

    if (g_inside_color > COLOR_BLACK)
    {
        s_o_color = g_inside_color;
    }
    else
    {
        // inside <= 0
        s_o_color++;
        if (s_o_color >= g_colors)   // another color to switch to?
        {
            s_o_color = 1;    // (don't use the background color)
        }
    }

    per_pixel(); // initialize the calculations

    for (long count = 0; count < g_max_iterations; count++)
    {
        if (orbit_calc() == 1 && g_periodicity_check)
        {
            continue;  // bailed out, don't plot
        }

        if (count < g_orbit_delay || count%g_orbit_interval)
        {
            continue;  // don't plot it
        }

        // else count >= orbit_delay, and we want to plot it
        const int col = (int)(s_o_cvt.a * g_new_z.x + s_o_cvt.b * g_new_z.y + s_o_cvt.e);
        const int row = (int)(s_o_cvt.c * g_new_z.x + s_o_cvt.d * g_new_z.y + s_o_cvt.f);
        if (col > 0 && col < g_logical_screen_x_dots && row > 0 && row < g_logical_screen_y_dots)
        {
            // plot if on the screen
            if (sound_var && (g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP)
            {
                write_sound((int)(*sound_var*100+g_base_hertz));
            }

            g_plot(col, row, s_o_color%g_colors);
        }
        else
        {
            // off-screen, don't continue unless periodicity=0
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
    g_which_image = g_glasses_type != GlassesType::NONE ? StereoImage::RED : StereoImage::NONE;
    plot_setup();
    g_plot = g_standard_plot;
    int status = calc();
    if (s_real_time && g_glasses_type < GlassesType::PHOTO)
    {
        s_real_time = false;
        goto done;
    }
    if (g_glasses_type != GlassesType::NONE && status == 0 && g_display_3d != Display3DMode::NONE)
    {
        if (g_glasses_type == GlassesType::PHOTO)
        {
            // photographer's mode
            stop_msg(StopMsgFlags::INFO_ONLY,
                "First image (left eye) is ready.  Hit any key to see it,\n"
                "then hit <s> to save, hit any other key to create second image.");
            for (int i = driver_get_key(); i == 's' || i == 'S'; i = driver_get_key())
            {
                save_image(g_save_filename);
            }
            // is there a better way to clear the screen in graphics mode?
            driver_set_video_mode(&g_video_entry);
        }
        g_which_image = StereoImage::BLUE;
        if (bit_set(g_cur_fractal_specific->flags, FractalFlags::INF_CALC))
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
        if (g_glasses_type == GlassesType::PHOTO)   // photographer's mode
        {
            stop_msg(StopMsgFlags::INFO_ONLY, "Second image (right eye) is ready");
        }
    }
done:
    if (g_glasses_type == GlassesType::STEREO_PAIR && g_screen_x_dots >= 2*g_logical_screen_x_dots)
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
static int ifs3d_float()
{
    int color;

    double new_x;
    double new_y;
    double new_z;

    int k;

    ViewTransform3D inf;

    // setup affine screen coord conversion
    setup_convert_to_screen(&inf.cvt);
    std::srand(1);
    int color_method = (int) g_params[0];
    if (driver_is_disk())                  // this would KILL a disk drive!
    {
        not_disk_msg();
    }

    inf.orbit[0] = 0;
    inf.orbit[1] = 0;
    inf.orbit[2] = 0;

    std::FILE *fp = open_orbit_save();

    int ret = 0;
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
        double r = std::rand();      // generate a random number between 0 and 1
        r /= RAND_MAX;

        // pick which iterated function to execute, weighted by probability
        constexpr int prob_index = NUM_IFS_3D_PARAMS - 1; // last parameter is probability
        double sum = g_ifs_definition[prob_index];
        for (k = 1; sum < r && k < g_num_affine_transforms; ++k)
        {
            sum += g_ifs_definition[k * NUM_IFS_3D_PARAMS + prob_index];
        }
        --k;

        // calculate image of last point under selected iterated function
        {
            const auto row = [&](int idx) { return g_ifs_definition[k * NUM_IFS_3D_PARAMS + idx]; };
            new_x = row(0) * inf.orbit[0] + row(1) * inf.orbit[1] + row(2) * inf.orbit[2] + row(9);
            new_y = row(3) * inf.orbit[0] + row(4) * inf.orbit[1] + row(5) * inf.orbit[2] + row(10);
            new_z = row(6) * inf.orbit[0] + row(7) * inf.orbit[1] + row(8) * inf.orbit[2] + row(11);
        }

        inf.orbit[0] = new_x;
        inf.orbit[1] = new_y;
        inf.orbit[2] = new_z;
        if (fp)
        {
            fmt::print(fp, "{:g} {:g} {:g} 15\n", new_x, new_y, new_z);
        }
        if (float_view_transf3d(&inf))
        {
            // plot if inside window
            if (inf.col >= 0)
            {
                if (s_real_time)
                {
                    g_which_image = StereoImage::RED;
                }
                if (color_method)
                {
                    color = (k%g_colors)+1;
                }
                else
                {
                    color = get_color(inf.col, inf.row)+1;
                }
                if (color < g_colors)     // color sticks on last value
                {
                    g_plot(inf.col, inf.row, color);
                }
            }
            else if (inf.col == -2)
            {
                return ret;
            }
            if (s_real_time)
            {
                g_which_image = StereoImage::BLUE;
                // plot if inside window
                if (inf.col1 >= 0)
                {
                    if (color_method)
                    {
                        color = (k%g_colors)+1;
                    }
                    else
                    {
                        color = get_color(inf.col1, inf.row1)+1;
                    }
                    if (color < g_colors)     // color sticks on last value
                    {
                        g_plot(inf.col1, inf.row1, color);
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

int ifs_type()                       // front-end for ifs2d and ifs3d
{
    if (g_ifs_definition.empty() && ifs_load() < 0)
    {
        return -1;
    }
    return g_ifs_dim == IFSDimension::TWO ? ifs2d() : ifs3d();
}

static int ifs2d()
{
    int color;
    Affine cvt;
    // setup affine screen coord conversion
    setup_convert_to_screen(&cvt);

    std::srand(1);
    int color_method = (int) g_params[0];

    std::FILE *fp = open_orbit_save();

    double x = 0;
    double y = 0;
    int ret = 0;
    g_max_count = g_max_iterations > 0x1fffffL ? 0x7fffffffL : g_max_iterations * 1024L;
    g_color_iter = 0L;
    while (g_color_iter++ <= g_max_count) // loop until keypress or maxit
    {
        if (driver_key_pressed())  // keypress bails out
        {
            ret = -1;
            break;
        }
        double r = static_cast<double>(RAND15())/32767.0; // generate random number between 0 and 1

        // pick which iterated function to execute, weighted by probability
        double sum = g_ifs_definition[6];  // [0][6]
        int k = 0;
        while (sum < r && k < g_num_affine_transforms-1)    // fixed bug of error if sum < 1
        {
            sum += g_ifs_definition[++k * NUM_IFS_2D_PARAMS + 6];
        }
        // calculate image of last point under selected iterated function
        float *f_f_ptr = g_ifs_definition.data() + k * NUM_IFS_2D_PARAMS; // point to first parm in row
        double new_x = *(f_f_ptr + 0) * x + *(f_f_ptr + 1) * y + *(f_f_ptr + 4);
        double new_y = *(f_f_ptr + 2) * x + *(f_f_ptr + 3) * y + *(f_f_ptr + 5);
        x = new_x;
        y = new_y;
        if (fp)
        {
            fmt::print(fp, "{:g} {:g} {:g} 15\n", new_x, new_y, 0.0);
        }

        // plot if inside window
        const int col = (int) (cvt.a * x + cvt.b * y + cvt.e);
        const int row = (int) (cvt.c * x + cvt.d * y + cvt.f);
        if (col >= 0 && col < g_logical_screen_x_dots && row >= 0 && row < g_logical_screen_y_dots)
        {
            if (color_method)
            {
                color = (k % g_colors) + 1;
            }
            else
            {
                // color is count of hits on this pixel
                color = get_color(col, row) + 1;
            }
            if (color < g_colors)     // color sticks on last value
            {
                g_plot(col, row, color);
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

static void setup_matrix(Matrix double_mat)
{
    // build transformation matrix
    identity(double_mat);

    // apply rotations - uses the same rotation variables as line3d.c
    x_rot((double)g_x_rot / 57.29577, double_mat);
    y_rot((double)g_y_rot / 57.29577, double_mat);
    z_rot((double)g_z_rot / 57.29577, double_mat);

    // apply scale
    //   scale((double)g_x_scale/100.0,(double)g_y_scale/100.0,(double)ROUGH/100.0,doublemat);
}

int orbit3d_type()
{
    g_display_3d = Display3DMode::MINUS_ONE ;
    s_real_time = g_glasses_type > GlassesType::NONE && g_glasses_type < GlassesType::PHOTO;
    return funny_glasses_call(orbit3d_float_calc);
}

static int ifs3d()
{
    g_display_3d = Display3DMode::MINUS_ONE;

    s_real_time = g_glasses_type > GlassesType::NONE && g_glasses_type < GlassesType::PHOTO;
    return funny_glasses_call(ifs3d_float); // double version of ifs3d
}

static bool float_view_transf3d(ViewTransform3D *inf)
{
    if (g_color_iter == 1)  // initialize on first call
    {
        for (int i = 0; i < 3; i++)
        {
            inf->min_vals[i] =  100000.0; // impossible value
            inf->max_vals[i] = -100000.0;
        }
        setup_matrix(inf->double_mat);
        if (s_real_time)
        {
            setup_matrix(inf->double_mat1);
        }
    }

    // 3D VIEWING TRANSFORM
    vec_mat_mul(inf->orbit, inf->double_mat, inf->view_vect);
    if (s_real_time)
    {
        vec_mat_mul(inf->orbit, inf->double_mat1, inf->view_vect1);
    }

    if (g_color_iter <= s_waste) // waste this many points to find minz and maxz
    {
        // find minz and maxz
        for (int i = 0; i < 3; i++)
        {
            const double tmp = inf->view_vect[i];
            if (tmp < inf->min_vals[i])
            {
                inf->min_vals[i] = tmp;
            }
            else if (tmp > inf->max_vals[i])
            {
                inf->max_vals[i] = tmp;
            }
        }
        if (g_color_iter == s_waste) // time to work it out
        {
            g_view[0] = 0; // center on origin
            g_view[1] = 0;
            /* z value of user's eye - should be more negative than extreme
                              negative part of image */
            g_view[2] = (inf->min_vals[2]-inf->max_vals[2])*(double)g_viewer_z/100.0;

            // center image on origin
            double tmp_x = (-inf->min_vals[0]-inf->max_vals[0])/(2.0); // center x
            double tmp_y = (-inf->min_vals[1]-inf->max_vals[1])/(2.0); // center y

            // apply perspective shift
            tmp_x += ((double)g_x_shift*(g_x_max-g_x_min))/(g_logical_screen_x_dots);
            tmp_y += ((double)g_y_shift*(g_y_max-g_y_min))/(g_logical_screen_y_dots);
            double tmp_z = -(inf->max_vals[2]);
            trans(tmp_x, tmp_y, tmp_z, inf->double_mat);

            if (s_real_time)
            {
                // center image on origin
                tmp_x = (-inf->min_vals[0]-inf->max_vals[0])/(2.0); // center x
                tmp_y = (-inf->min_vals[1]-inf->max_vals[1])/(2.0); // center y

                tmp_x += ((double)g_x_shift1*(g_x_max-g_x_min))/(g_logical_screen_x_dots);
                tmp_y += ((double)g_y_shift1*(g_y_max-g_y_min))/(g_logical_screen_y_dots);
                tmp_z = -(inf->max_vals[2]);
                trans(tmp_x, tmp_y, tmp_z, inf->double_mat1);
            }
        }
        return false;
    }

    // apply perspective if requested
    if (g_viewer_z)
    {
        perspective(inf->view_vect);
        if (s_real_time)
        {
            perspective(inf->view_vect1);
        }
    }
    inf->row = (int)(inf->cvt.c*inf->view_vect[0] + inf->cvt.d*inf->view_vect[1]
                     + inf->cvt.f + g_yy_adjust);
    inf->col = (int)(inf->cvt.a*inf->view_vect[0] + inf->cvt.b*inf->view_vect[1]
                     + inf->cvt.e + g_xx_adjust);
    if (inf->col < 0 || inf->col >= g_logical_screen_x_dots || inf->row < 0 || inf->row >= g_logical_screen_y_dots)
    {
        if ((long)std::abs(inf->col)+(long)std::abs(inf->row) > BAD_PIXEL)
        {
            inf->row = -2;
            inf->col = -2;
        }
        else
        {
            inf->row = -1;
            inf->col = -1;
        }
    }
    if (s_real_time)
    {
        inf->row1 = (int)(inf->cvt.c*inf->view_vect1[0] + inf->cvt.d*inf->view_vect1[1]
                          + inf->cvt.f + g_yy_adjust1);
        inf->col1 = (int)(inf->cvt.a*inf->view_vect1[0] + inf->cvt.b*inf->view_vect1[1]
                          + inf->cvt.e + g_xx_adjust1);
        if (inf->col1 < 0 || inf->col1 >= g_logical_screen_x_dots || inf->row1 < 0 || inf->row1 >= g_logical_screen_y_dots)
        {
            if ((long)std::abs(inf->col1)+(long)std::abs(inf->row1) > BAD_PIXEL)
            {
                inf->row1 = -2;
                inf->col1 = -2;
            }
            else
            {
                inf->row1 = -1;
                inf->col1 = -1;
            }
        }
    }
    return true;
}

static std::FILE *open_orbit_save()
{
    if (g_orbit_save_flags & OSF_RAW)
    {
        std::string path{id::io::get_save_path(id::io::WriteFile::ORBIT, g_orbit_save_name).string()};
        assert(!path.empty());
        check_write_file(path, ".raw");
        if (std::FILE *fp = std::fopen(path.c_str(), "w"); fp != nullptr)
        {
            fmt::print(fp, "pointlist x y z color\n");
            return fp;
        }
    }
    return nullptr;
}

// Plot a histogram by incrementing the pixel each time it is touched
static void plot_hist(int x, int y, int /*color*/)
{
    int color = get_color(x, y) + 1;
    if (color >= g_colors)
    {
        color = 1;
    }
    g_put_color(x, y, color);
}
