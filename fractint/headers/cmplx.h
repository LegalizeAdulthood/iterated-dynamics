/* various complex number defs */
#ifndef _CMPLX_DEFINED
#define _CMPLX_DEFINED

template <typename T>
struct HyperComplex
{
	T x, y;
	T z, t;
};

template <typename T>
struct Complex
{
	T x, y;
};

typedef struct Complex<double> DComplex;
typedef struct Complex<long> LComplex;
typedef struct HyperComplex<double> DHyperComplex;
typedef struct HyperComplex<long> LHyperComplex;

#endif
