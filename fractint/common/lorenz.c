/*
   This file contains two 3 dimensional orbit-type fractal
   generators - IFS and LORENZ3D, along with code to generate
   red/blue 3D images. Tim Wegner
*/

#include <string.h>
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "drivers.h"

/* orbitcalc is declared with no arguments so jump through hoops here */
#define LORBIT(x,y,z) \
   (*(int(*)(long *,long *,long *))curfractalspecific->orbitcalc)(x,y,z)
#define FORBIT(x,y,z) \
   (*(int(*)(double*,double*,double*))curfractalspecific->orbitcalc)(x,y,z)

#define RANDOM(x)  (rand()%(x))
/* BAD_PIXEL is used to cutoff orbits that are diverging. It might be better
to test the actual floating point orbit values, but this seems safe for now.
A higher value cannot be used - to test, turn off math coprocessor and
use +2.24 for type ICONS. If BAD_PIXEL is set to 20000, this will abort
Fractint with a math error. Note that this approach precludes zooming in very
far to an orbit type. */

#define BAD_PIXEL  10000L    /* pixels can't get this big */

struct l_affine
{
   /* weird order so a,b,e and c,d,f are vectors */
   long a;
   long b;
   long e;
   long c;
   long d;
   long f;
};
struct long3dvtinf /* data used by 3d view transform subroutine */
{
   long orbit[3];       /* interated function orbit value */
   long iview[3];       /* perspective viewer's coordinates */
   long viewvect[3];    /* orbit transformed for viewing */
   long viewvect1[3];   /* orbit transformed for viewing */
   long maxvals[3];
   long minvals[3];
   MATRIX doublemat;    /* transformation matrix */
   MATRIX doublemat1;   /* transformation matrix */
   long longmat[4][4];  /* long version of matrix */
   long longmat1[4][4]; /* long version of matrix */
   int row,col;         /* results */
   int row1,col1;
   struct l_affine cvt;
};

struct float3dvtinf /* data used by 3d view transform subroutine */
{
   double orbit[3];                /* interated function orbit value */
   double viewvect[3];        /* orbit transformed for viewing */
   double viewvect1[3];        /* orbit transformed for viewing */
   double maxvals[3];
   double minvals[3];
   MATRIX doublemat;    /* transformation matrix */
   MATRIX doublemat1;   /* transformation matrix */
   int row,col;         /* results */
   int row1,col1;
   struct affine cvt;
};

/* Routines in this module      */

static int  ifs2d(void);
static int  ifs3d(void);
static int  ifs3dlong(void);
static int  ifs3dfloat(void);
static int  l_setup_convert_to_screen(struct l_affine *);
static void setupmatrix(MATRIX);
static int  long3dviewtransf(struct long3dvtinf *inf);
static int  float3dviewtransf(struct float3dvtinf *inf);
static FILE *open_orbitsave(void);
static void _fastcall plothist(int x, int y, int color);
static int realtime;
static char orbitsavename[]    = {"orbits.raw"};
static char orbitsave_format[] = {"%g %g %g 15\n"};

S32 maxct;
static int t;
static long l_dx,l_dy,l_dz,l_dt,l_a,l_b,l_c,l_d;
static long l_adt,l_bdt,l_cdt,l_xdt,l_ydt;
static long initorbitlong[3];

static double dx,dy,dz,dt,a,b,c,d;
static double adt,bdt,cdt,xdt,ydt,zdt;
static double initorbitfp[3];

/*
 * The following declarations used for Inverse Julia.  MVS
 */

static char NoQueue[] =
  "Not enough memory: switching to random walk.\n";

static int    mxhits;
int    run_length;
/*
enum   {breadth_first, depth_first, random_walk, random_run} major_method;
enum   {left_first, right_first}                             minor_method;
*/
enum   Major major_method;
enum   Minor minor_method;
struct affine cvt;
struct l_affine lcvt;

double Cx, Cy;
long   CxLong, CyLong;

/*
 * end of Inverse Julia declarations;
 */

/* these are potential user parameters */
int connect = 1;    /* flag to connect points with a line */
int euler = 0;      /* use implicit euler approximation for dynamic system */
int waste = 100;    /* waste this many points before plotting */
int projection = 2; /* projection plane - default is to plot x-y */

/******************************************************************/
/*                 zoom box conversion functions                  */
/******************************************************************/

/*
   Conversion of complex plane to screen coordinates for rotating zoom box.
   Assume there is an affine transformation mapping complex zoom parallelogram
   to rectangular screen. We know this map must map parallelogram corners to
   screen corners, so we have following equations:

      a*xxmin+b*yymax+e == 0        (upper left)
      c*xxmin+d*yymax+f == 0

      a*xx3rd+b*yy3rd+e == 0        (lower left)
      c*xx3rd+d*yy3rd+f == ydots-1

      a*xxmax+b*yymin+e == xdots-1  (lower right)
      c*xxmax+d*yymin+f == ydots-1

      First we must solve for a,b,c,d,e,f - (which we do once per image),
      then we just apply the transformation to each orbit value.
*/

/*
   Thanks to Sylvie Gallet for the following. The original code for
   setup_convert_to_screen() solved for coefficients of the
   complex-plane-to-screen transformation using a very straight-forward
   application of determinants to solve a set of simulataneous
   equations. The procedure was simple and general, but inefficient.
   The inefficiecy wasn't hurting anything because the routine was called
   only once per image, but it seemed positively sinful to use it
   because the code that follows is SO much more compact, at the
   expense of being less general. Here are Sylvie's notes. I have further
   optimized the code a slight bit.
                                               Tim Wegner
                                               July, 1996
  Sylvie's notes, slightly edited follow:

  You don't need 3x3 determinants to solve these sets of equations because
  the unknowns e and f have the same coefficient: 1.

  First set of 3 equations:
     a*xxmin+b*yymax+e == 0
     a*xx3rd+b*yy3rd+e == 0
     a*xxmax+b*yymin+e == xdots-1
  To make things easy to read, I just replace xxmin, xxmax, xx3rd by x1,
  x2, x3 (ditto for yy...) and xdots-1 by xd.

     a*x1 + b*y2 + e == 0    (1)
     a*x3 + b*y3 + e == 0    (2)
     a*x2 + b*y1 + e == xd   (3)

  I subtract (1) to (2) and (3):
     a*x1      + b*y2      + e == 0   (1)
     a*(x3-x1) + b*(y3-y2)     == 0   (2)-(1)
     a*(x2-x1) + b*(y1-y2)     == xd  (3)-(1)

  I just have to calculate a 2x2 determinant:
     det == (x3-x1)*(y1-y2) - (y3-y2)*(x2-x1)

  And the solution is:
     a = -xd*(y3-y2)/det
     b =  xd*(x3-x1)/det
     e = - a*x1 - b*y2

The same technique can be applied to the second set of equations:

   c*xxmin+d*yymax+f == 0
   c*xx3rd+d*yy3rd+f == ydots-1
   c*xxmax+d*yymin+f == ydots-1

   c*x1 + d*y2 + f == 0    (1)
   c*x3 + d*y3 + f == yd   (2)
   c*x2 + d*y1 + f == yd   (3)

   c*x1      + d*y2      + f == 0    (1)
   c*(x3-x2) + d*(y3-y1)     == 0    (2)-(3)
   c*(x2-x1) + d*(y1-y2)     == yd   (3)-(1)

   det == (x3-x2)*(y1-y2) - (y3-y1)*(x2-x1)

   c = -yd*(y3-y1)/det
   d =  yd*(x3-x2))det
   f = - c*x1 - d*y2

        -  Sylvie
*/

int setup_convert_to_screen(struct affine *scrn_cnvt)
{
   double det, xd, yd;

   if((det = (xx3rd-xxmin)*(yymin-yymax) + (yymax-yy3rd)*(xxmax-xxmin))==0)
      return(-1);
   xd = dxsize/det;
   scrn_cnvt->a =  xd*(yymax-yy3rd);
   scrn_cnvt->b =  xd*(xx3rd-xxmin);
   scrn_cnvt->e = -scrn_cnvt->a*xxmin - scrn_cnvt->b*yymax;

   if((det = (xx3rd-xxmax)*(yymin-yymax) + (yymin-yy3rd)*(xxmax-xxmin))==0)
      return(-1);
   yd = dysize/det;
   scrn_cnvt->c =  yd*(yymin-yy3rd);
   scrn_cnvt->d =  yd*(xx3rd-xxmax);
   scrn_cnvt->f = -scrn_cnvt->c*xxmin - scrn_cnvt->d*yymax;
   return(0);
}

static int l_setup_convert_to_screen(struct l_affine *l_cvt)
{
   struct affine cvt;

   /* MCP 7-7-91, This function should return a something! */
   if(setup_convert_to_screen(&cvt))
      return(-1);
   l_cvt->a = (long)(cvt.a*fudge); l_cvt->b = (long)(cvt.b*fudge); l_cvt->c = (long)(cvt.c*fudge);
   l_cvt->d = (long)(cvt.d*fudge); l_cvt->e = (long)(cvt.e*fudge); l_cvt->f = (long)(cvt.f*fudge);

   /* MCP 7-7-91 */
   return(0);
}

/******************************************************************/
/*   setup functions - put in fractalspecific[fractype].per_image */
/******************************************************************/

static double orbit;
static long   l_orbit;
static long l_sinx,l_cosx;

