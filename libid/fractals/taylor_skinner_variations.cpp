// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/taylor_skinner_variations.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "fractals/frasetup.h"
#include "math/fixed_pt.h"
#include "math/mpmath.h"
#include "misc/debug_flags.h"
#include "ui/cmdfiles.h"

#include <cmath>

// call float version of fractal if integer math overflow
static int try_float_fractal(int (*fp_fractal)())
{
    g_overflow = false;
    // lold had better not be changed!
    g_old_z.x = g_l_old_z.x;
    g_old_z.x /= g_fudge_factor;
    g_old_z.y = g_l_old_z.y;
    g_old_z.y /= g_fudge_factor;
    g_temp_sqr_x = sqr(g_old_z.x);
    g_temp_sqr_y = sqr(g_old_z.y);
    fp_fractal();
    g_l_new_z.x = (long)(g_new_z.x*g_fudge_factor);
    g_l_new_z.y = (long)(g_new_z.y*g_fudge_factor);
    return 0;
}

static int trig_plus_sqr_fractal() // generalization of Scott and Skinner types
{
    // { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT }
    trig0(g_l_old_z, g_l_temp);       // ltmp = trig(lold)
    g_l_new_z = g_l_param * g_l_temp; // lnew = lparm*trig(lold)
    lcmplx_sqr_old(g_l_temp);          // ltmp = sqr(lold)
    g_l_temp = g_l_param2 * g_l_temp; // ltmp = lparm2*sqr(lold)
    g_l_new_z = g_l_new_z + g_l_temp; // lnew = lparm*trig(lold) + lparm2*sqr(lold)
    return g_bailout_long();
}

static int scott_trig_plus_sqr_fractal()
{
    //  { z=pixel: z=trig(z)+sqr(z), |z|<BAILOUT }
    trig0(g_l_old_z, g_l_new_z);       // lnew = trig(lold)
    lcmplx_sqr_old(g_l_temp);           // lold = sqr(lold)
    g_l_new_z = g_l_temp + g_l_new_z;  // lnew = trig(lold)+sqr(lold)
    return g_bailout_long();
}

static int skinner_trig_sub_sqr_fractal()
{
    // { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT }
    trig0(g_l_old_z, g_l_new_z);       // lnew = trig(lold)
    lcmplx_sqr_old(g_l_temp);           // lold = sqr(lold)
    g_l_new_z = g_l_new_z - g_l_temp;  // lnew = trig(lold) - sqr(lold)
    return g_bailout_long();
}

static bool trig_plus_sqr_long_setup()
{
    g_cur_fractal_specific->per_pixel =  julia_per_pixel;
    g_cur_fractal_specific->orbit_calc =  trig_plus_sqr_fractal;
    if (g_l_param.x == g_fudge_factor && g_l_param.y == 0L && g_l_param2.y == 0L && g_debug_flag != DebugFlags::FORCE_STANDARD_FRACTAL)
    {
        if (g_l_param2.x == g_fudge_factor)          // Scott variant
        {
            g_cur_fractal_specific->orbit_calc =  scott_trig_plus_sqr_fractal;
        }
        else if (g_l_param2.x == -g_fudge_factor)      // Skinner variant
        {
            g_cur_fractal_specific->orbit_calc =  skinner_trig_sub_sqr_fractal;
        }
    }
    return julia_long_setup();
}

static int scott_trig_plus_trig_fractal()
{
    // z = trig0(z) + trig1(z)
    trig0(g_l_old_z, g_l_temp);
    trig1(g_l_old_z, g_l_old_z);
    g_l_new_z = g_l_temp + g_l_old_z;
    return g_bailout_long();
}

static int skinner_trig_sub_trig_fractal()
{
    // z = trig(0, z) - trig1(z)
    trig0(g_l_old_z, g_l_temp);
    LComplex tmp;
    trig1(g_l_old_z, tmp);
    g_l_new_z = g_l_temp - tmp;
    return g_bailout_long();
}

