// SPDX-License-Identifier: GPL-3.0-only
//
// Complex.cpp: interface for the Complex class.
//
//////////////////////////////////////////////////////////////////////
#include "Complex.h"

//Complex::Complex(void)
//    {
//    }

Complex::~Complex(void)
    {
    }

Complex Complex::operator=(const double & value)	// Assignment to double Operator
    {
    x = value;
    y = 0.0;
    return *this;
    }

Complex Complex::operator=(const Complex & Cmplx1)	// Assignment Operator
    {
    x = Cmplx1.x;
    y = Cmplx1.y;
    return *this;
    }

// new stuff
Complex Complex::operator+=(const Complex & Cmplx1)
    {
    x+=Cmplx1.x;
    y+=Cmplx1.y;
    return *this;
    }

Complex Complex::operator+=(double & rvalue)
    {
    x+=rvalue;
    return *this;
    }

Complex Complex::operator-=(const Complex & Cmplx1)
    {
    x-=Cmplx1.x;
    y-=Cmplx1.y;
    return *this;
    }

Complex Complex::operator-=(double & rvalue)
    {
    x-=rvalue;
    return *this;
    }

Complex Complex::operator*=(const Complex & Cmplx1)
    {
    double  t;

    t = Cmplx1.x*x-Cmplx1.y*y; 
    y = Cmplx1.x*y+Cmplx1.y*x;
    x = t;
    return *this;
    }

Complex Complex::operator*=(double & rvalue)
    {
    x*=rvalue;
    y*=rvalue;
    return *this;
    }

bool Complex::operator==(Complex & rvalue)
    {
    return (x==rvalue.x && y==rvalue.y);
    }

Complex Complex::operator^(double & expon)
    {
    Complex	temp, temp1;

    temp.x = x;
    temp.y = y;
    temp1 = temp.CLog()*expon;
//    x = temp1.x;
//    y = temp1.y;
    temp = temp1.CExp();
//    temp.x = x;
//    temp.y = y;
//    temp = CExp(CLog(temp)*expon);
    return temp;			
    }

Complex Complex::operator^(Complex & expon)
    {
    Complex	temp, temp1;

    temp.x = x;
    temp.y = y;
    temp1 = temp.CLog()*expon;
    temp = temp1.CExp();
    //    temp.x = x;
    //    temp.y = y;
    //    temp = CExp(CLog(temp)*expon);
    return temp;
    }

Complex Complex::operator++(void)
    {
    x+=1;
    return *this;
    }
// end new stuff

Complex Complex::operator+(const Complex & Cmplx1)	// complex add Operator
    {
    Complex	temp;

    temp.x = x + Cmplx1.x;
    temp.y = y + Cmplx1.y;
    return  temp;
    }

Complex Complex::operator+(const double & Sum)	// complex add double Operator
    {
    Complex	temp;

    temp.x = x + Sum;
    temp.y = y;
    return  temp;
    }

Complex Complex::operator-(const Complex & Cmplx1)	// complex subtract Operator
    {
    Complex	temp;

    temp.x = x - Cmplx1.x;
    temp.y = y - Cmplx1.y;
    return  temp;
    }

Complex Complex::operator-(const double & Difference)	// complex subtract double Operator
    {
    Complex	temp;

    temp.x = x - Difference;
    temp.y = y;
    return  temp;
    }

Complex Complex::operator-(void)		    // unary minus
    {
    Complex	temp;

    temp.x = -x;
    temp.y = -y;
    return  temp;
    }

Complex Complex::operator*(const Complex & Cmplx1)	// complex multiply Operator
    {
    Complex	temp;

    temp.x = x * Cmplx1.x - y * Cmplx1.y;
    temp.y = y * Cmplx1.x + x * Cmplx1.y;
    return  temp;
    }

Complex Complex::operator*(const double & Multiplier)	// complex multiply by double Operator
    {
    Complex	temp;

    temp.x = x * Multiplier;
    temp.y = y * Multiplier;
    return  temp;
    }

