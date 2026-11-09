// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>
#include <engine/calcfrac.h>
#include <engine/fractalb.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigComplex.h>
#include <math/complex_fn.h>

#include <fmt/format.h>

#include <chrono>
#include <cstdlib>
#include <new>
#include <string_view>

#ifdef _WIN32
#include <malloc.h>
#endif

namespace
{

bool s_count_allocations{};
std::size_t s_allocations{};
volatile bool s_result{};

void count_allocation()
{
    if (s_count_allocations)
    {
        ++s_allocations;
    }
}

} // namespace

// Replacement allocation functions are confined to this measurement executable.
void *operator new(const std::size_t size)
{
    count_allocation();
    if (void *value = std::malloc(size == 0 ? 1 : size))
    {
        return value;
    }
    throw std::bad_alloc{};
}

void *operator new[](const std::size_t size)
{
    return ::operator new(size);
}

void operator delete(void *value) noexcept
{
    std::free(value);
}

void operator delete[](void *value) noexcept
{
    ::operator delete(value);
}

void operator delete(void *value, std::size_t) noexcept
{
    ::operator delete(value);
}

void operator delete[](void *value, std::size_t) noexcept
{
    ::operator delete(value);
}

void *operator new(const std::size_t size, const std::align_val_t alignment)
{
    count_allocation();
    const auto align = static_cast<std::size_t>(alignment);
#ifdef _WIN32
    void *value = _aligned_malloc(size == 0 ? 1 : size, align);
#else
    void *value = std::aligned_alloc(align, ((size == 0 ? 1 : size) + align - 1) / align * align);
#endif
    if (value != nullptr)
    {
        return value;
    }
    throw std::bad_alloc{};
}

void *operator new[](const std::size_t size, const std::align_val_t alignment)
{
    return ::operator new(size, alignment);
}

void operator delete(void *value, std::align_val_t) noexcept
{
#ifdef _WIN32
    _aligned_free(value);
#else
    std::free(value);
#endif
}

void operator delete[](void *value, const std::align_val_t alignment) noexcept
{
    ::operator delete(value, alignment);
}

void operator delete(void *value, std::size_t, const std::align_val_t alignment) noexcept
{
    ::operator delete(value, alignment);
}

void operator delete[](void *value, std::size_t, const std::align_val_t alignment) noexcept
{
    ::operator delete(value, alignment);
}

namespace
{

using namespace id::math;
using namespace id::engine;
using namespace id::fractals;

template <typename Task>
void measure(const std::string_view name, const int iterations, Task task)
{
    for (int i = 0; i < 16; ++i)
    {
        task();
    }
    const auto before = big_memory_stats();
    s_allocations = 0;
    s_count_allocations = true;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        task();
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    s_count_allocations = false;
    const auto after = big_memory_stats();
    fmt::print("{},{},{},{:.3f},{}\n", name, iterations, s_allocations,
        std::chrono::duration<double, std::milli>(elapsed).count(),
        after.globals + after.temporaries - before.globals - before.temporaries);
}

template <typename Value>
void measure_values(const std::string_view scalar_name, const std::string_view complex_name)
{
    const Value left{0.125L};
    const Value right{-0.25L};
    const Value limit{4};
    const id::math::Complex<Value> base{left, right};
    measure(scalar_name, 10000, [&] { s_result = (left * right + left).sqr() < limit; });
    measure(complex_name, 10000, [&] { s_result = mag_squared(power(base, 7)) < limit; });
}

} // namespace

int main()
{
    g_bf_digits = 0;
    g_user.bailout_value = 0;
    g_magnitude_limit = 4;
    set_bailout_formula(Bailout::MOD);
    fmt::print("case,iterations,heap_allocations,milliseconds,arena_high_water_growth\n");
    g_fractal_type = FractalType::MANDEL;
    init_bf_dec(40);
    measure_values<BigNum>("bn_scalar", "bn_complex_power");
    float_to_bn(g_old_z_bn.x, 0.25);
    float_to_bn(g_old_z_bn.y, 0.125);
    float_to_bn(g_param_z_bn.x.data(), -0.2);
    float_to_bn(g_param_z_bn.y.data(), 0.4);
    square_bn(g_tmp_sqr_x_bn, g_old_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_old_z_bn.y);
    measure("bn_julia_orbit", 10000, [] { s_result = julia_orbit_bn() != 0; });

    g_fractal_type = FractalType::JULIA_Z_POWER;
    init_bf_dec(40);
    init_big_pi();
    measure_values<BigFloat>("bf_scalar", "bf_complex_power");
    float_to_bf(g_old_z_bf.x.data(), 0.25);
    float_to_bf(g_old_z_bf.y.data(), 0.125);
    float_to_bf(g_param_z_bf.x, -0.2);
    float_to_bf(g_param_z_bf.y, 0.4);
    square_bf(g_tmp_sqr_x_bf.data(), g_old_z_bf.x.data());
    square_bf(g_tmp_sqr_y_bf.data(), g_old_z_bf.y.data());
    measure("bf_julia_orbit", 10000, [] { s_result = julia_orbit_bf() != 0; });
    g_params[2] = 2.5;
    g_params[3] = 0;
    measure("bf_julia_power_orbit", 250, [] { s_result = julia_z_power_orbit_bf() != 0; });
    free_bf_vars();
    return 0;
}
