/* MPMath_c.c (C) 1989, Mark C. Peterson, CompuServe [70441, 3353]
     All rights reserved.

	Code may be used in any program provided the author is credited
     either during program execution or in the documentation.  Source
     code may be distributed only in combination with public domain or
     shareware source code.  Source code may be modified provided the
     copyright notice and this message is left unchanged and all
     modifications are clearly documented.

     I would appreciate a copy of any work which incorporates this code,
     however this is optional.

     Mark C. Peterson
     405-C Queen St. Suite #181
     Southington, CT 06489
     (203) 276-9721
*/
#include <algorithm>
#include <string>

#include "port.h"
#include "prototyp.h"
#include "fpu.h"
#include "jiim.h"
#include "mpmath.h"

ComplexD ComplexPower(ComplexD const &xx, ComplexD const &yy)
{
	ComplexD z;
	ComplexD cLog;
	ComplexD t;

	/* fixes power bug - if any complaints, backwards compatibility hook
		goes here TIW 3/95 */
	if (!g_use_old_complex_power)
	{
		if (xx.x == 0 && xx.y == 0)
		{
			z.x = 0.0;
			z.y = 0.0;
			return z;
		}
	}

	FPUcplxlog(&xx, &cLog);
	FPUcplxmul(&cLog, &yy, &t);
	FPUcplxexp387(&t, &z);
	return z;
}

inline void Sqrtz(ComplexD z, ComplexD *rz)
{
	(*(rz) = ComplexSqrtFloat((z).x, (z).y));
}

// rz=Arcsin(z)=-i*Log{i*z + sqrt(1-z*z)}
void Arcsinz(ComplexD z, ComplexD *rz)
{
	ComplexD tempz1;
	ComplexD tempz2;

	FPUcplxmul(&z, &z, &tempz1);
	tempz1.x = 1 - tempz1.x;
	tempz1.y = -tempz1.y;			// tempz1 = 1 - tempz1
	Sqrtz(tempz1, &tempz1);

	tempz2.x = -z.y;
	tempz2.y = z.x;                // tempz2 = i*z
	tempz1.x += tempz2.x;
	tempz1.y += tempz2.y;    // tempz1 += tempz2
	FPUcplxlog(&tempz1, &tempz1);
	rz->x = tempz1.y;
	rz->y = -tempz1.x;           // rz = (-i)*tempz1
}   // end. Arcsinz


// rz=Arccos(z)=-i*Log{z + sqrt(z*z-1)}
void Arccosz(ComplexD z, ComplexD *rz)
{
	ComplexD temp;

	FPUcplxmul(&z, &z, &temp);
	temp.x -= 1;
	Sqrtz(temp, &temp);

	temp.x += z.x;
	temp.y += z.y;

	FPUcplxlog(&temp, &temp);
	rz->x = temp.y;
	rz->y = -temp.x;              // rz = (-i)*tempz1
}

void Arcsinhz(ComplexD z, ComplexD *rz)
{
	ComplexD temp;

	FPUcplxmul(&z, &z, &temp);
	temp.x += 1;                                 // temp = temp + 1
	Sqrtz(temp, &temp);
	temp.x += z.x;
	temp.y += z.y;                // temp = z + temp
	FPUcplxlog(&temp, rz);
}  // end. Arcsinhz

// rz=Arccosh(z)=Log(z + sqrt(z*z-1)}
void Arccoshz(ComplexD z, ComplexD *rz)
{
	ComplexD tempz;
	FPUcplxmul(&z, &z, &tempz);
	tempz.x -= 1;                              // tempz = tempz - 1
	Sqrtz(tempz, &tempz);
	tempz.x = z.x + tempz.x;
	tempz.y = z.y + tempz.y;  // tempz = z + tempz
	FPUcplxlog(&tempz, rz);
}   // end. Arccoshz

// rz=Arctanh(z)=1/2*Log{(1 + z)/(1-z)}
void Arctanhz(ComplexD z, ComplexD *rz)
{
	ComplexD temp0;
	ComplexD temp1;
	ComplexD temp2;

	if (z.x == 0.0)
	{
		rz->x = 0;
		rz->y = atan(z.y);
		return;
	}
	else
	{
		if (fabs(z.x) == 1.0 && z.y == 0.0)
		{
			return;
		}
		else if (fabs(z.x) < 1.0 && z.y == 0.0)
		{
			rz->x = log((1 + z.x)/(1-z.x))/2;
			rz->y = 0;
			return;
		}
		else
		{
			temp0.x = 1 + z.x;
			temp0.y = z.y;             // temp0 = 1 + z
			temp1.x = 1 - z.x;
			temp1.y = -z.y;            // temp1 = 1 - z
			FPUcplxdiv(&temp0, &temp1, &temp2);
			FPUcplxlog(&temp2, &temp2);
			rz->x = .5*temp2.x;
			rz->y = .5*temp2.y;       // rz = .5*temp2
			return;
		}
	}
}   // end. Arctanhz

