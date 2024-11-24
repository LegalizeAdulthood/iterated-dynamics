// SPDX-License-Identifier: GPL-3.0-only
//
// Perturbation passes engine
//
// Thanks to Claude Heiland-Allen
// <https://fractalforums.org/programming/11/perturbation-code-for-cubic-and-higher-order-polynomials/2783>

#include "PertEngine.h"

#include <algorithm>

#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "complex_fn.h"
#include "drivers.h"
#include "fractalb.h"
#include "id_data.h"

void PertEngine::initialize_frame(
    bf_t x_center_bf, bf_t y_center_bf, double x_center, double y_center, double zoom_radius)
{
    m_zoom_radius = zoom_radius;

    if (g_bf_math != bf_math_type::NONE)
    {
        m_saved = save_stack();
        m_zoom_pt_real_bf = alloc_stack(g_bf_length + 2);
        m_zoom_pt_imag_bf = alloc_stack(g_bf_length + 2);
        copy_bf(m_zoom_pt_real_bf, x_center_bf);
        copy_bf(m_zoom_pt_imag_bf, y_center_bf);
    }
    else
    {
        m_zoom_pt_real = x_center;
        m_zoom_pt_imag = y_center;
    }
}

// Generate Pascal's Triangle coefficients
static void load_pascal(long pascal_array[], int n)
{
    long j;
    long c = 1L;

    for (j = 0; j <= n; j++)
    {
        if (j == 0)
        {
            c = 1;
        }
        else
        {
            c = c * (n - j + 1) / j;
        }
        pascal_array[j] = c;
    }
}

// Full frame calculation
int PertEngine::calculate_one_frame(int power, int subtype)
{
    int i;
    BFComplex c_bf;
    BFComplex reference_coordinate_bf;
    std::complex<double> c;
    std::complex<double> reference_coordinate;

    m_reference_points = 0;
    m_glitch_point_count = 0L;
    m_remaining_point_count = 0L;

    // get memory for all point arrays
    m_points_remaining = new Point[g_screen_x_dots * g_screen_y_dots];
    if (m_points_remaining == NULL)
    {
        return -1;
    }
    m_glitch_points = new Point[g_screen_x_dots * g_screen_y_dots];
    if (m_glitch_points == NULL)
    {
        if (m_points_remaining)
        {
            delete[] m_points_remaining;
            m_points_remaining = NULL;
        }
        return -1;
    }
    // get memory for Perturbation Tolerance Check array
    m_perturbation_tolerance_check = new double[g_max_iterations * 2];
    if (m_perturbation_tolerance_check == NULL)
    {
        if (m_points_remaining)
        {
            delete[] m_points_remaining;
            m_points_remaining = NULL;
        }
        if (m_glitch_points)
        {
            delete[] m_glitch_points;
            m_glitch_points = NULL;
        }
        return -1;
    }
    // get memory for Z array
    m_xn = new std::complex<double>[g_max_iterations + 1];
    if (m_xn == NULL)
    {
        if (m_points_remaining)
        {
            delete[] m_points_remaining;
            m_points_remaining = NULL;
        }
        if (m_glitch_points)
        {
            delete[] m_glitch_points;
            m_glitch_points = NULL;
        }
        if (m_perturbation_tolerance_check)
        {
            delete[] m_perturbation_tolerance_check;
            m_perturbation_tolerance_check = NULL;
        }
        return -1;
    }
    m_power = std::min(std::max(power, 2), static_cast<int>(MAX_POWER));
    m_subtype = subtype;

    // calculate the pascal's triangle coefficients for powers > 3
    load_pascal(m_pascal_triangle, m_power);
    // Fill the list of points with all points in the image.
    for (long y = 0; y < g_screen_y_dots; y++)
    {
        for (long x = 0; x < g_screen_x_dots; x++)
        {
            Point pt(x, g_screen_y_dots - 1 - y);
            *(m_points_remaining + y * g_screen_x_dots + x) = pt;
            m_remaining_point_count++;
        }
    }

    double magnified_radius = m_zoom_radius;
    int window_radius = std::min(g_screen_x_dots, g_screen_y_dots);
    int cplxsaved;
    bf_t tmp_bf;

    if (g_bf_math != bf_math_type::NONE)
    {
        cplxsaved = save_stack();
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
                copy_bf(c_bf.x, m_zoom_pt_real_bf);
                copy_bf(c_bf.y, m_zoom_pt_imag_bf);
                copy_bf(reference_coordinate_bf.x, c_bf.x);
                copy_bf(reference_coordinate_bf.y, c_bf.y);
            }
            else
            {
                c.real(m_zoom_pt_real);
                c.imag(m_zoom_pt_imag);
                reference_coordinate = c;
            }

            m_delta_real = 0;
            m_delta_imag = 0;

            if (g_bf_math != bf_math_type::NONE)
            {
                reference_zoom_point_bf(reference_coordinate_bf, g_max_iterations);
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

            int referencePointIndex = 0;
            int Randomise;

            srand((unsigned) time(NULL)); // Seed the random-number generator with current time
            Randomise = rand();
            referencePointIndex = (int) ((double) Randomise / (RAND_MAX + 1) * m_remaining_point_count);
            Point pt = *(m_points_remaining + referencePointIndex);
            // Get the complex point at the chosen reference point
            double deltaReal = ((magnified_radius * (2 * pt.get_x() - g_screen_x_dots)) / window_radius);
            double deltaImaginary = ((-magnified_radius * (2 * pt.get_y() - g_screen_y_dots)) / window_radius);

            // We need to store this offset because the formula we use to convert pixels into a complex point
            // does so relative to the center of the image. We need to offset that calculation when our
            // reference point isn't in the center. The actual offsetting is done in calculate point.

            m_delta_real = deltaReal;
            m_delta_imag = deltaImaginary;

            if (g_bf_math != bf_math_type::NONE)
            {
                floattobf(tmp_bf, deltaReal);
                add_bf(reference_coordinate_bf.x, c_bf.x, tmp_bf);
                floattobf(tmp_bf, deltaImaginary);
                add_bf(reference_coordinate_bf.y, c_bf.y, tmp_bf);
            }
            else
            {
                reference_coordinate.real(c.real() + deltaReal);
                reference_coordinate.imag(c.imag() + deltaImaginary);
            }

            if (g_bf_math != bf_math_type::NONE)
            {
                reference_zoom_point_bf(reference_coordinate_bf, g_max_iterations);
            }
            else
            {
                reference_zoom_point(reference_coordinate, g_max_iterations);
            }
        }

        int lastChecked = -1;
        m_glitch_point_count = 0;
        for (i = 0; i < m_remaining_point_count; i++)
        {
            if (i % 1000 == 0 && driver_key_pressed())
            {
                return -1;
            }
            Point pt = *(m_points_remaining + i);
            if (calculate_point(pt, magnified_radius, window_radius, g_magnitude_limit, g_plot, potential) < 0)
            {
                return -1;
            }
            // Everything else in this loop is just for updating the progress counter.
            double progress = (double) i / m_remaining_point_count;
            if (int(progress * 100) != lastChecked)
            {
                lastChecked = int(progress * 100);
                m_status = "Pass: " + std::to_string(m_reference_points) + ", Ref (" +
                    std::to_string(int(progress * 100)) + "%)";
            }
        }

        // These points are glitched, so we need to mark them for recalculation. We need to recalculate them
        // using Pauldelbrot's glitch fixing method (see calculate point).
        memcpy(m_points_remaining, m_glitch_points, sizeof(Point) * m_glitch_point_count);
        m_remaining_point_count = m_glitch_point_count;
    }

    if (g_bf_math != bf_math_type::NONE)
    {
        restore_stack(cplxsaved);
    }
    cleanup();
    return 0;
}

