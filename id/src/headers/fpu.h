#if !defined(FPU_H)
#define FPU_H

extern int DivideOverflow;

inline float LongAsFloat(long l)
{
	return *reinterpret_cast<float *>(&l);
}
inline long FloatAsLong(float f)
{
	return *reinterpret_cast<long *>(&f);
}

inline void FPUsincos2(double angle, double *Sin, double *Cos)
{
	*Sin = std::sin(angle);
	*Cos = std::cos(angle);
}

inline void FPUsinhcosh(double angle, double *Sinh, double *Cosh)
{
	*Sinh = std::sinh(angle);
	*Cosh = std::cosh(angle);
}

inline void SinCos086(long x, long *sinx, long *cosx)
{
	double const angle = x/double(1 << 16);
	*sinx = long(std::sin(angle)*double(1 << 16));
	*cosx = long(std::cos(angle)*double(1 << 16));
}

inline void SinhCosh086(long x, long *sinx, long *cosx)
{
	double const angle = x/double(1 << 16);
	*sinx = long(std::sinh(angle)*double(1 << 16));
	*cosx = long(std::cosh(angle)*double(1 << 16));
}

inline long Exp086(long x)
{
	return long(std::exp(double(x)/double(1 << 16))*double(1 << 16));
}

/*
 * Input is a 16 bit offset number.  Output is shifted by Fudge.
 */
inline unsigned long ExpFudged(long x, int Fudge)
{
	return long(std::exp(double(x)/double(1 << 16))*double(1 << Fudge));
}

// This multiplies two e/m numbers and returns an e/m number.
inline long r16Mul(long x, long y)
{
	float f = LongAsFloat(x)*LongAsFloat(y);
	return FloatAsLong(f);
}

// This takes an exp/mant number and returns a shift-16 number
inline long LogFloat14(unsigned long x)
{
	return long(std::log(double(LongAsFloat(x))))*(1 << 16);
}

// This divides two e/m numbers and returns an e/m number.
inline long RegDivFloat(long x, long y)
{
	float f = LongAsFloat(x)/LongAsFloat(y);
	return FloatAsLong(f);
}

/*
 * This routine on the IBM converts shifted integer x, FudgeFact to
 * the 4 byte number: exp, mant, mant, mant
 * Instead of using exp/mant format, we'll just use floats.
 * Note: If sizeof(float) != sizeof(long), we're hosed.
 */
inline long RegFg2Float(long x, int FudgeFact)
{
	float f = float(x)/float(1 << FudgeFact);
	return FloatAsLong(f);
}

/*
 * This converts em to shifted integer format.
 */
inline long RegFloat2Fg(long x, int Fudge)
{
	return long(LongAsFloat(x)*float(1 << Fudge));
}

inline long RegSftFloat(long x, int Shift)
{
	float f = LongAsFloat(x);
	if (Shift > 0)
	{
		f *= (1 << Shift);
	}
	else
	{
		f /= (1 << -Shift);
	}
	return FloatAsLong(f);
}

#endif
