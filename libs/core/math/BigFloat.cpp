// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers. (C) 1994-95, Wesley B. Loewer
//
#include <math/BigFloat.h>

#include "BigStorage.h"

#include <math/big_init.h>
#include <math/BigNum.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace id::misc;

namespace id::math
{
namespace
{

using BigFloat10 = Byte *;

/********************************************************************/
// bftostring() - converts a bigfloat into a scientific notation string
// dec - decimal places, 0 for max
//   r - bigfloat
//   will convert to a floating point notation
//   SIDE-EFFECT: the bigfloat, r, is destroyed.
//                Copy it first if necessary.
// USES: g_bf_tmp1 - g_bf_tmp2
/********************************************************************/

std::string unsafe_bf_to_string(BigFloatPtr r, const int dec)
{
    LDouble value = bf_to_float(r);
    if (value == 0.0)
    {
        return "0.0";
    }

    std::vector<Byte> storage(static_cast<std::size_t>(dec == 0 ? g_decimals : dec) + 4);
    BigFloat10 decimal{storage.data()};
    unsafe_bf_to_bf10(decimal, dec, r);
    int power = static_cast<S16>(BIG_ACCESS16(decimal + dec + 2)); // where the exponent is stored
    if (power > -4 && power < 6)                                   // tinker with this
    {
        return bf10_to_string_f(decimal, dec);
    }
    return bf10_to_string_e(decimal, dec);
}

std::string unsafe_bf_to_string_e(BigFloatPtr r, const int dec)
{
    LDouble value = bf_to_float(r);
    if (value == 0.0)
    {
        return "0.0";
    }

    std::vector<Byte> storage(static_cast<std::size_t>(dec == 0 ? g_decimals : dec) + 4);
    BigFloat10 decimal{storage.data()};
    unsafe_bf_to_bf10(decimal, dec, r);
    return bf10_to_string_e(decimal, dec);
}

std::string unsafe_bf_to_string_f(BigFloatPtr r, const int dec)
{
    LDouble value = bf_to_float(r);
    if (value == 0.0)
    {
        return "0.0";
    }

    std::vector<Byte> storage(static_cast<std::size_t>(dec == 0 ? g_decimals : dec) + 4);
    BigFloat10 decimal{storage.data()};
    unsafe_bf_to_bf10(decimal, dec, r);
    return bf10_to_string_f(decimal, dec);
}

void validate_buffer(BigFloatConstPtr buffer, const std::size_t size, const std::size_t required)
{
    if (buffer == nullptr || size < required)
    {
        throw std::invalid_argument("BigFloat requires a complete legacy value buffer");
    }
}

BigFloatPtr binary_operation(BigFloatPtr result, BigFloatConstPtr left, BigFloatConstPtr right,
    BigFloatPtr (*operation)(BigFloatPtr, BigFloatPtr, BigFloatPtr))
{
    copy_bf(g_bf_tmp_copy1.data(), left);
    copy_bf(g_bf_tmp_copy2.data(), right);
    return operation(result, g_bf_tmp_copy1.data(), g_bf_tmp_copy2.data());
}

BigFloatPtr compound_operation(
    BigFloatPtr result, BigFloatConstPtr right, BigFloatPtr (*operation)(BigFloatPtr, BigFloatPtr))
{
    copy_bf(g_bf_tmp_copy1.data(), right);
    return operation(result, g_bf_tmp_copy1.data());
}

BigFloatPtr multiply(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    if (is_bf_zero(n1) || is_bf_zero(n2))
    {
        return clear_bf(r);
    }
    BigFloatProduct product{bf_arena(BigLifetime::TEMPORARY)};
    multiply_wide(product.data(), n1, n2);
    const BigFloat canonical{std::move(product)};
    copy_bf(r, canonical.data());
    return r;
}

BigFloatPtr square_value(BigFloatPtr r, BigFloatPtr n)
{
    if (is_bf_zero(n))
    {
        return clear_bf(r);
    }
    BigFloatProduct product{bf_arena(BigLifetime::TEMPORARY)};
    square_wide(product.data(), n);
    const BigFloat canonical{std::move(product)};
    copy_bf(r, canonical.data());
    return r;
}

BigFloatPtr logarithm(BigFloatPtr r, BigFloatPtr n)
{
    int almost_match = 0;

    // use Newton's recursive method for zeroing in on ln(n): r=r+n*exp(-r)-1

    if (is_bf_neg(n) || is_bf_zero(n))
    {
        // error, return largest neg value
        max_bf(r);
        neg_a_bf(r);
        return r;
    }

    LDouble f = bf_to_float(n);
    f = std::log(f); // approximate ln(x)
    // no need to check overflow
    // appears to be ok, do ln

    // With Newton's Method, there is no need to calculate all the digits
    // every time.  The precision approximately doubles each iteration.
    // Save original values.
    int orig_bf_length = g_bf_length;
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    int orig_r_bf_length = g_r_bf_length;
    BigFloatPtr orig_r = r;
    BigFloatPtr orig_n = n;
    BigFloatPtr orig_bf_tmp5 = g_bf_tmp5.data();
    BigFloatPtr bf_tmp5 = orig_bf_tmp5;

    // calculate new starting values
    g_bn_length = g_int_length + static_cast<int>(LDBL_DIG / LOG10_256) + 1; // round up
    g_bn_length = std::min(g_bn_length, orig_bn_length);
    calc_lengths();

    // adjust pointers
    r = orig_r + orig_bf_length - g_bf_length;
    bf_tmp5 = orig_bf_tmp5 + orig_bf_length - g_bf_length;

    float_to_bf(r, f);           // start with approximate ln
    neg_a_bf(r);                 // -r
    copy_bf(bf_tmp5, r);         // -r

    for (int i = 0; i < 25; i++) // safety net, this shouldn't ever be needed
    {
        // adjust lengths
        g_bn_length <<= 1; // double precision
        g_bn_length = std::min(g_bn_length, orig_bn_length);
        calc_lengths();
        r = orig_r + orig_bf_length - g_bf_length;
        n = orig_n + orig_bf_length - g_bf_length;
        bf_tmp5 = orig_bf_tmp5 + orig_bf_length - g_bf_length;

        exp_bf(g_bf_tmp6.data(), r, g_bf_tmp1.data(), g_bf_tmp2.data(), g_bf_tmp3.data()); // exp(-r)
        unsafe_mult_bf(g_bf_tmp2.data(), g_bf_tmp6.data(), n); // n*exp(-r)
        int_to_bf(g_bf_tmp4.data(), 1);
        unsafe_sub_a_bf(g_bf_tmp2.data(), g_bf_tmp4.data());   // n*exp(-r) - 1
        unsafe_sub_a_bf(r, g_bf_tmp2.data());                  // -r - (n*exp(-r) - 1)
        if (g_bf_length == orig_bf_length)
        {
            const int comp = std::abs(cmp_bf(r, bf_tmp5));
            if (comp < 8)                 // if match or almost match
            {
                if (comp < 4              // perfect or near perfect match
                    || almost_match == 1) // close enough for 2nd time
                {
                    break;
                }
                // this is the first time they almost matched
                almost_match++;
            }
        }
        copy_bf(bf_tmp5, r); // -r
    }

    // restore original values
    g_bf_length = orig_bf_length;
    g_bn_length = orig_bn_length;
    g_padding = orig_padding;
    g_r_length = orig_r_length;
    g_shift_factor = orig_shift_factor;
    g_r_bf_length = orig_r_bf_length;
    r = orig_r;
    bf_tmp5 = orig_bf_tmp5;

    neg_a_bf(r); // -(-r)
    return r;
}

BigFloatPtr sine_cosine(BigFloatPtr s, BigFloatPtr c, BigFloatPtr n, [[maybe_unused]] BigFloatPtr pi)
{
    U16 fact = 2;
    S16 *test_exp = reinterpret_cast<S16 *>(g_bf_tmp1.data() + g_bf_length);
    S16 *c_exp = reinterpret_cast<S16 *>(c + g_bf_length);
    S16 *s_exp = reinterpret_cast<S16 *>(s + g_bf_length);

#ifndef CALCULATING_BIG_PI
    // assure range 0 <= x < pi/4

    if (is_bf_zero(n))
    {
        clear_bf(s);     // sin(0) = 0
        int_to_bf(c, 1); // cos(0) = 1
        return s;
    }

    bool sign_sin = false;
    if (is_bf_neg(n))
    {
        sign_sin = !sign_sin; // sin(-x) = -sin(x), odd; cos(-x) = cos(x), even
        neg_a_bf(n);
    }
    // n >= 0

    double_bf(g_bf_tmp1.data(), pi); // 2*pi
    // this could be done with remainders, but it would probably be slower
    while (cmp_bf(n, g_bf_tmp1.data()) >= 0) // while n >= 2*pi
    {
        copy_bf(g_bf_tmp2.data(), g_bf_tmp1.data());
        unsafe_sub_a_bf(n, g_bf_tmp2.data());
    }
    // 0 <= n < 2*pi

    bool sign_cos = false;
    copy_bf(g_bf_tmp1.data(), pi);        // pi
    if (cmp_bf(n, g_bf_tmp1.data()) >= 0) // if n >= pi
    {
        unsafe_sub_a_bf(n, g_bf_tmp1.data());
        sign_sin = !sign_sin;
        sign_cos = !sign_cos;
    }
    // 0 <= n < pi

    half_bf(g_bf_tmp1.data(), pi);       // pi/2
    if (cmp_bf(n, g_bf_tmp1.data()) > 0) // if n > pi/2
    {
        copy_bf(g_bf_tmp2.data(), pi);
        unsafe_sub_bf(n, g_bf_tmp2.data(), n);
        sign_cos = !sign_cos;
    }
    // 0 <= n < pi/2

    bool switch_sin_cos = false;
    half_bf(g_bf_tmp1.data(), pi);                            // pi/2
    half_a_bf(g_bf_tmp1.data());                              // pi/4
    if (cmp_bf(n, g_bf_tmp1.data()) > 0)                      // if n > pi/4
    {
        copy_bf(g_bf_tmp2.data(), n);
        half_bf(g_bf_tmp1.data(), pi);                        // pi/2
        unsafe_sub_bf(n, g_bf_tmp1.data(), g_bf_tmp2.data()); // pi/2 - n
        switch_sin_cos = !switch_sin_cos;
    }
    // 0 <= n < pi/4

    // this looks redundant, but n could now be zero when it wasn't before
    if (is_bf_zero(n))
    {
        clear_bf(s);     // sin(0) = 0
        int_to_bf(c, 1); // cos(0) = 1
        return s;
    }

    // at this point, the double angle trig identities could be used as many
    // times as desired to reduce the range to pi/8, pi/16, etc...  Each time
    // the range is cut in half, the number of iterations required is reduced
    // by "quite a bit."  It's just a matter of testing to see what gives the
    // optimal results.
    // halves = g_bf_length / 10;  // this is experimental
    const int halves = 1;
    for (int i = 0; i < halves; i++)
    {
        half_a_bf(n);
    }
#endif

    // use Taylor Series (very slow convergence)
    copy_bf(s, n);         // start with s=n
    int_to_bf(c, 1);       // start with c=1
    copy_bf(g_bf_tmp1.data(), n); // the current x^n/n!
    bool sin_done = false;
    bool cos_done = false;
    bool k = false;
    do
    {
        // even terms for cosine
        copy_bf(g_bf_tmp2.data(), g_bf_tmp1.data());
        unsafe_mult_bf(g_bf_tmp1.data(), g_bf_tmp2.data(), n);
        div_a_bf_int(g_bf_tmp1.data(), fact++);
        if (!cos_done)
        {
            cos_done = BIG_ACCESS_S16(test_exp) < BIG_ACCESS_S16(c_exp) - (g_bf_length - 2); // too small to register
            if (!cos_done)
            {
                if (k) // alternate between adding and subtracting
                {
                    unsafe_add_a_bf(c, g_bf_tmp1.data());
                }
                else
                {
                    unsafe_sub_a_bf(c, g_bf_tmp1.data());
                }
            }
        }

        // odd terms for sine
        copy_bf(g_bf_tmp2.data(), g_bf_tmp1.data());
        unsafe_mult_bf(g_bf_tmp1.data(), g_bf_tmp2.data(), n);
        div_a_bf_int(g_bf_tmp1.data(), fact++);
        if (!sin_done)
        {
            sin_done = BIG_ACCESS_S16(test_exp) < BIG_ACCESS_S16(s_exp) - (g_bf_length - 2); // too small to register
            if (!sin_done)
            {
                if (k) // alternate between adding and subtracting
                {
                    unsafe_add_a_bf(s, g_bf_tmp1.data());
                }
                else
                {
                    unsafe_sub_a_bf(s, g_bf_tmp1.data());
                }
            }
        }
        k = !k;           // toggle
#if defined(CALCULATING_BIG_PI) && !defined(_WIN32)
        std::printf("."); // lets you know it's doing something
#endif
    } while (!cos_done || !sin_done);

#ifndef CALCULATING_BIG_PI
    // now need to undo what was done by cutting angles in half
    for (int i = 0; i < halves; i++)
    {
        unsafe_mult_bf(g_bf_tmp2.data(), s, c); // no need for safe mult
        double_bf(s, g_bf_tmp2.data());         // sin(2x) = 2*sin(x)*cos(x)
        square_value(g_bf_tmp2.data(), c);
        double_a_bf(g_bf_tmp2.data());
        int_to_bf(g_bf_tmp1.data(), 1);
        unsafe_sub_bf(c, g_bf_tmp2.data(), g_bf_tmp1.data()); // cos(2x) = 2*cos(x)*cos(x) - 1
    }

    if (switch_sin_cos)
    {
        copy_bf(g_bf_tmp1.data(), s);
        copy_bf(s, c);
        copy_bf(c, g_bf_tmp1.data());
    }
    if (sign_sin)
    {
        neg_a_bf(s);
    }
    if (sign_cos)
    {
        neg_a_bf(c);
    }
#endif

    return s; // return sine I guess
}

BigFloatPtr arctangent(BigFloatPtr r, BigFloatPtr n)
{
    int almost_match = 0;
    bool sign_flag = false;

    // use Newton's recursive method for zeroing in on atan(n): r=r-cos(r)(sin(r)-n*cos(r))

    if (is_bf_neg(n))
    {
        sign_flag = true;
        neg_a_bf(n);
    }

    // If n is very large, atanl() won't give enough decimal places to be a
    // good enough initial guess for Newton's Method.  If it is larger than
    // say, 1, atan(n) = pi/2 - acot(n) = pi/2 - atan(1/n).

    LDouble f = bf_to_float(n);
    const bool large_arg = f > 1.0;
    if (large_arg)
    {
        unsafe_inv_bf(g_bf_tmp3.data(), n, g_bf_tmp1.data(), g_bf_tmp2.data());
        copy_bf(n, g_bf_tmp3.data());
        f = bf_to_float(n);
    }

    clear_bf(g_bf_tmp3.data()); // not really necessary, but makes things more consistent

    // With Newton's Method, there is no need to calculate all the digits
    // every time.  The precision approximately doubles each iteration.
    // Save original values.
    int orig_bf_length = g_bf_length;
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    int orig_r_bf_length = g_r_bf_length;
    BigFloatPtr orig_bf_pi = g_bf_pi.data();
    BigFloatPtr bf_pi = orig_bf_pi;
    BigFloatPtr orig_r = r;
    BigFloatPtr orig_n = n;
    BigFloatPtr orig_bf_tmp3 = g_bf_tmp3.data();
    BigFloatPtr bf_tmp3 = orig_bf_tmp3;

    // calculate new starting values
    g_bn_length = g_int_length + static_cast<int>(LDBL_DIG / LOG10_256) + 1; // round up
    g_bn_length = std::min(g_bn_length, orig_bn_length);
    calc_lengths();

    // adjust pointers
    r = orig_r + orig_bf_length - g_bf_length;
    bf_pi = orig_bf_pi + orig_bf_length - g_bf_length;
    bf_tmp3 = orig_bf_tmp3 + orig_bf_length - g_bf_length;

    f = atanl(f); // approximate arctangent
    // no need to check overflow

    float_to_bf(r, f);           // start with approximate atan
    copy_bf(bf_tmp3, r);

    for (int i = 0; i < 25; i++) // safety net, this shouldn't ever be needed
    {
        // adjust lengths
        g_bn_length <<= 1; // double precision
        g_bn_length = std::min(g_bn_length, orig_bn_length);
        calc_lengths();
        r = orig_r + orig_bf_length - g_bf_length;
        n = orig_n + orig_bf_length - g_bf_length;
        bf_pi = orig_bf_pi + orig_bf_length - g_bf_length;
        bf_tmp3 = orig_bf_tmp3 + orig_bf_length - g_bf_length;

#if defined(CALCULATING_BIG_PI) && !defined(_WIN32)
        std::printf("\natan() loop #%i, g_bf_length=%i\nsincos() loops\n", i, g_bf_length);
#endif
        sine_cosine(g_bf_tmp4.data(), g_bf_tmp5.data(), bf_tmp3, bf_pi);      // sin(r), cos(r)
        copy_bf(bf_tmp3, r);                                                  // restore bf_tmp3 from sincos_bf()
        copy_bf(g_bf_tmp1.data(), g_bf_tmp5.data());
        unsafe_mult_bf(g_bf_tmp2.data(), n, g_bf_tmp1.data());                // n*cos(r)
        unsafe_sub_a_bf(g_bf_tmp4.data(), g_bf_tmp2.data());                  // sin(r) - n*cos(r)
        unsafe_mult_bf(g_bf_tmp1.data(), g_bf_tmp5.data(), g_bf_tmp4.data()); // cos(r) * (sin(r) - n*cos(r))
        copy_bf(bf_tmp3, r);
        unsafe_sub_a_bf(r, g_bf_tmp1.data());                                 // r - cos(r) * (sin(r) - n*cos(r))
#if defined(CALCULATING_BIG_PI) && !defined(_WIN32)
        putchar('\n');
        {
            const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
            BigArena dump_arena{size, 2, reduce_bf_product};
            bf_hex_dump(BigFloat::from_legacy(dump_arena, r, size));
        }
#endif
        if (g_bf_length == orig_bf_length)
        {
            const int comp = std::abs(cmp_bf(r, bf_tmp3));
            if (comp < 8) // if match or almost match
            {
#if defined(CALCULATING_BIG_PI) && !defined(_WIN32)
                std::printf("atan() loop comp=%i\n", comp);
#endif
                if (comp < 4              // perfect or near perfect match
                    || almost_match == 1) // close enough for 2nd time
                {
                    break;
                }
                // this is the first time they almost matched
                almost_match++;
            }
#if defined(CALCULATING_BIG_PI) && !defined(_WIN32)
            else
            {
                std::printf("atan() loop comp=%i\n", comp);
            }
#endif
        }

        copy_bf(bf_tmp3, r); // make a copy for later comparison
    }

    // restore original values
    g_bf_length = orig_bf_length;
    g_bn_length = orig_bn_length;
    g_padding = orig_padding;
    g_r_length = orig_r_length;
    g_shift_factor = orig_shift_factor;
    g_r_bf_length = orig_r_bf_length;
    bf_pi = orig_bf_pi;
    r = orig_r;
    bf_tmp3 = orig_bf_tmp3;

    if (large_arg)
    {
        half_bf(bf_tmp3, bf_pi);   // pi/2
        compound_operation(bf_tmp3, r, unsafe_sub_a_bf); // pi/2 - atan(1/n)
        copy_bf(r, bf_tmp3);
    }

    if (sign_flag)
    {
        neg_a_bf(r);
    }
    return r;
}

BigFloatPtr arctangent2(BigFloatPtr r, BigFloatPtr ny, BigFloatPtr nx)
{
    const int sign_x = sign_bf(nx);
    const int sign_y = sign_bf(ny);

    if (sign_y == 0)
    {
        if (sign_x < 0)
        {
            copy_bf(r, g_bf_pi.data()); // negative x axis, 180 deg
        }
        else                     // sign_x >= 0    positive x axis, 0
        {
            clear_bf(r);
        }
        return r;
    }
    if (sign_x == 0)
    {
        copy_bf(r, g_bf_pi.data()); // y axis
        half_a_bf(r);        // +90 deg
        if (sign_y < 0)
        {
            neg_a_bf(r);     // -90 deg
        }
        return r;
    }

    if (sign_y < 0)
    {
        neg_a_bf(ny);
    }
    if (sign_x < 0)
    {
        neg_a_bf(nx);
    }
    unsafe_div_bf(g_bf_tmp6.data(), ny, nx, g_bf_tmp1.data(), g_bf_tmp2.data());
    arctangent(r, g_bf_tmp6.data());
    if (sign_x < 0)
    {
        binary_operation(r, g_bf_pi.data(), r, unsafe_sub_bf);
    }
    if (sign_y < 0)
    {
        neg_a_bf(r);
    }
    return r;
}

void validate_scalar(const LDouble value)
{
    if (!std::isfinite(value))
    {
        throw std::invalid_argument("BigFloat requires a finite scalar value");
    }
}

BigArena::Allocation native_storage(BigArena &arena, const LDouble value)
{
    validate_scalar(value);
    auto storage = arena.allocate_single();
    std::memset(storage.data(), 0, arena.block_size());
    float_to_bf(storage.data(), value);
    return storage;
}

} // namespace

void reduce_bf_product(Byte *value)
{
    std::memmove(value, value + g_padding, g_bf_length + 2);
}

BigFloat::BigFloat() noexcept = default;

BigFloat::BigFloat(const LDouble value) :
    BigFloat(bf_arena(BigLifetime::WORKING), value)
{
}

BigFloat::BigFloat(const std::string_view text) :
    BigFloat(bf_arena(BigLifetime::WORKING), text)
{
}

BigFloat::BigFloat(const BigNum &value) :
    BigFloat(bf_arena(BigLifetime::WORKING), value)
{
}

BigFloat::BigFloat(BigFloatProduct &&value) :
    m_storage(std::move(value).release_reduced_storage())
{
}

BigFloat::BigFloat(BigArena &arena) :
    BigFloat(arena.allocate_single())
{
}

BigFloat::BigFloat(BigArena &arena, const LDouble value) :
    m_storage(native_storage(arena, value))
{
}

BigFloat::BigFloat(BigArena &arena, const std::string_view text) :
    BigFloat(arena)
{
    str_to_bf(*this, text);
}

BigFloat::BigFloat(BigArena &arena, const BigNum &value) :
    BigFloat(arena)
{
    value.copy_to_legacy_float(m_storage.data(), encoded_size());
}

BigFloat::BigFloat(const BigFloat &value) :
    m_storage(value.m_storage.clone())
{
}

BigFloat::BigFloat(BigArena &arena, const BigFloat &value) :
    m_storage(value.m_storage.clone(arena))
{
}

BigFloat::BigFloat(BigFloat &&value) noexcept = default;

BigFloat::BigFloat(BigArena::Allocation storage) :
    m_storage(std::move(storage))
{
    std::memset(m_storage.data(), 0, m_storage.blocks() * arena().block_size());
}

BigFloat &BigFloat::operator=(const BigFloat &value)
{
    if (this != &value)
    {
        if (!value.m_storage)
        {
            m_storage.reset();
        }
        else
        {
            if (!m_storage)
            {
                BigArena &destination = m_storage.arena_ptr() != nullptr ? arena() : bf_arena(BigLifetime::WORKING);
                m_storage = destination.allocate_single();
            }
            std::memcpy(m_storage.data(), value.m_storage.data(), encoded_size());
        }
    }
    return *this;
}

BigFloat &BigFloat::operator=(BigFloat &&value)
{
    if (this != &value)
    {
        if (!value.m_storage)
        {
            m_storage.reset();
        }
        else if ((m_storage.arena_ptr() != nullptr ? &arena() : &bf_arena(BigLifetime::WORKING)) == &value.arena())
        {
            m_storage = std::move(value.m_storage);
        }
        else
        {
            *this = static_cast<const BigFloat &>(value);
        }
    }
    return *this;
}

BigFloat &BigFloat::operator=(const LDouble value)
{
    validate_scalar(value);
    if (!m_storage)
    {
        BigArena &destination = m_storage.arena_ptr() != nullptr ? arena() : bf_arena(BigLifetime::WORKING);
        m_storage = destination.allocate_single();
        std::memset(m_storage.data(), 0, arena().block_size());
    }
    float_to_bf(m_storage.data(), value);
    return *this;
}

BigFloat BigFloat::operator-() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING), *this};
    neg_a_bf(result);
    return result;
}