bool trig_plus_trig_long_setup()
{
    fn_plus_fn_sym();
    if (g_trig_index[1] == TrigFn::SQR)
    {
        return trig_plus_sqr_long_setup();
    }
    g_cur_fractal_specific->per_pixel =  long_julia_per_pixel;
    g_cur_fractal_specific->orbit_calc =  trig_plus_trig_fractal;
    if (g_l_param.x == g_fudge_factor && g_l_param.y == 0L && g_l_param2.y == 0L && g_debug_flag != DebugFlags::FORCE_STANDARD_FRACTAL)
    {
        if (g_l_param2.x == g_fudge_factor)          // Scott variant
        {
            g_cur_fractal_specific->orbit_calc =  scott_trig_plus_trig_fractal;
        }
        else if (g_l_param2.x == -g_fudge_factor)      // Skinner variant
        {
            g_cur_fractal_specific->orbit_calc =  skinner_trig_sub_trig_fractal;
        }
    }
    return julia_long_setup();
}

// generalization of Scott and Skinner types
static int trig_plus_sqr_fp_fractal()
{
    // { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT }
    cmplx_trig0(g_old_z, g_tmp_z);            // tmp = trig(old)
    cmplx_mult(g_param_z1, g_tmp_z, g_new_z); // new = parm*trig(old)
    cmplx_sqr_old(g_tmp_z);                   // tmp = sqr(old)
    DComplex tmp2;                           //
    cmplx_mult(g_param_z2, g_tmp_z, tmp2);    // tmp = parm2*sqr(old)
    g_new_z += tmp2;                         // new = parm*trig(old)+parm2*sqr(old)
    return g_bailout_float();
}

static int scott_trig_plus_sqr_fp_fractal() // float version
{
    // { z=pixel: z=sin(z)+sqr(z), |z|<BAILOUT }
    cmplx_trig0(g_old_z, g_new_z); // new = trig(old)
    cmplx_sqr_old(g_tmp_z);        // tmp = sqr(old)
    g_new_z += g_tmp_z;           // new = trig(old)+sqr(old)
    return g_bailout_float();
}

static int skinner_trig_sub_sqr_fp_fractal()
{
    // { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT }
    cmplx_trig0(g_old_z, g_new_z);   // new = trig(old)
    cmplx_sqr_old(g_tmp_z);          // old = sqr(old)
    g_new_z -= g_tmp_z;             // new = trig(old)-sqr(old)
    return g_bailout_float();
}

static bool trig_plus_sqr_fp_setup()
{
    g_cur_fractal_specific->per_pixel =  julia_fp_per_pixel;
    g_cur_fractal_specific->orbit_calc =  trig_plus_sqr_fp_fractal;
    if (g_param_z1.x == 1.0 && g_param_z1.y == 0.0 && g_param_z2.y == 0.0 && g_debug_flag != DebugFlags::FORCE_STANDARD_FRACTAL)
    {
        if (g_param_z2.x == 1.0)          // Scott variant
        {
            g_cur_fractal_specific->orbit_calc =  scott_trig_plus_sqr_fp_fractal;
        }
        else if (g_param_z2.x == -1.0)      // Skinner variant
        {
            g_cur_fractal_specific->orbit_calc =  skinner_trig_sub_sqr_fp_fractal;
        }
    }
    return julia_fp_setup();
}

static int scott_trig_plus_trig_fp_fractal()
{
    // z = trig0(z)+trig1(z)
    cmplx_trig0(g_old_z, g_tmp_z);
    DComplex tmp2;
    cmplx_trig1(g_old_z, tmp2);
    g_new_z = g_tmp_z + tmp2;
    return g_bailout_float();
}

static int skinner_trig_sub_trig_fp_fractal()
{
    // z = trig0(z)-trig1(z)
    cmplx_trig0(g_old_z, g_tmp_z);
    DComplex tmp2;
    cmplx_trig1(g_old_z, tmp2);
    g_new_z = g_tmp_z - tmp2;
    return g_bailout_float();
}

