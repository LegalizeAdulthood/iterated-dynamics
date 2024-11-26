// SPDX-License-Identifier: GPL-3.0-only
//
#include "taylor_skinner_variations.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "frasetup.h"
#include "mpmath.h"
#include "mpmath_c.h"
#include "trig_fns.h"

// call float version of fractal if integer math overflow
static int TryFloatFractal(int (*fpFractal)())
{
    g_overflow = false;
    // lold had better not be changed!
    g_old_z.x = g_l_old_z.x;
    g_old_z.x /= g_fudge_factor;
    g_old_z.y = g_l_old_z.y;
    g_old_z.y /= g_fudge_factor;
    g_temp_sqr_x = sqr(g_old_z.x);
    g_temp_sqr_y = sqr(g_old_z.y);
    fpFractal();
    g_l_new_z.x = (long)(g_new_z.x*g_fudge_factor);
    g_l_new_z.y = (long)(g_new_z.y*g_fudge_factor);
    return 0;
}

int TrigPlusSqrFractal() // generalization of Scott and Skinner types
{
    // { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT }
    trig0(g_l_old_z, g_l_temp);       // ltmp = trig(lold)
    g_l_new_z = g_l_param * g_l_temp; // lnew = lparm*trig(lold)
    lcmplx_sqr_old(g_l_temp);          // ltmp = sqr(lold)
    g_l_temp = g_l_param2 * g_l_temp; // ltmp = lparm2*sqr(lold)
    g_l_new_z = g_l_new_z + g_l_temp; // lnew = lparm*trig(lold) + lparm2*sqr(lold)
    return g_bailout_long();
}

static int ScottTrigPlusSqrFractal()
{
    //  { z=pixel: z=trig(z)+sqr(z), |z|<BAILOUT }
    trig0(g_l_old_z, g_l_new_z);       // lnew = trig(lold)
    lcmplx_sqr_old(g_l_temp);           // lold = sqr(lold)
    g_l_new_z = g_l_temp + g_l_new_z;  // lnew = trig(lold)+sqr(lold)
    return g_bailout_long();
}

static int SkinnerTrigSubSqrFractal()
{
    // { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT }
    trig0(g_l_old_z, g_l_new_z);       // lnew = trig(lold)
    lcmplx_sqr_old(g_l_temp);           // lold = sqr(lold)
    g_l_new_z = g_l_new_z - g_l_temp;  // lnew = trig(lold) - sqr(lold)
    return g_bailout_long();
}

static bool TrigPlusSqrlongSetup()
{
    g_cur_fractal_specific->per_pixel =  julia_per_pixel;
    g_cur_fractal_specific->orbitcalc =  TrigPlusSqrFractal;
    if (g_l_param.x == g_fudge_factor && g_l_param.y == 0L && g_l_param2.y == 0L && g_debug_flag != debug_flags::force_standard_fractal)
    {
        if (g_l_param2.x == g_fudge_factor)          // Scott variant
        {
            g_cur_fractal_specific->orbitcalc =  ScottTrigPlusSqrFractal;
        }
        else if (g_l_param2.x == -g_fudge_factor)      // Skinner variant
        {
            g_cur_fractal_specific->orbitcalc =  SkinnerTrigSubSqrFractal;
        }
    }
    return julia_long_setup();
}

static int ScottTrigPlusTrigFractal()
{
    // z = trig0(z) + trig1(z)
    trig0(g_l_old_z, g_l_temp);
    trig1(g_l_old_z, g_l_old_z);
    g_l_new_z = g_l_temp + g_l_old_z;
    return g_bailout_long();
}

static int SkinnerTrigSubTrigFractal()
{
    // z = trig(0, z) - trig1(z)
    trig0(g_l_old_z, g_l_temp);
    LComplex ltmp2;
    trig1(g_l_old_z, ltmp2);
    g_l_new_z = g_l_temp - ltmp2;
    return g_bailout_long();
}