BigFloat &BigFloat::operator+=(const BigFloat &right)
{
    return add_a_bf(*this, right);
}

BigFloat &BigFloat::operator-=(const BigFloat &right)
{
    return sub_a_bf(*this, right);
}

BigFloat &BigFloat::operator*=(const BigFloat &right)
{
    *this = *this * right;
    return *this;
}

BigFloat &BigFloat::operator/=(const BigFloat &right)
{
    *this = *this / right;
    return *this;
}

BigFloat &BigFloat::operator/=(const U16 right)
{
    return div_a_bf_int(*this, right);
}

BigFloat BigFloat::operator+(const BigFloat &right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    add_bf(result, *this, right);
    return result;
}

BigFloat BigFloat::operator-(const BigFloat &right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    sub_bf(result, *this, right);
    return result;
}

BigFloat BigFloat::operator*(const BigFloat &right) const
{
    BigFloatProduct result{bf_arena(BigLifetime::WORKING)};
    binary_operation(result.data(), m_storage.data(), right.m_storage.data(), multiply_wide);
    return BigFloat{std::move(result)};
}

BigFloat BigFloat::operator*(const U16 right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    mult_bf_int(result, *this, right);
    return result;
}

BigFloat BigFloat::operator/(const BigFloat &right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    div_bf(result, *this, right);
    return result;
}

