/*
FRACTALS.C, FRACTALP.C and CALCFRAC.C actually calculate the fractal
images (well, SOMEBODY had to do it!).  The modules are set up so that
all logic that is independent of any fractal-specific code is in
CALCFRAC.C, the code that IS fractal-specific is in FRACTALS.C, and the
structure that ties (we hope!) everything together is in FRACTALP.C.
Original author Tim Wegner, but just about ALL the authors have
contributed SOME code to this routine at one time or another, or
contributed to one of the many massive restructurings.

The Fractal-specific routines are divided into three categories:

1. Routines that are called once-per-orbit to calculate the orbit
   value. These have names like "XxxxFractal", and their function
   pointers are stored in fractalspecific[fractype].orbitcalc. EVERY
   new fractal type needs one of these. Return 0 to continue iterations,
   1 if we're done. Results for integer fractals are left in 'lnew.x' and
   'lnew.y', for floating point fractals in 'new.x' and 'new.y'.

2. Routines that are called once per pixel to set various variables
   prior to the orbit calculation. These have names like xxx_per_pixel
   and are fairly generic - chances are one is right for your new type.
   They are stored in fractalspecific[fractype].per_pixel.

3. Routines that are called once per screen to set various variables.
   These have names like XxxxSetup, and are stored in
   fractalspecific[fractype].per_image.

4. The main fractal routine. Usually this will be StandardFractal(),
   but if you have written a stand-alone fractal routine independent
   of the StandardFractal mechanisms, your routine name goes here,
   stored in fractalspecific[fractype].calctype.per_image.

Adding a new fractal type should be simply a matter of adding an item
to the 'fractalspecific' structure, writing (or re-using one of the existing)
an appropriate setup, per_image, per_pixel, and orbit routines.

--------------------------------------------------------------------   */

#include <limits.h>
#include <string.h>
#ifdef __TURBOC__
#include <alloc.h>
#elif !defined(__386BSD__)
#include <malloc.h>
#endif
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"


#define NEWTONDEGREELIMIT  100
#if 0 /* why are these macros here? They are also in mpmath.h */
#define CMPLXsqr_old(out)       \
   (out).y = (old.x+old.x) * old.y;\
   (out).x = tempsqrx - tempsqry

#define CMPLXpwr(arg1,arg2,out)   (out)= ComplexPower((arg1), (arg2))
#define CMPLXmult1(arg1,arg2,out)    Arg2->d = (arg1); Arg1->d = (arg2);\
         dStkMul(); Arg1++; Arg2++; (out) = Arg2->d

#define CMPLXmult(arg1,arg2,out)  \
        {\
           _CMPLX TmP;\
           TmP.x = (arg1).x*(arg2).x - (arg1).y*(arg2).y;\
           TmP.y = (arg1).x*(arg2).y + (arg1).y*(arg2).x;\
           (out) = TmP;\
         }

#define CMPLXadd(arg1,arg2,out)    \
    (out).x = (arg1).x + (arg2).x; (out).y = (arg1).y + (arg2).y
#define CMPLXsub(arg1,arg2,out)    \
    (out).x = (arg1).x - (arg2).x; (out).y = (arg1).y - (arg2).y
#define CMPLXtimesreal(arg,real,out)   \
    (out).x = (arg).x*(real);\
    (out).y = (arg).y*(real)

#define CMPLXrecip(arg,out)    \
   { double denom; denom = sqr((arg).x) + sqr((arg).y);\
     if(denom==0.0) {(out).x = 1.0e10;(out).y = 1.0e10;}else\
    { (out).x =  (arg).x/denom;\
     (out).y = -(arg).y/denom;}}
#endif
int maxcolor;
int root, degree,basin;
double floatmin,floatmax;
double roverd, d1overd, threshold;
_CMPLX tmp2;
_CMPLX coefficient;
_CMPLX  staticroots[16]; /* roots array for degree 16 or less */
_CMPLX  *roots = staticroots;
_CMPLX pwr;

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#define modulus(z)       (sqr((z).x)+sqr((z).y))
#define conjugate(pz)   ((pz)->y = 0.0 - (pz)->y)
#define distance(z1,z2)  (sqr((z1).x-(z2).x)+sqr((z1).y-(z2).y))

double twopi = PI*2.0;
int c_exp;


/* These are local but I don't want to pass them as parameters */
_CMPLX parm,parm2;
_CMPLX *floatparm;

/* -------------------------------------------------------------------- */
/*              These variables are external for speed's sake only      */
/* -------------------------------------------------------------------- */

double sinx,cosx;
double siny,cosy;
double tmpexp;
double tempsqrx,tempsqry;

double foldxinitx,foldyinity,foldxinity,foldyinitx;

/* These are for quaternions */
double qc,qci,qcj,qck;

/*
**  details of finite attractors (required for Magnet Fractals)
**  (can also be used in "coloring in" the lakes of Julia types)
*/

/*
**  pre-calculated values for fractal types Magnet2M & Magnet2J
*/
_CMPLX  T_Cm1;        /* 3 * (floatparm - 1)                */
_CMPLX  T_Cm2;        /* 3 * (floatparm - 2)                */
_CMPLX  T_Cm1Cm2;     /* (floatparm - 1) * (floatparm - 2) */

void FloatPreCalcMagnet2(void) /* precalculation for Magnet2 (M & J) for speed */
  {
    T_Cm1.x = floatparm->x - 1.0;   T_Cm1.y = floatparm->y;
    T_Cm2.x = floatparm->x - 2.0;   T_Cm2.y = floatparm->y;
    T_Cm1Cm2.x = (T_Cm1.x * T_Cm2.x) - (T_Cm1.y * T_Cm2.y);
    T_Cm1Cm2.y = (T_Cm1.x * T_Cm2.y) + (T_Cm1.y * T_Cm2.x);
    T_Cm1.x += T_Cm1.x + T_Cm1.x;   T_Cm1.y += T_Cm1.y + T_Cm1.y;
    T_Cm2.x += T_Cm2.x + T_Cm2.x;   T_Cm2.y += T_Cm2.y + T_Cm2.y;
  }

/* -------------------------------------------------------------------- */
/*              Bailout Routines Macros                                                                                                 */
/* -------------------------------------------------------------------- */

int (near *floatbailout)(void);
int (near *bignumbailout)(void);
int (near *bigfltbailout)(void);

#if 0
int near fpMODbailout(void)
{
   if ( ( magnitude = ( tempsqrx=sqr(new.x) )
                    + ( tempsqry=sqr(new.y) ) ) >= rqlim ) return(1);
   old = new;
   return(0);
}
#endif
int near fpMODbailout(void)
{
   tempsqrx=sqr(new.x);
   tempsqry=sqr(new.y);
   magnitude = tempsqrx + tempsqry;
   if(magnitude >= rqlim) return(1);
   old = new;
   return(0);
}

int near fpREALbailout(void)
{
   tempsqrx=sqr(new.x);
   tempsqry=sqr(new.y);
   magnitude = tempsqrx + tempsqry;
   if(tempsqrx >= rqlim) return(1);
   old = new;
   return(0);
}

