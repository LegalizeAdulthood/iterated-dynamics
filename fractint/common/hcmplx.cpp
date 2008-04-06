// some hyper complex functions

#include <string>

#include "port.h"
#include "prototyp.h"
#include "hcmplx.h"
#include "mpmath.h"

void HComplexMult(HyperComplexD *arg1, HyperComplexD *arg2, HyperComplexD *out)
{
	/* it is possible to reoganize this code and reduce the multiplies
		from 16 to 10, but on my 486 it is SLOWER !!! so I left it
		like this - Tim Wegner */
	out->real(arg1->real()*arg2->real() - arg1->imag()*arg2->imag()
			- arg1->z()*arg2->z() + arg1->t()*arg2->t());
	out->imag(arg1->imag()*arg2->real() + arg1->real()*arg2->imag()
			- arg1->t()*arg2->z() - arg1->z()*arg2->t());
	out->z(arg1->z()*arg2->real() - arg1->t()*arg2->imag()
			+ arg1->real()*arg2->z() - arg1->imag()*arg2->t());
	out->t(arg1->t()*arg2->real() + arg1->z()*arg2->imag()
			+ arg1->imag()*arg2->z() + arg1->real()*arg2->t());
}

void HComplexSqr(HyperComplexD *arg, HyperComplexD *out)
{
	out->real(arg->real()*arg->real() - arg->imag()*arg->imag()
			- arg->z()*arg->z() + arg->t()*arg->t());
	out->imag(2*arg->real()*arg->imag() - 2*arg->z()*arg->t());
	out->z(2*arg->z()*arg->real() - 2*arg->t()*arg->imag());
	out->t(2*arg->t()*arg->real() + 2*arg->z()*arg->imag());
}

int HComplexInv(HyperComplexD *arg, HyperComplexD *out)
{
	double det = (sqr(arg->real() - arg->t()) + sqr(arg->imag() + arg->z()))*
		(sqr(arg->real() + arg->t()) + sqr(arg->imag() - arg->z()));

	if (det == 0.0)
	{
		return -1;
	}
	double mod = sqr(arg->real()) + sqr(arg->imag()) + sqr(arg->z()) + sqr(arg->t());
	double xt_minus_yz = arg->real()*arg->t() - arg->imag()*arg->z();

	out->real((arg->real()*mod - 2*arg->t()*xt_minus_yz)/det);
	out->imag((-arg->imag()*mod - 2*arg->z()*xt_minus_yz)/det);
	out->z((-arg->z()*mod - 2*arg->imag()*xt_minus_yz)/det);
	out->t((arg->t()*mod - 2*arg->real()*xt_minus_yz)/det);
	return 0;
}

void HComplexAdd(HyperComplexD *arg1, HyperComplexD *arg2, HyperComplexD *out)
{
	out->real(arg1->real() + arg2->real());
	out->imag(arg1->imag() + arg2->imag());
	out->z(arg1->z() + arg2->z());
	out->t(arg1->t() + arg2->t());
}

void HComplexSub(HyperComplexD *arg1, HyperComplexD *arg2, HyperComplexD *out)
{
	out->real(arg1->real() - arg2->real());
	out->imag(arg1->imag() - arg2->imag());
	out->z(arg1->z() - arg2->z());
	out->t(arg1->t() - arg2->t());
}

void HComplexMinus(HyperComplexD *arg1, HyperComplexD *out)
{
	out->real(-arg1->real());
	out->imag(-arg1->imag());
	out->z(-arg1->z());
	out->t(-arg1->t());
}

// extends the unary function f to *h1
void HComplexTrig0(HyperComplexD *h, HyperComplexD *out)
{
	/* This is the whole beauty of Hypercomplex numbers - *ANY* unary
       complex valued function of a complex variable can easily
       be generalized to hypercomplex numbers */

	ComplexD a;
	ComplexD b;
	ComplexD resulta;
	ComplexD resultb;

	// convert to duplex form
	a.real(h->real() - h->t());
	a.imag(h->imag() + h->z());
	b.real(h->real() + h->t());
	b.imag(h->imag() - h->z());

	// apply function to each part
	CMPLXtrig0(a, resulta);
	CMPLXtrig0(b, resultb);

	// convert back
	out->real((resulta.real() + resultb.real())/2);
	out->imag((resulta.imag() + resultb.imag())/2);
	out->z((resulta.imag() - resultb.imag())/2);
	out->t((resultb.real() - resulta.real())/2);
}
