#ifndef FMATH_H
#define FMATH_H

/* FMath.h (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
     All rights reserved.

   Code may be used in any program provided the author is credited
     either during program execution or in the documentation.  Source
     code may be distributed only in combination with public domain or
     shareware source code.  Source code may be modified provided the
     copyright notice and this message is left unchanged and all
     modifications are clearly documented.

     I would appreciate a copy of any work which incorporates this code,
     however this is optional.

     Mark C. Peterson
     128 Hamden Ave., F
     Waterbury, CT 06704
     (203) 754-1162

     Notes below document changes to Mark's original file:

     Date       Change                                    Changer
     ============================================================
     07-16-89 - Added sqrt define per Mark's suggestion   TIW
     07-26-89 - Added documentation and complex support   MCP
*/

/*****************
 * Documentation *
 *****************

   #include "fmath.h"
   float x, y, z;
   int Pot, Fudge;

                   23-bit accuracy (limit of type float)
     Regular Implementation               Fast Math Implementation
 --------------------------------------------------------------------
         z = x + y;                          fAdd(x, y, z);
         z = x * y;                          fMul(x, y, z);
         z = x * x;                          fSqr(x, z);
         z = x / y;                          fDiv(x, y, z);
         z = x * 2;                          fShift(x, 1, z);
         z = x * 16;                         fShift(x, 4, z);
         z = x / 32;                         fShift(x, -5, z);
         z = x / (pow(2.0, (double)Pot));    fShift(x, -Pot, z);
         z = (float)Pot * (1L << Fudge);     Fg2Float(Pot, Fudge, z);
         Pot = (int)(z / (1L << Fudge));     Pot = Float2Fg(z, Fudge);

                  Complex numbers using fComplex structures
         z = x**2                            fSqrZ(&x, &z);      mod updated
         z.mod = (z.x*z.x)+(z.y*z.y)         fModZ(&z);          mod updated
         z = 1 / x                           fInvZ(&x, &z);      mod updated
         z = x * y                           fMulZ(&x, &y, &z);  mod updated
         z = x / y                           fDivZ(&x, &y, &z);  mod updated

                            16-bit accuracy
     Regular Implementation               Fast Math Implementation
 --------------------------------------------------------------------
         z = x * y;                          fMul16(x, y, z);
         z = x * x;                          fSqr16(x, z);

                            14-bit accuracy
     Regular Implementation               Fast Math Implementation
 --------------------------------------------------------------------
         z = log(x);                         fLog14(x, z);
         z = exp(x);                         fExp14(x, z);
         z = pow(x, y);                      fPow14(x, y, z);

                            12-bit accuracy
     Regular Implementation               Fast Math Implementation
 --------------------------------------------------------------------
         z = sin(x);                         fSin12(x, z);
         z = cos(x);                         fCos12(x, z);
         z = sinh(x);                        fSinh12(x, z);
         z = cosh(x);                        fCosh12(x, z);

                  Complex numbers using fComplex structures
         z = sin(x)                          fSinZ(&x, &z);
         z = cos(x)                          fCosZ(&x, &z);
         z = tan(x)                          fTagZ(&x, &z);
         z = sinh(x)                         fSinhZ(&x, &z);
         z = cosh(x)                         fCoshZ(&x, &z);
         z = tanh(x)                         fCoshZ(&x, &z);

Just be sure to declare x, y, and z as type floats instead of type double.
*/

#include "fpu.h"

inline void fShift(float x, int shift, float &z)
{
	z = LongAsFloat(RegSftFloat(FloatAsLong(x), shift));
}

inline void fLog14(float x, float &z)
{
	z = LongAsFloat(RegFg2Float(LogFloat14(FloatAsLong(x)), 16));
}

inline long ExpFloat14(long xx)
{
	const float fLogTwo = 0.6931472f;
	const int f = 23 - int(RegFloat2Fg(RegDivFloat(xx, *(long*) &fLogTwo), 0));
	const long answer = ExpFudged(RegFloat2Fg(xx, 16), f);
	return RegFg2Float(answer, f);
}

inline void fExp14(float x, float &z)
{
	z = LongAsFloat(ExpFloat14(FloatAsLong(x)));
}

inline void fSqrt14(float x, float &z)
{
	fLog14(x, z);
	fShift(z, -1, z);
	fExp14(z, z);
}

#endif
