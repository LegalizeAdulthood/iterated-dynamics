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
	return (left.real() == right.real()) && (left.imag() == right.imag());
}
template <class Type>
bool operator==(const ComplexT<Type> &left, const Type &right)
{
	return (left.real() == right) && (left.imag() == 0);
}
template <class Type>
bool operator==(const Type &left, const ComplexT<Type> &right)
{
	return (left == right.real()) && (right.imag() == 0);
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
	z.real(FudgeToDouble(l.real()));
	z.imag(FudgeToDouble(l.imag()));
	return z;
}

inline ComplexL ComplexDoubleToFudge(const ComplexD &d)
{
	ComplexL z;
	z.real(DoubleToFudge(d.real()));
	z.imag(DoubleToFudge(d.imag()));
	return z;
}

inline void CMPLXconj(ComplexD &z)
{
	z.imag(-z.imag());
}

inline long LCMPLXmod(ComplexL const &z)
{
	return lsqr(z.real()) + lsqr(z.imag());
}

inline void LCMPLXconj(ComplexL &z)
{
	z.imag(-z.imag());
}

#endif
