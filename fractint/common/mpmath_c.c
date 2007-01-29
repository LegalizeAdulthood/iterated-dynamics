/* MPMath_c.c (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
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
     405-C Queen St. Suite #181
     Southington, CT 06489
     (203) 276-9721
*/


  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"

#if !defined(XFRACT)

struct MP *MPsub(struct MP x, struct MP y) {
   y.Exp ^= 0x8000;
   return(MPadd(x, y));
}

/* added by TW */
struct MP *MPsub086(struct MP x, struct MP y) {
   y.Exp ^= 0x8000;
   return(MPadd086(x, y));
}

/* added by TW */
struct MP *MPsub386(struct MP x, struct MP y) {
   y.Exp ^= 0x8000;
   return(MPadd386(x, y));
}

struct MP *MPabs(struct MP x) {
   Ans = x;
   Ans.Exp &= 0x7fff;
   return(&Ans);
}

struct MPC MPCsqr(struct MPC x) {
   struct MPC z;

        z.x = *pMPsub(*pMPmul(x.x, x.x), *pMPmul(x.y, x.y));
        z.y = *pMPmul(x.x, x.y);
        z.y.Exp++;
   return(z);
}

struct MP MPCmod(struct MPC x) {
        return(*pMPadd(*pMPmul(x.x, x.x), *pMPmul(x.y, x.y)));
}

struct MPC MPCmul(struct MPC x, struct MPC y) {
   struct MPC z;

        z.x = *pMPsub(*pMPmul(x.x, y.x), *pMPmul(x.y, y.y));
        z.y = *pMPadd(*pMPmul(x.x, y.y), *pMPmul(x.y, y.x));
   return(z);
}

struct MPC MPCdiv(struct MPC x, struct MPC y) {
   struct MP mod;

   mod = MPCmod(y);
        y.y.Exp ^= 0x8000;
        y.x = *pMPdiv(y.x, mod);
        y.y = *pMPdiv(y.y, mod);
   return(MPCmul(x, y));
}

struct MPC MPCadd(struct MPC x, struct MPC y) {
   struct MPC z;

        z.x = *pMPadd(x.x, y.x);
        z.y = *pMPadd(x.y, y.y);
   return(z);
}

struct MPC MPCsub(struct MPC x, struct MPC y) {
   struct MPC z;

        z.x = *pMPsub(x.x, y.x);
        z.y = *pMPsub(x.y, y.y);
   return(z);
}

struct MPC MPCone = { {0x3fff, 0x80000000l},
                      {0, 0l}
                    };

struct MPC MPCpow(struct MPC x, int exp) {
   struct MPC z;
   struct MPC zz;

   if (exp & 1)
      z = x;
   else
      z = MPCone;
   exp >>= 1;
   while (exp) {
                zz.x = *pMPsub(*pMPmul(x.x, x.x), *pMPmul(x.y, x.y));
                zz.y = *pMPmul(x.x, x.y);
                zz.y.Exp++;
      x = zz;
      if (exp & 1) {
                        zz.x = *pMPsub(*pMPmul(z.x, x.x), *pMPmul(z.y, x.y));
                        zz.y = *pMPadd(*pMPmul(z.x, x.y), *pMPmul(z.y, x.x));
         z = zz;
      }
      exp >>= 1;
   }
   return(z);
}

int MPCcmp(struct MPC x, struct MPC y) {
   struct MPC z;

        if (pMPcmp(x.x, y.x) || pMPcmp(x.y, y.y)) {
                z.x = MPCmod(x);
                z.y = MPCmod(y);
                return(pMPcmp(z.x, z.y));
   }
   else
      return(0);
}

_CMPLX MPC2cmplx(struct MPC x) {
   _CMPLX z;

        z.x = *pMP2d(x.x);
        z.y = *pMP2d(x.y);
   return(z);
}

struct MPC cmplx2MPC(_CMPLX z) {
   struct MPC x;

        x.x = *pd2MP(z.x);
        x.y = *pd2MP(z.y);
   return(x);
}