int orbit3dlongsetup()
{
   maxct = 0L;
   connect = 1;
   waste = 100;
   projection = 2;
   if (fractype==LHENON || fractype==KAM || fractype==KAM3D ||
       fractype==INVERSEJULIA)
      connect=0;
   if(fractype==LROSSLER)
      waste = 500;
   if(fractype==LLORENZ)
      projection = 1;

   initorbitlong[0] = fudge;  /* initial conditions */
   initorbitlong[1] = fudge;
   initorbitlong[2] = fudge;

   if(fractype==LHENON)
   {
      l_a =  (long)(param[0]*fudge);
      l_b =  (long)(param[1]*fudge);
      l_c =  (long)(param[2]*fudge);
      l_d =  (long)(param[3]*fudge);
   }
   else if(fractype==KAM || fractype==KAM3D)
   {
      maxct = 1L;
      a   = param[0];           /* angle */
      if(param[1] <= 0.0)
         param[1] = .01;
      l_b =  (long)(param[1]*fudge);    /* stepsize */
      l_c =  (long)(param[2]*fudge);    /* stop */
      t = (int)(l_d =  (long)(param[3]));     /* points per orbit */

      l_sinx = (long)(sin(a)*fudge);
      l_cosx = (long)(cos(a)*fudge);
      l_orbit = 0;
      initorbitlong[0] = initorbitlong[1] = initorbitlong[2] = 0;
   }
   else if (fractype == INVERSEJULIA)
   {
      LCMPLX Sqrt;

      CxLong = (long)(param[0] * fudge);
      CyLong = (long)(param[1] * fudge);

      mxhits    = (int) param[2];
      run_length = (int) param[3];
      if (mxhits <= 0)
          mxhits = 1;
      else if (mxhits >= colors)
          mxhits = colors - 1;
      param[2] = mxhits;

      setup_convert_to_screen(&cvt);
      /* Note: using bitshift of 21 for affine, 24 otherwise */

      lcvt.a = (long)(cvt.a * (1L << 21));
      lcvt.b = (long)(cvt.b * (1L << 21));
      lcvt.c = (long)(cvt.c * (1L << 21));
      lcvt.d = (long)(cvt.d * (1L << 21));
      lcvt.e = (long)(cvt.e * (1L << 21));
      lcvt.f = (long)(cvt.f * (1L << 21));

      Sqrt = ComplexSqrtLong(fudge - 4 * CxLong, -4 * CyLong);

      switch (major_method) {
         case breadth_first:
            if (Init_Queue((long)32*1024) == 0)
            { /* can't get queue memory: fall back to random walk */
               stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, NoQueue);
               major_method = random_walk;
               goto lrwalk;
            }
            EnQueueLong((fudge + Sqrt.x) / 2,  Sqrt.y / 2);
            EnQueueLong((fudge - Sqrt.x) / 2, -Sqrt.y / 2);
            break;
         case depth_first:
            if (Init_Queue((long)32*1024) == 0)
            { /* can't get queue memory: fall back to random walk */
               stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, NoQueue);
               major_method = random_walk;
               goto lrwalk;
            }
            switch (minor_method) {
               case left_first:
                  PushLong((fudge + Sqrt.x) / 2,  Sqrt.y / 2);
                  PushLong((fudge - Sqrt.x) / 2, -Sqrt.y / 2);
                  break;
               case right_first:
                  PushLong((fudge - Sqrt.x) / 2, -Sqrt.y / 2);
                  PushLong((fudge + Sqrt.x) / 2,  Sqrt.y / 2);
                  break;
            }
            break;
         case random_walk:
lrwalk:
            lnew.x = initorbitlong[0] = fudge + Sqrt.x / 2;
            lnew.y = initorbitlong[1] =         Sqrt.y / 2;
            break;
         case random_run:
            lnew.x = initorbitlong[0] = fudge + Sqrt.x / 2;
            lnew.y = initorbitlong[1] =         Sqrt.y / 2;
            break;
      }
   }
   else
   {
      l_dt = (long)(param[0]*fudge);
      l_a =  (long)(param[1]*fudge);
      l_b =  (long)(param[2]*fudge);
      l_c =  (long)(param[3]*fudge);
   }

   /* precalculations for speed */
   l_adt = multiply(l_a,l_dt,bitshift);
   l_bdt = multiply(l_b,l_dt,bitshift);
   l_cdt = multiply(l_c,l_dt,bitshift);
   return(1);
}

#define COSB   dx
#define SINABC dy

int orbit3dfloatsetup()
{
   maxct = 0L;
   connect = 1;
   waste = 100;
   projection = 2;

   if(fractype==FPHENON || fractype==FPPICKOVER || fractype==FPGINGERBREAD
            || fractype == KAMFP || fractype == KAM3DFP
            || fractype == FPHOPALONG || fractype == INVERSEJULIAFP)
      connect=0;
   if(fractype==FPLORENZ3D1 || fractype==FPLORENZ3D3 ||
      fractype==FPLORENZ3D4)
      waste = 750;
   if(fractype==FPROSSLER)
      waste = 500;
   if(fractype==FPLORENZ)
      projection = 1; /* plot x and z */

   initorbitfp[0] = 1;  /* initial conditions */
   initorbitfp[1] = 1;
   initorbitfp[2] = 1;
   if(fractype==FPGINGERBREAD)
   {
      initorbitfp[0] = param[0];        /* initial conditions */
      initorbitfp[1] = param[1];
   }

   if(fractype==ICON || fractype==ICON3D)        /* DMF */
   {
      initorbitfp[0] = 0.01;  /* initial conditions */
      initorbitfp[1] = 0.003;
      connect = 0;
      waste = 2000;
   }

   if(fractype==LATOO)        /* HB */
   {
      connect = 0;
   }

   if(fractype==FPHENON || fractype==FPPICKOVER)
   {
      a =  param[0];
      b =  param[1];
      c =  param[2];
      d =  param[3];
   }
   else if(fractype==ICON || fractype==ICON3D)        /* DMF */
   {
      initorbitfp[0] = 0.01;  /* initial conditions */
      initorbitfp[1] = 0.003;
      connect = 0;
      waste = 2000;
      /* Initialize parameters */
      a  =   param[0];
      b  =   param[1];
      c  =   param[2];
      d  =   param[3];
   }
   else if(fractype==KAMFP || fractype==KAM3DFP)
   {
      maxct = 1L;
      a = param[0];           /* angle */
      if(param[1] <= 0.0)
         param[1] = .01;
      b =  param[1];    /* stepsize */
      c =  param[2];    /* stop */
      t = (int)(l_d =  (long)(param[3]));     /* points per orbit */
      sinx = sin(a);
      cosx = cos(a);
      orbit = 0;
      initorbitfp[0] = initorbitfp[1] = initorbitfp[2] = 0;
   }
   else if(fractype==FPHOPALONG || fractype==FPMARTIN || fractype==CHIP
        || fractype==QUADRUPTWO || fractype==THREEPLY)
   {
      initorbitfp[0] = 0;  /* initial conditions */
      initorbitfp[1] = 0;
      initorbitfp[2] = 0;
      connect = 0;
      a =  param[0];
      b =  param[1];
      c =  param[2];
      d =  param[3];
      if(fractype==THREEPLY)
      {
         COSB   = cos(b);
         SINABC = sin(a+b+c);
      }
   }
   else if (fractype == INVERSEJULIAFP)
   {
      _CMPLX Sqrt;

      Cx = param[0];
      Cy = param[1];

      mxhits    = (int) param[2];
      run_length = (int) param[3];
      if (mxhits <= 0)
          mxhits = 1;
      else if (mxhits >= colors)
          mxhits = colors - 1;
      param[2] = mxhits;

      setup_convert_to_screen(&cvt);

      /* find fixed points: guaranteed to be in the set */
      Sqrt = ComplexSqrtFloat(1 - 4 * Cx, -4 * Cy);
      switch ((int) major_method) {
         case breadth_first:
            if (Init_Queue((long)32*1024) == 0)
            { /* can't get queue memory: fall back to random walk */
               stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, NoQueue);
               major_method = random_walk;
               goto rwalk;
            }
            EnQueueFloat((float)((1 + Sqrt.x) / 2), (float)(Sqrt.y / 2));
            EnQueueFloat((float)((1 - Sqrt.x) / 2), (float)(-Sqrt.y / 2));
            break;
         case depth_first:                      /* depth first (choose direction) */
            if (Init_Queue((long)32*1024) == 0)
            { /* can't get queue memory: fall back to random walk */
               stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, NoQueue);
               major_method = random_walk;
               goto rwalk;
            }
            switch (minor_method) {
               case left_first:
                  PushFloat((float)((1 + Sqrt.x) / 2), (float)(Sqrt.y / 2));
                  PushFloat((float)((1 - Sqrt.x) / 2), (float)(-Sqrt.y / 2));
                  break;
               case right_first:
                  PushFloat((float)((1 - Sqrt.x) / 2), (float)(-Sqrt.y / 2));
                  PushFloat((float)((1 + Sqrt.x) / 2), (float)(Sqrt.y / 2));
                  break;
            }
            break;
         case random_walk:
rwalk:
            g_new.x = initorbitfp[0] = 1 + Sqrt.x / 2;
            g_new.y = initorbitfp[1] = Sqrt.y / 2;
            break;
         case random_run:       /* random run, choose intervals */
            major_method = random_run;
            g_new.x = initorbitfp[0] = 1 + Sqrt.x / 2;
            g_new.y = initorbitfp[1] = Sqrt.y / 2;
            break;
      }
   }
   else
   {
      dt = param[0];
      a =  param[1];
      b =  param[2];
      c =  param[3];

   }

   /* precalculations for speed */
   adt = a*dt;
   bdt = b*dt;
   cdt = c*dt;

   return(1);
}

/******************************************************************/
/*   orbit functions - put in fractalspecific[fractype].orbitcalc */
/******************************************************************/

/* Julia sets by inverse iterations added by Juan J. Buhler 4/3/92 */
/* Integrated with Lorenz by Tim Wegner 7/20/92 */
/* Add Modified Inverse Iteration Method, 11/92 by Michael Snyder  */

int
Minverse_julia_orbit()
{
   static int   random_dir = 0, random_len = 0;
   int    newrow, newcol;
   int    color,  leftright;

   /*
    * First, compute new point
    */
   switch (major_method) {
      case breadth_first:
         if (QueueEmpty())
            return -1;
         g_new = DeQueueFloat();
         break;
      case depth_first:
         if (QueueEmpty())
            return -1;
         g_new = PopFloat();
         break;
      case random_walk:
#if 0
         g_new = ComplexSqrtFloat(g_new.x - Cx, g_new.y - Cy);
         if (RANDOM(2))
         {
            g_new.x = -g_new.x;
            g_new.y = -g_new.y;
         }
#endif
         break;
      case random_run:
#if 0
         g_new = ComplexSqrtFloat(g_new.x - Cx, g_new.y - Cy);
         if (random_len == 0)
         {
            random_len = RANDOM(run_length);
            random_dir = RANDOM(3);
         }
         switch (random_dir) {
            case 0:     /* left */
               break;
            case 1:     /* right */
               g_new.x = -g_new.x;
               g_new.y = -g_new.y;
               break;
            case 2:     /* random direction */
               if (RANDOM(2))
               {
                  g_new.x = -g_new.x;
                  g_new.y = -g_new.y;
               }
               break;
         }
#endif
         break;
   }

   /*
    * Next, find its pixel position
    */
   newcol = (int)(cvt.a * g_new.x + cvt.b * g_new.y + cvt.e);
   newrow = (int)(cvt.c * g_new.x + cvt.d * g_new.y + cvt.f);

   /*
    * Now find the next point(s), and flip a coin to choose one.
    */

   g_new       = ComplexSqrtFloat(g_new.x - Cx, g_new.y - Cy);
   leftright = (RANDOM(2)) ? 1 : -1;

   if (newcol < 1 || newcol >= xdots || newrow < 1 || newrow >= ydots)
   {
      /*
       * MIIM must skip points that are off the screen boundary,
       * since it cannot read their color.
       */
      switch (major_method) {
         case breadth_first:
            EnQueueFloat((float)(leftright * g_new.x), (float)(leftright * g_new.y));
            return 1;
         case depth_first:
            PushFloat   ((float)(leftright * g_new.x), (float)(leftright * g_new.y));
            return 1;
         case random_run:
         case random_walk:
            break;
      }
   }

   /*
    * Read the pixel's color:
    * For MIIM, if color >= mxhits, discard the point
    *           else put the point's children onto the queue
    */
   color  = getcolor(newcol, newrow);
   switch (major_method) {
      case breadth_first:
         if (color < mxhits)
         {
            putcolor(newcol, newrow, color+1);
/*          g_new = ComplexSqrtFloat(g_new.x - Cx, g_new.y - Cy); */
            EnQueueFloat( (float)g_new.x, (float)g_new.y);
            EnQueueFloat((float)-g_new.x, (float)-g_new.y);
         }
         break;
      case depth_first:
         if (color < mxhits)
         {
            putcolor(newcol, newrow, color+1);
/*          g_new = ComplexSqrtFloat(g_new.x - Cx, g_new.y - Cy); */
            if (minor_method == left_first)
            {
               if (QueueFullAlmost())
                  PushFloat((float)-g_new.x, (float)-g_new.y);
               else
               {
                  PushFloat( (float)g_new.x, (float)g_new.y);
                  PushFloat((float)-g_new.x, (float)-g_new.y);
               }
            }
            else
            {
               if (QueueFullAlmost())
                  PushFloat( (float)g_new.x, (float)g_new.y);
               else
               {
                  PushFloat((float)-g_new.x, (float)-g_new.y);
                  PushFloat( (float)g_new.x, (float)g_new.y);
               }
            }
         }
         break;
      case random_run:
         if (random_len-- == 0)
         {
            random_len = RANDOM(run_length);
            random_dir = RANDOM(3);
         }
         switch (random_dir) {
            case 0:     /* left */
               break;
            case 1:     /* right */
               g_new.x = -g_new.x;
               g_new.y = -g_new.y;
               break;
            case 2:     /* random direction */
               g_new.x = leftright * g_new.x;
               g_new.y = leftright * g_new.y;
               break;
         }
         if (color < colors-1)
            putcolor(newcol, newrow, color+1);
         break;
      case random_walk:
         if (color < colors-1)
            putcolor(newcol, newrow, color+1);
         g_new.x = leftright * g_new.x;
         g_new.y = leftright * g_new.y;
         break;
   }
   return 1;

}

