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

/* -------------------------------------------------------------------- */
/*              Setup (once per fractal image) routines                 */
/* -------------------------------------------------------------------- */

int
NewtonSetup(void)           /* Newton/NewtBasin Routines */
{
   int i;

   /* set up table of roots of 1 along unit circle */
   degree = (int)parm.x;
   if(degree < 2)
      degree = 3;   /* defaults to 3, but 2 is possible */
   root = 1;

   /* precalculated values */
   roverd       = (double)root / (double)degree;
   d1overd      = (double)(degree - 1) / (double)degree;
   maxcolor     = 0;
   threshold    = .3*PI/degree; /* less than half distance between roots */

   floatmin = FLT_MIN;
   floatmax = FLT_MAX;

   basin = 0;
   if(roots != staticroots) {
      free(roots);
      roots = staticroots;
   }

   if (fractype==NEWTBASIN)
   {
      if(parm.y)
         basin = 2; /*stripes */
      else
         basin = 1;
      if(degree > 16)
      {
         if((roots=(_CMPLX *)malloc(degree*sizeof(_CMPLX)))==NULL)
         {
            roots = staticroots;
            degree = 16;
         }
      }
      else
         roots = staticroots;

      /* list of roots to discover where we converged for newtbasin */
      for(i=0;i<degree;i++)
      {
         roots[i].x = cos(i*twopi/(double)degree);
         roots[i].y = sin(i*twopi/(double)degree);
      }
   }

   param[0] = (double)degree; /* JCO 7/1/92 */
   if (degree%4 == 0)
      symmetry = XYAXIS;
   else
      symmetry = XAXIS;

   calctype=StandardFractal;
   return(1);
}


int
StandaloneSetup(void)
{
   timer(0,curfractalspecific->calctype);
   return(0);           /* effectively disable solid-guessing */
}

int
UnitySetup(void)
{
   periodicitycheck = 0;
   return(1);
}

int
MandelfpSetup(void)
{
   bf_math = 0;
   c_exp = (int)param[2];
   pwr.x = param[2] - 1.0;
   pwr.y = param[3];
   floatparm = &init;
   switch (fractype)
   {
   case MARKSMANDELFP:
      if(c_exp < 1){
         c_exp = 1;
         param[2] = 1;
      }
      if(!(c_exp & 1))
         symmetry = XYAXIS_NOPARM;    /* odd exponents */
      if(c_exp & 1)
         symmetry = XAXIS_NOPARM;
      break;
   case MANDELFP:
        /*
           floating point code could probably be altered to handle many of
           the situations that otherwise are using StandardFractal().
           calcmandfp() can currently handle invert, any rqlim, potflag
           zmag, epsilon cross, and all the current outside options
                                                     Wes Loewer 11/03/91
           Took out support for inside= options, for speed. 7/13/97
        */
        if (debugflag != 90
            && !distest
            && decomp[0] == 0
            && biomorph == -1
            && (inside >= -1)
            /* uncomment this next line if more outside options are added */
            && outside >= -6
            && useinitorbit != 1
            && (soundflag & 0x07) < 2
            && using_jiim == 0 && bailoutest == Mod
            && (orbitsave&2) == 0)
        {
           calctype = calcmandfp; /* the normal case - use calcmandfp */
#ifndef XFRACT
           if (cpu >= 386 && fpu >= 387)
           {
              calcmandfpasmstart_p5();
              calcmandfpasm = (long (*)(void))calcmandfpasm_p5;
           }
           else if (cpu == 286 && fpu >= 287)
           {
              calcmandfpasmstart();
              calcmandfpasm = (long (*)(void))calcmandfpasm_287;
           }
           else
           {
              calcmandfpasmstart();
              calcmandfpasm = (long (*)(void))calcmandfpasm_87;
           }
#else
           {
#ifdef NASM
            if (fpu == -1)
            {
              calcmandfpasmstart_p5();
              calcmandfpasm = (long (*)(void))calcmandfpasm_p5;
            }
            else
#endif
            {
              calcmandfpasmstart();
              calcmandfpasm = (long (*)(void))calcmandfpasm_c;
            }
           }
#endif
        }
        else
        {
           /* special case: use the main processing loop */
           calctype = StandardFractal;
        }
        break;
   case FPMANDELZPOWER:
      if((double)c_exp == param[2] && (c_exp & 1)) /* odd exponents */
         symmetry = XYAXIS_NOPARM;
      if(param[3] != 0)
         symmetry = NOSYM;
      if(param[3] == 0.0 && debugflag != 6000 && (double)c_exp == param[2])
          fractalspecific[fractype].orbitcalc = floatZpowerFractal;
      else
          fractalspecific[fractype].orbitcalc = floatCmplxZpowerFractal;
      break;
   case MAGNET1M:
   case MAGNET2M:
      attr[0].x = 1.0;      /* 1.0 + 0.0i always attracts */
      attr[0].y = 0.0;      /* - both MAGNET1 and MAGNET2 */
      attrperiod[0] = 1;
      attractors = 1;
      break;
   case SPIDERFP:
      if(periodicitycheck==1) /* if not user set */
         periodicitycheck=4;
      break;
      /*
   case MANDELEXP:
      symmetry = XAXIS_NOPARM;
      break; */
/* Added to account for symmetry in manfn+exp and manfn+zsqrd */
/*     JCO 2/29/92 */
   case FPMANTRIGPLUSEXP:
   case FPMANTRIGPLUSZSQRD:
     if(parm.y == 0.0)
        symmetry = XAXIS;
     else
        symmetry = NOSYM;
     if ((trigndx[0] == LOG) || (trigndx[0] == 14)) /* LOG or FLIP */
        symmetry = NOSYM;
      break;
   case QUATFP:
      floatparm = &tmp;
      attractors = 0;
      periodicitycheck = 0;
      break;
   case HYPERCMPLXFP:
      floatparm = &tmp;
      attractors = 0;
      periodicitycheck = 0;
      if(param[2] != 0)
         symmetry = NOSYM;
      if(trigndx[0] == 14) /* FLIP */
        symmetry = NOSYM;
      break;
   case TIMSERRORFP:
      if(trigndx[0] == 14) /* FLIP */
        symmetry = NOSYM;
      break;
   case MARKSMANDELPWRFP:
      if(trigndx[0] == 14) /* FLIP */
        symmetry = NOSYM;
      break;
   default:
      break;
   }
   return(1);
}

