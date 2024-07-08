#include "divide_brot.h"

#include "bailout_formula.h"
#include "big.h"
#include "biginit.h"
#include "calcfrac.h"
#include "fpu087.h"
#include "fractalb.h"
#include "fractals.h"
#include "id_data.h"
#include "newton.h"
#include "parser.h"
#include "pixel_grid.h"

LDBL g_b_const{};

int dividebrot5bn_per_pixel()
{
    /* parm.x = xxmin + g_col*delx + g_row*delx2 */
    mult_bn_int(g_param_z_bn.x, g_delta_x_bn, (U16) g_col);
    mult_bn_int(g_bn_tmp, g_delta2_x_bn, (U16) g_row);

    add_a_bn(g_param_z_bn.x, g_bn_tmp);
    add_a_bn(g_param_z_bn.x, g_x_min_bn);

    /* parm.y = yymax - g_row*dely - g_col*dely2; */
    /* note: in next four lines, g_old_z_bn is just used as a temporary variable */
    mult_bn_int(g_old_z_bn.x, g_delta_y_bn, (U16) g_row);
    mult_bn_int(g_old_z_bn.y, g_delta2_y_bn, (U16) g_col);
    add_a_bn(g_old_z_bn.x, g_old_z_bn.y);
    sub_bn(g_param_z_bn.y, g_y_max_bn, g_old_z_bn.x);

    clear_bn(g_tmp_sqr_x_bn);
    clear_bn(g_tmp_sqr_y_bn);
    clear_bn(g_old_z_bn.x);
    clear_bn(g_old_z_bn.y);

    return 0; /* 1st iteration has NOT been done */
}

int dividebrot5bf_per_pixel()
{
    /* parm.x = xxmin + g_col*delx + g_row*delx2 */
    mult_bf_int(g_parm_z_bf.x, g_delta_x_bf, (U16) g_col);
    mult_bf_int(g_bf_tmp, g_delta2_x_bf, (U16) g_row);

    add_a_bf(g_parm_z_bf.x, g_bf_tmp);
    add_a_bf(g_parm_z_bf.x, g_bf_x_min);

    /* parm.y = yymax - g_row*dely - g_col*dely2; */
    /* note: in next four lines, g_old_z_bf is just used as a temporary variable */
    mult_bf_int(g_old_z_bf.x, g_delta_y_bf, (U16) g_row);
    mult_bf_int(g_old_z_bf.y, g_delta2_y_bf, (U16) g_col);
    add_a_bf(g_old_z_bf.x, g_old_z_bf.y);
    sub_bf(g_parm_z_bf.y, g_bf_y_max, g_old_z_bf.x);

    clear_bf(g_tmp_sqr_x_bf);
    clear_bf(g_tmp_sqr_y_bf);
    clear_bf(g_old_z_bf.x);
    clear_bf(g_old_z_bf.y);

    return 0; /* 1st iteration has NOT been done */
}

int DivideBrot5bnFractal()
{
    BNComplex bntmpnew, bnnumer, bnc_exp;
    bn_t tmp1, tmp2;
    int saved;
    saved = save_stack();

    bntmpnew.x = alloc_stack(g_r_length);
    bntmpnew.y = alloc_stack(g_r_length);
    bnnumer.x = alloc_stack(g_r_length);
    bnnumer.y = alloc_stack(g_r_length);
    bnc_exp.x = alloc_stack(g_bn_length);
    bnc_exp.y = alloc_stack(g_bn_length);
    tmp1 = alloc_stack(g_bn_length);
    tmp2 = alloc_stack(g_r_length);

    /* g_tmp_sqr_x_bn and g_tmp_sqr_y_bn were previously squared before getting to */
    /* this function, so they must be shifted.                           */

    /* sqr(z) */
    /* bnnumer.x = g_tmp_sqr_x_bn - g_tmp_sqr_y_bn;   */
    sub_bn(bnnumer.x, g_tmp_sqr_x_bn + g_shift_factor, g_tmp_sqr_y_bn + g_shift_factor);

    /* bnnumer.y = 2 * g_old_z_bn.x * g_old_z_bn.y; */
    mult_bn(tmp2, g_old_z_bn.x, g_old_z_bn.y);
    double_a_bn(tmp2 + g_shift_factor);
    copy_bn(bnnumer.y, tmp2 + g_shift_factor);

    /* z^(a) */
    inttobn(bnc_exp.x, g_c_exponent);
    clear_bn(bnc_exp.y);
    ComplexPower_bn(&bntmpnew, &g_old_z_bn, &bnc_exp);
    /* then add b */
    floattobn(tmp1, g_b_const);
    add_bn(bntmpnew.x, tmp1, bntmpnew.x + g_shift_factor);
    /* need to g_shift_factor bntmpnew.y */
    copy_bn(tmp2, bntmpnew.y + g_shift_factor);
    copy_bn(bntmpnew.y, tmp2);

    /* sqr(z)/(z^(a)+b) */
    cplxdiv_bn(&g_new_z_bn, &bnnumer, &bntmpnew);

    add_a_bn(g_new_z_bn.x, g_param_z_bn.x);
    add_a_bn(g_new_z_bn.y, g_param_z_bn.y);

    restore_stack(saved);
    return g_bailout_bignum();
}

