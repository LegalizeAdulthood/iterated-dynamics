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
#include "engine/perturbation.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "fractals/pickover_mandelbrot.h"
#include "math/biginit.h"
#include "math/complex_fn.h"
#include "misc/driver.h"
#include "misc/id.h"
#include "ui/cmdfiles.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

// Raising this number makes more calculations, but less variation between each calculation (less chance
// of mis-identifying a glitched point).
double GLITCH_TOLERANCE = g_perturbation_tolerance;

void PertEngine::initialize_frame(
    const BFComplex &center_bf, const std::complex<double> &center, double zoom_radius)
{
    m_zoom_radius = zoom_radius;

    cleanup();
    if (g_bf_math != BFMathType::NONE)
    {
        m_saved_stack = save_stack();
        m_center_bf.x = alloc_stack(g_bf_length + 2);
        m_center_bf.y = alloc_stack(g_bf_length + 2);
        m_c_bf.x = alloc_stack(g_r_bf_length + 2);
        m_c_bf.y = alloc_stack(g_r_bf_length + 2);
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
    BFComplex reference_coordinate_bf;
    std::complex<double> reference_coordinate;

    m_reference_points = 0;
    m_glitch_point_count = 0L;
    m_remaining_point_count = 0L;

    m_points_remaining.resize(g_screen_x_dots * g_screen_y_dots);
    m_glitch_points.resize(g_screen_x_dots * g_screen_y_dots);
    m_perturbation_tolerance_check.resize(g_max_iterations * 2);
    m_xn.resize(g_max_iterations + 1);
    m_glitches.resize(g_screen_x_dots * g_screen_y_dots);

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
            m_glitches[x + y * g_screen_x_dots] = -1; // not yet processed
            }
    }

    double magnified_radius = m_zoom_radius;
    int window_radius = std::min(g_screen_x_dots, g_screen_y_dots);
    int saved = save_stack();
    BigFloat tmp_bf;

    if (g_bf_math != BFMathType::NONE)
    {
        reference_coordinate_bf.x = alloc_stack(g_r_bf_length + 2);
        reference_coordinate_bf.y = alloc_stack(g_r_bf_length + 2);
        tmp_bf = alloc_stack(g_r_bf_length + 2);
    }

    if (m_remaining_point_count > (g_screen_x_dots * g_screen_y_dots) * (m_percent_glitch_tolerance / 100))
    {
        m_reference_points++;

        // Determine the reference point to calculate
        // Check whether this is the first time running the loop.
        if (m_reference_points == 1)
        {
            if (g_bf_math != BFMathType::NONE)
            {
                copy_bf(m_c_bf.x, m_center_bf.x);
                copy_bf(m_c_bf.y, m_center_bf.y);
                copy_bf(reference_coordinate_bf.x, m_c_bf.x);
                copy_bf(reference_coordinate_bf.y, m_c_bf.y);
            }
            else
            {
                m_c = m_center;
                reference_coordinate = m_center;
                m_old_reference_coordinate = reference_coordinate;
            }

            m_delta_real = 0;
            m_delta_imag = 0;

            if (g_bf_math != BFMathType::NONE)
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
            restore_stack(saved);
            return 0;
        }
    }

    restore_stack(saved);
    //    cleanup();
    return 0;
}

void PertEngine::cleanup()
{
    if (g_bf_math != BFMathType::NONE)
    {
        restore_stack(m_saved_stack);
    }
    m_points_remaining.clear();
    m_glitch_points.clear();
    m_perturbation_tolerance_check.clear();
    m_xn.clear();
}

void PertEngine::reference_zoom_point(const BFComplex &center, int max_iteration)
{
    int saved = save_stack();

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

        float_to_bf(tmp_bf, GLITCH_TOLERANCE);
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
    restore_stack(saved);
}

