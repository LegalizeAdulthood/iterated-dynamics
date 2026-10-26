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
#include "engine/Potential.h"
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
constexpr long STAR_TRAIL_ITERATIONS{16};

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
    // Ordinary manzpower uses other_mandel_per_pixel, without BOF seeding.
    return (g_fractal_type != FractalType::MANDEL_Z_POWER || g_bf_math != BFMathType::NONE) &&
        (g_inside_method == ColorMethod::BOF60 || g_inside_method == ColorMethod::BOF61) && g_bof_match_book_images;
}

void initialize_orbit_bf(const BFComplex z, const BFComplex &coordinate)
{
    BigStackSaver saved;
    const BigFloat offset{alloc_stack(g_r_bf_length + 2)};
    float_to_bf(z.x, g_params[0]);
    float_to_bf(z.y, g_params[1]);
    if (!bof_initialization())
    {
        if (g_use_init_orbit == InitOrbitMode::VALUE)
        {
            float_to_bf(offset, g_init_orbit.x);
            add_a_bf(z.x, offset);
            float_to_bf(offset, g_init_orbit.y);
            add_a_bf(z.y, offset);
        }
        else
        {
            add_a_bf(z.x, coordinate.x);
            add_a_bf(z.y, coordinate.y);
        }
    }
}

double reference_conversion_error()
{
    // bf_to_float reads whole bytes after the leading byte.
    constexpr int REFERENCE_BITS{8 * (std::numeric_limits<LDouble>::digits / 8)};
    return std::ldexp(1.0, -REFERENCE_BITS) + std::numeric_limits<double>::epsilon();
}

bool ambiguous_bailout(const std::complex<double> &reference, const std::complex<double> &delta)
{
    const double scale{
        std::abs(reference.real()) + std::abs(reference.imag()) + std::abs(delta.real()) + std::abs(delta.imag())};
    // Include conversion truncation, reconstruction, and predicate roundoff.
    const double error{8.0 * reference_conversion_error() * scale * scale};
    const std::complex<double> value{reference + delta};
    const double x{sqr(value.real())};
    const double y{sqr(value.imag())};
    const auto near_limit = [error](const double metric)
    { return !std::isfinite(metric) || std::abs(metric - g_magnitude_limit) <= error; };
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
    float_to_bf(limit, g_magnitude_limit);
    square_bf(x, z.x);
    square_bf(y, z.y);
    switch (g_bailout_test)
    {
    case Bailout::MOD:
        add_bf(metric, x, y);
        break;
    case Bailout::REAL:
        return cmp_bf(x, limit) >= 0;
    case Bailout::IMAG:
        return cmp_bf(y, limit) >= 0;
    case Bailout::OR:
        return cmp_bf(x, limit) >= 0 || cmp_bf(y, limit) >= 0;
    case Bailout::AND:
        return cmp_bf(x, limit) >= 0 && cmp_bf(y, limit) >= 0;
    case Bailout::MANH:
        abs_bf(x, z.x);
        abs_bf(y, z.y);
        add_bf(metric, x, y);
        square_bf(metric, metric);
        break;
    case Bailout::MANR:
        add_bf(metric, z.x, z.y);
        square_bf(metric, metric);
        break;
    default:
        throw std::logic_error("Unknown bailout test");
    }
    return cmp_bf(metric, limit) >= 0;
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
    return m_center_bf;
}

void PertEngine::initialize_frame(const std::complex<double> &center, const std::complex<double> &half_horizontal,
    const std::complex<double> &half_vertical)
{
    reset_for_frame(half_horizontal, half_vertical);
    m_center = center;
}

void PertEngine::initialize_pixel_strategy()
{
    if (m_frame_initialized)
    {
        return;
    }

    const ValueSaver saved_iterations{
        g_max_iterations, g_inside_method == ColorMethod::STAR_TRAIL ? STAR_TRAIL_ITERATIONS : g_max_iterations};
    try
    {
        initialize_pixel_state();
        set_reference_point({(g_logical_screen.x_dots - 1) / 2.0, (g_logical_screen.y_dots - 1) / 2.0});
        m_frame_initialized = true;
    }
    catch (...)
    {
        run_cleanup(std::current_exception(), [this] { cleanup(); });
    }
}

