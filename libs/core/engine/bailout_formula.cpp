// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>

#include <engine/calcfrac.h>
#include <engine/fractalb.h>
#include <engine/fractals.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <math/BigNum.h>

#include <cmath>
#include <limits>
#include <stdexcept>

using namespace id::math;

namespace id::engine
{

Bailout g_bailout_test{}; // test used for determining bailout
int (*g_bailout_float)(){};
int (*g_bailout_bignum)(){};
int (*g_bailout_bigfloat)(){};

bool bailout_reached(const Bailout test, const double x, const double y, const double limit, const bool strict)
{
    const auto reached = [limit, strict](const double metric) { return strict ? metric > limit : metric >= limit; };
    switch (test)
    {
    case Bailout::MOD:
        return reached(sqr(x) + sqr(y));
    case Bailout::REAL:
        return reached(sqr(x));
    case Bailout::IMAG:
        return reached(sqr(y));
    case Bailout::OR:
        return reached(sqr(x)) || reached(sqr(y));
    case Bailout::AND:
        return reached(sqr(x)) && reached(sqr(y));
    case Bailout::MANH:
        return reached(sqr(std::abs(x) + std::abs(y)));
    case Bailout::MANR:
        return reached(sqr(x + y));
    }
    throw std::logic_error("Unknown bailout test");
}

static void get_sqr_mag()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
}

