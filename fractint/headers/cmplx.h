// various complex number defs 
#ifndef _CMPLX_DEFINED
#define _CMPLX_DEFINED

#include "id.h"

template <typename T>
struct ComplexT
{
	T x, y;
	T real() const { return x; }
	T imag() const { return y; }
	T real(T right) { return (x = right); }
	T imag(T right) { return (y = right); }
};

template <class Type>
bool operator==(const ComplexT<Type> &left, const ComplexT<Type> &right)
{
	return (left.x == right.x) && (left.y == right.y);
}
template <class Type>
bool operator==(const ComplexT<Type> &left, const Type &right)
{
	return (left.x == right) && (left.y == 0);
}
template <class Type>
bool operator==(const Type &left, const ComplexT<Type> &right)
{
	return (left == right.x) && (right.y == 0);
}


template <typename T>
struct HyperComplexT : public ComplexT<T>
{
	T z, t;
};

typedef struct ComplexT<double> ComplexD;
typedef struct ComplexT<long> ComplexL;
typedef struct HyperComplexT<double> HyperComplexD;

inline double FudgeToDouble(long x)
{
	extern long g_fudge;
	return double(x)/g_fudge;
}

inline long DoubleToFudge(double x)
{
	extern long g_fudge;
	return long(x*g_fudge);
}

inline ComplexD ComplexFudgeToDouble(const ComplexL &l)
{
	ComplexD z;
	z.x = FudgeToDouble(l.x);
	z.y = FudgeToDouble(l.y);
	return z;
}

inline ComplexL ComplexDoubleToFudge(const ComplexD &d)
{
	ComplexL z;
	z.x = DoubleToFudge(d.x);
	z.y = DoubleToFudge(d.y);
	return z;
}

inline void CMPLXconj(ComplexD &z)
{
	((z).y =  -((z).y));
}

inline long LCMPLXmod(ComplexL const &z)
{
	return lsqr(z.x) + lsqr(z.y);
}

inline void LCMPLXconj(ComplexL &z)
{
	z.y = -z.y;
}

#endif