bool trig_plus_trig_long_setup()
{
    fn_plus_fn_sym();
    if (g_trig_index[1] == trig_fn::SQR)
    {
        return TrigPlusSqrlongSetup();
    }
    g_cur_fractal_specific->per_pixel =  long_julia_per_pixel;
    g_cur_fractal_specific->orbitcalc =  trig_plus_trig_fractal;
    if (g_l_param.x == g_fudge_factor && g_l_param.y == 0L && g_l_param2.y == 0L && g_debug_flag != debug_flags::force_standard_fractal)
    {
        if (g_l_param2.x == g_fudge_factor)          // Scott variant
        {
            g_cur_fractal_specific->orbitcalc =  ScottTrigPlusTrigFractal;
        }
        else if (g_l_param2.x == -g_fudge_factor)      // Skinner variant
        {
            g_cur_fractal_specific->orbitcalc =  SkinnerTrigSubTrigFractal;
        }
    }
    return julia_long_setup();
}

static int TrigPlusSqrfpFractal() // generalization of Scott and Skinner types
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

static int ScottTrigPlusSqrfpFractal() // float version
{
    // { z=pixel: z=sin(z)+sqr(z), |z|<BAILOUT }
    cmplx_trig0(g_old_z, g_new_z); // new = trig(old)
    cmplx_sqr_old(g_tmp_z);        // tmp = sqr(old)
    g_new_z += g_tmp_z;           // new = trig(old)+sqr(old)
    return g_bailout_float();
}

static int SkinnerTrigSubSqrfpFractal()
{
    // { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT }
    cmplx_trig0(g_old_z, g_new_z);   // new = trig(old)
    cmplx_sqr_old(g_tmp_z);          // old = sqr(old)
    g_new_z -= g_tmp_z;             // new = trig(old)-sqr(old)
    return g_bailout_float();
}

static bool TrigPlusSqrfpSetup()
{
    g_cur_fractal_specific->per_pixel =  julia_fp_per_pixel;
    g_cur_fractal_specific->orbitcalc =  TrigPlusSqrfpFractal;
    if (g_param_z1.x == 1.0 && g_param_z1.y == 0.0 && g_param_z2.y == 0.0 && g_debug_flag != debug_flags::force_standard_fractal)
    {
        if (g_param_z2.x == 1.0)          // Scott variant
        {
            g_cur_fractal_specific->orbitcalc =  ScottTrigPlusSqrfpFractal;
        }
        else if (g_param_z2.x == -1.0)      // Skinner variant
        {
            g_cur_fractal_specific->orbitcalc =  SkinnerTrigSubSqrfpFractal;
        }
    }
    return julia_fp_setup();
}

static int ScottTrigPlusTrigfpFractal()
{
    // z = trig0(z)+trig1(z)
    cmplx_trig0(g_old_z, g_tmp_z);
    DComplex tmp2;
    cmplx_trig1(g_old_z, tmp2);
    g_new_z = g_tmp_z + tmp2;
    return g_bailout_float();
}

static int SkinnerTrigSubTrigfpFractal()
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
    if (g_trig_index[1] == trig_fn::SQR)
    {
        return TrigPlusSqrfpSetup();
    }
    g_cur_fractal_specific->per_pixel =  other_julia_fp_per_pixel;
    g_cur_fractal_specific->orbitcalc =  trig_plus_trig_fp_fractal;
    if (g_param_z1.x == 1.0 && g_param_z1.y == 0.0 && g_param_z2.y == 0.0 && g_debug_flag != debug_flags::force_standard_fractal)
    {
        if (g_param_z2.x == 1.0)          // Scott variant
        {
            g_cur_fractal_specific->orbitcalc =  ScottTrigPlusTrigfpFractal;
        }
        else if (g_param_z2.x == -1.0)      // Skinner variant
        {
            g_cur_fractal_specific->orbitcalc =  SkinnerTrigSubTrigfpFractal;
        }
    }
    return julia_fp_setup();
}