// rz = Arctan(z) = i/2*Log{(1-i*z)/(1 + i*z)}
void Arctanz(ComplexD z, ComplexD *rz)
{
	ComplexD temp0;
	ComplexD temp1;
	ComplexD temp2;
	ComplexD temp3;
	if (z.x == 0.0 && z.y == 0.0)
	{
		rz->x = 0;
		rz->y = 0;
	}
	else if (z.x != 0.0 && z.y == 0.0)
	{
		rz->x = atan(z.x);
		rz->y = 0;
	}
	else if (z.x == 0.0 && z.y != 0.0)
	{
		temp0.x = z.y;
		temp0.y = 0.0;
		Arctanhz(temp0, &temp0);
		rz->x = -temp0.y;
		rz->y = temp0.x;              // i*temp0
	}
	else if (z.x != 0.0 && z.y != 0.0)
	{
		temp0.x = -z.y;
		temp0.y = z.x;                  // i*z
		temp1.x = 1 - temp0.x;
		temp1.y = -temp0.y;      // temp1 = 1 - temp0
		temp2.x = 1 + temp0.x;
		temp2.y = temp0.y;       // temp2 = 1 + temp0

		FPUcplxdiv(&temp1, &temp2, &temp3);
		FPUcplxlog(&temp3, &temp3);
		rz->x = -temp3.y*.5;
		rz->y = .5*temp3.x;           // .5*i*temp0
	}
}   // end. Arctanz

static long const SinCosFudge = 0x10000L;

#ifdef LONGSQRT
long lsqrt(long f)
{
	int N;
	unsigned long y0, z;
	static long a = 0, b = 0, c = 0;                  // constant factors

	if (f == 0)
	{
		return f;
	}
	if (f <  0)
	{
		return 0;
	}

	if (a == 0)                                   // one-time compute consts
	{
		a = long(g_externs.Fudge()*.41731);
		b = long(g_externs.Fudge()*.59016);
		c = long(g_externs.Fudge()*.7071067811);
	}

	N  = 0;
	while (f & 0xff000000L)                     // shift arg f into the
	{                                           // range: 0.5 <= f < 1
		N++;
		f /= 2;
	}
	while (!(f & 0xff800000L))
	{
		N--;
		f *= 2;
	}

	y0 = a + multiply(b, f,  g_bit_shift);         // Newton's approximation

	z  = y0 + divide (f, y0, g_bit_shift);
	y0 = (z >> 2) + divide(f, z,  g_bit_shift);

	if (N % 2)
	{
		N++;
		y0 = multiply(c, y0, g_bit_shift);
	}
	N /= 2;
	return (N >= 0) ? (y0 << N) : (y0 >> -N); // correct for shift above
}
#endif
ComplexL ComplexSqrtLong(long x, long y)
{
	double mag;
	double theta;
	long maglong;
	long thetalong;
	ComplexL    result;

#ifndef LONGSQRT
	mag       = sqrt(sqrt(FudgeToDouble(multiply(x, x, g_bit_shift)) +
						  FudgeToDouble(multiply(y, y, g_bit_shift))));
	maglong   = DoubleToFudge(mag);
#else
	maglong   = lsqrt(lsqrt(multiply(x, x, g_bit_shift) + multiply(y, y, g_bit_shift)));
#endif
	theta     = atan2(FudgeToDouble(y), FudgeToDouble(x))/2;
	thetalong = long(theta*SinCosFudge);
	SinCos086(thetalong, &result.y, &result.x);
	result.x  = multiply(result.x << (g_bit_shift - 16), maglong, g_bit_shift);
	result.y  = multiply(result.y << (g_bit_shift - 16), maglong, g_bit_shift);
	return result;
}

ComplexD ComplexSqrtFloat(double x, double y)
{
	double mag;
	double theta;
	ComplexD  result;

	if (x == 0.0 && y == 0.0)
	{
		result.x = 0.0;
		result.y = 0.0;
	}
	else
	{
		mag   = sqrt(sqrt(x*x + y*y));
		theta = atan2(y, x)/2;
		FPUsincos(&theta, &result.y, &result.x);
		result.x *= mag;
		result.y *= mag;
	}
	return result;
}


/***** FRACTINT specific routines and variables *****/

#ifndef TESTING_MATH

BYTE *g_log_table = 0;
long g_max_log_table_size;
bool g_log_calculation = false;
static double mlf;
static unsigned long lf;

/*
	g_log_palette_mode == 1  -- standard log palettes
	g_log_palette_mode == -1 -- 'old' log palettes
	g_log_palette_mode >  1  -- compress counts < g_log_palette_mode into color #1
	g_log_palette_mode < -1  -- use quadratic palettes based on square roots && compress
*/

