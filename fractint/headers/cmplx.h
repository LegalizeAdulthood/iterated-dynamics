/* various complex number defs */
#ifndef _CMPLX_DEFINED
#define _CMPLX_DEFINED

#include "id.h"

template <typename T>
struct ComplexT
{
	T x, y;
	T real() const { return x; }
	T imag() const { return y; }
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
