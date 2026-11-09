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

#include "BigStorage.h"

#include <engine/bailout_formula.h>
#include <engine/calcfrac.h>
#include <engine/UserData.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <math/BigFloat.h>
#include <math/BigNum.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
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
BigNumPtr g_bn_tmp1{};
BigNumPtr g_bn_tmp2{};
BigNumPtr g_bn_tmp3{};
BigNumPtr g_bn_tmp_copy1{};
BigNumPtr g_bn_tmp_copy2{};
// used by other routines, g_bn_length
BigNum g_x_min_bn{};
BigNum g_x_max_bn{};
BigNum g_y_min_bn{};
BigNum g_y_max_bn{};
BigNum g_x_3rd_bn{};
BigNum g_y_3rd_bn{};
// g_bn_length
BigNum g_delta_x_bn{};
BigNum g_delta_y_bn{};
BigNum g_delta2_x_bn{};
BigNum g_delta2_y_bn{};
BigNum g_close_enough_bn{};
// g_r_length
BigNumPtr g_tmp_sqr_x_bn{};
BigNumPtr g_tmp_sqr_y_bn{};
BigNumPtr g_bn_tmp{};
// g_bn_length
BNPtrComplex g_old_z_bn{};
BNComplex g_param_z_bn{};
BNComplex g_saved_z_bn{};
BNComplex g_new_z_bn{};
// g_r_bf_length+2
BigFloatPtr g_bf_tmp1{};
BigFloatPtr g_bf_tmp2{};
BigFloatPtr g_bf_tmp3{};
BigFloatPtr g_bf_tmp4{};
BigFloatPtr g_bf_tmp5{};
BigFloatPtr g_bf_tmp6{};
BigFloatPtr g_bf_tmp_copy1{};
BigFloatPtr g_bf_tmp_copy2{};
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
BigFloatPtr g_bf_pi{};  // TAKES NO SPACE
BigFloatPtr g_big_pi{}; // g_bf_length+2
// g_bf_length+2
BigFloatPtr g_bf_x_min{};
BigFloatPtr g_bf_x_max{};
BigFloatPtr g_bf_y_min{};
BigFloatPtr g_bf_y_max{};
BigFloatPtr g_bf_x_3rd{};
BigFloatPtr g_bf_y_3rd{};
BigFloatPtr g_bf_save_x_min{};
BigFloatPtr g_bf_save_x_max{};
BigFloatPtr g_bf_save_y_min{};
BigFloatPtr g_bf_save_y_max{};
BigFloatPtr g_bf_save_x_3rd{};
BigFloatPtr g_bf_save_y_3rd{};
BigFloatPtr g_bf_params[10]{}; // (g_bf_length + 2)*10
BigFloat g_bf_tmp{};

constexpr long BIGNUM_ARENA_SIZE{1024L * 1024};
// Supported g_bn_length limits are independent of working storage capacity.
constexpr int MAX_BIGNUM_LENGTH{672};
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

static BFMathType select_bf_math(const FractalType type)
{
    if (const int i = find_alternate_math(type, BFMathType::BIG_NUM); i > -1)
    {
        return g_alternate_math[i].math;
    }
    if (const int i = find_alternate_math(type, BFMathType::BIG_FLT); i > -1)
    {
        return g_alternate_math[i].math;
    }
    return BFMathType::BIG_NUM;
}

static bool supported_bn_length(const int bn_length, const BFMathType math)
{
    return bn_length > 0 && bn_length <= (math == BFMathType::BIG_NUM ? MAX_BIGNUM_LENGTH : MAX_BIGFLOAT_LENGTH);
}

static int bf_length_for_bn_length(const int bn_length)
{
    return (bn_length + 3) / 4 * 4 + 4;
}