int
JuliafpSetup()
{
   c_exp = (int)param[2];
   floatparm = &parm;
   if(fractype==COMPLEXMARKSJUL)
   {
      pwr.x = param[2] - 1.0;
      pwr.y = param[3];
      coefficient = ComplexPower(*floatparm, pwr);
   }
   switch (fractype)
   {
   case JULIAFP:
        /*
           floating point code could probably be altered to handle many of
           the situations that otherwise are using StandardFractal().
           calcmandfp() can currently handle invert, any rqlim, potflag
           zmag, epsilon cross, and all the current outside options
                                                     Wes Loewer 11/03/91
           Took out support for inside= options, for speed. 7/13/97
        */
        if (debugflag != 90
            && !distest
            && decomp[0] == 0
            && biomorph == -1
            && (inside >= -1)
            /* uncomment this next line if more outside options are added */
            && outside >= -6
            && useinitorbit != 1
            && (soundflag & 0x07) < 2
            && !finattract
            && using_jiim == 0 && bailoutest == Mod
            && (orbitsave&2) == 0)
        {
           calctype = calcmandfp; /* the normal case - use calcmandfp */
#ifndef XFRACT
           if (cpu >= 386 && fpu >= 387)
           {
              calcmandfpasmstart_p5();
              calcmandfpasm = (long (*)(void))calcmandfpasm_p5;
           }
           else if (cpu == 286 && fpu >= 287)
           {
              calcmandfpasmstart();
              calcmandfpasm = (long (*)(void))calcmandfpasm_287;
           }
           else

           {
              calcmandfpasmstart();
              calcmandfpasm = (long (*)(void))calcmandfpasm_87;
           }
#else
           {
#ifdef NASM
            if (fpu == -1)
            {
              calcmandfpasmstart_p5();
              calcmandfpasm = (long (*)(void))calcmandfpasm_p5;
            }
            else
#endif
            {
              calcmandfpasmstart();
              calcmandfpasm = (long (*)(void))calcmandfpasm_c;
            }
           }
#endif
        }
        else
        {
           /* special case: use the main processing loop */
           calctype = StandardFractal;
           get_julia_attractor (0.0, 0.0);   /* another attractor? */
        }
        break;
   case FPJULIAZPOWER:
      if((c_exp & 1) || param[3] != 0.0 || (double)c_exp != param[2] )
         symmetry = NOSYM;
      if(param[3] == 0.0 && debugflag != 6000 && (double)c_exp == param[2])
          fractalspecific[fractype].orbitcalc = floatZpowerFractal;
      else
          fractalspecific[fractype].orbitcalc = floatCmplxZpowerFractal;
      get_julia_attractor (param[0], param[1]); /* another attractor? */
      break;
   case MAGNET2J:
      FloatPreCalcMagnet2();
   case MAGNET1J:
      attr[0].x = 1.0;      /* 1.0 + 0.0i always attracts */
      attr[0].y = 0.0;      /* - both MAGNET1 and MAGNET2 */
      attrperiod[0] = 1;
      attractors = 1;
      get_julia_attractor (0.0, 0.0);   /* another attractor? */
      break;
   case LAMBDAFP:
      get_julia_attractor (0.0, 0.0);   /* another attractor? */
      get_julia_attractor (0.5, 0.0);   /* another attractor? */
      break;
#if 0
   case LAMBDAEXP:
      if(parm.y == 0.0)
         symmetry=XAXIS;
      get_julia_attractor (0.0, 0.0);   /* another attractor? */
      break;
#endif
/* Added to account for symmetry in julfn+exp and julfn+zsqrd */
/*     JCO 2/29/92 */
   case FPJULTRIGPLUSEXP:
   case FPJULTRIGPLUSZSQRD:
     if(parm.y == 0.0)
        symmetry = XAXIS;
     else
        symmetry = NOSYM;
     if ((trigndx[0] == LOG) || (trigndx[0] == 14)) /* LOG or FLIP */
        symmetry = NOSYM;
      get_julia_attractor (0.0, 0.0);   /* another attractor? */
      break;
   case HYPERCMPLXJFP:
      if(param[2] != 0)
         symmetry = NOSYM;
      if(trigndx[0] != SQR)
         symmetry=NOSYM;
   case QUATJULFP:
      attractors = 0;   /* attractors broken since code checks r,i not j,k */
      periodicitycheck = 0;
      if(param[4] != 0.0 || param[5] != 0)
         symmetry = NOSYM;
      break;
   case FPPOPCORN:
   case FPPOPCORNJUL:
      {
         int default_functions = 0;
         if(trigndx[0] == SIN &&
            trigndx[1] == TAN &&
            trigndx[2] == SIN &&
            trigndx[3] == TAN &&
            fabs(parm2.x - 3.0) < .0001 &&
            parm2.y == 0 &&
            parm.y == 0)
         {
            default_functions = 1;
            if(fractype == FPPOPCORNJUL)
               symmetry = ORIGIN;
         }
         if(save_release <=1960)
            curfractalspecific->orbitcalc = PopcornFractal_Old;
         else if(default_functions && debugflag == 96)
            curfractalspecific->orbitcalc = PopcornFractal;
         else
            curfractalspecific->orbitcalc = PopcornFractalFn;
         get_julia_attractor (0.0, 0.0);   /* another attractor? */
      }
      break;
   case FPCIRCLE:
      if (inside == STARTRAIL) /* FPCIRCLE locks up when used with STARTRAIL */
          inside = 0; /* arbitrarily set inside = NUMB */
      get_julia_attractor (0.0, 0.0);   /* another attractor? */
      break;
   default:
      get_julia_attractor (0.0, 0.0);   /* another attractor? */
      break;
   }
   return(1);
}