Complex Complex::operator/(const Complex & Cmplx1)	// complex divide Operator
    {
    double	d;
    Complex	temp;

    if (Cmplx1.x == 0.0 && Cmplx1.y == 0.0)
	d = zerotol;					// prevent divide by zero
    else
	d = Cmplx1.x * Cmplx1.x + Cmplx1.y * Cmplx1.y;
    temp.x = (x * Cmplx1.x + y * Cmplx1.y) / d;
    temp.y = (y * Cmplx1.x - x * Cmplx1.y) / d;
    return  temp;
    }

Complex Complex::operator/(const double & divisor)	// complex divide by double Operator
    {
    Complex	temp;
    double  d;

    if (divisor == 0.0)
	d = zerotol;					// prevent divide by zero
    else
	d = divisor;
    temp.x = x / d;
    temp.y = y / d;
    return  temp;
    }

/**************************************************************************
	sin(x+iy)  = sin(x)cosh(y) + icos(x)sinh(y)
***************************************************************************/

Complex	Complex::CSin()
    {
    Complex a;

    a.x = sin(x) * cosh(y);
    a.y = cos(x) * sinh(y);
    return a;
    }

/**************************************************************************
	cos(x+iy)  = cos(x)cosh(y) - isin(x)sinh(y)
***************************************************************************/

Complex	Complex::CCos()
    {
    Complex a;

    a.x = cos(x) * cosh(y);
    a.y = -sin(x) * sinh(y);
    return a;
    }

/**************************************************************************
	tan(x+iy)  = (tan(x) -i tanh(-y)) / (1 +i tanh(-y) tan(x))
***************************************************************************/

Complex	Complex::CTan(void)
    {
    Complex a;
    double  denom;

    x *= 2;
    y *= 2;
    denom = cos(x) + cosh(y);
    if (fabs(denom) <= DBL_MIN) 
	a = DBL_MAX;
    else
	{
	a.x = sin(x) / denom;
	a.y = sinh(y) / denom;
	}
    return a;
    }

/**************************************************************************
	sinh(x+iy)  = sinh(x)cos(y) + icosh(x)sin(y)
***************************************************************************/

Complex	Complex::CSinh(void)
    {
    Complex a;

    a.x = sinh(x) * cos(y);
    a.y = cosh(x) * sin(y);
    return a;
    }

/**************************************************************************
	cosh(x+iy)  = cosh(x)cos(y) - isinh(x)sin(y)
***************************************************************************/

Complex	Complex::CCosh(void)
    {
    Complex a;

    a.x = cosh(x) * cos(y);
    a.y = -sinh(x) * sin(y);
    return a;
    }

/**************************************************************************
	tanh(x+iy)  = (tanh(x) + i tan(y)) / (1 +i tanh(x) tan(y))
***************************************************************************/

Complex	Complex::CTanh(void)
    {
    Complex a;
    double  denom;

    x *= 2;
    y *= 2;

    denom = cosh(x) + cos(y);
    if (fabs(denom) <= DBL_MIN) 
	a = DBL_MAX;
    else
	{
	a.x = sinh(x) / denom;
	a.y = sin(y) / denom;
	}
    return a;
    }

/**************************************************************************
	Complex Exponent: e^(x+iy) = (e^x) * cos(y) + i * (e^x) * sin(y)
***************************************************************************/

Complex	Complex::CExp()
    {
    Complex a;

    a.x = exp(x) * cos(y);
    a.y = exp(x) * sin(y);
    return a;
    }

/**************************************************************************
	sqr(x+iy)  = x*x-y*y + i(x*y)*2
***************************************************************************/

Complex	Complex::CSqr()
    {
    Complex a;
    double  temp;

    a.x = (x * x) - (y * y);
    temp = x * y;
    a.y = temp + temp;
    return a;
    }

/**************************************************************************
	sqrt(x+iy)  =  

DeMoivre’s Theorem states that if n is any positive real number, then
(a + bi)^n = r^n(cos nθ + i sin nθ).

In particular, if n = ½, we have
√a + bi = √r [cos θ/2 + i sin θ/2]

If we apply the half-angle formula:
cos (θ/2) = ± √[(1 + cos θ) / 2]
and
sin (θ/2) = ± √[(1 - cos θ) / 2]

This gives us a straightforward way to calculate √(a + bi)

√(a + bi) = √r [√[(1 + cos θ) / 2] ± i√[(1 - cos θ) / 2]]
√(a + bi) = √r [√[(1 + a/r)/ 2] ± i√[(1 - a/r) / 2]]

Simplifying you get:
√(a + bi) = √[(r + a) / 2] ± i √[(r - a) / 2]
***************************************************************************/