int near fpIMAGbailout(void)
{
   tempsqrx=sqr(new.x);
   tempsqry=sqr(new.y);
   magnitude = tempsqrx + tempsqry;
   if(tempsqry >= rqlim) return(1);
   old = new;
   return(0);
}

int near fpORbailout(void)
{
   tempsqrx=sqr(new.x);
   tempsqry=sqr(new.y);
   magnitude = tempsqrx + tempsqry;
   if(tempsqrx >= rqlim || tempsqry >= rqlim) return(1);
   old = new;
   return(0);
}

int near fpANDbailout(void)
{
   tempsqrx=sqr(new.x);
   tempsqry=sqr(new.y);
   magnitude = tempsqrx + tempsqry;
   if(tempsqrx >= rqlim && tempsqry >= rqlim) return(1);
   old = new;
   return(0);
}

int near fpMANHbailout(void)
{
   double manhmag;
   tempsqrx=sqr(new.x);
   tempsqry=sqr(new.y);
   magnitude = tempsqrx + tempsqry;
   manhmag = fabs(new.x) + fabs(new.y);
   if((manhmag * manhmag) >= rqlim) return(1);
   old = new;
   return(0);
}

int near fpMANRbailout(void)
{
   double manrmag;
   tempsqrx=sqr(new.x);
   tempsqry=sqr(new.y);
   magnitude = tempsqrx + tempsqry;
   manrmag = new.x + new.y; /* don't need abs() since we square it next */
   if((manrmag * manrmag) >= rqlim) return(1);
   old = new;
   return(0);
}

#define FLOATTRIGBAILOUT()  \
   if (fabs(old.y) >= rqlim2) return(1);

#define FLOATXYTRIGBAILOUT()  \
   if (fabs(old.x) >= rqlim2 || fabs(old.y) >= rqlim2) return(1);

#define FLOATHTRIGBAILOUT()  \
   if (fabs(old.x) >= rqlim2) return(1);

#define TRIG16CHECK(X)  \
      if(labs((X)) > l16triglim) { return(1);}

#define OLD_FLOATEXPBAILOUT()  \
   if (fabs(old.y) >= 1.0e8) return(1);\
   if (fabs(old.x) >= 6.4e2) return(1);
  
#define FLOATEXPBAILOUT()  \
   if (fabs(old.y) >= 1.0e3) return(1);\
   if (fabs(old.x) >= 8) return(1);

#if 0
/* this define uses usual trig instead of fast trig */
#define FPUsincos(px,psinx,pcosx) \
   *(psinx) = sin(*(px));\
   *(pcosx) = cos(*(px));

#define FPUsinhcosh(px,psinhx,pcoshx) \
   *(psinhx) = sinh(*(px));\
   *(pcoshx) = cosh(*(px));
#endif

static int near Halleybailout(void)
{
   if ( fabs(modulus(new)-modulus(old)) < parm2.x)
      return(1);
   old = new;
   return(0);
}

#ifdef XFRACT
int asmfpMODbailout(void) { return 0;}
int asmfpREALbailout(void) { return 0;}
int asmfpIMAGbailout(void) { return 0;}
int asmfpORbailout(void) { return 0;}
int asmfpANDbailout(void) { return 0;}
int asmfpMANHbailout(void) { return 0;}
int asmfpMANRbailout(void) { return 0;}
#endif

/* -------------------------------------------------------------------- */
/*              Fractal (once per iteration) routines                   */
/* -------------------------------------------------------------------- */
static double xt, yt, t2;

/* Raise complex number (base) to the (exp) power, storing the result
** in complex (result).
*/
void cpower(_CMPLX *base, int exp, _CMPLX *result)
{
    if (exp<0) {
        cpower(base,-exp,result);
        CMPLXrecip(*result,*result);
        return;
    }

    xt = base->x;   yt = base->y;

    if (exp & 1)
    {
       result->x = xt;
       result->y = yt;
    }
    else
    {
       result->x = 1.0;
       result->y = 0.0;
    }

    exp >>= 1;
    while (exp)
    {
        t2 = xt * xt - yt * yt;
        yt = 2 * xt * yt;
        xt = t2;

        if (exp & 1)
        {
            t2 = xt * result->x - yt * result->y;
            result->y = result->y * xt + yt * result->x;
            result->x = t2;
        }
        exp >>= 1;
    }
}

#if 0
int
z_to_the_z(_CMPLX *z, _CMPLX *out)
{
    static _CMPLX tmp1,tmp2;
    /* raises complex z to the z power */
    int errno_xxx;
    errno_xxx = 0;

    if(fabs(z->x) < DBL_EPSILON) return(-1);

    /* log(x + iy) = 1/2(log(x*x + y*y) + i(arc_tan(y/x)) */
    tmp1.x = .5*log(sqr(z->x)+sqr(z->y));

    /* the fabs in next line added to prevent discontinuity in image */
    tmp1.y = atan(fabs(z->y/z->x));

    /* log(z)*z */
    tmp2.x = tmp1.x * z->x - tmp1.y * z->y;
    tmp2.y = tmp1.x * z->y + tmp1.y * z->x;

    /* z*z = e**(log(z)*z) */
    /* e**(x + iy) =  e**x * (cos(y) + isin(y)) */

    tmpexp = exp(tmp2.x);

    FPUsincos(&tmp2.y,&siny,&cosy);
    out->x = tmpexp*cosy;
    out->y = tmpexp*siny;
    return(errno_xxx);
}
#endif

#ifdef XFRACT /* fractint uses the NewtonFractal2 code in newton.asm */

int complex_div(_CMPLX arg1,_CMPLX arg2,_CMPLX *pz);
int complex_mult(_CMPLX arg1,_CMPLX arg2,_CMPLX *pz);

/* Distance of complex z from unit circle */
#define DIST1(z) (((z).x-1.0)*((z).x-1.0)+((z).y)*((z).y))

int NewtonFractal2(void)
{
    static char start=1;
    if(start)
    {
       start = 0;
    }
    cpower(&old, degree-1, &tmp);
    complex_mult(tmp, old, &new);

    if (DIST1(new) < threshold)
    {
       if(fractype==NEWTBASIN)
       {
          long tmpcolor;
          int i;
          tmpcolor = -1;
          /* this code determines which degree-th root of root the
             Newton formula converges to. The roots of a 1 are
             distributed on a circle of radius 1 about the origin. */
          for(i=0;i<degree;i++)
             /* color in alternating shades with iteration according to
                which root of 1 it converged to */
              if(distance(roots[i],old) < threshold)
              {
                  if (basin==2) {
                      tmpcolor = 1+(i&7)+((coloriter&1)<<3);
                  } else {
                      tmpcolor = 1+i;
                  }
                  break;
              }
           if(tmpcolor == -1)
              coloriter = maxcolor;
           else
              coloriter = tmpcolor;
       }
       return(1);
    }
    new.x = d1overd * new.x + roverd;
    new.y *= d1overd;

    /* Watch for divide underflow */
    if ((t2 = tmp.x * tmp.x + tmp.y * tmp.y) < FLT_MIN)
      return(1);
    else
    {
        t2 = 1.0 / t2;
        old.x = t2 * (new.x * tmp.x + new.y * tmp.y);
        old.y = t2 * (new.y * tmp.x - new.x * tmp.y);
    }
    return(0);
}

