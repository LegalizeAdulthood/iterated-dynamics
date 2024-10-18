// SPDX-License-Identifier: GPL-3.0-only
//
//////////////////////////////////////////////////////////////////////
// PertFN.cpp a module with Perturbation functions and reference functions
// Written in Microsoft Visual C++ by Paul de Leeuw.
//////////////////////////////////////////////////////////////////////

#include <sqr.h>
#include "pert_engine.h"
//#include "BigComplex.h"

    //////////////////////////////////////////////////////////////////////
// Individual function point calculations
//////////////////////////////////////////////////////////////////////

void CPertEngine::PertFunctions(Complex *XRef, Complex *DeltaSubN, Complex *DeltaSub0)

    {
    double  Dnr, Dni;
    double r = XRef->x;
    double i = XRef->y;
    double a = DeltaSubN->x;
    double b = DeltaSubN->y;
    double a2 = a * a;
    double b2 = b * b;
    double a0 = DeltaSub0->x;
    double b0 = DeltaSub0->y;
    double ar = a * r;
    double ib = i * b;
    double ab;
    double x = r;
    double y = i;
    double x2 = x * x;
    double y2 = y * y;;
    double r2 = r * r;
    double i2 = i * i;
    double c, d;
    double Multiplier;
    Complex Dn, z;

    switch (subtype)
	    {
	    case 0:								// Mandelbrot
            if (power == 3)                                             // Cubic
                {
                Dnr = 3 * r * r * a - 6 * r * i * b - 3 * i * i * a + 3 * r * a * a - 3 * r * b * b - 3 * i * 2 * a * b + a * a * a - 3 * a * b * b + a0;
                Dni = 3 * r * r * b + 6 * r * i * a - 3 * i * i * b + 3 * r * 2 * a * b + 3 * i * a * a - 3 * i * b * b + 3 * a * a * b - b * b * b + b0;
                DeltaSubN->y = Dni;
                DeltaSubN->x = Dnr;
                }
            else
                {
                Dnr = (2 * r + a) * a - (2 * i + b) * b + a0;
                Dni = 2 * ((r + a) * b + i * a) + b0;
                DeltaSubN->y = Dni;
                DeltaSubN->x = Dnr;
                }

	        //	    else if (power == 3)
	        //		DeltaSubN = (XSubN[iteration] * XSubN[iteration] * DeltaSubN) * 3.0 + (XSubN[iteration] * DeltaSubN * DeltaSubN) * 3.0 + DeltaSubN * DeltaSubN * DeltaSubN;
	        break;
	    case 1:								// Power
	        {
	        Complex Zp(1.0, 0.0);
	        Complex sum(0.0, 0.0);
	        for (int i = 0; i < power; i++)
		        {
		        sum += Zp * (double)PascalArray[i];
		        sum *= *DeltaSubN;
		        Zp *= *XRef;
		        }
	        *DeltaSubN = sum;
	        *DeltaSubN += *DeltaSub0;
	        }
	        break;
	    case 2:								// Burning Ship 
	        DeltaSubN->x = 2.0 * a * r + a2 - 2.0 * b * i - b2;
	        DeltaSubN->y = DiffAbs(r * i, r * b + i * a + a * b) * 2;
	        //	    DeltaSubN.y = 2.0 * (fabs(XSubN[iteration].x * (XSubN[iteration].y + DeltaSubN.y) + DeltaSubN.x * (XSubN[iteration].y + DeltaSubN.y)) - fabs(XSubN[iteration].x * XSubN[iteration].y)); // pixelation at 1.0E-18
	        *DeltaSubN += *DeltaSub0;
	        break;
	    case 3:								// Cubic Burning Ship 
	        {
	        Dnr = DiffAbs(r, a);
	        ab = r + a;
	        Dnr = (r*r - 3 * i*i) * Dnr + (2 * a*r + a2 - 6 * i*b - 3 * b2)*fabs(ab) + a0;
	        Dni = DiffAbs(i, b);
	        ab = i + b;
	        Dni = (3 * r*r - i * i) * Dni + (6 * r*a + 3 * a2 - 2 * i*b - b2) * fabs(ab) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        }
	        break;
	    case 4:								// 4th Power Burning Ship 
	        Dnr = 4 * r2*r*a + 6 * r2*a2 + 4 * r*a2*a + a2 * a2 + 4 * i2*i*b + 6 * i2*b2 + 4 * i*b2*b + b2 * b2 - 12 * r2*i*b - 6 * r2*b2 - 12 * r*a*i2 - 24 * r*a*i*b - 12 * r*a*b2 - 6 * a2*i2 - 12 * a2*i*b - 6 * a2*b2 + a0;
	        Dni = DiffAbs(r * i, r * b + a * i + a * b);
	        Dni = 4 * (r2 - i2)*(Dni)+4 * fabs(r*i + r * b + a * i + a * b)*(2 * a*r + a2 - 2 * b*i - b2) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 5:								// 5th Power Burning Ship 
	        Dnr = DiffAbs(r, a);
	        Dnr = (Dnr) * (r*r*r*r - 10 * r*r*i*i + 5 * i*i*i*i) + fabs(r + a) * (4 * r*r*r*a + 6 * r*r*a2 + 4 * r*a2*a + a2 * a2 - 20 * r2*i*b - 10 * r2*b2 - 20 * r*a*i2 - 40 * r*a*i*b - 20 * r*a*b2 - 10 * a2*i2 - 20 * a2*i*b - 10 * a2*b2
		    + 20 * i2*i*b + 30 * i2*b2 + 20 * i*b2*b + 5 * b2*b2) + a0;
	        Dni = DiffAbs(i, b);
	        Dni = (Dni) * (5 * r2*r2 - 10 * r2*i2 + i2 * i2) + fabs(i + b)*(20 * r2*r*a + 30 * r2*a2 + 20 * r*a2*a + 5 * a2*a2 - 20 * r2*i*b - 10 * r2*b2 - 20 * r*a*i2 - 40 * r*a*i*b - 20 * r*a*b2 - 10 * a2*i2 - 20 * a2*i*b - 10 * a2*b2
		    + 4 * i2*i*b + 6 * i2*b2 + 4 * i*b2*b + b2 * b2) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 6:								// Celtic 
	        Dnr = DiffAbs(r2 - i2, (2 * r + a)*a - (2 * i + b)*b);
	        Dnr += a0;
	        Dni = 2 * r*b + 2 * a*(i + b) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 7:								// Cubic Celtic
	        c = r*(r2 - 3 * i2);
	        d = a*(3 * r2 + a2) + 3 * r*(a2 - 2 * i*b - b2) - 3 * a*(i2 + 2 * i*b + b2);
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        Dni = 3 * i*(2 * r*a + a2 - b2) + 3 * b*(r2 + 2 * r*a + a2) - b*(b2 + 3 * i2) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 8:								// 4th Celtic Buffalo
	        c = r2*r2+i2*i2-6*r2*i2;
	        d = 4*r2*r*a+6*r2*a2+4*r*a2*a+a2*a2 + 4*i2*i*b+6*i2*b2+4*i*b2*b+b2*b2 -12*a*r*i2-6*a2*i2-12*b*r2*i-24*a*b*r*i-12*a2*b*i-6*b2*r2-12*a*b2*r-6*a2*b2;
	        Dnr = DiffAbs(c, d);
	        Dnr += a0;
	        Dni = 12*r2*i*a+12*r*i*a2-12*r*i2*b-12*r*i*b2+4*r2*r*b+12*r2*b*a+12*r*b*a2-4*r*b2*b+4*a2*a*i-4*a*i2*i-12*a*i2*b-12*a*i*b2+4*a2*a*b-4*a*b2*b + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 9:								// 5th Celtic
	        //Dnr = _abs((x+a) * ((x+a)*(x+a)*(x+a)*(x+a) - 10 * (x+a)*(x+a)*(y+b)*(y+b) + 5 * (y+b)*(y+b)*(y+b)*(y+b))) - _abs(x2*x2*x - 10*x2*x*y2 + 5*x*y2*y2) +a0;
	        //Dnr = _abs(5*x*y2*y2+20*x*b*y2*y-10*x2*x*y2-30*x2*a*y2+30*x*b2*y2-30*x*a2*y2-20*x2*x*b*y-60*x2*a*b*y+20*x*b2*b*y-60*x*a2*b*y+x2*x2*x+5*x2*x2*a-10*x2*x*b2+10*x2*x*a2-30*x2*a*b2
	        //	+10*x2*a2*a+5*x*b2*b2-30*x*a2*b2+5*x*a2*a2+5*a*y2*y2+20*a*b*y2*y+30*a*b2*y2-10*a2*a*y2+20*a*b2*b*y-20*a2*a*b*y+5*a*b2*b2-10*a2*a*b2+a2*a2*a) - _abs(x2*x2*x - 10*x2*x*y2 + 5*x*y2*y2) + a0;
	        c = r2*r2*r - 10*r2*r*i2 + 5*r*i2*i2;
	        d = 20*r*b*i2*i-30*r2*a*i2+30*r*b2*i2-30*r*a2*i2-20*r2*r*b*i-60*r2*a*b*i+20*r*b2*b*i-60*r*a2*b*i+5*r2*r2*a-10*r2*r*b2+10*r2*r*a2-30*r2*a*b2+10*r2*a2*a
			        +5*r*b2*b2-30*r*a2*b2+5*r*a2*a2+5*a*i2*i2+20*a*b*i2*i+30*a*b2*i2-10*a2*a*i2+20*a*b2*b*i-20*a2*a*b*i+5*a*b2*b2-10*a2*a*b2+a2*a2*a;
	        Dnr = DiffAbs(c, d);
	        Dnr += a0;

	        Dni = 20*i*r2*r*a+30*i*r2*a2+20*i*r*a2*a+5*i*a2*a2-30*i2*r2*b-30*i*r2*b2-20*i2*i*r*a-60*i2*r*a*b-60*i*r*a*b2-10*i2*i*a2-30*i2*a2*b-30*i*a2*b2+5*i2*i2*b
			        +10*i2*i*b2+10*i2*b2*b+5*i*b2*b2+5*b*r2*r2+20*b*r2*r*a+30*b*r2*a2+20*b*r*a2*a+5*b*a2*a2-10*b2*b*r2-20*b2*b*r*a-10*b2*b*a2+b2*b2*b +b0;
	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 10:							// Mandelbar (Tricorn) 
	        Dnr = 2 * r*a + a2 - b2 - 2 * b*i + a0;
	        Dni = b0 - (r*b + a * i + a * b) * 2;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 11:							// Mandelbar (power) 
	        {
	        Complex Zp(1.0, 0.0);
	        Complex sum(0.0, 0.0);
	        for (int i = 0; i < power; i++)
		        {
		        sum += Zp * (double)PascalArray[i];
		        sum *= *DeltaSubN;
		        Zp *= *XRef;
		        }
	        DeltaSubN->x = sum.x;
	        DeltaSubN->y = -sum.y;
	        *DeltaSubN += *DeltaSub0;
	        }
	        break;

	    case 12:							// Buffalo
	        Dnr = DiffAbs(r2 - i2, 2 * r*a + a2 - 2 * i*b - b2);
	        Dnr = Dnr + a0;
	        Dni = DiffAbs(r * i, r * b + a * i + a * b);
	        Dni = b0 - 2 * Dni;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 13:							// Cubic Buffalo
	        c = r * (r2 - 3 * i2);
	        d = a * (3 * r2 + a2) + 3 * r*(a2 - 2 * i*b - b2) - 3 * a*(i2 + 2 * i*b + b2);
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        c = i * (3 * r2 - i2);
	        d = 3 * i*(2 * r*a + a2 - b2) + 3 * b*(r2 + 2 * r*a + a2) - b * (3 * i2 + b2);
	        Dni = DiffAbs(c, d);
	        Dni = Dni + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 14:							// 4th power Buffalo
	        c = r2 * r2 + i2 * i2 - 6 * r2*i2;
	        d = 4 * r2*r*a + 6 * r2*a2 + 4 * r*a2*a + a2 * a2 + 4 * i2*i*b + 6 * i2*b2 + 4 * i*b2*b + b2 * b2 - 12 * r2*i*b - 6 * r2*b2 - 12 * r*a*i2 - 24 * r*a*i*b - 12 * r*a*b2 - 6 * a2*i2 - 12 * a2*i*b - 6 * a2*b2;
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        c = 4 * r2*r*i - 4 * r*i2*i;
	        d = -4 * a*i2*i - 12 * b*r*i2 - 12 * a*b*i2 + 12 * a*r2*i - 12 * b2*r*i + 12 * a2*r*i - 12 * a*b2*i + 4 * a2*a*i + 4 * b*r2*r + 12 * a*b*r2 - 4 * b2*b*r + 12 * a2*b*r - 4 * a*b2*b + 4 * a2*a*b;
	        Dni = DiffAbs(c, d);
	        Dni = Dni + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 15:							// 5th power Buffalo
	        c = 5 * r*i2*i2 - 10 * r2*r*i2 + r2 * r2*r;
	        d = 20 * r*b*i2*i - 30 * r2*a*i2 + 30 * r*b2*i2 - 30 * r*a2*i2 - 20 * r2*r*b*i - 60 * r2*a*b*i + 20 * r*b2*b*i - 60 * r*a2*b*i + 5 * r2*r2*a - 10 * r2*r*b2 + 10 * r2*r*a2 
				    - 30 * r2*a*b2 + 10 * r2*a2*a + 5 * r*b2*b2 - 30 * r*a2*b2 + 5 * r*a2*a2 + 5 * a*i2*i2 + 20 * a*b*i2*i + 30 * a*b2*i2 - 10 * a2*a*i2 + 20 * a*b2*b*i - 20 * a2*a*b*i + 5 * a*b2*b2 - 10 * a2*a*b2 + a2 * a2*a;
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        c = i2 * i2*i - 10 * i2*i*r2 + 5 * i*r2*r2;
	        d = 5 * i2*i2*b - 20 * i2*i*a*r + 10 * i2*i*b2 - 10 * i2*i*a2 - 30 * i2*b*r2 - 60 * i2*a*b*r + 10 * i2*b2*b - 30 * i2*a2*b + 20 * i*a*r2*r - 30 * i*b2*r2 + 30 * i*a2*r2 - 60 * i*a*b2*r + 20 * i*a2*a*r + 5 * i*b2*b2 
							    - 30 * i*a2*b2 + 5 * i*a2*a2 + 5 * b*r2*r2 + 20 * b*a*r2*r - 10 * b2*b*r2 + 30 * b*a2*r2 - 20 * b2*b*a*r + 20 * b*a2*a*r + b2 * b2*b - 10 * b2*b*a2 + 5 * b*a2*a2;
	        Dni = DiffAbs(c, d);
	        Dni = Dni + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 16:							// Mandelbar Celtic
	        c = r * r - i * i;
	        d = 2 * r*a + a2 - 2 * i*b - b2;
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        Dni = b0 - 2 * (r*b + a * i + a * b);

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 17:							// Perpendicular Mandelbrot
	        Dnr = 2 * r*a + a2 - b2 - 2 * b*i + a0;
	        c = r;
	        d = a;
	        Dni = DiffAbs(c, d);
	        Dni = b0 - Dni * i*2 - fabs(r + a)*b*2;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 18:							// Perpendicular Burning Ship
	        Dnr = 2 * r*a + a2 - b2 - 2 * b*i + a0;
	        c = i;
	        d = b;
	        Dni = DiffAbs(c, d);
	        Dni = b0 - Dni * r*2 - a * fabs(i + b)*2;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 19:							// Perpendicular Celtic
	        c = r * r - i * i;
	        d = 2 * r*a + a2 - 2 * i*b - b2;
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        c = r;
	        d = a;
	        Dni = DiffAbs(c, d);
	        Dni = b0 - fabs(r + a)*b*2 - Dni * i*2;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 20:							// Perpendicular Buffalo
	        c = r * r - i * i;
	        d = 2 * r*a + a2 - 2 * i*b - b2;
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        c = i;
	        d = b;
	        Dni = DiffAbs(c, d);
	        Dni = b0 - Dni * r*2 - a * fabs(i + b)*2;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 21:							// Cubic Quasi Burning Ship
	        ar = a*r;
	        ib = i*b;
	        Dnr = DiffAbs(r, a);
	        ab = r + a;
	        Dnr = (r2 - 3 * i2) * Dnr + (2 * ar + a2 - 6 * ib - 3 * b2)*fabs(ab) + a0;
	        c = i*(3 * r2 - i2);
	        d = 3 * i*(2 * r*a + a2 - b2) + 3 * b*(r2 + 2 * r*a + a2) - b*(3 * i2 + b2);
	        Dni = DiffAbs(c, d);
	        Dni = b0 - Dni;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 22:							// Cubic Partial BS Real
	        ar = a*r;
	        ib = i*b;
	        Dnr = DiffAbs(r, a);
	        ab = r + a;
	        Dnr = (r2 - 3 * i2) * Dnr + (2 * ar + a2 - 6 * ib - 3 * b2)*fabs(ab) + a0;
	        Dni = 6 * r*(i*a + a*b) + 3 * i*(a2 - b2) + 3 * b*(r2 - i2) + b*(3 * a2 - b2) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 23:							// Cubic Partial BS Imag
	        ar = a*r;
	        ib = i*b;
	        Dnr = 3 * r2*a + 3 * r*a2 - 6 * r*i*b - 3 * r*b2 + a*a2 - 3 * i2*a - 6 * i*a*b - 3 * a*b2 + a0;
	        Dni = DiffAbs(i, b);
	        ab = i + b;
	        Dni = (3 * r2 - i2) * Dni + (6 * ar + 3 * a2 - 2 * ib - b2) * fabs(ab) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 24:							// Cubic Flying Squirrel
	        Dnr = 3 * r2*a + 3 * r*a2 - 6 * r*i*b - 3 * r*b2 + a*a2 - 3 * i2*a - 6 * i*a*b - 3 * a*b2 + a0;
	        c = i*(3 * r2 - i2);
	        d = 3 * i*(2 * r*a + a2 - b2) + 3 * b*(r2 + 2 * r*a + a2) - b*(3 * i2 + b2);
	        Dni = DiffAbs(c, d);
	        Dni = Dni + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 25:							// Cubic Quasi Perpendicular
	        ar = a*r;
	        ib = i*b;
	        Dnr = DiffAbs(r, a);
	        ab = r + a;
	        Dnr = (r2 - 3 * i2) * Dnr + (2 * ar + a2 - 6 * ib - 3 * b2)*fabs(ab) + a0;
	        c = 3 * r2 - i2;
	        d = 6 * r*a + 3 * a2 - 2 * i*b - b2;
	        Dni = DiffAbs(c, d);
	        ab = 3 * r2 + 6 * r*a + 3 * a2 - i2 - 2 * i*b - b2;
	        Dni = b0 - Dni*i - fabs(ab)*b;
	
	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 26:							// 4th Burning Ship Partial Imag
	        Dnr = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2+4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2-12*x2*y*b-6*x2*b2-12*x*a*y2-24*x*a*y*b-12*x*a*b2-6*a2*y2-12*a2*y*b-6*a2*b2 + a0;
	        c = y;
	        d = b;
	        Dni = DiffAbs(c, d);
	        Dni = fabs(y+b)*(12*x2*a+12*x*a2+4*a2*a - 4*a*y2-8*b*x*y-8*a*b*y-4*b2*x-4*a*b2) + Dni*(4*x2*x - 4*x*y2) + b0;
						
	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 27:							// 4th Burning Ship Partial Real
	        Dnr = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2+4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2-12*x2*y*b-6*x2*b2-12*x*a*y2-24*x*a*y*b-12*x*a*b2-6*a2*y2-12*a2*y*b-6*a2*b2 + a0;
	        c = x;
	        d = a;
	        Dni = DiffAbs(c, d);
	        Dni = fabs(x+a)*(4*x2*b+8*x*a*y+8*x*a*b+4*a2*y+4*a2*b - 12*b*y2-12*b2*y-4*b2*b) + Dni*(4*x2*y - 4*y2*y) + b0;
						
	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 28:							// 4th Burning Ship Partial Real Mbar
	        Dnr = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2+4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2-12*x2*y*b-6*x2*b2-12*x*a*y2-24*x*a*y*b-12*x*a*b2-6*a2*y2-12*a2*y*b-6*a2*b2 + a0;
	        c = x;
	        d = a;
	        Dni = DiffAbs(c, d);
	        Dni = fabs(x+a)*(12*y2*b+12*y*b2+4*b2*b - 8*a*x*y-4*a2*y-4*b*x2-8*a*b*x-4*a2*b) + Dni*(4*y2*y - 4*x2*y) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 29:							// 4th Celtic Burning Ship Partial Imag
	        c = x2*x2 + y2*y2 - 6*x2*y2;
	        d = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2 + 4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2 - 12*a*x*y2-6*a2*y2-12*b*x2*y-24*a*b*x*y-12*a2*b*y-6*b2*x2-12*a*b2*x-6*a2*b2;
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        c = y;
	        d = b;
	        Dni = DiffAbs(c, d);
	        Dni = fabs(y+b)*(12*x2*a+12*x*a2+4*a2*a - 4*a*y2-8*b*x*y-8*a*b*y-4*b2*x-4*a*b2) + Dni*(4*x2*x - 4*x*y2) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 30:							// 4th Celtic Burning Ship Partial Real
	        c = x2*x2 + y2*y2 - 6*x2*y2;
	        d = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2 + 4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2 - 12*a*x*y2-6*a2*y2-12*b*x2*y-24*a*b*x*y-12*a2*b*y-6*b2*x2-12*a*b2*x-6*a2*b2;
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        c = x;
	        d = a;
	        Dni = DiffAbs(c, d);
	        Dni = fabs(x+a)*(4*x2*b+8*x*a*y+8*x*a*b+4*a2*y+4*a2*b - 12*b*y2-12*b2*y-4*b2*b) + Dni*(4*x2*y - 4*y2*y) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 31:							// 4th Celtic Burning Ship Partial Real Mbar
	        c = x2*x2 + y2*y2 - 6*x2*y2;
	        d = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2 + 4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2 - 12*a*x*y2-6*a2*y2-12*b*x2*y-24*a*b*x*y-12*a2*b*y-6*b2*x2-12*a*b2*x-6*a2*b2;
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        c = x;
	        d = a;
	        Dni = DiffAbs(c, d);
	        Dni = fabs(x+a)*(12*y2*b+12*y*b2+4*b2*b - 8*a*x*y-4*a2*y-4*b*x2-8*a*b*x-4*a2*b) + Dni*(4*y2*y - 4*x2*y) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 32:							// 4th Buffalo Partial Imag
	        Dnr = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2+4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2-12*x2*y*b-6*x2*b2-12*x*a*y2-24*x*a*y*b-12*x*a*b2-6*a2*y2-12*a2*y*b-6*a2*b2 + a0;
	        c = 4*x2*x*y-4*x*y2*y;
	        d = -4*a*y2*y-12*b*x*y2-12*a*b*y2+12*a*x2*y-12*b2*x*y+12*a2*x*y-12*a*b2*y+4*a2*a*y+4*b*x2*x+12*a*b*x2-4*b2*b*x+12*a2*b*x-4*a*b2*b+4*a2*a*b;
	        Dni = DiffAbs(c, d);
	        Dni=Dni+b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 33:							// 4th Celtic Mbar
	        c = x2*x2 + y2*y2 - 6*x2*y2;
	        d = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2 + 4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2 - 12*a*x*y2-6*a2*y2-12*b*x2*y-24*a*b*x*y-12*a2*b*y-6*b2*x2-12*a*b2*x-6*a2*b2;
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        Dni = b0 - (12*x2*y*a+12*x*y*a2-12*x*y2*b-12*x*y*b2+4*x2*x*b+12*x2*b*a+12*x*b*a2-4*x*b2*b+4*a2*a*y-4*a*y2*y-12*a*y2*b-12*a*y*b2+4*a2*a*b-4*a*b2*b);
						
	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 34:							// 4th False Quasi Perpendicular
	        Dnr = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2+4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2-12*x2*y*b-6*x2*b2-12*x*a*y2-24*x*a*y*b-12*x*a*b2-6*a2*y2-12*a2*y*b-6*a2*b2 + a0;
	        c = x2-y2;
	        d = -2*b*y+2*a*x-b2+a2;
	        Dni = DiffAbs(c, d);
	        Dni = -(4*x*y)*Dni - (4*x*b + 4*a*y + 4*a*b)*fabs(-y2-2*b*y+x2+2*a*x-b2+a2) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 35:							// 4th False Quasi Heart
	        Dnr = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2+4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2-12*x2*y*b-6*x2*b2-12*x*a*y2-24*x*a*y*b-12*x*a*b2-6*a2*y2-12*a2*y*b-6*a2*b2 + a0;
	        c = x2-y2;
	        d = -2*b*y+2*a*x-b2+a2;
	        Dni = DiffAbs(c, d);
	        Dni = (4*x*y)*Dni + (4*x*b + 4*a*y + 4*a*b)*fabs(-y2-2*b*y+x2+2*a*x-b2+a2) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 36:							// 4th Celtic False Quasi Perpendicular
	        c = x2*x2 + y2*y2 - 6*x2*y2;
	        d = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2 + 4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2 - 12*a*x*y2-6*a2*y2-12*b*x2*y-24*a*b*x*y-12*a2*b*y-6*b2*x2-12*a*b2*x-6*a2*b2;
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        c = x2-y2;
	        d = -2*b*y+2*a*x-b2+a2;
	        Dni = DiffAbs(c, d);
	        Dni = -(4*x*y)*Dni - (4*x*b + 4*a*y + 4*a*b)*fabs(-y2-2*b*y+x2+2*a*x-b2+a2) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 37:							// 4th Celtic False Quasi Heart
	        c = x2*x2 + y2*y2 - 6*x2*y2;
	        d = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2 + 4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2 - 12*a*x*y2-6*a2*y2-12*b*x2*y-24*a*b*x*y-12*a2*b*y-6*b2*x2-12*a*b2*x-6*a2*b2;
	        Dnr = DiffAbs(c, d);
	        Dnr = Dnr + a0;
	        c = x2-y2;
	        d = -2*b*y+2*a*x-b2+a2;
	        Dni = DiffAbs(c, d);
	        Dni = (4*x*y)*Dni + (4*x*b + 4*a*y + 4*a*b)*fabs(-y2-2*b*y+x2+2*a*x-b2+a2) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 38:							// 4th Imag Quasi Perpendicular / Heart
	        Dnr = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2+4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2-12*x2*y*b-6*x2*b2-12*x*a*y2-24*x*a*y*b-12*x*a*b2-6*a2*y2-12*a2*y*b-6*a2*b2 +a0;
	        Dni = 4 * (a) * fabs(- y2*y-3*b*y2+x2*y+2*a*x*y-3*b2*y+a2*y+b*x2+2*a*b*x-b2*b+a2*b) + 4 * x * DiffAbs(- y2*y+x2*y,-3*b*y2+2*a*x*y-3*b2*y+a2*y+b*x2+2*a*b*x-b2*b+a2*b) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 39:							// 4th Real Quasi Perpendicular
	        Dnr = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2+4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2-12*x2*y*b-6*x2*b2-12*x*a*y2-24*x*a*y*b-12*x*a*b2-6*a2*y2-12*a2*y*b-6*a2*b2 +a0;
	        Dni = -4*y*DiffAbs(x2*x-x*y2,-a*y2-2*b*x*y-2*a*b*y+3*a*x2-b2*x+3*a2*x-a*b2+a2*a) - 4*b*fabs(- x*y2-a*y2-2*b*x*y-2*a*b*y+x2*x+3*a*x2-b2*x+3*a2*x-a*b2+a2*a) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 40:							// 4th Real Quasi Heart
	        Dnr = 4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2+4*y2*y*b+6*y2*b2+4*y*b2*b+b2*b2-12*x2*y*b-6*x2*b2-12*x*a*y2-24*x*a*y*b-12*x*a*b2-6*a2*y2-12*a2*y*b-6*a2*b2 +a0;
	        Dni = 4*y*DiffAbs(x2*x-x*y2,-a*y2-2*b*x*y-2*a*b*y+3*a*x2-b2*x+3*a2*x-a*b2+a2*a) + 4*b*fabs(- x*y2-a*y2-2*b*x*y-2*a*b*y+x2*x+3*a*x2-b2*x+3*a2*x-a*b2+a2*a) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 41:							// 4th Celtic Imag Quasi Perpendicular / Heart
	        Dnr = DiffAbs(x2*x2 + y2*y2 - 6*x2*y2,4*y2*y*b-12*y2*a*x+6*y2*b2-6*y2*a2-12*x2*y*b-24*x*y*a*b+4*b2*b*y-12*b*y*a2+4*x2*x*a-6*x2*b2+6*x2*a2-12*b2*x*a+4*a2*a*x+b2*b2-6*b2*a2+a2*a2) +a0;
	        Dni = 4 * a * fabs(- y2*y-3*b*y2+x2*y+2*a*x*y-3*b2*y+a2*y+b*x2+2*a*b*x-b2*b+a2*b) + 4 * x * DiffAbs(- y2*y+x2*y,-3*b*y2+2*a*x*y-3*b2*y+a2*y+b*x2+2*a*b*x-b2*b+a2*b) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 42:							// 4th Celtic Real Quasi Perpendicular
	        Dnr = DiffAbs(x2*x2 + y2*y2 - 6*x2*y2,4*y2*y*b-12*y2*a*x+6*y2*b2-6*y2*a2-12*x2*y*b-24*x*y*a*b+4*b2*b*y-12*b*y*a2+4*x2*x*a-6*x2*b2+6*x2*a2-12*b2*x*a+4*a2*a*x+b2*b2-6*b2*a2+a2*a2) +a0;
	        Dni = -4*y*DiffAbs(x2*x-x*y2,-a*y2-2*b*x*y-2*a*b*y+3*a*x2-b2*x+3*a2*x-a*b2+a2*a) - 4*b*fabs(- x*y2-a*y2-2*b*x*y-2*a*b*y+x2*x+3*a*x2-b2*x+3*a2*x-a*b2+a2*a) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 43:							// 4th Celtic Real Quasi Heart
	        Dnr = DiffAbs(x2*x2 + y2*y2 - 6*x2*y2,4*y2*y*b-12*y2*a*x+6*y2*b2-6*y2*a2-12*x2*y*b-24*x*y*a*b+4*b2*b*y-12*b*y*a2+4*x2*x*a-6*x2*b2+6*x2*a2-12*b2*x*a+4*a2*a*x+b2*b2-6*b2*a2+a2*a2) +a0;
	        Dni = 4*y*DiffAbs(x2*x-x*y2,-a*y2-2*b*x*y-2*a*b*y+3*a*x2-b2*x+3*a2*x-a*b2+a2*a) + 4*b*fabs(- x*y2-a*y2-2*b*x*y-2*a*b*y+x2*x+3*a*x2-b2*x+3*a2*x-a*b2+a2*a) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 44:							// 5th Burning Ship Partial
	        c = x;
	        d = a;
	        Dnr = DiffAbs(c, d);
	        Dnr = (Dnr) * (x2*x2- 10*x2*y2 + 5*y2*y2) + fabs(x+a) * (4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2 -20*x2*y*b-10*x2*b2-20*x*a*y2-40*x*a*y*b-20*x*a*b2-10*a2*y2-20*a2*y*b-10*a2*b2 + 20*y2*y*b+30*y2*b2+20*y*b2*b+5*b2*b2) + a0;
	        Dni = 20*y*x2*x*a+30*y*x2*a2+20*y*x*a2*a+5*y*a2*a2-30*y2*x2*b-30*y*x2*b2-20*y2*y*x*a-60*y2*x*a*b-60*y*x*a*b2-10*y2*y*a2-30*y2*a2*b-30*y*a2*b2+5*y2*y2*b+10*y2*y*b2+10*y2*b2*b+5*y*b2*b2+5*b*x2*x2+20*b*x2*x*a
												    +30*b*x2*a2+20*b*x*a2*a+5*b*a2*a2-10*b2*b*x2-20*b2*b*x*a-10*b2*b*a2+b2*b2*b + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 45:							// 5th Burning Ship Partial Mbar
	        c = x;
	        d = a;
	        Dnr = DiffAbs(c, d);
	        Dnr = (Dnr) * (x2*x2- 10*x2*y2 + 5*y2*y2) + fabs(x+a) * (4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2 -20*x2*y*b-10*x2*b2-20*x*a*y2-40*x*a*y*b-20*x*a*b2-10*a2*y2-20*a2*y*b-10*a2*b2 + 20*y2*y*b+30*y2*b2+20*y*b2*b+5*b2*b2) + a0;
	        Dni = b0 - (20*y*x2*x*a+30*y*x2*a2+20*y*x*a2*a+5*y*a2*a2-30*y2*x2*b-30*y*x2*b2-20*y2*y*x*a-60*y2*x*a*b-60*y*x*a*b2-10*y2*y*a2-30*y2*a2*b-30*y*a2*b2+5*y2*y2*b+10*y2*y*b2+10*y2*b2*b+5*y*b2*b2+5*b*x2*x2+20*b*x2*x*a
												    +30*b*x2*a2+20*b*x*a2*a+5*b*a2*a2-10*b2*b*x2-20*b2*b*x*a-10*b2*b*a2+b2*b2*b);

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 46:							// 5th Celtic Mbar
	        c = 5*x*y2*y2-10*x2*x*y2+x2*x2*x;
	        d = 20*x*b*y2*y-30*x2*a*y2+30*x*b2*y2-30*x*a2*y2-20*x2*x*b*y-60*x2*a*b*y+20*x*b2*b*y-60*x*a2*b*y+5*x2*x2*a-10*x2*x*b2+10*x2*x*a2-30*x2*a*b2+10*x2*a2*a+5*x*b2*b2-30*x*a2*b2+5*x*a2*a2+5*a*y2*y2+20*a*b*y2*y
												    +30*a*b2*y2-10*a2*a*y2+20*a*b2*b*y-20*a2*a*b*y+5*a*b2*b2-10*a2*a*b2+a2*a2*a;
	        Dnr = DiffAbs(c, d);
	        Dnr+=a0;
	        Dni = b0 - (20*y*x2*x*a+30*y*x2*a2+20*y*x*a2*a+5*y*a2*a2-30*y2*x2*b-30*y*x2*b2-20*y2*y*x*a-60*y2*x*a*b-60*y*x*a*b2-10*y2*y*a2-30*y2*a2*b-30*y*a2*b2+5*y2*y2*b+10*y2*y*b2+10*y2*b2*b+5*y*b2*b2+5*b*x2*x2
												    +20*b*x2*x*a+30*b*x2*a2+20*b*x*a2*a+5*b*a2*a2-10*b2*b*x2-20*b2*b*x*a-10*b2*b*a2+b2*b2*b);

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 47:							// 5th Quasi Burning Ship (BS/Buffalo Hybrid)
	        c = x;
	        d = a;
	        Dnr = DiffAbs(c, d);
	        Dnr = (Dnr) * (x2*x2- 10*x2*y2 + 5*y2*y2) + fabs(x+a) * (4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2 -20*x2*y*b-10*x2*b2-20*x*a*y2-40*x*a*y*b-20*x*a*b2-10*a2*y2-20*a2*y*b-10*a2*b2 + 20*y2*y*b+30*y2*b2+20*y*b2*b+5*b2*b2) + a0;
	        c = y2*y2*y-10*y2*y*x2+5*y*x2*x2;
	        d = 5*y2*y2*b-20*y2*y*a*x+10*y2*y*b2-10*y2*y*a2-30*y2*b*x2-60*y2*a*b*x+10*y2*b2*b-30*y2*a2*b+20*y*a*x2*x-30*y*b2*x2+30*y*a2*x2-60*y*a*b2*x+20*y*a2*a*x+5*y*b2*b2-30*y*a2*b2+5*y*a2*a2+5*b*x2*x2+20*b*a*x2*x-10*b2*b*x2
												    +30*b*a2*x2-20*b2*b*a*x+20*b*a2*a*x+b2*b2*b-10*b2*b*a2+5*b*a2*a2;
	        Dni = DiffAbs(c, d);
	        Dni=b0-Dni;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 48:							// 5th Quasi Perpendicular
	        c = x;
	        d = a;
	        Dnr = DiffAbs(c, d);
	        Dnr = (Dnr) * (x2*x2- 10*x2*y2 + 5*y2*y2) + fabs(x+a) * (4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2 -20*x2*y*b-10*x2*b2-20*x*a*y2-40*x*a*y*b-20*x*a*b2-10*a2*y2-20*a2*y*b-10*a2*b2 + 20*y2*y*b+30*y2*b2+20*y*b2*b+5*b2*b2) + a0;
	        c = 5*x2*x2 - 10*x2*y2 + y2*y2;
	        d = 4*b*y2*y-20*a*x*y2+6*b2*y2-10*a2*y2-20*b*x2*y-40*a*b*x*y+4*b2*b*y-20*a2*b*y+20*a*x2*x-10*b2*x2+30*a2*x2-20*a*b2*x+20*a2*a*x+b2*b2-10*a2*b2+5*a2*a2;
	        Dni = DiffAbs(c, d);
	        Dni = -y * Dni - b * fabs(y2*y2+4*b*y2*y-10*x2*y2-20*a*x*y2+6*b2*y2-10*a2*y2-20*b*x2*y-40*a*b*x*y+4*b2*b*y-20*a2*b*y+5*x2*x2+20*a*x2*x-10*b2*x2+30*a2*x2-20*a*b2*x+20*a2*a*x+b2*b2-10*a2*b2+5*a2*a2) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 49:							// 5th Quasi Heart
	        c = x;
	        d = a;
	        Dnr = DiffAbs(c, d);
	        Dnr = (Dnr) * (x2*x2- 10*x2*y2 + 5*y2*y2) + fabs(x+a) * (4*x2*x*a+6*x2*a2+4*x*a2*a+a2*a2 -20*x2*y*b-10*x2*b2-20*x*a*y2-40*x*a*y*b-20*x*a*b2-10*a2*y2-20*a2*y*b-10*a2*b2 + 20*y2*y*b+30*y2*b2+20*y*b2*b+5*b2*b2) + a0;
	        c = 5*x2*x2 - 10*x2*y2 + y2*y2;
	        d = 4*b*y2*y-20*a*x*y2+6*b2*y2-10*a2*y2-20*b*x2*y-40*a*b*x*y+4*b2*b*y-20*a2*b*y+20*a*x2*x-10*b2*x2+30*a2*x2-20*a*b2*x+20*a2*a*x+b2*b2-10*a2*b2+5*a2*a2;
	        Dni = DiffAbs(c, d);
	        Dni = y * Dni + b * fabs(y2*y2+4*b*y2*y-10*x2*y2-20*a*x*y2+6*b2*y2-10*a2*y2-20*b*x2*y-40*a*b*x*y+4*b2*b*y-20*a2*b*y+5*x2*x2+20*a*x2*x-10*b2*x2+30*a2*x2-20*a*b2*x+20*a2*a*x+b2*b2-10*a2*b2+5*a2*a2) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 50:							// SimonBrot
	        //Dnr = b*b*abs(b*b)-4*b*abs(a*b)*a-a*a*abs(b*b)-b*b*abs(a*a)+a*a*abs(a*a) + a0;
	        //Dni = - 2*a*b*abs(b*b)-2*b*b*abs(a*b)+2*a*a*abs(a*b)+2*a*b*abs(a*a) + b0;
	        Dnr = (y2)*DiffAbs(y2,2*b*y+b2)-4*y*x*DiffAbs(x*y,x*b+a*y+a*b)-x2* DiffAbs(y2,2*b*y+b2)-y2* DiffAbs(x2,2*x*a+a2)+x2* DiffAbs(x2,2*x*a+a2)
							    + (2*b*y+b2)*fabs(y2+2*b*y+b2)-4*(y*a+b*x+b*a)*fabs(x*y+x*b+a*y+a*b)-(2*x*a+a2)*fabs(y2+2*b*y+b2)-(2*b*y+b2)*fabs(x2+2*x*a+a2)+(2*x*a+a2)*fabs(x2+2*x*a+a2) + a0;
	        Dni = 2*x2*DiffAbs(x*y,x*b+a*y+a*b)+2*x*y*DiffAbs(x2,2*x*a+a2)-2*x*y*DiffAbs(y2,2*b*y+b2)-2*y2*DiffAbs(x*y,x*b+a*y+a*b)
							    + 2*(2*x*a+a2)*fabs(x*y+x*b+a*y+a*b) +2*(x*b+a*y+a*b)*fabs(x2+2*x*a+a2)-2*(x*b+a*y+a*b)*fabs(y2+2*b*y+b2)-2*(2*b*y+b2)*fabs(x*y+x*b+a*y+a*b)  + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 51:							// Cubic SimonBrot
	        Dnr= 3*DiffAbs(x2*y,x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b)*(y2*y) - DiffAbs(y2*y,3*y2*b+3*y*b2+b2*b)*(y2*y) + 9* DiffAbs(x*y2,2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2)*(x*y2) - 3* DiffAbs(x2*x,3*x2*a+3*x*a2+a2*a)*(x*y2) + 3* DiffAbs(y2*y,3*y2*b+3*y*b2+b2*b)*(x2*y)
						    - 9* DiffAbs(x2*y,x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b)*(x2*y) - 3* DiffAbs(x*y2,2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2)*(x2*x) + DiffAbs(x2*x,3*x2*a+3*x*a2+a2*a)*(x2*x)
						    + 3*fabs(x2*y+x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b)*(3*y2*b+3*y*b2+b2*b) - fabs(y2*y+3*y2*b+3*y*b2+b2*b)*(3*y2*b+3*y*b2+b2*b) + 9*fabs(x*y2+2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2)*(2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2) 
						    - 3*fabs(x2*x+3*x2*a+3*x*a2+a2*a)*(2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2) + 3*fabs(y2*y+3*y2*b+3*y*b2+b2*b)*(x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b) 
						    - 9*fabs(x2*y+x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b)*(x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b) - 3*fabs(x*y2+2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2)*(3*x2*a+3*x*a2+a2*a) + fabs(x2*x+3*x2*a+3*x*a2+a2*a)*(3*x2*a+3*x*a2+a2*a) + a0;
	        Dni= 3*DiffAbs(x*y2,2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2)*(y2*y) - DiffAbs(x2*x,3*x2*a+3*x*a2+a2*a)*(y2*y) - 9* DiffAbs(x2*y,x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b)*(x*y2) - 9* DiffAbs(x*y2,2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2)*(x2*y)
						    + 3* DiffAbs(x2*x,3*x2*a+3*x*a2+a2*a)*(x2*y) + 3* DiffAbs(y2*y,3*y2*b+3*y*b2+b2*b)*(x*y2) - DiffAbs(y2*y,3*y2*b+3*y*b2+b2*b)*(x2*x) + 3* DiffAbs(x2*y,x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b)*(x2*x)
						    + 3*fabs(x*y2+2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2)*(3*y2*b+3*y*b2+b2*b) - fabs(x2*x+3*x2*a+3*x*a2+a2*a)*(3*y2*b+3*y*b2+b2*b) - 9*fabs(x2*y+x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b)*(2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2) 
						    - 9*fabs(x*y2+2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2)*(x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b) + 3*fabs(x2*x+3*x2*a+3*x*a2+a2*a)*(x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b) + 3*fabs(y2*y+3*y2*b+3*y*b2+b2*b)*(2*x*y*b+x*b2+a*y2+2*a*y*b+a*b2) 
						    - fabs(y2*y+3*y2*b+3*y*b2+b2*b)*(3*x2*a+3*x*a2+a2*a) + 3*fabs(x2*y+x2*b+2*x*a*y+2*x*a*b+a2*y+a2*b)*(3*x2*a+3*x*a2+a2*a)	+ b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
	        break;
	    case 52:							// SimonBrot2 4th
	        DeltaSubN->x = (x2 - y2)*DiffAbs(x2 - y2, +2 * x*a + a2 - 2 * y*b - b2) + (2 * x*a + a2 - 2 * y*b - b2)*fabs(x2 + 2 * x*a + a2 - y2 - 2 * y*b - b2)
		    - (2 * x*y)*DiffAbs(2 * x*y, 2 * x*b + 2 * a*y + 2 * a*b) - (2 * x*b + 2 * a*y + 2 * a*b)*fabs(2 * x*y + 2 * x*b + 2 * a*y + 2 * a*b) + a0;
	        DeltaSubN->y = (x2 - y2)*DiffAbs(2 * x*y, 2 * x*b + 2 * a*y + 2 * a*b) + (2 * x*a + a2 - 2 * y*b - b2)*fabs(2 * x*y + 2 * x*b + 2 * a*y + 2 * a*b)
		    + (2 * x*y)*DiffAbs(x2 - y2, 2 * x*a + a2 - 2 * y*b - b2) + (2 * x*b + 2 * a*y + 2 * a*b)*fabs(x2 - y2 + 2 * x*a + a2 - 2 * y*b - b2) + b0;
	        break;
	    case 53:							// TheRedshiftRider: a*z^2+/-z^n+c
	        {
	        Complex Zp(1.0, 0.0);
	        Complex sum(0.0, 0.0);
	        Dn = *DeltaSubN;
	        z = {x, y};
	        Multiplier = (rsrSign) ? 1.0 : -1.0;
	        for (int i = 0; i < power; i++)
		        {
		        sum += Zp * (double)PascalArray[i];
		        sum *= *DeltaSubN;
		        Zp *= *XRef;
		        }
	        *DeltaSubN = Multiplier * sum + (rsrA * (2 * z*Dn + Dn * Dn));
	        *DeltaSubN += *DeltaSub0;
	        }
	        break;
	    case 54:							// HPDZ Buffalo
	        Dnr = a*(2*x+a)-b*(b+2*y) - DiffAbs(x,a) + a0;
	        Dni = DiffAbs(x*y,x*b+a*y+a*b) * 2.0 - DiffAbs(y,b) + b0;

	        DeltaSubN->y = Dni;
	        DeltaSubN->x = Dnr;
            break;
        default:
            Dnr = (2 * r + a) * a - (2 * i + b) * b + a0;
            Dni = 2 * ((r + a) * b + i * a) + b0;
            DeltaSubN->y = Dni;
            DeltaSubN->x = Dnr;
            break;
	    }
    }