int
Linverse_julia_orbit()
{
   static int   random_dir = 0, random_len = 0;
   int    newrow, newcol;
   int    color;

   /*
    * First, compute new point
    */
   switch (major_method) {
      case breadth_first:
         if (QueueEmpty())
            return -1;
         lnew = DeQueueLong();
         break;
      case depth_first:
         if (QueueEmpty())
            return -1;
         lnew = PopLong();
         break;
      case random_walk:
         lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
         if (RANDOM(2))
         {
            lnew.x = -lnew.x;
            lnew.y = -lnew.y;
         }
         break;
      case random_run:
         lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
         if (random_len == 0)
         {
            random_len = RANDOM(run_length);
            random_dir = RANDOM(3);
         }
         switch (random_dir) {
            case 0:     /* left */
               break;
            case 1:     /* right */
               lnew.x = -lnew.x;
               lnew.y = -lnew.y;
               break;
            case 2:     /* random direction */
               if (RANDOM(2))
               {
                  lnew.x = -lnew.x;
                  lnew.y = -lnew.y;
               }
               break;
         }
   }

   /*
    * Next, find its pixel position
    *
    * Note: had to use a bitshift of 21 for this operation because
    * otherwise the values of lcvt were truncated.  Used bitshift
    * of 24 otherwise, for increased precision.
    */
   newcol = (int)((multiply(lcvt.a, lnew.x >> (bitshift - 21), 21) +
             multiply(lcvt.b, lnew.y >> (bitshift - 21), 21) + lcvt.e) >> 21);
   newrow = (int)((multiply(lcvt.c, lnew.x >> (bitshift - 21), 21) +
             multiply(lcvt.d, lnew.y >> (bitshift - 21), 21) + lcvt.f) >> 21);

   if (newcol < 1 || newcol >= xdots || newrow < 1 || newrow >= ydots)
   {
      /*
       * MIIM must skip points that are off the screen boundary,
       * since it cannot read their color.
       */
      if (RANDOM(2))
         color =  1;
      else
         color = -1;
      switch (major_method) {
         case breadth_first:
            lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
            EnQueueLong(color * lnew.x, color * lnew.y);
            break;
         case depth_first:
            lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
            PushLong(color * lnew.x, color * lnew.y);
            break;
         case random_run:
            random_len--;
         case random_walk:
            break;
      }
      return 1;
   }

   /*
    * Read the pixel's color:
    * For MIIM, if color >= mxhits, discard the point
    *           else put the point's children onto the queue
    */
   color  = getcolor(newcol, newrow);
   switch (major_method) {
      case breadth_first:
         if (color < mxhits)
         {
            putcolor(newcol, newrow, color+1);
            lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
            EnQueueLong( lnew.x,  lnew.y);
            EnQueueLong(-lnew.x, -lnew.y);
         }
         break;
      case depth_first:
         if (color < mxhits)
         {
            putcolor(newcol, newrow, color+1);
            lnew = ComplexSqrtLong(lnew.x - CxLong, lnew.y - CyLong);
            if (minor_method == left_first)
            {
               if (QueueFullAlmost())
                  PushLong(-lnew.x, -lnew.y);
               else
               {
                  PushLong( lnew.x,  lnew.y);
                  PushLong(-lnew.x, -lnew.y);
               }
            }
            else
            {
               if (QueueFullAlmost())
                  PushLong( lnew.x,  lnew.y);
               else
               {
                  PushLong(-lnew.x, -lnew.y);
                  PushLong( lnew.x,  lnew.y);
               }
            }
         }
         break;
      case random_run:
         random_len--;
         /* fall through */
      case random_walk:
         if (color < colors-1)
            putcolor(newcol, newrow, color+1);
         break;
   }
   return 1;
}

#if 0
int inverse_julia_orbit(double *x, double *y, double *z)
{
   double r, xo, yo;
   int waste;
   if(*z >= 1.0) /* this assumes intial value is 1.0!!! */
   {
      waste = 20; /* skip these points at first */
      *z = 0;
   }
   else
      waste = 1;
   while(waste--)
   {
      xo = *x - param[0];
      yo = *y - param[1];
      r  = sqrt(xo*xo + yo*yo);
      *x  = sqrt((r + xo)/2);
      if (yo < 0)
         *x = - *x;
      *y = sqrt((r - xo)/2);
      if(RANDOM(10) > 4)
      {
                  *x = -(*x);
                  *y = -(*y);
      }
   }
   return(0);
}
#endif

int lorenz3dlongorbit(long *l_x, long *l_y, long *l_z)
{
      l_xdt = multiply(*l_x,l_dt,bitshift);
      l_ydt = multiply(*l_y,l_dt,bitshift);
      l_dx  = -multiply(l_adt,*l_x,bitshift) + multiply(l_adt,*l_y,bitshift);
      l_dy  =  multiply(l_bdt,*l_x,bitshift) -l_ydt -multiply(*l_z,l_xdt,bitshift);
      l_dz  = -multiply(l_cdt,*l_z,bitshift) + multiply(*l_x,l_ydt,bitshift);

      *l_x += l_dx;
      *l_y += l_dy;
      *l_z += l_dz;
      return(0);
}

int lorenz3d1floatorbit(double *x, double *y, double *z)
{
      double norm;

      xdt = (*x)*dt;
      ydt = (*y)*dt;
      zdt = (*z)*dt;

      /* 1-lobe Lorenz */
      norm = sqrt((*x)*(*x)+(*y)*(*y));
      dx   = (-adt-dt)*(*x) + (adt-bdt)*(*y) + (dt-adt)*norm + ydt*(*z);
      dy   = (bdt-adt)*(*x) - (adt+dt)*(*y) + (bdt+adt)*norm - xdt*(*z) -
             norm*zdt;
      dz   = (ydt/2) - cdt*(*z);

      *x += dx;
      *y += dy;
      *z += dz;
      return(0);
}

int lorenz3dfloatorbit(double *x, double *y, double *z)
{
      xdt = (*x)*dt;
      ydt = (*y)*dt;
      zdt = (*z)*dt;

      /* 2-lobe Lorenz (the original) */
      dx  = -adt*(*x) + adt*(*y);
      dy  =  bdt*(*x) - ydt - (*z)*xdt;
      dz  = -cdt*(*z) + (*x)*ydt;

      *x += dx;
      *y += dy;
      *z += dz;
      return(0);
}

int lorenz3d3floatorbit(double *x, double *y, double *z)
{
      double norm;

      xdt = (*x)*dt;
      ydt = (*y)*dt;
      zdt = (*z)*dt;

      /* 3-lobe Lorenz */
      norm = sqrt((*x)*(*x)+(*y)*(*y));
      dx   = (-(adt+dt)*(*x) + (adt-bdt+zdt)*(*y)) / 3 +
             ((dt-adt)*((*x)*(*x)-(*y)*(*y)) +
             2*(bdt+adt-zdt)*(*x)*(*y))/(3*norm);
      dy   = ((bdt-adt-zdt)*(*x) - (adt+dt)*(*y)) / 3 +
             (2*(adt-dt)*(*x)*(*y) +
             (bdt+adt-zdt)*((*x)*(*x)-(*y)*(*y)))/(3*norm);
      dz   = (3*xdt*(*x)*(*y)-ydt*(*y)*(*y))/2 - cdt*(*z);

      *x += dx;
      *y += dy;
      *z += dz;
      return(0);
}

int lorenz3d4floatorbit(double *x, double *y, double *z)
{
      xdt = (*x)*dt;
      ydt = (*y)*dt;
      zdt = (*z)*dt;

      /* 4-lobe Lorenz */
      dx   = (-adt*(*x)*(*x)*(*x) + (2*adt+bdt-zdt)*(*x)*(*x)*(*y) +
             (adt-2*dt)*(*x)*(*y)*(*y) + (zdt-bdt)*(*y)*(*y)*(*y)) /
             (2 * ((*x)*(*x)+(*y)*(*y)));
      dy   = ((bdt-zdt)*(*x)*(*x)*(*x) + (adt-2*dt)*(*x)*(*x)*(*y) +
             (-2*adt-bdt+zdt)*(*x)*(*y)*(*y) - adt*(*y)*(*y)*(*y)) /
             (2 * ((*x)*(*x)+(*y)*(*y)));
      dz   = (2*xdt*(*x)*(*x)*(*y) - 2*xdt*(*y)*(*y)*(*y) - cdt*(*z));

      *x += dx;
      *y += dy;
      *z += dz;
      return(0);
}

int henonfloatorbit(double *x, double *y, double *z)
{
      double newx,newy;
      *z = *x; /* for warning only */
      newx  = 1 + *y - a*(*x)*(*x);
      newy  = b*(*x);
      *x = newx;
      *y = newy;
      return(0);
}