int PertEngine::calculate_pixel(const Point &pt)
{
    const ValueSaver saved_iterations{
        g_max_iterations, g_inside_method == ColorMethod::STAR_TRAIL ? STAR_TRAIL_ITERATIONS : g_max_iterations};
    if (!m_frame_initialized)
    {
        initialize_pixel_strategy();
    }

    for (;;)
    {
        const int color{calculate_point(pt)};
        if (color != PERTURBATION_RETRY)
        {
            return color;
        }
        set_reference_point({static_cast<double>(pt.get_x()), static_cast<double>(pt.get_y())});
    }
}

int PertEngine::calculate_pixel(const int col, const int row)
{
    return calculate_pixel(Point{col, g_logical_screen.y_dots - 1 - row});
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
    m_reference_count = 0;
    m_cycle_len = -1;
    m_distance_estimator = {};
    m_reference_coordinate_bf = {};
    m_tmp_bf = {};
    m_center_bf = {};
    m_done = true;
    m_frame_initialized = false;
}

void PertEngine::initialize_pixel_state()
{
    if (g_logical_screen.x_dots <= 0 || g_logical_screen.y_dots <= 0 || g_max_iterations <= 0)
    {
        throw std::logic_error{"Perturbation requires image dimensions and a positive iteration limit"};
    }

    pascal_triangle();
    allocate_working_values();
}

