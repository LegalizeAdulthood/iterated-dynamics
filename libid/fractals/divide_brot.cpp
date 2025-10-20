// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/divide_brot.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/fractalb.h"
#include "engine/fractals.h"
#include "engine/Inversion.h"
#include "engine/pixel_grid.h"
#include "fractals/newton.h"
#include "math/big.h"
#include "math/biginit.h"
#include "math/fpu087.h"

using namespace id::engine;
using namespace id::math;
using namespace id::misc;

namespace id::fractals
{

LDouble g_b_const{};

int divide_brot5_bn_per_pixel()
{
    /* parm.x = xxmin + g_col*delx + g_row*delx2 */
    mult_bn_int(g_param_z_bn.x, g_delta_x_bn, static_cast<U16>(g_col));
    mult_bn_int(g_bn_tmp, g_delta2_x_bn, static_cast<U16>(g_row));

    add_a_bn(g_param_z_bn.x, g_bn_tmp);
    add_a_bn(g_param_z_bn.x, g_x_min_bn);

    /* parm.y = yymax - g_row*dely - g_col*dely2; */
    /* note: in next four lines, g_old_z_bn is just used as a temporary variable */
    mult_bn_int(g_old_z_bn.x, g_delta_y_bn, static_cast<U16>(g_row));
    mult_bn_int(g_old_z_bn.y, g_delta2_y_bn, static_cast<U16>(g_col));
    add_a_bn(g_old_z_bn.x, g_old_z_bn.y);
    sub_bn(g_param_z_bn.y, g_y_max_bn, g_old_z_bn.x);

    clear_bn(g_tmp_sqr_x_bn);
    clear_bn(g_tmp_sqr_y_bn);
    clear_bn(g_old_z_bn.x);
    clear_bn(g_old_z_bn.y);

    return 0; /* 1st iteration has NOT been done */
}

int divide_brot5_bf_per_pixel()
{
    /* parm.x = xxmin + g_col*delx + g_row*delx2 */
    mult_bf_int(g_param_z_bf.x, g_delta_x_bf, static_cast<U16>(g_col));
    mult_bf_int(g_bf_tmp, g_delta2_x_bf, static_cast<U16>(g_row));

    add_a_bf(g_param_z_bf.x, g_bf_tmp);
    add_a_bf(g_param_z_bf.x, g_bf_x_min);

    /* parm.y = yymax - g_row*dely - g_col*dely2; */
    /* note: in next four lines, g_old_z_bf is just used as a temporary variable */
    mult_bf_int(g_old_z_bf.x, g_delta_y_bf, static_cast<U16>(g_row));
    mult_bf_int(g_old_z_bf.y, g_delta2_y_bf, static_cast<U16>(g_col));
    add_a_bf(g_old_z_bf.x, g_old_z_bf.y);
    sub_bf(g_param_z_bf.y, g_bf_y_max, g_old_z_bf.x);

    clear_bf(g_tmp_sqr_x_bf);
    clear_bf(g_tmp_sqr_y_bf);
    clear_bf(g_old_z_bf.x);
    clear_bf(g_old_z_bf.y);

    return 0; /* 1st iteration has NOT been done */
}

int divide_brot5_bn_fractal()
{
    BigStackSaver saved;
    BNComplex bn_tmp_new;
    BNComplex bn_numer;
    BNComplex bn_c_exp;

    bn_tmp_new.x = alloc_stack(g_r_length);
    bn_tmp_new.y = alloc_stack(g_r_length);
    bn_numer.x = alloc_stack(g_r_length);
    bn_numer.y = alloc_stack(g_r_length);
    bn_c_exp.x = alloc_stack(g_bn_length);
    bn_c_exp.y = alloc_stack(g_bn_length);
    BigNum tmp1 = alloc_stack(g_bn_length);
    BigNum tmp2 = alloc_stack(g_r_length);

    /* g_tmp_sqr_x_bn and g_tmp_sqr_y_bn were previously squared before getting to */
    /* this function, so they must be shifted.                           */

    /* sqr(z) */
    /* bnnumer.x = g_tmp_sqr_x_bn - g_tmp_sqr_y_bn;   */
    sub_bn(bn_numer.x, g_tmp_sqr_x_bn + g_shift_factor, g_tmp_sqr_y_bn + g_shift_factor);

    /* bnnumer.y = 2 * g_old_z_bn.x * g_old_z_bn.y; */
    mult_bn(tmp2, g_old_z_bn.x, g_old_z_bn.y);
    double_a_bn(tmp2 + g_shift_factor);
    copy_bn(bn_numer.y, tmp2 + g_shift_factor);

    /* z^(a) */
    int_to_bn(bn_c_exp.x, g_c_exponent);
    clear_bn(bn_c_exp.y);
    cmplx_pow_bn(&bn_tmp_new, &g_old_z_bn, &bn_c_exp);
    /* then add b */
    float_to_bn(tmp1, g_b_const);
    add_bn(bn_tmp_new.x, tmp1, bn_tmp_new.x + g_shift_factor);
    /* need to g_shift_factor bntmpnew.y */
    copy_bn(tmp2, bn_tmp_new.y + g_shift_factor);
    copy_bn(bn_tmp_new.y, tmp2);

    /* sqr(z)/(z^(a)+b) */
    cmplx_div_bn(&g_new_z_bn, &bn_numer, &bn_tmp_new);

    add_a_bn(g_new_z_bn.x, g_param_z_bn.x);
    add_a_bn(g_new_z_bn.y, g_param_z_bn.y);

    return g_bailout_bignum();
}

int divide_brot5_orbit_bf()
{
    BigStackSaver saved;
    BFComplex bf_tmp_new;
    BFComplex bf_numer;
    BFComplex bf_c_exp;

    bf_tmp_new.x = alloc_stack(g_r_bf_length + 2);
    bf_tmp_new.y = alloc_stack(g_r_bf_length + 2);
    bf_numer.x = alloc_stack(g_r_bf_length + 2);
    bf_numer.y = alloc_stack(g_r_bf_length + 2);
    bf_c_exp.x = alloc_stack(g_bf_length + 2);
    bf_c_exp.y = alloc_stack(g_bf_length + 2);
    BigFloat tmp1 = alloc_stack(g_bf_length + 2);

    /* sqr(z) */
    /* bfnumer.x = g_tmp_sqr_x_bf - g_tmp_sqr_y_bf;   */
    sub_bf(bf_numer.x, g_tmp_sqr_x_bf, g_tmp_sqr_y_bf);

    /* bfnumer.y = 2 * g_old_z_bf.x * g_old_z_bf.y; */
    mult_bf(bf_numer.y, g_old_z_bf.x, g_old_z_bf.y);
    double_a_bf(bf_numer.y);

    /* z^(a) */
    int_to_bf(bf_c_exp.x, g_c_exponent);
    clear_bf(bf_c_exp.y);
    cmplx_pow_bf(&bf_tmp_new, &g_old_z_bf, &bf_c_exp);
    /* then add b */
    float_to_bf(tmp1, g_b_const);
    add_a_bf(bf_tmp_new.x, tmp1);

    /* sqr(z)/(z^(a)+b) */
    cmplx_div_bf(&g_new_z_bf, &bf_numer, &bf_tmp_new);

    add_a_bf(g_new_z_bf.x, g_param_z_bf.x);
    add_a_bf(g_new_z_bf.y, g_param_z_bf.y);

    return g_bailout_bigfloat();
}

bool divide_brot5_per_image()
{
    g_c_exponent = -(static_cast<int>(g_params[0]) - 2); /* use negative here so only need it once */
    g_b_const = g_params[1] + 1.0e-20;
    return true;
}

int divide_brot5_per_pixel()
{
    if (g_inversion.invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = dx_pixel();
        g_init.y = dy_pixel();
    }

    g_temp_sqr_x = 0.0; /* precalculated value */
    g_temp_sqr_y = 0.0;
    g_old_z.x = 0.0;
    g_old_z.y = 0.0;
    return 0; /* 1st iteration has NOT been done */
}

int divide_brot5_orbit() /* from formula by Jim Muth */
{
    /* z=sqr(z)/(z^(-a)+b)+c */
    /* we'll set a to -a in setup, so don't need it here */
    /* z=sqr(z)/(z^(a)+b)+c */
    DComplex tmp_sqr;
    DComplex tmp1;
    DComplex tmp2;

    /* sqr(z) */
    tmp_sqr.x = g_temp_sqr_x - g_temp_sqr_y;
    tmp_sqr.y = g_old_z.x * g_old_z.y * 2.0;

    /* z^(a) = e^(a * log(z))*/
    fpu_cmplx_log(&g_old_z, &tmp1);
    tmp1.x *= g_c_exponent;
    tmp1.y *= g_c_exponent;
    fpu_cmplx_exp(&tmp1, &tmp2);
    /* then add b */
    tmp2.x += g_b_const;
    /* sqr(z)/(z^(a)+b) */
    fpu_cmplx_div(&tmp_sqr, &tmp2, &g_new_z);
    /* then add c = g_init = pixel */
    g_new_z.x += g_init.x;
    g_new_z.y += g_init.y;

    return g_bailout_float();
}

} // namespace id::fractals
