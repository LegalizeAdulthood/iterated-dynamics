/* various complex number defs */
#ifndef _CMPLX_DEFINED
#define _CMPLX_DEFINED

template <typename T>
struct ComplexT
{
	T x, y;
	T real() const { return x; }
	T imag() const { return y; }
};

template <typename T>
struct HyperComplexT : public ComplexT<T>
{
	T z, t;
};

typedef struct ComplexT<double> ComplexD;
typedef struct ComplexT<long> ComplexL;
typedef struct HyperComplexT<double> HyperComplexD;

#endif
