// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// C routines for bignumbers

/*
Note: This is NOT the biginit.c file that come the standard BigNum library,
but is a customized version specific to the original Fractint code.
The biggest difference is in the allocations of memory for the big numbers.
*/
#include <math/big_init.h>

#include <engine/bailout_formula.h>
#include <engine/calcfrac.h>
#include <engine/UserData.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <math/big.h>
#include <math/BigFloat.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::fractals;

namespace id::math
{

int g_bf_digits{};                      // digits to use (force) for g_bf_math

int g_bn_step{};
int g_bn_length{};
int g_int_length{};
int g_r_length{};
int g_padding{};
int g_shift_factor{};
int g_decimals{};
int g_bf_length{};
int g_r_bf_length{};
int g_bf_decimals{};
// g_bf_length+2
BigFloat g_bf_tmp1{};
BigFloat g_bf_tmp2{};
BigFloat g_bf_tmp3{};
BigFloat g_bf_tmp4{};
BigFloat g_bf_tmp5{};
BigFloat g_bf_tmp6{};
BigArena::Allocation g_bf_tmp_copy1{};
BigArena::Allocation g_bf_tmp_copy2{};
BigFloat g_delta_x_bf{};
BigFloat g_delta_y_bf{};
BigFloat g_delta2_x_bf{};
BigFloat g_delta2_y_bf{};
BigFloat g_close_enough_bf{};
BigFloat g_tmp_sqr_x_bf{};
BigFloat g_tmp_sqr_y_bf{};
// g_bf_length+2
BFComplex g_param_z_bf{};
BFComplex g_saved_z_bf{};
// g_bf_length+2
BFComplex g_old_z_bf{};
BFComplex g_new_z_bf{};
BigFloat g_bf_pi{}; // g_bf_length+2
// g_bf_length+2
BigFloat g_bf_x_min{};
BigFloat g_bf_x_max{};
BigFloat g_bf_y_min{};
BigFloat g_bf_y_max{};
BigFloat g_bf_x_3rd{};
BigFloat g_bf_y_3rd{};
BigFloat g_bf_save_x_min{};
BigFloat g_bf_save_x_max{};
BigFloat g_bf_save_y_min{};
BigFloat g_bf_save_y_max{};
BigFloat g_bf_save_x_3rd{};
BigFloat g_bf_save_y_3rd{};
BigFloat g_bf_params[10]{}; // (g_bf_length + 2)*10
BigFloat g_bf_tmp{};

constexpr long BIGNUM_ARENA_SIZE{1024L * 1024};
// Supported g_bn_length limits are independent of working storage capacity.
constexpr int MAX_BIGFLOAT_LENGTH{708};

/*********************************************************************/
// given g_bn_length, calc_lengths will calculate all the other lengths
void calc_lengths()
{
    g_bn_step = 4;  // use 4 in all cases

    if (g_bn_length % g_bn_step != 0)
    {
        g_bn_length = (g_bn_length / g_bn_step + 1) * g_bn_step;
    }
    if (g_bn_length == g_bn_step)
    {
        g_padding = g_bn_length;
    }
    else
    {
        g_padding = 2*g_bn_step;
    }
    g_r_length = g_bn_length + g_padding;

    // This shift factor assumes non-full multiplications will be performed.
    // Change to g_bn_length-g_int_length for full multiplications.
    g_shift_factor = g_padding - g_int_length;

    g_bf_length = g_bn_length+g_bn_step; // one extra step for added precision
    g_r_bf_length = g_bf_length + g_padding;
    g_bf_decimals = static_cast<int>((g_bf_length - 2) * LOG10_256);
}

/************************************************************************/
// intended only to be called from init_bf_dec() or init_bf_length().
// initialize bignumber global variables

static bool supported_bn_length(const int bn_length)
{
    return bn_length > 0 && bn_length <= MAX_BIGFLOAT_LENGTH;
}

static int bf_length_for_bn_length(const int bn_length)
{
    return (bn_length + 3) / 4 * 4 + 4;
}

size_t bf_parameter_bytes(const int bn_length, const FractalType)
{
    if (!supported_bn_length(bn_length))
    {
        throw PrecisionError(fmt::format("Requested bignumber length of {} is unsupported", bn_length));
    }
    return 22 * static_cast<size_t>(bf_length_for_bn_length(bn_length) + 2);
}

namespace
{

std::size_t arena_blocks(const std::size_t block_size)
{
    const std::size_t stride = (block_size + BigArena::ALIGNMENT - 1) / BigArena::ALIGNMENT * BigArena::ALIGNMENT;
    return static_cast<std::size_t>(BIGNUM_ARENA_SIZE) / stride & ~std::size_t{1};
}

std::array<BigFloat *, 22> parameter_bindings()
{
    return {&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd, &g_bf_y_3rd, &g_bf_params[0],
        &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4], &g_bf_params[5], &g_bf_params[6],
        &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min, &g_bf_save_x_max, &g_bf_save_y_min,
        &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
}

class NumericArenas
{
public:
    NumericArenas(std::size_t value_size, std::size_t block_size, BigArena::Reduce reduce);
    BigArena &get(BigLifetime lifetime);