void PertEngine::cleanup()
{
    if (g_bf_math != bf_math_type::NONE)
    {
        restore_stack(m_saved);
    }
    if (m_points_remaining)
    {
        delete[] m_points_remaining;
        m_points_remaining = NULL;
    }
    if (m_glitch_points)
    {
        delete[] m_glitch_points;
        m_glitch_points = NULL;
    }
    if (m_xn)
    {
        delete[] m_xn;
        m_xn = NULL;
    }
    if (m_perturbation_tolerance_check)
    {
        delete[] m_perturbation_tolerance_check;
        m_perturbation_tolerance_check = NULL;
    }
}

// Individual point calculation
int PertEngine::calculate_point(const Point &pt, double magnified_radius, int window_radius, double bailout,
    void (*plot)(int, int, int), int potential(double, long))
{
    // Get the complex number at this pixel.
    // This calculates the number relative to the reference point, so we need to translate that to the center
    // when the reference point isn't in the center. That's why for the first reference,
    // m_calculated_real_delta and m_calculated_imaginary_delta are 0: it's calculating relative to the
    // center.
    double delta_real =
        ((magnified_radius * (2 * pt.get_x() - g_screen_x_dots)) / window_radius) - m_delta_real;
    double delta_imaginary =
        ((-magnified_radius * (2 * pt.get_y() - g_screen_y_dots)) / window_radius) - m_delta_imag;
    double magnitude = 0.0;
    std::complex<double> delta_sub_0{delta_real, delta_imaginary};
    std::complex<double> delta_sub_n;
    delta_sub_n = delta_sub_0;
    int iteration = 0;
    bool glitched = false;

    double BOF_magnitude;
    double min_orbit; // orbit value closest to origin
    long min_index;   // iteration of min_orbit
    if (g_inside_color == BOF60 || g_inside_color == BOF61)
    {
        BOF_magnitude = 0.0;
        min_orbit = 100000.0;
    }

    // Iteration loop
    do
    {
        pert_functions((m_xn + iteration), &delta_sub_n, &delta_sub_0);
        iteration++;
        std::complex<double> CoordMag = *(m_xn + iteration) + delta_sub_n;
        m_z_magnitude_squared = sqr(CoordMag.real()) + sqr(CoordMag.imag());

        if (g_inside_color == BOF60 || g_inside_color == BOF61)
        {
            std::complex<double> z = *(m_xn + iteration) + delta_sub_n;
            BOF_magnitude = mag_squared(z);
            if (BOF_magnitude < min_orbit)
            {
                min_orbit = BOF_magnitude;
                min_index = iteration + 1L;
            }
        }

        // This is Pauldelbrot's glitch detection method. You can see it here:
        // http://www.fractalforums.com/announcements-and-news/pertubation-theory-glitches-improvement/. As
        // for why it looks so weird, it's because I've squared both sides of his equation and moved the
        // |ZsubN| to the other side to be precalculated. For more information, look at where the reference
        // point is calculated. I also only want to store this point once.
        if (m_calculate_glitches == true && glitched == false &&
            m_z_magnitude_squared < m_perturbation_tolerance_check[iteration])
        {
            Point pt(pt.get_x(), pt.get_y(), iteration);
            m_glitch_points[m_glitch_point_count] = pt;
            m_glitch_point_count++;
            glitched = true;
            break;
        }

        // use bailout radius of 256 for smooth coloring.
    } while (m_z_magnitude_squared < bailout && iteration < g_max_iterations);

    if (!glitched)
    {
        int index;
        double rqlim2 = sqrt(bailout);
        std::complex<double> w = m_xn[iteration] + delta_sub_n;

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
                    magnitude = sqr(w.real()) + sqr(w.imag());
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
                        index = (int) (sqrt(min_orbit) * 75.0);
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
            plot(pt.get_x(), g_screen_y_dots - 1 - pt.get_y(), index);
        }
    }
    return 0;
}