int
complex_mult(_CMPLX arg1,_CMPLX arg2,_CMPLX *pz)
{
   pz->x = arg1.x*arg2.x - arg1.y*arg2.y;
   pz->y = arg1.x*arg2.y+arg1.y*arg2.x;
   return(0);
}

int
complex_div(_CMPLX numerator,_CMPLX denominator,_CMPLX *pout)
{
   double mod;
   if((mod = modulus(denominator)) < FLT_MIN)
      return(1);
   conjugate(&denominator);
   complex_mult(numerator,denominator,pout);
   pout->x = pout->x/mod;
   pout->y = pout->y/mod;
   return(0);
}
#endif /* newton code only used by xfractint */

int
Barnsley1FPFractal(void)
{
   /* Barnsley's Mandelbrot type M1 from "Fractals
   Everywhere" by Michael Barnsley, p. 322 */
   /* note that fast >= 287 equiv in fracsuba.asm must be kept in step */

   /* calculate intermediate products */
   foldxinitx = old.x * floatparm->x;
   foldyinity = old.y * floatparm->y;
   foldxinity = old.x * floatparm->y;
   foldyinitx = old.y * floatparm->x;
   /* orbit calculation */
   if(old.x >= 0)
   {
      new.x = (foldxinitx - floatparm->x - foldyinity);
      new.y = (foldyinitx - floatparm->y + foldxinity);
   }
   else
   {
      new.x = (foldxinitx + floatparm->x - foldyinity);
      new.y = (foldyinitx + floatparm->y + foldxinity);
   }
   return(floatbailout());
}

int
Barnsley2FPFractal(void)
{
   /* An unnamed Mandelbrot/Julia function from "Fractals
   Everywhere" by Michael Barnsley, p. 331, example 4.2 */

   /* calculate intermediate products */
   foldxinitx = old.x * floatparm->x;
   foldyinity = old.y * floatparm->y;
   foldxinity = old.x * floatparm->y;
   foldyinitx = old.y * floatparm->x;

   /* orbit calculation */
   if(foldxinity + foldyinitx >= 0)
   {
      new.x = foldxinitx - floatparm->x - foldyinity;
      new.y = foldyinitx - floatparm->y + foldxinity;
   }
   else
   {
      new.x = foldxinitx + floatparm->x - foldyinity;
      new.y = foldyinitx + floatparm->y + foldxinity;
   }
   return(floatbailout());
}

int
JuliafpFractal(void)
{
   /* floating point version of classical Mandelbrot/Julia */
   /* note that fast >= 287 equiv in fracsuba.asm must be kept in step */
   new.x = tempsqrx - tempsqry + floatparm->x;
   new.y = 2.0 * old.x * old.y + floatparm->y;
   return(floatbailout());
}

int
LambdaFPFractal(void)
{
   /* variation of classical Mandelbrot/Julia */
   /* note that fast >= 287 equiv in fracsuba.asm must be kept in step */

   tempsqrx = old.x - tempsqrx + tempsqry;
   tempsqry = -(old.y * old.x);
   tempsqry += tempsqry + old.y;

   new.x = floatparm->x * tempsqrx - floatparm->y * tempsqry;
   new.y = floatparm->x * tempsqry + floatparm->y * tempsqrx;
   return(floatbailout());
}

int
SierpinskiFPFractal(void)
{
   /* following code translated from basic - see "Fractals
   Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */

   new.x = old.x + old.x;
   new.y = old.y + old.y;
   if(old.y > .5)
      new.y = new.y - 1;
   else if (old.x > .5)
      new.x = new.x - 1;

   /* end barnsley code */
   return(floatbailout());
}

int
LambdaexponentFractal(void)
{
   /* found this in  "Science of Fractal Images" */
   if (save_release > 2002) { /* need braces since these are macros */
      FLOATEXPBAILOUT();
   }
    else {
      OLD_FLOATEXPBAILOUT();
   }
   FPUsincos  (&old.y,&siny,&cosy);

   if (old.x >= rqlim && cosy >= 0.0) return(1);
   tmpexp = exp(old.x);
   tmp.x = tmpexp*cosy;
   tmp.y = tmpexp*siny;

   /*multiply by lamda */
   new.x = floatparm->x*tmp.x - floatparm->y*tmp.y;
   new.y = floatparm->y*tmp.x + floatparm->x*tmp.y;
   old = new;
   return(0);
}

int
FloatTrigPlusExponentFractal(void)
{
   /* another Scientific American biomorph type */
   /* z(n+1) = e**z(n) + trig(z(n)) + C */

   if (fabs(old.x) >= 6.4e2) return(1); /* DOMAIN errors */
   tmpexp = exp(old.x);
   FPUsincos  (&old.y,&siny,&cosy);
   CMPLXtrig0(old,new);

   /*new =   trig(old) + e**old + C  */
   new.x += tmpexp*cosy + floatparm->x;
   new.y += tmpexp*siny + floatparm->y;
   return(floatbailout());
}

int
MarksLambdafpFractal(void)
{
   /* Mark Peterson's variation of "lambda" function */

   /* Z1 = (C^(exp-1) * Z**2) + C */
   tmp.x = tempsqrx - tempsqry;
   tmp.y = old.x * old.y *2;

   new.x = coefficient.x * tmp.x - coefficient.y * tmp.y + floatparm->x;
   new.y = coefficient.x * tmp.y + coefficient.y * tmp.x + floatparm->y;

   return(floatbailout());
}

int
UnityfpFractal(void)
{
double XXOne;
   /* brought to you by Mark Peterson - you won't find this in any fractal
      books unless they saw it here first - Mark invented it! */

   XXOne = sqr(old.x) + sqr(old.y);
   if((XXOne > 2.0) || (fabs(XXOne - 1.0) < ddelmin))
      return(1);
   old.y = (2.0 - XXOne)* old.x;
   old.x = (2.0 - XXOne)* old.y;
   new=old;  /* TW added this line */
   return(0);
}

int
Mandel4fpFractal(void)
{
   /* first, compute (x + iy)**2 */
   new.x  = tempsqrx - tempsqry;
   new.y = old.x*old.y*2;
   if (floatbailout()) return(1);

   /* then, compute ((x + iy)**2)**2 + lambda */
   new.x  = tempsqrx - tempsqry + floatparm->x;
   new.y =  old.x*old.y*2 + floatparm->y;
   return(floatbailout());
}

int
floatZtozPluszpwrFractal(void)
{
   cpower(&old,(int)param[2],&new);
   old = ComplexPower(old,old);
   new.x = new.x + old.x +floatparm->x;
   new.y = new.y + old.y +floatparm->y;
   return(floatbailout());
}

int
floatZpowerFractal(void)
{
   cpower(&old,c_exp,&new);
   new.x += floatparm->x;
   new.y += floatparm->y;
   return(floatbailout());
}

int
floatCmplxZpowerFractal(void)
{
   new = ComplexPower(old, parm2);
   new.x += floatparm->x;
   new.y += floatparm->y;
   return(floatbailout());
}