BigFloat BigFloat::abs() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING), *this};
    abs_a_bf(result);
    return result;
}

BigFloat BigFloat::sqr() const
{
    BigFloatProduct result{bf_arena(BigLifetime::WORKING)};
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    square_wide(result.data(), g_bf_tmp_copy1.data());
    return BigFloat{std::move(result)};
}

BigFloat BigFloat::half() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING), *this};
    half_a_bf(result);
    return result;
}

BigFloat BigFloat::log() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    logarithm(result.m_storage.data(), g_bf_tmp_copy1.data());
    return result;
}

BigFloat BigFloat::exp() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    exp_bf(result, *this);
    return result;
}

std::pair<BigFloat, BigFloat> BigFloat::sin_cos() const
{
    std::pair<BigFloat, BigFloat> result{
        BigFloat{bf_arena(BigLifetime::WORKING)}, BigFloat{bf_arena(BigLifetime::WORKING)}};
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    sine_cosine(result.first.m_storage.data(), result.second.m_storage.data(), g_bf_tmp_copy1.data(), g_bf_pi.data());
    return result;
}

BigFloat BigFloat::atan2(const BigFloat &x) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    binary_operation(result.m_storage.data(), m_storage.data(), x.m_storage.data(), arctangent2);
    return result;
}