int trig_plus_trig_fractal()
{
    // z = trig(0,z)*p1 + trig1(z)*p2
    trig0(g_l_old_z, g_l_temp);
    g_l_temp = g_l_param * g_l_temp;
    LComplex ltmp2;
    trig1(g_l_old_z, ltmp2);
    g_l_old_z = g_l_param2 * ltmp2;
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
    static symmetry_type fnplusfn[7][7] =
    {
        // fn2 ->sin     cos    sinh    cosh   exp    log    sqr
        // fn1
        /* sin */ {symmetry_type::PI_SYM,  symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* cos */ {symmetry_type::X_AXIS,  symmetry_type::PI_SYM,  symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* sinh*/ {symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* cosh*/ {symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* exp */ {symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* log */ {symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* sqr */ {symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::XY_AXIS}
    };
    if (g_param_z1.y == 0.0 && g_param_z2.y == 0.0)
    {
        if (g_trig_index[0] <= trig_fn::SQR && g_trig_index[1] < trig_fn::SQR)    // bounds of array
        {
            g_symmetry = fnplusfn[+g_trig_index[0]][+g_trig_index[1]];
        }
        if (g_trig_index[0] == trig_fn::FLIP || g_trig_index[1] == trig_fn::FLIP)
        {
            g_symmetry = symmetry_type::NONE;
        }
    }                 // defaults to X_AXIS symmetry
    else
    {
        g_symmetry = symmetry_type::NONE;
    }
    return false;
}

bool fn_x_fn_setup()
{
    static symmetry_type fnxfn[7][7] =
    {
        // fn2 ->sin     cos    sinh    cosh  exp    log    sqr
        // fn1
        /* sin */ {symmetry_type::PI_SYM,  symmetry_type::Y_AXIS,  symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* cos */ {symmetry_type::Y_AXIS,  symmetry_type::PI_SYM,  symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* sinh*/ {symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* cosh*/ {symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* exp */ {symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* log */ {symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::X_AXIS, symmetry_type::NONE},
        /* sqr */ {symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::NONE,   symmetry_type::XY_AXIS},
    };
    if (g_trig_index[0] <= trig_fn::SQR && g_trig_index[1] <= trig_fn::SQR)    // bounds of array
    {
        g_symmetry = fnxfn[+g_trig_index[0]][+g_trig_index[1]];
        // defaults to X_AXIS symmetry
    }
    else
    {
        if (g_trig_index[0] == trig_fn::LOG || g_trig_index[1] == trig_fn::LOG)
        {
            g_symmetry = symmetry_type::NONE;
        }
        if (g_trig_index[0] == trig_fn::COS || g_trig_index[1] == trig_fn::COS)
        {
            if (g_trig_index[0] == trig_fn::SIN || g_trig_index[1] == trig_fn::SIN)
            {
                g_symmetry = symmetry_type::PI_SYM;
            }
            if (g_trig_index[0] == trig_fn::COSXX || g_trig_index[1] == trig_fn::COSXX)
            {
                g_symmetry = symmetry_type::PI_SYM;
            }
        }
        if (g_trig_index[0] == trig_fn::COS && g_trig_index[1] == trig_fn::COS)
        {
            g_symmetry = symmetry_type::PI_SYM;
        }
    }
    if (g_cur_fractal_specific->isinteger)
    {
        return julia_long_setup();
    }
    else
    {
        return julia_fp_setup();
    }
}

int trig_x_trig_fractal()
{
    LComplex ltmp3;
    // z = trig0(z)*trig1(z)
    trig0(g_l_old_z, g_l_temp);
    trig1(g_l_old_z, ltmp3);
    g_l_new_z = g_l_temp * ltmp3;
    if (g_overflow)
    {
        TryFloatFractal(trig_x_trig_fp_fractal);
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
    long l16triglim_2 = 8L << 15;
    lcmplx_sqr_old(g_l_temp);
    if (labs(g_l_temp.x) > l16triglim_2 || labs(g_l_temp.y) > l16triglim_2)
    {
        g_overflow = true;
    }
    else
    {
        trig0(g_l_temp, g_l_new_z);
    }
    if (g_overflow)
    {
        TryFloatFractal(trig_z_sqrd_fp_fractal);
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
    case trig_fn::SIN:
    case trig_fn::COSXX: // cosxx
    case trig_fn::COS:   // 'real' cos
        g_symmetry = symmetry_type::PI_SYM;
        break;
        // default is for X_AXIS symmetry

    default:
        break;
    }
    if (g_cur_fractal_specific->isinteger)
    {
        return julia_long_setup();
    }
    else
    {
        return julia_fp_setup();
    }
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

static int ScottZXTrigPlusZFractal()
{
    // z = (z*trig(z))+z
    trig0(g_l_old_z, g_l_temp);        // ltmp  = trig(old)
    g_l_new_z = g_l_old_z * g_l_temp;  // lnew  = old*trig(old)
    g_l_new_z = g_l_new_z + g_l_old_z; // lnew  = trig(old) + old
    return g_bailout_long();
}

static int SkinnerZXTrigSubZFractal()
{
    // z = (z*trig(z))-z
    trig0(g_l_old_z, g_l_temp);        // ltmp  = trig(old)
    g_l_new_z = g_l_old_z * g_l_temp;  // lnew  = old*trig(old)
    g_l_new_z = g_l_new_z - g_l_old_z; // lnew  = trig(old) - old
    return g_bailout_long();
}

static int ScottZXTrigPlusZfpFractal()
{
    // z = (z*trig(z))+z
    cmplx_trig0(g_old_z, g_tmp_z);         // tmp  = trig(old)
    cmplx_mult(g_old_z, g_tmp_z, g_new_z); // new  = old*trig(old)
    g_new_z += g_old_z;                   // new  = trig(old) + old
    return g_bailout_float();
}

static int SkinnerZXTrigSubZfpFractal()
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
        case trig_fn::COSXX:
        case trig_fn::COSH:
        case trig_fn::SQR:
        case trig_fn::COS:
            g_symmetry = symmetry_type::XY_AXIS;
            break;
        case trig_fn::FLIP:
            g_symmetry = symmetry_type::Y_AXIS;
            break;
        case trig_fn::LOG:
            g_symmetry = symmetry_type::NONE;
            break;
        default:
            g_symmetry = symmetry_type::X_AXIS;
            break;
        }
    }
    else
    {
        //      symmetry = ZXTrigPlusZSym2[g_trig_index[0]];
        switch (g_trig_index[0])
        {
        case trig_fn::COSXX:
        case trig_fn::COSH:
        case trig_fn::SQR:
        case trig_fn::COS:
            g_symmetry = symmetry_type::ORIGIN;
            break;
        case trig_fn::FLIP:
            g_symmetry = symmetry_type::NONE;
            break;
        default:
            g_symmetry = symmetry_type::NONE;
            break;
        }
    }
    if (g_cur_fractal_specific->isinteger)
    {
        g_cur_fractal_specific->orbitcalc =  z_x_trig_plus_z_fractal;
        if (g_l_param.x == g_fudge_factor && g_l_param.y == 0L && g_l_param2.y == 0L && g_debug_flag != debug_flags::force_standard_fractal)
        {
            if (g_l_param2.x == g_fudge_factor)       // Scott variant
            {
                g_cur_fractal_specific->orbitcalc =  ScottZXTrigPlusZFractal;
            }
            else if (g_l_param2.x == -g_fudge_factor)      // Skinner variant
            {
                g_cur_fractal_specific->orbitcalc =  SkinnerZXTrigSubZFractal;
            }
        }
        return julia_long_setup();
    }
    else
    {
        g_cur_fractal_specific->orbitcalc =  z_x_trig_plus_z_fp_fractal;
        if (g_param_z1.x == 1.0 && g_param_z1.y == 0.0 && g_param_z2.y == 0.0 && g_debug_flag != debug_flags::force_standard_fractal)
        {
            if (g_param_z2.x == 1.0)       // Scott variant
            {
                g_cur_fractal_specific->orbitcalc =  ScottZXTrigPlusZfpFractal;
            }
            else if (g_param_z2.x == -1.0)           // Skinner variant
            {
                g_cur_fractal_specific->orbitcalc =  SkinnerZXTrigSubZfpFractal;
            }
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
    g_new_z = ComplexPower(*g_float_param, g_old_z);
    return g_bailout_float();
}