bool trig_plus_trig_fp_setup()
{
    fn_plus_fn_sym();
    if (g_trig_index[1] == TrigFn::SQR)
    {
        return trig_plus_sqr_fp_setup();
    }
    g_cur_fractal_specific->per_pixel =  other_julia_fp_per_pixel;
    g_cur_fractal_specific->orbit_calc =  trig_plus_trig_fp_fractal;
    if (g_param_z1.x == 1.0 && g_param_z1.y == 0.0 && g_param_z2.y == 0.0 && g_debug_flag != DebugFlags::FORCE_STANDARD_FRACTAL)
    {
        if (g_param_z2.x == 1.0)          // Scott variant
        {
            g_cur_fractal_specific->orbit_calc =  scott_trig_plus_trig_fp_fractal;
        }
        else if (g_param_z2.x == -1.0)      // Skinner variant
        {
            g_cur_fractal_specific->orbit_calc =  skinner_trig_sub_trig_fp_fractal;
        }
    }
    return julia_fp_setup();
}

int trig_plus_trig_fractal()
{
    // z = trig(0,z)*p1 + trig1(z)*p2
    trig0(g_l_old_z, g_l_temp);
    g_l_temp = g_l_param * g_l_temp;
    LComplex tmp;
    trig1(g_l_old_z, tmp);
    g_l_old_z = g_l_param2 * tmp;
    g_l_new_z = g_l_temp + g_l_old_z;
    return g_bailout_long();
}

int trig_plus_trig_fp_fractal()
{
    // z = trig0(z)*p1+trig1(z)*p2
    cmplx_trig0(g_old_z, g_tmp_z);
    cmplx_mult(g_param_z1, g_tmp_z, g_tmp_z);
    cmplx_trig1(g_old_z, g_old_z);
    cmplx_mult(g_param_z2, g_old_z, g_old_z);
    g_new_z = g_tmp_z + g_old_z;
    return g_bailout_float();
}

bool fn_plus_fn_sym() // set symmetry matrix for fn+fn type
{
    static SymmetryType fn_plus_fn[7][7] =
    {
        // fn2 ->sin     cos    sinh    cosh   exp    log    sqr
        // fn1
        /* sin */ {SymmetryType::PI_SYM,  SymmetryType::X_AXIS,  SymmetryType::XY_AXIS, SymmetryType::X_AXIS,  SymmetryType::X_AXIS, SymmetryType::X_AXIS, SymmetryType::X_AXIS},
        /* cos */ {SymmetryType::X_AXIS,  SymmetryType::PI_SYM,  SymmetryType::X_AXIS,  SymmetryType::XY_AXIS, SymmetryType::X_AXIS, SymmetryType::X_AXIS, SymmetryType::X_AXIS},
        /* sinh*/ {SymmetryType::XY_AXIS, SymmetryType::X_AXIS,  SymmetryType::XY_AXIS, SymmetryType::X_AXIS,  SymmetryType::X_AXIS, SymmetryType::X_AXIS, SymmetryType::X_AXIS},
        /* cosh*/ {SymmetryType::X_AXIS,  SymmetryType::XY_AXIS, SymmetryType::X_AXIS,  SymmetryType::XY_AXIS, SymmetryType::X_AXIS, SymmetryType::X_AXIS, SymmetryType::X_AXIS},
        /* exp */ {SymmetryType::X_AXIS,  SymmetryType::XY_AXIS, SymmetryType::X_AXIS,  SymmetryType::X_AXIS,  SymmetryType::XY_AXIS, SymmetryType::X_AXIS, SymmetryType::X_AXIS},
        /* log */ {SymmetryType::X_AXIS,  SymmetryType::X_AXIS,  SymmetryType::X_AXIS,  SymmetryType::X_AXIS,  SymmetryType::X_AXIS, SymmetryType::X_AXIS, SymmetryType::X_AXIS},
        /* sqr */ {SymmetryType::X_AXIS,  SymmetryType::X_AXIS,  SymmetryType::X_AXIS,  SymmetryType::X_AXIS,  SymmetryType::X_AXIS, SymmetryType::X_AXIS, SymmetryType::XY_AXIS}
    };
    if (g_param_z1.y == 0.0 && g_param_z2.y == 0.0)
    {
        if (g_trig_index[0] <= TrigFn::SQR && g_trig_index[1] < TrigFn::SQR)    // bounds of array
        {
            g_symmetry = fn_plus_fn[+g_trig_index[0]][+g_trig_index[1]];
        }
        if (g_trig_index[0] == TrigFn::FLIP || g_trig_index[1] == TrigFn::FLIP)
        {
            g_symmetry = SymmetryType::NONE;
        }
    }                 // defaults to X_AXIS symmetry
    else
    {
        g_symmetry = SymmetryType::NONE;
    }
    return false;
}