int
Barnsley3FPFractal(void)
{
   /* An unnamed Mandelbrot/Julia function from "Fractals
   Everywhere" by Michael Barnsley, p. 292, example 4.1 */


   /* calculate intermediate products */
   foldxinitx  = old.x * old.x;
   foldyinity  = old.y * old.y;
   foldxinity  = old.x * old.y;

   /* orbit calculation */
   if(old.x > 0)
   {
      new.x = foldxinitx - foldyinity - 1.0;
      new.y = foldxinity * 2;
   }
   else
   {
      new.x = foldxinitx - foldyinity -1.0 + floatparm->x * old.x;
      new.y = foldxinity * 2;

      /* This term added by Tim Wegner to make dependent on the
         imaginary part of the parameter. (Otherwise Mandelbrot
         is uninteresting. */
      new.y += floatparm->y * old.x;
   }
   return(floatbailout());
}

int
TrigPlusZsquaredfpFractal(void)
{
   /* From Scientific American, July 1989 */
   /* A Biomorph                          */
   /* z(n+1) = trig(z(n))+z(n)**2+C       */

   CMPLXtrig0(old,new);
   new.x += tempsqrx - tempsqry + floatparm->x;
   new.y += 2.0 * old.x * old.y + floatparm->y;
   return(floatbailout());
}

int
Richard8fpFractal(void)
{
   /*  Richard8 {c = z = pixel: z=sin(z)+sin(pixel),|z|<=50} */
   CMPLXtrig0(old,new);
/*   CMPLXtrig1(*floatparm,tmp); */
   new.x += tmp.x;
   new.y += tmp.y;
   return(floatbailout());
}

int
PopcornFractal_Old(void)
{
   tmp = old;
   tmp.x *= 3.0;
   tmp.y *= 3.0;
   FPUsincos(&tmp.x,&sinx,&cosx);
   FPUsincos(&tmp.y,&siny,&cosy);
   tmp.x = sinx/cosx + old.x;
   tmp.y = siny/cosy + old.y;
   FPUsincos(&tmp.x,&sinx,&cosx);
   FPUsincos(&tmp.y,&siny,&cosy);
   new.x = old.x - parm.x*siny;
   new.y = old.y - parm.x*sinx;
   /*
   new.x = old.x - parm.x*sin(old.y+tan(3*old.y));
   new.y = old.y - parm.x*sin(old.x+tan(3*old.x));
   */
   if(plot == noplot)
   {
      plot_orbit(new.x,new.y,1+row%colors);
      old = new;
   }
   else
   /* FLOATBAILOUT(); */
   /* PB The above line was weird, not what it seems to be!  But, bracketing
         it or always doing it (either of which seem more likely to be what
         was intended) changes the image for the worse, so I'm not touching it.
         Same applies to int form in next routine. */
   /* PB later: recoded inline, still leaving it weird */
      tempsqrx = sqr(new.x);
   tempsqry = sqr(new.y);
   if((magnitude = tempsqrx + tempsqry) >= rqlim) return(1);
   old = new;
   return(0);
}

int
PopcornFractal(void)
{
   tmp = old;
   tmp.x *= 3.0;
   tmp.y *= 3.0;
   FPUsincos(&tmp.x,&sinx,&cosx);
   FPUsincos(&tmp.y,&siny,&cosy);
   tmp.x = sinx/cosx + old.x;
   tmp.y = siny/cosy + old.y;
   FPUsincos(&tmp.x,&sinx,&cosx);
   FPUsincos(&tmp.y,&siny,&cosy);
   new.x = old.x - parm.x*siny;
   new.y = old.y - parm.x*sinx;
   /*
   new.x = old.x - parm.x*sin(old.y+tan(3*old.y));
   new.y = old.y - parm.x*sin(old.x+tan(3*old.x));
   */
   if(plot == noplot)
   {
      plot_orbit(new.x,new.y,1+row%colors);
      old = new;
   }
   /* else */
   /* FLOATBAILOUT(); */
   /* PB The above line was weird, not what it seems to be!  But, bracketing
         it or always doing it (either of which seem more likely to be what
         was intended) changes the image for the worse, so I'm not touching it.
         Same applies to int form in next routine. */
   /* PB later: recoded inline, still leaving it weird */
   /* JCO: sqr's should always be done, else magnitude could be wrong */
   tempsqrx = sqr(new.x);
   tempsqry = sqr(new.y);
   if((magnitude = tempsqrx + tempsqry) >= rqlim
     || fabs(new.x) > rqlim2 || fabs(new.y) > rqlim2 )
           return(1);
   old = new;
   return(0);
}

/* Popcorn generalization proposed by HB  */

int
PopcornFractalFn(void)
{
   _CMPLX tmpx;
   _CMPLX tmpy;

   /* tmpx contains the generalized value of the old real "x" equation */
   CMPLXtimesreal(parm2,old.y,tmp);  /* tmp = (C * old.y)         */
   CMPLXtrig1(tmp,tmpx);             /* tmpx = trig1(tmp)         */
   tmpx.x += old.y;                  /* tmpx = old.y + trig1(tmp) */
   CMPLXtrig0(tmpx,tmp);             /* tmp = trig0(tmpx)         */
   CMPLXmult(tmp,parm,tmpx);         /* tmpx = tmp * h            */

   /* tmpy contains the generalized value of the old real "y" equation */
   CMPLXtimesreal(parm2,old.x,tmp);  /* tmp = (C * old.x)         */
   CMPLXtrig3(tmp,tmpy);             /* tmpy = trig3(tmp)         */
   tmpy.x += old.x;                  /* tmpy = old.x + trig1(tmp) */
   CMPLXtrig2(tmpy,tmp);             /* tmp = trig2(tmpy)         */

   CMPLXmult(tmp,parm,tmpy);         /* tmpy = tmp * h            */

   new.x = old.x - tmpx.x - tmpy.y;
   new.y = old.y - tmpy.x - tmpx.y;

   if(plot == noplot)
   {
      plot_orbit(new.x,new.y,1+row%colors);
      old = new;
   }

   tempsqrx = sqr(new.x);
   tempsqry = sqr(new.y);
   if((magnitude = tempsqrx + tempsqry) >= rqlim
     || fabs(new.x) > rqlim2 || fabs(new.y) > rqlim2 )
           return(1);
   old = new;
   return(0);
}

int MarksCplxMand(void)
{
   tmp.x = tempsqrx - tempsqry;
   tmp.y = 2*old.x*old.y;
   FPUcplxmul(&tmp, &coefficient, &new);
   new.x += floatparm->x;
   new.y += floatparm->y;
   return(floatbailout());
}

int SpiderfpFractal(void)
{
   /* Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
   new.x = tempsqrx - tempsqry + tmp.x;
   new.y = 2 * old.x * old.y + tmp.y;
   tmp.x = tmp.x/2 + new.x;
   tmp.y = tmp.y/2 + new.y;
   return(floatbailout());
}

int
TetratefpFractal(void)
{
   /* Tetrate(XAXIS) { c=z=pixel: z=c^z, |z|<=(P1+3) } */
   new = ComplexPower(*floatparm,old);
   return(floatbailout());
}