// Reference Zoom Point - BigFlt
void PertEngine::reference_zoom_point_bf(const BFComplex &center, int max_iteration)
{
    // Raising this number makes more calculations, but less variation between each calculation (less chance
    // of mis-identifying a glitched point).
    BFComplex z_times_2_bf;
    BFComplex z_bf;
    bf_t temp_real_bf;
    bf_t temp_imag_bf;
    bf_t tmp_bf;
    double glitch_tolerancy = 1e-6;
    int cplxsaved;

    cplxsaved = save_stack();
    z_times_2_bf.x = alloc_stack(g_r_bf_length + 2);
    z_times_2_bf.y = alloc_stack(g_r_bf_length + 2);
    z_bf.x = alloc_stack(g_r_bf_length + 2);
    z_bf.y = alloc_stack(g_r_bf_length + 2);
    temp_real_bf = alloc_stack(g_r_bf_length + 2);
    temp_imag_bf = alloc_stack(g_r_bf_length + 2);
    tmp_bf = alloc_stack(g_r_bf_length + 2);

    copy_bf(z_bf.x, center.x);
    copy_bf(z_bf.y, center.y);
    //    Z = *centre;

    for (int i = 0; i <= max_iteration; i++)
    {
        std::complex<double> c;
        // pre multiply by two
        double_bf(z_times_2_bf.x, z_bf.x);
        double_bf(z_times_2_bf.y, z_bf.y);

        c.real(bftofloat(z_bf.x));
        c.imag(bftofloat(z_bf.y));

        // The reason we are storing the same value times two is that we can precalculate this value here
        // because multiplying this value by two is needed many times in the program.
        // Also, for some reason, we can't multiply complex numbers by anything greater than 1 using
        // std::complex, so we have to multiply the individual terms each time. This is expensive to do above,
        // so we are just doing it here.

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

        floattobf(tmp_bf, glitch_tolerancy);
        mult_bf(temp_real_bf, z_bf.x, tmp_bf);
        mult_bf(temp_imag_bf, z_bf.y, tmp_bf);
        std::complex<double> tolerancy;
        tolerancy.real(bftofloat(temp_real_bf));
        tolerancy.imag(bftofloat(temp_imag_bf));

        m_perturbation_tolerance_check[i] = sqr(tolerancy.real()) + sqr(tolerancy.imag());

        // Calculate the set
        ref_functions_bf(center, &z_bf, &z_times_2_bf);
    }
    restore_stack(cplxsaved);
}

//////////////////////////////////////////////////////////////////////
// Reference Zoom Point - BigFlt
//////////////////////////////////////////////////////////////////////