void SetupLogTable()
{
	unsigned long prev;

	if (g_log_dynamic_calculate == LOGDYNAMIC_DYNAMIC)  // set up on-the-fly variables
	{
		if (g_log_palette_mode > LOGPALETTE_NONE)  // new log function
		{
			lf = (g_log_palette_mode > LOGPALETTE_STANDARD) ? g_log_palette_mode : 0;
			if (lf >= (unsigned long)g_max_log_table_size)
			{
				lf = g_max_log_table_size - 1;
			}
			mlf = (g_colors - (lf ? 2 : 1 ))/log(double(g_max_log_table_size - lf));
		}
		else if (g_log_palette_mode == LOGPALETTE_OLD)  // old log function
		{
			mlf = (g_colors - 1)/log(double(g_max_log_table_size));
		}
		else if (g_log_palette_mode <= -2)  // sqrt function
		{
			lf = -g_log_palette_mode;
			if (lf >= (unsigned long)g_max_log_table_size)
			{
				lf = g_max_log_table_size - 1;
			}
			mlf = (g_colors - 2)/sqrt(double(g_max_log_table_size - lf));
		}
	}

	if (g_log_calculation)
	{
		return; // g_log_table not defined, bail out now
	}

	g_log_calculation = true;   // turn it on
	for (prev = 0; prev <= (unsigned long)g_max_log_table_size; prev++)
	{
		g_log_table[prev] = BYTE(logtablecalc(long(prev)));
	}
	g_log_calculation = false;   // turn it off, again
}

long logtablecalc(long citer)
{
	long ret = 0;

	if (g_log_palette_mode == LOGPALETTE_NONE && !g_ranges_length) // Oops, how did we get here?
	{
		return citer;
	}
	if (g_log_table && !g_log_calculation)
	{
		return g_log_table[std::min(citer, g_max_log_table_size)];
	}

	if (g_log_palette_mode > LOGPALETTE_NONE)  // new log function
	{
		if ((unsigned long)citer <= lf + 1)
		{
			ret = 1;
		}
		else if ((citer - lf)/log(double(citer - lf)) <= mlf)
		{
			ret = long(citer - lf);
		}
		else
		{
			ret = long(mlf*log(double(citer - lf))) + 1;
		}
	}
	else if (g_log_palette_mode == LOGPALETTE_OLD)  // old log function
	{
		ret = (citer == 0) ? 1 : long(mlf*log(double(citer))) + 1;
	}
	else if (g_log_palette_mode <= -2)  // sqrt function
	{
		if ((unsigned long)citer <= lf)
		{
			ret = 1;
		}
		else if ((unsigned long)(citer - lf) <= (unsigned long)(mlf*mlf))
		{
			ret = long(citer - lf + 1);
		}
		else
		{
			ret = long(mlf*sqrt(double(citer - lf))) + 1;
		}
	}
	return ret;
}

long ExpFloat14(long xx)
{
	static float fLogTwo = 0.6931472f;
	int f = 23 - int(RegFloat2Fg(RegDivFloat(xx, *(long*) &fLogTwo), 0));
	long answer = ExpFudged(RegFloat2Fg(xx, 16), f);
	return RegFg2Float(answer, (char) f);
}

double TwoPi;
ComplexD temp;
ComplexD BaseLog;
ComplexD cdegree = { 3.0, 0.0 };
ComplexD croot   = { 1.0, 0.0 };

int complex_basin()
{
	ComplexD cd1;
	double mod;

	/* new = ((cdegree-1)*old**cdegree) + croot
				----------------------------------
                 cdegree*old**(cdegree-1)         */

	cd1.x = cdegree.x - 1.0;
	cd1.y = cdegree.y;

	temp = ComplexPower(g_old_z, cd1);
	FPUcplxmul(&temp, &g_old_z, &g_new_z);

	g_temp_z.x = g_new_z.real() - croot.x;
	g_temp_z.y = g_new_z.imag() - croot.y;
	if ((sqr(g_temp_z.x) + sqr(g_temp_z.y)) < g_threshold)
	{
		if (fabs(g_old_z.imag()) < .01)
		{
			g_old_z.imag(0.0);
		}
		FPUcplxlog(&g_old_z, &temp);
		FPUcplxmul(&temp, &cdegree, &g_temp_z);
		mod = g_temp_z.y/TwoPi;
		g_color_iter = long(mod);
		if (fabs(mod - g_color_iter) > 0.5)
		{
			if (mod < 0.0)
			{
				g_color_iter--;
			}
			else
			{
				g_color_iter++;
			}
		}
		g_color_iter += 2;
		if (g_color_iter < 0)
		{
			g_color_iter += 128;
		}
		return 1;
	}

	FPUcplxmul(&g_new_z, &cd1, &g_temp_z);
	g_temp_z.x += croot.x;
	g_temp_z.y += croot.y;

	FPUcplxmul(&temp, &cdegree, &cd1);
	FPUcplxdiv(&g_temp_z, &cd1, &g_old_z);
	if (g_overflow)
	{
		return 1;
	}
	g_new_z = g_old_z;
	return 0;
}

#endif