int henonlongorbit(long *l_x, long *l_y, long *l_z)
{
      long newx,newy;
      *l_z = *l_x; /* for warning only */
      newx = multiply(*l_x,*l_x,bitshift);
      newx = multiply(newx,l_a,bitshift);
      newx  = fudge + *l_y - newx;
      newy  = multiply(l_b,*l_x,bitshift);
      *l_x = newx;
      *l_y = newy;
      return(0);
}

int rosslerfloatorbit(double *x, double *y, double *z)
{
      xdt = (*x)*dt;
      ydt = (*y)*dt;

      dx = -ydt - (*z)*dt;
      dy = xdt + (*y)*adt;
      dz = bdt + (*z)*xdt - (*z)*cdt;

      *x += dx;
      *y += dy;
      *z += dz;
      return(0);
}

int pickoverfloatorbit(double *x, double *y, double *z)
{
      double newx,newy,newz;
      newx = sin(a*(*y)) - (*z)*cos(b*(*x));
      newy = (*z)*sin(c*(*x)) - cos(d*(*y));
      newz = sin(*x);
      *x = newx;
      *y = newy;
      *z = newz;
      return(0);
}
/* page 149 "Science of Fractal Images" */
int gingerbreadfloatorbit(double *x, double *y, double *z)
{
      double newx;
      *z = *x; /* for warning only */
      newx = 1 - (*y) + fabs(*x);
      *y = *x;
      *x = newx;
      return(0);
}

int rosslerlongorbit(long *l_x, long *l_y, long *l_z)
{
      l_xdt = multiply(*l_x,l_dt,bitshift);
      l_ydt = multiply(*l_y,l_dt,bitshift);

      l_dx  = -l_ydt - multiply(*l_z,l_dt,bitshift);
      l_dy  =  l_xdt + multiply(*l_y,l_adt,bitshift);
      l_dz  =  l_bdt + multiply(*l_z,l_xdt,bitshift)
                     - multiply(*l_z,l_cdt,bitshift);

      *l_x += l_dx;
      *l_y += l_dy;
      *l_z += l_dz;

      return(0);
}

/* OSTEP  = Orbit Step (and inner orbit value) */
/* NTURNS = Outside Orbit */
/* TURN2  = Points per orbit */
/* a      = Angle */


int kamtorusfloatorbit(double *r, double *s, double *z)
{
   double srr;
   if(t++ >= l_d)
   {
      orbit += b;
      (*r) = (*s) = orbit/3;
      t = 0;
      *z = orbit;
      if(orbit > c)
         return(1);
   }
   srr = (*s)-(*r)*(*r);
   (*s)=(*r)*sinx+srr*cosx;
   (*r)=(*r)*cosx-srr*sinx;
   return(0);
}

int kamtoruslongorbit(long *r, long *s, long *z)
{
   long srr;
   if(t++ >= l_d)
   {
      l_orbit += l_b;
      (*r) = (*s) = l_orbit/3;
      t = 0;
      *z = l_orbit;
      if(l_orbit > l_c)
         return(1);
   }
   srr = (*s)-multiply((*r),(*r),bitshift);
   (*s)=multiply((*r),l_sinx,bitshift)+multiply(srr,l_cosx,bitshift);
   (*r)=multiply((*r),l_cosx,bitshift)-multiply(srr,l_sinx,bitshift);
   return(0);
}

int hopalong2dfloatorbit(double *x, double *y, double *z)
{
   double tmp;
   *z = *x; /* for warning only */
   tmp = *y - sign(*x)*sqrt(fabs(b*(*x)-c));
   *y = a - *x;
   *x = tmp;
   return(0);
}

/* from Michael Peters and HOP */
int chip2dfloatorbit(double *x, double *y, double *z)
{
   double tmp;
   *z = *x; /* for warning only */
   tmp = *y - sign(*x) * cos(sqr(log(fabs(b*(*x)-c))))
                       * atan(sqr(log(fabs(c*(*x)-b))));
   *y = a - *x;
   *x = tmp;
   return(0);
}

/* from Michael Peters and HOP */
int quadruptwo2dfloatorbit(double *x, double *y, double *z)
{
   double tmp;
   *z = *x; /* for warning only */
   tmp = *y - sign(*x) * sin(log(fabs(b*(*x)-c)))
                       * atan(sqr(log(fabs(c*(*x)-b))));
   *y = a - *x;
   *x = tmp;
   return(0);
}

/* from Michael Peters and HOP */
int threeply2dfloatorbit(double *x, double *y, double *z)
{
   double tmp;
   *z = *x; /* for warning only */
   tmp = *y - sign(*x)*(fabs(sin(*x)*COSB+c-(*x)*SINABC));
   *y = a - *x;
   *x = tmp;
   return(0);
}

int martin2dfloatorbit(double *x, double *y, double *z)
{
   double tmp;
   *z = *x;  /* for warning only */
   tmp = *y - sin(*x);
   *y = a - *x;
   *x = tmp;
   return(0);
}

int mandelcloudfloat(double *x, double *y, double *z)
{
    double newx,newy,x2,y2;
#ifndef XFRACT
    newx = *z; /* for warning only */
#endif
    x2 = (*x)*(*x);
    y2 = (*y)*(*y);
    if (x2+y2>2) return 1;
    newx = x2-y2+a;
    newy = 2*(*x)*(*y)+b;
    *x = newx;
    *y = newy;
    return(0);
}

int dynamfloat(double *x, double *y, double *z)
{
      _CMPLX cp,tmp;
      double newx,newy;
#ifndef XFRACT
      newx = *z; /* for warning only */
#endif
      cp.x = b* *x;
      cp.y = 0;
      CMPLXtrig0(cp, tmp);
      newy = *y + dt*sin(*x + a*tmp.x);
      if (euler) {
          *y = newy;
      }

      cp.x = b* *y;
      cp.y = 0;
      CMPLXtrig0(cp, tmp);
      newx = *x - dt*sin(*y + a*tmp.x);
      *x = newx;
      *y = newy;
      return(0);
}

/* dmf */
#undef  LAMBDA
#define LAMBDA  param[0]
#define ALPHA   param[1]
#define BETA    param[2]
#define GAMMA   param[3]
#define OMEGA   param[4]
#define DEGREE  param[5]

int iconfloatorbit(double *x, double *y, double *z)
{

    double oldx, oldy, zzbar, zreal, zimag, za, zb, zn, p;
    int i;

    oldx = *x;
    oldy = *y;

    zzbar = oldx * oldx + oldy * oldy;
    zreal = oldx;
    zimag = oldy;

    for(i=1; i <= DEGREE-2; i++) {
        za = zreal * oldx - zimag * oldy;
        zb = zimag * oldx + zreal * oldy;
        zreal = za;
        zimag = zb;
    }
    zn = oldx * zreal - oldy * zimag;
    p = LAMBDA + ALPHA * zzbar + BETA * zn;
    *x = p * oldx + GAMMA * zreal - OMEGA * oldy;
    *y = p * oldy - GAMMA * zimag + OMEGA * oldx;

    *z = zzbar;
    return(0);
}
#ifdef LAMBDA  /* Tim says this will make me a "good citizen" */
#undef LAMBDA  /* Yeah, but you were bad, Dan - LAMBDA was already */
#undef ALPHA   /* defined! <grin!> TW */
#undef BETA
#undef GAMMA
#endif

/* hb */
#define PAR_A   param[0]
#define PAR_B   param[1]
#define PAR_C   param[2]
#define PAR_D   param[3]

int latoofloatorbit(double *x, double *y, double *z)
{

    double xold, yold, tmp;

    xold = *z; /* for warning only */

    xold = *x;
    yold = *y;

/*    *x = sin(yold * PAR_B) + PAR_C * sin(xold * PAR_B); */
    old.x = yold * PAR_B;
    old.y = 0;          /* old = (y * B) + 0i (in the complex)*/
    CMPLXtrig0(old,g_new);
    tmp = (double) g_new.x;
    old.x = xold * PAR_B;
    old.y = 0;          /* old = (x * B) + 0i */
    CMPLXtrig1(old,g_new);
    *x  = PAR_C * g_new.x + tmp;

/*    *y = sin(xold * PAR_A) + PAR_D * sin(yold * PAR_A); */
    old.x = xold * PAR_A;
    old.y = 0;          /* old = (y * A) + 0i (in the complex)*/
    CMPLXtrig2(old,g_new);
    tmp = (double) g_new.x;
    old.x = yold * PAR_A;
    old.y = 0;          /* old = (x * B) + 0i */
    CMPLXtrig3(old,g_new);
    *y  = PAR_D * g_new.x + tmp;

    return(0);
}

#undef PAR_A
#undef PAR_B
#undef PAR_C
#undef PAR_D

/**********************************************************************/
/*   Main fractal engines - put in fractalspecific[fractype].calctype */
/**********************************************************************/

int inverse_julia_per_image()
{
   int color = 0;

   if (resuming)            /* can't resume */
      return -1;

   while (color >= 0)       /* generate points */
   {
      if (check_key())
      {
         Free_Queue();
         return -1;
      }
      color = curfractalspecific->orbitcalc();
      old = g_new;
   }
   Free_Queue();
   return 0;
}

int orbit2dfloat()
{
   FILE *fp;
   double *soundvar;
   double x,y,z;
   int color,col,row;
   int count;
   int oldrow, oldcol;
   double *p0,*p1,*p2;
   struct affine cvt;
   int ret;
   soundvar = p0 = p1 = p2 = NULL;

   fp = open_orbitsave();
   /* setup affine screen coord conversion */
   setup_convert_to_screen(&cvt);

   /* set up projection scheme */
   if(projection==0)
   {
      p0 = &z;
      p1 = &x;
      p2 = &y;
   }
   else if(projection==1)
   {
      p0 = &x;
      p1 = &z;
      p2 = &y;
   }
   else if(projection==2)
   {
      p0 = &x;
      p1 = &y;
      p2 = &z;
   }
   if((soundflag&7) == 2)
      soundvar = &x;
   else if((soundflag&7) == 3)
      soundvar = &y;
   else if((soundflag&7) == 4)
      soundvar = &z;

   if(inside > 0)
      color = inside;
   if(color >= colors)
      color = 1;
   oldcol = oldrow = -1;
   x = initorbitfp[0];
   y = initorbitfp[1];
   z = initorbitfp[2];
   coloriter = 0L;
   count = ret = 0;
   if(maxit > 0x1fffffL || maxct)
      maxct = 0x7fffffffL;
   else
      maxct = maxit*1024L;

   if (resuming)
   {
      start_resume();
      get_resume(sizeof(count),&count,sizeof(color),&color,
          sizeof(oldrow),&oldrow,sizeof(oldcol),&oldcol,
          sizeof(x),&x,sizeof(y),&y,sizeof(z),&z,sizeof(t),&t,
          sizeof(orbit),&orbit,sizeof(coloriter),&coloriter,
          0);
      end_resume();
   }

   while(coloriter++ <= maxct) /* loop until keypress or maxit */
   {
      if (driver_key_pressed())
      {
         driver_mute();
         alloc_resume(100,1);
         put_resume(sizeof(count),&count,sizeof(color),&color,
             sizeof(oldrow),&oldrow,sizeof(oldcol),&oldcol,
             sizeof(x),&x,sizeof(y),&y,sizeof(z),&z,sizeof(t),&t,
             sizeof(orbit),&orbit,sizeof(coloriter),&coloriter,
             0);
         ret = -1;
         break;
      }
      if (++count > 1000)
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
            color = 1;  /* (don't use the background color) */
      }

      col = (int)(cvt.a*x + cvt.b*y + cvt.e);
      row = (int)(cvt.c*x + cvt.d*y + cvt.f);
      if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
      {
         if ((soundflag&7) > 1)
            w_snd((int)(*soundvar*100+basehertz));
	 if((fractype!=ICON) && (fractype!=LATOO))
         {
         if(oldcol != -1 && connect)
            driver_draw_line(col,row,oldcol,oldrow,color%colors);
            else
            (*plot)(col,row,color%colors);
         } else {
            /* should this be using plothist()? */
            color = getcolor(col,row)+1;
            if( color < colors ) /* color sticks on last value */
               (*plot)(col,row,color);

         }

         oldcol = col;
         oldrow = row;
      }
      else if((long)abs(row) + (long)abs(col) > BAD_PIXEL) /* sanity check */
         return(ret);
      else
         oldrow = oldcol = -1;

      if(FORBIT(p0, p1, p2))
         break;
      if(fp)
         fprintf(fp,orbitsave_format,*p0,*p1,0.0);
   }
   if(fp)
      fclose(fp);
   return(ret);
}