//////////////////////////////////////////////////////////////////////
// Reference Zoom Point Functions
// Code is written in raw MPFR code to optimise speedy
//////////////////////////////////////////////////////////////////////

void CPertEngine::BigRefFunctions(BFComplex *centre, BFComplex *Z, BFComplex *ZTimes2)
    {
    bf_t	TempReal, TempImag, SqrReal, SqrImag, RealImag;
//    bf_t	zisqr, zrsqr, realimag, temp, RealImagSqr;
    BFComplex TempCmplx, TempCmplx1;
        //    BFComplex sqrsqr, zabsBig, tempzBig, sqrtzBig;
//    BFComplex aBig;
    int cplxsaved;

    cplxsaved = save_stack();

    TempReal = alloc_stack(g_r_bf_length + 2);
    TempImag = alloc_stack(g_r_bf_length + 2);
    SqrReal = alloc_stack(g_r_bf_length + 2);
    SqrImag = alloc_stack(g_r_bf_length + 2);
    RealImag = alloc_stack(g_r_bf_length + 2);
    TempCmplx.x = alloc_stack(g_r_bf_length + 2);
    TempCmplx.y = alloc_stack(g_r_bf_length + 2);
    TempCmplx1.x = alloc_stack(g_r_bf_length + 2);
    TempCmplx1.y = alloc_stack(g_r_bf_length + 2);

    switch (subtype)
	{
	case 0:							// optimise for Mandelbrot by taking out as many steps as possible
	    //	    Z = Z.CSqr() + centre;
	    square_bf(SqrReal, Z->x);
	    square_bf(SqrImag, Z->y);
	    sub_bf(TempReal, SqrReal, SqrImag);
	    add_bf(Z->x, TempReal, centre->x);
	    mult_bf(RealImag, ZTimes2->x, Z->y);
	    add_bf(Z->y, RealImag, centre->y);
	    break;
	case 1:
	    if (power == 3)
            {
            CCube(&TempCmplx, *Z);
            add_bf(Z->x, TempCmplx.x, centre->x);
            add_bf(Z->y, TempCmplx.y, centre->y);
            }
	    else
		    {
            copy_bf(TempCmplx.x, Z->x);
            copy_bf(TempCmplx.y, Z->y);
		    for (int k = 0; k < power - 1; k++)
                cplxmul_bf(&TempCmplx, &TempCmplx, Z);
            add_bf(Z->x, TempCmplx.x, centre->x);
            add_bf(Z->y, TempCmplx.y, centre->y);
		    }
	    break;
	case 2:							// Burning Ship
	    square_bf(SqrReal, Z->x);
	    square_bf(SqrImag, Z->y);
	    sub_bf(TempReal, SqrReal, SqrImag);
	    add_bf(Z->x, TempReal, centre->x);
	    mult_bf(TempImag, ZTimes2->x, Z->y);
	    abs_bf(RealImag, TempImag);
	    add_bf(Z->y, RealImag, centre->y);
	    break;

	case 3:							// Cubic Burning Ship
	case 4:							// 4th Power Burning Ship
	case 5:							// 5th Power Burning Ship
        abs_bf(TempCmplx.x, Z->x);
        abs_bf(TempCmplx.y, Z->y);
        CPolynomial(&TempCmplx1, TempCmplx, power);
        add_bf(Z->x, TempCmplx1.x, centre->x);
        add_bf(Z->y, TempCmplx1.y, centre->y);
	    break;
	case 6:							// Celtic
	    square_bf(SqrReal, Z->x);
	    square_bf(SqrImag, Z->y);
	    mult_bf(RealImag, ZTimes2->x, Z->y);
        add_bf(Z->y, RealImag, centre->y);
  	    sub_bf(TempReal, SqrReal, SqrImag);
        abs_bf(TempImag, TempReal);
        add_bf(Z->x, TempImag, centre->x);
	    break;
	case 7:							// Cubic Celtic
        CPolynomial(&TempCmplx, *Z, 3);
        abs_bf(TempReal, TempCmplx.x);
        add_bf(Z->x, TempReal, centre->x);
        add_bf(Z->y, centre->y, TempCmplx.y);
	    break;
	case 8:							// 4th Celtic Buffalo
        CPolynomial(&TempCmplx, *Z, 4);
        abs_bf(TempReal, TempCmplx.x);
        add_bf(Z->x, TempReal, centre->x);
        add_bf(Z->y, centre->y, TempCmplx.y);
	    break;
	case 9:							// 5th Celtic
        CPolynomial(&TempCmplx, *Z, 5);
        abs_bf(TempReal, TempCmplx.x);
        add_bf(Z->x, TempReal, centre->x);
        add_bf(Z->y, centre->y, TempCmplx.y);
	    break;
	case 10:						// Mandelbar (Tricorn)
	    square_bf(SqrReal, Z->x);
	    square_bf(SqrImag, Z->y);
	    mult_bf(RealImag, ZTimes2->x, Z->y);
   	    sub_bf(TempReal, SqrReal, SqrImag);
        add_bf(Z->x, TempReal, centre->x);
        sub_bf(Z->y, centre->y, RealImag);
	    break;
	case 11:						// Mandelbar (power)
        CPolynomial(&TempCmplx, *Z, power);
        sub_bf(Z->y, centre->y, TempCmplx.y);
        add_bf(Z->x, TempCmplx.x, centre->x);
	    break;
#ifdef ALLOW_ALL_DERIVATIVES

	case 12:						// Buffalo
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    Z->y = Z->x.BigAbs() * Z->y.BigAbs() * -2.0 + centre->y;
	    temp = zrsqr - zisqr;
	    Z->x = centre->x + temp.BigAbs();
	    break;
	case 13:						// Cubic Buffalo
	case 14:						// 4th power Buffalo
	case 15:						// 5th power Buffalo
//	    zrsqr = Z->x.BigSqr();
//	    zisqr = Z->y.BigSqr();
//	    temp = (zrsqr * 3.0 - zisqr) * Z->y;
//	    Z->y = temp.BigAbs() + centre->y;
//	    temp = (zisqr - zisqr * 3.0) * Z->x;
//	    Z->x = centre->x + temp.BigAbs();
	    *Z = Z->CPolynomial(power);
	    Z->y = Z->y.BigAbs() + centre->y;
	    Z->x = Z->x.BigAbs() + centre->x;
	    break;
	case 16:						// Mandelbar Celtic
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    Z->y = Z->x * Z->y * -2.0 + centre->y;
	    temp = zrsqr - zisqr;
	    Z->x = centre->x + temp.BigAbs();
	    break;
	case 17:						// Perpendicular Mandelbrot
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = Z->x;
	    realimag = temp.BigAbs() * Z->y;
	    Z->x = centre->x + zrsqr - zisqr;
	    Z->y = -(realimag + realimag - centre->y);
	    break;
	case 18:						// Perpendicular Burning Ship
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    Z->y = -Z->x * Z->y.BigAbs() * 2 + centre->y;
	    Z->x = zrsqr - zisqr + centre->x;
	    break;
	case 19:						// Perpendicular Celtic
	    temp = Z->x.BigAbs();
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    Z->y = Z->y * temp * -2.0 + centre->y;
	    temp = zrsqr - zisqr;
	    Z->x = centre->x + temp.BigAbs();
	    break;
	case 20:						// Perpendicular Buffalo
	    temp = Z->y.BigAbs();
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    Z->y = Z->x * temp * -2.0 + centre->y;
	    temp = zrsqr - zisqr;
	    Z->x = centre->x + temp.BigAbs();
	    break;
	case 21:						// Cubic Quasi Burning Ship
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr * 3.0 - zisqr) * Z->y;
	    Z->y = -temp.BigAbs() + centre->y;
	    Z->x = centre->x + (zrsqr - zisqr * 3.0) * Z->x.BigAbs();
	    break;
	case 22:						// Cubic Partial BS Real
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    Z->y = (zrsqr * 3.0 - zisqr) * Z->y + centre->y;
	    Z->x = centre->x + (zrsqr - zisqr * 3.0) * Z->x.BigAbs();
	    break;
	case 23:						// Cubic Partial BS Imag
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    Z->y = (zrsqr * 3.0 - zisqr) * Z->y.BigAbs() + centre->y;
	    Z->x = centre->x + (zrsqr - zisqr * 3.0) * Z->x;
	    break;
	case 24:						// Cubic Flying Squirrel
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr * 3.0 - zisqr) * Z->y;
	    Z->y = temp.BigAbs() + centre->y;
	    Z->x = centre->x + (zrsqr - zisqr * 3.0) * Z->x;
	    break;
	case 25:						// Cubic Quasi Perpendicular
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = zrsqr * 3.0 - zisqr;
	    Z->y = -temp.BigAbs() * Z->y + centre->y;
	    Z->x = centre->x + (zrsqr - zisqr * 3.0) * Z->x.BigAbs();
	    break;
	case 26:							// 4th Burning Ship Partial Imag
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    Z->y = Z->x * Z->y.BigAbs() * 4.0 * (zrsqr - zisqr) + centre->y;
	    Z->x = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0 + centre->x;
	    break;
	case 27:							// 4th Burning Ship Partial Real
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    Z->y = Z->x.BigAbs() * Z->y * 4.0 * (zrsqr - zisqr) + centre->y;
	    Z->x = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0 + centre->x;
	    break;
	case 28:							// 4th Burning Ship Partial Real Mbar
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    Z->y = -Z->x.BigAbs() * Z->y * 4.0 * (zrsqr - zisqr) + centre->y;
	    Z->x = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0 + centre->x;
	    break;
	case 29:							// 4th Celtic Burning Ship Partial Imag
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0;
	    Z->y = Z->x * Z->y.BigAbs() * 4.0 * (zrsqr - zisqr) + centre->y;
	    Z->x = temp.BigAbs() + centre->x;
	    break;
	case 30:							// 4th Celtic Burning Ship Partial Real
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0;
	    Z->y = Z->x.BigAbs() * Z->y * 4.0 * (zrsqr - zisqr) + centre->y;
	    Z->x = temp.BigAbs() + centre->x;
	    break;
	case 31:							// 4th Celtic Burning Ship Partial Real Mbar
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0;
	    Z->y = -Z->x.BigAbs() * Z->y * 4.0 * (zrsqr - zisqr) + centre->y;
	    Z->x = temp.BigAbs() + centre->x;
	    break;
	case 32:							// 4th Buffalo Partial Imag
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = Z->x * Z->y * (zrsqr - zisqr);
	    Z->y = temp.BigAbs() * 4.0 + centre->y;
	    Z->x = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0 + centre->x;
	    break;
	case 33:							// 4th Celtic Mbar
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    Z->y = -Z->x * Z->y * (zrsqr - zisqr) * 4.0 + centre->y;
	    temp = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0;
	    Z->x = temp.BigAbs() + centre->x;
	    break;
	case 34:							// 4th False Quasi Perpendicular
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr - zisqr);
	    Z->y = -Z->x * Z->y * temp.BigAbs() * 4.0 + centre->y;
	    Z->x = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0 + centre->x;
	    break;
	case 35:							// 4th False Quasi Heart
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr - zisqr);
	    Z->y = Z->x * Z->y * temp.BigAbs() * 4.0 + centre->y;
	    Z->x = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0 + centre->x;
	    break;
	case 36:							// 4th Celtic False Quasi Perpendicular
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr - zisqr);
	    Z->y = -Z->x * Z->y * temp.BigAbs() * 4.0 + centre->y;
	    temp = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0;
	    Z->x = temp.BigAbs() + centre->x;
	    break;
	case 37:							// 4th Celtic False Quasi Heart
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr - zisqr);
	    Z->y = Z->x * Z->y * temp.BigAbs() * 4.0 + centre->y;
	    temp = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0;
	    Z->x = temp.BigAbs() + centre->x;
	    break;
	case 38:							// 4th Imag Quasi Perpendicular / Heart
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr - zisqr);
	    Z->y = Z->x * Z->y.BigAbs() * temp.BigAbs() * 4.0 + centre->y;
	    Z->x = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0 + centre->x;
	    break;
	case 39:							// 4th Real Quasi Perpendicular
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr - zisqr);
	    Z->y = -Z->x.BigAbs() * Z->y * temp.BigAbs() * 4.0 + centre->y;
	    Z->x = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0 + centre->x;
	    break;
	case 40:							// 4th Real Quasi Heart
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr - zisqr);
	    Z->y = Z->x.BigAbs() * Z->y * temp.BigAbs() * 4.0 + centre->y;
	    Z->x = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0 + centre->x;
	    break;
	case 41:							// 4th Celtic Imag Quasi Perpendicular / Heart
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr - zisqr);
	    Z->y = Z->x * Z->y.BigAbs() * temp.BigAbs() * 4.0 + centre->y;
	    temp = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0;
	    Z->x = temp.BigAbs() + centre->x;
	    break;
	case 42:							// 4th Celtic Real Quasi Perpendicular
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr - zisqr);
	    Z->y = -Z->x.BigAbs() * Z->y * temp.BigAbs() * 4.0 + centre->y;
	    temp = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0;
	    Z->x = temp.BigAbs() + centre->x;
	    break;
	case 43:							// 4th Celtic Real Quasi Heart
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    temp = (zrsqr - zisqr);
	    Z->y = Z->x.BigAbs() * Z->y * temp.BigAbs() * 4.0 + centre->y;
	    temp = zrsqr * zrsqr + zisqr * zisqr - zrsqr * zisqr * 6.0;
	    Z->x = temp.BigAbs() + centre->x;
	    break;
	case 44:							// 5th Burning Ship Partial
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    sqrsqr.x = zrsqr.BigSqr();
	    sqrsqr.y = zisqr.BigSqr();
	    RealImagSqr = zrsqr * zisqr;
	    Z->y = (sqrsqr.x * 5.0 - RealImagSqr * 10.0 + sqrsqr.y) * Z->y + centre->y;
	    Z->x = (sqrsqr.x - RealImagSqr * 10.0 + sqrsqr.y * 5.0) * Z->x.BigAbs() + centre->x;
	    break;
	case 45:							// 5th Burning Ship Partial Mbar
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    sqrsqr.x = zrsqr.BigSqr();
	    sqrsqr.y = zisqr.BigSqr();
	    RealImagSqr = zrsqr * zisqr;
	    Z->y = -(sqrsqr.x * 5.0 - RealImagSqr * 10.0 + sqrsqr.y) * Z->y + centre->y;
	    Z->x = (sqrsqr.x - RealImagSqr * 10.0 + sqrsqr.y * 5.0) * Z->x.BigAbs() + centre->x;
	    break;
	case 46:							// 5th Celtic Mbar
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    sqrsqr.x = zrsqr.BigSqr();
	    sqrsqr.y = zisqr.BigSqr();
	    RealImagSqr = zrsqr * zisqr;
	    Z->y = -(sqrsqr.x * 5.0 - RealImagSqr * 10.0 + sqrsqr.y) * Z->y + centre->y;
	    temp = (sqrsqr.x - RealImagSqr * 10.0 + sqrsqr.y * 5.0) * Z->x;
	    Z->x = temp.BigAbs() + centre->x;
	    break;
	case 47:							// 5th Quasi Burning Ship (BS/Buffalo Hybrid)
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    sqrsqr.x = zrsqr.BigSqr();
	    sqrsqr.y = zisqr.BigSqr();
	    RealImagSqr = zrsqr * zisqr;
	    temp = (sqrsqr.x * 5.0 - RealImagSqr * 10.0 + sqrsqr.y) * Z->y;
	    Z->y = -temp.BigAbs() + centre->y;
	    Z->x = (sqrsqr.x - RealImagSqr * 10.0 + sqrsqr.y * 5.0) * Z->x.BigAbs() + centre->x;
	    break;
	case 48:							// 5th Quasi Perpendicular
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    sqrsqr.x = zrsqr.BigSqr();
	    sqrsqr.y = zisqr.BigSqr();
	    RealImagSqr = zrsqr * zisqr;
	    temp = (sqrsqr.x * 5.0 - RealImagSqr * 10.0 + sqrsqr.y);
	    Z->y = -temp.BigAbs() * Z->y + centre->y;
	    Z->x = (sqrsqr.x - RealImagSqr * 10.0 + sqrsqr.y * 5.0) * Z->x.BigAbs() + centre->x;
	    break;
	case 49:							// 5th Quasi Heart
	    zrsqr = Z->x.BigSqr();
	    zisqr = Z->y.BigSqr();
	    sqrsqr.x = zrsqr.BigSqr();
	    sqrsqr.y = zisqr.BigSqr();
	    RealImagSqr = zrsqr * zisqr;
	    temp = (sqrsqr.x * 5.0 - RealImagSqr * 10.0 + sqrsqr.y);
	    Z->y = temp.BigAbs() * Z->y + centre->y;
	    Z->x = (sqrsqr.x - RealImagSqr * 10.0 + sqrsqr.y * 5.0) * Z->x.BigAbs() + centre->x;
	    break;
	case 50:							// SimonBrot
	    zabsBig.x = Z->x.BigAbs();
	    zabsBig.y = Z->y.BigAbs();
	    tempzBig.y = Z->y * zabsBig.x + Z->x * zabsBig.y;
	    tempzBig.x = Z->x * zabsBig.x - Z->y * zabsBig.y;
	    *Z = tempzBig.CPolynomial(2) + *centre;
	    break;
	case 51:							// Cubic SimonBrot
	    zabsBig.x = Z->x.BigAbs();
	    zabsBig.y = Z->y.BigAbs();
	    tempzBig.y = Z->y * zabsBig.x + Z->x * zabsBig.y;
	    tempzBig.x = Z->x * zabsBig.x - Z->y * zabsBig.y;
	    *Z = tempzBig.CPolynomial(3) + *centre;
	    break;
	case 52:							// SimonBrot2 4th
	    tempzBig = *Z * *Z;
	    zabsBig.x = tempzBig.x.BigAbs();
	    zabsBig.y = tempzBig.y.BigAbs();
	    *Z = *Z * *Z * zabsBig;
	    Z->y = Z->y + centre->y;
	    Z->x = Z->x + centre->x;
	    break;
	case 53:							// TheRedshiftRider 1: a*z^2+z^3+c
	    aBig = rsrA;	// convert complex -> big complex
	    *Z = aBig * *Z * *Z + Z->CPolynomial(power) * ((rsrSign) ? 1.0 : -1.0);
	    *Z = *Z + *centre;
	    break;
	case 54:							// HPDZ Buffalo
	    tempzBig.x = Z->x.BigAbs();
	    tempzBig.y = Z->y.BigAbs();
	    *Z = tempzBig * tempzBig - tempzBig;
	    Z->y += centre->y;
	    Z->x += centre->x;
	    break;
