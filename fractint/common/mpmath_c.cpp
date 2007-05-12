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


/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"

#if !defined(XFRACT)

static struct MP s_answer = { 0 };

struct MPC g_one_mpc =
{
	{ 0x3fff, 0x80000000L },
    { 0, 0L }
};

struct MP *MPsub(struct MP x, struct MP y)
{
	y.Exp ^= 0x8000;
	return MPadd(x, y);
}

struct MP *MPabs(struct MP x)
{
	s_answer = x;
	s_answer.Exp &= 0x7fff;
	return &s_answer;
}

struct MPC MPCsqr(struct MPC x)
{
	struct MPC z;

	z.x = *MPsub(*MPmul(x.x, x.x), *MPmul(x.y, x.y));
	z.y = *MPmul(x.x, x.y);
	z.y.Exp++;
	return z;
}

struct MP MPCmod(struct MPC x)
{
	return *MPadd(*MPmul(x.x, x.x), *MPmul(x.y, x.y));
}

struct MPC MPCmul(struct MPC x, struct MPC y)
{
	struct MPC z;

	z.x = *MPsub(*MPmul(x.x, y.x), *MPmul(x.y, y.y));
	z.y = *MPadd(*MPmul(x.x, y.y), *MPmul(x.y, y.x));
	return z;
}

struct MPC MPCdiv(struct MPC x, struct MPC y)
{
	struct MP mod;

	mod = MPCmod(y);
	y.y.Exp ^= 0x8000;
	y.x = *MPdiv(y.x, mod);
	y.y = *MPdiv(y.y, mod);
	return MPCmul(x, y);
}

struct MPC MPCadd(struct MPC x, struct MPC y)
{
	struct MPC z;

	z.x = *MPadd(x.x, y.x);
	z.y = *MPadd(x.y, y.y);
	return z;
}

struct MPC MPCsub(struct MPC x, struct MPC y)
{
	struct MPC z;

	z.x = *MPsub(x.x, y.x);
	z.y = *MPsub(x.y, y.y);
	return z;
}

struct MPC MPCpow(struct MPC x, int exp)
{
	struct MPC z;
	struct MPC zz;

	z = (exp & 1) ? x : g_one_mpc;
	exp >>= 1;
	while (exp)
	{
		zz.x = *MPsub(*MPmul(x.x, x.x), *MPmul(x.y, x.y));
		zz.y = *MPmul(x.x, x.y);
		zz.y.Exp++;
		x = zz;
		if (exp & 1)
		{
			zz.x = *MPsub(*MPmul(z.x, x.x), *MPmul(z.y, x.y));
			zz.y = *MPadd(*MPmul(z.x, x.y), *MPmul(z.y, x.x));
			z = zz;
		}
		exp >>= 1;
	}
	return z;
}

int MPCcmp(struct MPC x, struct MPC y)
{
	struct MPC z;

	if (MPcmp(x.x, y.x) || MPcmp(x.y, y.y))
	{
		z.x = MPCmod(x);
		z.y = MPCmod(y);
		return MPcmp(z.x, z.y);
	}
	else
	{
		return 0;
	}
}

DComplex MPC2cmplx(struct MPC x)
{
	DComplex z;

	z.x = *MP2d(x.x);
	z.y = *MP2d(x.y);
	return z;
}

struct MPC cmplx2MPC(DComplex z)
{
	struct MPC x;

	x.x = *d2MP(z.x);
	x.y = *d2MP(z.y);
	return x;
}

#endif /* XFRACT */

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

DComplex ComplexPower(DComplex xx, DComplex yy)
{
	DComplex z, cLog, t;

	/* fixes power bug - if any complaints, backwards compatibility hook
		goes here TIW 3/95 */
	if (!g_use_old_complex_power)
	{
		if (xx.x == 0 && xx.y == 0)
		{
			z.x = z.y = 0.0;
			return z;
		}
	}

	FPUcplxlog(&xx, &cLog);
	FPUcplxmul(&cLog, &yy, &t);
	FPUcplxexp387(&t, &z);
	return z;
}

#define Sqrtz(z, rz) (*(rz) = ComplexSqrtFloat((z).x, (z).y))

