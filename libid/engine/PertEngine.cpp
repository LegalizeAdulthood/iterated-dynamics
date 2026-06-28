// SPDX-License-Identifier: GPL-3.0-only
//
// Perturbation passes engine
//
// Thanks to Claude Heiland-Allen
// <https://fractalforums.org/programming/11/perturbation-code-for-cubic-and-higher-order-polynomials/2783>
//
// Much of perturbation code is based on the work of Shirom Makkad
// <https://github.com/ShiromMakkad/MandelbrotPerturbation>

#include "engine/PertEngine.h"

#include "engine/calcfrac.h"
#include "engine/Potential.h"
#include "engine/random_seed.h"
#include "engine/VideoInfo.h"
#include "fractals/fractalp.h"
#include "fractals/pickover_mandelbrot.h"
#include "math/biginit.h"
#include "math/complex_fn.h"
#include "misc/id.h"
#include "ui/video.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

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
    select_center_reference();
    m_frame_initialized = true;
}

int PertEngine::calculate_pixel(const Point &pt)
{
    if (!m_frame_initialized)
    {
        initialize_pixel_strategy();
    }

    return calculate_point(pt, m_magnified_radius, m_window_radius);
}

int PertEngine::calculate_pixel(const int col, const int row)
{
    return calculate_pixel(Point{col, g_screen_y_dots - 1 - row});
}

bool PertEngine::retry_needed(const std::vector<Point> &points) const
{
    return m_calculate_glitches &&
        points.size() >
        static_cast<std::size_t>(g_screen_x_dots * g_screen_y_dots * (m_percent_glitch_tolerance / 100));
}

bool PertEngine::select_retry_reference(const std::vector<Point> &points)
{
    if (!retry_needed(points))
    {
        return false;
    }

    set_random_seed();

    const int index{static_cast<int>(random_unit() * points.size())};
    select_reference_point(points[index]);
    return true;
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
    return m_reference_points;
}

std::vector<Point> PertEngine::take_glitch_points()
{
    std::vector<Point> points;
    points.reserve(m_glitch_point_count);
    for (long i = 0; i < m_glitch_point_count; ++i)
    {
        points.push_back(m_glitch_points[i]);
    }
    m_glitch_point_count = 0;
    return points;
}

void PertEngine::finish()
{
    if (!m_done)
    {
        complete_frame();
    }
}

void PertEngine::cleanup()
{
    if (m_center_stack_saved)
    {
        restore_stack(m_saved_stack);
        m_center_stack_saved = false;
    }
    m_glitch_points.clear();
    m_perturbation_tolerance_check.clear();
    m_xn.clear();
    m_c_bf = {};
    m_reference_coordinate_bf = {};
    m_tmp_bf = {};
    m_center_bf = {};
    m_done = true;
    m_frame_initialized = false;
}

void PertEngine::complete_frame()
{
    cleanup();
}

void PertEngine::initialize_pixel_state()
{
    m_reference_points = 0;
    m_glitch_point_count = 0L;
    m_magnified_radius = m_zoom_radius;
    m_window_radius = std::min(g_screen_x_dots, g_screen_y_dots);

    m_glitch_points.resize(g_screen_x_dots * g_screen_y_dots);
    m_perturbation_tolerance_check.resize(g_max_iterations * 2);
    m_xn.resize(g_max_iterations + 1);

    pascal_triangle();
    allocate_working_values();
}