/* function pointer versions added by Tim Wegner 12/07/89 */
/* int        (*ppMPcmp)() = MPcmp086; */
int        (*pMPcmp)(struct MP x, struct MP y) = MPcmp086;
struct MP  *(*pMPmul)(struct MP x, struct MP y)= MPmul086;
struct MP  *(*pMPdiv)(struct MP x, struct MP y)= MPdiv086;
struct MP  *(*pMPadd)(struct MP x, struct MP y)= MPadd086;
struct MP  *(*pMPsub)(struct MP x, struct MP y)= MPsub086;
struct MP  *(*pd2MP)(double x)                 = d2MP086 ;
double *(*pMP2d)(struct MP m)                  = MP2d086 ;
/* struct MP  *(*pfg2MP)(long x, int fg)          = fg2MP086; */

void setMPfunctions(void) {
   if (cpu >= 386)
   {
      pMPmul = MPmul386;
      pMPdiv = MPdiv386;
      pMPadd = MPadd386;
      pMPsub = MPsub386;
      pMPcmp = MPcmp386;
      pd2MP  = d2MP386 ;
      pMP2d  = MP2d386 ;
      /* pfg2MP = fg2MP386; */
   }
   else
   {
      pMPmul = MPmul086;
      pMPdiv = MPdiv086;
      pMPadd = MPadd086;
      pMPsub = MPsub086;
      pMPcmp = MPcmp086;
      pd2MP  = d2MP086 ;
      pMP2d  = MP2d086 ;
      /* pfg2MP = fg2MP086; */
   }
}
#endif /* XFRACT */

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

_CMPLX ComplexPower(_CMPLX xx, _CMPLX yy) {
   _CMPLX z, cLog, t;
   double e2x, siny, cosy;

   /* fixes power bug - if any complaints, backwards compatibility hook
      goes here TIW 3/95 */
   if (ldcheck == 0)
      if (xx.x == 0 && xx.y == 0) {
         z.x = z.y = 0.0;
         return(z);
      }

   FPUcplxlog(&xx, &cLog);
   FPUcplxmul(&cLog, &yy, &t);

   if (fpu >= 387)
      FPUcplxexp387(&t, &z);
   else {
      if (t.x < -690)
         e2x = 0;
      else
         e2x = exp(t.x);
      FPUsincos(&t.y, &siny, &cosy);
      z.x = e2x * cosy;
      z.y = e2x * siny;
   }
   return(z);
}

/*

  The following Complex function routines added by Tim Wegner November 1994.

*/

#define Sqrtz(z,rz) (*(rz) = ComplexSqrtFloat((z).x, (z).y))

/* rz=Arcsin(z)=-i*Log{i*z+sqrt(1-z*z)} */
void Arcsinz(_CMPLX z,_CMPLX *rz)
{
  _CMPLX tempz1,tempz2;

  FPUcplxmul( &z, &z, &tempz1);
  tempz1.x = 1 - tempz1.x; tempz1.y = -tempz1.y;  /* tempz1 = 1 - tempz1 */
  Sqrtz( tempz1, &tempz1);

  tempz2.x = -z.y; tempz2.y = z.x;                /* tempz2 = i*z  */
  tempz1.x += tempz2.x;  tempz1.y += tempz2.y;    /* tempz1 += tempz2 */
  FPUcplxlog( &tempz1, &tempz1);
  rz->x = tempz1.y;  rz->y = -tempz1.x;           /* rz = (-i)*tempz1 */
}   /* end. Arcsinz */


/* rz=Arccos(z)=-i*Log{z+sqrt(z*z-1)} */
void Arccosz(_CMPLX z,_CMPLX *rz)
{
  _CMPLX temp;

  FPUcplxmul( &z, &z, &temp);
  temp.x -= 1;                                 /* temp = temp - 1 */
  Sqrtz( temp, &temp);

  temp.x += z.x; temp.y += z.y;                /* temp = z + temp */

  FPUcplxlog( &temp, &temp);
  rz->x = temp.y;  rz->y = -temp.x;              /* rz = (-i)*tempz1 */
}   /* end. Arccosz */

void Arcsinhz(_CMPLX z,_CMPLX *rz)
{
  _CMPLX temp;

  FPUcplxmul( &z, &z, &temp);
  temp.x += 1;                                 /* temp = temp + 1 */
  Sqrtz( temp, &temp);
  temp.x += z.x; temp.y += z.y;                /* temp = z + temp */
  FPUcplxlog( &temp, rz);
}  /* end. Arcsinhz */

