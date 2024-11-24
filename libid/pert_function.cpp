// SPDX-License-Identifier: GPL-3.0-only
//

#include "PertEngine.h"
#include "biginit.h"
#include "complex_fn.h"
#include "fractalb.h"
#include "sqr.h"

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
            sum += zp * (double) m_pascal_array[j];
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
            sum += zp * (double) m_pascal_array[j];
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

// Reference Zoom Point Functions
//
void PertEngine::ref_functions_bf(BFComplex *centre, BFComplex *Z, BFComplex *ZTimes2)
{
    bf_t temp_real_bf, temp_imag_bf, sqr_real_bf, sqr_imag_bf, real_imag_bf;
    BFComplex temp_cmplx_cbf, temp_cmplx1_cbf;
    int cplxsaved;

    cplxsaved = save_stack();

    temp_real_bf = alloc_stack(g_r_bf_length + 2);
    temp_imag_bf = alloc_stack(g_r_bf_length + 2);
    sqr_real_bf = alloc_stack(g_r_bf_length + 2);
    sqr_imag_bf = alloc_stack(g_r_bf_length + 2);
    real_imag_bf = alloc_stack(g_r_bf_length + 2);
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
        add_bf(Z->x, temp_real_bf, centre->x);
        mult_bf(real_imag_bf, ZTimes2->x, Z->y);
        add_bf(Z->y, real_imag_bf, centre->y);
        break;

    case 1:
        if (m_power == 3)
        {
            complex_cube_bf(&temp_cmplx_cbf, *Z);
            add_bf(Z->x, temp_cmplx_cbf.x, centre->x);
            add_bf(Z->y, temp_cmplx_cbf.y, centre->y);
        }
        else
        {
            copy_bf(temp_cmplx_cbf.x, Z->x);
            copy_bf(temp_cmplx_cbf.y, Z->y);
            for (int k = 0; k < m_power - 1; k++)
                cplxmul_bf(&temp_cmplx_cbf, &temp_cmplx_cbf, Z);
            add_bf(Z->x, temp_cmplx_cbf.x, centre->x);
            add_bf(Z->y, temp_cmplx_cbf.y, centre->y);
        }
        break;

    case 2: // Burning Ship
        square_bf(sqr_real_bf, Z->x);
        square_bf(sqr_imag_bf, Z->y);
        sub_bf(temp_real_bf, sqr_real_bf, sqr_imag_bf);
        add_bf(Z->x, temp_real_bf, centre->x);
        mult_bf(temp_imag_bf, ZTimes2->x, Z->y);
        abs_bf(real_imag_bf, temp_imag_bf);
        add_bf(Z->y, real_imag_bf, centre->y);
        break;

    case 3: // Cubic Burning Ship
    case 4: // 4th Power Burning Ship
    case 5: // 5th Power Burning Ship
        abs_bf(temp_cmplx_cbf.x, Z->x);
        abs_bf(temp_cmplx_cbf.y, Z->y);
        complex_polynomial_bf(&temp_cmplx1_cbf, temp_cmplx_cbf, m_power);
        add_bf(Z->x, temp_cmplx1_cbf.x, centre->x);
        add_bf(Z->y, temp_cmplx1_cbf.y, centre->y);
        break;

    case 6: // Celtic
        square_bf(sqr_real_bf, Z->x);
        square_bf(sqr_imag_bf, Z->y);
        mult_bf(real_imag_bf, ZTimes2->x, Z->y);
        add_bf(Z->y, real_imag_bf, centre->y);
        sub_bf(temp_real_bf, sqr_real_bf, sqr_imag_bf);
        abs_bf(temp_imag_bf, temp_real_bf);
        add_bf(Z->x, temp_imag_bf, centre->x);
        break;

    case 7: // Cubic Celtic
        complex_polynomial_bf(&temp_cmplx_cbf, *Z, 3);
        abs_bf(temp_real_bf, temp_cmplx_cbf.x);
        add_bf(Z->x, temp_real_bf, centre->x);
        add_bf(Z->y, centre->y, temp_cmplx_cbf.y);
        break;

    case 8: // 4th Celtic Buffalo
        complex_polynomial_bf(&temp_cmplx_cbf, *Z, 4);
        abs_bf(temp_real_bf, temp_cmplx_cbf.x);
        add_bf(Z->x, temp_real_bf, centre->x);
        add_bf(Z->y, centre->y, temp_cmplx_cbf.y);
        break;

    case 9: // 5th Celtic
        complex_polynomial_bf(&temp_cmplx_cbf, *Z, 5);
        abs_bf(temp_real_bf, temp_cmplx_cbf.x);
        add_bf(Z->x, temp_real_bf, centre->x);
        add_bf(Z->y, centre->y, temp_cmplx_cbf.y);
        break;

    case 10: // Mandelbar (Tricorn)
        square_bf(sqr_real_bf, Z->x);
        square_bf(sqr_imag_bf, Z->y);
        mult_bf(real_imag_bf, ZTimes2->x, Z->y);
        sub_bf(temp_real_bf, sqr_real_bf, sqr_imag_bf);
        add_bf(Z->x, temp_real_bf, centre->x);
        sub_bf(Z->y, centre->y, real_imag_bf);
        break;

    case 11: // Mandelbar (power)
        complex_polynomial_bf(&temp_cmplx_cbf, *Z, m_power);
        sub_bf(Z->y, centre->y, temp_cmplx_cbf.y);
        add_bf(Z->x, temp_cmplx_cbf.x, centre->x);
        break;

    default:
        //	    Z = Z.CSqr() + centre;
        square_bf(sqr_real_bf, Z->x);
        square_bf(sqr_imag_bf, Z->y);
        sub_bf(temp_real_bf, sqr_real_bf, sqr_imag_bf);
        add_bf(Z->x, temp_real_bf, centre->x);
        mult_bf(real_imag_bf, ZTimes2->x, Z->y);
        add_bf(Z->y, real_imag_bf, centre->y);
        break;
    }
    restore_stack(cplxsaved);
}

