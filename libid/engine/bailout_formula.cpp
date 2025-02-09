// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/bailout_formula.h"

#include "engine/calcfrac.h"
#include "engine/fractalb.h"
#include "engine/fractals.h"
#include "math/biginit.h"

#include <cmath>

Bailout g_bailout_test{}; // test used for determining bailout
int (*g_bailout_float)(){};
int (*g_bailout_bignum)(){};
int (*g_bailout_bigfloat)(){};

static int fp_mod_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_real_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_x >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_imag_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_y >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_or_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_x >= g_magnitude_limit || g_temp_sqr_y >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_and_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_x >= g_magnitude_limit && g_temp_sqr_y >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_manh_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    double manh_mag = std::abs(g_new_z.x) + std::abs(g_new_z.y);
    if ((manh_mag * manh_mag) >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_manr_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    double manr_mag = g_new_z.x + g_new_z.y; // don't need abs() since we square it next
    if ((manr_mag * manr_mag) >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

// --------------------------------------------------------------------
//    Bignumber Bailout Routines
// --------------------------------------------------------------------

// Note:
// No need to set magnitude
// as color schemes that need it calculate it later.

static int bn_mod_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    add_bn(g_bn_tmp, g_tmp_sqr_x_bn+g_shift_factor, g_tmp_sqr_y_bn+g_shift_factor);

    long long_magnitude = bn_to_int(g_bn_tmp);  // works with any fractal type
    if (long_magnitude >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

static int bn_real_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    long long_temp_sqr_x = bn_to_int(g_tmp_sqr_x_bn + g_shift_factor);
    if (long_temp_sqr_x >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

static int bn_imag_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    long long_temp_sqr_y = bn_to_int(g_tmp_sqr_y_bn + g_shift_factor);
    if (long_temp_sqr_y >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

static int bn_or_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    long long_temp_sqr_x = bn_to_int(g_tmp_sqr_x_bn + g_shift_factor);
    long long_temp_sqr_y = bn_to_int(g_tmp_sqr_y_bn + g_shift_factor);
    if (long_temp_sqr_x >= (long)g_magnitude_limit || long_temp_sqr_y >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

static int bn_and_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    long long_temp_sqr_x = bn_to_int(g_tmp_sqr_x_bn + g_shift_factor);
    long long_temp_sqr_y = bn_to_int(g_tmp_sqr_y_bn + g_shift_factor);
    if (long_temp_sqr_x >= (long)g_magnitude_limit && long_temp_sqr_y >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

static int bn_manh_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    // note: in next five lines, g_old_z_bn is just used as a temporary variable
    abs_bn(g_old_z_bn.x, g_new_z_bn.x);
    abs_bn(g_old_z_bn.y, g_new_z_bn.y);
    add_bn(g_bn_tmp, g_old_z_bn.x, g_old_z_bn.y);
    square_bn(g_old_z_bn.x, g_bn_tmp);
    long long_temp_mag = bn_to_int(g_old_z_bn.x + g_shift_factor);
    if (long_temp_mag >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

static int bn_manr_bailout()
{
    square_bn(g_tmp_sqr_x_bn, g_new_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_new_z_bn.y);
    add_bn(g_bn_tmp, g_new_z_bn.x, g_new_z_bn.y); // don't need abs since we square it next
    // note: in next two lines, g_old_z_bn is just used as a temporary variable
    square_bn(g_old_z_bn.x, g_bn_tmp);
    long long_temp_mag = bn_to_int(g_old_z_bn.x + g_shift_factor);
    if (long_temp_mag >= (long)g_magnitude_limit)
    {
        return 1;
    }
    copy_bn(g_old_z_bn.x, g_new_z_bn.x);
    copy_bn(g_old_z_bn.y, g_new_z_bn.y);
    return 0;
}

static int bf_mod_bailout()
{
    int saved = save_stack();
    BigFloat tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    add_bf(g_bf_tmp, g_tmp_sqr_x_bf, g_tmp_sqr_y_bf);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_bf_tmp, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }
    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

static int bf_real_bailout()
{
    int saved = save_stack();
    BigFloat tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_tmp_sqr_x_bf, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}


static int bf_imag_bailout()
{
    int saved = save_stack();
    BigFloat tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_tmp_sqr_y_bf, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

static int bf_or_bailout()
{
    int saved = save_stack();
    BigFloat tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_tmp_sqr_x_bf, tmp1) > 0 || cmp_bf(g_tmp_sqr_y_bf, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

static int bf_and_bailout()
{
    int saved = save_stack();
    BigFloat tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_tmp_sqr_x_bf, tmp1) > 0 && cmp_bf(g_tmp_sqr_y_bf, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

static int bf_manh_bailout()
{
    int saved = save_stack();
    BigFloat tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    // note: in next five lines, g_old_z_bf is just used as a temporary variable
    abs_bf(g_old_z_bf.x, g_new_z_bf.x);
    abs_bf(g_old_z_bf.y, g_new_z_bf.y);
    add_bf(g_bf_tmp, g_old_z_bf.x, g_old_z_bf.y);
    square_bf(g_old_z_bf.x, g_bf_tmp);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_old_z_bf.x, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

static int bf_manr_bailout()
{
    int saved = save_stack();
    BigFloat tmp1 = alloc_stack(g_bf_length + 2);

    square_bf(g_tmp_sqr_x_bf, g_new_z_bf.x);
    square_bf(g_tmp_sqr_y_bf, g_new_z_bf.y);
    add_bf(g_bf_tmp, g_new_z_bf.x, g_new_z_bf.y); // don't need abs since we square it next
    // note: in next two lines, g_old_z_bf is just used as a temporary variable
    square_bf(g_old_z_bf.x, g_bf_tmp);
    float_to_bf(tmp1, g_magnitude_limit);
    if (cmp_bf(g_old_z_bf.x, tmp1) > 0)
    {
        restore_stack(saved);
        return 1;
    }

    copy_bf(g_old_z_bf.x, g_new_z_bf.x);
    copy_bf(g_old_z_bf.y, g_new_z_bf.y);
    restore_stack(saved);
    return 0;
}

void set_bailout_formula(Bailout test)
{
    switch (test)
    {
    case Bailout::MOD:
        g_bailout_float = fp_mod_bailout;
        g_bailout_bignum = bn_mod_bailout;
        g_bailout_bigfloat = bf_mod_bailout;
        break;

    case Bailout::REAL:
        g_bailout_float = fp_real_bailout;
        g_bailout_bignum = bn_real_bailout;
        g_bailout_bigfloat = bf_real_bailout;
        break;

    case Bailout::IMAG:
        g_bailout_float = fp_imag_bailout;
        g_bailout_bignum = bn_imag_bailout;
        g_bailout_bigfloat = bf_imag_bailout;
        break;

    case Bailout::OR:
        g_bailout_float = fp_or_bailout;
        g_bailout_bignum = bn_or_bailout;
        g_bailout_bigfloat = bf_or_bailout;
        break;

    case Bailout::AND:
        g_bailout_float = fp_and_bailout;
        g_bailout_bignum = bn_and_bailout;
        g_bailout_bigfloat = bf_and_bailout;
        break;

    case Bailout::MANH:
        g_bailout_float = fp_manh_bailout;
        g_bailout_bignum = bn_manh_bailout;
        g_bailout_bigfloat = bf_manh_bailout;
        break;

    case Bailout::MANR:
        g_bailout_float = fp_manr_bailout;
        g_bailout_bignum = bn_manr_bailout;
        g_bailout_bigfloat = bf_manr_bailout;
        break;
    }
}