/* rz=Arccosh(z)=Log(z+sqrt(z*z-1)} */
void Arccoshz(_CMPLX z,_CMPLX *rz)
{
  _CMPLX tempz;
  FPUcplxmul( &z, &z, &tempz);
  tempz.x -= 1;                              /* tempz = tempz - 1 */
  Sqrtz( tempz, &tempz);
  tempz.x = z.x + tempz.x; tempz.y = z.y + tempz.y;  /* tempz = z + tempz */
  FPUcplxlog( &tempz, rz);
}   /* end. Arccoshz */

/* rz=Arctanh(z)=1/2*Log{(1+z)/(1-z)} */
void Arctanhz(_CMPLX z,_CMPLX *rz)
{
  _CMPLX temp0,temp1,temp2;

  if ( z.x == 0.0){
    rz->x = 0;
    rz->y = atan( z.y);
    return;
  }
  else{
    if ( fabs(z.x) == 1.0 && z.y == 0.0){
      return;
    }
    else if ( fabs( z.x) < 1.0 && z.y == 0.0){
      rz->x = log((1+z.x)/(1-z.x))/2;
      rz->y = 0;
      return;
    }
    else{
      temp0.x = 1 + z.x; temp0.y = z.y;             /* temp0 = 1 + z */
      temp1.x = 1 - z.x; temp1.y = -z.y;            /* temp1 = 1 - z */
      FPUcplxdiv( &temp0, &temp1, &temp2);
      FPUcplxlog( &temp2, &temp2);
      rz->x = .5*temp2.x; rz->y = .5*temp2.y;       /* rz = .5*temp2 */
      return;
    }
  }
}   /* end. Arctanhz */

/* rz=Arctan(z)=i/2*Log{(1-i*z)/(1+i*z)} */
void Arctanz(_CMPLX z,_CMPLX *rz)
{
  _CMPLX temp0,temp1,temp2,temp3;
  if ( z.x == 0.0 && z.y == 0.0)
    rz->x = rz->y = 0;
  else if ( z.x != 0.0 && z.y == 0.0){
    rz->x = atan( z.x);
    rz->y = 0;
  }
  else if ( z.x == 0.0 && z.y != 0.0){
    temp0.x = z.y;  temp0.y = 0.0;
    Arctanhz( temp0, &temp0);
    rz->x = -temp0.y; rz->y = temp0.x;              /* i*temp0 */
  }
  else if ( z.x != 0.0 && z.y != 0.0){

    temp0.x = -z.y; temp0.y = z.x;                  /* i*z */
    temp1.x = 1 - temp0.x; temp1.y = -temp0.y;      /* temp1 = 1 - temp0 */
    temp2.x = 1 + temp0.x; temp2.y = temp0.y;       /* temp2 = 1 + temp0 */

    FPUcplxdiv( &temp1, &temp2, &temp3);
    FPUcplxlog( &temp3, &temp3);
    rz->x = -temp3.y*.5; rz->y = .5*temp3.x;           /* .5*i*temp0 */
  }
}   /* end. Arctanz */