#endif // ALLOW_ALL_DERIVATIVES

	default:
	    //	    Z = Z.CSqr() + centre;
	    square_bf(SqrReal, Z->x);
	    square_bf(SqrImag, Z->y);
	    sub_bf(TempReal, SqrReal, SqrImag);
	    add_bf(Z->x, TempReal, centre->x);
	    mult_bf(RealImag, ZTimes2->x, Z->y);
	    add_bf(Z->y, RealImag, centre->y);
	    break;
	}
    restore_stack(cplxsaved);
    }

//////////////////////////////////////////////////////////////////////
// Reference Zoom Point Functions
// Code is written in raw MPFR code to optimise speedy
//////////////////////////////////////////////////////////////////////

void CPertEngine::RefFunctions(Complex *centre, Complex *Z, Complex *ZTimes2)
    {
    double	TempReal, TempImag, SqrReal, SqrImag, RealImag;
    double	zisqr, zrsqr, realimag, temp, RealImagSqr;
//    Complex sqrsqr, zabsBig, tempzBig, sqrtzBig;
//    Complex aBig;
    Complex z;

    switch (subtype)
	{
	case 0:							// optimise for Mandelbrot by taking out as many steps as possible
	    //	    Z = Z.CSqr() + centre;
	    SqrReal = sqr(Z->x);
        SqrImag = sqr(Z->y);
        TempReal = SqrReal - SqrImag;
        Z->x = TempReal + centre->x;
        RealImag = ZTimes2->x * Z->y;
        Z->y = RealImag + centre->y;
	    break;
	case 1:
	    if (power == 3)
		    *Z = Z->CCube() + *centre;			// optimise for Cubic by taking out as many multiplies as possible
	    else
		    {
		    Complex ComplexTemp = *Z;
		    for (int k = 0; k < power - 1; k++)
		        ComplexTemp *= *Z;
		    *Z = ComplexTemp + *centre;
		    }
	    break;
	case 2:							// Burning Ship
	    SqrReal = sqr(Z->x);
        SqrImag = sqr(Z->y);
        TempReal = SqrReal - SqrImag;
	    Z->x = TempReal + centre->x;
        TempImag = ZTimes2->x * Z->y;
        RealImag = fabs(TempImag);
        Z->y = RealImag + centre->y;
	    break;
	case 3:							// Cubic Burning Ship
	case 4:							// 4th Power Burning Ship
	case 5:							// 5th Power Burning Ship
        z.x = fabs(Z->x);
        z.y = -fabs(Z->y);
        z = z.CPolynomial(power);
        Z->x = z.x + centre->x;
        Z->y = z.y + centre->y;
	    break;
	case 6:							// Celtic
        SqrReal = sqr(Z->x);
        SqrImag = sqr(Z->y);
        RealImag = ZTimes2->x * Z->y;
        Z->y = RealImag + centre->y;
        Z->x = fabs(SqrReal - SqrImag) + centre->x;
        break;
	case 7:							// Cubic Celtic
        z = Z->CPolynomial(3);
        Z->x = fabs(z.x) + centre->x;
        Z->y = z.y + centre->y;
	    break;
	case 8:							// 4th Celtic Buffalo
        z = Z->CPolynomial(4);
        Z->x = fabs(z.x) + centre->x;
        Z->y = z.y + centre->y;
	    break;
	case 9:							// 5th Celtic
        z = Z->CPolynomial(5);
        Z->x = fabs(z.x) + centre->x;
        Z->y = z.y + centre->y;
	    break;
	case 10:						// Mandelbar (Tricorn)
        SqrReal = sqr(Z->x);
        SqrImag = sqr(Z->y);
        RealImag = Z->x * ZTimes2->y;
        Z->x = SqrReal - SqrImag + centre->x;
        Z->y = -RealImag + centre->y;
	    break;
	case 11:						// Mandelbar (power)
        z = Z->CPolynomial(power);
        Z->x = z.x + centre->x;
        Z->y = -z.y + centre->y;
	    break;
        }
    }

