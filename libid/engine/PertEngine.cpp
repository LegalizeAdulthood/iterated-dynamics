// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Per-pixel perturbation orbit calculation
//
// Thanks to Claude Heiland-Allen
// <https://fractalforums.org/programming/11/perturbation-code-for-cubic-and-higher-order-polynomials/2783>
//
// Much of perturbation code is based on the work of Shirom Makkad
// <https://github.com/ShiromMakkad/MandelbrotPerturbation>

#include "engine/PertEngine.h"

#include "engine/bailout_formula.h"
#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/ImageRegion.h"
#include "engine/Inversion.h"
#include "engine/log_map.h"
#include "engine/LogicalScreen.h"
#include "engine/OrbitPlot.h"
#include "engine/Potential.h"
#include "engine/SoundOutput.h"
#include "engine/VideoInfo.h"
#include "fractals/fractalp.h"
#include "fractals/pickover_mandelbrot.h"
#include "math/biginit.h"
#include "math/complex_fn.h"
#include "misc/id.h"
#include "misc/run_cleanup.h"
#include "misc/ValueSaver.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>
#include <stdexcept>

using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::engine
{

namespace
{

constexpr int PERTURBATION_RETRY{-3};
constexpr std::size_t REFERENCE_CHUNK_SIZE{2048};
constexpr std::size_t PIXEL_CHUNK_SIZE{2048};

bool conjugated_orbit()
{
    return g_fractal_type == FractalType::BURNING_SHIP && static_cast<int>(g_params[2]) > 2;
}

std::complex<double> native_orbit_value(const std::complex<double> &value)
{
    // Higher-degree Burning Ship references use the conjugate native orbit.
    // Keep recurrence state internal; convert only when observing its values.
    return conjugated_orbit() ? std::conj(value) : value;
}

std::complex<double> star_trail_point(const std::complex<double> &z)
{
    constexpr double LIMIT{std::numeric_limits<float>::max()};
    return {std::max(std::min(z.real(), LIMIT), -LIMIT), std::max(std::min(z.imag(), LIMIT), -LIMIT)};
}

void clamp_star_trail_point(const BFComplex &z)
{
    const std::complex<double> value{static_cast<double>(bf_to_float(z.x)), static_cast<double>(bf_to_float(z.y))};
    const std::complex<double> bounded{star_trail_point(value)};
    if (bounded.real() != value.real())
    {
        float_to_bf(z.x, bounded.real());
    }
    if (bounded.imag() != value.imag())
    {
        float_to_bf(z.y, bounded.imag());
    }
}

bool bof_initialization()
{
    // other_mandel_per_pixel does not apply BOF seeding.
    return (g_bf_math != BFMathType::NONE ||
               (g_fractal_type != FractalType::MANDEL_Z_POWER && g_fractal_type != FractalType::BURNING_SHIP)) &&
        (g_inside_method == ColorMethod::BOF60 || g_inside_method == ColorMethod::BOF61) && g_bof_match_book_images;
}

bool explicit_initial_orbit()
{
    // Burning Ship's BIG_FLT initializer ignores initorbit.
    return g_use_init_orbit == InitOrbitMode::VALUE &&
        (g_fractal_type != FractalType::BURNING_SHIP || g_bf_math == BFMathType::NONE);
}

std::complex<double> initialize_orbit(const std::complex<double> &coordinate)
{
    std::complex<double> value{g_param_z1.x, g_param_z1.y};
    if (!bof_initialization())
    {
        const std::complex<double> native_coordinate{
            g_fractal_type == FractalType::BURNING_SHIP ? std::conj(coordinate) : coordinate};
        value += explicit_initial_orbit() ? std::complex<double>{g_init_orbit.x, g_init_orbit.y} : native_coordinate;
    }
    // Conjugation also converts the native seed to the internal orbit.
    return native_orbit_value(value);
}

void initialize_orbit_bf(const BFComplex z, const BFComplex &coordinate)
{
    BigStackSaver saved;
    const BigFloat offset{alloc_stack(g_r_bf_length + 2)};
    float_to_bf(z.x, g_params[0]);
    float_to_bf(z.y, g_params[1]);
    if (!bof_initialization())
    {
        if (explicit_initial_orbit())
        {
            float_to_bf(offset, g_init_orbit.x);
            add_a_bf(z.x, offset);
            float_to_bf(offset, g_init_orbit.y);
            add_a_bf(z.y, offset);
        }
        else
        {
            add_a_bf(z.x, coordinate.x);
            if (g_fractal_type == FractalType::BURNING_SHIP)
            {
                sub_a_bf(z.y, coordinate.y);
            }
            else
            {
                add_a_bf(z.y, coordinate.y);
            }
        }
    }
    if (conjugated_orbit())
    {
        neg_a_bf(z.y);
    }
}

double reference_conversion_error()
{
    // bf_to_float reads whole bytes after the leading byte.
    constexpr int REFERENCE_BITS{8 * (std::numeric_limits<LDouble>::digits / 8)};
    return std::ldexp(1.0, -REFERENCE_BITS) + std::numeric_limits<double>::epsilon();
}

double bailout_limit()
{
    return g_bf_math == BFMathType::BIG_NUM ? static_cast<long>(g_magnitude_limit) : g_magnitude_limit;
}

bool relative_delta(const BFComplex &value, const BFComplex &reference, std::complex<double> &delta)
{
    BigStackSaver saved;
    const BFComplex difference{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    sub_bf(difference.x, value.x, reference.x);
    sub_bf(difference.y, value.y, reference.y);
    delta = {static_cast<double>(bf_to_float(difference.x)), static_cast<double>(bf_to_float(difference.y))};
    // Subnormal displacements do not retain full relative precision.
    return (std::isnormal(delta.real()) || is_bf_zero(difference.x)) &&
        (std::isnormal(delta.imag()) || is_bf_zero(difference.y));
}

bool ambiguous_bailout(const std::complex<double> &reference, const std::complex<double> &delta)
{
    const double scale{
        std::abs(reference.real()) + std::abs(reference.imag()) + std::abs(delta.real()) + std::abs(delta.imag())};
    // Include conversion truncation, reconstruction, and predicate roundoff.
    const double error{8.0 * reference_conversion_error() * scale * scale};
    const std::complex<double> value{native_orbit_value(reference + delta)};
    const double x{sqr(value.real())};
    const double y{sqr(value.imag())};
    const double limit{bailout_limit()};
    const auto near_limit = [error, limit](const double metric)
    { return !std::isfinite(metric) || std::abs(metric - limit) <= error; };
    switch (g_bailout_test)
    {
    case Bailout::MOD:
        return near_limit(x + y);
    case Bailout::REAL:
        return near_limit(x);
    case Bailout::IMAG:
        return near_limit(y);
    case Bailout::OR:
    case Bailout::AND:
        return near_limit(x) || near_limit(y);
    case Bailout::MANH:
        return near_limit(sqr(std::abs(value.real()) + std::abs(value.imag())));
    case Bailout::MANR:
        return near_limit(sqr(value.real() + value.imag()));
    }
    throw std::logic_error("Unknown bailout test");
}

bool precise_bailout_reached(const BFComplex &z)
{
    BigStackSaver saved;
    const BigFloat x{alloc_stack(g_r_bf_length + 2)};
    const BigFloat y{alloc_stack(g_r_bf_length + 2)};
    const BigFloat metric{alloc_stack(g_r_bf_length + 2)};
    const BigFloat limit{alloc_stack(g_r_bf_length + 2)};
    float_to_bf(limit, bailout_limit());
    const auto reached = [limit](const BigFloat value)
    {
        const int comparison{cmp_bf(value, limit)};
        return g_bf_math == BFMathType::BIG_FLT ? comparison > 0 : comparison >= 0;
    };
    square_bf(x, z.x);
    square_bf(y, z.y);
    switch (g_bailout_test)
    {
    case Bailout::MOD:
        add_bf(metric, x, y);
        break;
    case Bailout::REAL:
        return reached(x);
    case Bailout::IMAG:
        return reached(y);
    case Bailout::OR:
        return reached(x) || reached(y);
    case Bailout::AND:
        return reached(x) && reached(y);
    case Bailout::MANH:
        abs_bf(x, z.x);
        abs_bf(y, z.y);
        add_bf(metric, x, y);
        square_bf(metric, metric);
        break;
    case Bailout::MANR:
        if (conjugated_orbit())
        {
            sub_bf(metric, z.x, z.y);
        }
        else
        {
            add_bf(metric, z.x, z.y);
        }
        square_bf(metric, metric);
        break;
    default:
        throw std::logic_error("Unknown bailout test");
    }
    return reached(metric);
}

} // namespace

std::size_t PertEngine::reference_sample_count(const long max_iterations)
{
    if (max_iterations <= 0)
    {
        throw std::logic_error{"Perturbation requires a positive iteration limit"};
    }
    constexpr std::size_t BYTES_PER_SAMPLE{sizeof(std::complex<double>) + sizeof(double)};
    const std::size_t max_samples{std::min({std::vector<std::complex<double>>{}.max_size(),
        std::vector<double>{}.max_size(), std::numeric_limits<std::size_t>::max() / BYTES_PER_SAMPLE})};
    const std::uintmax_t iterations{static_cast<std::uintmax_t>(max_iterations)};
    if (iterations >= max_samples)
    {
        throw std::length_error{"Perturbation reference buffers exceed addressable storage"};
    }
    return static_cast<std::size_t>(iterations) + 1;
}

BFComplex PertEngine::initialize_frame_bf(
    const std::complex<double> &half_horizontal, const std::complex<double> &half_vertical)
{
    reset_for_frame(half_horizontal, half_vertical);
    m_saved_stack = save_stack();
    m_center_stack_saved = true;
    m_center_bf.x = alloc_stack(g_bf_length + 2);
    m_center_bf.y = alloc_stack(g_bf_length + 2);
    m_half_horizontal_bf = {alloc_stack(g_bf_length + 2), alloc_stack(g_bf_length + 2)};
    m_half_vertical_bf = {alloc_stack(g_bf_length + 2), alloc_stack(g_bf_length + 2)};
    float_to_bf(m_half_horizontal_bf.x, half_horizontal.real());
    float_to_bf(m_half_horizontal_bf.y, half_horizontal.imag());
    float_to_bf(m_half_vertical_bf.x, half_vertical.real());
    float_to_bf(m_half_vertical_bf.y, half_vertical.imag());
    return m_center_bf;
}

void PertEngine::initialize_frame_bf(const BFComplex &min, const BFComplex &max, const BFComplex &third)
{
    initialize_frame_bf({}, {});
    add_bf(m_center_bf.x, min.x, max.x);
    half_a_bf(m_center_bf.x);
    add_bf(m_center_bf.y, min.y, max.y);
    half_a_bf(m_center_bf.y);
    if (g_fractal_type == FractalType::BURNING_SHIP)
    {
        neg_a_bf(m_center_bf.y);
    }
    sub_bf(m_half_horizontal_bf.x, max.x, third.x);
    half_a_bf(m_half_horizontal_bf.x);
    sub_bf(m_half_horizontal_bf.y, min.y, third.y);
    half_a_bf(m_half_horizontal_bf.y);
    sub_bf(m_half_vertical_bf.x, min.x, third.x);
    half_a_bf(m_half_vertical_bf.x);
    sub_bf(m_half_vertical_bf.y, max.y, third.y);
    half_a_bf(m_half_vertical_bf.y);
    m_half_horizontal = {static_cast<double>(bf_to_float(m_half_horizontal_bf.x)),
        static_cast<double>(bf_to_float(m_half_horizontal_bf.y))};
    m_half_vertical = {
        static_cast<double>(bf_to_float(m_half_vertical_bf.x)), static_cast<double>(bf_to_float(m_half_vertical_bf.y))};
}

void PertEngine::initialize_frame(const std::complex<double> &center, const std::complex<double> &half_horizontal,
    const std::complex<double> &half_vertical)
{
    reset_for_frame(half_horizontal, half_vertical);
    m_center = center;
}

bool PertEngine::initialize_pixel_strategy()
{
    if (m_explicit_orbit && (!m_pixel.active || m_pixel.phase != PixelState::Phase::COMPLETE))
    {
        throw std::logic_error("Cannot change pixels during an incomplete perturbation orbit");
    }
    if (!m_reference.explicit_orbit && m_frame_initialized && m_reference_ready)
    {
        return true;
    }
    const ValueSaver saved_iterations{
        g_max_iterations, g_inside_method == ColorMethod::STAR_TRAIL ? STAR_TRAIL_ITERATIONS : g_max_iterations};
    try
    {
        if (m_reference.explicit_orbit)
        {
            clear_point();
            m_explicit_orbit = false;
            set_reference_point({(g_logical_screen.x_dots - 1) / 2.0, (g_logical_screen.y_dots - 1) / 2.0});
        }
        if (!m_frame_initialized)
        {
            initialize_pixel_state();
            set_reference_point({(g_logical_screen.x_dots - 1) / 2.0, (g_logical_screen.y_dots - 1) / 2.0});
            m_frame_initialized = true;
        }
        return iterate_reference();
    }
    catch (...)
    {
        run_cleanup(std::current_exception(), [this] { cleanup(); });
        throw;
    }
}

int PertEngine::calculate_pixel(const Point &pt, const bool incremental)
{
    if (m_explicit_orbit || m_reference.explicit_orbit)
    {
        initialize_pixel_strategy();
    }
    m_orbit_overlay_displayed = false;
    const ValueSaver saved_iterations{
        g_max_iterations, g_inside_method == ColorMethod::STAR_TRAIL ? STAR_TRAIL_ITERATIONS : g_max_iterations};
    std::size_t remaining{PIXEL_CHUNK_SIZE};
    for (;;)
    {
        if (!m_reference_ready && !initialize_pixel_strategy())
        {
            if (incremental)
            {
                return CALC_YIELDED;
            }
            continue;
        }
        int color;
        try
        {
            color = calculate_point(pt, remaining);
        }
        catch (...)
        {
            run_cleanup(std::current_exception(), [this] { clear_output(); }, [this] { clear_point(); });
            throw;
        }
        if (color == CALC_YIELDED)
        {
            if (incremental || m_orbit_output_pending || (m_orbit_output_started && !g_sound->ready()))
            {
                return color;
            }
            remaining = PIXEL_CHUNK_SIZE;
            continue;
        }
        clear_point();
        if (color != PERTURBATION_RETRY)
        {
            clear_output();
            return color;
        }
        try
        {
            set_reference_point({static_cast<double>(pt.get_x()), static_cast<double>(pt.get_y())});
        }
        catch (...)
        {
            run_cleanup(std::current_exception(), [this] { cleanup(); });
            throw;
        }
    }
}

int PertEngine::calculate_pixel(const int col, const int row, const bool incremental)
{
    return calculate_pixel(Point{col, g_logical_screen.y_dots - 1 - row}, incremental);
}

void PertEngine::begin_orbit(
    const std::complex<double> &parameter, const std::complex<double> &value, const long iteration)
{
    if (m_done || iteration < 0 || iteration > g_max_iterations)
    {
        throw std::logic_error("Perturbation continuation requires a frame and a valid iteration position");
    }
    if ((m_pixel.active && m_pixel.phase != PixelState::Phase::COMPLETE) || (m_explicit_orbit && !m_pixel.active))
    {
        throw std::logic_error("Cannot replace an incomplete perturbation orbit");
    }
    clear_point();
    m_orbit_input = {parameter, value, iteration};
    m_orbit_value = value;
    m_orbit_iteration = iteration;
    m_orbit_escaped = false;
    m_explicit_orbit = true;
    try
    {
        if (!m_frame_initialized)
        {
            initialize_pixel_state();
            m_frame_initialized = true;
        }
        if (!m_reference.explicit_orbit || iteration < m_reference.iteration)
        {
            set_reference_orbit();
        }
    }
    catch (...)
    {
        run_cleanup(std::current_exception(), [this] { cleanup(); });
        throw;
    }
}

void PertEngine::begin_orbit_bf(const BFComplex &parameter, const BFComplex &value, const long iteration)
{
    if (g_bf_math == BFMathType::NONE)
    {
        throw std::logic_error("Big-float orbit continuation requires arbitrary precision");
    }
    const bool replace_reference{!m_reference.explicit_orbit || iteration != m_reference.iteration};
    begin_orbit(std::complex<double>{static_cast<double>(bf_to_float(parameter.x)),
                    static_cast<double>(bf_to_float(parameter.y))},
        std::complex<double>{static_cast<double>(bf_to_float(value.x)), static_cast<double>(bf_to_float(value.y))},
        iteration);
    try
    {
        copy_bf(m_orbit_parameter_bf.x, parameter.x);
        copy_bf(m_orbit_parameter_bf.y, parameter.y);
        copy_bf(m_orbit_initial_bf.x, value.x);
        copy_bf(m_orbit_initial_bf.y, value.y);
        m_orbit_input.precise = true;
        if (replace_reference)
        {
            set_reference_orbit();
        }
    }
    catch (...)
    {
        run_cleanup(std::current_exception(), [this] { cleanup(); });
        throw;
    }
}

void PertEngine::begin_orbit_bn(const BNComplex &parameter, const BNComplex &value, const long iteration)
{
    if (g_bf_math != BFMathType::BIG_NUM)
    {
        throw std::logic_error("Fixed-point orbit continuation requires fixed-point math");
    }
    const bool replace_reference{!m_reference.explicit_orbit || iteration != m_reference.iteration};
    begin_orbit(std::complex<double>{static_cast<double>(bn_to_float(parameter.x)),
                    static_cast<double>(bn_to_float(parameter.y))},
        std::complex<double>{static_cast<double>(bn_to_float(value.x)), static_cast<double>(bn_to_float(value.y))},
        iteration);
    try
    {
        bn_to_bf(m_orbit_parameter_bf.x, parameter.x);
        bn_to_bf(m_orbit_parameter_bf.y, parameter.y);
        bn_to_bf(m_orbit_initial_bf.x, value.x);
        bn_to_bf(m_orbit_initial_bf.y, value.y);
        m_orbit_input.precise = true;
        if (replace_reference)
        {
            set_reference_orbit();
        }
    }
    catch (...)
    {
        run_cleanup(std::current_exception(), [this] { cleanup(); });
        throw;
    }
}

bool PertEngine::advance_bailout()
{
    if (g_fractal_type != FractalType::MANDEL)
    {
        throw std::logic_error("Cached-square continuation requires Mandelbrot");
    }
    bool escaped;
    if (g_bf_math != BFMathType::NONE)
    {
        std::size_t remaining{PIXEL_CHUNK_SIZE};
        if (!advance_precise_orbit(m_pixel.sample, remaining))
        {
            return false;
        }
        {
            BigStackSaver saved;
            const BigFloat real{alloc_stack(g_r_bf_length + 2)};
            const BigFloat imag{alloc_stack(g_r_bf_length + 2)};
            square_bf(real, m_pixel.precise_orbit.x);
            square_bf(imag, m_pixel.precise_orbit.y);
            sub_bf(real, real, imag);
            add_bf(m_pixel.precise_orbit.x, real, m_pixel.precise_coordinate.x);
        }
        m_orbit_value = {static_cast<double>(bf_to_float(m_pixel.precise_orbit.x)),
            static_cast<double>(bf_to_float(m_pixel.precise_orbit.y))};
        escaped = precise_bailout_reached(m_pixel.precise_orbit);
        if (!escaped && m_orbit_input.precise)
        {
            copy_bf(m_orbit_initial_bf.x, m_pixel.precise_orbit.x);
            copy_bf(m_orbit_initial_bf.y, m_pixel.precise_orbit.y);
        }
    }
    else
    {
        m_orbit_value.real(sqr(m_orbit_value.real()) - sqr(m_orbit_value.imag()) + m_orbit_input.parameter.real());
        escaped = bailout_reached(g_bailout_test, m_orbit_value.real(), m_orbit_value.imag(), g_magnitude_limit);
    }
    ++m_orbit_iteration;
    m_orbit_escaped = escaped;
    m_pixel.w = m_orbit_value;
    m_pixel.iteration = m_orbit_iteration;
    if (!escaped)
    {
        m_orbit_input.value = m_orbit_value;
        m_orbit_input.iteration = m_orbit_iteration;
        clear_point();
        set_reference_orbit();
    }
    return true;
}

bool PertEngine::iterate_orbit(const bool continue_bailout)
{
    if (!m_explicit_orbit)
    {
        throw std::logic_error("Perturbation continuation requires begin_orbit");
    }
    try
    {
        if (continue_bailout && m_orbit_escaped && m_orbit_iteration < g_max_iterations)
        {
            return advance_bailout() && (m_orbit_escaped || m_orbit_iteration == g_max_iterations);
        }
        if (!iterate_reference())
        {
            return false;
        }
        std::size_t remaining{PIXEL_CHUNK_SIZE};
        for (;;)
        {
            const int result{calculate_point(Point{}, remaining)};
            if (result != PERTURBATION_RETRY)
            {
                if (m_pixel.phase == PixelState::Phase::ORBIT || m_pixel.phase == PixelState::Phase::COMPLETE)
                {
                    m_orbit_value = m_pixel.w;
                    m_orbit_iteration = m_pixel.iteration;
                }
                return result != CALC_YIELDED;
            }
            if (m_orbit_input.precise)
            {
                m_pixel.phase = PixelState::Phase::REBASE;
                const std::size_t sample{static_cast<std::size_t>(m_orbit_iteration - m_reference.iteration)};
                if (!advance_precise_orbit(sample, remaining))
                {
                    return false;
                }
                copy_bf(m_orbit_initial_bf.x, m_pixel.precise_orbit.x);
                copy_bf(m_orbit_initial_bf.y, m_pixel.precise_orbit.y);
            }
            // Rebase at the last accepted state, never publish the failed update.
            m_orbit_input.value = m_orbit_value;
            m_orbit_input.iteration = m_orbit_iteration;
            clear_point();
            set_reference_orbit();
            if (!iterate_reference())
            {
                return false;
            }
        }
    }
    catch (...)
    {
        run_cleanup(std::current_exception(), [this] { cleanup(); });
        throw;
    }
}

std::complex<double> PertEngine::orbit_value() const
{
    if (!m_explicit_orbit)
    {
        throw std::logic_error("Perturbation continuation requires begin_orbit");
    }
    return m_orbit_value;
}

long PertEngine::orbit_iteration() const
{
    if (!m_explicit_orbit)
    {
        throw std::logic_error("Perturbation continuation requires begin_orbit");
    }
    return m_orbit_iteration;
}

bool PertEngine::orbit_escaped() const
{
    if (!m_explicit_orbit)
    {
        throw std::logic_error("Perturbation continuation requires begin_orbit");
    }
    return m_orbit_escaped;
}

void PertEngine::finish_orbit()
{
    if (!m_explicit_orbit)
    {
        throw std::logic_error("Perturbation continuation requires begin_orbit");
    }
    clear_point();
    m_explicit_orbit = false;
    m_orbit_escaped = false;
}

bool PertEngine::orbit_overlay_displayed() const
{
    return m_orbit_overlay_displayed;
}

void PertEngine::set_glitch_tolerance(const double tolerance)
{
    m_glitch_tolerance = tolerance;
}

double PertEngine::glitch_tolerance_threshold(const std::complex<double> &value) const
{
    return mag_squared(value * m_glitch_tolerance);
}

int PertEngine::reference_count() const
{
    return m_reference_count;
}

const DistanceEstimator &PertEngine::distance_estimator() const
{
    return m_distance_estimator;
}

void PertEngine::finish()
{
    if (!m_done)
    {
        cleanup();
    }
}

void PertEngine::cleanup()
{
    if (m_center_stack_saved)
    {
        restore_stack(m_saved_stack);
        m_center_stack_saved = false;
    }
    m_reference = {};
    m_orbit_input = {};
    m_orbit_value = {};
    m_orbit_iteration = 0;
    m_explicit_orbit = false;
    m_orbit_escaped = false;
    m_pixel = {};
    m_reference_count = 0;
    m_cycle_len = -1;
    m_distance_estimator = {};
    m_reference_coordinate_bf = {};
    m_reference_z_bf = {};
    m_reference_initial_bf = {};
    m_orbit_parameter_bf = {};
    m_orbit_initial_bf = {};
    m_reference_z = {};
    m_reference_sample = 0;
    m_reference_samples = 0;
    m_reference_ready = false;
    m_tmp_bf = {};
    m_center_bf = {};
    m_half_horizontal_bf = {};
    m_half_vertical_bf = {};
    m_done = true;
    m_frame_initialized = false;
    clear_output();
    m_orbit_overlay_displayed = false;
}

void PertEngine::initialize_pixel_state()
{
    if (g_logical_screen.x_dots <= 0 || g_logical_screen.y_dots <= 0 || g_max_iterations <= 0)
    {
        throw std::logic_error{"Perturbation requires image dimensions and a positive iteration limit"};
    }

    if (g_bf_math != BFMathType::NONE && g_fractal_type == FractalType::MANDEL_Z_POWER &&
        mandel_z_power_uses_complex_power())
    {
        init_big_pi();
    }
    allocate_working_values();
}

void PertEngine::allocate_working_values()
{
    if (g_bf_math != BFMathType::NONE)
    {
        m_reference_coordinate_bf.x = alloc_stack(g_r_bf_length + 2);
        m_reference_coordinate_bf.y = alloc_stack(g_r_bf_length + 2);
        m_reference_z_bf.x = alloc_stack(g_r_bf_length + 2);
        m_reference_z_bf.y = alloc_stack(g_r_bf_length + 2);
        m_reference_initial_bf = {alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
        m_orbit_parameter_bf = {alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
        m_orbit_initial_bf = {alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
        m_tmp_bf = alloc_stack(g_r_bf_length + 2);
    }
}

void PertEngine::reset_for_frame(const std::complex<double> &half_horizontal, const std::complex<double> &half_vertical)
{
    if (!m_done)
    {
        cleanup();
    }
    m_half_horizontal = half_horizontal;
    m_half_vertical = half_vertical;
    m_done = false;
    m_frame_initialized = false;
}

std::complex<double> PertEngine::pixel_delta(const std::complex<double> &pixel_position) const
{
    const double width{g_logical_screen.x_dots - 1.0};
    const double height{g_logical_screen.y_dots - 1.0};
    const double x{width == 0.0 ? 0.0 : 2.0 * pixel_position.real() / width - 1.0};
    const double y{height == 0.0 ? 0.0 : 2.0 * pixel_position.imag() / height - 1.0};
    const std::complex<double> delta{m_half_horizontal * x + m_half_vertical * y};
    if (g_fractal_type == FractalType::BURNING_SHIP)
    {
        return std::conj(delta);
    }
    return delta;
}

std::complex<double> PertEngine::inverted_coordinate(const std::complex<double> &pixel_position) const
{
    const double col{pixel_position.real()};
    const double row{g_logical_screen.y_dots - 1 - pixel_position.imag()};
    const double width{g_logical_screen.x_dots - 1.0};
    const double height{g_logical_screen.y_dots - 1.0};
    DComplex coordinate{g_image_region.m_min.x + col * ((g_image_region.m_max.x - g_image_region.m_3rd.x) / width) +
            row * ((g_image_region.m_3rd.x - g_image_region.m_min.x) / height),
        g_image_region.m_max.y - row * ((g_image_region.m_max.y - g_image_region.m_3rd.y) / height) -
            col * ((g_image_region.m_3rd.y - g_image_region.m_min.y) / width)};
    g_inversion.apply(coordinate);
    if (g_fractal_type == FractalType::BURNING_SHIP)
    {
        coordinate.y = -coordinate.y;
    }
    return {coordinate.x, coordinate.y};
}

void PertEngine::inverted_coordinate(const BFComplex coordinate, const std::complex<double> &pixel_position) const
{
    BigStackSaver saved;
    const BigFloat horizontal{alloc_stack(g_r_bf_length + 2)};
    const BigFloat vertical{alloc_stack(g_r_bf_length + 2)};
    const BigFloat scale{alloc_stack(g_r_bf_length + 2)};
    const BigFloat delta{alloc_stack(g_r_bf_length + 2)};
    float_to_bf(horizontal, pixel_position.real());
    float_to_bf(scale, g_logical_screen.x_dots - 1);
    div_bf(horizontal, horizontal, scale);
    float_to_bf(vertical, g_logical_screen.y_dots - 1 - pixel_position.imag());
    float_to_bf(scale, g_logical_screen.y_dots - 1);
    div_bf(vertical, vertical, scale);

    sub_bf(coordinate.x, g_bf_x_max, g_bf_x_3rd);
    mult_bf(coordinate.x, coordinate.x, horizontal);
    sub_bf(delta, g_bf_x_3rd, g_bf_x_min);
    mult_bf(delta, delta, vertical);
    add_a_bf(coordinate.x, delta);
    add_a_bf(coordinate.x, g_bf_x_min);
    sub_bf(coordinate.y, g_bf_y_3rd, g_bf_y_max);
    mult_bf(coordinate.y, coordinate.y, vertical);
    sub_bf(delta, g_bf_y_min, g_bf_y_3rd);
    mult_bf(delta, delta, horizontal);
    add_a_bf(coordinate.y, delta);
    add_a_bf(coordinate.y, g_bf_y_max);
    g_inversion.apply(coordinate);
    if (g_fractal_type == FractalType::BURNING_SHIP)
    {
        neg_a_bf(coordinate.y);
    }
}

void PertEngine::pixel_delta(const BFComplex delta, const std::complex<double> &pixel_position) const
{
    BigStackSaver saved;
    const BigFloat factor{alloc_stack(g_r_bf_length + 2)};
    const BigFloat divisor{alloc_stack(g_r_bf_length + 2)};
    const BigFloat term{alloc_stack(g_r_bf_length + 2)};
    clear_bf(delta.x);
    clear_bf(delta.y);
    const double width{g_logical_screen.x_dots - 1.0};
    const double height{g_logical_screen.y_dots - 1.0};
    if (width != 0.0)
    {
        float_to_bf(factor, 2.0 * pixel_position.real() - width);
        float_to_bf(divisor, width);
        div_bf(factor, factor, divisor);
        mult_bf(delta.x, m_half_horizontal_bf.x, factor);
        mult_bf(delta.y, m_half_horizontal_bf.y, factor);
    }
    if (height != 0.0)
    {
        float_to_bf(factor, 2.0 * pixel_position.imag() - height);
        float_to_bf(divisor, height);
        div_bf(factor, factor, divisor);
        mult_bf(term, m_half_vertical_bf.x, factor);
        add_a_bf(delta.x, term);
        mult_bf(term, m_half_vertical_bf.y, factor);
        add_a_bf(delta.y, term);
    }
    if (g_fractal_type == FractalType::BURNING_SHIP)
    {
        neg_a_bf(delta.y);
    }
}

bool PertEngine::parameter_delta(const std::complex<double> &pixel_position, std::complex<double> &delta) const
{
    if (g_bf_math == BFMathType::NONE)
    {
        delta = g_inversion.invert != 0 ? inverted_coordinate(pixel_position) - m_reference.coordinate
                                        : pixel_delta(pixel_position) - m_reference.delta;
        return true;
    }
    BigStackSaver saved;
    const BFComplex coordinate{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    coordinate_bf(coordinate, pixel_position);
    return relative_delta(coordinate, m_reference_coordinate_bf, delta);
}

void PertEngine::coordinate_bf(const BFComplex coordinate, const std::complex<double> &pixel_position) const
{
    if (g_inversion.invert != 0)
    {
        inverted_coordinate(coordinate, pixel_position);
    }
    else
    {
        pixel_delta(coordinate, pixel_position);
        add_a_bf(coordinate.x, m_center_bf.x);
        add_a_bf(coordinate.y, m_center_bf.y);
    }
}

void PertEngine::set_reference_point(const std::complex<double> &pixel_position)
{
    reset_reference(reference_sample_count(g_max_iterations));
    m_reference.explicit_orbit = false;
    m_reference.iteration = 0;
    ReferencePoint &reference{m_reference};
    reference.pixel_position = pixel_position;
    reference.delta = pixel_delta(pixel_position);

    if (g_bf_math != BFMathType::NONE)
    {
        coordinate_bf(m_reference_coordinate_bf, pixel_position);
        initialize_orbit_bf(m_reference_z_bf, m_reference_coordinate_bf);
    }
    else
    {
        reference.coordinate =
            g_inversion.invert != 0 ? inverted_coordinate(pixel_position) : m_center + reference.delta;
        m_reference_z = initialize_orbit(reference.coordinate);
    }
}

void PertEngine::set_reference_orbit()
{
    const long remaining{g_max_iterations - m_orbit_input.iteration};
    reset_reference(remaining == 0 ? 1 : reference_sample_count(remaining));
    m_reference.explicit_orbit = true;
    m_reference.iteration = m_orbit_input.iteration;
    m_reference.coordinate = m_orbit_input.parameter;
    m_reference_z = m_orbit_input.value;
    if (g_bf_math != BFMathType::NONE)
    {
        if (m_orbit_input.precise)
        {
            copy_bf(m_reference_coordinate_bf.x, m_orbit_parameter_bf.x);
            copy_bf(m_reference_coordinate_bf.y, m_orbit_parameter_bf.y);
            copy_bf(m_reference_z_bf.x, m_orbit_initial_bf.x);
            copy_bf(m_reference_z_bf.y, m_orbit_initial_bf.y);
        }
        else
        {
            float_to_bf(m_reference_coordinate_bf.x, m_orbit_input.parameter.real());
            float_to_bf(m_reference_coordinate_bf.y, m_orbit_input.parameter.imag());
            float_to_bf(m_reference_z_bf.x, m_orbit_input.value.real());
            float_to_bf(m_reference_z_bf.y, m_orbit_input.value.imag());
        }
        copy_bf(m_reference_initial_bf.x, m_reference_z_bf.x);
        copy_bf(m_reference_initial_bf.y, m_reference_z_bf.y);
    }
}

void PertEngine::reset_reference(const std::size_t samples)
{
    m_reference_ready = false;
    m_reference_sample = 0;
    m_reference_samples = samples;
    ReferencePoint &reference{m_reference};
    reference.orbit.clear();
    reference.tolerance.clear();
    reference.orbit.reserve(samples);
    reference.tolerance.reserve(samples);
}

bool PertEngine::iterate_reference()
{
    if (m_reference_ready)
    {
        return true;
    }
    const std::size_t end{
        m_reference_sample + std::min(REFERENCE_CHUNK_SIZE, m_reference_samples - m_reference_sample)};
    if (g_bf_math != BFMathType::NONE)
    {
        advance_reference_bf(end);
    }
    else
    {
        advance_reference(end);
    }
    if (m_reference_sample == m_reference_samples)
    {
        m_reference_ready = true;
        ++m_reference_count;
    }
    return m_reference_ready;
}

void PertEngine::clear_point()
{
    if (m_pixel.active)
    {
        restore_stack(m_pixel.stack);
    }
    m_pixel = {};
}

void PertEngine::clear_output()
{
    const bool started{m_orbit_output_started};
    m_orbit_output_started = false;
    m_orbit_output_pending = false;
    m_orbit_output_sample = 0;
    if (started)
    {
        orbit_plot().scrub();
    }
}

void PertEngine::begin_point(const Point &pt)
{
    m_pixel.stack = save_stack();
    m_pixel.active = true;
    if (m_explicit_orbit)
    {
        m_pixel.iteration = m_orbit_input.iteration;
        m_pixel.sample = static_cast<std::size_t>(m_pixel.iteration - m_reference.iteration);
        m_pixel.parameter_displacement = m_orbit_input.parameter - m_reference.coordinate;
        m_pixel.orbit_displacement = m_orbit_input.value - m_reference.orbit[m_pixel.sample];
        if (m_orbit_input.precise)
        {
            const bool parameter_fits{
                relative_delta(m_orbit_parameter_bf, m_reference_coordinate_bf, m_pixel.parameter_displacement)};
            const bool orbit_fits{
                relative_delta(m_orbit_initial_bf, m_reference_initial_bf, m_pixel.orbit_displacement)};
            if (!parameter_fits || !orbit_fits)
            {
                m_pixel.phase = PixelState::Phase::REBASE;
            }
        }
        m_pixel.w = m_pixel.old = m_orbit_input.value;
        m_pixel.magnitude = mag_squared(m_pixel.w);
        m_pixel.at_reference = m_pixel.parameter_displacement == std::complex<double>{} &&
            m_pixel.orbit_displacement == std::complex<double>{} && m_pixel.iteration == m_reference.iteration;
        return;
    }
    m_cycle_len = -1;
    m_distance_estimator = g_distance_estimator ? standard_distance_estimator() : DistanceEstimator{};
    if (g_distance_estimator)
    {
        g_magnitude_limit = m_distance_estimator.bailout();
    }
    m_pixel.star_trail = g_inside_method == ColorMethod::STAR_TRAIL;
    m_pixel.high_precision = g_bf_math != BFMathType::NONE;
    m_pixel.check_periodicity =
        g_periodicity_check != 0 && g_inside_method != ColorMethod::ZMAG && g_inside_method != ColorMethod::STAR_TRAIL;
    if (!m_pixel.check_periodicity)
    {
        g_old_color_iter = 2147483647L;
    }
    else if (g_inside_method == ColorMethod::PERIOD)
    {
        g_old_color_iter = g_max_iterations / 5 * 4;
    }
    else if (g_reset_periodicity)
    {
        g_old_color_iter = 255;
    }
    g_old_color_iter = std::max(g_old_color_iter, g_first_saved_and);
    m_pixel.saved_and = g_inside_method == ColorMethod::PERIOD ? 16 : g_first_saved_and;
    if (!m_pixel.high_precision && explicit_initial_orbit())
    {
        m_pixel.saved_z = native_orbit_value({g_init_orbit.x, g_init_orbit.y});
    }
    const ReferencePoint &reference{m_reference};
    m_pixel.position = {static_cast<double>(pt.get_x()), static_cast<double>(pt.get_y())};
    if (!parameter_delta(m_pixel.position, m_pixel.parameter_displacement))
    {
        // A displacement without full double precision needs its own exact reference.
        m_pixel.phase = PixelState::Phase::REBASE;
    }
    m_pixel.bof = bof_initialization();
    m_pixel.orbit_displacement =
        m_pixel.bof || explicit_initial_orbit() ? std::complex<double>{} : m_pixel.parameter_displacement;
    if (g_fractal_type == FractalType::BURNING_SHIP && !conjugated_orbit())
    {
        m_pixel.orbit_displacement = std::conj(m_pixel.orbit_displacement);
    }
    m_pixel.at_reference = m_pixel.position == reference.pixel_position;
    m_pixel.iteration = m_pixel.bof ? -1L : 0L;
    m_pixel.remember_fmod = g_inside_method == ColorMethod::FMODI || g_outside_method == ColorMethod::FMOD;
    m_pixel.w = m_reference.orbit.front() + m_pixel.orbit_displacement;
    m_pixel.old = m_pixel.w;
    m_pixel.magnitude = mag_squared(m_pixel.w);
    if (g_show_orbit && !m_orbit_output_started)
    {
        g_sound->begin_orbit();
        m_orbit_output_started = true;
    }
}

bool PertEngine::advance_precise_orbit(const std::size_t target, std::size_t &remaining)
{
    if (m_pixel.precise_orbit.x == nullptr)
    {
        m_pixel.precise_coordinate = {alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
        m_pixel.precise_orbit = {alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
        if (m_explicit_orbit)
        {
            if (m_orbit_input.precise)
            {
                copy_bf(m_pixel.precise_coordinate.x, m_orbit_parameter_bf.x);
                copy_bf(m_pixel.precise_coordinate.y, m_orbit_parameter_bf.y);
            }
            else
            {
                float_to_bf(m_pixel.precise_coordinate.x, m_orbit_input.parameter.real());
                float_to_bf(m_pixel.precise_coordinate.y, m_orbit_input.parameter.imag());
            }
        }
        else
        {
            coordinate_bf(m_pixel.precise_coordinate, m_pixel.position);
        }
    }
    if (m_pixel.precise_sample == 0 || target < m_pixel.precise_sample)
    {
        if (m_explicit_orbit)
        {
            if (m_orbit_input.precise)
            {
                copy_bf(m_pixel.precise_orbit.x, m_orbit_initial_bf.x);
                copy_bf(m_pixel.precise_orbit.y, m_orbit_initial_bf.y);
            }
            else
            {
                float_to_bf(m_pixel.precise_orbit.x, m_orbit_input.value.real());
                float_to_bf(m_pixel.precise_orbit.y, m_orbit_input.value.imag());
            }
            m_pixel.precise_sample = static_cast<std::size_t>(m_orbit_input.iteration - m_reference.iteration);
        }
        else
        {
            initialize_orbit_bf(m_pixel.precise_orbit, m_pixel.precise_coordinate);
            m_pixel.precise_sample = 0;
        }
    }
    for (; m_pixel.precise_sample < target; ++m_pixel.precise_sample)
    {
        if (remaining == 0)
        {
            return false;
        }
        --remaining;
        if (m_pixel.star_trail && m_pixel.precise_sample > 0)
        {
            clamp_star_trail_point(m_pixel.precise_orbit);
        }
        m_pixel.precise_previous = {static_cast<double>(bf_to_float(m_pixel.precise_orbit.x)),
            static_cast<double>(bf_to_float(m_pixel.precise_orbit.y))};
        g_cur_fractal_specific->pert_ref_bf(m_pixel.precise_coordinate, m_pixel.precise_orbit);
    }
    return true;
}

bool PertEngine::save_precise_value(std::size_t &remaining)
{
    if (m_pixel.precise_saved.x == nullptr)
    {
        m_pixel.precise_saved = {alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
        m_pixel.precise_tolerance = alloc_stack(g_r_bf_length + 2);
        if (g_bf_math == BFMathType::BIG_NUM)
        {
            bn_to_bf(m_pixel.precise_tolerance, g_close_enough_bn);
        }
        else
        {
            copy_bf(m_pixel.precise_tolerance, g_close_enough_bf);
        }
    }
    if (m_pixel.have_saved_sample)
    {
        if (!advance_precise_orbit(m_pixel.saved_sample, remaining))
        {
            return false;
        }
        copy_bf(m_pixel.precise_saved.x, m_pixel.precise_orbit.x);
        copy_bf(m_pixel.precise_saved.y, m_pixel.precise_orbit.y);
    }
    else
    {
        clear_bf(m_pixel.precise_saved.x);
        clear_bf(m_pixel.precise_saved.y);
    }
    m_pixel.saved_is_precise = true;
    return true;
}

int PertEngine::calculate_point(const Point &pt, std::size_t &remaining)
{
    if (!m_pixel.active)
    {
        begin_point(pt);
    }
    else if (m_pixel.position != std::complex<double>{static_cast<double>(pt.get_x()), static_cast<double>(pt.get_y())})
    {
        throw std::logic_error("Cannot change pixels during an incomplete perturbation calculation");
    }
    const ReferencePoint &reference{m_reference};
    for (;;)
    {
        switch (m_pixel.phase)
        {
        case PixelState::Phase::ORBIT:
        {
            if (m_explicit_orbit ? m_pixel.iteration >= g_max_iterations : m_pixel.iteration + 1 >= g_max_iterations)
            {
                if (!m_explicit_orbit)
                {
                    ++m_pixel.iteration;
                }
                m_pixel.phase = PixelState::Phase::COMPLETE;
                continue;
            }
            if (remaining == 0)
            {
                return CALC_YIELDED;
            }
            --remaining;
            ++m_pixel.iteration;
            if (g_cur_fractal_specific->pert_pt == nullptr)
            {
                throw std::runtime_error("No perturbation point function defined for fractal type (" +
                    std::string{g_cur_fractal_specific->name} + ")");
            }
            m_pixel.sample = static_cast<std::size_t>(
                m_pixel.iteration - (m_explicit_orbit ? reference.iteration : 0) + (m_pixel.bof ? 1 : 0));
            m_pixel.old = m_pixel.w;
            const std::complex<double> previous{native_orbit_value(m_pixel.old)};
            if (!m_explicit_orbit && g_distance_estimator &&
                !m_distance_estimator.advance({previous.real(), previous.imag()}))
            {
                m_pixel.phase = PixelState::Phase::COMPLETE;
                continue;
            }
            const std::complex<double> reference_z{m_pixel.star_trail && m_pixel.sample > 1
                    ? star_trail_point(reference.orbit[m_pixel.sample - 1])
                    : reference.orbit[m_pixel.sample - 1]};
            if (m_explicit_orbit && g_bf_math == BFMathType::NONE && g_fractal_type == FractalType::MANDEL_Z_POWER &&
                mandel_z_power_uses_complex_power() && m_pixel.old.real() < 0.0 && m_pixel.old.imag() == 0.0 &&
                reference_z.imag() == 0.0 && std::signbit(m_pixel.old.imag()) != std::signbit(reference_z.imag()))
            {
                // A zero displacement cannot encode the other side of the cut.
                return PERTURBATION_RETRY;
            }
            if (g_bf_math != BFMathType::NONE && g_fractal_type == FractalType::MANDEL_Z_POWER &&
                (g_c_exponent <= 0 || mandel_z_power_uses_complex_power()) &&
                m_pixel.orbit_displacement != std::complex<double>{})
            {
                // Resolve cancellation at a pole, zero, or branch cut from
                // an exact replacement reference.
                const double error{8.0 * reference_conversion_error() *
                    (std::abs(reference_z) + std::abs(m_pixel.orbit_displacement))};
                const std::complex<double> value{reference_z + m_pixel.orbit_displacement};
                if (std::abs(value) <= error ||
                    (mandel_z_power_uses_complex_power() && value.real() <= error && std::abs(value.imag()) <= error))
                {
                    return PERTURBATION_RETRY;
                }
            }
            g_cur_fractal_specific->pert_pt(reference_z, m_pixel.orbit_displacement, m_pixel.parameter_displacement);
            m_pixel.w = reference.orbit[m_pixel.sample] + m_pixel.orbit_displacement;
            m_pixel.magnitude = mag_squared(m_pixel.w);

            // At the reference pixel, zero delta uses the reference orbit directly.
            if (!m_pixel.at_reference &&
                (!std::isfinite(m_pixel.magnitude) || m_pixel.magnitude < reference.tolerance[m_pixel.sample]))
            {
                return PERTURBATION_RETRY;
            }
            m_pixel.phase = PixelState::Phase::BAILOUT;
        }
            [[fallthrough]];

        case PixelState::Phase::BAILOUT:
        {
            const std::complex<double> value{native_orbit_value(m_pixel.w)};
            bool escaped{bailout_reached(
                g_bailout_test, value.real(), value.imag(), bailout_limit(), g_bf_math == BFMathType::BIG_FLT)};
            const bool resolve_zero_power{g_fractal_type == FractalType::MANDEL_Z_POWER && g_c_exponent == 0 &&
                m_pixel.old == std::complex<double>{}};
            const bool resolve_power_branch{g_fractal_type == FractalType::MANDEL_Z_POWER &&
                mandel_z_power_uses_complex_power() && m_pixel.old.real() <= 0.0 &&
                std::abs(m_pixel.old.imag()) <= 8.0 * reference_conversion_error() * std::abs(m_pixel.old)};
            if (g_bf_math != BFMathType::NONE &&
                (resolve_zero_power || resolve_power_branch ||
                    ambiguous_bailout(reference.orbit[m_pixel.sample], m_pixel.orbit_displacement)))
            {
                // Retain only the current high-precision value, not another orbit history.
                if (!advance_precise_orbit(m_pixel.sample, remaining))
                {
                    return CALC_YIELDED;
                }
                m_pixel.old = m_pixel.precise_previous;
                m_pixel.w = {static_cast<double>(bf_to_float(m_pixel.precise_orbit.x)),
                    static_cast<double>(bf_to_float(m_pixel.precise_orbit.y))};
                m_pixel.magnitude = mag_squared(m_pixel.w);
                escaped = precise_bailout_reached(m_pixel.precise_orbit);
                if (m_explicit_orbit || resolve_power_branch)
                {
                    m_pixel.orbit_displacement = m_pixel.w - reference.orbit[m_pixel.sample];
                }
            }
            if (m_explicit_orbit)
            {
                m_orbit_escaped = escaped;
                if (escaped || m_pixel.iteration == g_max_iterations)
                {
                    m_pixel.phase = PixelState::Phase::COMPLETE;
                    continue;
                }
                m_pixel.phase = PixelState::Phase::ORBIT;
                return CALC_YIELDED;
            }
            if (g_inside_method == ColorMethod::BOF60 || g_inside_method == ColorMethod::BOF61)
            {
                if (m_pixel.magnitude < m_pixel.min_orbit)
                {
                    m_pixel.min_orbit = m_pixel.magnitude;
                    m_pixel.min_index = m_pixel.iteration + 1L;
                }
            }
            if (escaped && (g_distance_estimator || !m_pixel.star_trail))
            {
                if (!g_distance_estimator)
                {
                    m_pixel.phase = PixelState::Phase::COMPLETE;
                    continue;
                }
                const std::complex<double> escape{native_orbit_value(m_pixel.w)};
                const bool complete{
                    m_distance_estimator.escaped({escape.real(), escape.imag()}, m_pixel.iteration, m_pixel.magnitude)};
                g_magnitude_limit = m_distance_estimator.bailout();
                if (complete)
                {
                    m_pixel.phase = PixelState::Phase::COMPLETE;
                    continue;
                }
            }
            m_pixel.phase = PixelState::Phase::OUTPUT;
        }
            [[fallthrough]];

        case PixelState::Phase::OUTPUT:
            if (g_show_orbit && m_pixel.sample > m_orbit_output_sample)
            {
                if (!m_orbit_output_started)
                {
                    g_sound->begin_orbit();
                    m_orbit_output_started = true;
                }
                if (!m_orbit_output_pending)
                {
                    const std::complex<double> value{native_orbit_value(m_pixel.w)};
                    orbit_plot().reset(value.real(), value.imag());
                    m_orbit_output_pending = true;
                }
                const bool displayed{orbit_plot().iterate()};
                if (!orbit_plot().done())
                {
                    return CALC_YIELDED;
                }
                m_orbit_output_pending = false;
                m_orbit_output_sample = m_pixel.sample;
                m_pixel.phase = PixelState::Phase::STATISTICS;
                if (displayed)
                {
                    m_orbit_overlay_displayed = true;
                    return CALC_YIELDED;
                }
            }
            else
            {
                m_orbit_output_pending = false;
                m_orbit_output_sample = std::max(m_orbit_output_sample, m_pixel.sample);
                m_pixel.phase = PixelState::Phase::STATISTICS;
            }
            [[fallthrough]];

        case PixelState::Phase::STATISTICS:
        {
            if (m_pixel.star_trail)
            {
                const std::complex<double> bounded{star_trail_point(m_pixel.w)};
                const std::complex<double> bounded_reference{star_trail_point(reference.orbit[m_pixel.sample])};
                if (bounded != m_pixel.w || bounded_reference != reference.orbit[m_pixel.sample])
                {
                    m_pixel.orbit_displacement = bounded - bounded_reference;
                }
                m_pixel.w = bounded;
                m_pixel.magnitude = mag_squared(m_pixel.w);
                const int color{static_cast<int>((m_pixel.iteration - 1) % g_and_color + 1)};
                m_pixel.tan_table[color - 1] = m_pixel.w.imag() / (m_pixel.w.real() + 0.000001);
            }
            if (g_inside_method == ColorMethod::EPS_CROSS)
            {
                if (std::abs(m_pixel.w.real()) < std::abs(g_close_proximity))
                {
                    m_pixel.epsilon_axis = 1;
                }
                else if (std::abs(m_pixel.w.imag()) < std::abs(g_close_proximity))
                {
                    m_pixel.epsilon_axis = 2;
                }
                if (m_pixel.epsilon_axis != 0)
                {
                    m_pixel.old = m_pixel.w;
                    m_pixel.phase = PixelState::Phase::COMPLETE;
                    continue;
                }
            }
            if (m_pixel.remember_fmod)
            {
                const std::complex<double> value{native_orbit_value(m_pixel.w)};
                if (const double mag = fmod_magnitude({value.real(), value.imag()}, m_pixel.magnitude);
                    mag < g_close_proximity)
                {
                    m_pixel.mem_value = mag;
                }
            }
            if (g_outside_method == ColorMethod::TDIS)
            {
                m_pixel.total_distance +=
                    std::sqrt(sqr(m_pixel.old.real() - m_pixel.w.real()) + sqr(m_pixel.old.imag() - m_pixel.w.imag()));
            }
            m_pixel.old = m_pixel.w;
            m_pixel.phase = PixelState::Phase::PERIODICITY;
        }
            [[fallthrough]];

        case PixelState::Phase::PERIODICITY:
        {
            m_pixel.phase = PixelState::Phase::ORBIT;
            if (!m_pixel.check_periodicity || m_pixel.iteration <= g_old_color_iter)
            {
                break;
            }
            double error{};
            if (m_pixel.high_precision)
            {
                error = 4.0 * reference_conversion_error() *
                    (std::abs(reference.orbit[m_pixel.sample].real()) +
                        std::abs(reference.orbit[m_pixel.sample].imag()) + std::abs(m_pixel.orbit_displacement.real()) +
                        std::abs(m_pixel.orbit_displacement.imag()));
            }
            if ((m_pixel.iteration & m_pixel.saved_and) == 0)
            {
                m_pixel.saved_iteration = m_pixel.iteration;
                m_pixel.saved_sample = m_pixel.sample;
                m_pixel.have_saved_sample = true;
                m_pixel.saved_z = m_pixel.w;
                m_pixel.saved_error = error;
                m_pixel.saved_is_precise = false;
                if (m_pixel.precise_orbit.x != nullptr)
                {
                    m_pixel.phase = PixelState::Phase::SAVE_PERIODICITY;
                }
                if (--m_pixel.saved_incr == 0)
                {
                    m_pixel.saved_and = (m_pixel.saved_and << 1) + 1;
                    m_pixel.saved_incr = g_periodicity_next_saved_incr;
                }
            }
            else
            {
                const double difference_x{std::abs(m_pixel.saved_z.real() - m_pixel.w.real())};
                const double difference_y{std::abs(m_pixel.saved_z.imag() - m_pixel.w.imag())};
                bool caught{difference_x < g_close_enough && difference_y < g_close_enough};
                if (m_pixel.high_precision)
                {
                    const double tolerance{
                        static_cast<double>(g_bf_math == BFMathType::BIG_NUM ? bn_to_float(g_close_enough_bn)
                                                                             : bf_to_float(g_close_enough_bf))};
                    const double candidate_tolerance{tolerance + m_pixel.saved_error + error};
                    caught = difference_x <= candidate_tolerance && difference_y <= candidate_tolerance;
                    if (caught)
                    {
                        m_pixel.phase = PixelState::Phase::CHECK_PERIODICITY;
                        break;
                    }
                }
                if (caught)
                {
                    m_cycle_len = m_pixel.iteration - m_pixel.saved_iteration;
                    m_pixel.iteration = g_max_iterations;
                    m_pixel.phase = PixelState::Phase::COMPLETE;
                }
            }
            break;
        }

        case PixelState::Phase::SAVE_PERIODICITY:
            if (!save_precise_value(remaining))
            {
                return CALC_YIELDED;
            }
            m_pixel.phase = PixelState::Phase::ORBIT;
            break;

        case PixelState::Phase::CHECK_PERIODICITY:
            if ((!m_pixel.saved_is_precise && !save_precise_value(remaining)) ||
                !advance_precise_orbit(m_pixel.sample, remaining))
            {
                return CALC_YIELDED;
            }
            if (cmp_bf(abs_a_bf(sub_bf(m_tmp_bf, m_pixel.precise_saved.x, m_pixel.precise_orbit.x)),
                    m_pixel.precise_tolerance) < 0 &&
                cmp_bf(abs_a_bf(sub_bf(m_tmp_bf, m_pixel.precise_saved.y, m_pixel.precise_orbit.y)),
                    m_pixel.precise_tolerance) < 0)
            {
                m_cycle_len = m_pixel.iteration - m_pixel.saved_iteration;
                m_pixel.iteration = g_max_iterations;
                m_pixel.phase = PixelState::Phase::COMPLETE;
            }
            else
            {
                m_pixel.phase = PixelState::Phase::ORBIT;
            }
            break;

        case PixelState::Phase::REBASE:
            return PERTURBATION_RETRY;

        case PixelState::Phase::COMPLETE:
            if (m_explicit_orbit)
            {
                return 0;
            }
            if (m_orbit_output_started)
            {
                if (!g_sound->ready())
                {
                    return CALC_YIELDED;
                }
                orbit_plot().scrub();
            }
            return finish_point(pt);
        }
    }
}

int PertEngine::finish_point(const Point &pt)
{
    if (m_pixel.epsilon_axis == 0)
    {
        g_old_color_iter = m_pixel.iteration >= g_max_iterations ? 0 : m_pixel.iteration + 10;
    }
    g_color_iter = m_pixel.iteration;
    g_real_color_iter = m_pixel.iteration;
    const std::complex<double> previous{native_orbit_value(m_pixel.old)};
    const std::complex<double> value{native_orbit_value(m_pixel.w)};
    g_old_z = {previous.real(), previous.imag()};
    g_new_z = {value.real(), value.imag()};
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = m_pixel.magnitude;

    // BOF's first update is iteration zero, but escape colors start at one.
    if (m_pixel.iteration == 0)
    {
        m_pixel.iteration = 1;
    }
    long index;

    const bool highlight_cycle{g_periodicity_check < 0 && m_cycle_len > 0};
    const bool use_distance{g_distance_estimator != 0 && !g_potential.flag && m_pixel.epsilon_axis == 0 &&
        m_pixel.iteration < g_max_iterations};
    const double distance{use_distance ? m_distance_estimator.distance_squared(g_new_z) : 0};
    const bool boundary{use_distance && m_distance_estimator.on_boundary(distance)};
    const bool direct_distance_color{
        use_distance && (boundary ? g_distance_estimator < 0 : g_colors == 2 || g_distance_estimator > 1)};
    const bool inside{
        m_pixel.iteration == g_max_iterations || m_pixel.epsilon_axis != 0 || (boundary && g_distance_estimator > 0)};
    const bool regular_escape_color{!g_potential.flag && !inside && !direct_distance_color};
    if (g_potential.flag && m_pixel.epsilon_axis == 0)
    {
        index = potential(mag_squared(m_pixel.w), m_pixel.iteration);
    }
    else if (direct_distance_color)
    {
        if (boundary)
        {
            index = -g_distance_estimator;
        }
        else if (g_colors == 2)
        {
            index = !g_inside_color;
        }
        else
        {
            index = m_distance_estimator.band_color(distance, g_old_demm_colors);
        }
    }
    else if (regular_escape_color)
    {
        index = m_pixel.iteration;
        if (use_distance && g_use_old_distance_estimator)
        {
            index = m_distance_estimator.escape_iteration();
            g_new_z = m_distance_estimator.escape_point();
        }
        else if (g_outside_method < ColorMethod::ITER)
        {
            if (g_outside_method == ColorMethod::FMOD)
            {
                index = fmod_color(m_pixel.mem_value);
            }
            else if (g_outside_method == ColorMethod::TDIS)
            {
                index = static_cast<long>(m_pixel.total_distance);
            }
            index = standard_outside_color(index, g_new_z);
        }
        index = standard_escape_color(index, g_new_z, false);
    }
    else if (m_pixel.epsilon_axis != 0)
    {
        constexpr int GREEN{2};
        constexpr int YELLOW{6};
        index = g_close_proximity > 0 ? m_pixel.epsilon_axis == 1 ? GREEN : YELLOW : m_pixel.iteration;
    }
    else if (inside && (highlight_cycle || g_inside_method == ColorMethod::PERIOD))
    {
        if (highlight_cycle)
        {
            index = 7;
        }
        else
        {
            index = m_cycle_len > 0 ? m_cycle_len : g_max_iterations;
        }
    }
    else if (m_pixel.star_trail)
    {
        index = 0;
        for (int i = 1; i < STAR_TRAIL_ITERATIONS; ++i)
        {
            if (std::abs(m_pixel.tan_table[0] - m_pixel.tan_table[i]) < 0.05)
            {
                index = i;
                break;
            }
        }
    }
    else
    {
        index = g_max_iterations;
        if (g_inside_method >= ColorMethod::COLOR) // no filter
        {
            index = g_inside_color;
        }
        else
        {
            switch (g_inside_method)
            {
            case ColorMethod::ZMAG:
                index = static_cast<long>(mag_squared(m_pixel.w) * (g_max_iterations >> 1) + 1);
                break;
            case ColorMethod::BOF60:
                index = static_cast<long>(std::sqrt(m_pixel.min_orbit) * 75.0);
                break;
            case ColorMethod::BOF61:
                index = m_pixel.min_index;
                break;

            case ColorMethod::FMODI:
                index = fmod_color(m_pixel.mem_value);
                break;

            case ColorMethod::ATANI:
                index =
                    static_cast<long>(std::abs(std::atan2(m_pixel.w.imag(), m_pixel.w.real()) * g_atan_colors / PI));
                break;

            default:
                break;
            }
        }
    }
    const bool map_color{!direct_distance_color && !regular_escape_color &&
        (g_potential.flag ||
            (inside ? !highlight_cycle && g_inside_method < ColorMethod::COLOR
                    : g_outside_method < ColorMethod::COLOR))};
    if (map_color && (!g_log_map_table.empty() || g_log_map_calculate))
    {
        index = log_table_calc(index);
    }
    g_color = standard_palette_color(index);
    g_plot(pt.get_x(), g_logical_screen.y_dots - 1 - pt.get_y(), g_color);
    return g_color;
}

void PertEngine::advance_reference_bf(const std::size_t end)
{
    BigStackSaver saved;

    BigFloat temp_real_bf = alloc_stack(g_r_bf_length + 2);
    BigFloat temp_imag_bf = alloc_stack(g_r_bf_length + 2);
    BigFloat tmp_bf = alloc_stack(g_r_bf_length + 2);

    float_to_bf(tmp_bf, m_glitch_tolerance);
    for (; m_reference_sample < end; ++m_reference_sample)
    {
        std::complex<double> c;
        c.real(bf_to_float(m_reference_z_bf.x));
        c.imag(bf_to_float(m_reference_z_bf.y));

        m_reference.orbit.push_back(c);
        // Norm is the squared version of abs and the perturbation tolerance is squared.
        // The reason we are storing this into an array is that we need to check the magnitude against this
        // value to see if the value is glitched. We are leaving it squared because otherwise we'd need to do
        // a square root operation, which is expensive, so we'll just compare this to the squared magnitude.

        mult_bf(temp_real_bf, m_reference_z_bf.x, tmp_bf);
        mult_bf(temp_imag_bf, m_reference_z_bf.y, tmp_bf);
        std::complex<double> tolerance;
        tolerance.real(bf_to_float(temp_real_bf));
        tolerance.imag(bf_to_float(temp_imag_bf));

        m_reference.tolerance.push_back(mag_squared(tolerance));

        if (g_cur_fractal_specific->pert_ref_bf == nullptr)
        {
            throw std::runtime_error("No reference orbit function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        if (m_reference_sample + 1 < m_reference_samples)
        {
            if (!m_reference.explicit_orbit && g_inside_method == ColorMethod::STAR_TRAIL && m_reference_sample > 0)
            {
                clamp_star_trail_point(m_reference_z_bf);
            }
            g_cur_fractal_specific->pert_ref_bf(m_reference_coordinate_bf, m_reference_z_bf);
        }
    }
}

void PertEngine::advance_reference(const std::size_t end)
{
    for (; m_reference_sample < end; ++m_reference_sample)
    {
        m_reference.orbit.push_back(m_reference_z);

        // Norm is the squared version of abs and the perturbation tolerance is squared.
        // The reason we are storing this into an array is that we need to check the magnitude against this
        // value to see if the value is glitched. We are leaving it squared because otherwise we'd need to do
        // a square root operation, which is expensive, so we'll just compare this to the squared magnitude.

        m_reference.tolerance.push_back(glitch_tolerance_threshold(m_reference_z));

        if (g_cur_fractal_specific->pert_ref == nullptr)
        {
            throw std::runtime_error("No reference orbit function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        if (m_reference_sample + 1 < m_reference_samples)
        {
            if (!m_reference.explicit_orbit && g_inside_method == ColorMethod::STAR_TRAIL && m_reference_sample > 0)
            {
                m_reference_z = star_trail_point(m_reference_z);
            }
            g_cur_fractal_specific->pert_ref(m_reference.coordinate, m_reference_z);
        }
    }
}

} // namespace id::engine
