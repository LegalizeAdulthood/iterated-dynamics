// SPDX-License-Identifier: GPL-3.0-only
//
//////////////////////////////////////////////////////////////////////
// COMPLEX_FN.cpp a module with complex functions
// Written in Microsoft Visual C++ by Paul de Leeuw.
//////////////////////////////////////////////////////////////////////

#include <sqr.h>
#include "complex_fn.h"

// the following functions are put in a different class so that can also be used in both perturbation and tierazon

double CComplexFn::sum_squared(std::complex<double> z)
    {
    return (sqr(z.real()) + sqr(z.imag()));
    }

/**************************************************************************
	Cube c + jd = (a + jb) * (a + jb) * (a + jb) 
***************************************************************************/

std::complex<double> CComplexFn::complex_cube(std::complex<double> z)

    {
    std::complex<double> temp;
    double	x, y, sqr_real, sqr_imag;

    x = z.real();
    y = z.imag();
    sqr_real = x * x;
    sqr_imag = y * y;
    temp.real(x * (sqr_real - (sqr_imag + sqr_imag + sqr_imag)));
    temp.imag(y * ((sqr_real + sqr_real + sqr_real) - sqr_imag));
    return  temp;
    }

/**************************************************************************
	Complex Power Function
**************************************************************************/

void CComplexFn::complex_power(std::complex<double> &result, std::complex<double> &base, int exp)

    {
    double	xt, yt, t2;

    xt = base.real();   yt = base.imag();

    if (exp & 1)
	    {
	    result.real(xt);
	    result.imag(yt);
	    }
    else
	    {
	    result.real(1.0);
	    result.imag(0.0);
	    }

    exp >>= 1;
    while (exp)
	    {
	    t2 = (xt + yt) * (xt - yt);
	    yt = xt * yt;
	    yt = yt + yt;
	    xt = t2;

	    if (exp & 1)
	        {
	        t2 = xt * result.real() - yt * result.imag();
	        result.imag(result.imag() * xt + yt * result.real());
	        result.real(t2);
	        }
	    exp >>= 1;
	    }
    }

/**************************************************************************
	Evaluate a Complex Polynomial
**************************************************************************/
                         
std::complex<double> CComplexFn::complex_polynomial(std::complex<double> z, int degree)

    {
    std::complex<double> temp, temp1;

    temp1.real(z.real());
    temp1.imag(z.imag());
    if (degree < 0)
	    degree = 0;
    complex_power(temp, temp1, degree);
    return  temp;
    }

/**************************************************************************
	Evaluate a Complex Inverse
**************************************************************************/
                         
std::complex<double> CComplexFn::complex_invert(std::complex<double> z)

    {
    std::complex<double> temp;
    double d, x, y;
    double zerotol = 1.e-50;

    x = z.real();
    y = z.imag();

    if (x == 0.0 && y == 0.0)
        d = zerotol; // prevent divide by zero
    else
        d = x * x + y * y;
    temp.real(x / d);
    temp.imag(-y / d);
    return temp;
    }

/**************************************************************************
	Evaluate a Complex Square
**************************************************************************/
                         
std::complex<double> CComplexFn::complex_square(std::complex<double> z)

    {
    std::complex<double> a;
    double  temp, x, y;

    x = z.real();
    y = z.imag();
    a.real((x * x) - (y * y));
    temp = x * y;
    a.imag(temp + temp);
    return a;
    }