#define SinCosFudge 0x10000L
#ifdef LONGSQRT
long lsqrt(long f)
{
    int N;
    unsigned long y0, z;
    static long a=0, b=0, c=0;                  /* constant factors */

    if (f == 0)
        return f;
    if (f <  0)
        return 0;

    if (a==0)                                   /* one-time compute consts */
    {
        a = (long)(fudge * .41731);
        b = (long)(fudge * .59016);
        c = (long)(fudge * .7071067811);
    }

    N  = 0;
    while (f & 0xff000000L)                     /* shift arg f into the */
    {                                           /* range: 0.5 <= f < 1  */
        N++;
        f /= 2;
    }
    while (!(f & 0xff800000L))
    {
        N--;
        f *= 2;
    }

    y0 = a + multiply(b, f,  bitshift);         /* Newton's approximation */

    z  = y0 + divide (f, y0, bitshift);
    y0 = (z>>2) + divide(f, z,  bitshift);

    if (N % 2)
    {
        N++;
        y0 = multiply(c,y0, bitshift);
    }
    N /= 2;
    if (N >= 0)
        return y0 <<  N;                        /* correct for shift above */
    else
        return y0 >> -N;
}
#endif
LCMPLX ComplexSqrtLong(long x, long y)
{
   double    mag, theta;
   long      maglong, thetalong;
   LCMPLX    result;

#ifndef LONGSQRT
   mag       = sqrt(sqrt(((double) multiply(x,x,bitshift))/fudge +
                         ((double) multiply(y,y,bitshift))/ fudge));
   maglong   = (long)(mag * fudge);
#else
   maglong   = lsqrt(lsqrt(multiply(x,x,bitshift)+multiply(y,y,bitshift)));
#endif
   theta     = atan2((double) y/fudge, (double) x/fudge)/2;
   thetalong = (long)(theta * SinCosFudge);
   SinCos086(thetalong, &result.y, &result.x);
   result.x  = multiply(result.x << (bitshift - 16), maglong, bitshift);
   result.y  = multiply(result.y << (bitshift - 16), maglong, bitshift);
   return result;
}

_CMPLX ComplexSqrtFloat(double x, double y)
{
   double mag;
   double theta;
   _CMPLX  result;

   if (x == 0.0 && y == 0.0)
      result.x = result.y = 0.0;
   else
   {
      mag   = sqrt(sqrt(x*x + y*y));
      theta = atan2(y, x) / 2;
      FPUsincos(&theta, &result.y, &result.x);
      result.x *= mag;
      result.y *= mag;
   }
   return result;
}


/***** FRACTINT specific routines and variables *****/

#ifndef TESTING_MATH

BYTE *LogTable = (BYTE *)0;
long MaxLTSize;
int  Log_Calc = 0;
static double mlf;
static unsigned long lf;

   /* int LogFlag;
      LogFlag == 1  -- standard log palettes
      LogFlag == -1 -- 'old' log palettes
      LogFlag >  1  -- compress counts < LogFlag into color #1
      LogFlag < -1  -- use quadratic palettes based on square roots && compress
   */

void SetupLogTable(void) {
   float l, f, c, m;
   unsigned long prev, limit, sptop;
   unsigned n;

 if (save_release > 1920 || Log_Fly_Calc == 1) { /* set up on-the-fly variables */
   if (LogFlag > 0) { /* new log function */
      lf = (LogFlag > 1) ? LogFlag : 0;
      if (lf >= (unsigned long)MaxLTSize)
         lf = MaxLTSize - 1;
      mlf = (colors - (lf?2:1)) / log(MaxLTSize - lf);
   } else if (LogFlag == -1) { /* old log function */
      mlf = (colors - 1) / log(MaxLTSize);
   } else if (LogFlag <= -2) { /* sqrt function */
      if ((lf = 0 - LogFlag) >= (unsigned long)MaxLTSize)
         lf = MaxLTSize - 1;
      mlf = (colors - 2) / sqrt(MaxLTSize - lf);
   }
 }

 if (Log_Calc)
    return; /* LogTable not defined, bail out now */

 if (save_release > 1920 && !Log_Calc) {
    Log_Calc = 1;   /* turn it on */
    for (prev = 0; prev <= (unsigned long)MaxLTSize; prev++)
        LogTable[prev] = (BYTE)logtablecalc((long)prev);
    Log_Calc = 0;   /* turn it off, again */
    return;
 }

   if (LogFlag > -2) {
      lf = (LogFlag > 1) ? LogFlag : 0;
      if (lf >= (unsigned long)MaxLTSize)
         lf = MaxLTSize - 1;
      Fg2Float((long)(MaxLTSize-lf), 0, m);
      fLog14(m, m);
      Fg2Float((long)(colors-(lf?2:1)), 0, c);
      fDiv(m, c, m);
      for (prev = 1; prev <= lf; prev++)
         LogTable[prev] = 1;
      for (n = (lf?2:1); n < (unsigned int)colors; n++) {
         Fg2Float((long)n, 0, f);
         fMul16(f, m, f);
         fExp14(f, l);
         limit = (unsigned long)Float2Fg(l, 0) + lf;
         if (limit > (unsigned long)MaxLTSize || n == (unsigned int)(colors-1))
            limit = MaxLTSize;
         while (prev <= limit)
            LogTable[prev++] = (BYTE)n;
      }
   } else {
      if ((lf = 0 - LogFlag) >= (unsigned long)MaxLTSize)
         lf = MaxLTSize - 1;
      Fg2Float((long)(MaxLTSize-lf), 0, m);
      fSqrt14(m, m);
      Fg2Float((long)(colors-2), 0, c);
      fDiv(m, c, m);
      for (prev = 1; prev <= lf; prev++)
         LogTable[prev] = 1;
      for (n = 2; n < (unsigned int)colors; n++) {
         Fg2Float((long)n, 0, f);
         fMul16(f, m, f);
         fMul16(f, f, l);
         limit = (unsigned long)(Float2Fg(l, 0) + lf);
         if (limit > (unsigned long)MaxLTSize || n == (unsigned int)(colors-1))
            limit = MaxLTSize;
         while (prev <= limit)
            LogTable[prev++] = (BYTE)n;
      }
   }
   LogTable[0] = 0;
   if (LogFlag != -1)
      for (sptop = 1; sptop < (unsigned long)MaxLTSize; sptop++) /* spread top to incl unused colors */
         if (LogTable[sptop] > LogTable[sptop-1])
            LogTable[sptop] = (BYTE)(LogTable[sptop-1]+1);
}