void PertEngine::allocate_working_values()
{
    if (g_bf_math != BFMathType::NONE)
    {
        m_c_bf.x = alloc_stack(g_r_bf_length + 2);
        m_c_bf.y = alloc_stack(g_r_bf_length + 2);
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

bool PertEngine::select_center_reference()
{
    m_reference_points++;

    if (g_bf_math != BFMathType::NONE)
    {
        copy_bf(m_c_bf.x, m_center_bf.x);
        copy_bf(m_c_bf.y, m_center_bf.y);
        copy_bf(m_reference_coordinate_bf.x, m_c_bf.x);
        copy_bf(m_reference_coordinate_bf.y, m_c_bf.y);
    }
    else
    {
        m_c = m_center;
        m_reference_coordinate = m_center;
    }

    m_delta_real = 0;
    m_delta_imag = 0;

    if (g_bf_math != BFMathType::NONE)
    {
        reference_zoom_point(m_reference_coordinate_bf, g_max_iterations);
    }
    else
    {
        reference_zoom_point(m_reference_coordinate, g_max_iterations);
    }
    return true;
}

void PertEngine::select_reference_point(const Point &pt)
{
    m_reference_points++;

    const double delta_real = m_magnified_radius * (2 * pt.get_x() - g_screen_x_dots) / m_window_radius;
    const double delta_imag = -m_magnified_radius * (2 * pt.get_y() - g_screen_y_dots) / m_window_radius;

    m_delta_real = delta_real;
    m_delta_imag = delta_imag;

    if (g_bf_math != BFMathType::NONE)
    {
        float_to_bf(m_tmp_bf, delta_real);
        add_bf(m_reference_coordinate_bf.x, m_c_bf.x, m_tmp_bf);
        float_to_bf(m_tmp_bf, delta_imag);
        add_bf(m_reference_coordinate_bf.y, m_c_bf.y, m_tmp_bf);
    }
    else
    {
        m_reference_coordinate.real(m_c.real() + delta_real);
        m_reference_coordinate.imag(m_c.imag() + delta_imag);
    }

    if (g_bf_math != BFMathType::NONE)
    {
        reference_zoom_point(m_reference_coordinate_bf, g_max_iterations);
    }
    else
    {
        reference_zoom_point(m_reference_coordinate, g_max_iterations);
    }
}

int PertEngine::calculate_point(const Point &pt, const double magnified_radius, const int window_radius)
{
    // Get the complex number at this pixel.
    // This calculates the number relative to the reference point, so we need to translate that to the center
    // when the reference point isn't in the center. That's why for the first reference,
    // m_calculated_real_delta and m_calculated_imaginary_delta are 0: it's calculating relative to the
    // center.
    const double delta_real = magnified_radius * (2 * pt.get_x() - g_screen_x_dots) / window_radius - m_delta_real;
    const double delta_imaginary =
        -magnified_radius * (2 * pt.get_y() - g_screen_y_dots) / window_radius - m_delta_imag;
    const std::complex<double> delta_sub_0{delta_real, delta_imaginary};
    std::complex<double> delta_sub_n{delta_real, delta_imaginary};
    int iteration{};
    bool glitched{};

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
        g_cur_fractal_specific->pert_pt(m_xn[iteration], delta_sub_n, delta_sub_0);
        iteration++;
        magnitude = mag_squared(m_xn[iteration] + delta_sub_n);

        if (g_inside_method == ColorMethod::BOF60 || g_inside_method == ColorMethod::BOF61)
        {
            if (magnitude < min_orbit)
            {
                min_orbit = magnitude;
                min_index = iteration + 1L;
            }
        }

        // This is Pauldelbrot's glitch detection method. You can see it here:
        // http://www.fractalforums.com/announcements-and-news/pertubation-theory-glitches-improvement/. As
        // for why it looks so weird, it's because I've squared both sides of his equation and moved the
        // |ZsubN| to the other side to be precalculated. For more information, look at where the reference
        // point is calculated. I also only want to store this point once.
        if (m_calculate_glitches && !glitched && magnitude < m_perturbation_tolerance_check[iteration])
        {
            m_glitch_points[m_glitch_point_count] = Point(pt.get_x(), pt.get_y(), iteration);
            m_glitch_point_count++;
            glitched = true;
            break;
        }
    } while (magnitude < g_magnitude_limit && iteration < g_max_iterations);

    if (!glitched)
    {
        int index;
        const double rq_lim2{std::sqrt(g_magnitude_limit)};
        const std::complex<double> w{m_xn[iteration] + delta_sub_n};

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
        g_color = index;
        g_plot(pt.get_x(), g_screen_y_dots - 1 - pt.get_y(), g_color);
    }
    else
    {
        g_color = get_color(pt.get_x(), g_screen_y_dots - 1 - pt.get_y());
    }
    return g_color;
}

void PertEngine::reference_zoom_point(const BFComplex &center, const int max_iteration)
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

    for (int i = 0; i <= max_iteration; i++)
    {
        std::complex<double> c;
        c.real(bf_to_float(z_bf.x));
        c.imag(bf_to_float(z_bf.y));

        m_xn[i] = c;
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

        m_perturbation_tolerance_check[i] = mag_squared(tolerance);

        if (g_cur_fractal_specific->pert_ref_bf == nullptr)
        {
            throw std::runtime_error("No reference orbit function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        g_cur_fractal_specific->pert_ref_bf(center, z_bf);
    }
}

void PertEngine::reference_zoom_point(const std::complex<double> &center, const int max_iteration)
{
    std::complex<double> z = center;

    for (int i = 0; i <= max_iteration; i++)
    {
        m_xn[i] = z;

        // Norm is the squared version of abs and the perturbation tolerance is squared.
        // The reason we are storing this into an array is that we need to check the magnitude against this
        // value to see if the value is glitched. We are leaving it squared because otherwise we'd need to do
        // a square root operation, which is expensive, so we'll just compare this to the squared magnitude.

        m_perturbation_tolerance_check[i] = glitch_tolerance_threshold(z);

        if (g_cur_fractal_specific->pert_ref == nullptr)
        {
            throw std::runtime_error("No reference orbit function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        g_cur_fractal_specific->pert_ref(center, z);
    }
}

} // namespace id::engine