size_t bf_parameter_bytes(const int bn_length, const FractalType type)
{
    if (!supported_bn_length(bn_length, select_bf_math(type)))
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

using WorkingBinding = std::pair<BigNumPtr *, BigArena::Allocation>;

std::array<BigFloatPtr *, 22> parameter_bindings()
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
    void single(BigNumPtr &target);
    void wide(BigNumPtr &target);
    bool is_workspace(const Byte *source) const;
    void bind_working();
    void unbind_working();

    BigArena persistent;
    BigArena working;
    BigArena temporary;
    BigStack stack;
    std::vector<WorkingBinding> bindings;
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

void NumericArenas::single(BigNumPtr &target)
{
    bindings.emplace_back(&target, working.allocate_single());
}

void NumericArenas::wide(BigNumPtr &target)
{
    bindings.emplace_back(&target, working.allocate_double());
}

bool NumericArenas::is_workspace(const Byte *source) const
{
    return stack.contains(source) ||
        std::any_of(bindings.begin(), bindings.end(),
            [source](const WorkingBinding &binding) { return binding.second.data() == source; });
}

void NumericArenas::bind_working()
{
    for (auto &binding : bindings)
    {
        *binding.first = binding.second.data();
        std::memset(binding.second.data(), 0, binding.second.blocks() * working.block_size());
    }
}

void NumericArenas::unbind_working()
{
    for (const auto &binding : bindings)
    {
        *binding.first = nullptr;
    }
}

class MathArenas
{
public:
    MathArenas(int bn_length, int bf_length, int int_length, int padding);
    ~MathArenas();
    void change_precision(int bn_length, int bf_length, int int_length, int padding, bool preserve_parameters);
    void bind(bool preserve_parameters);
    void unbind();
    bool bound() const;

    NumericArenas big_num;
    NumericArenas big_float;

private:
    int m_bn_length;
    int m_bf_length;
    int m_int_length;
    bool m_bound{};
    std::array<BigArena::Allocation, 22> m_parameters;
};

MathArenas::MathArenas(const int bn_length, const int bf_length, const int int_length, const int padding) :
    big_num(bn_length, static_cast<std::size_t>(bn_length + padding), detail::reduce_bn_product),
    big_float(bf_length + 2, static_cast<std::size_t>(bf_length + padding + 2), detail::reduce_bf_product),
    m_bn_length(bn_length),
    m_bf_length(bf_length),
    m_int_length(int_length)
{
    big_num.single(g_bn_tmp1);
    big_num.single(g_bn_tmp2);
    big_num.single(g_bn_tmp3);
    big_num.wide(g_bn_tmp_copy1);
    big_num.wide(g_bn_tmp_copy2);
    big_num.single(g_old_z_bn.x);
    big_num.single(g_old_z_bn.y);
    big_num.single(g_tmp_sqr_x_bn);
    big_num.single(g_tmp_sqr_y_bn);
    big_num.single(g_bn_tmp);
    big_float.single(g_bf_tmp1);
    big_float.single(g_bf_tmp2);
    big_float.single(g_bf_tmp3);
    big_float.single(g_bf_tmp4);
    big_float.single(g_bf_tmp5);
    big_float.single(g_bf_tmp6);
    big_float.wide(g_bf_tmp_copy1);
    big_float.wide(g_bf_tmp_copy2);
    big_float.single(g_big_pi);
    for (auto &value : m_parameters)
    {
        value = big_float.persistent.allocate_single();
    }
}

MathArenas::~MathArenas()
{
    unbind();
}

void MathArenas::change_precision(
    const int bn_length, const int bf_length, const int int_length, const int padding, const bool preserve_parameters)
{
    if (bn_length == m_bn_length && bf_length == m_bf_length && int_length == m_int_length)
    {
        return;
    }
    std::vector<BigArena::Relocation> replacements;
    replacements.reserve(6);
    const std::size_t bn_stride = (bn_length + padding + 63) / 64 * 64;
    const std::size_t bf_stride = (bf_length + padding + 2 + 63) / 64 * 64;
    for (BigLifetime lifetime : {BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY})
    {
        replacements.push_back(big_num.get(lifetime).prepare_relocation(bn_length, bn_length + padding,
            [&](Byte *destination, const Byte *source, std::size_t blocks)
            {
                std::memset(destination, 0, blocks * bn_stride);
                if (!big_num.is_workspace(source))
                {
                    convert_bn(destination, source, bn_length, int_length, m_bn_length, m_int_length);
                    // Legacy scratch conversion zero-fills new integer bytes;
                    // live signed values require sign extension instead.
                    if (int_length > m_int_length && (source[m_bn_length - 1] & 0x80) != 0)
                    {
                        const int extension = int_length - m_int_length;
                        std::memset(destination + bn_length - extension, 0xff, extension);
                    }
                }
            }));
        replacements.push_back(big_float.get(lifetime).prepare_relocation(bf_length + 2, bf_length + padding + 2,
            [&](Byte *destination, const Byte *source, std::size_t blocks)
            {
                std::memset(destination, 0, blocks * bf_stride);
                const bool discarded_parameter = !preserve_parameters &&
                    std::any_of(m_parameters.begin(), m_parameters.end(),
                        [source](const BigArena::Allocation &value) { return value.data() == source; });
                if (!big_float.is_workspace(source) && !discarded_parameter)
                {
                    convert_bf(destination, source, bf_length, m_bf_length);
                }
            }));
    }
    for (auto &replacement : replacements)
    {
        replacement.commit();
    }
    m_bn_length = bn_length;
    m_bf_length = bf_length;
    m_int_length = int_length;
}

void MathArenas::bind(const bool preserve_parameters)
{
    big_num.stack.restore(0);
    big_float.stack.restore(0);
    big_num.bind_working();
    g_x_min_bn = 0;
    g_x_max_bn = 0;
    g_y_min_bn = 0;
    g_y_max_bn = 0;
    g_x_3rd_bn = 0;
    g_y_3rd_bn = 0;
    g_delta_x_bn = 0;
    g_delta_y_bn = 0;
    g_delta2_x_bn = 0;
    g_delta2_y_bn = 0;
    g_param_z_bn.x = 0;
    g_param_z_bn.y = 0;
    g_new_z_bn.x = 0;
    g_new_z_bn.y = 0;
    g_saved_z_bn.x = 0;
    g_saved_z_bn.y = 0;
    g_close_enough_bn = 0;
    big_float.bind_working();
    g_delta_x_bf = 0;
    g_delta_y_bf = 0;
    g_delta2_x_bf = 0;
    g_delta2_y_bf = 0;
    g_param_z_bf.x = 0;
    g_param_z_bf.y = 0;
    g_old_z_bf.x = 0;
    g_old_z_bf.y = 0;
    g_new_z_bf.x = 0;
    g_new_z_bf.y = 0;
    g_saved_z_bf.x = 0;
    g_saved_z_bf.y = 0;
    g_close_enough_bf = 0;
    g_tmp_sqr_x_bf = 0;
    g_tmp_sqr_y_bf = 0;
    g_bf_tmp = 0;
    const auto targets = parameter_bindings();
    for (std::size_t i = 0; i < targets.size(); ++i)
    {
        *targets[i] = m_parameters[i].data();
        if (!preserve_parameters)
        {
            std::memset(m_parameters[i].data(), 0, big_float.persistent.block_size());
        }
    }
    m_bound = true;
}

void MathArenas::unbind()
{
    m_bound = false;
    big_num.stack.restore(0);
    big_float.stack.restore(0);
    big_num.unbind_working();
    g_x_min_bn = BigNum{};
    g_x_max_bn = BigNum{};
    g_y_min_bn = BigNum{};
    g_y_max_bn = BigNum{};
    g_x_3rd_bn = BigNum{};
    g_y_3rd_bn = BigNum{};
    g_delta_x_bn = BigNum{};
    g_delta_y_bn = BigNum{};
    g_delta2_x_bn = BigNum{};
    g_delta2_y_bn = BigNum{};
    g_param_z_bn = {};
    g_new_z_bn = {};
    g_saved_z_bn = {};
    g_close_enough_bn = BigNum{};
    big_float.unbind_working();
    g_delta_x_bf = BigFloat{};
    g_delta_y_bf = BigFloat{};
    g_delta2_x_bf = BigFloat{};
    g_delta2_y_bf = BigFloat{};
    g_param_z_bf = {};
    g_old_z_bf = {};
    g_new_z_bf = {};
    g_saved_z_bf = {};
    g_close_enough_bf = BigFloat{};
    g_tmp_sqr_x_bf = BigFloat{};
    g_tmp_sqr_y_bf = BigFloat{};
    g_bf_tmp = BigFloat{};
    for (auto target : parameter_bindings())
    {
        *target = nullptr;
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
    const BFMathType math = select_bf_math(g_fractal_type);
    if (!supported_bn_length(bn_length, math))
    {
        throw PrecisionError(fmt::format("Requested precision of {} too high", decimals));
    }
    const int bf_length = bf_length_for_bn_length(bn_length);
    const int rounded_length = bf_length - 4;
    const int padding = rounded_length == 4 ? 4 : 8;
    const bool preserve_parameters = s_arenas && s_arenas->bound();
    if (s_arenas)
    {
        s_arenas->change_precision(rounded_length, bf_length, int_length, padding, preserve_parameters);
    }
    else
    {
        s_arenas = std::make_unique<MathArenas>(rounded_length, bf_length, int_length, padding);
    }
    // All potentially throwing storage preparation has completed.
    g_bn_length = rounded_length;
    g_int_length = int_length;
    g_decimals = decimals;
    calc_lengths();
    g_bf_math = math;
    s_arenas->bind(preserve_parameters);
}

BigArena &bn_arena(const BigLifetime lifetime)
{
    return s_arenas->big_num.get(lifetime);
}

BigArena &bf_arena(const BigLifetime lifetime)
{
    return s_arenas->big_float.get(lifetime);
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

BigStack &bn_stack()
{
    if (!s_arenas || !s_arenas->bound() || g_bf_math == BFMathType::NONE)
    {
        throw std::logic_error("BigNum stack called without initialized arbitrary precision");
    }
    return s_arenas->big_num.stack;
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
    return big_num == right.big_num && big_float == right.big_float;
}

bool BigStackMark::operator!=(const BigStackMark &right) const
{
    return !(*this == right);
}

bool BigStackMark::operator>(const BigStackMark &right) const
{
    return big_num + big_float > right.big_num + right.big_float;
}

BigStackMark save_stack()
{
    return !s_arenas || !s_arenas->bound()
        ? BigStackMark{}
        : BigStackMark{s_arenas->big_num.stack.mark(), s_arenas->big_float.stack.mark()};
}

void restore_stack(const BigStackMark old_mark)
{
    if (s_arenas)
    {
        s_arenas->big_num.stack.restore(old_mark.big_num);
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
            for (const BigArena *arena : {&bn_arena(lifetime), &bf_arena(lifetime)})
            {
                const std::size_t used = arena->high_water_mark() * arena->block_size();
                (lifetime == BigLifetime::TEMPORARY ? result.temporaries : result.globals) += used;
                result.capacity += arena->capacity() * arena->block_size();
            }
        }
    }
    return result;
}

std::vector<char> pack_bf_parameters()
{
    const std::size_t length = static_cast<std::size_t>(g_bf_length) + 2;
    std::vector<char> result(22 * length);
    std::size_t offset = 0;
    for (BigFloatPtr *value : parameter_bindings())
    {
        std::memcpy(result.data() + offset, *value, length);
        offset += length;
    }
    return result;
}

void unpack_bf_parameters(const std::vector<char> &parameters)
{
    const std::size_t length = static_cast<std::size_t>(g_bf_length) + 2;
    if (parameters.size() != 22 * length)
    {
        throw std::invalid_argument("Incomplete arbitrary precision parameter payload");
    }
    std::size_t offset = 0;
    for (BigFloatPtr *value : parameter_bindings())
    {
        std::memcpy(*value, parameters.data() + offset, length);
        offset += length;
    }
}

void free_bf_vars()
{
    if (s_arenas)
    {
        s_arenas->unbind();
    }
    g_bf_pi = nullptr;
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
    // What, don't you recognize the first 700 digits of pi,
    // in base 256, in reverse order?
    static Byte pi_table[] =
    {
        0x44, 0xD5, 0xDB, 0x69, 0x17, 0xDF, 0x2E, 0x56, 0x87, 0x1A,
        0xA0, 0x8C, 0x6F, 0xCA, 0xBB, 0x57, 0x5C, 0x9E, 0x82, 0xDF,
        0x00, 0x3E, 0x48, 0x7B, 0x31, 0x53, 0x60, 0x87, 0x23, 0xFD,
        0xFA, 0xB5, 0x3D, 0x32, 0xAB, 0x52, 0x05, 0xAD, 0xC8, 0x1E,
        0x50, 0x2F, 0x15, 0x6B, 0x61, 0xFD, 0xDF, 0x16, 0x75, 0x3C,
        0xF8, 0x22, 0x32, 0xDB, 0xF8, 0xE9, 0xA5, 0x8E, 0xCC, 0xA3,
        0x1F, 0xFB, 0xFE, 0x25, 0x9F, 0x67, 0x79, 0x72, 0x2C, 0x40,
        0xC6, 0x00, 0xA1, 0xD6, 0x0A, 0x32, 0x60, 0x1A, 0xBD, 0xC0,
        0x79, 0x55, 0xDB, 0xFB, 0xD3, 0xB9, 0x39, 0x5F, 0x0B, 0xD2,
        0x0F, 0x74, 0xC8, 0x45, 0x57, 0xA8, 0xCB, 0xC0, 0xB3, 0x4B,
        0x2E, 0x19, 0x07, 0x28, 0x0F, 0x66, 0xFD, 0x4A, 0x33, 0xDE,
        0x04, 0xD0, 0xE3, 0xBE, 0x09, 0xBD, 0x5E, 0xAF, 0x44, 0x45,
        0x81, 0xCC, 0x2C, 0x95, 0x30, 0x9B, 0x1F, 0x51, 0xFC, 0x6D,
        0x6F, 0xEC, 0x52, 0x3B, 0xEB, 0xB2, 0x39, 0x13, 0xB5, 0x53,
        0x6C, 0x3E, 0xAF, 0x6F, 0xFB, 0x68, 0x63, 0x24, 0x6A, 0x19,
        0xC2, 0x9E, 0x5C, 0x5E, 0xC4, 0x60, 0x9F, 0x40, 0xB6, 0x4F,
        0xA9, 0xC1, 0xBA, 0x06, 0xC0, 0x04, 0xBD, 0xE0, 0x6C, 0x97,
        0x3B, 0x4C, 0x79, 0xB6, 0x1A, 0x50, 0xFE, 0xE3, 0xF7, 0xDE,
        0xE8, 0xF6, 0xD8, 0x79, 0xD4, 0x25, 0x7B, 0x1B, 0x99, 0x80,
        0xC9, 0x72, 0x53, 0x07, 0x9B, 0xC0, 0xF1, 0x49, 0xD3, 0xEA,
        0x0F, 0xDB, 0x48, 0x12, 0x0A, 0xD0, 0x24, 0xD7, 0xD0, 0x37,
        0x3D, 0x02, 0x9B, 0x42, 0x72, 0xDF, 0xFE, 0x1B, 0x06, 0x77,
        0x3F, 0x36, 0x62, 0xAA, 0xD3, 0x4E, 0xA6, 0x6A, 0xC1, 0x56,
        0x9F, 0x44, 0x1A, 0x40, 0x73, 0x20, 0xC1, 0x85, 0xD8, 0x75,
        0x6F, 0xE0, 0xBE, 0x5E, 0x8B, 0x3B, 0xC3, 0xA5, 0x84, 0x7D,
        0xB4, 0x9F, 0x6F, 0x45, 0x19, 0x86, 0xEE, 0x8C, 0x88, 0x0E,
        0x43, 0x82, 0x3E, 0x59, 0xCA, 0x66, 0x76, 0x01, 0xAF, 0x39,
        0x1D, 0x65, 0xF1, 0xA1, 0x98, 0x2A, 0xFB, 0x7E, 0x50, 0xF0,
        0x3B, 0xBA, 0xE4, 0x3B, 0x7A, 0x13, 0x6C, 0x0B, 0xEF, 0x6E,
        0xA3, 0x33, 0x51, 0xAB, 0x28, 0xA7, 0x0F, 0x96, 0x68, 0x2F,
        0x54, 0xD8, 0xD2, 0xA0, 0x51, 0x6A, 0xF0, 0x88, 0xD3, 0xAB,
        0x61, 0x9C, 0x0C, 0x67, 0x9A, 0x6C, 0xE9, 0xF6, 0x42, 0x68,
        0xC6, 0x21, 0x5E, 0x9B, 0x1F, 0x9E, 0x4A, 0xF0, 0xC8, 0x69,
        0x04, 0x20, 0x84, 0xA4, 0x82, 0x44, 0x0B, 0x2E, 0x39, 0x42,
        0xF4, 0x83, 0xF3, 0x6F, 0x6D, 0x0F, 0xC5, 0xAC, 0x96, 0xD3,
        0x81, 0x3E, 0x89, 0x23, 0x88, 0x1B, 0x65, 0xEB, 0x02, 0x23,
        0x26, 0xDC, 0xB1, 0x75, 0x85, 0xE9, 0x5D, 0x5D, 0x84, 0xEF,
        0x32, 0x80, 0xEC, 0x5D, 0x60, 0xAC, 0x7C, 0x48, 0x91, 0xA9,
        0x21, 0xFB, 0xCC, 0x09, 0xD8, 0x61, 0x93, 0x21, 0x28, 0x66,
        0x1B, 0xE8, 0xBF, 0xC4, 0xAF, 0xB9, 0x4B, 0x6B, 0x98, 0x48,
        0x8F, 0x3B, 0x77, 0x86, 0x95, 0x28, 0x81, 0x53, 0x32, 0x7A,
        0x5C, 0xCF, 0x24, 0x6C, 0x33, 0xBA, 0xD6, 0xAF, 0x1E, 0x93,
        0x87, 0x9B, 0x16, 0x3E, 0x5C, 0xCE, 0xF6, 0x31, 0x18, 0x74,
        0x5D, 0xC5, 0xA9, 0x2B, 0x2A, 0xBC, 0x6F, 0x63, 0x11, 0x14,
        0xEE, 0xB3, 0x93, 0xE9, 0x72, 0x7C, 0xAF, 0x86, 0x54, 0xA1,
        0xCE, 0xE8, 0x41, 0x11, 0x34, 0x5C, 0xCC, 0xB4, 0xB6, 0x10,
        0xAB, 0x2A, 0x6A, 0x39, 0xCA, 0x55, 0x40, 0x14, 0xE8, 0x63,
        0x62, 0x98, 0x48, 0x57, 0x94, 0xAB, 0x55, 0xAA, 0xF3, 0x25,
        0x55, 0xE6, 0x60, 0x5C, 0x60, 0x55, 0xDA, 0x2F, 0xAF, 0x78,
        0x27, 0x4B, 0x31, 0xBD, 0xC1, 0x77, 0x15, 0xD7, 0x3E, 0x8A,
        0x1E, 0xB0, 0x8B, 0x0E, 0x9E, 0x6C, 0x0E, 0x18, 0x3A, 0x60,
        0xB0, 0xDC, 0x79, 0x8E, 0xEF, 0x38, 0xDB, 0xB8, 0x18, 0x79,
        0x41, 0xCA, 0xF0, 0x85, 0x60, 0x28, 0x23, 0xB0, 0xD1, 0xC5,
        0x13, 0x60, 0xF2, 0x2A, 0x39, 0xD5, 0x30, 0x9C, 0xB5, 0x59,
        0x5A, 0xC2, 0x1D, 0xA4, 0x54, 0x7B, 0xEE, 0x4A, 0x15, 0x82,
        0x58, 0xCD, 0x8B, 0x71, 0x58, 0xB6, 0x8E, 0x72, 0x8F, 0x74,
        0x95, 0x0D, 0x7E, 0x3D, 0x93, 0xF4, 0xA3, 0xFE, 0x58, 0xA4,
        0x69, 0x4E, 0x57, 0x71, 0xD8, 0x20, 0x69, 0x63, 0x16, 0xFC,
        0x8E, 0x85, 0xE2, 0xF2, 0x01, 0x08, 0xF7, 0x6C, 0x91, 0xB3,
        0x47, 0x99, 0xA1, 0x24, 0x99, 0x7F, 0x2C, 0xF1, 0x45, 0x90,
        0x7C, 0xBA, 0x96, 0x7E, 0x26, 0x6A, 0xED, 0xAF, 0xE1, 0xB8,
        0xB7, 0xDF, 0x1A, 0xD0, 0xDB, 0x72, 0xFD, 0x2F, 0xAC, 0xB5,
        0xDF, 0x98, 0xA6, 0x0B, 0x31, 0xD1, 0x1B, 0xFB, 0x79, 0x89,
        0xD9, 0xD5, 0x16, 0x92, 0x17, 0x09, 0x47, 0xB5, 0xB5, 0xD5,
        0x84, 0x3F, 0xDD, 0x50, 0x7C, 0xC9, 0xB7, 0x29, 0xAC, 0xC0,
        0x6C, 0x0C, 0xE9, 0x34, 0xCF, 0x66, 0x54, 0xBE, 0x77, 0x13,
        0xD0, 0x38, 0xE6, 0x21, 0x28, 0x45, 0x89, 0x6C, 0x4E, 0xEC,
        0x98, 0xFA, 0x2E, 0x08, 0xD0, 0x31, 0x9F, 0x29, 0x22, 0x38,
        0x09, 0xA4, 0x44, 0x73, 0x70, 0x03, 0x2E, 0x8A, 0x19, 0x13,
        0xD3, 0x08, 0xA3, 0x85, 0x88, 0x6A, 0x3F, 0x24,
        /* . */  0x03, 0x00, 0x00, 0x00
        //  <- up to g_int_length 4 ->
        // or BigFloat int length of 2 + 2 byte exp
    };

    int length = g_bf_length + 2; // 2 byte exp
    const int pi_offset = sizeof(pi_table) - length;
    std::memcpy(g_big_pi, pi_table + pi_offset, length);

    g_bf_pi = g_big_pi;
}

} // namespace id::math