long logtablecalc(long citer) {
   long ret = 0;

   if (LogFlag == 0 && !rangeslen) /* Oops, how did we get here? */
      return(citer);
   if (LogTable && !Log_Calc)
      return(LogTable[(long)min(citer, MaxLTSize)]);

   if (LogFlag > 0) { /* new log function */
      if ((unsigned long)citer <= lf + 1)
         ret = 1;
      else if ((citer - lf) / log(citer - lf) <= mlf) {
         if (save_release < 2002)
            ret = (long)(citer - lf + (lf?1:0));
         else
            ret = (long)(citer - lf);
      }
      else
         ret = (long)(mlf * log(citer - lf)) + 1;
   } else if (LogFlag == -1) { /* old log function */
      if (citer == 0)
         ret = 1;
      else
         ret = (long)(mlf * log(citer)) + 1;
   } else if (LogFlag <= -2) { /* sqrt function */
      if ((unsigned long)citer <= lf)
         ret = 1;
      else if ((unsigned long)(citer - lf) <= (unsigned long)(mlf * mlf))
         ret = (long)(citer - lf + 1);
      else
         ret = (long)(mlf * sqrt(citer - lf)) + 1;
   }
   return (ret);
}

long ExpFloat14(long xx) {
   static float fLogTwo = (float)0.6931472;
   int f;
   long Ans;

   f = 23 - (int)RegFloat2Fg(RegDivFloat(xx, *(long*)&fLogTwo), 0);
   Ans = ExpFudged(RegFloat2Fg(xx, 16), f);
   return(RegFg2Float(Ans, (char)f));
}

double TwoPi;
_CMPLX temp, BaseLog;
_CMPLX cdegree = { 3.0, 0.0 }, croot   = { 1.0, 0.0 };

int ComplexNewtonSetup(void) {
   threshold = .001;
   periodicitycheck = 0;
   if (param[0] != 0.0 || param[1] != 0.0 || param[2] != 0.0 ||
      param[3] != 0.0) {
      croot.x = param[2];
      croot.y = param[3];
      cdegree.x = param[0];
      cdegree.y = param[1];
      FPUcplxlog(&croot, &BaseLog);
      TwoPi = asin(1.0) * 4;
   }
   return(1);
}

int ComplexNewton(void) {
   _CMPLX cd1;

   /* new = ((cdegree-1) * old**cdegree) + croot
            ----------------------------------
                 cdegree * old**(cdegree-1)         */

   cd1.x = cdegree.x - 1.0;
   cd1.y = cdegree.y;

   temp = ComplexPower(old, cd1);
   FPUcplxmul(&temp, &old, &g_new);

   tmp.x = g_new.x - croot.x;
   tmp.y = g_new.y - croot.y;
   if ((sqr(tmp.x) + sqr(tmp.y)) < threshold)
      return(1);

   FPUcplxmul(&g_new, &cd1, &tmp);
   tmp.x += croot.x;
   tmp.y += croot.y;

   FPUcplxmul(&temp, &cdegree, &cd1);
   FPUcplxdiv(&tmp, &cd1, &old);
   if (overflow)
   {
      return(1);
   }
   g_new = old;
   return(0);
}