int
TrigPlusSqrfpSetup(void)
{
   curfractalspecific->per_pixel =  juliafp_per_pixel;
   curfractalspecific->orbitcalc =  TrigPlusSqrfpFractal;
   if(parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != 90)
   {
      if(parm2.x == 1.0)        /* Scott variant */
         curfractalspecific->orbitcalc =  ScottTrigPlusSqrfpFractal;
      else if(parm2.x == -1.0)  /* Skinner variant */
         curfractalspecific->orbitcalc =  SkinnerTrigSubSqrfpFractal;
   }
   return(JuliafpSetup());
}

int
TrigPlusTrigfpSetup(void)
{
   FnPlusFnSym();
   if(trigndx[1] == SQR)
      return(TrigPlusSqrfpSetup());
   curfractalspecific->per_pixel =  otherjuliafp_per_pixel;
   curfractalspecific->orbitcalc =  TrigPlusTrigfpFractal;
   if(parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != 90)
   {
      if(parm2.x == 1.0)        /* Scott variant */
         curfractalspecific->orbitcalc =  ScottTrigPlusTrigfpFractal;
      else if(parm2.x == -1.0)  /* Skinner variant */
         curfractalspecific->orbitcalc =  SkinnerTrigSubTrigfpFractal;
   }
   return(JuliafpSetup());
}

