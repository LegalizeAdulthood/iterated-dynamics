// SPDX-License-Identifier: GPL-3.0-only
//
// Per-pixel perturbation orbit calculation
//
// Thanks to Claude Heiland-Allen
// <https://fractalforums.org/programming/11/perturbation-code-for-cubic-and-higher-order-polynomials/2783>
//
// Much of perturbation code is based on the work of Shirom Makkad
// <https://github.com/ShiromMakkad/MandelbrotPerturbation>

#include "engine/PertEngine.h"

#include "engine/calcfrac.h"
#include "engine/Potential.h"
#include "engine/VideoInfo.h"
#include "fractals/fractalp.h"
#include "fractals/pickover_mandelbrot.h"
#include "math/biginit.h"
#include "math/complex_fn.h"
#include "misc/id.h"
#include "misc/debug_flags.h"
#include "ui/video.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::ui;

namespace id::engine
{

BFComplex PertEngine::initialize_frame_bf(const double zoom_radius)
{
    reset_for_frame(zoom_radius);
    m_saved_stack = save_stack();
    m_center_stack_saved = true;
    m_center_bf.x = alloc_stack(g_bf_length + 2);
    m_center_bf.y = alloc_stack(g_bf_length + 2);
    return m_center_bf;
}

void PertEngine::initialize_frame(const std::complex<double> &center, const double zoom_radius)
{
    reset_for_frame(zoom_radius);
    m_center = center;
}

void PertEngine::initialize_pixel_strategy()
{
    if (m_frame_initialized)
    {
        return;
    }

    initialize_pixel_state();
    add_reference_point({g_screen_x_dots / 2.0, g_screen_y_dots / 2.0});
    m_frame_initialized = true;
}

int PertEngine::calculate_pixel(const Point &pt)
{
    if (!m_frame_initialized)
    {
        initialize_pixel_strategy();
    }

    for (;;)
    {
        const int color{calculate_point(pt)};
        if (color >= 0)
        {
            return color;
        }
        const std::size_t closest{closest_reference(pt)};
        if (closest != m_active_reference)
        {
            m_active_reference = closest;
        }
        else
        {
            add_reference_point({static_cast<double>(pt.get_x()), static_cast<double>(pt.get_y())});
        }
    }
}

int PertEngine::calculate_pixel(const int col, const int row)
{
    return calculate_pixel(Point{col, g_screen_y_dots - 1 - row});
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
    return static_cast<int>(m_references.size());
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
    m_references.clear();
    m_active_reference = 0;
    m_reference_coordinate_bf = {};
    m_tmp_bf = {};
    m_center_bf = {};
    m_done = true;
    m_frame_initialized = false;
}

void PertEngine::initialize_pixel_state()
{
    m_window_radius = std::min(g_screen_x_dots, g_screen_y_dots);
    if (m_window_radius <= 0 || g_max_iterations <= 0)
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

void PertEngine::reset_for_frame(const double zoom_radius)
{
    if (!m_done)
    {
        cleanup();
    }
    m_zoom_radius = zoom_radius;
    m_done = false;
    m_frame_initialized = false;
}

std::complex<double> PertEngine::pixel_delta(const std::complex<double> &pixel_position) const
{
    return {m_zoom_radius * (2 * pixel_position.real() - g_screen_x_dots) / m_window_radius,
        -m_zoom_radius * (2 * pixel_position.imag() - g_screen_y_dots) / m_window_radius};
}

void PertEngine::add_reference_point(const std::complex<double> &pixel_position)
{
    ReferencePoint reference{pixel_position, pixel_delta(pixel_position)};
    reference.orbit.resize(g_max_iterations + 1);
    reference.tolerance.resize(g_max_iterations + 1);

    if (g_bf_math != BFMathType::NONE)
    {
        float_to_bf(m_tmp_bf, reference.delta.real());
        add_bf(m_reference_coordinate_bf.x, m_center_bf.x, m_tmp_bf);
        float_to_bf(m_tmp_bf, reference.delta.imag());
        add_bf(m_reference_coordinate_bf.y, m_center_bf.y, m_tmp_bf);
        reference_zoom_point(reference, m_reference_coordinate_bf);
    }
    else
    {
        reference_zoom_point(reference, m_center + reference.delta);
    }
    m_references.push_back(std::move(reference));
    m_active_reference = m_references.size() - 1;
}

std::size_t PertEngine::closest_reference(const Point &pt) const
{
    // Screen distances avoid underflow at deep zoom scales.
    const std::complex<double> pixel_position{static_cast<double>(pt.get_x()), static_cast<double>(pt.get_y())};
    std::size_t closest{m_active_reference};
    double distance{std::norm(pixel_position - m_references[closest].pixel_position)};
    for (std::size_t index = 0; index < m_references.size(); ++index)
    {
        const double candidate{std::norm(pixel_position - m_references[index].pixel_position)};
        if (candidate < distance)
        {
            closest = index;
            distance = candidate;
        }
    }
    return closest;
}

int PertEngine::calculate_point(const Point &pt)
{
    const ReferencePoint &reference{m_references[m_active_reference]};
    const std::complex<double> pixel_position{static_cast<double>(pt.get_x()), static_cast<double>(pt.get_y())};
    const std::complex<double> delta_sub_0{pixel_delta(pixel_position) - reference.delta};
    std::complex<double> delta_sub_n{delta_sub_0};
    const bool at_reference{pixel_position == reference.pixel_position};
    int iteration{};

    double min_orbit{1e5}; // orbit value closest to origin
    long min_index{};      // iteration of min_orbit
    double magnitude;
    do
    {
        if (g_cur_fractal_specific->pert_pt == nullptr)
        {
            throw std::runtime_error("No perturbation point function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        g_cur_fractal_specific->pert_pt(reference.orbit[iteration], delta_sub_n, delta_sub_0);
        iteration++;
        magnitude = mag_squared(reference.orbit[iteration] + delta_sub_n);

        if (g_inside_method == ColorMethod::BOF60 || g_inside_method == ColorMethod::BOF61)
        {
            if (magnitude < min_orbit)
            {
                min_orbit = magnitude;
                min_index = iteration + 1L;
            }
        }

        // At the reference pixel, zero delta uses the reference orbit directly.
        if (!at_reference && (!std::isfinite(magnitude) || magnitude < reference.tolerance[iteration]))
        {
            return -1;
        }
    } while (magnitude < g_magnitude_limit && iteration < g_max_iterations);

    int index;
    const double rq_lim2{std::sqrt(g_magnitude_limit)};
    const std::complex<double> w{reference.orbit[iteration] + delta_sub_n};

    if (g_biomorph >= 0)
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
                index = iteration % 256;
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
                index = iteration % 256;
            }
            break;

        case ColorMethod::ZMAG:
            if (iteration == g_max_iterations)
            {
                index = static_cast<int>((w.real() * w.real() + w.imag() + w.imag()) * (g_max_iterations >> 1) + 1);
            }
            else
            {
                index = iteration % 256;
            }
            break;

        case ColorMethod::REAL:
            if (iteration == g_max_iterations)
            {
                index = g_max_iterations;
            }
            else
            {
                index = iteration + static_cast<long>(w.real()) + 7;
            }
            break;

        case ColorMethod::IMAG:
            if (iteration == g_max_iterations)
            {
                index = g_max_iterations;
            }
            else
            {
                index = iteration + static_cast<long>(w.imag()) + 7;
            }
            break;

        case ColorMethod::MULT:
            if (iteration == g_max_iterations)
            {
                index = g_max_iterations;
            }
            else if (w.imag())
            {
                index = static_cast<long>(static_cast<double>(iteration) * (w.real() / w.imag()));
            }
            else
            {
                index = iteration;
            }
            break;

        case ColorMethod::SUM:
            if (iteration == g_max_iterations)
            {
                index = g_max_iterations;
            }
            else
            {
                index = iteration + static_cast<long>(w.real() + w.imag());
            }
            break;

        case ColorMethod::ATAN:
            if (iteration == g_max_iterations)
            {
                index = g_max_iterations;
            }
            else
            {
                index = static_cast<long>(std::abs(atan2(w.imag(), w.real()) * 180.0 / PI));
            }
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
                    index = iteration % 256;
                }
            }
            break;
        }