int orbit2dlong()
{
   FILE *fp;
   long *soundvar;
   long x,y,z;
   int color,col,row;
   int count;
   int oldrow, oldcol;
   long *p0,*p1,*p2;
   struct l_affine cvt;
   int ret,start;
   start = 1;
   soundvar = p0 = p1 = p2 = NULL;
   fp = open_orbitsave();

   /* setup affine screen coord conversion */
   l_setup_convert_to_screen(&cvt);

   /* set up projection scheme */
   if(projection==0)
   {
      p0 = &z;
      p1 = &x;
      p2 = &y;
   }
   else if(projection==1)
   {
      p0 = &x;
      p1 = &z;
      p2 = &y;
   }
   else if(projection==2)
   {
      p0 = &x;
      p1 = &y;
      p2 = &z;
   }
   if((soundflag&7)==2)
      soundvar = &x;
   else if((soundflag&7)==3)
      soundvar = &y;
   else if((soundflag&7)==4)
      soundvar = &z;
   if(inside > 0)
      color = inside;
   if(color >= colors)
      color = 1;
   oldcol = oldrow = -1;
   x = initorbitlong[0];
   y = initorbitlong[1];
   z = initorbitlong[2];
   count = ret = 0;
   if(maxit > 0x1fffffL || maxct)
      maxct = 0x7fffffffL;
   else
      maxct = maxit*1024L;
   coloriter = 0L;

   if (resuming)
   {
      start_resume();
      get_resume(sizeof(count),&count,sizeof(color),&color,
          sizeof(oldrow),&oldrow,sizeof(oldcol),&oldcol,
          sizeof(x),&x,sizeof(y),&y,sizeof(z),&z,sizeof(t),&t,
          sizeof(l_orbit),&l_orbit,sizeof(coloriter),&coloriter,
          0);
      end_resume();
   }

   while(coloriter++ <= maxct) /* loop until keypress or maxit */
   {
      if (driver_key_pressed())
      {
         driver_mute();
         alloc_resume(100,1);
         put_resume(sizeof(count),&count,sizeof(color),&color,
             sizeof(oldrow),&oldrow,sizeof(oldcol),&oldcol,
             sizeof(x),&x,sizeof(y),&y,sizeof(z),&z,sizeof(t),&t,
             sizeof(l_orbit),&l_orbit,sizeof(coloriter),&coloriter,
             0);
         ret = -1;
         break;
      }
      if (++count > 1000)
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
            color = 1;  /* (don't use the background color) */
      }

      col = (int)((multiply(cvt.a,x,bitshift) + multiply(cvt.b,y,bitshift) + cvt.e) >> bitshift);
      row = (int)((multiply(cvt.c,x,bitshift) + multiply(cvt.d,y,bitshift) + cvt.f) >> bitshift);
      if(overflow)
      {
         overflow = 0;
         return(ret);
      }
      if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
      {
         if ((soundflag&7) > 1)
         {
            double yy;
            yy = *soundvar;
            yy = yy/fudge;
            w_snd((int)(yy*100+basehertz));
         }
         if(oldcol != -1 && connect)
            driver_draw_line(col,row,oldcol,oldrow,color%colors);
         else if(!start)
            (*plot)(col,row,color%colors);
         oldcol = col;
         oldrow = row;
         start = 0;
      }
      else if((long)abs(row) + (long)abs(col) > BAD_PIXEL) /* sanity check */
         return(ret);
      else
         oldrow = oldcol = -1;

      /* Calculate the next point */
      if(LORBIT(p0, p1, p2))
         break;
      if(fp)
         fprintf(fp,orbitsave_format,(double)*p0/fudge,(double)*p1/fudge,0.0);
   }
   if(fp)
      fclose(fp);
   return(ret);
}

static int orbit3dlongcalc(void)
{
   FILE *fp;
   unsigned long count;
   int oldcol,oldrow;
   int oldcol1,oldrow1;
   struct long3dvtinf inf;
   int color;
   int ret;

   /* setup affine screen coord conversion */
   l_setup_convert_to_screen(&inf.cvt);

   oldcol1 = oldrow1 = oldcol = oldrow = -1;
   color = 2;
   if(color >= colors)
      color = 1;

   inf.orbit[0] = initorbitlong[0];
   inf.orbit[1] = initorbitlong[1];
   inf.orbit[2] = initorbitlong[2];

   if(diskvideo)                /* this would KILL a disk drive! */
      notdiskmsg();

   fp = open_orbitsave();

   count = ret = 0;
   if(maxit > 0x1fffffL || maxct)
      maxct = 0x7fffffffL;
   else
      maxct = maxit*1024L;
   coloriter = 0L;
   while(coloriter++ <= maxct) /* loop until keypress or maxit */
   {
      /* calc goes here */
      if (++count > 1000)
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
            color = 1;        /* (don't use the background color) */
      }
      if (driver_key_pressed())
      {
         driver_mute();
         ret = -1;
         break;
      }

      LORBIT(&inf.orbit[0],&inf.orbit[1],&inf.orbit[2]);
      if(fp)
         fprintf(fp,orbitsave_format,(double)inf.orbit[0]/fudge,(double)inf.orbit[1]/fudge,(double)inf.orbit[2]/fudge);
      if (long3dviewtransf(&inf))
      {
         /* plot if inside window */
         if (inf.col >= 0)
         {
            if(realtime)
               whichimage=1;
            if ((soundflag&7) > 1)
            {
               double yy;
               yy = inf.viewvect[((soundflag&7) - 2)];
               yy = yy/fudge;
               w_snd((int)(yy*100+basehertz));
            }
            if(oldcol != -1 && connect)
               driver_draw_line(inf.col,inf.row,oldcol,oldrow,color%colors);
            else
               (*plot)(inf.col,inf.row,color%colors);
         }
         else if (inf.col == -2)
            return(ret);
         oldcol = inf.col;
         oldrow = inf.row;
         if(realtime)
         {
            whichimage=2;
            /* plot if inside window */
            if (inf.col1 >= 0)
            {
               if(oldcol1 != -1 && connect)
                  driver_draw_line(inf.col1,inf.row1,oldcol1,oldrow1,color%colors);
               else
                  (*plot)(inf.col1,inf.row1,color%colors);
            }
            else if (inf.col1 == -2)
               return(ret);
            oldcol1 = inf.col1;
            oldrow1 = inf.row1;
         }
      }
   }
   if(fp)
      fclose(fp);
   return(ret);
}


static int orbit3dfloatcalc(void)
{
   FILE *fp;
   unsigned long count;
   int oldcol,oldrow;
   int oldcol1,oldrow1;
   int color;
   int ret;
   struct float3dvtinf inf;

   /* setup affine screen coord conversion */
   setup_convert_to_screen(&inf.cvt);

   oldcol = oldrow = -1;
   oldcol1 = oldrow1 = -1;
   color = 2;
   if(color >= colors)
      color = 1;
   inf.orbit[0] = initorbitfp[0];
   inf.orbit[1] = initorbitfp[1];
   inf.orbit[2] = initorbitfp[2];

   if(diskvideo)                /* this would KILL a disk drive! */
      notdiskmsg();

   fp = open_orbitsave();

   ret = 0;
   if(maxit > 0x1fffffL || maxct)
      maxct = 0x7fffffffL;
   else
      maxct = maxit*1024L;
   count = coloriter = 0L;
   while(coloriter++ <= maxct) /* loop until keypress or maxit */
   {
      /* calc goes here */
      if (++count > 1000)
      {        /* time to switch colors? */
         count = 0;
         if (++color >= colors)   /* another color to switch to? */
            color = 1;        /* (don't use the background color) */
      }

      if (driver_key_pressed())
      {
         driver_mute();
         ret = -1;
         break;
      }

      FORBIT(&inf.orbit[0],&inf.orbit[1],&inf.orbit[2]);
      if(fp)
         fprintf(fp,orbitsave_format,inf.orbit[0],inf.orbit[1],inf.orbit[2]);
      if (float3dviewtransf(&inf))
      {
         /* plot if inside window */
         if (inf.col >= 0)
         {
            if(realtime)
               whichimage=1;
            if ((soundflag&7) > 1) {
               w_snd((int)(inf.viewvect[((soundflag&7) - 2)]*100+basehertz));
            }
            if(oldcol != -1 && connect)
               driver_draw_line(inf.col,inf.row,oldcol,oldrow,color%colors);
            else
               (*plot)(inf.col,inf.row,color%colors);
         }
         else if (inf.col == -2)
            return(ret);
         oldcol = inf.col;
         oldrow = inf.row;
         if(realtime)
         {
            whichimage=2;
            /* plot if inside window */
            if (inf.col1 >= 0)
            {
               if(oldcol1 != -1 && connect)
                  driver_draw_line(inf.col1,inf.row1,oldcol1,oldrow1,color%colors);
               else
                  (*plot)(inf.col1,inf.row1,color%colors);
            }
            else if (inf.col1 == -2)
               return(ret);
            oldcol1 = inf.col1;
            oldrow1 = inf.row1;
         }
      }
   }
   if(fp)
      fclose(fp);
   return(ret);
}