Complex	Complex::CSqrt(void)
    {
    Complex a;

    a.x = sqrt(sqrt(x*x+y*y)) * cos(atan2(y,x)/2);
    a.y = sqrt(sqrt(x*x+y*y)) * sin(atan2(y,x)/2);
    return a;
    }

/**************************************************************************
	sumsqr(x+iy)  = x*x+y*y
***************************************************************************/

double	Complex::CSumSqr(void)
    {
    return (x * x) + (y * y);
    }

/**************************************************************************
	Abs(x+iy)  = sqrt(x*x+y*y)
***************************************************************************/

double	Complex::CFabs(void)
    {
    Complex temp;

    temp = (Complex(x, y));
    return sqrt(temp.CSumSqr());
    }

/**************************************************************************
	CFlip(x+iy)  = (y+ix) i.e. swap real and imaginary
***************************************************************************/

Complex	Complex::CFlip(void)
    {
    return Complex(y, x); 
    }

/**************************************************************************
	Complex Power Function
**************************************************************************/

void	Complex::CPower(Complex  & result, Complex  & base, int exp)

    {
    double	xt, yt, t2;

    xt = base.x;   yt = base.y;

    if (exp & 1)
	{
	result.x = xt;
	result.y = yt;
	}
    else
	{
	result.x = 1.0;
	result.y = 0.0;
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
	    t2 = xt * result.x - yt * result.y;
	    result.y = result.y * xt + yt * result.x;
	    result.x = t2;
	    }
	exp >>= 1;
	}
    }

/**************************************************************************
	Evaluate a Complex Polynomial
**************************************************************************/
                         
Complex	Complex::CPolynomial(int degree)

    {
    Complex	temp, temp1;

    temp1.x = x;
    temp1.y = y;
    if (degree < 0)
	degree = 0;
    CPower(temp, temp1, degree);
    return  temp;
    }

/**************************************************************************
	invert c + jd = (1 + j0) / (a + jb)
***************************************************************************/

Complex	Complex::CInvert(void)

    {
    double	d;
    Complex	temp;

    if (x == 0.0 && y == 0.0)
	d = zerotol;						// prevent divide by zero
    else
	d = x * x + y * y;
    temp.x = x / d;
    temp.y = - y / d;
    return  temp;
    }

/**************************************************************************
	Cube c + jd = (a + jb) * (a + jb) * (a + jb) 
***************************************************************************/

Complex	Complex::CCube()

    {
    Complex	temp;
    double	sqr_real, sqr_imag;

    sqr_real = x * x;
    sqr_imag = y * y;
    temp.x = x * (sqr_real - (sqr_imag + sqr_imag + sqr_imag));
    temp.y = y * ((sqr_real + sqr_real + sqr_real) - sqr_imag);
    return  temp;
    }

/**************************************************************************
	Evaluate a Complex Log: ln(x+i y) = 0.5*ln(x²+y²) + i atan(x/y) 
**************************************************************************/
                         
Complex	Complex::CLog()

    {
    Complex a, t;
    double  mod,zx,zy;

    t.x = x;
    t.y = y;
    mod = t.CSumSqr();
    mod = sqrt(mod);
    zx = log(mod);
    zy = atan2(y,x);

    a.x = zx;
    a.y = zy;
    return a;
    }

/**************************************************************************
	Evaluate a Complex Polynomial with a complex power
**************************************************************************/

// now replaced with operator z^x where z and x are both complex.

//void SinCos(double *Angle, double *Sin, double *Cos)
//    {
//    *Sin = sin(*Angle);
//    *Cos = cos(*Angle);
//    }

//Complex	CComplexPower(Complex  & Cmplx1, Complex & power)
//
//    {
//    static  Complex z;
//	    Complex t;
//	    double  e2x, siny, cosy;
//	    double  e2x;

//    t = 0;
//    t = power*CLog(Cmplx1);
//    e2x = exp(t.x);
//    SinCos(&t.y, &siny, &cosy);
//    z.x = e2x * cos(t.y);
//    z.y = e2x * sin(t.y);
//    return(z);
//    }