        if (g_inside_method >= ColorMethod::COLOR) // no filter
        {
            if (iteration == g_max_iterations)
            {
                index = g_inside_color;
            }
            else
            {
                index = iteration % 256;
            }
        }
        else
        {
            switch (g_inside_method)
            {
            case ColorMethod::ZMAG:
                if (iteration == g_max_iterations)
                {
                    index = static_cast<int>(mag_squared(w) * (g_max_iterations >> 1) + 1);
                }
                break;
            case ColorMethod::BOF60:
                if (iteration == g_max_iterations)
                {
                    index = static_cast<int>(std::sqrt(min_orbit) * 75.0);
                }
                break;
            case ColorMethod::BOF61:
                if (iteration == g_max_iterations)
                {
                    index = min_index;
                }
                break;

            default:
                break;
            }
        }
    }
    g_color = std::abs(index);
    if (g_color >= g_colors)
    {
        g_color = g_colors < 16 ? g_color & g_and_color : (g_color - 1) % g_and_color + 1;
    }
    if (g_std_calc_mode == CalcMode::BOUNDARY_TRACE && g_color == 0 &&
        g_debug_flag != DebugFlags::FORCE_BOUNDARY_TRACE_ERROR)
    {
        g_color = 1;
    }
    g_plot(pt.get_x(), g_screen_y_dots - 1 - pt.get_y(), g_color);
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

    copy_bf(z_bf.x, center.x);
    copy_bf(z_bf.y, center.y);

    for (int i = 0; i <= g_max_iterations; i++)
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
        g_cur_fractal_specific->pert_ref_bf(center, z_bf);
    }
}

void PertEngine::reference_zoom_point(ReferencePoint &reference, const std::complex<double> &center)
{
    std::complex<double> z = center;

    for (int i = 0; i <= g_max_iterations; i++)
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
        g_cur_fractal_specific->pert_ref(center, z);
    }
}

} // namespace id::engine