/* rz=Arcsin(z)=-i*Log{i*z + sqrt(1-z*z)} */
void Arcsinz(DComplex z, DComplex *rz)
{
	DComplex tempz1;
	DComplex tempz2;

	FPUcplxmul(&z, &z, &tempz1);
	tempz1.x = 1 - tempz1.x;
	tempz1.y = -tempz1.y;			/* tempz1 = 1 - tempz1 */
	Sqrtz(tempz1, &tempz1);

	tempz2.x = -z.y; tempz2.y = z.x;                /* tempz2 = i*z  */
	tempz1.x += tempz2.x;
	tempz1.y += tempz2.y;    /* tempz1 += tempz2 */
	FPUcplxlog(&tempz1, &tempz1);
	rz->x = tempz1.y;
	rz->y = -tempz1.x;           /* rz = (-i)*tempz1 */
}   /* end. Arcsinz */


/* rz=Arccos(z)=-i*Log{z + sqrt(z*z-1)} */
void Arccosz(DComplex z, DComplex *rz)
{
	DComplex temp;

	FPUcplxmul(&z, &z, &temp);
	temp.x -= 1;
	Sqrtz(temp, &temp);

	temp.x += z.x;
	temp.y += z.y;

	FPUcplxlog(&temp, &temp);
	rz->x = temp.y;
	rz->y = -temp.x;              /* rz = (-i)*tempz1 */
}

void Arcsinhz(DComplex z, DComplex *rz)
{
	DComplex temp;

	FPUcplxmul(&z, &z, &temp);
	temp.x += 1;                                 /* temp = temp + 1 */
	Sqrtz(temp, &temp);
	temp.x += z.x;
	temp.y += z.y;                /* temp = z + temp */
	FPUcplxlog(&temp, rz);
}  /* end. Arcsinhz */

/* rz=Arccosh(z)=Log(z + sqrt(z*z-1)} */
void Arccoshz(DComplex z, DComplex *rz)
{
	DComplex tempz;
	FPUcplxmul(&z, &z, &tempz);
	tempz.x -= 1;                              /* tempz = tempz - 1 */
	Sqrtz(tempz, &tempz);
	tempz.x = z.x + tempz.x;
	tempz.y = z.y + tempz.y;  /* tempz = z + tempz */
	FPUcplxlog(&tempz, rz);
}   /* end. Arccoshz */

/* rz=Arctanh(z)=1/2*Log{(1 + z)/(1-z)} */
void Arctanhz(DComplex z, DComplex *rz)
{
	DComplex temp0, temp1, temp2;

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
			temp0.y = z.y;             /* temp0 = 1 + z */
			temp1.x = 1 - z.x;
			temp1.y = -z.y;            /* temp1 = 1 - z */
			FPUcplxdiv(&temp0, &temp1, &temp2);
			FPUcplxlog(&temp2, &temp2);
			rz->x = .5*temp2.x;
			rz->y = .5*temp2.y;       /* rz = .5*temp2 */
			return;
		}
	}
}   /* end. Arctanhz */

/* rz = Arctan(z) = i/2*Log{(1-i*z)/(1 + i*z)} */
void Arctanz(DComplex z, DComplex *rz)
{
	DComplex temp0, temp1, temp2, temp3;
	if (z.x == 0.0 && z.y == 0.0)
	{
		rz->x = rz->y = 0;
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
		rz->y = temp0.x;              /* i*temp0 */
	}
	else if (z.x != 0.0 && z.y != 0.0)
	{
		temp0.x = -z.y;
		temp0.y = z.x;                  /* i*z */
		temp1.x = 1 - temp0.x;
		temp1.y = -temp0.y;      /* temp1 = 1 - temp0 */
		temp2.x = 1 + temp0.x;
		temp2.y = temp0.y;       /* temp2 = 1 + temp0 */

		FPUcplxdiv(&temp1, &temp2, &temp3);
		FPUcplxlog(&temp3, &temp3);
		rz->x = -temp3.y*.5;
		rz->y = .5*temp3.x;           /* .5*i*temp0 */
	}
}   /* end. Arctanz */