int
ZXTrigPlusZfpFractal(void)
{
   /* z = (p1*z*trig(z))+p2*z */
   CMPLXtrig0(old,tmp);          /* tmp  = trig(old)             */
   CMPLXmult(parm,tmp,tmp);      /* tmp  = p1*trig(old)          */
   CMPLXmult(old,tmp,tmp2);      /* tmp2 = p1*old*trig(old)      */
   CMPLXmult(parm2,old,tmp);     /* tmp  = p2*old                */
   CMPLXadd(tmp2,tmp,new);       /* new  = p1*trig(old) + p2*old */
   return(floatbailout());
}

int
ScottZXTrigPlusZfpFractal(void)
{
   /* z = (z*trig(z))+z */
   CMPLXtrig0(old,tmp);         /* tmp  = trig(old)       */
   CMPLXmult(old,tmp,new);       /* new  = old*trig(old)   */
   CMPLXadd(new,old,new);        /* new  = trig(old) + old */
   return(floatbailout());
}

int
SkinnerZXTrigSubZfpFractal(void)
{
   /* z = (z*trig(z))-z */
   CMPLXtrig0(old,tmp);         /* tmp  = trig(old)       */
   CMPLXmult(old,tmp,new);       /* new  = old*trig(old)   */
   CMPLXsub(new,old,new);        /* new  = trig(old) - old */
   return(floatbailout());
}

int
Sqr1overTrigfpFractal(void)
{
   /* z = sqr(1/trig(z)) */
   CMPLXtrig0(old,old);
   CMPLXrecip(old,old);
   CMPLXsqr(old,new);
   return(floatbailout());
}

int
TrigPlusTrigfpFractal(void)
{
   /* z = trig0(z)*p1+trig1(z)*p2 */
   CMPLXtrig0(old,tmp);
   CMPLXmult(parm,tmp,tmp);
   CMPLXtrig1(old,old);
   CMPLXmult(parm2,old,old);
   CMPLXadd(tmp,old,new);
   return(floatbailout());
}

/* The following four fractals are based on the idea of parallel
   or alternate calculations.  The shift is made when the mod
   reaches a given value.  JCO  5/6/92 */

int
LambdaTrigOrTrigfpFractal(void)
{
   /* z = trig0(z)*p1 if mod(old) < p2.x and
          trig1(z)*p1 if mod(old) >= p2.x */
   if (CMPLXmod(old) < parm2.x){
     CMPLXtrig0(old,old);
     FPUcplxmul(floatparm,&old,&new);}
   else{
     CMPLXtrig1(old,old);
     FPUcplxmul(floatparm,&old,&new);}
   return(floatbailout());
}

int
JuliaTrigOrTrigfpFractal(void)
{
   /* z = trig0(z)+p1 if mod(old) < p2.x and
          trig1(z)+p1 if mod(old) >= p2.x */
   if (CMPLXmod(old) < parm2.x){
     CMPLXtrig0(old,old);
     CMPLXadd(*floatparm,old,new);}
   else{
     CMPLXtrig1(old,old);
     CMPLXadd(*floatparm,old,new);}
   return(floatbailout());
}

int AplusOne, Ap1deg;

int
HalleyFractal(void)
{
   /*  X(X^a - 1) = 0, Halley Map */
   /*  a = parm.x = degree, relaxation coeff. = parm.y, epsilon = parm2.x  */

int ihal;
_CMPLX XtoAlessOne, XtoA, XtoAplusOne; /* a-1, a, a+1 */
_CMPLX FX, F1prime, F2prime, Halnumer1, Halnumer2, Haldenom;
_CMPLX relax;

   XtoAlessOne = old;
   for(ihal=2; ihal<degree; ihal++) {
     FPUcplxmul(&old, &XtoAlessOne, &XtoAlessOne);
   }
   FPUcplxmul(&old, &XtoAlessOne, &XtoA);
   FPUcplxmul(&old, &XtoA, &XtoAplusOne);

   CMPLXsub(XtoAplusOne, old, FX);        /* FX = X^(a+1) - X  = F */
   F2prime.x = Ap1deg * XtoAlessOne.x; /* Ap1deg in setup */
   F2prime.y = Ap1deg * XtoAlessOne.y;        /* F" */

   F1prime.x = AplusOne * XtoA.x - 1.0;
   F1prime.y = AplusOne * XtoA.y;                             /*  F'  */

   FPUcplxmul(&F2prime, &FX, &Halnumer1);                  /*  F * F"  */
   Haldenom.x = F1prime.x + F1prime.x;
   Haldenom.y = F1prime.y + F1prime.y;                     /*  2 * F'  */

   FPUcplxdiv(&Halnumer1, &Haldenom, &Halnumer1);         /*  F"F/2F'  */
   CMPLXsub(F1prime, Halnumer1, Halnumer2);          /*  F' - F"F/2F'  */
   FPUcplxdiv(&FX, &Halnumer2, &Halnumer2);
   /* parm.y is relaxation coef. */
   /* new.x = old.x - (parm.y * Halnumer2.x);
   new.y = old.y - (parm.y * Halnumer2.y); */
   relax.x = parm.y;
   relax.y = param[3];
   FPUcplxmul(&relax, &Halnumer2, &Halnumer2);
   new.x = old.x - Halnumer2.x;
   new.y = old.y - Halnumer2.y;
   return(Halleybailout());
}

int
PhoenixFractal(void)
{
/* z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n) */
   tmp.x = old.x * old.y;
   new.x = tempsqrx - tempsqry + floatparm->x + (floatparm->y * tmp2.x);
   new.y = (tmp.x + tmp.x) + (floatparm->y * tmp2.y);
   tmp2 = old; /* set tmp2 to Y value */
   return(floatbailout());
}

int
PhoenixFractalcplx(void)
{
/* z(n+1) = z(n)^2 + p1 + p2*y(n),  y(n+1) = z(n) */
   tmp.x = old.x * old.y;
   new.x = tempsqrx - tempsqry + floatparm->x + (parm2.x * tmp2.x) - (parm2.y * tmp2.y);
   new.y = (tmp.x + tmp.x) + floatparm->y + (parm2.x * tmp2.y) + (parm2.y * tmp2.x);
   tmp2 = old; /* set tmp2 to Y value */
   return(floatbailout());
}

int
PhoenixPlusFractal(void)
{
/* z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n) */
int i;
_CMPLX oldplus, newminus;
   oldplus = old;
   tmp = old;
   for(i=1; i<degree; i++) { /* degree >= 2, degree=degree-1 in setup */
     FPUcplxmul(&old, &tmp, &tmp); /* = old^(degree-1) */
   }
   oldplus.x += floatparm->x;
   FPUcplxmul(&tmp, &oldplus, &newminus);
   new.x = newminus.x + (floatparm->y * tmp2.x);
   new.y = newminus.y + (floatparm->y * tmp2.y);
   tmp2 = old; /* set tmp2 to Y value */
   return(floatbailout());
}

int
PhoenixMinusFractal(void)
{
/* z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n) */
int i;
_CMPLX oldsqr, newminus;
   FPUcplxmul(&old, &old, &oldsqr);
   tmp = old;
   for(i=1; i<degree; i++) { /* degree >= 3, degree=degree-2 in setup */
     FPUcplxmul(&old, &tmp, &tmp); /* = old^(degree-2) */
   }
   oldsqr.x += floatparm->x;
   FPUcplxmul(&tmp, &oldsqr, &newminus);
   new.x = newminus.x + (floatparm->y * tmp2.x);
   new.y = newminus.y + (floatparm->y * tmp2.y);
   tmp2 = old; /* set tmp2 to Y value */
   return(floatbailout());
}