void PertEngine::reference_zoom_point(const std::complex<double> &center, int max_iteration)
{
    // Raising this number makes more calculations, but less variation between each calculation (less chance
    // of mis-identifying a glitched point).
    std::complex<double> z_times_2;
    std::complex<double> z;
    double glitch_tolerancy = 1e-6;

    z = center;

    for (int i = 0; i <= max_iteration; i++)
    {
        // pre multiply by two
        z_times_2 = z + z;
        std::complex<double> c = z;

        // The reason we are storing the same value times two is that we can precalculate this value here
        // because multiplying this value by two is needed many times in the program.
        // Also, for some reason, we can't multiply complex numbers by anything greater than 1 using
        // std::complex, so we have to multiply the individual terms each time. This is expensive to do above,
        // so we are just doing it here.

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

        std::complex<double> tolerancy = z * glitch_tolerancy;
        m_perturbation_tolerance_check[i] = sqr(tolerancy.real()) + sqr(tolerancy.imag());

        // Calculate the set
        ref_functions(center, &z, &z_times_2);
    }
}

// Laser Blaster's Code for removing absolutes from Mandelbrot derivatives
inline double diff_abs(const double c, const double d)
{
    const double cd{c + d};

    if (c >= 0.0)
    {
        return cd >= 0.0 ? d : -d - 2.0 * c;
    }
    return cd > 0.0 ? d + 2.0 * c : -d;
}