int dynam2dfloatsetup()
{
   connect = 0;
   euler = 0;
   d = param[0]; /* number of intervals */
   if (d<0) {
      d = -d;
      connect = 1;
   }
   else if (d==0) {
      d = 1;
   }
   if (fractype==DYNAMICFP) {
       a = param[2]; /* parameter */
       b = param[3]; /* parameter */
       dt = param[1]; /* step size */
       if (dt<0) {
          dt = -dt;
          euler = 1;
       }
       if (dt==0) dt = 0.01;
   }
   if (outside == SUM) {
       plot = plothist;
   }
   return(1);
}

/*
 * This is the routine called to perform a time-discrete dynamical
 * system image.
 * The starting positions are taken by stepping across the image in steps
 * of parameter1 pixels.  maxit differential equation steps are taken, with
 * a step size of parameter2.
 */
int dynam2dfloat()
{
   FILE *fp;
   double *soundvar = NULL;
   double x,y,z;
   int color,col,row;
   long count;
   int oldrow, oldcol;
   double *p0,*p1;
   struct affine cvt;
   int ret;
   int xstep, ystep; /* The starting position step number */
   double xpixel, ypixel; /* Our pixel position on the screen */

   fp = open_orbitsave();
   /* setup affine screen coord conversion */
   setup_convert_to_screen(&cvt);

   p0 = &x;
   p1 = &y;


   if((soundflag&7)==2)
      soundvar = &x;
   else if((soundflag&7)==3)
      soundvar = &y;
   else if((soundflag&7)==4)
      soundvar = &z;

   count = 0;
   if(inside > 0)
      color = inside;
   if(color >= colors)
      color = 1;
   oldcol = oldrow = -1;

   xstep = -1;
   ystep = 0;

   if (resuming) {
       start_resume();
       get_resume(sizeof(count),&count, sizeof(color),&color,
                 sizeof(oldrow),&oldrow, sizeof(oldcol),&oldcol,
                 sizeof(x),&x, sizeof(y), &y, sizeof(xstep), &xstep,
                 sizeof(ystep), &ystep, 0);
       end_resume();
   }

   ret = 0;
   for(;;)
   {
      if (driver_key_pressed())
      {
             driver_mute();
             alloc_resume(100,1);
             put_resume(sizeof(count),&count, sizeof(color),&color,
                     sizeof(oldrow),&oldrow, sizeof(oldcol),&oldcol,
                     sizeof(x),&x, sizeof(y), &y, sizeof(xstep), &xstep,
                     sizeof(ystep), &ystep, 0);
             ret = -1;
             break;
      }

      xstep ++;
      if (xstep>=d) {
          xstep = 0;
          ystep ++;
          if (ystep>d) {
              driver_mute();
              ret = -1;
              break;
          }
      }

      xpixel = dxsize*(xstep+.5)/d;
      ypixel = dysize*(ystep+.5)/d;
      x = (double)((xxmin+delxx*xpixel) + (delxx2*ypixel));
      y = (double)((yymax-delyy*ypixel) + (-delyy2*xpixel));
      if (fractype==MANDELCLOUD) {
          a = x;
          b = y;
      }
      oldcol = -1;

      if (++color >= colors)   /* another color to switch to? */
          color = 1;    /* (don't use the background color) */

      for (count=0;count<maxit;count++) {

          if (count % 2048L == 0)
             if (driver_key_pressed())
                 break;

          col = (int)(cvt.a*x + cvt.b*y + cvt.e);
          row = (int)(cvt.c*x + cvt.d*y + cvt.f);
          if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
          {
             if ((soundflag&7) > 1)
               w_snd((int)(*soundvar*100+basehertz));

             if (count>=orbit_delay) {
                 if(oldcol != -1 && connect)
                    driver_draw_line(col,row,oldcol,oldrow,color%colors);
                 else if(count > 0 || fractype != MANDELCLOUD)
                    (*plot)(col,row,color%colors);
             }
             oldcol = col;
             oldrow = row;
          }
          else if((long)abs(row) + (long)abs(col) > BAD_PIXEL) /* sanity check */
            return(ret);
          else
             oldrow = oldcol = -1;

          if(FORBIT(p0, p1, NULL))
             break;
          if(fp)
              fprintf(fp,orbitsave_format,*p0,*p1,0.0);
        }
   }
   if(fp)
      fclose(fp);
   return(ret);
}

int keep_scrn_coords = 0;
int set_orbit_corners = 0;
long orbit_interval;
double oxmin, oymin, oxmax, oymax, ox3rd, oy3rd;
struct affine o_cvt;
static int o_color;

int setup_orbits_to_screen(struct affine *scrn_cnvt)
{
   double det, xd, yd;

   if((det = (ox3rd-oxmin)*(oymin-oymax) + (oymax-oy3rd)*(oxmax-oxmin))==0)
      return(-1);
   xd = dxsize/det;
   scrn_cnvt->a =  xd*(oymax-oy3rd);
   scrn_cnvt->b =  xd*(ox3rd-oxmin);
   scrn_cnvt->e = -scrn_cnvt->a*oxmin - scrn_cnvt->b*oymax;

   if((det = (ox3rd-oxmax)*(oymin-oymax) + (oymin-oy3rd)*(oxmax-oxmin))==0)
      return(-1);
   yd = dysize/det;
   scrn_cnvt->c =  yd*(oymin-oy3rd);
   scrn_cnvt->d =  yd*(ox3rd-oxmax);
   scrn_cnvt->f = -scrn_cnvt->c*oxmin - scrn_cnvt->d*oymax;
   return(0);
}

int plotorbits2dsetup(void)
{

#ifndef XFRACT
   if (curfractalspecific->isinteger != 0) {
      int tofloat;
      if ((tofloat = curfractalspecific->tofloat) == NOFRACTAL)
         return(-1);
      floatflag = usr_floatflag = 1; /* force floating point */
      curfractalspecific = &fractalspecific[tofloat];
      fractype = tofloat;
   }
#endif

   PER_IMAGE();

   /* setup affine screen coord conversion */
   if (keep_scrn_coords) {
      if(setup_orbits_to_screen(&o_cvt))
         return(-1);
   } else {
      if(setup_convert_to_screen(&o_cvt))
         return(-1);
   }
   /* set so truncation to int rounds to nearest */
   o_cvt.e += 0.5;
   o_cvt.f += 0.5;

   if (orbit_delay >= maxit) /* make sure we get an image */
      orbit_delay = (int)(maxit - 1);

   o_color = 1;

   if (outside == SUM) {
       plot = plothist;
   }
   return(1);
}

int plotorbits2dfloat(void)
{
   double *soundvar = NULL;
   double x,y,z;
   int col,row;
   long count;

   if (driver_key_pressed())
   {
      driver_mute();
      alloc_resume(100,1);
      put_resume(sizeof(o_color),&o_color, 0);
      return(-1);
   }

#if 0
    col = (int)(o_cvt.a*g_new.x + o_cvt.b*g_new.y + o_cvt.e);
    row = (int)(o_cvt.c*g_new.x + o_cvt.d*g_new.y + o_cvt.f);
    if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
       (*plot)(col,row,1);
    return(0);
#endif

   if((soundflag&7)==2)
      soundvar = &x;
   else if((soundflag&7)==3)
      soundvar = &y;
   else if((soundflag&7)==4)
      soundvar = &z;

   if (resuming) {
       start_resume();
       get_resume(sizeof(o_color),&o_color, 0);
       end_resume();
   }

   if(inside > 0)
      o_color = inside;
   else { /* inside <= 0 */
      o_color++;
      if (o_color >= colors) /* another color to switch to? */
          o_color = 1;    /* (don't use the background color) */
   }

   PER_PIXEL(); /* initialize the calculations */

   for (count = 0; count < maxit; count++) {

       if (ORBITCALC() == 1 && periodicitycheck)
          continue;  /* bailed out, don't plot */

       if (count < orbit_delay || count%orbit_interval)
          continue;  /* don't plot it */

       /* else count >= orbit_delay and we want to plot it */
       col = (int)(o_cvt.a*g_new.x + o_cvt.b*g_new.y + o_cvt.e);
       row = (int)(o_cvt.c*g_new.x + o_cvt.d*g_new.y + o_cvt.f);
#ifdef XFRACT
       if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
#else
/* don't know why the next line is necessary, the one above should work */
       if ( col > 0 && col < xdots && row > 0 && row < ydots )
#endif
       {             /* plot if on the screen */
          if ((soundflag&7) > 1)
             w_snd((int)(*soundvar*100+basehertz));

          (*plot)(col,row,o_color%colors);
       }
       else
       {             /* off screen, don't continue unless periodicity=0 */
          if (periodicitycheck)
             return(0); /* skip to next pixel */
       }
   }
   return(0);
}

/* this function's only purpose is to manage funnyglasses related */
/* stuff so the code is not duplicated for ifs3d() and lorenz3d() */
int funny_glasses_call(int (*calc)(void))
{
   int status;
   status = 0;
   if(glassestype)
      whichimage = 1;
   else
      whichimage = 0;
   plot_setup();
   plot = standardplot;
   status = calc();
   if(realtime && glassestype < 3)
   {
      realtime = 0;
      goto done;
   }
   if(glassestype && status == 0 && display3d)
   {
      if(glassestype==3)  { /* photographer's mode */
         if(active_system == 0) { /* dos version */
            int i;
static char firstready[]={"\
First image (left eye) is ready.  Hit any key to see it,\n\
then hit <s> to save, hit any other key to create second image."};
            stopmsg(STOPMSG_INFO_ONLY,firstready);
            while ((i = driver_get_key()) == 's' || i == 'S') {
               diskisactive = 1;
               savetodisk(savename);
               diskisactive = 0;
               }
            /* is there a better way to clear the screen in graphics mode? */
            driver_set_video_mode(g_video_entry.videomodeax,
                g_video_entry.videomodebx,
                g_video_entry.videomodecx,
                g_video_entry.videomodedx);
         }
         else {                   /* Windows version */
static char firstready2[]={"First (Left Eye) image is complete"};
            stopmsg(0,firstready2);
            clear_screen(0);
            }
      }
      whichimage = 2;
      if(curfractalspecific->flags & INFCALC)
         curfractalspecific->per_image(); /* reset for 2nd image */
      plot_setup();
      plot = standardplot;
      /* is there a better way to clear the graphics screen ? */
      if((status = calc()) != 0)
         goto done;
      if(glassestype==3) /* photographer's mode */
         if(active_system == 0) { /* dos version */
static char secondready[]={"Second image (right eye) is ready"};
            stopmsg(STOPMSG_INFO_ONLY,secondready);
         }
   }
done:
   if(glassestype == 4 && sxdots >= 2*xdots)
   {
      /* turn off view windows so will save properly */
      sxoffs = syoffs = 0;
      xdots = sxdots;
      ydots = sydots;
      viewwindow = 0;
   }
   return(status);
}