void PertEngine::allocate_working_values()
{
    if (g_bf_math != BFMathType::NONE)
    {
        m_reference_coordinate_bf.x = alloc_stack(g_r_bf_length + 2);
        m_reference_coordinate_bf.y = alloc_stack(g_r_bf_length + 2);
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

std::complex<double> PertEngine::parameter_delta(const std::complex<double> &pixel_position) const
{
    if (g_inversion.invert == 0)
    {
        return pixel_delta(pixel_position) - m_reference.delta;
    }
    if (g_bf_math == BFMathType::NONE)
    {
        return inverted_coordinate(pixel_position) - m_reference.coordinate;
    }
    BigStackSaver saved;
    const BFComplex coordinate{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    inverted_coordinate(coordinate, pixel_position);
    sub_a_bf(coordinate.x, m_reference_coordinate_bf.x);
    sub_a_bf(coordinate.y, m_reference_coordinate_bf.y);
    return {static_cast<double>(bf_to_float(coordinate.x)), static_cast<double>(bf_to_float(coordinate.y))};
}

void PertEngine::coordinate_bf(const BFComplex coordinate, const std::complex<double> &pixel_position) const
{
    if (g_inversion.invert != 0)
    {
        inverted_coordinate(coordinate, pixel_position);
    }
    else
    {
        const std::complex<double> delta{pixel_delta(pixel_position)};
        float_to_bf(m_tmp_bf, delta.real());
        add_bf(coordinate.x, m_center_bf.x, m_tmp_bf);
        float_to_bf(m_tmp_bf, delta.imag());
        add_bf(coordinate.y, m_center_bf.y, m_tmp_bf);
    }
}

void PertEngine::set_reference_point(const std::complex<double> &pixel_position)
{
    const std::size_t samples{reference_sample_count(g_max_iterations)};
    ReferencePoint &reference{m_reference};
    reference.pixel_position = pixel_position;
    reference.delta = pixel_delta(pixel_position);
    reference.orbit.resize(samples);
    reference.tolerance.resize(samples);

    if (g_bf_math != BFMathType::NONE)
    {
        coordinate_bf(m_reference_coordinate_bf, pixel_position);
        reference_zoom_point(reference, m_reference_coordinate_bf);
    }
    else
    {
        reference.coordinate =
            g_inversion.invert != 0 ? inverted_coordinate(pixel_position) : m_center + reference.delta;
        reference_zoom_point(reference, reference.coordinate);
    }
    ++m_reference_count;
}

int PertEngine::calculate_point(const Point &pt)
{
    m_cycle_len = -1;
    m_distance_estimator = g_distance_estimator ? standard_distance_estimator() : DistanceEstimator{};
    if (g_distance_estimator)
    {
        g_magnitude_limit = m_distance_estimator.bailout();
    }
    const bool star_trail{g_inside_method == ColorMethod::STAR_TRAIL};
    std::array<double, STAR_TRAIL_ITERATIONS> tan_table{};
    const bool high_precision{g_bf_math != BFMathType::NONE};
    const bool check_periodicity{
        g_periodicity_check != 0 && g_inside_method != ColorMethod::ZMAG && g_inside_method != ColorMethod::STAR_TRAIL};
    if (!check_periodicity)
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
    long saved_and{g_inside_method == ColorMethod::PERIOD ? 16 : g_first_saved_and};
    int saved_incr{1};
    long saved_iteration{};
    std::size_t saved_sample{};
    bool have_saved_sample{};
    std::complex<double> saved_z;
    if (!high_precision && g_use_init_orbit == InitOrbitMode::VALUE)
    {
        saved_z = {g_init_orbit.x, g_init_orbit.y};
    }
    double saved_error{};
    const ReferencePoint &reference{m_reference};
    const std::complex<double> pixel_position{static_cast<double>(pt.get_x()), static_cast<double>(pt.get_y())};
    const std::complex<double> parameter_displacement{parameter_delta(pixel_position)};
    const bool bof{bof_initialization()};
    std::complex<double> orbit_displacement{
        bof || g_use_init_orbit == InitOrbitMode::VALUE ? std::complex<double>{} : parameter_displacement};
    const bool at_reference{pixel_position == reference.pixel_position};
    long iteration{bof ? -1L : 0L};
    BigStackSaver saved;
    BFComplex precise_coordinate{};
    BFComplex precise_orbit{};
    std::size_t precise_sample{};
    std::complex<double> precise_previous;
    BFComplex precise_saved{};
    BigFloat precise_tolerance{};
    bool saved_is_precise{};
    const auto advance_precise_orbit = [&](const std::size_t target)
    {
        if (precise_orbit.x == nullptr)
        {
            precise_coordinate = {alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
            precise_orbit = {alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
            coordinate_bf(precise_coordinate, pixel_position);
            initialize_orbit_bf(precise_orbit, precise_coordinate);
        }
        if (target < precise_sample)
        {
            initialize_orbit_bf(precise_orbit, precise_coordinate);
            precise_sample = 0;
        }
        for (; precise_sample < target; ++precise_sample)
        {
            if (star_trail && precise_sample > 0)
            {
                clamp_star_trail_point(precise_orbit);
            }
            precise_previous = {
                static_cast<double>(bf_to_float(precise_orbit.x)), static_cast<double>(bf_to_float(precise_orbit.y))};
            g_cur_fractal_specific->pert_ref_bf(precise_coordinate, precise_orbit);
        }
    };
    const auto save_precise_value = [&]
    {
        if (precise_saved.x == nullptr)
        {
            precise_saved = {alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
            precise_tolerance = alloc_stack(g_r_bf_length + 2);
            if (g_bf_math == BFMathType::BIG_NUM)
            {
                bn_to_bf(precise_tolerance, g_close_enough_bn);
            }
            else
            {
                copy_bf(precise_tolerance, g_close_enough_bf);
            }
        }
        if (have_saved_sample)
        {
            advance_precise_orbit(saved_sample);
            copy_bf(precise_saved.x, precise_orbit.x);
            copy_bf(precise_saved.y, precise_orbit.y);
        }
        else
        {
            clear_bf(precise_saved.x);
            clear_bf(precise_saved.y);
        }
        saved_is_precise = true;
    };

    double min_orbit{1e5}; // orbit value closest to origin
    long min_index{};      // iteration of min_orbit
    double mem_value{};
    double total_distance{};
    int epsilon_axis{};
    const bool remember_fmod{g_inside_method == ColorMethod::FMODI || g_outside_method == ColorMethod::FMOD};
    std::complex<double> w{reference.orbit.front() + orbit_displacement};
    std::complex<double> old{w};
    double magnitude{mag_squared(w)};
    while (++iteration < g_max_iterations)
    {
        if (g_cur_fractal_specific->pert_pt == nullptr)
        {
            throw std::runtime_error("No perturbation point function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        const std::size_t sample{static_cast<std::size_t>(iteration + (bof ? 1 : 0))};
        old = w;
        if (g_distance_estimator && !m_distance_estimator.advance({old.real(), old.imag()}))
        {
            break;
        }
        const std::complex<double> reference_z{
            star_trail && sample > 1 ? star_trail_point(reference.orbit[sample - 1]) : reference.orbit[sample - 1]};
        g_cur_fractal_specific->pert_pt(reference_z, orbit_displacement, parameter_displacement);
        w = reference.orbit[sample] + orbit_displacement;
        magnitude = mag_squared(w);

        // At the reference pixel, zero delta uses the reference orbit directly.
        if (!at_reference && (!std::isfinite(magnitude) || magnitude < reference.tolerance[sample]))
        {
            return PERTURBATION_RETRY;
        }
        bool escaped{bailout_reached(g_bailout_test, w.real(), w.imag(), g_magnitude_limit)};
        if (g_bf_math != BFMathType::NONE && ambiguous_bailout(reference.orbit[sample], orbit_displacement))
        {
            // Retain only the current high-precision value, not another orbit history.
            advance_precise_orbit(sample);
            old = precise_previous;
            w = {static_cast<double>(bf_to_float(precise_orbit.x)), static_cast<double>(bf_to_float(precise_orbit.y))};
            magnitude = mag_squared(w);
            escaped = precise_bailout_reached(precise_orbit);
        }
        if (g_inside_method == ColorMethod::BOF60 || g_inside_method == ColorMethod::BOF61)
        {
            if (magnitude < min_orbit)
            {
                min_orbit = magnitude;
                min_index = iteration + 1L;
            }
        }
        if (escaped && (g_distance_estimator || !star_trail))
        {
            if (!g_distance_estimator)
            {
                break;
            }
            const bool complete{m_distance_estimator.escaped({w.real(), w.imag()}, iteration, magnitude)};
            g_magnitude_limit = m_distance_estimator.bailout();
            if (complete)
            {
                break;
            }
        }
        if (star_trail)
        {
            const std::complex<double> bounded{star_trail_point(w)};
            const std::complex<double> bounded_reference{star_trail_point(reference.orbit[sample])};
            if (bounded != w || bounded_reference != reference.orbit[sample])
            {
                orbit_displacement = bounded - bounded_reference;
            }
            w = bounded;
            magnitude = mag_squared(w);
            const int color{static_cast<int>((iteration - 1) % g_and_color + 1)};
            tan_table[color - 1] = w.imag() / (w.real() + 0.000001);
        }
        if (g_inside_method == ColorMethod::EPS_CROSS)
        {
            if (std::abs(w.real()) < std::abs(g_close_proximity))
            {
                epsilon_axis = 1;
            }
            else if (std::abs(w.imag()) < std::abs(g_close_proximity))
            {
                epsilon_axis = 2;
            }
            if (epsilon_axis != 0)
            {
                old = w;
                break;
            }
        }
        if (remember_fmod)
        {
            if (const double mag = fmod_magnitude({w.real(), w.imag()}, magnitude); mag < g_close_proximity)
            {
                mem_value = mag;
            }
        }
        if (g_outside_method == ColorMethod::TDIS)
        {
            total_distance += std::sqrt(sqr(old.real() - w.real()) + sqr(old.imag() - w.imag()));
        }
        old = w;
        if (check_periodicity && iteration > g_old_color_iter)
        {
            double error{};
            if (high_precision)
            {
                error = 4.0 * reference_conversion_error() *
                    (std::abs(reference.orbit[sample].real()) + std::abs(reference.orbit[sample].imag()) +
                        std::abs(orbit_displacement.real()) + std::abs(orbit_displacement.imag()));
            }
            if ((iteration & saved_and) == 0)
            {
                saved_iteration = iteration;
                saved_sample = sample;
                have_saved_sample = true;
                saved_z = w;
                saved_error = error;
                saved_is_precise = false;
                if (precise_orbit.x != nullptr)
                {
                    save_precise_value();
                }
                if (--saved_incr == 0)
                {
                    saved_and = (saved_and << 1) + 1;
                    saved_incr = g_periodicity_next_saved_incr;
                }
            }
            else
            {
                const double difference_x{std::abs(saved_z.real() - w.real())};
                const double difference_y{std::abs(saved_z.imag() - w.imag())};
                bool caught{difference_x < g_close_enough && difference_y < g_close_enough};
                if (high_precision)
                {
                    const double tolerance{
                        static_cast<double>(g_bf_math == BFMathType::BIG_NUM ? bn_to_float(g_close_enough_bn)
                                                                             : bf_to_float(g_close_enough_bf))};
                    const double candidate_tolerance{tolerance + saved_error + error};
                    caught = difference_x <= candidate_tolerance && difference_y <= candidate_tolerance;
                    if (caught)
                    {
                        if (!saved_is_precise)
                        {
                            save_precise_value();
                        }
                        advance_precise_orbit(sample);
                        caught = cmp_bf(abs_a_bf(sub_bf(m_tmp_bf, precise_saved.x, precise_orbit.x)),
                                     precise_tolerance) < 0 &&
                            cmp_bf(abs_a_bf(sub_bf(m_tmp_bf, precise_saved.y, precise_orbit.y)), precise_tolerance) < 0;
                    }
                }
                if (caught)
                {
                    m_cycle_len = iteration - saved_iteration;
                    iteration = g_max_iterations;
                    break;
                }
            }
        }
    }

    if (epsilon_axis == 0)
    {
        g_old_color_iter = iteration >= g_max_iterations ? 0 : iteration + 10;
    }
    g_color_iter = iteration;
    g_real_color_iter = iteration;
    g_old_z = {old.real(), old.imag()};
    g_new_z = {w.real(), w.imag()};
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = magnitude;

    // BOF's first update is iteration zero, but escape colors start at one.
    if (iteration == 0)
    {
        iteration = 1;
    }
    long index;
    const double rq_lim2{std::sqrt(g_magnitude_limit)};

    const bool highlight_cycle{g_periodicity_check < 0 && m_cycle_len > 0};
    const bool use_distance{
        g_distance_estimator != 0 && !g_potential.flag && epsilon_axis == 0 && iteration < g_max_iterations};
    const double distance{use_distance ? m_distance_estimator.distance_squared(g_new_z) : 0};
    const bool boundary{use_distance && m_distance_estimator.on_boundary(distance)};
    const bool direct_distance_color{
        use_distance && (boundary ? g_distance_estimator < 0 : g_colors == 2 || g_distance_estimator > 1)};
    const bool regular_distance_color{use_distance && !boundary && !direct_distance_color};
    const bool inside{iteration == g_max_iterations || epsilon_axis != 0 || (boundary && g_distance_estimator > 0)};
    if (direct_distance_color)
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
    else if (regular_distance_color)
    {
        index = iteration;
        if (g_use_old_distance_estimator)
        {
            index = m_distance_estimator.escape_iteration();
            g_new_z = m_distance_estimator.escape_point();
        }
        else if (g_outside_method < ColorMethod::ITER)
        {
            if (g_outside_method == ColorMethod::FMOD)
            {
                index = fmod_color(mem_value);
            }
            else if (g_outside_method == ColorMethod::TDIS)
            {
                index = static_cast<long>(total_distance);
            }
            index = standard_outside_color(index, g_new_z);
        }
        index = standard_escape_color(index, g_new_z, false);
    }
    else if (epsilon_axis != 0)
    {
        constexpr int GREEN{2};
        constexpr int YELLOW{6};
        index = g_close_proximity > 0 ? epsilon_axis == 1 ? GREEN : YELLOW : iteration;
    }
    else if (inside && (highlight_cycle || g_inside_method == ColorMethod::PERIOD))
    {
        if (g_potential.flag)
        {
            index = potential(magnitude, iteration);
        }
        else if (highlight_cycle)
        {
            index = 7;
        }
        else
        {
            index = m_cycle_len > 0 ? m_cycle_len : g_max_iterations;
        }
    }
    else if (star_trail)
    {
        index = 0;
        for (int i = 1; i < STAR_TRAIL_ITERATIONS; ++i)
        {
            if (std::abs(tan_table[0] - tan_table[i]) < 0.05)
            {
                index = i;
                break;
            }
        }
        if (g_potential.flag)
        {
            index = potential(magnitude, iteration);
        }
    }
    else if (!inside && g_decomp[0] > 0 && !g_potential.flag)
    {
        index = standard_decomposition_color(g_new_z);
        if (g_outside_method >= ColorMethod::COLOR)
        {
            index = g_outside_color;
        }
    }
    else if (g_biomorph >= 0 && !boundary)
    {
        if (iteration == g_max_iterations)
        {
            index = g_max_iterations;
        }
        else
        {
            if (std::abs(w.real()) < rq_lim2 || std::abs(w.imag()) < rq_lim2)
            {
                index = g_biomorph;
            }
            else
            {
                index = iteration;
            }
        }
    }
    else
    {
        switch (g_outside_method)
        {
        case ColorMethod::COLOR: // no filter
            if (iteration == g_max_iterations)
            {
                index = g_max_iterations;
            }
            else
            {
                index = iteration;
            }
            break;

        case ColorMethod::ZMAG:
            if (iteration == g_max_iterations)
            {
                index = static_cast<long>((w.real() * w.real() + w.imag() + w.imag()) * (g_max_iterations >> 1) + 1);
            }
            else
            {
                index = iteration;
            }
            break;

        case ColorMethod::REAL:
        case ColorMethod::IMAG:
        case ColorMethod::MULT:
        case ColorMethod::SUM:
        case ColorMethod::ATAN:
            index = iteration == g_max_iterations ? g_max_iterations : standard_outside_color(iteration, g_new_z);
            break;

        case ColorMethod::FMOD:
            index = iteration == g_max_iterations ? g_max_iterations : fmod_color(mem_value);
            break;

        case ColorMethod::TDIS:
            index = iteration == g_max_iterations ? g_max_iterations
                                                  : standard_outside_color(static_cast<long>(total_distance), g_new_z);
            break;

        default:
            if (g_potential.flag)
            {
                index = potential(mag_squared(w), iteration);
            }
            else // no filter
            {
                if (iteration == g_max_iterations)
                {
                    index = g_max_iterations;
                }
                else
                {
                    index = iteration;
                }
            }
            break;
        }

        if (g_inside_method >= ColorMethod::COLOR) // no filter
        {
            if (inside)
            {
                index = g_inside_color;
            }
            else
            {
                index = iteration;
            }
        }
        else
        {
            switch (g_inside_method)
            {
            case ColorMethod::ITER:
            case ColorMethod::EPS_CROSS:
                if (inside)
                {
                    index = g_max_iterations;
                }
                break;
            case ColorMethod::ZMAG:
                if (inside)
                {
                    index = static_cast<long>(mag_squared(w) * (g_max_iterations >> 1) + 1);
                }
                break;
            case ColorMethod::BOF60:
                if (inside)
                {
                    index = static_cast<long>(std::sqrt(min_orbit) * 75.0);
                }
                break;
            case ColorMethod::BOF61:
                if (inside)
                {
                    index = min_index;
                }
                break;

            case ColorMethod::FMODI:
                if (inside)
                {
                    index = fmod_color(mem_value);
                }
                break;

            case ColorMethod::ATANI:
                if (inside)
                {
                    index = static_cast<long>(std::abs(std::atan2(w.imag(), w.real()) * g_atan_colors / PI));
                }
                break;

            default:
                break;
            }
        }
    }
    const bool map_color{!direct_distance_color && !regular_distance_color &&
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

void PertEngine::reference_zoom_point(ReferencePoint &reference, const BFComplex &center)
{
    BigStackSaver saved;

    BFComplex z_bf;
    z_bf.x = alloc_stack(g_r_bf_length + 2);
    z_bf.y = alloc_stack(g_r_bf_length + 2);
    BigFloat temp_real_bf = alloc_stack(g_r_bf_length + 2);
    BigFloat temp_imag_bf = alloc_stack(g_r_bf_length + 2);
    BigFloat tmp_bf = alloc_stack(g_r_bf_length + 2);

    initialize_orbit_bf(z_bf, center);

    for (std::size_t i = 0; i < reference.orbit.size(); ++i)
    {
        std::complex<double> c;
        c.real(bf_to_float(z_bf.x));
        c.imag(bf_to_float(z_bf.y));

        reference.orbit[i] = c;
        // Norm is the squared version of abs and the perturbation tolerance is squared.
        // The reason we are storing this into an array is that we need to check the magnitude against this
        // value to see if the value is glitched. We are leaving it squared because otherwise we'd need to do
        // a square root operation, which is expensive, so we'll just compare this to the squared magnitude.

        float_to_bf(tmp_bf, m_glitch_tolerance);
        mult_bf(temp_real_bf, z_bf.x, tmp_bf);
        mult_bf(temp_imag_bf, z_bf.y, tmp_bf);
        std::complex<double> tolerance;
        tolerance.real(bf_to_float(temp_real_bf));
        tolerance.imag(bf_to_float(temp_imag_bf));

        reference.tolerance[i] = mag_squared(tolerance);

        if (g_cur_fractal_specific->pert_ref_bf == nullptr)
        {
            throw std::runtime_error("No reference orbit function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        if (i + 1 < reference.orbit.size())
        {
            if (g_inside_method == ColorMethod::STAR_TRAIL && i > 0)
            {
                clamp_star_trail_point(z_bf);
            }
            g_cur_fractal_specific->pert_ref_bf(center, z_bf);
        }
    }
}

void PertEngine::reference_zoom_point(ReferencePoint &reference, const std::complex<double> &center)
{
    std::complex<double> z{g_param_z1.x, g_param_z1.y};
    if (!bof_initialization())
    {
        z += g_use_init_orbit == InitOrbitMode::VALUE ? std::complex<double>{g_init_orbit.x, g_init_orbit.y} : center;
    }

    for (std::size_t i = 0; i < reference.orbit.size(); ++i)
    {
        reference.orbit[i] = z;

        // Norm is the squared version of abs and the perturbation tolerance is squared.
        // The reason we are storing this into an array is that we need to check the magnitude against this
        // value to see if the value is glitched. We are leaving it squared because otherwise we'd need to do
        // a square root operation, which is expensive, so we'll just compare this to the squared magnitude.

        reference.tolerance[i] = glitch_tolerance_threshold(z);

        if (g_cur_fractal_specific->pert_ref == nullptr)
        {
            throw std::runtime_error("No reference orbit function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        if (i + 1 < reference.orbit.size())
        {
            if (g_inside_method == ColorMethod::STAR_TRAIL && i > 0)
            {
                z = star_trail_point(z);
            }
            g_cur_fractal_specific->pert_ref(center, z);
        }
    }
}

} // namespace id::engine