int
FnPlusFnSym(void) /* set symmetry matrix for fn+fn type */
{
   static char far fnplusfn[7][7] =
   {/* fn2 ->sin     cos    sinh    cosh   exp    log    sqr  */
   /* fn1 */
   /* sin */ {PI_SYM,XAXIS, XYAXIS, XAXIS, XAXIS, XAXIS, XAXIS},
   /* cos */ {XAXIS, PI_SYM,XAXIS,  XYAXIS,XAXIS, XAXIS, XAXIS},
   /* sinh*/ {XYAXIS,XAXIS, XYAXIS, XAXIS, XAXIS, XAXIS, XAXIS},
   /* cosh*/ {XAXIS, XYAXIS,XAXIS,  XYAXIS,XAXIS, XAXIS, XAXIS},
   /* exp */ {XAXIS, XYAXIS,XAXIS,  XAXIS, XYAXIS,XAXIS, XAXIS},
   /* log */ {XAXIS, XAXIS, XAXIS,  XAXIS, XAXIS, XAXIS, XAXIS},
   /* sqr */ {XAXIS, XAXIS, XAXIS,  XAXIS, XAXIS, XAXIS, XYAXIS}
   };
   if(parm.y == 0.0 && parm2.y == 0.0)
    { if(trigndx[0] < 7 && trigndx[1] < 7)  /* bounds of array JCO 5/6/92*/
        symmetry = fnplusfn[trigndx[0]][trigndx[1]];  /* JCO 5/6/92 */
      if(trigndx[0] == 14 || trigndx[1] == 14) /* FLIP */
        symmetry = NOSYM;
    }                 /* defaults to XAXIS symmetry JCO 5/6/92 */
   else
      symmetry = NOSYM;
   return(0);
}

int
LambdaTrigOrTrigSetup(void)
{
/* default symmetry is ORIGIN  JCO 2/29/92 (changed from PI_SYM) */
   floatparm = &parm;
   if ((trigndx[0] == EXP) || (trigndx[1] == EXP))
      symmetry = NOSYM; /* JCO 1/9/93 */
   if ((trigndx[0] == LOG) || (trigndx[1] == LOG))
      symmetry = XAXIS;
   get_julia_attractor (0.0, 0.0);      /* an attractor? */
   return(1);
}

int
JuliaTrigOrTrigSetup(void)
{
/* default symmetry is XAXIS */
   floatparm = &parm;
   if(parm.y != 0.0)
     symmetry = NOSYM;
   if(trigndx[0] == 14 || trigndx[1] == 14) /* FLIP */
     symmetry = NOSYM;
   get_julia_attractor (0.0, 0.0);      /* an attractor? */
   return(1);
}

int
ManlamTrigOrTrigSetup(void)
{ /* psuedo */
/* default symmetry is XAXIS */
   floatparm = &init;
   if (trigndx[0] == SQR)
      symmetry = NOSYM;
   if ((trigndx[0] == LOG) || (trigndx[1] == LOG))
      symmetry = NOSYM;
   return(1);
}

int
MandelTrigOrTrigSetup(void)
{
/* default symmetry is XAXIS_NOPARM */
   floatparm = &init;
   if ((trigndx[0] == 14) || (trigndx[1] == 14)) /* FLIP  JCO 5/28/92 */
      symmetry = NOSYM;
   return(1);
}


