// various complex number defs
#if !defined(CMPLX_H)
#define CMPLX_H

namespace id
{

template <typename T>
struct hyper_complex
{
    T x;
    T y;
    T z;
    T t;
};

template <typename T>
struct complex
{
    T x;
    T y;
};

}

using DHyperComplex = id::hyper_complex<double>;
using LHyperComplex = id::hyper_complex<long>;

using DComplex = id::complex<double>;
using LDComplex = id::complex<LDBL>;
using LComplex = id::complex<long>;

#endif