BigFloat &clear_bf(BigFloat &r)
{
    clear_bf(r.data());
    return r;
}

BigFloat &copy_bf(BigFloat &r, const BigFloat &n)
{
    copy_bf(r.data(), n.data());
    return r;
}

BigFloat &max_bf(BigFloat &r)
{
    max_bf(r.data());
    return r;
}

int sign_bf(const BigFloat &n)
{
    return sign_bf(n.data());
}

bool is_bf_zero(const BigFloat &n)
{
    return is_bf_zero(n.data());
}

bool is_bf_neg(const BigFloat &n)
{
    return is_bf_neg(n.data());
}

bool is_bf_not_zero(const BigFloat &n)
{
    return is_bf_not_zero(n.data());
}

int cmp_bf(const BigFloat &n1, const BigFloat &n2)
{
    return cmp_bf(n1.data(), n2.data());
}

BigFloat &neg_bf(BigFloat &r, const BigFloat &n)
{
    neg_bf(r.data(), n.data());
    return r;
}

BigFloat &neg_a_bf(BigFloat &r)
{
    neg_a_bf(r.data());
    return r;
}

BigFloat &abs_bf(BigFloat &r, const BigFloat &n)
{
    abs_bf(r.data(), n.data());
    return r;
}

BigFloat &abs_a_bf(BigFloat &r)
{
    abs_a_bf(r.data());
    return r;
}

