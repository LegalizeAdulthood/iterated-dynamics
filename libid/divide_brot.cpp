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

LDBL g_b_const;

static BNComplex *cplxmul_bn(BNComplex *t, BNComplex *x, BNComplex *y)
{
    bn_t tmp1;
    int saved;
    saved = save_stack();
    tmp1 = alloc_stack(rlength);
    mult_bn(t->x, x->x, y->x);
    mult_bn(t->y, x->y, y->y);
    sub_bn(t->x, t->x + shiftfactor, t->y + shiftfactor);

    mult_bn(tmp1, x->x, y->y);
    mult_bn(t->y, x->y, y->x);
    add_bn(t->y, tmp1 + shiftfactor, t->y + shiftfactor);
    restore_stack(saved);
    return t;
}

static BNComplex *cplxdiv_bn(BNComplex *t, BNComplex *x, BNComplex *y)
{
    bn_t tmp1, tmp2, denom;
    int saved;
    saved = save_stack();
    tmp1 = alloc_stack(rlength);
    tmp2 = alloc_stack(rlength);
    denom = alloc_stack(rlength);

    square_bn(tmp1, y->x);
    square_bn(tmp2, y->y);
    add_bn(denom, tmp1 + shiftfactor, tmp2 + shiftfactor);

    if (is_bn_zero(x->x) && is_bn_zero(x->y))
    {
        clear_bn(t->x);
        clear_bn(t->y);
    }
    else if (is_bn_zero(denom))
    {
        g_overflow = true;
    }
    else
    {
        mult_bn(tmp1, x->x, y->x);
        mult_bn(t->x, x->y, y->y);
        add_bn(tmp2, tmp1 + shiftfactor, t->x + shiftfactor);
        div_bn(t->x, tmp2, denom);

        mult_bn(tmp1, x->y, y->x);
        mult_bn(t->y, x->x, y->y);
        sub_bn(tmp2, tmp1 + shiftfactor, t->y + shiftfactor);
        div_bn(t->y, tmp2, denom);
    }

    restore_stack(saved);
    return t;
}

/* note: ComplexPower_bn() returns need to be +shiftfactor'ed */
static BNComplex *ComplexPower_bn(BNComplex *t, BNComplex *xx, BNComplex *yy)
{
    BNComplex tmp;
    bn_t e2x, siny, cosy;
    int saved;
    saved = save_stack();
    e2x = alloc_stack(rlength);
    siny = alloc_stack(rlength);
    cosy = alloc_stack(rlength);
    tmp.x = alloc_stack(rlength);
    tmp.y = alloc_stack(rlength);

    /* 0 raised to anything is 0 */
    if (is_bn_zero(xx->x) && is_bn_zero(xx->y))
    {
        clear_bn(t->x);
        clear_bn(t->y);
    }
    else
    {
        cmplxlog_bn(t, xx);
        cplxmul_bn(&tmp, t, yy);
        exp_bn(e2x, tmp.x);
        sincos_bn(siny, cosy, tmp.y);
        mult_bn(t->x, e2x, cosy);
        mult_bn(t->y, e2x, siny);
    }
    restore_stack(saved);
    return t;
}

int dividebrot5bn_per_pixel()
{
    /* parm.x = xxmin + g_col*delx + g_row*delx2 */
    mult_bn_int(bnparm.x, bnxdel, (U16) g_col);
    mult_bn_int(bntmp, bnxdel2, (U16) g_row);

    add_a_bn(bnparm.x, bntmp);
    add_a_bn(bnparm.x, bnxmin);

    /* parm.y = yymax - g_row*dely - g_col*dely2; */
    /* note: in next four lines, bnold is just used as a temporary variable */
    mult_bn_int(bnold.x, bnydel, (U16) g_row);
    mult_bn_int(bnold.y, bnydel2, (U16) g_col);
    add_a_bn(bnold.x, bnold.y);
    sub_bn(bnparm.y, bnymax, bnold.x);

    clear_bn(bntmpsqrx);
    clear_bn(bntmpsqry);
    clear_bn(bnold.x);
    clear_bn(bnold.y);

    return 0; /* 1st iteration has NOT been done */
}