#define SinCosFudge 0x10000L
#ifdef LONGSQRT
long lsqrt(long f)
{
	int N;
	unsigned long y0, z;
	static long a = 0, b = 0, c = 0;                  /* constant factors */

	if (f == 0)
	{
		return f;
	}
	if (f <  0)
	{
		return 0;
	}

	if (a == 0)                                   /* one-time compute consts */
	{
		a = (long)(g_fudge*.41731);
		b = (long)(g_fudge*.59016);
		c = (long)(g_fudge*.7071067811);
	}

	N  = 0;
	while (f & 0xff000000L)                     /* shift arg f into the */
	{                                           /* range: 0.5 <= f < 1  */
		N++;
		f /= 2;
	}
	while (!(f & 0xff800000L))
	{
		N--;
		f *= 2;
	}

	y0 = a + multiply(b, f,  g_bit_shift);         /* Newton's approximation */

	z  = y0 + divide (f, y0, g_bit_shift);
	y0 = (z >> 2) + divide(f, z,  g_bit_shift);

	if (N % 2)
	{
		N++;
		y0 = multiply(c, y0, g_bit_shift);
	}
	N /= 2;
	return (N >= 0) ? (y0 << N) : (y0 >> -N); /* correct for shift above */
}
#endif
LComplex ComplexSqrtLong(long x, long y)
{
	double    mag, theta;
	long      maglong, thetalong;
	LComplex    result;

#ifndef LONGSQRT
	mag       = sqrt(sqrt(((double) multiply(x, x, g_bit_shift))/g_fudge +
						((double) multiply(y, y, g_bit_shift))/ g_fudge));
	maglong   = (long)(mag*g_fudge);
#else
	maglong   = lsqrt(lsqrt(multiply(x, x, g_bit_shift) + multiply(y, y, g_bit_shift)));
#endif
	theta     = atan2((double) y/g_fudge, (double) x/g_fudge)/2;
	thetalong = (long)(theta*SinCosFudge);
	SinCos086(thetalong, &result.y, &result.x);
	result.x  = multiply(result.x << (g_bit_shift - 16), maglong, g_bit_shift);
	result.y  = multiply(result.y << (g_bit_shift - 16), maglong, g_bit_shift);
	return result;
}