int DivideBrot5bfFractal()
{
    BFComplex bftmpnew, bfnumer, bfc_exp;
    bf_t tmp1;
    int saved;
    saved = save_stack();

    bftmpnew.x = alloc_stack(g_r_bf_length + 2);
    bftmpnew.y = alloc_stack(g_r_bf_length + 2);
    bfnumer.x = alloc_stack(g_r_bf_length + 2);
    bfnumer.y = alloc_stack(g_r_bf_length + 2);
    bfc_exp.x = alloc_stack(g_bf_length + 2);
    bfc_exp.y = alloc_stack(g_bf_length + 2);
    tmp1 = alloc_stack(g_bf_length + 2);

    /* sqr(z) */
    /* bfnumer.x = g_tmp_sqr_x_bf - g_tmp_sqr_y_bf;   */
    sub_bf(bfnumer.x, g_tmp_sqr_x_bf, g_tmp_sqr_y_bf);

    /* bfnumer.y = 2 * g_old_z_bf.x * g_old_z_bf.y; */
    mult_bf(bfnumer.y, g_old_z_bf.x, g_old_z_bf.y);
    double_a_bf(bfnumer.y);

    /* z^(a) */
    inttobf(bfc_exp.x, g_c_exponent);
    clear_bf(bfc_exp.y);
    ComplexPower_bf(&bftmpnew, &g_old_z_bf, &bfc_exp);
    /* then add b */
    floattobf(tmp1, g_b_const);
    add_a_bf(bftmpnew.x, tmp1);

    /* sqr(z)/(z^(a)+b) */
    cplxdiv_bf(&g_new_z_bf, &bfnumer, &bftmpnew);

    add_a_bf(g_new_z_bf.x, g_parm_z_bf.x);
    add_a_bf(g_new_z_bf.y, g_parm_z_bf.y);

    restore_stack(saved);
    return g_bailout_bigfloat();
}

bool DivideBrot5Setup()
{
    g_c_exponent = -((int) g_params[0] - 2); /* use negative here so only need it once */
    g_b_const = g_params[1] + 1.0e-20;
    return true;
}

int DivideBrot5fp_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        g_init.y = g_dy_pixel();
    }

    g_temp_sqr_x = 0.0; /* precalculated value */
    g_temp_sqr_y = 0.0;
    g_old_z.x = 0.0;
    g_old_z.y = 0.0;
    return 0; /* 1st iteration has NOT been done */
}

int DivideBrot5fpFractal() /* from formula by Jim Muth */
{
    /* z=sqr(z)/(z^(-a)+b)+c */
    /* we'll set a to -a in setup, so don't need it here */
    /* z=sqr(z)/(z^(a)+b)+c */
    DComplex tmp_sqr, tmp1, tmp2;

    /* sqr(z) */
    tmp_sqr.x = g_temp_sqr_x - g_temp_sqr_y;
    tmp_sqr.y = g_old_z.x * g_old_z.y * 2.0;

    /* z^(a) = e^(a * log(z))*/
    FPUcplxlog(&g_old_z, &tmp1);
    tmp1.x *= g_c_exponent;
    tmp1.y *= g_c_exponent;
    FPUcplxexp(&tmp1, &tmp2);
    /* then add b */
    tmp2.x += g_b_const;
    /* sqr(z)/(z^(a)+b) */
    FPUcplxdiv(&tmp_sqr, &tmp2, &g_new_z);
    /* then add c = g_init = pixel */
    g_new_z.x += g_init.x;
    g_new_z.y += g_init.y;

    return g_bailout_float();
}