bool fn_x_fn_setup()
{
    static SymmetryType fn_x_fn[7][7] =
    {
        // fn2 ->sin     cos    sinh    cosh  exp    log    sqr
        // fn1
        /* sin */ {SymmetryType::PI_SYM,  SymmetryType::Y_AXIS,  SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::X_AXIS,  SymmetryType::NONE,   SymmetryType::XY_AXIS},
        /* cos */ {SymmetryType::Y_AXIS,  SymmetryType::PI_SYM,  SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::X_AXIS,  SymmetryType::NONE,   SymmetryType::XY_AXIS},
        /* sinh*/ {SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::X_AXIS,  SymmetryType::NONE,   SymmetryType::XY_AXIS},
        /* cosh*/ {SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::X_AXIS,  SymmetryType::NONE,   SymmetryType::XY_AXIS},
        /* exp */ {SymmetryType::X_AXIS,  SymmetryType::X_AXIS,  SymmetryType::X_AXIS,  SymmetryType::X_AXIS,  SymmetryType::X_AXIS,  SymmetryType::NONE,   SymmetryType::XY_AXIS},
        /* log */ {SymmetryType::NONE,    SymmetryType::NONE,    SymmetryType::NONE,    SymmetryType::NONE,    SymmetryType::NONE,    SymmetryType::X_AXIS, SymmetryType::NONE},
        /* sqr */ {SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::XY_AXIS, SymmetryType::NONE,   SymmetryType::XY_AXIS},
    };
    if (g_trig_index[0] <= TrigFn::SQR && g_trig_index[1] <= TrigFn::SQR)    // bounds of array
    {
        g_symmetry = fn_x_fn[+g_trig_index[0]][+g_trig_index[1]];
        // defaults to X_AXIS symmetry
    }
    else
    {
        if (g_trig_index[0] == TrigFn::LOG || g_trig_index[1] == TrigFn::LOG)
        {
            g_symmetry = SymmetryType::NONE;
        }
        if (g_trig_index[0] == TrigFn::COS || g_trig_index[1] == TrigFn::COS)
        {
            if (g_trig_index[0] == TrigFn::SIN || g_trig_index[1] == TrigFn::SIN)
            {
                g_symmetry = SymmetryType::PI_SYM;
            }
            if (g_trig_index[0] == TrigFn::COSXX || g_trig_index[1] == TrigFn::COSXX)
            {
                g_symmetry = SymmetryType::PI_SYM;
            }
        }
        if (g_trig_index[0] == TrigFn::COS && g_trig_index[1] == TrigFn::COS)
        {
            g_symmetry = SymmetryType::PI_SYM;
        }
    }
    return g_cur_fractal_specific->is_integer ? julia_long_setup() : julia_fp_setup();
}

int trig_x_trig_fractal()
{
    LComplex tmp;
    // z = trig0(z)*trig1(z)
    trig0(g_l_old_z, g_l_temp);
    trig1(g_l_old_z, tmp);
    g_l_new_z = g_l_temp * tmp;
    if (g_overflow)
    {
        try_float_fractal(trig_x_trig_fp_fractal);
    }
    return g_bailout_long();
}