DComplex ComplexSqrtFloat(double x, double y)
{
	double mag;
	double theta;
	DComplex  result;

	if (x == 0.0 && y == 0.0)
	{
		result.x = result.y = 0.0;
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

BYTE *g_log_table = (BYTE *)0;
long g_max_log_table_size;
int  g_log_calculation = 0;
static double mlf;
static unsigned long lf;

/*
	g_log_palette_flag == 1  -- standard log palettes
	g_log_palette_flag == -1 -- 'old' log palettes
	g_log_palette_flag >  1  -- compress counts < g_log_palette_flag into color #1
	g_log_palette_flag < -1  -- use quadratic palettes based on square roots && compress
*/

void SetupLogTable()
{
	float l, f, c, m;
	unsigned long prev, limit, sptop;
	unsigned n;

	if (g_save_release > 1920 || g_log_dynamic_calculate == LOGDYNAMIC_DYNAMIC)  /* set up on-the-fly variables */
	{
		if (g_log_palette_flag > LOGPALETTE_NONE)  /* new log function */
		{
			lf = (g_log_palette_flag > LOGPALETTE_STANDARD) ? g_log_palette_flag : 0;
			if (lf >= (unsigned long)g_max_log_table_size)
			{
				lf = g_max_log_table_size - 1;
			}
			mlf = (g_colors - (lf ? 2 : 1 ))/log((double) (g_max_log_table_size - lf));
		}
		else if (g_log_palette_flag == LOGPALETTE_OLD)  /* old log function */
		{
			mlf = (g_colors - 1)/log((double) g_max_log_table_size);
		}
		else if (g_log_palette_flag <= -2)  /* sqrt function */
		{
			lf = -g_log_palette_flag;
			if (lf >= (unsigned long)g_max_log_table_size)
			{
				lf = g_max_log_table_size - 1;
			}
			mlf = (g_colors - 2)/sqrt((double) (g_max_log_table_size - lf));
		}
	}

	if (g_log_calculation)
	{
		return; /* g_log_table not defined, bail out now */
	}

	if (g_save_release > 1920 && !g_log_calculation)
	{
		g_log_calculation = 1;   /* turn it on */
		for (prev = 0; prev <= (unsigned long)g_max_log_table_size; prev++)
		{
			g_log_table[prev] = (BYTE)logtablecalc((long)prev);
		}
		g_log_calculation = 0;   /* turn it off, again */
		return;
	}

	if (g_log_palette_flag > -2)
	{
		lf = (g_log_palette_flag > LOGPALETTE_STANDARD) ? g_log_palette_flag : 0;
		if (lf >= (unsigned long)g_max_log_table_size)
		{
			lf = g_max_log_table_size - 1;
		}
		Fg2Float((long)(g_max_log_table_size-lf), 0, m);
		fLog14(m, m);
		Fg2Float((long)(g_colors - (lf ? 2 : 1)), 0, c);
		fDiv(m, c, m);
		for (prev = 1; prev <= lf; prev++)
		{
			g_log_table[prev] = 1;
		}
		for (n = (lf ? 2 : 1); n < (unsigned int)g_colors; n++)
		{
			Fg2Float((long)n, 0, f);
			fMul16(f, m, f);
			fExp14(f, l);
			limit = (unsigned long)Float2Fg(l, 0) + lf;
			if (limit > (unsigned long)g_max_log_table_size || n == (unsigned int)(g_colors-1))
			{
				limit = g_max_log_table_size;
			}
			while (prev <= limit)
			{
				g_log_table[prev++] = (BYTE)n;
			}
		}
	}
	else
	{
		lf = -g_log_palette_flag;
		if (lf >= (unsigned long)g_max_log_table_size)
		{
			lf = g_max_log_table_size - 1;
		}
		Fg2Float((long)(g_max_log_table_size-lf), 0, m);
		fSqrt14(m, m);
		Fg2Float((long)(g_colors-2), 0, c);
		fDiv(m, c, m);
		for (prev = 1; prev <= lf; prev++)
		{
			g_log_table[prev] = 1;
		}
		for (n = 2; n < (unsigned int)g_colors; n++)
		{
			Fg2Float((long)n, 0, f);
			fMul16(f, m, f);
			fMul16(f, f, l);
			limit = (unsigned long)(Float2Fg(l, 0) + lf);
			if (limit > (unsigned long)g_max_log_table_size || n == (unsigned int)(g_colors-1))
			{
				limit = g_max_log_table_size;
			}
			while (prev <= limit)
			{
				g_log_table[prev++] = (BYTE)n;
			}
		}
	}
	g_log_table[0] = 0;
	if (g_log_palette_flag != LOGPALETTE_OLD)
	{
		for (sptop = 1; sptop < (unsigned long)g_max_log_table_size; sptop++) /* spread top to incl unused g_colors */
		{
			if (g_log_table[sptop] > g_log_table[sptop-1])
			{
				g_log_table[sptop] = (BYTE)(g_log_table[sptop-1] + 1);
			}
		}
	}
}

long logtablecalc(long citer)
{
	long ret = 0;

	if (g_log_palette_flag == LOGPALETTE_NONE && !g_ranges_length) /* Oops, how did we get here? */
	{
		return citer;
	}
	if (g_log_table && !g_log_calculation)
	{
		return g_log_table[(long)min(citer, g_max_log_table_size)];
	}

	if (g_log_palette_flag > LOGPALETTE_NONE)  /* new log function */
	{
		if ((unsigned long)citer <= lf + 1)
		{
			ret = 1;
		}
		else if ((citer - lf)/log((double) (citer - lf)) <= mlf)
		{
			ret = (g_save_release < 2002) ? ((long) (citer - lf + (lf ? 1 : 0))) : ((long) (citer - lf));
		}
		else
		{
			ret = (long)(mlf*log((double) (citer - lf))) + 1;
		}
	}
	else if (g_log_palette_flag == LOGPALETTE_OLD)  /* old log function */
	{
		ret = (citer == 0) ? 1 : (long)(mlf*log(static_cast<double>(citer))) + 1;
	}
	else if (g_log_palette_flag <= -2)  /* sqrt function */
	{
		if ((unsigned long)citer <= lf)
		{
			ret = 1;
		}
		else if ((unsigned long)(citer - lf) <= (unsigned long)(mlf*mlf))
		{
			ret = (long)(citer - lf + 1);
		}
		else
		{
			ret = (long)(mlf*sqrt(static_cast<double>(citer - lf))) + 1;
		}
	}
	return ret;
}

long ExpFloat14(long xx)
{
	static float fLogTwo = 0.6931472f;
	int f = 23 - (int) RegFloat2Fg(RegDivFloat(xx, *(long*) &fLogTwo), 0);
	long answer = ExpFudged(RegFloat2Fg(xx, 16), f);
	return RegFg2Float(answer, (char) f);
}

double TwoPi;
DComplex temp, BaseLog;
DComplex cdegree = { 3.0, 0.0 };
DComplex croot   = { 1.0, 0.0 };

int complex_basin()
{
	DComplex cd1;
	double mod;

	/* new = ((cdegree-1)*old**cdegree) + croot
				----------------------------------
                 cdegree*old**(cdegree-1)         */

	cd1.x = cdegree.x - 1.0;
	cd1.y = cdegree.y;

	temp = ComplexPower(g_old_z, cd1);
	FPUcplxmul(&temp, &g_old_z, &g_new_z);

	g_temp_z.x = g_new_z.x - croot.x;
	g_temp_z.y = g_new_z.y - croot.y;
	if ((sqr(g_temp_z.x) + sqr(g_temp_z.y)) < g_threshold)
	{
		if (fabs(g_old_z.y) < .01)
		{
			g_old_z.y = 0.0;
		}
		FPUcplxlog(&g_old_z, &temp);
		FPUcplxmul(&temp, &cdegree, &g_temp_z);
		mod = g_temp_z.y/TwoPi;
		g_color_iter = (long)mod;
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

/*
 * Generate a gaussian distributed number.
 * The right half of the distribution is folded onto the lower half.
 * That is, the curve slopes up to the peak and then drops to 0.
 * The larger slope is, the smaller the standard deviation.
 * The values vary from 0 + offset to range + offset, with the peak
 * at range + offset.
 * To make this more complicated, you only have a
 * 1 in Distribution*(1-Probability/Range*con) + 1 chance of getting a
 * Gaussian; otherwise you just get offset.
 */
int gaussian_number(int probability, int range)
{
	long p = divide((long) probability << 16, (long) range << 16, 16);
	p = multiply(p, g_gaussian_constant, 16);
	p = multiply((long) g_gaussian_distribution << 16, p, 16);
	if (!(rand15() % (g_gaussian_distribution - (int) (p >> 16) + 1)))
	{
		long accum = 0;
		for (int n = 0; n < g_gaussian_slope; n++)
		{
			accum += rand15();
		}
		accum /= g_gaussian_slope;
		int r = (int) (multiply((long) range << 15, accum, 15) >> 14);
		r = r - range;
		if (r < 0)
		{
			r = -r;
		}
		return range - r + g_gaussian_offset;
	}
	return g_gaussian_offset;
}

#endif

#if defined(_WIN32)
/*
d2MP386     PROC     uses si di, x:QWORD
	mov   si, WORD PTR [x + 6]
.386
	mov   edx, DWORD PTR [x + 4]
	mov   eax, DWORD PTR [x]

	mov   ebx, edx
	shl   ebx, 1
	or    ebx, eax
	jnz   NonZero

	xor   si, si
	xor   edx, edx
	jmp   StoreAns

NonZero:
	shl   si, 1
	pushf
	shr   si, 4
	popf
	rcr   si, 1
	add   si, (1 SHL 14) - (1 SHL 10)

	shld  edx, eax, 12
	stc
	rcr   edx, 1

StoreAns:
	mov   s_answer.Exp, si
	mov   s_answer.Mant, edx

	lea   ax, s_answer
	mov   dx, ds
.8086
	ret
d2MP386     ENDP
*/
struct MP *d2MP(double x)
{
	/* TODO: implement */
#if defined(_WIN32)
	_ASSERTE(0 && "d2MP called.");
#endif
	return &s_answer;
}

/*
MP2d386     PROC     uses si di, xExp:WORD, xMant:DWORD
	sub   xExp, (1 SHL 14) - (1 SHL 10)
.386
	jo    Overflow

	mov   bx, xExp
	and   bx, 0111100000000000b
	jz    InRangeOfDouble

Overflow:
	mov   g_overflow_mp, 1
	xor   eax, eax
	xor   edx, edx
	jmp   StoreAns

InRangeOfDouble:
	mov   bx, xExp
	mov   ax, bx
	shl   bx, 5
	shl   ax, 1
	rcr   bx, 1
	shr   bx, 4

	mov   edx, xMant
	shl   edx, 1
	xor   eax, eax
	shrd  eax, edx, 12
	shrd  edx, ebx, 12

StoreAns:
	mov   DWORD PTR Double + 4, edx
	mov   DWORD PTR Double, eax

	lea   ax, Double
	mov   dx, ds
.8086
	ret
MP2d386     ENDP
*/
double *MP2d(struct MP x)
{
	/* TODO: implement */
	static double ans = 0.0;
#if defined(_WIN32)
	_ASSERTE(0 && "MP2d called.");
#endif
	return &ans;
}

/*
MPadd386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
							yMant:DWORD
	mov   si, xExp
.386
	mov   eax, xMant

	mov   di, yExp
	mov   edx, yMant

	mov   cx, si
	xor   cx, di
	js    Subtract
	jz    SameMag

	cmp   si, di
	jg    XisGreater

	xchg  si, di
	xchg  eax, edx

XisGreater:
	mov   cx, si
	sub   cx, di
	cmp   cx, 32
	jge   StoreAns

	shr   edx, cl

SameMag:
	add   eax, edx
	jnc   StoreAns

	rcr   eax, 1
	add   si, 1
	jno   StoreAns

Overflow:
	mov   g_overflow_mp, 1

ZeroAns:
	xor   si, si
	xor   edx, edx
	jmp   StoreAns

Subtract:
	xor   di, 8000h
	mov   cx, si
	sub   cx, di
	jnz   DifferentMag

	cmp   eax, edx
	jg    SubtractNumbers
	je    ZeroAns

	xor   si, 8000h
	xchg  eax, edx
	jmp   SubtractNumbers

DifferentMag:
	or    cx, cx
	jns   NoSwap

	xchg  si, di
	xchg  eax, edx
	xor   si, 8000h
	neg   cx

NoSwap:
	cmp   cx, 32
	jge   StoreAns

	shr   edx, cl

SubtractNumbers:
	sub   eax, edx
	bsr   ecx, eax
	neg   cx
	add   cx, 31
	shl   eax, cl
	sub   si, cx
	jo    Overflow

StoreAns:
	mov   s_answer.Exp, si
	mov   s_answer.Mant, eax

	lea   ax, s_answer
	mov   dx, ds
.8086
	ret
MPadd386    ENDP
*/
struct MP *MPadd(struct MP x, struct MP y)
{
	/* TODO: implement */
#if defined(_WIN32)
	_ASSERTE(0 && "MPadd called.");
#endif
	return &s_answer;
}

/*
MPcmp386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
	mov   si, 0
	mov   di, si
.386
	mov   ax, xExp
	mov   edx, xMant
	mov   bx, yExp
	mov   ecx, yMant
	or    ax, ax
	jns   AtLeastOnePos

	or    bx, bx
	jns   AtLeastOnePos

	mov   si, 1
	and   ah, 7fh
	and   bh, 7fh

AtLeastOnePos:
	cmp   ax, bx
	jle   Cmp1

	mov   di, 1
	jmp   ChkRev

Cmp1:
	je    Cmp2

	mov   di, -1
	jmp   ChkRev

Cmp2:
	cmp   edx, ecx
	jbe   Cmp3

	mov   di, 1
	jmp   ChkRev

Cmp3:
	je    ChkRev

	mov   di, -1

ChkRev:
		or    si, si
	jz    ExitCmp

	neg   di

ExitCmp:
	mov   ax, di
.8086
	ret
MPcmp386    ENDP
*/
int MPcmp(struct MP x, struct MP y)
{
	/* TODO: implement */
#if defined(_WIN32)
	_ASSERTE(0 && "MPcmp called.");
#endif
	return 0;
}

/*
MPdiv386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
							yMant:DWORD
	mov   ax, xExp
	mov   bx, yExp
.386
	xor   ch, ch
	shl   bh, 1
	rcr   ch, 1
	shr   bh, 1
	xor   ah, ch

	sub   bx, (1 SHL 14) - 2
	sub   ax, bx
	jno   NoOverflow

Overflow:
	mov   g_overflow_mp, 1

ZeroAns:
	xor   eax, eax
	mov   s_answer.Exp, ax
	jmp   StoreMant

NoOverflow:
	mov   s_answer.Exp, ax

	xor   eax, eax
	mov   edx, xMant
	mov   ecx, yMant
	or    edx, edx
	jz    ZeroAns

	or    ecx, ecx
	jz    Overflow

	cmp   edx, ecx
	jl    Divide

	shr   edx, 1
	rcr   eax, 1
	add   s_answer.Exp, 1
	jo    Overflow

Divide:
	div   ecx

StoreMant:
	mov   s_answer.Mant, eax
	lea   ax, s_answer
	mov   dx, ds
.8086
	ret
MPdiv386    ENDP
*/
struct MP *MPdiv(struct MP x, struct MP y)
{
	/* TODO: implement */
#if defined(_WIN32)
	_ASSERTE(0 && "MPdiv called.");
#endif
	return &s_answer;
}

/*
MPmul386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
							yMant:DWORD
	mov   ax, xExp
	mov   bx, yExp
.386
	xor   ch, ch
	shl   bh, 1
	rcr   ch, 1
	shr   bh, 1
	xor   ah, ch

	sub   bx, (1 SHL 14) - 2
	add   ax, bx
	jno   NoOverflow

Overflow:
	or    WORD PTR [xMant + 2], 0
	jz    ZeroAns
	or    WORD PTR [yMant + 2], 0
	jz    ZeroAns

	mov   g_overflow_mp, 1

ZeroAns:
	xor   edx, edx
	mov   s_answer.Exp, dx
	jmp   StoreMant

NoOverflow:
	mov   s_answer.Exp, ax

	mov   eax, xMant
	mov   edx, yMant
	or    eax, eax
	jz    ZeroAns

	or    edx, edx
	jz    ZeroAns

	mul   edx

	or    edx, edx
	js    StoreMant

	shld  edx, eax, 1
	sub   s_answer.Exp, 1
	jo    Overflow

StoreMant:
	mov   s_answer.Mant, edx
	lea   ax, s_answer
	mov   dx, ds
.8086
	ret
MPmul386    ENDP
*/
struct MP *MPmul(struct MP x, struct MP y)
{
	/* TODO: implement */
#if defined(_WIN32)
	_ASSERTE(0 && "MPmul386 called.");
#endif
	return &s_answer;
}

/*
fg2MP386    PROC     x:DWORD, fg:WORD
	mov   bx, 1 SHL 14 + 30
	sub   bx, fg
.386
	mov   edx, x

	or    edx, edx
	jnz   ChkNegMP

	mov   bx, dx
	jmp   StoreAns

ChkNegMP:
	jns   BitScanRight

	or    bh, 80h
	neg   edx

BitScanRight:
	bsr   ecx, edx
	neg   cx
	add   cx, 31
	sub   bx, cx
	shl   edx, cl

StoreAns:
	mov   s_answer.Exp, bx
	mov   s_answer.Mant, edx
.8086
	lea   ax, s_answer
	mov   dx, ds
	ret
fg2MP386    ENDP
*/
struct MP *fg2MP(long x, int fg)
{
#if defined(_WIN32)
	_ASSERTE(0 && "fg2MP called");
#endif
	return &s_answer;
}

#endif