int
PhoenixCplxPlusFractal(void)
{
/* z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n) */
int i;
_CMPLX oldplus, newminus;
   oldplus = old;
   tmp = old;
   for(i=1; i<degree; i++) { /* degree >= 2, degree=degree-1 in setup */
     FPUcplxmul(&old, &tmp, &tmp); /* = old^(degree-1) */
   }
   oldplus.x += floatparm->x;
   oldplus.y += floatparm->y;
   FPUcplxmul(&tmp, &oldplus, &newminus);
   FPUcplxmul(&parm2, &tmp2, &tmp);
   new.x = newminus.x + tmp.x;
   new.y = newminus.y + tmp.y;
   tmp2 = old; /* set tmp2 to Y value */
   return(floatbailout());
}

int
PhoenixCplxMinusFractal(void)
{
/* z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n) */
int i;
_CMPLX oldsqr, newminus;
   FPUcplxmul(&old, &old, &oldsqr);
   tmp = old;
   for(i=1; i<degree; i++) { /* degree >= 3, degree=degree-2 in setup */
     FPUcplxmul(&old, &tmp, &tmp); /* = old^(degree-2) */
   }
   oldsqr.x += floatparm->x;
   oldsqr.y += floatparm->y;
   FPUcplxmul(&tmp, &oldsqr, &newminus);
   FPUcplxmul(&parm2, &tmp2, &tmp);
   new.x = newminus.x + tmp.x;
   new.y = newminus.y + tmp.y;
   tmp2 = old; /* set tmp2 to Y value */
   return(floatbailout());
}

int
ScottTrigPlusTrigfpFractal(void)
{
   /* z = trig0(z)+trig1(z) */
   CMPLXtrig0(old,tmp);
   CMPLXtrig1(old,tmp2);
   CMPLXadd(tmp,tmp2,new);
   return(floatbailout());
}

int
SkinnerTrigSubTrigfpFractal(void)
{
   /* z = trig0(z)-trig1(z) */
   CMPLXtrig0(old,tmp);
   CMPLXtrig1(old,tmp2);
   CMPLXsub(tmp,tmp2,new);
   return(floatbailout());
}

int
TrigXTrigfpFractal(void)
{
   /* z = trig0(z)*trig1(z) */
   CMPLXtrig0(old,tmp);
   CMPLXtrig1(old,old);
   CMPLXmult(tmp,old,new);
   return(floatbailout());
}

int
TrigPlusSqrfpFractal(void) /* generalization of Scott and Skinner types */
{
   /* { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT } */
   CMPLXtrig0(old,tmp);     /* tmp = trig(old)                     */
   CMPLXmult(parm,tmp,new); /* new = parm*trig(old)                */
   CMPLXsqr_old(tmp);        /* tmp = sqr(old)                      */
   CMPLXmult(parm2,tmp,tmp2); /* tmp = parm2*sqr(old)                */
   CMPLXadd(new,tmp2,new);    /* new = parm*trig(old)+parm2*sqr(old) */
   return(floatbailout());
}

int
ScottTrigPlusSqrfpFractal(void) /* float version */
{
   /* { z=pixel: z=sin(z)+sqr(z), |z|<BAILOUT } */
   CMPLXtrig0(old,new);       /* new = trig(old)          */
   CMPLXsqr_old(tmp);          /* tmp = sqr(old)           */
   CMPLXadd(new,tmp,new);      /* new = trig(old)+sqr(old) */
   return(floatbailout());
}

int
SkinnerTrigSubSqrfpFractal(void)
{
   /* { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT } */
   CMPLXtrig0(old,new);       /* new = trig(old) */
   CMPLXsqr_old(tmp);          /* old = sqr(old)  */
   CMPLXsub(new,tmp,new);      /* new = trig(old)-sqr(old) */
   return(floatbailout());
}

int
TrigZsqrdfpFractal(void)
{
   /* { z=pixel: z=trig(z*z), |z|<TEST } */
   CMPLXsqr_old(tmp);
   CMPLXtrig0(tmp,new);
   return(floatbailout());
}

int
SqrTrigfpFractal(void)
{
   /* SZSB(XYAXIS) { z=pixel, TEST=(p1+3): z=sin(z)*sin(z), |z|<TEST} */
   CMPLXtrig0(old,tmp);
   CMPLXsqr(tmp,new);
   return(floatbailout());
}

int
Magnet1Fractal(void)    /*    Z = ((Z**2 + C - 1)/(2Z + C - 2))**2    */
  {                   /*  In "Beauty of Fractals", code by Kev Allen. */
    _CMPLX top, bot, tmp;
    double div;

    top.x = tempsqrx - tempsqry + floatparm->x - 1; /* top = Z**2+C-1 */
    top.y = old.x * old.y;
    top.y = top.y + top.y + floatparm->y;

    bot.x = old.x + old.x + floatparm->x - 2;       /* bot = 2*Z+C-2  */
    bot.y = old.y + old.y + floatparm->y;

    div = bot.x*bot.x + bot.y*bot.y;                /* tmp = top/bot  */
    if (div < FLT_MIN) return(1);
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    new.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);      /* Z = tmp**2     */
    new.y = tmp.x * tmp.y;
    new.y += new.y;

    return(floatbailout());
  }

int
Magnet2Fractal(void)  /* Z = ((Z**3 + 3(C-1)Z + (C-1)(C-2)  ) /      */
                    /*       (3Z**2 + 3(C-2)Z + (C-1)(C-2)+1) )**2  */
  {                 /*   In "Beauty of Fractals", code by Kev Allen.  */
    _CMPLX top, bot, tmp;
    double div;

    top.x = old.x * (tempsqrx-tempsqry-tempsqry-tempsqry + T_Cm1.x)
          - old.y * T_Cm1.y + T_Cm1Cm2.x;
    top.y = old.y * (tempsqrx+tempsqrx+tempsqrx-tempsqry + T_Cm1.x)
          + old.x * T_Cm1.y + T_Cm1Cm2.y;

    bot.x = tempsqrx - tempsqry;
    bot.x = bot.x + bot.x + bot.x
          + old.x * T_Cm2.x - old.y * T_Cm2.y
          + T_Cm1Cm2.x + 1.0;
    bot.y = old.x * old.y;
    bot.y += bot.y;
    bot.y = bot.y + bot.y + bot.y
          + old.x * T_Cm2.y + old.y * T_Cm2.x
          + T_Cm1Cm2.y;

    div = bot.x*bot.x + bot.y*bot.y;                /* tmp = top/bot  */
    if (div < FLT_MIN) return(1);
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    new.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);      /* Z = tmp**2     */
    new.y = tmp.x * tmp.y;
    new.y += new.y;

    return(floatbailout());
  }

int
LambdaTrigfpFractal(void)
{
   FLOATXYTRIGBAILOUT();
   CMPLXtrig0(old,tmp);              /* tmp = trig(old)           */
   CMPLXmult(*floatparm,tmp,new);   /* new = longparm*trig(old)  */
   old = new;
   return(0);
}