int dividebrot5bf_per_pixel()
{
    /* parm.x = xxmin + g_col*delx + g_row*delx2 */
    mult_bf_int(bfparm.x, bfxdel, (U16) g_col);
    mult_bf_int(bftmp, bfxdel2, (U16) g_row);

    add_a_bf(bfparm.x, bftmp);
    add_a_bf(bfparm.x, g_bf_x_min);

    /* parm.y = yymax - g_row*dely - g_col*dely2; */
    /* note: in next four lines, bfold is just used as a temporary variable */
    mult_bf_int(bfold.x, bfydel, (U16) g_row);
    mult_bf_int(bfold.y, bfydel2, (U16) g_col);
    add_a_bf(bfold.x, bfold.y);
    sub_bf(bfparm.y, g_bf_y_max, bfold.x);

    clear_bf(bftmpsqrx);
    clear_bf(bftmpsqry);
    clear_bf(bfold.x);
    clear_bf(bfold.y);

    return 0; /* 1st iteration has NOT been done */
}

int DivideBrot5bnFractal()
{
    BNComplex bntmpnew, bnnumer, bnc_exp;
    bn_t tmp1, tmp2;
    int saved;
    saved = save_stack();

    bntmpnew.x = alloc_stack(rlength);
    bntmpnew.y = alloc_stack(rlength);
    bnnumer.x = alloc_stack(rlength);
    bnnumer.y = alloc_stack(rlength);
    bnc_exp.x = alloc_stack(bnlength);
    bnc_exp.y = alloc_stack(bnlength);
    tmp1 = alloc_stack(bnlength);
    tmp2 = alloc_stack(rlength);

    /* bntmpsqrx and bntmpsqry were previously squared before getting to */
    /* this function, so they must be shifted.                           */

    /* sqr(z) */
    /* bnnumer.x = bntmpsqrx - bntmpsqry;   */
    sub_bn(bnnumer.x, bntmpsqrx + shiftfactor, bntmpsqry + shiftfactor);

    /* bnnumer.y = 2 * bnold.x * bnold.y; */
    mult_bn(tmp2, bnold.x, bnold.y);
    double_a_bn(tmp2 + shiftfactor);
    copy_bn(bnnumer.y, tmp2 + shiftfactor);

    /* z^(a) */
    inttobn(bnc_exp.x, g_c_exponent);
    clear_bn(bnc_exp.y);
    ComplexPower_bn(&bntmpnew, &bnold, &bnc_exp);
    /* then add b */
    floattobn(tmp1, g_b_const);
    add_bn(bntmpnew.x, tmp1, bntmpnew.x + shiftfactor);
    /* need to shiftfactor bntmpnew.y */
    copy_bn(tmp2, bntmpnew.y + shiftfactor);
    copy_bn(bntmpnew.y, tmp2);

    /* sqr(z)/(z^(a)+b) */
    cplxdiv_bn(&bnnew, &bnnumer, &bntmpnew);

    add_a_bn(bnnew.x, bnparm.x);
    add_a_bn(bnnew.y, bnparm.y);

    restore_stack(saved);
    return g_bailout_bignum();
}

int DivideBrot5bfFractal()
{
    BFComplex bftmpnew, bfnumer, bfc_exp;
    bf_t tmp1;
    int saved;
    saved = save_stack();

    bftmpnew.x = alloc_stack(rbflength + 2);
    bftmpnew.y = alloc_stack(rbflength + 2);
    bfnumer.x = alloc_stack(rbflength + 2);
    bfnumer.y = alloc_stack(rbflength + 2);
    bfc_exp.x = alloc_stack(bflength + 2);
    bfc_exp.y = alloc_stack(bflength + 2);
    tmp1 = alloc_stack(bflength + 2);

    /* sqr(z) */
    /* bfnumer.x = bftmpsqrx - bftmpsqry;   */
    sub_bf(bfnumer.x, bftmpsqrx, bftmpsqry);

    /* bfnumer.y = 2 * bfold.x * bfold.y; */
    mult_bf(bfnumer.y, bfold.x, bfold.y);
    double_a_bf(bfnumer.y);

    /* z^(a) */
    inttobf(bfc_exp.x, g_c_exponent);
    clear_bf(bfc_exp.y);
    ComplexPower_bf(&bftmpnew, &bfold, &bfc_exp);
    /* then add b */
    floattobf(tmp1, g_b_const);
    add_a_bf(bftmpnew.x, tmp1);

    /* sqr(z)/(z^(a)+b) */
    cplxdiv_bf(&bfnew, &bfnumer, &bftmpnew);

    add_a_bf(bfnew.x, bfparm.x);
    add_a_bf(bfnew.y, bfparm.y);

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
