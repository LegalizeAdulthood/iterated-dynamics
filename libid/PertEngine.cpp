// SPDX-License-Identifier: GPL-3.0-only
//
// Perturbation passes engine
//
// Thanks to Claude Heiland-Allen
// <https://fractalforums.org/programming/11/perturbation-code-for-cubic-and-higher-order-polynomials/2783>

#include "PertEngine.h"

#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "complex_fn.h"
#include "drivers.h"
#include "fractalp.h"
#include "id.h"
#include "id_data.h"
#include "pickover_mandelbrot.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

// Raising this number makes more calculations, but less variation between each calculation (less chance
// of mis-identifying a glitched point).
constexpr double GLITCH_TOLERANCE{1e-6};

void PertEngine::initialize_frame(
    const BFComplex &center_bf, const std::complex<double> &center, double zoom_radius)
{
    m_zoom_radius = zoom_radius;

    if (g_bf_math != bf_math_type::NONE)
    {
        m_saved_stack = save_stack();
        m_center_bf.x = alloc_stack(g_bf_length + 2);
        m_center_bf.y = alloc_stack(g_bf_length + 2);
        copy_bf(m_center_bf.x, center_bf.x);
        copy_bf(m_center_bf.y, center_bf.y);
    }
    else
    {
        m_center = center;
    }
}

// Full frame calculation
int PertEngine::calculate_one_frame()
{
    BFComplex c_bf{};
    BFComplex reference_coordinate_bf;
    std::complex<double> c;
    std::complex<double> reference_coordinate;

    m_reference_points = 0;
    m_glitch_point_count = 0L;
    m_remaining_point_count = 0L;

    m_points_remaining.resize(g_screen_x_dots * g_screen_y_dots);
    m_glitch_points.resize(g_screen_x_dots * g_screen_y_dots);
    m_perturbation_tolerance_check.resize(g_max_iterations * 2);
    m_xn.resize(g_max_iterations + 1);

    // calculate the pascal's triangle coefficients for powers > 3
    pascal_triangle();
    // Fill the list of points with all points in the image.
    for (long y = 0; y < g_screen_y_dots; y++)
    {
        for (long x = 0; x < g_screen_x_dots; x++)
        {
            Point pt(x, g_screen_y_dots - 1 - y);
            m_points_remaining[y * g_screen_x_dots + x] = pt;
            m_remaining_point_count++;
        }
    }

    double magnified_radius = m_zoom_radius;
    int window_radius = std::min(g_screen_x_dots, g_screen_y_dots);
    BigStackSaver saved;
    bf_t tmp_bf;

    if (g_bf_math != bf_math_type::NONE)
    {
        c_bf.x = alloc_stack(g_r_bf_length + 2);
        c_bf.y = alloc_stack(g_r_bf_length + 2);
        reference_coordinate_bf.x = alloc_stack(g_r_bf_length + 2);
        reference_coordinate_bf.y = alloc_stack(g_r_bf_length + 2);
        tmp_bf = alloc_stack(g_r_bf_length + 2);
    }

    while (m_remaining_point_count > (g_screen_x_dots * g_screen_y_dots) * (m_percent_glitch_tolerance / 100))
    {
        m_reference_points++;

        // Determine the reference point to calculate
        // Check whether this is the first time running the loop.
        if (m_reference_points == 1)
        {
            if (g_bf_math != bf_math_type::NONE)
            {
                copy_bf(c_bf.x, m_center_bf.x);
                copy_bf(c_bf.y, m_center_bf.y);
                copy_bf(reference_coordinate_bf.x, c_bf.x);
                copy_bf(reference_coordinate_bf.y, c_bf.y);
            }
            else
            {
                c = m_center;
                reference_coordinate = m_center;
            }

            m_delta_real = 0;
            m_delta_imag = 0;

            if (g_bf_math != bf_math_type::NONE)
            {
                reference_zoom_point(reference_coordinate_bf, g_max_iterations);
            }
            else
            {
                reference_zoom_point(reference_coordinate, g_max_iterations);
            }
        }
        else
        {
            if (!m_calculate_glitches)
            {
                break;
            }

            std::srand(g_random_seed);
            if (!g_random_seed_flag)
            {
                ++g_random_seed;
            }
            const int index{(int) ((double) std::rand() / RAND_MAX * m_remaining_point_count)};
            Point pt{m_points_remaining[index]};
            // Get the complex point at the chosen reference point
            double delta_real = magnified_radius * (2 * pt.get_x() - g_screen_x_dots) / window_radius;
            double delta_imag = -magnified_radius * (2 * pt.get_y() - g_screen_y_dots) / window_radius;

            // We need to store this offset because the formula we use to convert pixels into a complex point
            // does so relative to the center of the image. We need to offset that calculation when our
            // reference point isn't in the center. The actual offsetting is done in calculate point.

            m_delta_real = delta_real;
            m_delta_imag = delta_imag;

            if (g_bf_math != bf_math_type::NONE)
            {
                floattobf(tmp_bf, delta_real);
                add_bf(reference_coordinate_bf.x, c_bf.x, tmp_bf);
                floattobf(tmp_bf, delta_imag);
                add_bf(reference_coordinate_bf.y, c_bf.y, tmp_bf);
            }
            else
            {
                reference_coordinate.real(c.real() + delta_real);
                reference_coordinate.imag(c.imag() + delta_imag);
            }

            if (g_bf_math != bf_math_type::NONE)
            {
                reference_zoom_point(reference_coordinate_bf, g_max_iterations);
            }
            else
            {
                reference_zoom_point(reference_coordinate, g_max_iterations);
            }
        }

        int last_checked{-1};
        m_glitch_point_count = 0;
        for (int i = 0; i < m_remaining_point_count; i++)
        {
            if (i % 1000 == 0 && driver_key_pressed())
            {
                return -1;
            }
            Point pt{m_points_remaining[i]};
            if (calculate_point(pt, magnified_radius, window_radius) < 0)
            {
                return -1;
            }
            // Everything else in this loop is just for updating the progress counter.
            double progress = (double) i / m_remaining_point_count;
            if (int(progress * 100) != last_checked)
            {
                last_checked = int(progress * 100);
                m_status = "Pass: " + std::to_string(m_reference_points) + ", Ref (" +
                    std::to_string(int(progress * 100)) + "%)";
            }
        }

        // These points are glitched, so we need to mark them for recalculation. We need to recalculate them
        // using Pauldelbrot's glitch fixing method (see calculate point).
        std::memcpy(m_points_remaining.data(), m_glitch_points.data(), sizeof(Point) * m_glitch_point_count);
        m_remaining_point_count = m_glitch_point_count;
    }

    cleanup();
    return 0;
}