int trig_x_trig_fp_fractal()
{
    // z = trig0(z)*trig1(z)
    cmplx_trig0(g_old_z, g_tmp_z);
    cmplx_trig1(g_old_z, g_old_z);
    cmplx_mult(g_tmp_z, g_old_z, g_new_z);
    return g_bailout_float();
}

int trig_z_sqrd_fp_fractal()
{
    // { z=pixel: z=trig(z*z), |z|<TEST }
    cmplx_sqr_old(g_tmp_z);
    cmplx_trig0(g_tmp_z, g_new_z);
    return g_bailout_float();
}

int trig_z_sqrd_fractal() // this doesn't work very well
{
    // { z=pixel: z=trig(z*z), |z|<TEST }
    long l16_trig_lim2 = 8L << 15;
    lcmplx_sqr_old(g_l_temp);
    if (std::abs(g_l_temp.x) > l16_trig_lim2 || std::abs(g_l_temp.y) > l16_trig_lim2)
    {
        g_overflow = true;
    }
    else
    {
        trig0(g_l_temp, g_l_new_z);
    }
    if (g_overflow)
    {
        try_float_fractal(trig_z_sqrd_fp_fractal);
    }
    return g_bailout_long();
}

bool sqr_trig_setup()
{
    //   static char SqrTrigSym[] =
    // fn1 ->  sin    cos    sinh   cosh   sqr    exp   log
    //           {PI_SYM, PI_SYM, XY_AXIS, XY_AXIS, XY_AXIS, X_AXIS, X_AXIS};
    switch (g_trig_index[0]) // fix sqr symmetry & add additional functions
    {
    case TrigFn::SIN:
    case TrigFn::COSXX: // cosxx
    case TrigFn::COS:   // 'real' cos
        g_symmetry = SymmetryType::PI_SYM;
        break;
        // default is for X_AXIS symmetry

    default:
        break;
    }
    return g_cur_fractal_specific->is_integer ? julia_long_setup() : julia_fp_setup();
}

int sqr_trig_fractal()
{
    // { z=pixel: z=sqr(trig(z)), |z|<TEST}
    trig0(g_l_old_z, g_l_temp);
    lcmplx_sqr(g_l_temp, g_l_new_z);
    return g_bailout_long();
}

int sqr_trig_fp_fractal()
{
    // SZSB(XYAXIS) { z=pixel, TEST=(p1+3): z=sin(z)*sin(z), |z|<TEST}
    cmplx_trig0(g_old_z, g_tmp_z);
    cmplx_sqr(g_tmp_z, g_new_z);
    return g_bailout_float();
}

int sqr_1_over_trig_fractal()
{
    // z = sqr(1/trig(z))
    trig0(g_l_old_z, g_l_old_z);
    lcmplx_recip(g_l_old_z, g_l_old_z);
    lcmplx_sqr(g_l_old_z, g_l_new_z);
    return g_bailout_long();
}

int sqr_1_over_trig_fp_fractal()
{
    // z = sqr(1/trig(z))
    cmplx_trig0(g_old_z, g_old_z);
    cmplx_recip(g_old_z, g_old_z);
    cmplx_sqr(g_old_z, g_new_z);
    return g_bailout_float();
}

static int scott_z_x_trig_plus_z_fractal()
{
    // z = (z*trig(z))+z
    trig0(g_l_old_z, g_l_temp);        // ltmp  = trig(old)
    g_l_new_z = g_l_old_z * g_l_temp;  // lnew  = old*trig(old)
    g_l_new_z = g_l_new_z + g_l_old_z; // lnew  = trig(old) + old
    return g_bailout_long();
}

static int skinner_z_x_trig_sub_z_fractal()
{
    // z = (z*trig(z))-z
    trig0(g_l_old_z, g_l_temp);        // ltmp  = trig(old)
    g_l_new_z = g_l_old_z * g_l_temp;  // lnew  = old*trig(old)
    g_l_new_z = g_l_new_z - g_l_old_z; // lnew  = trig(old) - old
    return g_bailout_long();
}