// Individual function point calculations
//
void PertEngine::pert_functions(
    std::complex<double> *x_ref, std::complex<double> *delta_sub_n, std::complex<double> *delta_sub_0)
{
    double dnr;
    double dni;
    const double r = x_ref->real();
    const double i = x_ref->imag();
    const double a = delta_sub_n->real();
    const double b = delta_sub_n->imag();
    const double a2 = a * a;
    const double b2 = b * b;
    const double a0 = delta_sub_0->real();
    const double b0 = delta_sub_0->imag();
    const double r2 = r * r;
    const double i2 = i * i;
    double c;
    double d;

    switch (m_subtype)
    {
    case 0:               // Mandelbrot
        if (m_power == 3) // Cubic
        {
            dnr = 3 * r * r * a - 6 * r * i * b - 3 * i * i * a + 3 * r * a * a - 3 * r * b * b -
                3 * i * 2 * a * b + a * a * a - 3 * a * b * b + a0;
            dni = 3 * r * r * b + 6 * r * i * a - 3 * i * i * b + 3 * r * 2 * a * b + 3 * i * a * a -
                3 * i * b * b + 3 * a * a * b - b * b * b + b0;
            delta_sub_n->imag(dni);
            delta_sub_n->real(dnr);
        }
        else
        {
            dnr = (2 * r + a) * a - (2 * i + b) * b + a0;
            dni = 2 * ((r + a) * b + i * a) + b0;
            delta_sub_n->imag(dni);
            delta_sub_n->real(dnr);
        }
        break;

    case 1: // Power
    {
        std::complex<double> zp(1.0, 0.0);
        std::complex<double> sum(0.0, 0.0);
        for (int j = 0; j < m_power; j++)
        {
            sum += zp * (double) m_pascal_triangle[j];
            sum *= *delta_sub_n;
            zp *= *x_ref;
        }
        *delta_sub_n = sum;
        *delta_sub_n += *delta_sub_0;
    }
    break;

    case 2: // Burning Ship
        delta_sub_n->real(2.0 * a * r + a2 - 2.0 * b * i - b2);
        delta_sub_n->imag(diff_abs(r * i, r * b + i * a + a * b) * 2);
        *delta_sub_n += *delta_sub_0;
        break;

    case 3: // Cubic Burning Ship
    {
        dnr = diff_abs(r, a);
        double ab = r + a;
        dnr = (r * r - 3 * i * i) * dnr + (2 * a * r + a2 - 6 * i * b - 3 * b2) * fabs(ab) + a0;
        dni = diff_abs(i, b);
        ab = i + b;
        dni = (3 * r * r - i * i) * dni + (6 * r * a + 3 * a2 - 2 * i * b - b2) * fabs(ab) + b0;

        delta_sub_n->imag(dni);
        delta_sub_n->real(dnr);
    }
    break;

    case 4: // 4th Power Burning Ship
        dnr = 4 * r2 * r * a + 6 * r2 * a2 + 4 * r * a2 * a + a2 * a2 + 4 * i2 * i * b + 6 * i2 * b2 +
            4 * i * b2 * b + b2 * b2 - 12 * r2 * i * b - 6 * r2 * b2 - 12 * r * a * i2 - 24 * r * a * i * b -
            12 * r * a * b2 - 6 * a2 * i2 - 12 * a2 * i * b - 6 * a2 * b2 + a0;
        dni = diff_abs(r * i, r * b + a * i + a * b);
        dni = 4 * (r2 - i2) * (dni) +
            4 * fabs(r * i + r * b + a * i + a * b) * (2 * a * r + a2 - 2 * b * i - b2) + b0;

        delta_sub_n->imag(dni);
        delta_sub_n->real(dnr);
        break;

    case 5: // 5th Power Burning Ship
        dnr = diff_abs(r, a);
        dnr = (dnr) * (r * r * r * r - 10 * r * r * i * i + 5 * i * i * i * i) +
            fabs(r + a) *
                (4 * r * r * r * a + 6 * r * r * a2 + 4 * r * a2 * a + a2 * a2 - 20 * r2 * i * b -
                    10 * r2 * b2 - 20 * r * a * i2 - 40 * r * a * i * b - 20 * r * a * b2 - 10 * a2 * i2 -
                    20 * a2 * i * b - 10 * a2 * b2 + 20 * i2 * i * b + 30 * i2 * b2 + 20 * i * b2 * b +
                    5 * b2 * b2) +
            a0;
        dni = diff_abs(i, b);
        dni = (dni) * (5 * r2 * r2 - 10 * r2 * i2 + i2 * i2) +
            fabs(i + b) *
                (20 * r2 * r * a + 30 * r2 * a2 + 20 * r * a2 * a + 5 * a2 * a2 - 20 * r2 * i * b -
                    10 * r2 * b2 - 20 * r * a * i2 - 40 * r * a * i * b - 20 * r * a * b2 - 10 * a2 * i2 -
                    20 * a2 * i * b - 10 * a2 * b2 + 4 * i2 * i * b + 6 * i2 * b2 + 4 * i * b2 * b +
                    b2 * b2) +
            b0;

        delta_sub_n->imag(dni);
        delta_sub_n->real(dnr);
        break;

    case 6: // Celtic
        dnr = diff_abs(r2 - i2, (2 * r + a) * a - (2 * i + b) * b);
        dnr += a0;
        dni = 2 * r * b + 2 * a * (i + b) + b0;

        delta_sub_n->imag(dni);
        delta_sub_n->real(dnr);
        break;

    case 7: // Cubic Celtic
        c = r * (r2 - 3 * i2);
        d = a * (3 * r2 + a2) + 3 * r * (a2 - 2 * i * b - b2) - 3 * a * (i2 + 2 * i * b + b2);
        dnr = diff_abs(c, d);
        dnr = dnr + a0;
        dni = 3 * i * (2 * r * a + a2 - b2) + 3 * b * (r2 + 2 * r * a + a2) - b * (b2 + 3 * i2) + b0;

        delta_sub_n->imag(dni);
        delta_sub_n->real(dnr);
        break;

    case 8: // 4th Celtic Buffalo
        c = r2 * r2 + i2 * i2 - 6 * r2 * i2;
        d = 4 * r2 * r * a + 6 * r2 * a2 + 4 * r * a2 * a + a2 * a2 + 4 * i2 * i * b + 6 * i2 * b2 +
            4 * i * b2 * b + b2 * b2 - 12 * a * r * i2 - 6 * a2 * i2 - 12 * b * r2 * i - 24 * a * b * r * i -
            12 * a2 * b * i - 6 * b2 * r2 - 12 * a * b2 * r - 6 * a2 * b2;
        dnr = diff_abs(c, d);
        dnr += a0;
        dni = 12 * r2 * i * a + 12 * r * i * a2 - 12 * r * i2 * b - 12 * r * i * b2 + 4 * r2 * r * b +
            12 * r2 * b * a + 12 * r * b * a2 - 4 * r * b2 * b + 4 * a2 * a * i - 4 * a * i2 * i -
            12 * a * i2 * b - 12 * a * i * b2 + 4 * a2 * a * b - 4 * a * b2 * b + b0;

        delta_sub_n->imag(dni);
        delta_sub_n->real(dnr);
        break;

    case 9: // 5th Celtic
        c = r2 * r2 * r - 10 * r2 * r * i2 + 5 * r * i2 * i2;
        d = 20 * r * b * i2 * i - 30 * r2 * a * i2 + 30 * r * b2 * i2 - 30 * r * a2 * i2 -
            20 * r2 * r * b * i - 60 * r2 * a * b * i + 20 * r * b2 * b * i - 60 * r * a2 * b * i +
            5 * r2 * r2 * a - 10 * r2 * r * b2 + 10 * r2 * r * a2 - 30 * r2 * a * b2 + 10 * r2 * a2 * a +
            5 * r * b2 * b2 - 30 * r * a2 * b2 + 5 * r * a2 * a2 + 5 * a * i2 * i2 + 20 * a * b * i2 * i +
            30 * a * b2 * i2 - 10 * a2 * a * i2 + 20 * a * b2 * b * i - 20 * a2 * a * b * i +
            5 * a * b2 * b2 - 10 * a2 * a * b2 + a2 * a2 * a;
        dnr = diff_abs(c, d);
        dnr += a0;
        dni = 20 * i * r2 * r * a + 30 * i * r2 * a2 + 20 * i * r * a2 * a + 5 * i * a2 * a2 -
            30 * i2 * r2 * b - 30 * i * r2 * b2 - 20 * i2 * i * r * a - 60 * i2 * r * a * b -
            60 * i * r * a * b2 - 10 * i2 * i * a2 - 30 * i2 * a2 * b - 30 * i * a2 * b2 + 5 * i2 * i2 * b +
            10 * i2 * i * b2 + 10 * i2 * b2 * b + 5 * i * b2 * b2 + 5 * b * r2 * r2 + 20 * b * r2 * r * a +
            30 * b * r2 * a2 + 20 * b * r * a2 * a + 5 * b * a2 * a2 - 10 * b2 * b * r2 -
            20 * b2 * b * r * a - 10 * b2 * b * a2 + b2 * b2 * b + b0;
        delta_sub_n->imag(dni);
        delta_sub_n->real(dnr);
        break;

    case 10: // Mandelbar (Tricorn)
        dnr = 2 * r * a + a2 - b2 - 2 * b * i + a0;
        dni = b0 - (r * b + a * i + a * b) * 2;

        delta_sub_n->imag(dni);
        delta_sub_n->real(dnr);
        break;

    case 11: // Mandelbar (power)
    {
        std::complex<double> zp(1.0, 0.0);
        std::complex<double> sum(0.0, 0.0);
        for (int j = 0; j < m_power; j++)
        {
            sum += zp * (double) m_pascal_triangle[j];
            sum *= *delta_sub_n;
            zp *= *x_ref;
        }
        delta_sub_n->real(sum.real());
        delta_sub_n->imag(-sum.imag());
        *delta_sub_n += *delta_sub_0;
    }
    break;

    default:
        dnr = (2 * r + a) * a - (2 * i + b) * b + a0;
        dni = 2 * ((r + a) * b + i * a) + b0;
        delta_sub_n->imag(dni);
        delta_sub_n->real(dnr);
        break;
    }
}