void PertEngine::reference_zoom_point(const std::complex<double> &center, int max_iteration)
{
    std::complex<double> z = center;

    for (int i = 0; i <= max_iteration; i++)
    {
        m_xn[i] = z;

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

        m_perturbation_tolerance_check[i] = mag_squared(z * GLITCH_TOLERANCE);

        if (g_cur_fractal_specific->pert_ref == nullptr)
        {
            throw std::runtime_error("No reference orbit function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
        }
        g_cur_fractal_specific->pert_ref(center, z);
    }
}

int PertEngine::perturbation_per_pixel(int x, int y, double bailout)
{
    double magnified_radius = m_zoom_radius;
    int window_radius = std::min(g_screen_x_dots, g_screen_y_dots);

    if (m_glitches[x + y * g_screen_x_dots] == 0) // processed and not glitched
        return -2;

    const double delta_real =
        ((magnified_radius * (2 * x - g_screen_x_dots)) / window_radius) - m_delta_real;
    const double delta_imaginary =
        ((-magnified_radius * (2 * y - g_screen_y_dots)) / window_radius) - m_delta_imag;
    m_delta_sub_0 = {delta_real, -delta_imaginary};
    m_delta_sub_n = m_delta_sub_0;
    m_points_count++;
    m_glitched = false;
    return 0;
}

int PertEngine::calculate_orbit(int x, int y, long iteration)
{
    // Get the complex number at this pixel.
    // This calculates the number relative to the reference point, so we need to translate that to the center
    // when the reference point isn't in the center. That's why for the first reference, calculatedRealDelta
    // and calculatedImaginaryDelta are 0: it's calculating relative to the center.

    std::complex<double> temp, temp1;
    double magnitude;

    temp1 = m_xn[iteration - 1] + m_delta_sub_n;
    g_cur_fractal_specific->pert_pt(m_xn[iteration - 1], m_delta_sub_n, m_delta_sub_0);

    if (g_cur_fractal_specific->pert_pt == nullptr)
    {
        throw std::runtime_error("No perturbation point function defined for fractal type (" +
                std::string{g_cur_fractal_specific->name} + ")");
    }
    temp = m_xn[iteration] + m_delta_sub_n;
    m_glitches[x + y * g_screen_x_dots] = 0; // assume not glitched
    magnitude = mag_squared(temp);

    // This is Pauldelbrot's glitch detection method. You can see it here:
    // http://www.fractalforums.com/announcements-and-news/pertubation-theory-glitches-improvement/. As for
    // why it looks so weird, it's because I've squared both sides of his equation and moved the |ZsubN| to
    // the other side to be precalculated. For more information, look at where the reference point is
    // calculated. I also only want to store this point once.
    if (m_calculate_glitches && !m_glitched && magnitude < m_perturbation_tolerance_check[iteration])
    {
        m_glitches[x + y * g_screen_x_dots] = 1; // yep, she's glitched
        Point pt(x, y, iteration);
        m_glitch_points[m_glitch_point_count] = pt;
        m_glitch_point_count++;
        m_glitched = true;
        return true;
    }
    g_new_z.x = temp.real();
    g_new_z.y = temp.imag();
    // the following are needed because although perturbation operates in doubles, the math type may not be
    if (g_bf_math == BFMathType::BIG_NUM)
    {
        float_to_bn(g_new_z_bn.x, temp.real());
        float_to_bn(g_new_z_bn.y, temp.imag());
    }
    else if (g_bf_math == BFMathType::BIG_FLT)
    {
        float_to_bf(g_new_z_bf.x, temp.real());
        float_to_bf(g_new_z_bf.y, temp.imag());
    }
    return g_bailout_float();
    }

int PertEngine::calculate_reference()
{
    double magnified_radius = m_zoom_radius;
    int window_radius = std::min(g_screen_x_dots, g_screen_y_dots);
    BFComplex reference_coordinate_bf{};
    BigFloat tmp_bf{};
    std::complex<double> reference_coordinate;
    int saved = save_stack();

    if (m_calculate_glitches == false)
        return 0;

    if (g_bf_math != BFMathType::NONE)
    {
        reference_coordinate_bf.x = alloc_stack(g_r_bf_length + 2);
        reference_coordinate_bf.y = alloc_stack(g_r_bf_length + 2);
        tmp_bf = alloc_stack(g_r_bf_length + 2);
    }

    // These points are glitched, so we need to mark them for recalculation. We need to recalculate them
    // using Pauldelbrot's glitch fixing method (see calculate point).
    std::memcpy(m_points_remaining.data(), m_glitch_points.data(), sizeof(Point) * m_glitch_point_count);
    m_remaining_point_count = m_glitch_point_count;

    std::srand(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }

    m_reference_points++;

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

    if (g_bf_math != BFMathType::NONE)
    {
        float_to_bf(tmp_bf, delta_real);
        add_bf(reference_coordinate_bf.x, m_c_bf.x, tmp_bf);
        float_to_bf(tmp_bf, delta_imag);
        sub_bf(reference_coordinate_bf.y, m_c_bf.y, tmp_bf);
    }
    else
    {
        reference_coordinate.real(m_c.real() + delta_real);
        reference_coordinate.imag(m_c.imag() - delta_imag);
    }

    if (g_bf_math != BFMathType::NONE)
    {
        reference_zoom_point(reference_coordinate_bf, g_max_iterations);
    }
    else
    {
        reference_zoom_point(reference_coordinate, g_max_iterations);
    }

    if (g_bf_math != BFMathType::NONE)
    {
        restore_stack(saved);
    }

    m_glitch_point_count = 0;
    return 0;
}

long PertEngine::get_glitch_point_count()
{
    return (m_remaining_point_count > (g_screen_x_dots * g_screen_y_dots) * (m_percent_glitch_tolerance / 100));
}

void PertEngine::push_glitch(int x, int y, int value)
{
    m_glitches[x + y * g_screen_x_dots] = value;
}

bool PertEngine::is_glitched()
{
    return m_glitched;
}

bool PertEngine::is_pixel_complete(int x, int y)
{
    return (m_glitches[x + y * g_screen_x_dots] == 0);
}

void PertEngine::set_glitched(bool status)
{
    m_glitched = status;
}

void PertEngine::set_points_count(long count)
{
    m_points_count = count;
}

void PertEngine::set_glitch_points_count(long count)
{
    m_glitch_point_count = count;
}