static int scott_z_x_trig_plus_z_fp_fractal()
{
    // z = (z*trig(z))+z
    cmplx_trig0(g_old_z, g_tmp_z);         // tmp  = trig(old)
    cmplx_mult(g_old_z, g_tmp_z, g_new_z); // new  = old*trig(old)
    g_new_z += g_old_z;                   // new  = trig(old) + old
    return g_bailout_float();
}

static int skinner_z_x_trig_sub_z_fp_fractal()
{
    // z = (z*trig(z))-z
    cmplx_trig0(g_old_z, g_tmp_z);         // tmp  = trig(old)
    cmplx_mult(g_old_z, g_tmp_z, g_new_z); // new  = old*trig(old)
    g_new_z -= g_old_z;                   // new  = trig(old) - old
    return g_bailout_float();
}

bool z_x_trig_plus_z_setup()
{
    //   static char ZXTrigPlusZSym1[] =
    // fn1 ->  sin   cos    sinh  cosh exp   log   sqr
    //           {X_AXIS, XY_AXIS, X_AXIS, XY_AXIS, X_AXIS, NONE, XY_AXIS};
    //   static char ZXTrigPlusZSym2[] =
    // fn1 ->  sin   cos    sinh  cosh exp   log   sqr
    //           {NONE, ORIGIN, NONE, ORIGIN, NONE, NONE, ORIGIN};

    if (g_params[1] == 0.0 && g_params[3] == 0.0)
    {
        //      symmetry = ZXTrigPlusZSym1[g_trig_index[0]];
        switch (g_trig_index[0])
        {
        case TrigFn::COSXX:
        case TrigFn::COSH:
        case TrigFn::SQR:
        case TrigFn::COS:
            g_symmetry = SymmetryType::XY_AXIS;
            break;
        case TrigFn::FLIP:
            g_symmetry = SymmetryType::Y_AXIS;
            break;
        case TrigFn::LOG:
            g_symmetry = SymmetryType::NONE;
            break;
        default:
            g_symmetry = SymmetryType::X_AXIS;
            break;
        }
    }
    else
    {
        //      symmetry = ZXTrigPlusZSym2[g_trig_index[0]];
        switch (g_trig_index[0])
        {
        case TrigFn::COSXX:
        case TrigFn::COSH:
        case TrigFn::SQR:
        case TrigFn::COS:
            g_symmetry = SymmetryType::ORIGIN;
            break;
        case TrigFn::FLIP:
            g_symmetry = SymmetryType::NONE;
            break;
        default:
            g_symmetry = SymmetryType::NONE;
            break;
        }
    }
    if (g_cur_fractal_specific->is_integer)
    {
        g_cur_fractal_specific->orbit_calc = z_x_trig_plus_z_fractal;
        if (g_l_param.x == g_fudge_factor && g_l_param.y == 0L && g_l_param2.y == 0L &&
            g_debug_flag != DebugFlags::FORCE_STANDARD_FRACTAL)
        {
            if (g_l_param2.x == g_fudge_factor) // Scott variant
            {
                g_cur_fractal_specific->orbit_calc = scott_z_x_trig_plus_z_fractal;
            }
            else if (g_l_param2.x == -g_fudge_factor) // Skinner variant
            {
                g_cur_fractal_specific->orbit_calc = skinner_z_x_trig_sub_z_fractal;
            }
        }
        return julia_long_setup();
    }
    g_cur_fractal_specific->orbit_calc = z_x_trig_plus_z_fp_fractal;
    if (g_param_z1.x == 1.0 && g_param_z1.y == 0.0 && g_param_z2.y == 0.0 &&
        g_debug_flag != DebugFlags::FORCE_STANDARD_FRACTAL)
    {
        if (g_param_z2.x == 1.0) // Scott variant
        {
            g_cur_fractal_specific->orbit_calc = scott_z_x_trig_plus_z_fp_fractal;
        }
        else if (g_param_z2.x == -1.0) // Skinner variant
        {
            g_cur_fractal_specific->orbit_calc = skinner_z_x_trig_sub_z_fp_fractal;
        }
    }
    return julia_fp_setup();
}