int
ZXTrigPlusZSetup(void)
{
/*   static char far ZXTrigPlusZSym1[] = */
   /* fn1 ->  sin   cos    sinh  cosh exp   log   sqr */
/*           {XAXIS,XYAXIS,XAXIS,XYAXIS,XAXIS,NOSYM,XYAXIS}; */
/*   static char far ZXTrigPlusZSym2[] = */
   /* fn1 ->  sin   cos    sinh  cosh exp   log   sqr */
/*           {NOSYM,ORIGIN,NOSYM,ORIGIN,NOSYM,NOSYM,ORIGIN}; */

   if(param[1] == 0.0 && param[3] == 0.0)
/*      symmetry = ZXTrigPlusZSym1[trigndx[0]]; */
   switch(trigndx[0])
   {
      case COS:   /* changed to two case statments and made any added */
      case COSH:  /* functions default to XAXIS symmetry. JCO 5/25/92 */
      case SQR:
      case 9:   /* 'real' cos */
         symmetry = XYAXIS;
         break;
      case 14:   /* FLIP  JCO 2/29/92 */
         symmetry = YAXIS;
         break;
      case LOG:
         symmetry = NOSYM;
         break;
      default:
         symmetry = XAXIS;
         break;
      }
   else
/*      symmetry = ZXTrigPlusZSym2[trigndx[0]]; */
   switch(trigndx[0])
   {
      case COS:
      case COSH:
      case SQR:
      case 9:   /* 'real' cos */
         symmetry = ORIGIN;
         break;
      case 14:   /* FLIP  JCO 2/29/92 */
         symmetry = NOSYM;
         break;
      default:
         symmetry = NOSYM;
         break;
   }
   curfractalspecific->orbitcalc =  ZXTrigPlusZfpFractal;
   if(parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != 90)
   {
      if(parm2.x == 1.0)     /* Scott variant */
              curfractalspecific->orbitcalc =  ScottZXTrigPlusZfpFractal;
      else if(parm2.x == -1.0)       /* Skinner variant */
              curfractalspecific->orbitcalc =  SkinnerZXTrigSubZfpFractal;
   }
   return(JuliafpSetup());
}

int
LambdaTrigSetup(void)
{
   curfractalspecific->orbitcalc =  LambdaTrigfpFractal;
   switch(trigndx[0])
   {
   case SIN:
   case COS:
   case 9:   /* 'real' cos, added this and default for additional functions */
      symmetry = PI_SYM;
      curfractalspecific->orbitcalc =  LambdaTrigfpFractal1;
      break;
   case SINH:
   case COSH:
      symmetry = ORIGIN;
      curfractalspecific->orbitcalc =  LambdaTrigfpFractal2;
      break;
   case SQR:
      symmetry = ORIGIN;
      break;
   case EXP:
      curfractalspecific->orbitcalc =  LambdaexponentFractal;
      symmetry = NOSYM; /* JCO 1/9/93 */
      break;
   case LOG:
      symmetry = NOSYM;
      break;
   default:   /* default for additional functions */
      symmetry = ORIGIN;  /* JCO 5/8/92 */
      break;
   }
   get_julia_attractor (0.0, 0.0);      /* an attractor? */
   return(JuliafpSetup());
}

int
JuliafnPlusZsqrdSetup(void)
{
/*   static char far fnpluszsqrd[] = */
   /* fn1 ->  sin   cos    sinh  cosh   sqr    exp   log  */
   /* sin    {NOSYM,ORIGIN,NOSYM,ORIGIN,ORIGIN,NOSYM,NOSYM}; */

/*   symmetry = fnpluszsqrd[trigndx[0]];   JCO  5/8/92 */
   switch(trigndx[0]) /* fix sqr symmetry & add additional functions */
   {
   case COS: /* cosxx */
   case COSH:
   case SQR:
   case 9:   /* 'real' cos */
   case 10:  /* tan */
   case 11:  /* tanh */
   symmetry = ORIGIN;
    /* default is for NOSYM symmetry */
   }
   return(JuliafpSetup());
}

int
SqrTrigSetup(void)
{
/*   static char far SqrTrigSym[] = */
   /* fn1 ->  sin    cos    sinh   cosh   sqr    exp   log  */
/*           {PI_SYM,PI_SYM,XYAXIS,XYAXIS,XYAXIS,XAXIS,XAXIS}; */
/*   symmetry = SqrTrigSym[trigndx[0]];      JCO  5/9/92 */
   switch(trigndx[0]) /* fix sqr symmetry & add additional functions */
   {
   case SIN:
   case COS: /* cosxx */
   case 9:   /* 'real' cos */
   symmetry = PI_SYM;
    /* default is for XAXIS symmetry */
   }
   return(JuliafpSetup());
}