// Cube c + jd = (a + jb) * (a + jb) * (a + jb)
static void cube_bf(BFComplex &out, const BFComplex &in)
{
    const int saved = save_stack();
    bf_t t = alloc_stack(g_r_bf_length + 2);
    bf_t t1 = alloc_stack(g_r_bf_length + 2);
    bf_t t2 = alloc_stack(g_r_bf_length + 2);
    bf_t sqr_real = alloc_stack(g_r_bf_length + 2);
    bf_t sqr_imag = alloc_stack(g_r_bf_length + 2);

    mult_bf(sqr_real, in.x, in.x); // sqr_real = x * x;
    mult_bf(sqr_imag, in.y, in.y); // sqr_imag = y * y;
    inttobf(t, 3);
    mult_bf(t1, t, sqr_imag); // sqr_real + sqr_real + sqr_real
    sub_bf(t2, sqr_real, t1); // sqr_real - (sqr_imag + sqr_imag + sqr_imag)
    mult_bf(out.x, in.x, t2); // c = x * (sqr_real - (sqr_imag + sqr_imag + sqr_imag))

    mult_bf(t1, t, sqr_real); // sqr_imag + sqr_imag + sqr_imag
    sub_bf(t2, t1, sqr_imag); // (sqr_real + sqr_real + sqr_real) - sqr_imag
    mult_bf(out.y, in.y, t2); // d = y * ((sqr_real + sqr_real + sqr_real) - sqr_imag)
    restore_stack(saved);
}

// Evaluate a complex polynomial
static void power_bf(BFComplex &result, const BFComplex &z, int degree)
{
    const int saved = save_stack();
    bf_t t = alloc_stack(g_r_bf_length + 2);
    bf_t t1 = alloc_stack(g_r_bf_length + 2);
    bf_t t2 = alloc_stack(g_r_bf_length + 2);
    bf_t t3 = alloc_stack(g_r_bf_length + 2);
    bf_t t4 = alloc_stack(g_r_bf_length + 2);

    if (degree < 0)
    {
        degree = 0;
    }

    copy_bf(t1, z.x); // BigTemp1 = xt
    copy_bf(t2, z.y); // BigTemp2 = yt

    if (degree & 1)
    {
        copy_bf(result.x, t1); // new.x = result real
        copy_bf(result.y, t2); // new.y = result imag
    }
    else
    {
        inttobf(result.x, 1);
        inttobf(result.y, 0);
    }

    degree >>= 1;
    while (degree)
    {
        sub_bf(t, t1, t2);  // (xt - yt)
        add_bf(t3, t1, t2); // (xt + yt)
        mult_bf(t4, t, t3); // t2 = (xt + yt) * (xt - yt)
        copy_bf(t, t2);
        mult_bf(t3, t, t1); // yt = xt * yt
        add_bf(t2, t3, t3); // yt = yt + yt
        copy_bf(t1, t4);

        if (degree & 1)
        {
            mult_bf(t, t1, result.x);  // xt * result->x
            mult_bf(t3, t2, result.y); // yt * result->y
            sub_bf(t4, t, t3);         // t2 = xt * result->x - yt * result->y
            mult_bf(t, t1, result.y);  // xt * result->y
            mult_bf(t3, t2, result.x); // yt * result->x
            add_bf(result.y, t, t3);   // result->y = result->y * xt + yt * result->x
            copy_bf(result.x, t4);     // result->x = t2
        }
        degree >>= 1;
    }
    restore_stack(saved);
}