BigFloat &add_bf(BigFloat &r, const BigFloat &n1, const BigFloat &n2)
{
    binary_operation(r.data(), n1.data(), n2.data(), unsafe_add_bf);
    return r;
}

BigFloat &add_a_bf(BigFloat &r, const BigFloat &n)
{
    compound_operation(r.data(), n.data(), unsafe_add_a_bf);
    return r;
}

BigFloat &sub_bf(BigFloat &r, const BigFloat &n1, const BigFloat &n2)
{
    binary_operation(r.data(), n1.data(), n2.data(), unsafe_sub_bf);
    return r;
}

BigFloat &sub_a_bf(BigFloat &r, const BigFloat &n)
{
    compound_operation(r.data(), n.data(), unsafe_sub_a_bf);
    return r;
}

BigFloat &mult_bf(BigFloat &r, const BigFloat &n1, const BigFloat &n2)
{
    binary_operation(r.data(), n1.data(), n2.data(), multiply);
    return r;
}

BigFloat &square_bf(BigFloat &r, const BigFloat &n)
{
    copy_bf(g_bf_tmp_copy1.data(), n.data());
    square_value(r.data(), g_bf_tmp_copy1.data());
    return r;
}

BigFloat &mult_bf_int(BigFloat &r, const BigFloat &n, const U16 u)
{
    copy_bf(g_bf_tmp_copy1.data(), n.data());
    unsafe_mult_bf_int(r.data(), g_bf_tmp_copy1.data(), u);
    return r;
}

