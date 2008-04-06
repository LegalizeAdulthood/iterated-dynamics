#if !defined(CMPLX_H)
#define CMPLX_H

// various complex number defs

#include "id.h"

template <typename T>
struct ComplexT
{
	T real() const { return _real; }
	T imag() const { return _imag; }
	T real(T right) { return (_real = right); }
	T imag(T right) { return (_imag = right); }

	T _real;
	T _imag;
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
public:
	T z() { return _z; }
	T t() { return _t; }
	T z(T value) { return (_z = value); }
	T t(T value) { return (_t = value); }
private:
	T _z, _t;
};

template <typename T>
class InitializedComplexT : public ComplexT<T>
{
public:
	InitializedComplexT(T re, T im) : ComplexT()
	{
		real(re);
		imag(im);
	}
};

typedef ComplexT<double> ComplexD;
typedef InitializedComplexT<double> InitializedComplexD;
typedef ComplexT<long> ComplexL;
typedef HyperComplexT<double> HyperComplexD;

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