//////////////////////////////////////////////////////////////////////
// Generate Pascal's Triangle coefficients
//////////////////////////////////////////////////////////////////////

void CPertEngine::LoadPascal(long PascalArray[], int n)
    {
    long    j, c = 1L;

    for (j = 0; j <= n; j++)
	{
	if (j == 0)
	    c = 1;
	else
	    c = c * (n - j + 1) / j;
	PascalArray[j] = c;
	}
    }

//////////////////////////////////////////////////////////////////////
// Laser Blaster's Code for removing absolutes from Mandelbrot derivatives
//////////////////////////////////////////////////////////////////////

double CPertEngine::DiffAbs(const double c, const double d)
    {
    double cd = c + d;

    if (c >= 0.0)
	{
	    if (cd >= 0.0)
	        return d;
	    else
	        return -d - 2.0 * c;
	    }
    else
	    {
	    if (cd > 0.0)
	        return d + 2.0 * c;
	    else
	        return -d;
	    }
    }

/**************************************************************************
	Evaluate a Complex Polynomial
**************************************************************************/

void CPertEngine::CPolynomial(BFComplex *out, BFComplex in, int degree)

    {
    bf_t	t, t1, t2, t3, t4;
    int cplxsaved;

    cplxsaved = save_stack();
    t = alloc_stack(g_r_bf_length + 2);
    t1 = alloc_stack(g_r_bf_length + 2);
    t2 = alloc_stack(g_r_bf_length + 2);
    t3 = alloc_stack(g_r_bf_length + 2);
    t4 = alloc_stack(g_r_bf_length + 2);

    if (degree < 0)
	    degree = 0;

    copy_bf(t1, in.x);	  		        // BigTemp1 = xt  
    copy_bf(t2, in.y);                  // BigTemp2 = yt

    if (degree & 1)
	    {
        copy_bf(out->x, t1);            // new.x = result real  
	    copy_bf(out->y, t2);            // new.y = result imag
	    }
    else
	    {
        inttobf(out->x, 1);
	    inttobf(out->y, 0);
	    }

    degree >>= 1;
    while (degree)
	    {
	    sub_bf(t, t1, t2);				// (xt - yt)
	    add_bf(t3, t1, t2);				// (xt + yt)
	    mult_bf(t4, t, t3);				// t2 = (xt + yt) * (xt - yt)
	    copy_bf(t, t2);
	    mult_bf(t3, t, t1);				// yt = xt * yt
	    add_bf(t2, t3, t3);				// yt = yt + yt
	    copy_bf(t1, t4);

	    if (degree & 1)
	        {
	        mult_bf(t, t1, out->x);			// xt * result->x
	        mult_bf(t3, t2, out->y);			// yt * result->y
	        sub_bf(t4, t, t3);				// t2 = xt * result->x - yt * result->y
	        mult_bf(t, t1, out->y);			// xt * result->y
	        mult_bf(t3, t2, out->x);			// yt * result->x
	        add_bf(out->y, t, t3);			// result->y = result->y * xt + yt * result->x
	        copy_bf(out->x, t4);    			// result->x = t2
	        }
	    degree >>= 1;
	    }
    restore_stack(cplxsaved);
    }