BigFloat &mult_a_bf_int(BigFloat &r, const U16 u)
{
    mult_a_bf_int(r.data(), u);
    return r;
}

BigFloat &double_bf(BigFloat &r, const BigFloat &n)
{
    double_bf(r.data(), n.data());
    return r;
}

BigFloat &double_a_bf(BigFloat &r)
{
    double_a_bf(r.data());
    return r;
}

BigFloat &half_bf(BigFloat &r, const BigFloat &n)
{
    half_bf(r.data(), n.data());
    return r;
}

BigFloat &half_a_bf(BigFloat &r)
{
    half_a_bf(r.data());
    return r;
}

BigFloat &unsafe_inv_bf(BigFloat &r, BigFloat &n)
{
    unsafe_inv_bf(r.data(), n.data(), g_bf_tmp1.data(), g_bf_tmp2.data());
    return r;
}

BigFloat &div_bf(BigFloat &r, const BigFloat &n1, const BigFloat &n2)
{
    copy_bf(g_bf_tmp_copy1.data(), n1.data());
    copy_bf(g_bf_tmp_copy2.data(), n2.data());
    unsafe_div_bf(r.data(), g_bf_tmp_copy1.data(), g_bf_tmp_copy2.data(), g_bf_tmp1.data(), g_bf_tmp2.data());
    return r;
}