int
LambdaTrigfpFractal1(void)
{
   FLOATTRIGBAILOUT(); /* sin,cos */
   CMPLXtrig0(old,tmp);              /* tmp = trig(old)           */
   CMPLXmult(*floatparm,tmp,new);   /* new = longparm*trig(old)  */
   old = new;
   return(0);
}

int
LambdaTrigfpFractal2(void)
{
   FLOATHTRIGBAILOUT(); /* sinh,cosh */
   CMPLXtrig0(old,tmp);              /* tmp = trig(old)           */
   CMPLXmult(*floatparm,tmp,new);   /* new = longparm*trig(old)  */
   old = new;
   return(0);
}

int
ManOWarfpFractal(void)
{
   /* From Art Matrix via Lee Skinner */
   /* note that fast >= 287 equiv in fracsuba.asm must be kept in step */
   new.x = tempsqrx - tempsqry + tmp.x + floatparm->x;
   new.y = 2.0 * old.x * old.y + tmp.y + floatparm->y;
   tmp = old;
   return(floatbailout());
}

/*
   MarksMandelPwr (XAXIS) {
      z = pixel, c = z ^ (z - 1):
         z = c * sqr(z) + pixel,
      |z| <= 4
   }
*/

int
MarksMandelPwrfpFractal(void)
{
   CMPLXtrig0(old,new);
   CMPLXmult(tmp,new,new);
   new.x += floatparm->x;
   new.y += floatparm->y;
   return(floatbailout());
}

/* I was coding Marksmandelpower and failed to use some temporary
   variables. The result was nice, and since my name is not on any fractal,
   I thought I would immortalize myself with this error!
                Tim Wegner */

int
TimsErrorfpFractal(void)
{
   CMPLXtrig0(old,new);
   new.x = new.x * tmp.x - new.y * tmp.y;
   new.y = new.x * tmp.y - new.y * tmp.x;
   new.x += floatparm->x;
   new.y += floatparm->y;
   return(floatbailout());
}

int
CirclefpFractal(void)
{
   long i;
   i = (long)(param[0]*(tempsqrx+tempsqry));
   coloriter = i%colors;
   return(1);
}

/* -------------------------------------------------------------------- */
/*              Initialization (once per pixel) routines                                                */
/* -------------------------------------------------------------------- */

#ifdef XFRACT
/* this code translated to asm - lives in newton.asm */
/* transform points with reciprocal function */
void invertz2(_CMPLX *z)
{
   z->x = dxpixel();
   z->y = dypixel();
   z->x -= f_xcenter; z->y -= f_ycenter;  /* Normalize values to center of circle */

   tempsqrx = sqr(z->x) + sqr(z->y);  /* Get old radius */
   if(fabs(tempsqrx) > FLT_MIN)
      tempsqrx = f_radius / tempsqrx;
   else
      tempsqrx = FLT_MAX;   /* a big number, but not TOO big */
   z->x *= tempsqrx;      z->y *= tempsqrx;      /* Perform inversion */
   z->x += f_xcenter; z->y += f_ycenter; /* Renormalize */
}
#endif


int marksmandelfp_per_pixel(void)
{
   /* marksmandel */

   if(invert)
      invertz2(&init);
   else {
      init.x = dxpixel();
      if(save_release >= 2004)
         init.y = dypixel();
   }

   if(useinitorbit == 1)
      old = initorbit;
   else
      old = init;

   old.x += parm.x;      /* initial pertubation of parameters set */
   old.y += parm.y;

   tempsqrx = sqr(old.x);
   tempsqry = sqr(old.y);

   if(c_exp > 3)
      cpower(&old,c_exp-1,&coefficient);
   else if(c_exp == 3) {
      coefficient.x = tempsqrx - tempsqry;
      coefficient.y = old.x * old.y * 2;
   }
   else if(c_exp == 2)
      coefficient = old;
   else if(c_exp < 2) {
      coefficient.x = 1.0;
      coefficient.y = 0.0;
   }

   return(1); /* 1st iteration has been done */
}

int
marks_mandelpwrfp_per_pixel(void)
{
   mandelfp_per_pixel();
   tmp = old;
   tmp.x -= 1;
   CMPLXpwr(old,tmp,tmp);
   return(1);
}

int mandelfp_per_pixel(void)
{
   /* floating point mandelbrot */
   /* mandelfp */

   if(invert)
      invertz2(&init);
   else {
      init.x = dxpixel();
      if(save_release >= 2004)
         init.y = dypixel();
   }
    switch (fractype)
      {
        case MAGNET2M:
            FloatPreCalcMagnet2();
        case MAGNET1M:           /* Crit Val Zero both, but neither   */
            old.x = old.y = 0.0; /* is of the form f(Z,C) = Z*g(Z)+C  */
            break;
        case MANDELLAMBDAFP:            /* Critical Value 0.5 + 0.0i  */
            old.x = 0.5;
            old.y = 0.0;
            break;
        default:
            old = init;
            break;
      }

   /* alter init value */
   if(useinitorbit == 1)
      old = initorbit;
   else if(useinitorbit == 2)
      old = init;

   if((inside == BOF60 || inside == BOF61) && !nobof)
   {
      /* kludge to match "Beauty of Fractals" picture since we start
         Mandelbrot iteration with init rather than 0 */
      old.x = parm.x; /* initial pertubation of parameters set */
      old.y = parm.y;
      coloriter = -1;
   }
   else
   {
     old.x += parm.x;
     old.y += parm.y;
   }
   tmp = init; /* for spider */
   tempsqrx = sqr(old.x);  /* precalculated value for regular Mandelbrot */
   tempsqry = sqr(old.y);
   return(1); /* 1st iteration has been done */
}

int juliafp_per_pixel(void)
{
   /* floating point julia */
   /* juliafp */
   if(invert)
      invertz2(&old);
   else
   {
      old.x = dxpixel();
      old.y = dypixel();
   }
   tempsqrx = sqr(old.x);  /* precalculated value for regular Julia */
   tempsqry = sqr(old.y);
   tmp = old;
   return(0);
}

int
otherrichard8fp_per_pixel(void)
{
    othermandelfp_per_pixel();
    CMPLXtrig1(*floatparm,tmp);
    CMPLXmult(tmp,parm2,tmp);
    return(1);
}

int othermandelfp_per_pixel(void)
{
   if(invert)
      invertz2(&init);
   else {
      init.x = dxpixel();
      if(save_release >= 2004)
         init.y = dypixel();
   }

   if(useinitorbit == 1)
      old = initorbit;
   else
      old = init;

   old.x += parm.x;      /* initial pertubation of parameters set */
   old.y += parm.y;

   return(1); /* 1st iteration has been done */
}

int Halley_per_pixel(void)
{
   if(invert)
      invertz2(&init);
   else {
      init.x = dxpixel();
      if(save_release >= 2004)
         init.y = dypixel();
   }
   old = init;
   return(0); /* 1st iteration is not done */
}

int otherjuliafp_per_pixel(void)
{
   if(invert)
      invertz2(&old);
   else
   {
      old.x = dxpixel();
      old.y = dypixel();
   }
   return(0);
}

#define Q0 0
#define Q1 0