/**************************************************************************
	Cube c + jd = (a + jb) * (a + jb) * (a + jb) 
***************************************************************************/

void CPertEngine::CCube(BFComplex *out, BFComplex in)

    {
    bf_t	t, t1, t2, sqr_real, sqr_imag;

    int cplxsaved;

    cplxsaved = save_stack();
    t = alloc_stack(g_r_bf_length + 2);
    t1 = alloc_stack(g_r_bf_length + 2);
    t2 = alloc_stack(g_r_bf_length + 2);
    sqr_real = alloc_stack(g_r_bf_length + 2);
    sqr_imag = alloc_stack(g_r_bf_length + 2);

    mult_bf(sqr_real, in.x, in.x);          // sqr_real = x * x;
    mult_bf(sqr_imag, in.y, in.y);          // sqr_imag = y * y;
    inttobf(t, 3);
    mult_bf(t1, t, sqr_imag);               // sqr_real + sqr_real + sqr_real
    sub_bf(t2, sqr_real, t1);		        // sqr_real - (sqr_imag + sqr_imag + sqr_imag)
    mult_bf(out->x, in.x, t2);              // c = x * (sqr_real - (sqr_imag + sqr_imag + sqr_imag))

    mult_bf(t1, t, sqr_real);               // sqr_imag + sqr_imag + sqr_imag
    sub_bf(t2, t1, sqr_imag);               // (sqr_real + sqr_real + sqr_real) - sqr_imag
    mult_bf(out->y, in.y, t2);              // d = y * ((sqr_real + sqr_real + sqr_real) - sqr_imag)
    restore_stack(cplxsaved);
    }