static int fp_mod_bailout()
{
    get_sqr_mag();
    if (bailout_reached(Bailout::MOD, g_new_z.x, g_new_z.y, g_magnitude_limit))
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_real_bailout()
{
    get_sqr_mag();
    if (bailout_reached(Bailout::REAL, g_new_z.x, g_new_z.y, g_magnitude_limit))
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_imag_bailout()
{
    get_sqr_mag();
    if (bailout_reached(Bailout::IMAG, g_new_z.x, g_new_z.y, g_magnitude_limit))
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_or_bailout()
{
    get_sqr_mag();
    if (bailout_reached(Bailout::OR, g_new_z.x, g_new_z.y, g_magnitude_limit))
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_and_bailout()
{
    get_sqr_mag();
    if (bailout_reached(Bailout::AND, g_new_z.x, g_new_z.y, g_magnitude_limit))
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_manh_bailout()
{
    get_sqr_mag();
    if (bailout_reached(Bailout::MANH, g_new_z.x, g_new_z.y, g_magnitude_limit))
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static int fp_manr_bailout()
{
    get_sqr_mag();
    if (bailout_reached(Bailout::MANR, g_new_z.x, g_new_z.y, g_magnitude_limit))
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

static bool bn_bailout_reached(const BigNum &metric)
{
    // Keep bn_to_int's unsigned 1/2-byte and signed 4-byte ranges,
    // including wrapped metrics, but compare their fractional bytes too.
    const bool signed_metric = g_int_length == 4;
    const double upper = std::ldexp(1.0, 8 * g_int_length - (signed_metric ? 1 : 0));
    if (g_magnitude_limit < (signed_metric ? -upper : 0))
    {
        return true;
    }
    if (g_magnitude_limit >= upper)
    {
        return false;
    }

    double threshold = g_magnitude_limit;
    const int fractional_bits = 8 * (g_bn_length - g_int_length);
    if (threshold != 0 && fractional_bits < std::numeric_limits<double>::digits - std::ilogb(threshold))
    {
        // The metric is on this fixed-point grid. Round the threshold up
        // so metric >= threshold also holds when the limit lies between
        // two representable values.
        threshold = std::ldexp(std::ceil(std::ldexp(threshold, fractional_bits)), -fractional_bits);
        if (threshold >= upper)
        {
            return false;
        }
    }
    BigStackSaver saved;
    const BigNum limit{bn_arena(BigLifetime::TEMPORARY), threshold};
    if (!signed_metric && metric.is_negative() != limit.is_negative())
    {
        return metric.is_negative();
    }
    return metric >= limit;
}

static int bn_mod_bailout()
{
    g_tmp_sqr_x_bn = g_new_z_bn.x.sqr();
    g_tmp_sqr_y_bn = g_new_z_bn.y.sqr();
    g_bn_tmp = g_tmp_sqr_x_bn + g_tmp_sqr_y_bn;

    if (bn_bailout_reached(g_bn_tmp))
    {
        return 1;
    }
    g_old_z_bn.x = g_new_z_bn.x;
    g_old_z_bn.y = g_new_z_bn.y;
    return 0;
}

static int bn_real_bailout()
{
    g_tmp_sqr_x_bn = g_new_z_bn.x.sqr();
    g_tmp_sqr_y_bn = g_new_z_bn.y.sqr();
    if (bn_bailout_reached(g_tmp_sqr_x_bn))
    {
        return 1;
    }
    g_old_z_bn.x = g_new_z_bn.x;
    g_old_z_bn.y = g_new_z_bn.y;
    return 0;
}

static int bn_imag_bailout()
{
    g_tmp_sqr_x_bn = g_new_z_bn.x.sqr();
    g_tmp_sqr_y_bn = g_new_z_bn.y.sqr();
    if (bn_bailout_reached(g_tmp_sqr_y_bn))
    {
        return 1;
    }
    g_old_z_bn.x = g_new_z_bn.x;
    g_old_z_bn.y = g_new_z_bn.y;
    return 0;
}

static int bn_or_bailout()
{
    g_tmp_sqr_x_bn = g_new_z_bn.x.sqr();
    g_tmp_sqr_y_bn = g_new_z_bn.y.sqr();
    if (bn_bailout_reached(g_tmp_sqr_x_bn) || bn_bailout_reached(g_tmp_sqr_y_bn))
    {
        return 1;
    }
    g_old_z_bn.x = g_new_z_bn.x;
    g_old_z_bn.y = g_new_z_bn.y;
    return 0;
}

static int bn_and_bailout()
{
    g_tmp_sqr_x_bn = g_new_z_bn.x.sqr();
    g_tmp_sqr_y_bn = g_new_z_bn.y.sqr();
    if (bn_bailout_reached(g_tmp_sqr_x_bn) && bn_bailout_reached(g_tmp_sqr_y_bn))
    {
        return 1;
    }
    g_old_z_bn.x = g_new_z_bn.x;
    g_old_z_bn.y = g_new_z_bn.y;
    return 0;
}

static int bn_manh_bailout()
{
    BigStackSaver saved;
    BigNum metric{bn_arena(BigLifetime::TEMPORARY)};

    g_tmp_sqr_x_bn = g_new_z_bn.x.sqr();
    g_tmp_sqr_y_bn = g_new_z_bn.y.sqr();
    const BigNum abs_x = g_new_z_bn.x.abs();
    const BigNum abs_y = g_new_z_bn.y.abs();
    g_bn_tmp = abs_x + abs_y;
    metric = g_bn_tmp.sqr();
    if (bn_bailout_reached(metric))
    {
        return 1;
    }
    g_old_z_bn.x = g_new_z_bn.x;
    g_old_z_bn.y = g_new_z_bn.y;
    return 0;
}

static int bn_manr_bailout()
{
    BigStackSaver saved;
    BigNum metric{bn_arena(BigLifetime::TEMPORARY)};

    g_tmp_sqr_x_bn = g_new_z_bn.x.sqr();
    g_tmp_sqr_y_bn = g_new_z_bn.y.sqr();
    g_bn_tmp = g_new_z_bn.x + g_new_z_bn.y; // don't need abs since we square it next
    metric = g_bn_tmp.sqr();
    if (bn_bailout_reached(metric))
    {
        return 1;
    }
    g_old_z_bn.x = g_new_z_bn.x;
    g_old_z_bn.y = g_new_z_bn.y;
    return 0;
}

static int bf_mod_bailout()
{
    BigStackSaver saved;
    BigFloat tmp1{bf_arena(BigLifetime::TEMPORARY)};

    g_tmp_sqr_x_bf = sqr(g_new_z_bf.x);
    g_tmp_sqr_y_bf = sqr(g_new_z_bf.y);
    g_bf_tmp = g_tmp_sqr_x_bf + g_tmp_sqr_y_bf;
    tmp1 = g_magnitude_limit;
    if (g_bf_tmp >= tmp1)
    {
        return 1;
    }
    g_old_z_bf.x = g_new_z_bf.x;
    g_old_z_bf.y = g_new_z_bf.y;
    return 0;
}

static int bf_real_bailout()
{
    BigStackSaver saved;
    BigFloat tmp1{bf_arena(BigLifetime::TEMPORARY)};

    g_tmp_sqr_x_bf = sqr(g_new_z_bf.x);
    g_tmp_sqr_y_bf = sqr(g_new_z_bf.y);
    tmp1 = g_magnitude_limit;
    if (g_tmp_sqr_x_bf >= tmp1)
    {
        return 1;
    }

    g_old_z_bf.x = g_new_z_bf.x;
    g_old_z_bf.y = g_new_z_bf.y;
    return 0;
}


static int bf_imag_bailout()
{
    BigStackSaver saved;
    BigFloat tmp1{bf_arena(BigLifetime::TEMPORARY)};

    g_tmp_sqr_x_bf = sqr(g_new_z_bf.x);
    g_tmp_sqr_y_bf = sqr(g_new_z_bf.y);
    tmp1 = g_magnitude_limit;
    if (g_tmp_sqr_y_bf >= tmp1)
    {
        return 1;
    }

    g_old_z_bf.x = g_new_z_bf.x;
    g_old_z_bf.y = g_new_z_bf.y;
    return 0;
}

static int bf_or_bailout()
{
    BigStackSaver saved;
    BigFloat tmp1{bf_arena(BigLifetime::TEMPORARY)};

    g_tmp_sqr_x_bf = sqr(g_new_z_bf.x);
    g_tmp_sqr_y_bf = sqr(g_new_z_bf.y);
    tmp1 = g_magnitude_limit;
    if (g_tmp_sqr_x_bf >= tmp1 || g_tmp_sqr_y_bf >= tmp1)
    {
        return 1;
    }

    g_old_z_bf.x = g_new_z_bf.x;
    g_old_z_bf.y = g_new_z_bf.y;
    return 0;
}

static int bf_and_bailout()
{
    BigStackSaver saved;
    BigFloat tmp1{bf_arena(BigLifetime::TEMPORARY)};

    g_tmp_sqr_x_bf = sqr(g_new_z_bf.x);
    g_tmp_sqr_y_bf = sqr(g_new_z_bf.y);
    tmp1 = g_magnitude_limit;
    if (g_tmp_sqr_x_bf >= tmp1 && g_tmp_sqr_y_bf >= tmp1)
    {
        return 1;
    }

    g_old_z_bf.x = g_new_z_bf.x;
    g_old_z_bf.y = g_new_z_bf.y;
    return 0;
}

static int bf_manh_bailout()
{
    BigStackSaver saved;
    BigFloat tmp1{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat abs_x{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat abs_y{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat metric{bf_arena(BigLifetime::TEMPORARY)};

    g_tmp_sqr_x_bf = sqr(g_new_z_bf.x);
    g_tmp_sqr_y_bf = sqr(g_new_z_bf.y);
    abs_bf(abs_x, g_new_z_bf.x);
    abs_bf(abs_y, g_new_z_bf.y);
    g_bf_tmp = abs_x + abs_y;
    metric = sqr(g_bf_tmp);
    tmp1 = g_magnitude_limit;
    if (metric >= tmp1)
    {
        return 1;
    }

    g_old_z_bf.x = g_new_z_bf.x;
    g_old_z_bf.y = g_new_z_bf.y;
    return 0;
}

static int bf_manr_bailout()
{
    BigStackSaver saved;
    BigFloat tmp1{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat metric{bf_arena(BigLifetime::TEMPORARY)};

    g_tmp_sqr_x_bf = sqr(g_new_z_bf.x);
    g_tmp_sqr_y_bf = sqr(g_new_z_bf.y);
    g_bf_tmp = g_new_z_bf.x + g_new_z_bf.y; // don't need abs since we square it next
    metric = sqr(g_bf_tmp);
    tmp1 = g_magnitude_limit;
    if (metric >= tmp1)
    {
        return 1;
    }

    g_old_z_bf.x = g_new_z_bf.x;
    g_old_z_bf.y = g_new_z_bf.y;
    return 0;
}

void set_bailout_formula(const Bailout test)
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

} // namespace id::engine