// Reference Zoom Point Functions
//
void PertEngine::ref_functions(
    std::complex<double> *centre, std::complex<double> *Z, std::complex<double> *z_times_2)
{
    double temp_real, temp_imag, sqr_real, sqr_imag, real_imag;
    std::complex<double> z;

    switch (m_subtype)
    {
    case 0: // optimise for Mandelbrot by taking out as many steps as possible
        //	    Z = Z.CSqr() + centre;
        sqr_real = sqr(Z->real());
        sqr_imag = sqr(Z->imag());
        temp_real = sqr_real - sqr_imag;
        Z->real(temp_real + centre->real());
        real_imag = z_times_2->real() * Z->imag();
        Z->imag(real_imag + centre->imag());
        break;

    case 1:
        if (m_power == 3)
            *Z = cube(*Z) +
                *centre; // optimise for Cubic by taking out as many multiplies as possible
        else
        {
            std::complex<double> ComplexTemp = *Z;
            for (int k = 0; k < m_power - 1; k++)
                ComplexTemp *= *Z;
            *Z = ComplexTemp + *centre;
        }
        break;

    case 2: // Burning Ship
        sqr_real = sqr(Z->real());
        sqr_imag = sqr(Z->imag());
        temp_real = sqr_real - sqr_imag;
        Z->real(temp_real + centre->real());
        temp_imag = z_times_2->real() * Z->imag();
        real_imag = fabs(temp_imag);
        Z->imag(real_imag + centre->imag());
        break;

    case 3: // Cubic Burning Ship
    case 4: // 4th Power Burning Ship
    case 5: // 5th Power Burning Ship
        z.real(fabs(Z->real()));
        z.imag(fabs(Z->imag()));
        z = power(z, m_power);
        *Z = z + *centre;
        break;

    case 6: // Celtic
        sqr_real = sqr(Z->real());
        sqr_imag = sqr(Z->imag());
        real_imag = z_times_2->real() * Z->imag();
        Z->imag(real_imag + centre->imag());
        Z->real(fabs(sqr_real - sqr_imag) + centre->real());
        break;

    case 7: // Cubic Celtic
        z = power(*Z, 3);
        Z->real(fabs(z.real()) + centre->real());
        Z->imag(z.imag() + centre->imag());
        break;

    case 8: // 4th Celtic Buffalo
        z = power(*Z, 4);
        Z->real(fabs(z.real()) + centre->real());
        Z->imag(z.imag() + centre->imag());
        break;

    case 9: // 5th Celtic
        z = power(*Z, 5);
        Z->real(fabs(z.real()) + centre->real());
        Z->imag(z.imag() + centre->imag());
        break;

    case 10: // Mandelbar (Tricorn)
        sqr_real = sqr(Z->real());
        sqr_imag = sqr(Z->imag());
        real_imag = Z->real() * z_times_2->imag();
        Z->real(sqr_real - sqr_imag + centre->real());
        Z->imag(-real_imag + centre->imag());
        break;

    case 11: // Mandelbar (power)
        z = power(*Z, m_power);
        Z->real(z.real() + centre->real());
        Z->imag(-z.imag() + centre->imag());
        break;
    }
}