int ComplexBasin(void) {
   _CMPLX cd1;
   double mod;

   /* new = ((cdegree-1) * old**cdegree) + croot
            ----------------------------------
                 cdegree * old**(cdegree-1)         */

   cd1.x = cdegree.x - 1.0;
   cd1.y = cdegree.y;

   temp = ComplexPower(old, cd1);
   FPUcplxmul(&temp, &old, &g_new);

   tmp.x = g_new.x - croot.x;
   tmp.y = g_new.y - croot.y;
   if ((sqr(tmp.x) + sqr(tmp.y)) < threshold) {
      if (fabs(old.y) < .01)
         old.y = 0.0;
      FPUcplxlog(&old, &temp);
      FPUcplxmul(&temp, &cdegree, &tmp);
      mod = tmp.y/TwoPi;
      coloriter = (long)mod;
      if (fabs(mod - coloriter) > 0.5) {
         if (mod < 0.0)
            coloriter--;
         else
            coloriter++;
      }
      coloriter += 2;
      if (coloriter < 0)
         coloriter += 128;
      return(1);
   }

   FPUcplxmul(&g_new, &cd1, &tmp);
   tmp.x += croot.x;
   tmp.y += croot.y;

   FPUcplxmul(&temp, &cdegree, &cd1);
   FPUcplxdiv(&tmp, &cd1, &old);
   if (overflow)
   {
      return(1);
   }
   g_new = old;
   return(0);
}

/*
 * Generate a gaussian distributed number.
 * The right half of the distribution is folded onto the lower half.
 * That is, the curve slopes up to the peak and then drops to 0.
 * The larger slope is, the smaller the standard deviation.
 * The values vary from 0+offset to range+offset, with the peak
 * at range+offset.
 * To make this more complicated, you only have a
 * 1 in Distribution*(1-Probability/Range*con)+1 chance of getting a
 * Gaussian; otherwise you just get offset.
 */
int GausianNumber(int Probability, int Range) {
   int n, r;
   long Accum = 0, p;

   p = divide((long)Probability << 16, (long)Range << 16, 16);
   p = multiply(p, con, 16);
   p = multiply((long)Distribution << 16, p, 16);
   if (!(rand15() % (Distribution - (int)(p >> 16) + 1))) {
      for (n = 0; n < Slope; n++)
         Accum += rand15();
      Accum /= Slope;
      r = (int)(multiply((long)Range << 15, Accum, 15) >> 14);
      r = r - Range;
      if (r < 0)
         r = -r;
      return(Range - r + Offset);
   }
   return(Offset);
}

#endif

#if defined(_WIN32)
double *MP2d086(struct MP x)
{
	/* TODO: implement */
	static double ans = 0.0;
	_ASSERTE(0 && "MP2d086 called.");
	return &ans;
}

/*
d2MP086     PROC     uses si di, x:QWORD
   mov   dx, word ptr [x+6]
   mov   ax, word ptr [x+4]
   mov   bx, word ptr [x+2]
   mov   cx, word ptr [x]
   mov   si, dx
   shl   si, 1
   pushf
   mov   cl, 4
   shr   si, cl
   popf
   rcr   si, 1
   add   si, (1 SHL 14) - (1 SHL 10)

   mov   di, ax                           ; shl dx:ax:bx 12 bits
   mov   cl, 12
   shl   dx, cl
   shl   ax, cl
   mov   cl, 4
   shr   di, cl
   shr   bx, cl
   or    dx, di
   or    ax, bx
   stc
   rcr   dx, 1
   rcr   ax, 1

StoreAns:
   mov   Ans.Exp, si
   mov   word ptr Ans.Mant+2, dx
   mov   word ptr Ans.Mant, ax

   lea   ax, Ans
   mov   dx, ds
   ret
d2MP086     ENDP
*/
struct MP *d2MP086(double x)
{
	/* TODO: implement */
	if (0.0 == x)
	{
		Ans.Exp = 0;
		Ans.Mant = 0;
	}
	else
	{
		__asm
		{
			mov dx, word ptr [x+6]
			mov ax, word ptr [x+4]
			mov bx, word ptr [x+2]
			mov cx, word ptr [x]
			xor	esi, esi
			mov   si, dx
			shl   si, 1
			pushf
			mov   cl, 4
			shr   si, cl
			popf
			rcr   si, 1
			add   si, (1 SHL 14) - (1 SHL 10)

			mov   di, ax                           ; shl dx:ax:bx 12 bits
			mov   cl, 12
			shl   dx, cl
			shl   ax, cl
			mov   cl, 4
			shr   di, cl
			shr   bx, cl
			or    dx, di
			or    ax, bx
			stc
			rcr   dx, 1
			rcr   ax, 1

			mov   Ans.Exp, esi
			mov   word ptr Ans.Mant+2, dx
			mov   word ptr Ans.Mant, ax
		}
	}
	return &Ans;
}