int z_x_trig_plus_z_fp_fractal()
{
    // z = (p1*z*trig(z))+p2*z
    cmplx_trig0(g_old_z, g_tmp_z);            // tmp  = trig(old)
    cmplx_mult(g_param_z1, g_tmp_z, g_tmp_z); // tmp  = p1*trig(old)
    DComplex tmp2;                           //
    cmplx_mult(g_old_z, g_tmp_z, tmp2);       // tmp2 = p1*old*trig(old)
    cmplx_mult(g_param_z2, g_old_z, g_tmp_z); // tmp  = p2*old
    g_new_z = tmp2 + g_tmp_z;                // new  = p1*trig(old) + p2*old
    return g_bailout_float();
}

int z_x_trig_plus_z_fractal()
{
    // z = (p1*z*trig(z))+p2*z
    trig0(g_l_old_z, g_l_temp);                 // ltmp  = trig(old)
    g_l_temp = g_l_param * g_l_temp;            // ltmp  = p1*trig(old)
    const LComplex ltmp2{g_l_old_z * g_l_temp}; // ltmp2 = p1*old*trig(old)
    g_l_temp = g_l_param2 * g_l_old_z;          // ltmp  = p2*old
    g_l_new_z = ltmp2 + g_l_temp;               // lnew  = p1*trig(old) + p2*old
    return g_bailout_long();
}

int man_o_war_fractal()
{
    // From Art Matrix via Lee Skinner
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_l_temp.x + g_long_param->x;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_l_temp.y + g_long_param->y;
    g_l_temp = g_l_old_z;
    return g_bailout_long();
}

int man_o_war_fp_fractal()
{
    // From Art Matrix via Lee Skinner
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step
    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_tmp_z.x + g_float_param->x;
    g_new_z.y = 2.0 * g_old_z.x * g_old_z.y + g_tmp_z.y + g_float_param->y;
    g_tmp_z = g_old_z;
    return g_bailout_float();
}

int richard_8_fp_fractal()
{
    //  Richard8 {c = z = pixel: z=sin(z)+sin(pixel),|z|<=50}
    cmplx_trig0(g_old_z, g_new_z);
    g_new_z.x += g_tmp_z.x;
    g_new_z.y += g_tmp_z.y;
    return g_bailout_float();
}

int richard_8_fractal()
{
    //  Richard8 {c = z = pixel: z=sin(z)+sin(pixel),|z|<=50}
    trig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x += g_l_temp.x;
    g_l_new_z.y += g_l_temp.y;
    return g_bailout_long();
}

int other_richard_8_fp_per_pixel()
{
    other_mandel_fp_per_pixel();
    cmplx_trig1(*g_float_param, g_tmp_z);
    cmplx_mult(g_tmp_z, g_param_z2, g_tmp_z);
    return 1;
}

int long_richard_8_per_pixel()
{
    long_mandel_per_pixel();
    trig1(*g_long_param, g_l_temp);
    g_l_temp = g_l_temp * g_l_param2;
    return 1;
}

int spider_fp_fractal()
{
    // Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 }
    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_tmp_z.x;
    g_new_z.y = 2 * g_old_z.x * g_old_z.y + g_tmp_z.y;
    g_tmp_z.x = g_tmp_z.x/2 + g_new_z.x;
    g_tmp_z.y = g_tmp_z.y/2 + g_new_z.y;
    return g_bailout_float();
}

int spider_fractal()
{
    // Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 }
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_l_temp.x;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_l_temp.y;
    g_l_temp.x = (g_l_temp.x >> 1) + g_l_new_z.x;
    g_l_temp.y = (g_l_temp.y >> 1) + g_l_new_z.y;
    return g_bailout_long();
}

int tetrate_fp_fractal()
{
    // Tetrate(XAXIS) { c=z=pixel: z=c^z, |z|<=(P1+3) }
    g_new_z = complex_power(*g_float_param, g_old_z);
    return g_bailout_float();
}