BigFloat &unsafe_div_bf_int(BigFloat &r, BigFloat &n, const U16 u)
{
    unsafe_div_bf_int(r.data(), n.data(), u);
    return r;
}

BigFloat &div_a_bf_int(BigFloat &r, const U16 u)
{
    div_a_bf_int(r.data(), u);
    return r;
}

BigFloat &exp_bf(BigFloat &r, const BigFloat &n)
{
    exp_bf(r.data(), n.data(), g_bf_tmp1.data(), g_bf_tmp2.data(), g_bf_tmp3.data());
    return r;
}

BigFloat &str_to_bf(BigFloat &r, const std::string_view text)
{
    str_to_bf(r.data(), text, g_bf_tmp1.data());
    return r;
}

BigFloat &int_to_bf(BigFloat &r, const long value)
{
    int_to_bf(r.data(), value);
    return r;
}

BigFloat &float_to_bf(BigFloat &r, const LDouble value)
{
    float_to_bf(r.data(), value);
    return r;
}

LDouble bf_to_float(BigFloat &value)
{
    return bf_to_float(value.data());
}

const Byte *unpack_bf(BigFloat &destination, const Byte *source, const int stored_length)
{
    return unpack_bf(destination.data(), source, stored_length);
}

/**********************************************************************/
std::string bf_to_string(const BigFloat &r, const int dec)
{
    copy_bf(g_bf_tmp_copy1.data(), r.data());
    return unsafe_bf_to_string(g_bf_tmp_copy1.data(), dec);
}

/**********************************************************************/
std::string bf_to_string_e(const BigFloat &r, const int dec)
{
    copy_bf(g_bf_tmp_copy1.data(), r.data());
    return unsafe_bf_to_string_e(g_bf_tmp_copy1.data(), dec);
}

/**********************************************************************/
std::string bf_to_string_f(const BigFloat &r, const int dec)
{
    copy_bf(g_bf_tmp_copy1.data(), r.data());
    return unsafe_bf_to_string_f(g_bf_tmp_copy1.data(), dec);
}