/* double version - mainly for testing */
static int ifs3dfloat(void)
{
   int color_method;
   FILE *fp;
   int color;

   double newx,newy,newz,r,sum;

   int k;
   int ret;

   struct float3dvtinf inf;

   float *ffptr;

   /* setup affine screen coord conversion */
   setup_convert_to_screen(&inf.cvt);
   srand(1);
   color_method = (int)param[0];
   if(diskvideo)                /* this would KILL a disk drive! */
      notdiskmsg();

   inf.orbit[0] = 0;
   inf.orbit[1] = 0;
   inf.orbit[2] = 0;

   fp = open_orbitsave();

   ret = 0;
   if(maxit > 0x1fffffL)
      maxct = 0x7fffffffL;
   else
      maxct = maxit*1024;
   coloriter = 0L;
   while(coloriter++ <= maxct) /* loop until keypress or maxit */
   {
      if (driver_key_pressed())  /* keypress bails out */
      {
         ret = -1;
         break;
      }
      r = rand();      /* generate a random number between 0 and 1 */
      r /= RAND_MAX;

      /* pick which iterated function to execute, weighted by probability */
      sum = ifs_defn[12]; /* [0][12] */
      k = 0;
      while ( sum < r && ++k < numaffine*IFS3DPARM)
      {
         sum += ifs_defn[k*IFS3DPARM+12];
         if (ifs_defn[(k+1)*IFS3DPARM+12] == 0) break; /* for safety */
      }

      /* calculate image of last point under selected iterated function */
      ffptr = ifs_defn + k*IFS3DPARM; /* point to first parm in row */
      newx = *ffptr * inf.orbit[0] +
             *(ffptr+1) * inf.orbit[1] +
             *(ffptr+2) * inf.orbit[2] + *(ffptr+9);
      newy = *(ffptr+3) * inf.orbit[0] +
             *(ffptr+4) * inf.orbit[1] +
             *(ffptr+5) * inf.orbit[2] + *(ffptr+10);
      newz = *(ffptr+6) * inf.orbit[0] +
             *(ffptr+7) * inf.orbit[1] +
             *(ffptr+8) * inf.orbit[2] + *(ffptr+11);

      inf.orbit[0] = newx;
      inf.orbit[1] = newy;
      inf.orbit[2] = newz;
      if(fp)
          fprintf(fp,orbitsave_format,newx,newy,newz);
      if (float3dviewtransf(&inf))
      {
         /* plot if inside window */
         if (inf.col >= 0)
         {
            if(realtime)
               whichimage=1;
            if(color_method)
               color = (k%colors)+1;
            else
            color = getcolor(inf.col,inf.row)+1;
            if( color < colors ) /* color sticks on last value */
               (*plot)(inf.col,inf.row,color);
         }
         else if (inf.col == -2)
            return(ret);
         if(realtime)
         {
            whichimage=2;
            /* plot if inside window */
            if (inf.col1 >= 0)
            {
              if(color_method)
                 color = (k%colors)+1;
              else
                color = getcolor(inf.col1,inf.row1)+1;
                if( color < colors ) /* color sticks on last value */
                  (*plot)(inf.col1,inf.row1,color);
            }
            else if (inf.col1 == -2)
               return(ret);
         }
      }
   } /* end while */
   if(fp)
      fclose(fp);
   return(ret);
}

int ifs()                       /* front-end for ifs2d and ifs3d */
{
   if (ifs_defn == NULL && ifsload() < 0)
      return(-1);
   if(diskvideo)                /* this would KILL a disk drive! */
      notdiskmsg();
   return((ifs_type == 0) ? ifs2d() : ifs3d());
}


/* IFS logic shamelessly converted to integer math */
static int ifs2d(void)
{
   int color_method;
   FILE *fp;
   int col;
   int row;
   int color;
   int ret;
   long *localifs;
   long *lfptr;
   long x,y,newx,newy,r,sum, tempr;

   int i,j,k;
   struct l_affine cvt;
   /* setup affine screen coord conversion */
   l_setup_convert_to_screen(&cvt);

   srand(1);
   color_method = (int)param[0];
   if((localifs=(long *)malloc((long)numaffine*IFSPARM*sizeof(long)))==NULL)
   {
      stopmsg(0,insufficient_ifs_mem);
      return(-1);
   }

   for (i = 0; i < numaffine; i++)    /* fill in the local IFS array */
      for (j = 0; j < IFSPARM; j++)
         localifs[i*IFSPARM+j] = (long)(ifs_defn[i*IFSPARM+j] * fudge);

   tempr = fudge / 32767;        /* find the proper rand() fudge */

   fp = open_orbitsave();

   x = y = 0;
   ret = 0;
   if(maxit > 0x1fffffL)
      maxct = 0x7fffffffL;
   else
      maxct = maxit*1024L;
   coloriter = 0L;
   while(coloriter++ <= maxct) /* loop until keypress or maxit */
   {
      if (driver_key_pressed())  /* keypress bails out */
      {
         ret = -1;
         break;
      }
      r = rand15();      /* generate fudged random number between 0 and 1 */
      r *= tempr;

      /* pick which iterated function to execute, weighted by probability */
      sum = localifs[6];  /* [0][6] */
      k = 0;
      while ( sum < r && k < numaffine-1) /* fixed bug of error if sum < 1 */
         sum += localifs[++k*IFSPARM+6];
      /* calculate image of last point under selected iterated function */
      lfptr = localifs + k*IFSPARM; /* point to first parm in row */
      newx = multiply(lfptr[0],x,bitshift) +
             multiply(lfptr[1],y,bitshift) + lfptr[4];
      newy = multiply(lfptr[2],x,bitshift) +
             multiply(lfptr[3],y,bitshift) + lfptr[5];
      x = newx;
      y = newy;
      if(fp)
         fprintf(fp,orbitsave_format,(double)newx/fudge,(double)newy/fudge,0.0);

      /* plot if inside window */
      col = (int)((multiply(cvt.a,x,bitshift) + multiply(cvt.b,y,bitshift) + cvt.e) >> bitshift);
      row = (int)((multiply(cvt.c,x,bitshift) + multiply(cvt.d,y,bitshift) + cvt.f) >> bitshift);
      if ( col >= 0 && col < xdots && row >= 0 && row < ydots )
      {
         /* color is count of hits on this pixel */
         if(color_method)
            color = (k%colors)+1;
         else
         color = getcolor(col,row)+1;
         if( color < colors ) /* color sticks on last value */
            (*plot)(col,row,color);
      }
      else if((long)abs(row) + (long)abs(col) > BAD_PIXEL) /* sanity check */
            return(ret);
   }
   if(fp)
      fclose(fp);
   free(localifs);
   return(ret);
}

static int ifs3dlong(void)
{
   int color_method;
   FILE *fp;
   int color;
   int ret;

   long *localifs;
   long *lfptr;
   long newx,newy,newz,r,sum, tempr;

   int i,j,k;

   struct long3dvtinf inf;
   srand(1);
   color_method = (int)param[0];
   if((localifs=(long *)malloc((long)numaffine*IFS3DPARM*sizeof(long)))==NULL)
   {
      stopmsg(0,insufficient_ifs_mem);
      return(-1);
   }

   /* setup affine screen coord conversion */
   l_setup_convert_to_screen(&inf.cvt);

   for (i = 0; i < numaffine; i++)    /* fill in the local IFS array */
      for (j = 0; j < IFS3DPARM; j++)
         localifs[i*IFS3DPARM+j] = (long)(ifs_defn[i*IFS3DPARM+j] * fudge);

   tempr = fudge / 32767;        /* find the proper rand() fudge */

   inf.orbit[0] = 0;
   inf.orbit[1] = 0;
   inf.orbit[2] = 0;

   fp = open_orbitsave();

   ret = 0;
   if(maxit > 0x1fffffL)
      maxct = 0x7fffffffL;
   else
      maxct = maxit*1024L;
   coloriter = 0L;
   while(coloriter++ <= maxct) /* loop until keypress or maxit */
   {
      if (driver_key_pressed())  /* keypress bails out */
      {
         ret = -1;
         break;
      }
      r = rand15();      /* generate fudged random number between 0 and 1 */
      r *= tempr;

      /* pick which iterated function to execute, weighted by probability */
      sum = localifs[12];  /* [0][12] */
      k = 0;
      while ( sum < r && ++k < numaffine*IFS3DPARM)
      {
         sum += localifs[k*IFS3DPARM+12];
         if (ifs_defn[(k+1)*IFS3DPARM+12] == 0) break; /* for safety */
      }

      /* calculate image of last point under selected iterated function */
      lfptr = localifs + k*IFS3DPARM; /* point to first parm in row */

      /* calculate image of last point under selected iterated function */
      newx = multiply(lfptr[0], inf.orbit[0], bitshift) +
             multiply(lfptr[1], inf.orbit[1], bitshift) +
             multiply(lfptr[2], inf.orbit[2], bitshift) + lfptr[9];
      newy = multiply(lfptr[3], inf.orbit[0], bitshift) +
             multiply(lfptr[4], inf.orbit[1], bitshift) +
             multiply(lfptr[5], inf.orbit[2], bitshift) + lfptr[10];
      newz = multiply(lfptr[6], inf.orbit[0], bitshift) +
             multiply(lfptr[7], inf.orbit[1], bitshift) +
             multiply(lfptr[8], inf.orbit[2], bitshift) + lfptr[11];

      inf.orbit[0] = newx;
      inf.orbit[1] = newy;
      inf.orbit[2] = newz;
      if(fp)
         fprintf(fp,orbitsave_format,(double)newx/fudge,(double)newy/fudge,(double)newz/fudge);

      if (long3dviewtransf(&inf))
      {
         if((long)abs(inf.row) + (long)abs(inf.col) > BAD_PIXEL) /* sanity check */
            return(ret);
         /* plot if inside window */
         if (inf.col >= 0)
         {
            if(realtime)
               whichimage=1;
            if(color_method)
               color = (k%colors)+1;
            else
            color = getcolor(inf.col,inf.row)+1;
            if( color < colors ) /* color sticks on last value */
               (*plot)(inf.col,inf.row,color);
         }
         if(realtime)
         {
            whichimage=2;
            /* plot if inside window */
            if (inf.col1 >= 0)
            {
               if(color_method)
                  color = (k%colors)+1;
               else
               color = getcolor(inf.col1,inf.row1)+1;
               if( color < colors ) /* color sticks on last value */
                  (*plot)(inf.col1,inf.row1,color);
            }
         }
      }
   }
   if(fp)
      fclose(fp);
   free(localifs);
   return(ret);
}

static void setupmatrix(MATRIX doublemat)
{
   /* build transformation matrix */
   identity (doublemat);

   /* apply rotations - uses the same rotation variables as line3d.c */
   xrot ((double)XROT / 57.29577,doublemat);
   yrot ((double)YROT / 57.29577,doublemat);
   zrot ((double)ZROT / 57.29577,doublemat);

   /* apply scale */
/*   scale((double)XSCALE/100.0,(double)YSCALE/100.0,(double)ROUGH/100.0,doublemat);*/

}