// Reference Zoom Point Functions
//
void PertEngine::ref_functions_bf(const BFComplex &center, BFComplex *Z, BFComplex *ZTimes2)
{
    BFComplex temp_cmplx_cbf;
    BFComplex temp_cmplx1_cbf;

    const int cplxsaved = save_stack();

    bf_t temp_real_bf = alloc_stack(g_r_bf_length + 2);
    bf_t temp_imag_bf = alloc_stack(g_r_bf_length + 2);
    bf_t sqr_real_bf = alloc_stack(g_r_bf_length + 2);
    bf_t sqr_imag_bf = alloc_stack(g_r_bf_length + 2);
    bf_t real_imag_bf = alloc_stack(g_r_bf_length + 2);
    temp_cmplx_cbf.x = alloc_stack(g_r_bf_length + 2);
    temp_cmplx_cbf.y = alloc_stack(g_r_bf_length + 2);
    temp_cmplx1_cbf.x = alloc_stack(g_r_bf_length + 2);
    temp_cmplx1_cbf.y = alloc_stack(g_r_bf_length + 2);

    switch (m_subtype)
    {
    case 0: // optimise for Mandelbrot by taking out as many steps as possible
        //	    Z = Z.CSqr() + centre;
        square_bf(sqr_real_bf, Z->x);
        square_bf(sqr_imag_bf, Z->y);
        sub_bf(temp_real_bf, sqr_real_bf, sqr_imag_bf);
        add_bf(Z->x, temp_real_bf, center.x);
        mult_bf(real_imag_bf, ZTimes2->x, Z->y);
        add_bf(Z->y, real_imag_bf, center.y);
        break;

    case 1:
        if (m_power == 3)
        {
            cube_bf(temp_cmplx_cbf, *Z);
            add_bf(Z->x, temp_cmplx_cbf.x, center.x);
            add_bf(Z->y, temp_cmplx_cbf.y, center.y);
        }
        else
        {
            copy_bf(temp_cmplx_cbf.x, Z->x);
            copy_bf(temp_cmplx_cbf.y, Z->y);
            for (int k = 0; k < m_power - 1; k++)
                cplxmul_bf(&temp_cmplx_cbf, &temp_cmplx_cbf, Z);
            add_bf(Z->x, temp_cmplx_cbf.x, center.x);
            add_bf(Z->y, temp_cmplx_cbf.y, center.y);
        }
        break;

    case 2: // Burning Ship
        square_bf(sqr_real_bf, Z->x);
        square_bf(sqr_imag_bf, Z->y);
        sub_bf(temp_real_bf, sqr_real_bf, sqr_imag_bf);
        add_bf(Z->x, temp_real_bf, center.x);
        mult_bf(temp_imag_bf, ZTimes2->x, Z->y);
        abs_bf(real_imag_bf, temp_imag_bf);
        add_bf(Z->y, real_imag_bf, center.y);
        break;

    case 3: // Cubic Burning Ship
    case 4: // 4th Power Burning Ship
    case 5: // 5th Power Burning Ship
        abs_bf(temp_cmplx_cbf.x, Z->x);
        abs_bf(temp_cmplx_cbf.y, Z->y);
        power_bf(temp_cmplx1_cbf, temp_cmplx_cbf, m_power);
        add_bf(Z->x, temp_cmplx1_cbf.x, center.x);
        add_bf(Z->y, temp_cmplx1_cbf.y, center.y);
        break;

    case 6: // Celtic
        square_bf(sqr_real_bf, Z->x);
        square_bf(sqr_imag_bf, Z->y);
        mult_bf(real_imag_bf, ZTimes2->x, Z->y);
        add_bf(Z->y, real_imag_bf, center.y);
        sub_bf(temp_real_bf, sqr_real_bf, sqr_imag_bf);
        abs_bf(temp_imag_bf, temp_real_bf);
        add_bf(Z->x, temp_imag_bf, center.x);
        break;

    case 7: // Cubic Celtic
        power_bf(temp_cmplx_cbf, *Z, 3);
        abs_bf(temp_real_bf, temp_cmplx_cbf.x);
        add_bf(Z->x, temp_real_bf, center.x);
        add_bf(Z->y, center.y, temp_cmplx_cbf.y);
        break;

    case 8: // 4th Celtic Buffalo
        power_bf(temp_cmplx_cbf, *Z, 4);
        abs_bf(temp_real_bf, temp_cmplx_cbf.x);
        add_bf(Z->x, temp_real_bf, center.x);
        add_bf(Z->y, center.y, temp_cmplx_cbf.y);
        break;

    case 9: // 5th Celtic
        power_bf(temp_cmplx_cbf, *Z, 5);
        abs_bf(temp_real_bf, temp_cmplx_cbf.x);
        add_bf(Z->x, temp_real_bf, center.x);
        add_bf(Z->y, center.y, temp_cmplx_cbf.y);
        break;

    case 10: // Mandelbar (Tricorn)
        square_bf(sqr_real_bf, Z->x);
        square_bf(sqr_imag_bf, Z->y);
        mult_bf(real_imag_bf, ZTimes2->x, Z->y);
        sub_bf(temp_real_bf, sqr_real_bf, sqr_imag_bf);
        add_bf(Z->x, temp_real_bf, center.x);
        sub_bf(Z->y, center.y, real_imag_bf);
        break;

    case 11: // Mandelbar (power)
        power_bf(temp_cmplx_cbf, *Z, m_power);
        sub_bf(Z->y, center.y, temp_cmplx_cbf.y);
        add_bf(Z->x, temp_cmplx_cbf.x, center.x);
        break;

    default:
        //	    Z = Z.CSqr() + centre;
        square_bf(sqr_real_bf, Z->x);
        square_bf(sqr_imag_bf, Z->y);
        sub_bf(temp_real_bf, sqr_real_bf, sqr_imag_bf);
        add_bf(Z->x, temp_real_bf, center.x);
        mult_bf(real_imag_bf, ZTimes2->x, Z->y);
        add_bf(Z->y, real_imag_bf, center.y);
        break;
    }
    restore_stack(cplxsaved);
}