int
FnXFnSetup(void)
{
   static char far fnxfn[7][7] =
   {/* fn2 ->sin     cos    sinh    cosh  exp    log    sqr */
   /* fn1 */
   /* sin */ {PI_SYM,YAXIS, XYAXIS,XYAXIS,XAXIS, NOSYM, XYAXIS},
   /* cos */ {YAXIS, PI_SYM,XYAXIS,XYAXIS,XAXIS, NOSYM, XYAXIS},
   /* sinh*/ {XYAXIS,XYAXIS,XYAXIS,XYAXIS,XAXIS, NOSYM, XYAXIS},
   /* cosh*/ {XYAXIS,XYAXIS,XYAXIS,XYAXIS,XAXIS, NOSYM, XYAXIS},
   /* exp */ {XAXIS, XAXIS, XAXIS, XAXIS, XAXIS, NOSYM, XYAXIS},
   /* log */ {NOSYM, NOSYM, NOSYM, NOSYM, NOSYM, XAXIS, NOSYM},
   /* sqr */ {XYAXIS,XYAXIS,XYAXIS,XYAXIS,XYAXIS,NOSYM, XYAXIS},
   };
   /*
   if(trigndx[0]==EXP || trigndx[0]==LOG || trigndx[1]==EXP || trigndx[1]==LOG)
      symmetry = XAXIS;
   else if((trigndx[0]==SIN && trigndx[1]==SIN)||(trigndx[0]==COS && trigndx[1]==COS))
      symmetry = PI_SYM;
   else if((trigndx[0]==SIN && trigndx[1]==COS)||(trigndx[0]==COS && trigndx[1]==SIN))
      symmetry = YAXIS;
   else
      symmetry = XYAXIS;
   */
   if(trigndx[0] < 7 && trigndx[1] < 7)  /* bounds of array JCO 5/22/92*/
        symmetry = fnxfn[trigndx[0]][trigndx[1]];  /* JCO 5/22/92 */
                    /* defaults to XAXIS symmetry JCO 5/22/92 */
   else {  /* added to complete the symmetry JCO 5/22/92 */
      if (trigndx[0]==LOG || trigndx[1] ==LOG) symmetry = NOSYM;
      if (trigndx[0]==9 || trigndx[1] ==9) { /* 'real' cos */
         if (trigndx[0]==SIN || trigndx[1] ==SIN) symmetry = PI_SYM;
         if (trigndx[0]==COS || trigndx[1] ==COS) symmetry = PI_SYM;
      }
      if (trigndx[0]==9 && trigndx[1] ==9) symmetry = PI_SYM;
   }
   return(JuliafpSetup());
}

int
MandelTrigSetup(void)
{
   curfractalspecific->orbitcalc =  LambdaTrigfpFractal;
   symmetry = XYAXIS_NOPARM;
   switch(trigndx[0])
   {
   case SIN:
   case COS:
      curfractalspecific->orbitcalc =  LambdaTrigfpFractal1;
      break;
   case SINH:
   case COSH:
      curfractalspecific->orbitcalc =  LambdaTrigfpFractal2;
      break;
   case EXP:
      symmetry = XAXIS_NOPARM;
      curfractalspecific->orbitcalc =  LambdaexponentFractal;
      break;
   case LOG:
      symmetry = XAXIS_NOPARM;
      break;
   default:   /* added for additional functions, JCO 5/25/92 */
      symmetry = XYAXIS_NOPARM;
      break;
   }
   return(MandelfpSetup());
}

int
MarksJuliafpSetup(void)
{
   if(param[2] < 1)
      param[2] = 1;
   c_exp = (int)param[2];
   floatparm = &parm;
   old = *floatparm;
   if(c_exp > 3)
      cpower(&old,c_exp-1,&coefficient);
   else if(c_exp == 3)
   {
      coefficient.x = sqr(old.x) - sqr(old.y);
      coefficient.y = old.x * old.y * 2;
   }
   else if(c_exp == 2)
      coefficient = old;
   else if(c_exp < 2) {
      coefficient.x = 1.0;
      coefficient.y = 0.0;
   }
   get_julia_attractor (0.0, 0.0);      /* an attractor? */
   return(1);
}