BigFloat abs(const BigFloat &value)
{
    return value.abs();
}

BigFloat sqr(const BigFloat &value)
{
    return value.sqr();
}

BigFloat log(const BigFloat &value)
{
    return value.log();
}

BigFloat exp(const BigFloat &value)
{
    return value.exp();
}

std::pair<BigFloat, BigFloat> sin_cos(const BigFloat &value)
{
    return value.sin_cos();
}

BigFloat atan2(const BigFloat &y, const BigFloat &x)
{
    return y.atan2(x);
}

bool BigFloat::operator==(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) == 0;
}

bool BigFloat::operator!=(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) != 0;
}

bool BigFloat::operator<(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) < 0;
}

bool BigFloat::operator<=(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) <= 0;
}

bool BigFloat::operator>(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) > 0;
}

bool BigFloat::operator>=(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) >= 0;
}

LDouble BigFloat::to_float() const
{
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    return bf_to_float(g_bf_tmp_copy1.data());
}

std::string BigFloat::to_string(const int digits) const
{
    if (digits < 0)
    {
        throw std::invalid_argument("BigFloat formatting requires a nonnegative digit count");
    }
    const int default_digits = g_decimals > 0 ? g_decimals : static_cast<int>((g_bf_length - 2) * LOG10_256);
    return bf_to_string(*this, digits == 0 ? default_digits : digits);
}

BigFloat BigFloat::from_legacy(BigFloatConstPtr source, const std::size_t size)
{
    return from_legacy(bf_arena(BigLifetime::WORKING), source, size);
}

BigFloat BigFloat::from_legacy(BigArena &arena, BigFloatConstPtr source, const std::size_t size)
{
    BigFloat result{arena};
    validate_buffer(source, size, result.encoded_size());
    std::copy_n(source, result.encoded_size(), result.m_storage.data());
    return result;
}

void BigFloat::copy_to_legacy(BigFloatPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, encoded_size());
    std::copy_n(m_storage.data(), encoded_size(), destination);
}

void BigFloat::copy_to_legacy_num(BigNumPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, static_cast<std::size_t>(g_bn_length));
    bf_to_bn(destination, m_storage.data());
}

BigFloatProduct::BigFloatProduct() noexcept = default;

BigFloatProduct::BigFloatProduct(BigArena &arena) :
    m_storage(arena.allocate_double())
{
    std::memset(data(), 0, 2 * arena.block_size());
}

BigFloatProduct::BigFloatProduct(const BigFloatProduct &value) :
    m_storage(value.m_storage.clone())
{
}

BigFloatProduct::BigFloatProduct(BigArena &arena, const BigFloatProduct &value) :
    m_storage(value.m_storage.clone(arena))
{
}

BigFloatProduct::BigFloatProduct(BigFloatProduct &&value) noexcept = default;

BigFloatProduct &BigFloatProduct::operator=(const BigFloatProduct &value)
{
    if (this != &value)
    {
        if (!value.m_storage)
        {
            m_storage.reset();
        }
        else
        {
            if (!m_storage)
            {
                BigArena &destination = m_storage.arena_ptr() != nullptr ? arena() : bf_arena(BigLifetime::WORKING);
                m_storage = destination.allocate_double();
            }
            std::memcpy(data(), value.data(), g_r_bf_length + 2);
        }
    }
    return *this;
}

BigFloatProduct &BigFloatProduct::operator=(BigFloatProduct &&value)
{
    if (this != &value)
    {
        if (!value.m_storage)
        {
            m_storage.reset();
        }
        else if ((m_storage.arena_ptr() != nullptr ? &arena() : &bf_arena(BigLifetime::WORKING)) == &value.arena())
        {
            m_storage = std::move(value.m_storage);
        }
        else
        {
            *this = static_cast<const BigFloatProduct &>(value);
        }
    }
    return *this;
}

BigArena::Allocation BigFloatProduct::release_reduced_storage() &&
{
    BigArena::Allocation result = std::move(m_storage);
    if (result)
    {
        result.reduce();
    }
    return result;
}

Byte *unsafe_square_bf(Byte *r, Byte *n)
{
    return square_value(r, n);
}

BigFloatPtr unsafe_ln_bf(BigFloatPtr r, BigFloatPtr n)
{
    return logarithm(r, n);
}

BigFloatPtr unsafe_sin_cos_bf(BigFloatPtr s, BigFloatPtr c, BigFloatPtr n)
{
    return sine_cosine(s, c, n, g_bf_pi.data());
}

BigFloatPtr unsafe_atan2_bf(BigFloatPtr r, BigFloatPtr y, BigFloatPtr x)
{
    return arctangent2(r, y, x);
}

Byte *unsafe_mult_bf(Byte *r, Byte *n1, Byte *n2)
{
    return multiply(r, n1, n2);
}

} // namespace id::math