struct MP *MPadd086(struct MP x, struct MP y)
{
	/* TODO: implement */
	_ASSERTE(0 && "MPadd086 called.");
	return &Ans;
}

int MPcmp086(struct MP x, struct MP y)
{
	/* TODO: implement */
	_ASSERTE(0 && "MPcmp086 called.");
	return 0;
}

struct MP *MPdiv086(struct MP x, struct MP y)
{
	/* TODO: implement */
	_ASSERTE(0 && "MPdiv086 called.");
	return &Ans;
}

/*
MPmul086    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
   mov   ax, xExp
   mov   bx, yExp
   xor   ch, ch
   shl   bh, 1
   rcr   ch, 1
   shr   bh, 1
   xor   ah, ch

   sub   bx, (1 SHL 14) - 2
   add   ax, bx
   jno   NoOverflow

Overflow:
   or    word ptr [xMant+2], 0
   jz    ZeroAns
   or    word ptr [yMant+2], 0
   jz    ZeroAns

   mov   MPOverflow, 1

ZeroAns:
   xor   ax, ax
   xor   dx, dx
   mov   Ans.Exp, ax
   jmp   StoreMant

NoOverflow:
   mov   Ans.Exp, ax

   mov   si, word ptr [xMant+2]
   mov   bx, word ptr [xMant]
   mov   di, word ptr [yMant+2]
   mov   cx, word ptr [yMant]

   mov   ax, si
   or    ax, bx
   jz    ZeroAns

   mov   ax, di
   or    ax, cx
   jz    ZeroAns

   mov   ax, cx
   mul   bx
   push  dx

   mov   ax, cx
   mul   si
   push  ax
   push  dx

   mov   ax, bx
   mul   di
   push  ax
   push  dx

   mov   ax, si
   mul   di
   pop   bx
   pop   cx
   pop   si
   pop   di

   add   ax, bx
   adc   dx, 0
   pop   bx
   add   di, bx
   adc   ax, 0
   adc   dx, 0
   add   di, cx
   adc   ax, si
   adc   dx, 0

   or    dx, dx
   js    StoreMant

   shl   di, 1
   rcl   ax, 1
   rcl   dx, 1
   sub   Ans.Exp, 1
   jo    Overflow

StoreMant:
   mov   word ptr Ans.Mant+2, dx
   mov   word ptr Ans.Mant, ax

   lea   ax, Ans
   mov   dx, ds
   ret
MPmul086    ENDP
*/
struct MP *MPmul086(struct MP x, struct MP y)
{
	/* TODO: implement */
	__asm
	{
		xor   eax, eax
		xor   ebx, ebx
		mov   eax, x.Exp
		mov   ebx, y.Exp
		xor   ch, ch
		shl   bh, 1
		rcr   ch, 1
		shr   bh, 1
		xor   ah, ch

		sub   bx, (1 SHL 14) - 2
		add   ax, bx
		jno   NoOverflow

	Overflow:
		or    word ptr [x.Mant+2], 0
		jz    ZeroAns
		or    word ptr [y.Mant+2], 0
		jz    ZeroAns

		mov   MPOverflow, 1

	ZeroAns:
		xor   ax, ax
		xor   dx, dx
		mov   Ans.Exp, eax
		jmp   StoreMant

	NoOverflow:
		mov   Ans.Exp, eax

		mov   si, word ptr [x.Mant+2]
		mov   bx, word ptr [x.Mant]
		mov   di, word ptr [y.Mant+2]
		mov   cx, word ptr [y.Mant]

		mov   ax, si
		or    ax, bx
		jz    ZeroAns

		mov   ax, di
		or    ax, cx
		jz    ZeroAns

		mov   ax, cx
		mul   bx
		push  dx

		mov   ax, cx
		mul   si
		push  ax
		push  dx

		mov   ax, bx
		mul   di
		push  ax
		push  dx

		mov   ax, si
		mul   di
		pop   bx
		pop   cx
		pop   si
		pop   di

		add   ax, bx
		adc   dx, 0
		pop   bx
		add   di, bx
		adc   ax, 0
		adc   dx, 0
		add   di, cx
		adc   ax, si
		adc   dx, 0

		or    dx, dx
		js    StoreMant

		shl   di, 1
		rcl   ax, 1
		rcl   dx, 1
		sub   Ans.Exp, 1
		jo    Overflow

	StoreMant:
		mov   word ptr Ans.Mant+2, dx
		mov   word ptr Ans.Mant, ax
	}