    BigArena persistent;
    BigArena working;
    BigArena temporary;
    BigStack stack;
};

NumericArenas::NumericArenas(
    const std::size_t value_size, const std::size_t block_size, const BigArena::Reduce reduce) :
    persistent(value_size, block_size, arena_blocks(block_size), reduce),
    working(value_size, block_size, arena_blocks(block_size), reduce),
    temporary(value_size, block_size, arena_blocks(block_size), reduce),
    stack(temporary)
{
}

BigArena &NumericArenas::get(const BigLifetime lifetime)
{
    switch (lifetime)
    {
    case BigLifetime::PERSISTENT:
        return persistent;
    case BigLifetime::WORKING:
        return working;
    case BigLifetime::TEMPORARY:
        return temporary;
    }
    throw std::invalid_argument("Unknown big-number storage lifetime");
}

class MathArenas
{
public:
    MathArenas(int bn_length, int bf_length, int int_length);
    ~MathArenas();
    bool matches_layout(int bn_length, int int_length) const;
    void bind(bool preserve_parameters, bool preserve_precision);
    void unbind();
    bool bound() const;

    NumericArenas big_float;

private:
    int m_bn_length;
    int m_int_length;
    bool m_bound{};
};

MathArenas::MathArenas(const int bn_length, const int bf_length, const int int_length) :
    big_float(bf_length + 2, static_cast<std::size_t>(bf_length + 2), reduce_bf_product),
    m_bn_length(bn_length),
    m_int_length(int_length)
{
}

MathArenas::~MathArenas()
{
    if (m_bound)
    {
        unbind();
    }
}

bool MathArenas::matches_layout(const int bn_length, const int int_length) const
{
    return bn_length == m_bn_length && int_length == m_int_length;
}

void MathArenas::bind(const bool preserve_parameters, const bool preserve_precision)
{
    big_float.stack.restore(0);
    const MPPrecision precision{bf_precision()};
    const auto init_big_float = [this, &precision](BigFloat &target)
    {
        if (!target)
        {
            BigFloat value{big_float.working};
            std::destroy_at(&target);
            ::new (&target) BigFloat{std::move(value)};
        }
        target.set_precision(precision);
        target.clear();
    };
    init_big_float(g_bf_tmp1);
    init_big_float(g_bf_tmp2);
    init_big_float(g_bf_tmp3);
    init_big_float(g_bf_tmp4);
    init_big_float(g_bf_tmp5);
    init_big_float(g_bf_tmp6);
    const auto init_big_float_copy = [this](BigArena::Allocation &target)
    {
        if (!target)
        {
            target = big_float.working.allocate_double();
        }
        std::memset(target.data(), 0, 2 * big_float.working.block_size());
    };
    init_big_float_copy(g_bf_tmp_copy1);
    init_big_float_copy(g_bf_tmp_copy2);
    init_big_float(g_bf_pi);
    init_big_float(g_delta_x_bf);
    init_big_float(g_delta_y_bf);
    init_big_float(g_delta2_x_bf);
    init_big_float(g_delta2_y_bf);
    init_big_float(g_param_z_bf.x);
    init_big_float(g_param_z_bf.y);
    init_big_float(g_old_z_bf.x);
    init_big_float(g_old_z_bf.y);
    init_big_float(g_new_z_bf.x);
    init_big_float(g_new_z_bf.y);
    init_big_float(g_saved_z_bf.x);
    init_big_float(g_saved_z_bf.y);
    init_big_float(g_close_enough_bf);
    init_big_float(g_tmp_sqr_x_bf);
    init_big_float(g_tmp_sqr_y_bf);
    init_big_float(g_bf_tmp);
    for (BigFloat *target : parameter_bindings())
    {
        if (!preserve_parameters && !*target)
        {
            BigFloat value{big_float.persistent};
            std::destroy_at(target);
            ::new (target) BigFloat{std::move(value)};
        }
        if (!preserve_parameters || !preserve_precision)
        {
            target->set_precision(precision);
        }
    }
    m_bound = true;
}

void MathArenas::unbind()
{
    m_bound = false;
    big_float.stack.restore(0);
    const auto reset = [](auto &value)
    {
        std::destroy_at(&value);
        ::new (&value) std::remove_reference_t<decltype(value)>{};
    };
    reset(g_bf_pi);
    reset(g_bf_tmp1);
    reset(g_bf_tmp2);
    reset(g_bf_tmp3);
    reset(g_bf_tmp4);
    reset(g_bf_tmp5);
    reset(g_bf_tmp6);
    g_bf_tmp_copy1.reset();
    g_bf_tmp_copy2.reset();
    reset(g_delta_x_bf);
    reset(g_delta_y_bf);
    reset(g_delta2_x_bf);
    reset(g_delta2_y_bf);
    reset(g_param_z_bf);
    reset(g_old_z_bf);
    reset(g_new_z_bf);
    reset(g_saved_z_bf);
    reset(g_close_enough_bf);
    reset(g_tmp_sqr_x_bf);
    reset(g_tmp_sqr_y_bf);
    reset(g_bf_tmp);
    for (BigFloat *target : parameter_bindings())
    {
        reset(*target);
    }
}

bool MathArenas::bound() const
{
    return m_bound;
}

std::unique_ptr<MathArenas> s_arenas;

} // namespace

static void init_bf2(const int bn_length, const int int_length, const int decimals)
{
    if (!supported_bn_length(bn_length))
    {
        throw PrecisionError(fmt::format("Requested precision of {} too high", decimals));
    }
    const int bf_length = bf_length_for_bn_length(bn_length);
    const int rounded_length = bf_length - 4;
    const bool carry_view = s_arenas && s_arenas->bound();
    const bool same_precision = carry_view && g_decimals == decimals;
    const bool same_layout = s_arenas && s_arenas->matches_layout(rounded_length, int_length);
    if (!same_layout)
    {
        auto replacement = std::make_unique<MathArenas>(rounded_length, bf_length, int_length);
        if (carry_view)
        {
            const std::array<BigFloat *, 12> corners{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd,
                &g_bf_y_3rd, &g_bf_save_x_min, &g_bf_save_x_max, &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd,
                &g_bf_save_y_3rd};
            std::vector<BigFloat> converted;
            converted.reserve(corners.size());
            for (const BigFloat *corner : corners)
            {
                converted.push_back(
                    corner->convert_precision(replacement->big_float.persistent, bf_length, g_bf_length, decimals));
            }
            s_arenas->unbind();
            for (std::size_t i = 0; i < corners.size(); ++i)
            {
                std::destroy_at(corners[i]);
                ::new (corners[i]) BigFloat{std::move(converted[i])};
            }
        }
        s_arenas = std::move(replacement);
    }
    // All potentially throwing storage preparation has completed.
    g_bn_length = rounded_length;
    g_int_length = int_length;
    g_decimals = decimals;
    calc_lengths();
    g_bf_math = BFMathType::BIG_FLT;
    s_arenas->bind(same_layout && carry_view, same_precision);
    if (carry_view && !same_layout)
    {
        for (int i = 0; i < MAX_PARAMS; ++i)
        {
            g_bf_params[i] = g_params[i];
        }
    }
}

BigArena &bf_arena(const BigLifetime lifetime)
{
    return s_arenas->big_float.get(lifetime);
}

MPPrecision bf_precision()
{
    // A stored byte layout can have no requested decimal fraction digits.
    const int native_digits = std::numeric_limits<LDouble>::max_digits10 - static_cast<int>(MPPrecision::GUARD_DIGITS);
    const int digits = std::max(g_decimals > 0 ? g_decimals : g_bf_decimals, native_digits);
    return MPPrecision{std::max(digits, 1), g_bf_digits};
}

BigStack::BigStack(BigArena &arena) :
    m_arena(arena)
{
}

Byte *BigStack::allocate_single()
{
    auto value = m_arena.allocate_single();
    Byte *result = value.data();
    std::memset(result, 0, m_arena.block_size());
    m_values.push_back(std::move(value));
    return result;
}

Byte *BigStack::allocate_double()
{
    auto value = m_arena.allocate_double();
    Byte *result = value.data();
    std::memset(result, 0, 2 * m_arena.block_size());
    m_values.push_back(std::move(value));
    return result;
}

std::size_t BigStack::mark() const
{
    return m_values.size();
}

void BigStack::restore(const std::size_t mark)
{
    while (m_values.size() > mark)
    {
        m_values.pop_back();
    }
}

bool BigStack::contains(const Byte *address) const
{
    return std::any_of(m_values.begin(), m_values.end(),
        [address](const BigArena::Allocation &value) { return value.data() == address; });
}

BigStack &bf_stack()
{
    if (!s_arenas || !s_arenas->bound() || g_bf_math == BFMathType::NONE)
    {
        throw std::logic_error("BigFloat stack called without initialized arbitrary precision");
    }
    return s_arenas->big_float.stack;
}

bool BigStackMark::operator==(const BigStackMark &right) const
{
    return big_float == right.big_float;
}

bool BigStackMark::operator!=(const BigStackMark &right) const
{
    return !(*this == right);
}

bool BigStackMark::operator>(const BigStackMark &right) const
{
    return big_float > right.big_float;
}

BigStackMark save_stack()
{
    return !s_arenas || !s_arenas->bound() ? BigStackMark{} : BigStackMark{s_arenas->big_float.stack.mark()};
}

void restore_stack(const BigStackMark old_mark)
{
    if (s_arenas)
    {
        s_arenas->big_float.stack.restore(old_mark.big_float);
    }
}

BigStackSaver::BigStackSaver() :
    m_mark(save_stack())
{
}

BigStackSaver::~BigStackSaver()
{
    restore_stack(m_mark);
}

BigMemoryStats big_memory_stats()
{
    BigMemoryStats result;
    if (s_arenas)
    {
        for (BigLifetime lifetime : {BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY})
        {
            for (const BigArena *arena : {&bf_arena(lifetime)})
            {
                const std::size_t used = arena->high_water_mark() * arena->block_size();
                (lifetime == BigLifetime::TEMPORARY ? result.temporaries : result.globals) += used;
                result.capacity += arena->capacity() * arena->block_size();
            }
        }
    }
    return result;
}

std::vector<Byte> pack_bf_parameters()
{
    const std::size_t length = static_cast<std::size_t>(g_bf_length) + 2;
    std::vector<Byte> result(22 * length);
    std::size_t offset = 0;
    for (const BigFloat *value : parameter_bindings())
    {
        value->pack_bf(result.data() + offset, length);
        offset += length;
    }
    return result;
}

void unpack_bf_parameters(const std::vector<Byte> &parameters)
{
    const std::size_t length = parameters.size() / 22;
    if (parameters.size() % 22 != 0 || length <= 6 || length > static_cast<std::size_t>(MAX_BIGFLOAT_LENGTH) + 6)
    {
        throw std::invalid_argument("Incomplete arbitrary precision parameter payload");
    }
    // Two exponent bytes and four extra BigFloat mantissa bytes.
    const int bn_length = static_cast<int>(length) - 6;
    if (parameters.size() != bf_parameter_bytes(bn_length, g_fractal_type))
    {
        throw std::invalid_argument("Incomplete arbitrary precision parameter payload");
    }
    init_bf_length(bn_length);
    const Byte *cursor = parameters.data();
    for (BigFloat *value : parameter_bindings())
    {
        cursor = BigFloat::unpack_bf(*value, cursor, g_bf_length);
    }
}

void free_bf_vars()
{
    if (s_arenas)
    {
        s_arenas->unbind();
    }
    g_bf_math = BFMathType::NONE;
    g_bn_step = 0;
    g_bn_length = 0;
    g_int_length = 0;
    g_r_length = 0;
    g_padding = 0;
    g_shift_factor = 0;
    g_decimals = 0;
    g_bf_length = 0;
    g_r_bf_length = 0;
    g_bf_decimals = 0;
}

static int integer_length()
{
    if (g_user.bailout_value > 10)      // arbitrary value
    {
        // using 2 doesn't gain much and requires another test
        return 4;
    }
    else if (g_fractal_type == FractalType::MANDEL_Z_POWER //
        || g_fractal_type == FractalType::JULIA_Z_POWER    //
        || g_fractal_type == FractalType::DIVIDE_BROT5)
    {
        return 4;                                          // 2 leaves artifacts in the center of the lakes
    }
    // the bailout tests need greater dynamic range
    else if (g_bailout_test == Bailout::REAL //
        || g_bailout_test == Bailout::IMAG   //
        || g_bailout_test == Bailout::AND    //
        || g_bailout_test == Bailout::MANR)
    {
        return 2;
    }
    return 1;
}

/************************************************************************/
// initialize bignumber global variables
//   dec = decimal places after decimal point
void init_bf_dec(const int dec)
{
    const int decimals{g_bf_digits > 0 ? g_bf_digits : dec};
    const int int_length{integer_length()};
    // conservative estimate
    const int bn_length{int_length + static_cast<int>(decimals / LOG10_256) + 1}; // round up
    init_bf2(bn_length, int_length, decimals);
}

/************************************************************************/
// initialize bignumber global variables
//   bnl = bignumber length
//   intl = bytes for integer part (1, 2, or 4)
void init_bf_length(const int bnl)
{
    if (bnl <= 0 || bnl > MAX_BIGFLOAT_LENGTH)
    {
        throw PrecisionError(fmt::format("Requested bignumber length of {} is unsupported", bnl));
    }
    const int int_length{integer_length()};
    // conservative estimate
    const int decimals{static_cast<int>((bnl - int_length) * LOG10_256)};
    init_bf2(bnl, int_length, decimals);
}

void init_big_pi()
{
    g_bf_pi.load_pi();
}

} // namespace id::math