int quaternionjulfp_per_pixel(void)
{
   old.x = dxpixel();
   old.y = dypixel();
   floatparm->x = param[4];
   floatparm->y = param[5];
   qc  = param[0];
   qci = param[1];
   qcj = param[2];
   qck = param[3];
   return(0);
}

int quaternionfp_per_pixel(void)
{
   old.x = 0;
   old.y = 0;
   floatparm->x = 0;
   floatparm->y = 0;
   qc  = dxpixel();
   qci = dypixel();
   qcj = param[2];
   qck = param[3];
   return(0);
}

int MarksCplxMandperp(void)
{
   if(invert)
      invertz2(&init);
   else {
      init.x = dxpixel();
      if(save_release >= 2004)
         init.y = dypixel();
   }
   old.x = init.x + parm.x; /* initial pertubation of parameters set */
   old.y = init.y + parm.y;
   tempsqrx = sqr(old.x);  /* precalculated value */
   tempsqry = sqr(old.y);
   coefficient = ComplexPower(init, pwr);
   return(1);
}

int phoenix_per_pixel(void)
{
   if(invert)
      invertz2(&old);
   else
   {
      old.x = dxpixel();
      old.y = dypixel();
   }
   tempsqrx = sqr(old.x);  /* precalculated value */
   tempsqry = sqr(old.y);
   tmp2.x = 0; /* use tmp2 as the complex Y value */
   tmp2.y = 0;
   return(0);
}

int mandphoenix_per_pixel(void)
{
   if(invert)
      invertz2(&init);
   else {
      init.x = dxpixel();
      if(save_release >= 2004)
         init.y = dypixel();
   }

   if(useinitorbit == 1)
      old = initorbit;
   else
      old = init;

   old.x += parm.x;      /* initial pertubation of parameters set */
   old.y += parm.y;
   tempsqrx = sqr(old.x);  /* precalculated value */
   tempsqry = sqr(old.y);
   tmp2.x = 0;
   tmp2.y = 0;
   return(1); /* 1st iteration has been done */
}

int
QuaternionFPFractal(void)
{
   double a0,a1,a2,a3,n0,n1,n2,n3;
   a0 = old.x;
   a1 = old.y;
   a2 = floatparm->x;
   a3 = floatparm->y;

   n0 = a0*a0-a1*a1-a2*a2-a3*a3 + qc;
   n1 = 2*a0*a1 + qci;
   n2 = 2*a0*a2 + qcj;
   n3 = 2*a0*a3 + qck;
   /* Check bailout */
   magnitude = a0*a0+a1*a1+a2*a2+a3*a3;
   if (magnitude>rqlim) {
       return 1;
   }
   old.x = new.x = n0;
   old.y = new.y = n1;
   floatparm->x = n2;
   floatparm->y = n3;
   return(0);
}

int
HyperComplexFPFractal(void)
{
   _HCMPLX hold, hnew;
   hold.x = old.x;
   hold.y = old.y;
   hold.z = floatparm->x;
   hold.t = floatparm->y;

/*   HComplexSqr(&hold,&hnew); */
   HComplexTrig0(&hold,&hnew);

   hnew.x += qc;
   hnew.y += qci;
   hnew.z += qcj;
   hnew.t += qck;

   old.x = new.x = hnew.x;
   old.y = new.y = hnew.y;
   floatparm->x = hnew.z;
   floatparm->y = hnew.t;

   /* Check bailout */
   magnitude = sqr(old.x)+sqr(old.y)+sqr(floatparm->x)+sqr(floatparm->y);
   if (magnitude>rqlim) {
       return 1;
   }
   return(0);
}

int
VLfpFractal(void) /* Beauty of Fractals pp. 125 - 127 */
{
   double a, b, ab, half, u, w, xy;

   half = param[0] / 2.0;
   xy = old.x * old.y;
   u = old.x - xy;
   w = -old.y + xy;
   a = old.x + param[1] * u;
   b = old.y + param[1] * w;
   ab = a * b;
   new.x = old.x + half * (u + (a - ab));
   new.y = old.y + half * (w + (-b + ab));
   return(floatbailout());
}

int
EscherfpFractal(void) /* Science of Fractal Images pp. 185, 187 */
{
   _CMPLX oldtest, newtest, testsqr;
   double testsize = 0.0;
   long testiter = 0;

   new.x = tempsqrx - tempsqry; /* standard Julia with C == (0.0, 0.0i) */
   new.y = 2.0 * old.x * old.y;
   oldtest.x = new.x * 15.0;    /* scale it */
   oldtest.y = new.y * 15.0;
   testsqr.x = sqr(oldtest.x);  /* set up to test with user-specified ... */
   testsqr.y = sqr(oldtest.y);  /*    ... Julia as the target set */
   while (testsize <= rqlim && testiter < maxit) /* nested Julia loop */
   {
      newtest.x = testsqr.x - testsqr.y + param[0];
      newtest.y = 2.0 * oldtest.x * oldtest.y + param[1];
      testsize = (testsqr.x = sqr(newtest.x)) + (testsqr.y = sqr(newtest.y));
      oldtest = newtest;
      testiter++;
   }
   if (testsize > rqlim) return(floatbailout()); /* point not in target set */
   else /* make distinct level sets if point stayed in target set */
   {
      coloriter = ((3L * coloriter) % 255L) + 1L;
      return 1;
   }
}

/*
 * The following functions calculate the real and imaginary complex
 * coordinates of the point in the complex plane corresponding to
 * the screen coordinates (col,row) at the current zoom corners
 * settings. The functions come in two flavors. One looks up the pixel
 * values using the precalculated grid arrays dx0, dx1, dy0, and dy1,
 * which has a speed advantage but is limited to MAXPIXELS image
 * dimensions. The other calculates the complex coordinates at a
 * cost of two additions and two multiplications for each component,
 * but works at any resolution.
 *
 * With Microsoft C's _fastcall keyword, the function call overhead
 * appears to be negligible. It also appears that the speed advantage
 * of the lookup vs the calculation is negligible on machines with
 * coprocessors. Bert Tyler's original implementation was designed for
 * machines with no coprocessor; on those machines the saving was
 * significant. For the time being, the table lookup capability will
 * be maintained.
 */

/* Real component, grid lookup version - requires dx0/dx1 arrays */
static double _fastcall dxpixel_grid(void)
{
   return(dx0[col]+dx1[row]);
}

/* Real component, calculation version - does not require arrays */
static double _fastcall dxpixel_calc(void)
{
   return((double)(xxmin + col*delxx + row*delxx2));
}

/* Imaginary component, grid lookup version - requires dy0/dy1 arrays */
static double _fastcall dypixel_grid(void)
{
   return(dy0[row]+dy1[col]);
}

/* Imaginary component, calculation version - does not require arrays */
static double _fastcall dypixel_calc(void)
{
   return((double)(yymax - row*delyy - col*delyy2));
}

double (_fastcall *dxpixel)(void) = dxpixel_calc;
double (_fastcall *dypixel)(void) = dypixel_calc;

void set_pixel_calc_functions(void)
{
   if(use_grid)
   {
      dxpixel = dxpixel_grid;
      dypixel = dypixel_grid;
   }
   else
   {
      dxpixel = dxpixel_calc;
      dypixel = dypixel_calc;
   }
}