	return &Ans;
}

struct MP *d2MP386(double x)
{
	/* TODO: implement */
	_ASSERTE(0 && "d2MP386 called.");
	return &Ans;
}

double *MP2d386(struct MP x)
{
	/* TODO: implement */
	static double ans = 0.0;
	_ASSERTE(0 && "MP2d386 called.");
	return &ans;
}

struct MP *MPadd(struct MP x, struct MP y)
{
	/* TODO: implement */
	_ASSERTE(0 && "MPadd called.");
	return &Ans;
}

struct MP *MPadd386(struct MP x, struct MP y)
{
	/* TODO: implement */
	_ASSERTE(0 && "MPadd386 called.");
	return &Ans;
}

int MPcmp386(struct MP x, struct MP y)
{
	/* TODO: implement */
	_ASSERTE(0 && "MPcmp386 called.");
	return 0;
}

struct MP *MPdiv386(struct MP x, struct MP y)
{
	/* TODO: implement */
	_ASSERTE(0 && "MPdiv386 called.");
	return &Ans;
}

struct MP *MPmul386(struct MP x, struct MP y)
{
	/* TODO: implement */
	_ASSERTE(0 && "MPmul386 called.");
	return &Ans;
}

struct MP *d2MP(double x)
{
	/* TODO: implement */
	_ASSERTE(0 && "d2MP called.");
	return &Ans;
}

struct MP *MPmul(struct MP x, struct MP y)
{
	/* TODO: implement */
	_ASSERTE(0 && "MPmul called.");
	return &Ans;
}

struct MP *MPdiv(struct MP x, struct MP y)
{
	/* TODO: implement */
	_ASSERTE(0 && "MPdiv called.");
	return &Ans;
}

int MPcmp(struct MP x, struct MP y)
{
	/* TODO: implement */
	_ASSERTE(0 && "MPcmp called.");
	return 0;
}

/*
fg2MP:
   cmp   cpu, 386
   jae   Use386fg2MP
   jmp   fg2MP086

Use386fg2MP:
   jmp   fg2MP386

fg2MP386    PROC     x:DWORD, fg:WORD
   mov   bx, 1 SHL 14 + 30
   sub   bx, fg
.386
   mov   edx, x

   or    edx, edx
   jnz   ChkNegMP

   mov   bx, dx
   jmp   StoreAns

ChkNegMP:
   jns   BitScanRight

   or    bh, 80h
   neg   edx

BitScanRight:
   bsr   ecx, edx
   neg   cx
   add   cx, 31
   sub   bx, cx
   shl   edx, cl

StoreAns:
   mov   Ans.Exp, bx
   mov   Ans.Mant, edx
.8086
   lea   ax, Ans
   mov   dx, ds
   ret
fg2MP386    ENDP

*/
struct MP *fg2MP(long x, int fg)
{
	/* TODO: implement */
	_ASSERTE(0 && "fg2MP called.");
	return &Ans;
}

#endif