// Generate Pascal's Triangle coefficients
void PertEngine::load_pascal(long pascal_array[], int n)
{
    long j, c = 1L;

    for (j = 0; j <= n; j++)
    {
        if (j == 0)
            c = 1;
        else
            c = c * (n - j + 1) / j;
        pascal_array[j] = c;
    }
}

// Laser Blaster's Code for removing absolutes from Mandelbrot derivatives
double PertEngine::diff_abs(const double c, const double d)
{
    const double cd{c + d};

    if (c >= 0.0)
    {
        return cd >= 0.0 ? d : -d - 2.0 * c;
    }
    return cd > 0.0 ? d + 2.0 * c : -d;
}

// Evaluate a Complex Polynomial
void PertEngine::complex_polynomial_bf(BFComplex *out, BFComplex in, int degree)
{
    bf_t t, t1, t2, t3, t4;
    int cplxsaved;

    cplxsaved = save_stack();
    t = alloc_stack(g_r_bf_length + 2);
    t1 = alloc_stack(g_r_bf_length + 2);
    t2 = alloc_stack(g_r_bf_length + 2);
    t3 = alloc_stack(g_r_bf_length + 2);
    t4 = alloc_stack(g_r_bf_length + 2);

    if (degree < 0)
        degree = 0;

    copy_bf(t1, in.x); // BigTemp1 = xt
    copy_bf(t2, in.y); // BigTemp2 = yt

    if (degree & 1)
    {
        copy_bf(out->x, t1); // new.x = result real
        copy_bf(out->y, t2); // new.y = result imag
    }
    else
    {
        inttobf(out->x, 1);
        inttobf(out->y, 0);
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
            mult_bf(t, t1, out->x);  // xt * result->x
            mult_bf(t3, t2, out->y); // yt * result->y
            sub_bf(t4, t, t3);       // t2 = xt * result->x - yt * result->y
            mult_bf(t, t1, out->y);  // xt * result->y
            mult_bf(t3, t2, out->x); // yt * result->x
            add_bf(out->y, t, t3);   // result->y = result->y * xt + yt * result->x
            copy_bf(out->x, t4);     // result->x = t2
        }
        degree >>= 1;
    }
    restore_stack(cplxsaved);
}

// Cube c + jd = (a + jb) * (a + jb) * (a + jb)
void PertEngine::complex_cube_bf(BFComplex *out, BFComplex in)
{
    bf_t t, t1, t2, sqr_real, sqr_imag;

    int cplxsaved;

    cplxsaved = save_stack();
    t = alloc_stack(g_r_bf_length + 2);
    t1 = alloc_stack(g_r_bf_length + 2);
    t2 = alloc_stack(g_r_bf_length + 2);
    sqr_real = alloc_stack(g_r_bf_length + 2);
    sqr_imag = alloc_stack(g_r_bf_length + 2);

    mult_bf(sqr_real, in.x, in.x); // sqr_real = x * x;
    mult_bf(sqr_imag, in.y, in.y); // sqr_imag = y * y;
    inttobf(t, 3);
    mult_bf(t1, t, sqr_imag);  // sqr_real + sqr_real + sqr_real
    sub_bf(t2, sqr_real, t1);  // sqr_real - (sqr_imag + sqr_imag + sqr_imag)
    mult_bf(out->x, in.x, t2); // c = x * (sqr_real - (sqr_imag + sqr_imag + sqr_imag))

    mult_bf(t1, t, sqr_real);  // sqr_imag + sqr_imag + sqr_imag
    sub_bf(t2, t1, sqr_imag);  // (sqr_real + sqr_real + sqr_real) - sqr_imag
    mult_bf(out->y, in.y, t2); // d = y * ((sqr_real + sqr_real + sqr_real) - sqr_imag)
    restore_stack(cplxsaved);
}