// Reference Zoom Point Functions
//
void PertEngine::ref_functions(
    const std::complex<double> &center, std::complex<double> *Z, std::complex<double> *z_times_2)
{
    double temp_real;
    double temp_imag;
    double sqr_real;
    double sqr_imag;
    double real_imag;
    std::complex<double> z;

    switch (m_subtype)
    {
    case 0: // optimise for Mandelbrot by taking out as many steps as possible
        //	    Z = Z.CSqr() + centre;
        sqr_real = sqr(Z->real());
        sqr_imag = sqr(Z->imag());
        temp_real = sqr_real - sqr_imag;
        Z->real(temp_real + center.real());
        real_imag = z_times_2->real() * Z->imag();
        Z->imag(real_imag + center.imag());
        break;

    case 1:
        if (m_power == 3)
        {
            *Z = cube(*Z) + center;
        }
        else
        {
            std::complex<double> ComplexTemp = *Z;
            for (int k = 0; k < m_power - 1; k++)
            {
                ComplexTemp *= *Z;
            }
            *Z = ComplexTemp + center;
        }
        break;

    case 2: // Burning Ship
        sqr_real = sqr(Z->real());
        sqr_imag = sqr(Z->imag());
        temp_real = sqr_real - sqr_imag;
        Z->real(temp_real + center.real());
        temp_imag = z_times_2->real() * Z->imag();
        real_imag = fabs(temp_imag);
        Z->imag(real_imag + center.imag());
        break;

    case 3: // Cubic Burning Ship
    case 4: // 4th Power Burning Ship
    case 5: // 5th Power Burning Ship
        z.real(fabs(Z->real()));
        z.imag(fabs(Z->imag()));
        z = power(z, m_power);
        *Z = z + center;
        break;

    case 6: // Celtic
        sqr_real = sqr(Z->real());
        sqr_imag = sqr(Z->imag());
        real_imag = z_times_2->real() * Z->imag();
        Z->imag(real_imag + center.imag());
        Z->real(fabs(sqr_real - sqr_imag) + center.real());
        break;

    case 7: // Cubic Celtic
        z = power(*Z, 3);
        Z->real(fabs(z.real()) + center.real());
        Z->imag(z.imag() + center.imag());
        break;

    case 8: // 4th Celtic Buffalo
        z = power(*Z, 4);
        Z->real(fabs(z.real()) + center.real());
        Z->imag(z.imag() + center.imag());
        break;

    case 9: // 5th Celtic
        z = power(*Z, 5);
        Z->real(fabs(z.real()) + center.real());
        Z->imag(z.imag() + center.imag());
        break;

    case 10: // Mandelbar (Tricorn)
        sqr_real = sqr(Z->real());
        sqr_imag = sqr(Z->imag());
        real_imag = Z->real() * z_times_2->imag();
        Z->real(sqr_real - sqr_imag + center.real());
        Z->imag(-real_imag + center.imag());
        break;

    case 11: // Mandelbar (power)
        z = power(*Z, m_power);
        Z->real(z.real() + center.real());
        Z->imag(-z.imag() + center.imag());
        break;
    }
}