int orbit3dfloat()
{
   display3d = -1;
   if(0 < glassestype && glassestype < 3)
      realtime = 1;
   else
      realtime = 0;
   return(funny_glasses_call(orbit3dfloatcalc));
}

int orbit3dlong()
{
   display3d = -1;
   if(0 < glassestype && glassestype < 3)
      realtime = 1;
   else
      realtime = 0;
   return(funny_glasses_call(orbit3dlongcalc));
}

static int ifs3d(void)
{
   display3d = -1;

   if(0 < glassestype && glassestype < 3)
      realtime = 1;
   else
      realtime = 0;
   if(floatflag)
      return(funny_glasses_call(ifs3dfloat)); /* double version of ifs3d */
   else
      return(funny_glasses_call(ifs3dlong));  /* long version of ifs3d   */
}



static int long3dviewtransf(struct long3dvtinf *inf)
{
   int i,j;
   double tmpx, tmpy, tmpz;
   long tmp;

   if (coloriter == 1)  /* initialize on first call */
   {
      for(i=0;i<3;i++)
      {
         inf->minvals[i] =  1L << 30;
         inf->maxvals[i] = -inf->minvals[i];
      }
      setupmatrix(inf->doublemat);
      if(realtime)
         setupmatrix(inf->doublemat1);
      /* copy xform matrix to long for for fixed point math */
      for (i = 0; i < 4; i++)
         for (j = 0; j < 4; j++)
         {
            inf->longmat[i][j] = (long)(inf->doublemat[i][j] * fudge);
            if(realtime)
               inf->longmat1[i][j] = (long)(inf->doublemat1[i][j] * fudge);
         }
   }

   /* 3D VIEWING TRANSFORM */
   longvmult(inf->orbit,inf->longmat,inf->viewvect,bitshift);
   if(realtime)
      longvmult(inf->orbit,inf->longmat1,inf->viewvect1,bitshift);

   if(coloriter <= waste) /* waste this many points to find minz and maxz */
   {
      /* find minz and maxz */
      for(i=0;i<3;i++)
         if ((tmp = inf->viewvect[i]) < inf->minvals[i])
            inf->minvals[i] = tmp;
         else if (tmp > inf->maxvals[i])
            inf->maxvals[i] = tmp;

      if(coloriter == waste) /* time to work it out */
      {
         inf->iview[0] = inf->iview[1] = 0L; /* center viewer on origin */

         /* z value of user's eye - should be more negative than extreme
                        negative part of image */
         inf->iview[2] = (long)((inf->minvals[2]-inf->maxvals[2])*(double)ZVIEWER/100.0);

         /* center image on origin */
         tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0*fudge); /* center x */
         tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0*fudge); /* center y */

         /* apply perspective shift */
         tmpx += ((double)xshift*(xxmax-xxmin))/(xdots);
         tmpy += ((double)yshift*(yymax-yymin))/(ydots);
         tmpz = -((double)inf->maxvals[2]) / fudge;
         trans(tmpx,tmpy,tmpz,inf->doublemat);

         if(realtime)
         {
            /* center image on origin */
            tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0*fudge); /* center x */
            tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0*fudge); /* center y */

            tmpx += ((double)xshift1*(xxmax-xxmin))/(xdots);
            tmpy += ((double)yshift1*(yymax-yymin))/(ydots);
            tmpz = -((double)inf->maxvals[2]) / fudge;
            trans(tmpx,tmpy,tmpz,inf->doublemat1);
         }
         for(i=0;i<3;i++)
            view[i] = (double)inf->iview[i] / fudge;

         /* copy xform matrix to long for for fixed point math */
         for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
            {
               inf->longmat[i][j] = (long)(inf->doublemat[i][j] * fudge);
               if(realtime)
                  inf->longmat1[i][j] = (long)(inf->doublemat1[i][j] * fudge);
            }
      }
      return(0);
   }

   /* apply perspective if requested */
   if(ZVIEWER)
   {
      if(debugflag==22 || ZVIEWER < 100) /* use float for small persp */
      {
         /* use float perspective calc */
         VECTOR tmpv;
         for(i=0;i<3;i++)
            tmpv[i] = (double)inf->viewvect[i] / fudge;
         perspective(tmpv);
         for(i=0;i<3;i++)
            inf->viewvect[i] = (long)(tmpv[i]*fudge);
         if(realtime)
         {
            for(i=0;i<3;i++)
               tmpv[i] = (double)inf->viewvect1[i] / fudge;
            perspective(tmpv);
            for(i=0;i<3;i++)
               inf->viewvect1[i] = (long)(tmpv[i]*fudge);
         }
      }
      else
      {
         longpersp(inf->viewvect,inf->iview,bitshift);
         if(realtime)
            longpersp(inf->viewvect1,inf->iview,bitshift);
      }
   }

   /* work out the screen positions */
   inf->row = (int)(((multiply(inf->cvt.c,inf->viewvect[0],bitshift) +
                multiply(inf->cvt.d,inf->viewvect[1],bitshift) + inf->cvt.f)
                >> bitshift)
              + yyadjust);
   inf->col = (int)(((multiply(inf->cvt.a,inf->viewvect[0],bitshift) +
                multiply(inf->cvt.b,inf->viewvect[1],bitshift) + inf->cvt.e)
                >> bitshift)
              + xxadjust);
   if (inf->col < 0 || inf->col >= xdots || inf->row < 0 || inf->row >= ydots)
   {
      if((long)abs(inf->col)+(long)abs(inf->row) > BAD_PIXEL)
        inf->col= inf->row = -2;
      else
        inf->col= inf->row = -1;
   }
   if(realtime)
   {
      inf->row1 = (int)(((multiply(inf->cvt.c,inf->viewvect1[0],bitshift) +
                    multiply(inf->cvt.d,inf->viewvect1[1],bitshift) +
                    inf->cvt.f) >> bitshift)
                  + yyadjust1);
      inf->col1 = (int)(((multiply(inf->cvt.a,inf->viewvect1[0],bitshift) +
                    multiply(inf->cvt.b,inf->viewvect1[1],bitshift) +
                    inf->cvt.e) >> bitshift)
                  + xxadjust1);
      if (inf->col1 < 0 || inf->col1 >= xdots || inf->row1 < 0 || inf->row1 >= ydots)
      {
         if((long)abs(inf->col1)+(long)abs(inf->row1) > BAD_PIXEL)
           inf->col1= inf->row1 = -2;
         else
           inf->col1= inf->row1 = -1;
      }
   }
   return(1);
}

static int float3dviewtransf(struct float3dvtinf *inf)
{
   int i;
   double tmpx, tmpy, tmpz;
   double tmp;

   if (coloriter == 1)  /* initialize on first call */
   {
      for(i=0;i<3;i++)
      {
         inf->minvals[i] =  100000.0; /* impossible value */
         inf->maxvals[i] = -100000.0;
      }
      setupmatrix(inf->doublemat);
      if(realtime)
         setupmatrix(inf->doublemat1);
   }

   /* 3D VIEWING TRANSFORM */
   vmult(inf->orbit,inf->doublemat,inf->viewvect );
   if(realtime)
      vmult(inf->orbit,inf->doublemat1,inf->viewvect1);

   if(coloriter <= waste) /* waste this many points to find minz and maxz */
   {
      /* find minz and maxz */
      for(i=0;i<3;i++)
         if ((tmp = inf->viewvect[i]) < inf->minvals[i])
            inf->minvals[i] = tmp;
         else if (tmp > inf->maxvals[i])
            inf->maxvals[i] = tmp;
      if(coloriter == waste) /* time to work it out */
      {
         view[0] = view[1] = 0; /* center on origin */
         /* z value of user's eye - should be more negative than extreme
                           negative part of image */
         view[2] = (inf->minvals[2]-inf->maxvals[2])*(double)ZVIEWER/100.0;

         /* center image on origin */
         tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0); /* center x */
         tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0); /* center y */

         /* apply perspective shift */
         tmpx += ((double)xshift*(xxmax-xxmin))/(xdots);
         tmpy += ((double)yshift*(yymax-yymin))/(ydots);
         tmpz = -(inf->maxvals[2]);
         trans(tmpx,tmpy,tmpz,inf->doublemat);

         if(realtime)
         {
            /* center image on origin */
            tmpx = (-inf->minvals[0]-inf->maxvals[0])/(2.0); /* center x */
            tmpy = (-inf->minvals[1]-inf->maxvals[1])/(2.0); /* center y */

            tmpx += ((double)xshift1*(xxmax-xxmin))/(xdots);
            tmpy += ((double)yshift1*(yymax-yymin))/(ydots);
            tmpz = -(inf->maxvals[2]);
            trans(tmpx,tmpy,tmpz,inf->doublemat1);
            }
         }
      return(0);
      }

   /* apply perspective if requested */
   if(ZVIEWER)
   {
      perspective(inf->viewvect);
      if(realtime)
         perspective(inf->viewvect1);
   }
   inf->row = (int)(inf->cvt.c*inf->viewvect[0] + inf->cvt.d*inf->viewvect[1]
            + inf->cvt.f + yyadjust);
   inf->col = (int)(inf->cvt.a*inf->viewvect[0] + inf->cvt.b*inf->viewvect[1]
            + inf->cvt.e + xxadjust);
   if (inf->col < 0 || inf->col >= xdots || inf->row < 0 || inf->row >= ydots)
   {
      if((long)abs(inf->col)+(long)abs(inf->row) > BAD_PIXEL)
        inf->col= inf->row = -2;
      else
        inf->col= inf->row = -1;
   }
   if(realtime)
   {
      inf->row1 = (int)(inf->cvt.c*inf->viewvect1[0] + inf->cvt.d*inf->viewvect1[1]
                + inf->cvt.f + yyadjust1);
      inf->col1 = (int)(inf->cvt.a*inf->viewvect1[0] + inf->cvt.b*inf->viewvect1[1]
                + inf->cvt.e + xxadjust1);
      if (inf->col1 < 0 || inf->col1 >= xdots || inf->row1 < 0 || inf->row1 >= ydots)
      {
         if((long)abs(inf->col1)+(long)abs(inf->row1) > BAD_PIXEL)
           inf->col1= inf->row1 = -2;
         else
           inf->col1= inf->row1 = -1;
      }
   }
   return(1);
}

static FILE *open_orbitsave(void)
{
   FILE *fp;
   if ((orbitsave&1) && (fp = fopen(orbitsavename,"w")) != NULL)
   {
      fprintf(fp,"pointlist x y z color\n");
      return fp;
   }
   return NULL;
}

/* Plot a histogram by incrementing the pixel each time it it touched */
static void _fastcall plothist(int x, int y, int color)
{
    color = getcolor(x,y)+1;
    if (color >= colors)
        color = 1;
    putcolor(x,y,color);
}

