#if !defined(CMPLX_H)
#define CMPLX_H
//
// c m p l x . h
//
// various complex number defs
//
#include <cmath>
#include <complex>
#include "id.h"
#include "fpu.h"

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

typedef std::complex<double> StdComplexD;
typedef std::complex<long> StdComplexL;

typedef ComplexT<double> ComplexD;
typedef InitializedComplexT<double> InitializedComplexD;
typedef ComplexT<long> ComplexL;
typedef HyperComplexT<double> HyperComplexD;

extern ComplexD ComplexSqrtFloat(double re, double im);

template <typename T>
inline ComplexT<T> ComplexTFromStd(std::complex<T> const &right)
{
	ComplexT<T> result;
	result.real(right.real());
	result.imag(right.imag());
	return result;
}

template <typename T>
inline std::complex<T> ComplexStdFromT(ComplexT<T> const &right)
{
	return std::complex<T>(right.real(), right.imag());
}

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

inline long LCMPLXmod(ComplexL const &z)
{
	return lsqr(z.real()) + lsqr(z.imag());
}

template <typename T>
inline ComplexT<T> MakeComplexT(T re = T(), T im = T())
{
	ComplexT<T> z;
	z.real(re);
	z.imag(im);
	return z;
}

template <typename T>
inline ComplexT<T> operator+(ComplexT<T> const &left, ComplexT<T> const &right)
{ return MakeComplexT(left.real() + right.real(), left.imag() + right.imag()); }
template <typename T>
inline ComplexT<T> operator+(ComplexT<T> const &left, T const &right)
{ return MakeComplexT(left.real() + right, left.imag()); }
template <typename T>
inline ComplexT<T> operator+(T const &left, ComplexT<T> const &right)
{ return right + left; }

template <typename T>
inline ComplexT<T> operator-(ComplexT<T> const &left, ComplexT<T> const &right)
{ return MakeComplexT(left.real() - right.real(), left.imag() - right.imag()); }
template <typename T>
inline ComplexT<T> operator-(ComplexT<T> const &left, T const &right)
{ return MakeComplexT(left.real() - right, left.imag()); }
template <typename T>
inline ComplexT<T> operator-(T const &left, ComplexT<T> const &right)
{ return MakeComplexT(left - right.real(), -right.imag()); }

inline ComplexD operator*(ComplexD const &left, ComplexD const &right)
{
	return MakeComplexT(
		left.real()*right.real() - left.imag()*right.imag(),
		left.real()*right.imag() + left.imag()*right.real());
}
inline ComplexD operator*(ComplexD const &left, StdComplexD const &right)
{ return left*MakeComplexT(right.real(), right.imag()); }
template <typename T>
inline ComplexT<T> operator*(T const &left, ComplexT<T> const &right)
{ return MakeComplexT(left*right.real(), left*right.imag()); }
template <typename T>
inline ComplexT<T> operator*(ComplexT<T> const &left, T const &right)
{ return MakeComplexT(left.real()*right, left.imag()*right); }

inline ComplexD operator/(ComplexD const &x, ComplexD const &y)
{
	double const mod = y.real()*y.real() + y.imag()*y.imag();
	if (mod == 0.0)
	{
		DivideOverflow++;
	}
	double const yxmod = y.real()/mod;
	double const yymod = -y.imag()/mod;
	double const tx = x.real()*yxmod - x.imag()*yymod;
	return MakeComplexT(tx, x.real()*yymod + x.imag()*yxmod);
}
inline ComplexD operator/(ComplexD const &left, double right)
{ return MakeComplexT(left.real()/right, left.imag()/right); }

inline ComplexD operator-(ComplexD const &right)
{ return MakeComplexT(-right.real(), -right.imag()); }

inline ComplexD dot(ComplexD const &left, ComplexD const &right)
{ return MakeComplexT(left.real()*right.real(), left.imag()*right.imag()); }

inline ComplexD sqrt(ComplexD const &z)
{
	return ComplexSqrtFloat(z.real(), z.imag());
}

inline ComplexD TimesI(ComplexD const &z)
{
	return MakeComplexT(-z.imag(), z.real());
}

inline ComplexD ComplexLog(ComplexD const &x)
{
	double const mod = std::sqrt(x.real()*x.real() + x.imag()*x.imag());
	double const zx = std::log(mod);
	double const zy = std::atan2(x.imag(), x.real());
	return MakeComplexT(zx, zy);
}

// rz = Arccosh(z) = Log(z + sqrt(z*z-1)}
inline ComplexD acosh(ComplexD const &z)
{
	return ComplexLog(z + sqrt(z*z - 1.0));
}

inline ComplexD asinh(ComplexD const &z)
{
	return ComplexLog(sqrt(z*z + 1.0) + z);
}

// rz = Arctanh(z) = 1/2*Log((1 + z)/(1 - z))
inline ComplexD atanh(ComplexD const &z)
{
	if (z.real() == 0.0)
	{
		return MakeComplexT(0.0, std::atan(z.imag()));
	}
	if (std::abs(z.real()) == 1.0 && z.imag() == 0.0)
	{
		return z;
	}
	if (std::abs(z.real()) < 1.0 && z.imag() == 0.0)
	{
		return MakeComplexT(std::log((1 + z.real())/(1.0 - z.real()))/2.0);
	}

	return 0.5*ComplexLog((1.0 + z)/(1.0 - z));
}

// rz = Arccos(z) = -i*Log(z + sqrt(z*z-1))
inline ComplexD acos(ComplexD const &z)
{
	return -TimesI(ComplexLog(sqrt(z*z - 1.0) + z));
}

// rz = Arcsin(z) = -i*Log(i*z + sqrt(1-z*z))
inline ComplexD asin(ComplexD z)
{
	return -TimesI(ComplexLog(TimesI(z) + sqrt(1.0 - z*z)));
}

// rz = Arctan(z) = i/2*Log{(1-i*z)/(1 + i*z)}
inline ComplexD atan(ComplexD const &z)
{
	if (z.imag() == 0.0)
	{
		if (z.real() == 0.0)
		{
			return MakeComplexT(0.0);
		}
		else
		{
			return MakeComplexT(std::atan(z.real()));
		}
	}
	else
	{
		if (z.real() == 0.0)
		{
			return TimesI(atanh(MakeComplexT(z.imag())));
		}
		else
		{
			ComplexD const zi = TimesI(z);
			return 0.5*TimesI(ComplexLog((1.0 - zi)/(1.0 + zi)));
		}
	}
}

inline ComplexD ComplexExp(ComplexD const &x)
{
	return std::exp(x.real())*MakeComplexT(std::cos(x.imag()), std::sin(x.imag()));
}

// returns x^y
inline ComplexD pow(ComplexD const &x, ComplexD const &y)
{
	if (x.real() == 0 && x.imag() == 0)
	{
		return MakeComplexT(0.0);
	}

	return ComplexExp(ComplexLog(x)*y);
}

#endif
