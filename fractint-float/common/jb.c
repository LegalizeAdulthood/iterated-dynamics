
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"

/* these need to be accessed elsewhere for saving data */
double mxminfp = -.83;
double myminfp = -.25;
double mxmaxfp = -.83;
double mymaxfp =  .25;

static double x_per_inchfp, y_per_inchfp, inch_per_xdotfp, inch_per_ydotfp;
static int bbase;
static double xpixelfp, ypixelfp;
static double initzfp, djxfp, djyfp, dmxfp, dmyfp;
static double jxfp, jyfp, mxfp, myfp, xoffsetfp, yoffsetfp;

struct Perspectivefp
{
   double x, y, zx, zy;
};

struct Perspectivefp LeftEyefp, RightEyefp, *Perfp;

_CMPLX jbcfp;

int zdots = 128;

float originfp  = (float)8.0;
float heightfp  = (float)7.0;
float widthfp   = (float)10.0;
float distfp    = (float)24.0;
float eyesfp    = (float)2.5;
float depthfp   = (float)8.0;
float brratiofp = (float)1.0;

int juli3Dmode = 0;

int neworbittype = JULIAFP;

int
JulibrotSetup(void)
{
   int r = 0;
   char *mapname;

#ifndef XFRACT
   if (colors < 255)
   {
      static FCODE msg[] =
      {"Sorry, but Julibrots require a 256-color video mode"};
      stopmsg(0, msg);
      return (0);
   }
#endif

   xoffsetfp = (xxmax + xxmin) / 2;     /* Calculate average */
   yoffsetfp = (yymax + yymin) / 2;     /* Calculate average */
   dmxfp = (mxmaxfp - mxminfp) / zdots;
   dmyfp = (mymaxfp - myminfp) / zdots;
   floatparm = &jbcfp;
   x_per_inchfp = (xxmin - xxmax) / widthfp;
   y_per_inchfp = (yymax - yymin) / heightfp;
   inch_per_xdotfp = widthfp / xdots;
   inch_per_ydotfp = heightfp / ydots;
   initzfp = originfp - (depthfp / 2);
   if(juli3Dmode == 0)
      RightEyefp.x = 0.0;
   else
      RightEyefp.x = eyesfp / 2;
   LeftEyefp.x = -RightEyefp.x;
   LeftEyefp.y = RightEyefp.y = 0;
   LeftEyefp.zx = RightEyefp.zx = distfp;
   LeftEyefp.zy = RightEyefp.zy = distfp;
   bbase = 128;

   if (juli3Dmode == 3)
   {
      savedac = 0;
      mapname = Glasses1Map;
   }
   else
      mapname = GreyFile;
   if(savedac != 1)
   {
   if (ValidateLuts(mapname) != 0)
      return (0);
   spindac(0, 1);               /* load it, but don't spin */
      if(savedac == 2)
        savedac = 1;
   }
   return (r >= 0);
}

int
jbfp_per_pixel(void)
{
   jxfp = ((Perfp->x - xpixelfp) * initzfp / distfp - xpixelfp) * x_per_inchfp;
   jxfp += xoffsetfp;
   djxfp = (depthfp / distfp) * (Perfp->x - xpixelfp) * x_per_inchfp / zdots;

   jyfp = ((Perfp->y - ypixelfp) * initzfp / distfp - ypixelfp) * y_per_inchfp;
   jyfp += yoffsetfp;
   djyfp = depthfp / distfp * (Perfp->y - ypixelfp) * y_per_inchfp / zdots;

   return (1);
}

static int zpixel, plotted;
static long n;

int
zlinefp(double x, double y)
{
#ifdef XFRACT
   static int keychk = 0;
#endif
   xpixelfp = x;
   ypixelfp = y;
   mxfp = mxminfp;
   myfp = myminfp;
   switch(juli3Dmode)
   {
   case 0:
   case 1:
      Perfp = &LeftEyefp;
      break;
   case 2:
      Perfp = &RightEyefp;
      break;
   case 3:
      if ((row + col) & 1)
         Perfp = &LeftEyefp;
      else
         Perfp = &RightEyefp;
      break;
   }
   jbfp_per_pixel();
   for (zpixel = 0; zpixel < zdots; zpixel++)
   {
      /* Special initialization for Mandelbrot types */
      if ((neworbittype == QUATFP || neworbittype == HYPERCMPLXFP)
          && save_release > 2002)
      {
         old.x = 0.0;
         old.y = 0.0;
         jbcfp.x = 0.0;
         jbcfp.y = 0.0;
         qc = jxfp;
         qci = jyfp;
         qcj = mxfp;
         qck = myfp;
      }
      else
      {
         old.x = jxfp;
         old.y = jyfp;
         jbcfp.x = mxfp;
         jbcfp.y = myfp;
         qc = param[0];
         qci = param[1];
         qcj = param[2];
         qck = param[3];
      }
#ifdef XFRACT
      if (keychk++ > 500)
      {
         keychk = 0;
         if (keypressed())
            return (-1);
      }
#else
      if (keypressed())
         return (-1);
#endif
      tempsqrx = sqr(old.x);
      tempsqry = sqr(old.y);

      for (n = 0; n < maxit; n++)
         if (fractalspecific[neworbittype].orbitcalc())
            break;
      if (n == maxit)
      {
         if (juli3Dmode == 3)
         {
            color = (int) (128l * zpixel / zdots);
            if ((row + col) & 1)
               (*plot) (col, row, 127 - color);
            else
            {
               color = (int)(color * brratiofp);
               if (color < 1)
                  color = 1;
               if (color > 127)
                  color = 127;
               (*plot) (col, row, 127 + bbase - color);
            }
         }
         else
         {
            color = (int) (254l * zpixel / zdots);
            (*plot) (col, row, color + 1);
         }
         plotted = 1;
         break;
      }
      mxfp += dmxfp;
      myfp += dmyfp;
      jxfp += djxfp;
      jyfp += djyfp;
   }
   return (0);
}

int
Std4dfpFractal(void)
{
   double x, y;
   int xdot, ydot;
   c_exp = (int)param[2];

   if(neworbittype == FPJULIAZPOWER)
   {
      if(param[3] == 0.0 && debugflag != 6000 && (double)c_exp == param[2])
          fractalspecific[neworbittype].orbitcalc = floatZpowerFractal;
      else
          fractalspecific[neworbittype].orbitcalc = floatCmplxZpowerFractal;
      get_julia_attractor (param[0], param[1]); /* another attractor? */
   }

   for (y = 0, ydot = (ydots >> 1) - 1; ydot >= 0; ydot--, y -= inch_per_ydotfp)
   {
      plotted = 0;
      x = -widthfp / 2;
      for (xdot = 0; xdot < xdots; xdot++, x += inch_per_xdotfp)
      {
         col = xdot;
         row = ydot;
         if (zlinefp(x, y) < 0)
            return (-1);
         col = xdots - col - 1;
         row = ydots - row - 1;
         if (zlinefp(-x, -y) < 0)
            return (-1);
      }
      if (plotted == 0)
      {
         if (y == 0)
           plotted = -1;  /* no points first pass; don't give up */
         else
           break;
      }
   }
   return (0);
}