void PertEngine::cleanup()
{
    if (g_bf_math != bf_math_type::NONE)
    {
        restore_stack(m_saved_stack);
    }
    m_points_remaining.clear();
    m_glitch_points.clear();
    m_perturbation_tolerance_check.clear();
    m_xn.clear();
}

int PertEngine::calculate_point(const Point &pt, double magnified_radius, int window_radius)
{
    // Get the complex number at this pixel.
    // This calculates the number relative to the reference point, so we need to translate that to the center
    // when the reference point isn't in the center. That's why for the first reference,
    // m_calculated_real_delta and m_calculated_imaginary_delta are 0: it's calculating relative to the
    // center.
    const double delta_real =
        ((magnified_radius * (2 * pt.get_x() - g_screen_x_dots)) / window_radius) - m_delta_real;
    const double delta_imaginary =
        ((-magnified_radius * (2 * pt.get_y() - g_screen_y_dots)) / window_radius) - m_delta_imag;
    std::complex<double> delta_sub_0{delta_real, delta_imaginary};
    std::complex<double> delta_sub_n{delta_real, delta_imaginary};
    int iteration{};
    bool glitched{};

    double min_orbit{}; // orbit value closest to origin
    long min_index{};   // iteration of min_orbit
    if (g_inside_color == BOF60 || g_inside_color == BOF61)
    {
        min_orbit = 100000.0;
    }

    do
    {
        if (g_cur_fractal_specific->pert_pt == nullptr)
        {
            throw std::runtime_error("No perturbation point function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        g_cur_fractal_specific->pert_pt(m_xn[iteration], delta_sub_n, delta_sub_0);
        iteration++;
        std::complex<double> coord_mag{m_xn[iteration] + delta_sub_n};
        m_z_magnitude_squared = sqr(coord_mag.real()) + sqr(coord_mag.imag());

        if (g_inside_color == BOF60 || g_inside_color == BOF61)
        {
            const std::complex<double> z{m_xn[iteration] + delta_sub_n};
            const double bof_magnitude{mag_squared(z)};
            if (bof_magnitude < min_orbit)
            {
                min_orbit = bof_magnitude;
                min_index = iteration + 1L;
            }
        }

        // This is Pauldelbrot's glitch detection method. You can see it here:
        // http://www.fractalforums.com/announcements-and-news/pertubation-theory-glitches-improvement/. As
        // for why it looks so weird, it's because I've squared both sides of his equation and moved the
        // |ZsubN| to the other side to be precalculated. For more information, look at where the reference
        // point is calculated. I also only want to store this point once.
        if (m_calculate_glitches && !glitched &&
            m_z_magnitude_squared < m_perturbation_tolerance_check[iteration])
        {
            m_glitch_points[m_glitch_point_count] = Point(pt.get_x(), pt.get_y(), iteration);
            m_glitch_point_count++;
            glitched = true;
            break;
        }
    } while (m_z_magnitude_squared < g_magnitude_limit && iteration < g_max_iterations);

    if (!glitched)
    {
        int index;
        const double rqlim2{std::sqrt(g_magnitude_limit)};
        const std::complex<double> w{m_xn[iteration] + delta_sub_n};

        if (g_biomorph >= 0)
        {
            if (iteration == g_max_iterations)
            {
                index = g_max_iterations;
            }
            else
            {
                if (fabs(w.real()) < rqlim2 || fabs(w.imag()) < rqlim2)
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
            switch (g_outside_color)
            {
            case 0: // no filter
                if (iteration == g_max_iterations)
                {
                    index = g_max_iterations;
                }
                else
                {
                    index = iteration % 256;
                }
                break;

            case ZMAG:
                if (iteration == g_max_iterations)
                {
                    index = (int) ((w.real() * w.real() + w.imag() + w.imag()) * (g_max_iterations >> 1) + 1);
                }
                else
                {
                    index = iteration % 256;
                }
                break;

            case REAL:
                if (iteration == g_max_iterations)
                {
                    index = g_max_iterations;
                }
                else
                {
                    index = iteration + (long) w.real() + 7;
                }
                break;

            case IMAG:
                if (iteration == g_max_iterations)
                {
                    index = g_max_iterations;
                }
                else
                {
                    index = iteration + (long) w.imag() + 7;
                }
                break;

            case MULT:
                if (iteration == g_max_iterations)
                {
                    index = g_max_iterations;
                }
                else if (w.imag())
                {
                    index = (long) ((double) iteration * (w.real() / w.imag()));
                }
                else
                {
                    index = iteration;
                }
                break;

            case SUM:
                if (iteration == g_max_iterations)
                    index = g_max_iterations;
                else
                    index = iteration + (long) (w.real() + w.imag());
                break;

            case ATAN:
                if (iteration == g_max_iterations)
                {
                    index = g_max_iterations;
                }
                else
                {
                    index = (long) fabs(atan2(w.imag(), w.real()) * 180.0 / PI);
                }
                break;

            default:
                if (g_potential_flag)
                {
                    const double magnitude{sqr(w.real()) + sqr(w.imag())};
                    index = potential(magnitude, iteration);
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

            if (g_inside_color >= 0) // no filter
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
                switch (g_inside_color)
                {
                case ZMAG:
                    if (iteration == g_max_iterations)
                    {
                        index = (int) (mag_squared(w) * (g_max_iterations >> 1) + 1);
                    }
                    break;
                case BOF60:
                    if (iteration == g_max_iterations)
                    {
                        index = (int) (std::sqrt(min_orbit) * 75.0);
                    }
                    break;
                case BOF61:
                    if (iteration == g_max_iterations)
                    {
                        index = min_index;
                    }
                    break;
                }
            }
        }
        g_plot(pt.get_x(), g_screen_y_dots - 1 - pt.get_y(), index);
    }
    return 0;
}

void PertEngine::reference_zoom_point(const BFComplex &center, int max_iteration)
{
    BigStackSaver saved;

    BFComplex z_bf;
    z_bf.x = alloc_stack(g_r_bf_length + 2);
    z_bf.y = alloc_stack(g_r_bf_length + 2);
    bf_t temp_real_bf = alloc_stack(g_r_bf_length + 2);
    bf_t temp_imag_bf = alloc_stack(g_r_bf_length + 2);
    bf_t tmp_bf = alloc_stack(g_r_bf_length + 2);

    copy_bf(z_bf.x, center.x);
    copy_bf(z_bf.y, center.y);

    for (int i = 0; i <= max_iteration; i++)
    {
        std::complex<double> c;
        c.real(bftofloat(z_bf.x));
        c.imag(bftofloat(z_bf.y));

        m_xn[i] = c;
        // Norm is the squared version of abs and 0.000001 is 10^-3 squared.
        // The reason we are storing this into an array is that we need to check the magnitude against this
        // value to see if the value is glitched. We are leaving it squared because otherwise we'd need to do
        // a square root operation, which is expensive, so we'll just compare this to the squared magnitude.

        // Everything else in this loop is just for updating the progress counter.
        int last_checked = -1;
        double progress = (double) i / max_iteration;
        if (int(progress * 100) != last_checked)
        {
            last_checked = int(progress * 100);
            m_status = "Pass: " + std::to_string(m_reference_points) + ", Ref (" +
                std::to_string(int(progress * 100)) + "%)";
        }

        floattobf(tmp_bf, GLITCH_TOLERANCE);
        mult_bf(temp_real_bf, z_bf.x, tmp_bf);
        mult_bf(temp_imag_bf, z_bf.y, tmp_bf);
        std::complex<double> tolerance;
        tolerance.real(bftofloat(temp_real_bf));
        tolerance.imag(bftofloat(temp_imag_bf));

        m_perturbation_tolerance_check[i] = sqr(tolerance.real()) + sqr(tolerance.imag());

        if (g_cur_fractal_specific->pert_ref_bf == nullptr)
        {
            throw std::runtime_error("No reference orbit function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        g_cur_fractal_specific->pert_ref_bf(center, z_bf);
    }
}

void PertEngine::reference_zoom_point(const std::complex<double> &center, int max_iteration)
{
    std::complex<double> z = center;

    for (int i = 0; i <= max_iteration; i++)
    {
        std::complex<double> c = z;

        m_xn[i] = c;
        // Norm is the squared version of abs and 0.000001 is 10^-3 squared.
        // The reason we are storing this into an array is that we need to check the magnitude against this
        // value to see if the value is glitched. We are leaving it squared because otherwise we'd need to do
        // a square root operation, which is expensive, so we'll just compare this to the squared magnitude.

        // Everything else in this loop is just for updating the progress counter.
        int last_checked = -1;
        double progress = (double) i / max_iteration;
        if (int(progress * 100) != last_checked)
        {
            last_checked = int(progress * 100);
            m_status = "Pass: " + std::to_string(m_reference_points) + ", Ref (" +
                std::to_string(int(progress * 100)) + "%)";
        }

        std::complex<double> tolerance = z * GLITCH_TOLERANCE;
        m_perturbation_tolerance_check[i] = sqr(tolerance.real()) + sqr(tolerance.imag());

        if (g_cur_fractal_specific->pert_ref == nullptr)
        {
            throw std::runtime_error("No reference orbit function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        g_cur_fractal_specific->pert_ref(center, z);
    }
}