int
SierpinskiFPSetup(void)
{
   /* sierpinski */
   periodicitycheck = 0;                /* disable periodicity checks */
   tmp.x = 1;
   tmp.y = 0.5;
   return(1);
}

int
HalleySetup(void)
{
   /* Halley */
   periodicitycheck=0;

   fractype = HALLEY; /* float on */

   curfractalspecific = &fractalspecific[fractype];

   degree = (int)parm.x;
   if(degree < 2)
      degree = 2;
   param[0] = (double)degree;

/*  precalculated values */
   AplusOne = degree + 1; /* a+1 */
   Ap1deg = AplusOne * degree;

   if(degree % 2)
     symmetry = XAXIS;   /* odd */
   else
     symmetry = XYAXIS; /* even */
   return(1);
}

int
PhoenixSetup(void)
{
   floatparm = &parm;
   degree = (int)parm2.x;
   if(degree < 2 && degree > -3) degree = 0;
   param[2] = (double)degree;
   if(degree == 0)
      curfractalspecific->orbitcalc =  PhoenixFractal;

   if(degree >= 2){
     degree = degree - 1;
     curfractalspecific->orbitcalc =  PhoenixPlusFractal;
   }
   
   if(degree <= -3){
     degree = abs(degree) - 2;
     curfractalspecific->orbitcalc =  PhoenixMinusFractal;
   }

   return(1);
}

int
PhoenixCplxSetup(void)
{
   floatparm = &parm;
   degree = (int)param[4];
   if(degree < 2 && degree > -3) degree = 0;
   param[4] = (double)degree;
   if(degree == 0){
     if(parm2.x != 0 || parm2.y != 0)
       symmetry = NOSYM;
     else
       symmetry = ORIGIN;
     if(parm.y == 0 && parm2.y == 0)
       symmetry = XAXIS;
     curfractalspecific->orbitcalc =  PhoenixFractalcplx;
   }
   if(degree >= 2){
     degree = degree - 1;
     if(parm.y == 0 && parm2.y == 0)
       symmetry = XAXIS;
     else
       symmetry = NOSYM;
     curfractalspecific->orbitcalc =  PhoenixCplxPlusFractal;
   }
   if(degree <= -3){
     degree = abs(degree) - 2;
     if(parm.y == 0 && parm2.y == 0)
       symmetry = XAXIS;
     else
       symmetry = NOSYM;
     curfractalspecific->orbitcalc =  PhoenixCplxMinusFractal;
   }

   return(1);
}

int
MandPhoenixSetup(void)
{
   floatparm = &init;
   degree = (int)parm2.x;
   if(degree < 2 && degree > -3) degree = 0;
   param[2] = (double)degree;
   if(degree == 0){
     curfractalspecific->orbitcalc =  PhoenixFractal;
   }
   if(degree >= 2){
     degree = degree - 1;
     curfractalspecific->orbitcalc =  PhoenixPlusFractal;
   }
   if(degree <= -3){
     degree = abs(degree) - 2;
     curfractalspecific->orbitcalc =  PhoenixMinusFractal;
   }

   return(1);
}

int
MandPhoenixCplxSetup(void)
{
   floatparm = &init;
   degree = (int)param[4];
   if(degree < 2 && degree > -3) degree = 0;
   param[4] = (double)degree;
   if(parm.y != 0 || parm2.y != 0)
     symmetry = NOSYM;
   if(degree == 0){
     curfractalspecific->orbitcalc =  PhoenixFractalcplx;
   }
   if(degree >= 2){
     degree = degree - 1;
     curfractalspecific->orbitcalc =  PhoenixCplxPlusFractal;
   }
   if(degree <= -3){
     degree = abs(degree) - 2;
     curfractalspecific->orbitcalc =  PhoenixCplxMinusFractal;
   }

   return(1);
}

int
StandardSetup(void)
{
   if(fractype==UNITYFP)
      periodicitycheck=0;
   return(1);
}

int
VLSetup(void)
{
   if (param[0] < 0.0) param[0] = 0.0;
   if (param[1] < 0.0) param[1] = 0.0;
   if (param[0] > 1.0) param[0] = 1.0;
   if (param[1] > 1.0) param[1] = 1.0;
   floatparm = &parm;
   return 1;
}
