/************************************************************************/
/* This file contains a 3D replacement for the out_line function called */
/* by the decoder. The purpose is to apply various 3D transformations   */
/* before displaying points. Called once per line of the input file.    */
/*                                                                      */
/* Original Author Tim Wegner, with extensive help from Marc Reinig.    */
/*    July 1994 - TW broke out several pieces of code and added pragma  */
/*                to eliminate compiler warnings. Did a long-overdue    */
/*                formatting cleanup.                                   */
/************************************************************************/

#include <limits.h>

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "drivers.h"

struct point
{
   int x;
   int y;
   int color;
};

struct f_point
{
   float x;
   float y;
   float color;
};

struct minmax
{
   int minx;
   int maxx;
};

/* routines in this module */
int line3d(BYTE *, unsigned);
int _fastcall targa_color(int, int, int);
int targa_validate(char *);
static int first_time(int, VECTOR);
static int H_R(BYTE *, BYTE *, BYTE *, unsigned long, unsigned long, unsigned long);
static int line3dmem(void);
static int R_H(BYTE, BYTE, BYTE, unsigned long *, unsigned long *, unsigned long *);
static int set_pixel_buff(BYTE *, BYTE *, unsigned);
int startdisk1(char *, FILE *, int);
static void set_upr_lwr(void);
static int _fastcall end_object(int);
static int _fastcall offscreen(struct point);
static int _fastcall out_triangle(struct f_point, struct f_point, struct f_point, int, int, int);
static int _fastcall RAY_Header(void);
static int _fastcall start_object(void);
static void corners(MATRIX, int, double *, double *, double *, double *, double *, double *);
static void draw_light_box(double *, double *, MATRIX);
static void draw_rect(VECTOR, VECTOR, VECTOR, VECTOR, int, int);
static void File_Error(char *, int);
static void line3d_cleanup(void);
static void _fastcall clipcolor(int, int, int);
static void _fastcall interpcolor(int, int, int);
static void _fastcall putatriangle(struct point, struct point, struct point, int);
static void _fastcall putminmax(int, int, int);
static void _fastcall triangle_bounds(float pt_t[3][3]);
static void _fastcall T_clipcolor(int, int, int);
static void _fastcall vdraw_line(double *, double *, int color);
static void (_fastcall * fillplot) (int, int, int);
static void (_fastcall * normalplot) (int, int, int);

/* static variables */
static float deltaphi;          /* increment of latitude, longitude */
static double rscale;           /* surface roughness factor */
static long xcenter, ycenter;   /* circle center */
static double sclx, scly, sclz; /* scale factors */
static double R;                /* radius values */
static double Rfactor;          /* for intermediate calculation */
static LMATRIX llm;             /* "" */
static LVECTOR lview;           /* for perspective views */
static double zcutoff;          /* perspective backside cutoff value */
static float twocosdeltaphi;
static float cosphi, sinphi;    /* precalculated sin/cos of longitude */
static float oldcosphi1, oldsinphi1;
static float oldcosphi2, oldsinphi2;
static BYTE *fraction;      /* float version of pixels array */
static float min_xyz[3], max_xyz[3];        /* For Raytrace output */
static int line_length1;
static int T_header_24 = 18;/* Size of current Targa-24 header */
static FILE *File_Ptr1 = NULL;
static unsigned int IAmbient;
static int rand_factor;
static int HAZE_MULT;
static void File_Error(char *File_Name1, int ERROR);
static BYTE T24 = 24;
static BYTE T32 = 32;
static BYTE upr_lwr[4];
static int T_Safe; /* Original Targa Image successfully copied to targa_temp */
static VECTOR light_direction;
static BYTE Real_Color;  /* Actual color of cur pixel */
static int RO, CO, CO_MAX;  /* For use in Acrospin support */
static char acro_s1[] =
   {"Set Layer 1\nSet Color 2\nEndpointList X Y Z Name\n"};
static char acro_s2[] = {"LineList From To\n"};
static char s3[] = {"{ Created by FRACTINT Ver. "};
static char s3a[] = {" }\n\n"};
#ifndef XFRACT
static char banner[] = "%Fs%#4.2f%Fs";
#else
static char banner[] = "%s%#4.2f%s";
#endif
static int localpreviewfactor;
static int zcoord = 256;
static double aspect;       /* aspect ratio */
static int evenoddrow;
static float *sinthetaarray;    /* all sine thetas go here  */
static float *costhetaarray;    /* all cosine thetas go here */
static double rXrscale;     /* precalculation factor */
static int persp;  /* flag for indicating perspective transformations */
static struct point p1, p2, p3;
static struct f_point f_bad;/* out of range value */
static struct point bad;    /* out of range value */
static long num_tris; /* number of triangles output to ray trace file */

/* global variables defined here */
struct f_point *f_lastrow;
void (_fastcall * standardplot) (int, int, int);
MATRIX m; /* transformation matrix */
int Ambient;
int RANDOMIZE;
int haze;
int Real_V = 0; /* mrr Actual value of V for fillytpe>4 monochrome images */
char light_name[FILE_MAX_PATH] = "fract001";
int Targa_Overlay, error;
char targa_temp[14] = "fractemp.tga";
int P = 250; /* Perspective dist used when viewing light vector */
BYTE back_color[3];
char ray_name[FILE_MAX_PATH] = "fract001";
char preview = 0;
char showbox = 0;
int previewfactor = 20;
int xadjust = 0;
int yadjust = 0;
int xxadjust;
int yyadjust;
int xshift;
int yshift;
int bad_value = -10000; /* set bad values to this */
int bad_check = -3000;  /* check values against this to determine if good */
struct point *lastrow; /* this array remembers the previous line */
int RAY = 0;        /* Flag to generate Ray trace compatible files in 3d */
int BRIEF = 0;      /* 1 = short ray trace files */

/* array of min and max x values used in triangle fill */
struct minmax *minmax_x;
VECTOR view;                /* position of observer for perspective */
VECTOR cross;
VECTOR tmpcross;

struct point oldlast = { 0, 0, 0 }; /* old pixels */


int line3d(BYTE * pixels, unsigned linelen)
{
   int tout;                    /* triangle has been sent to ray trace file */
   int RND;
   float f_water = (float)0.0;        /* transformed WATERLINE for ray trace files */
   double r0;
   int xcenter0 = 0;
   int ycenter0 = 0;      /* Unfudged versions */
   double r;                    /* sphere radius */
   float costheta, sintheta;    /* precalculated sin/cos of latitude */
   int next;                    /* used by preview and grid */
   int col;                     /* current column (original GIF) */
   struct point cur;            /* current pixels */
   struct point old;            /* old pixels */
   struct f_point f_cur;
   struct f_point f_old;
   VECTOR v;                    /* double vector */
   VECTOR v1, v2;
   VECTOR crossavg;
   char crossnotinit;           /* flag for crossavg init indication */
   LVECTOR lv;                  /* long equivalent of v */
   LVECTOR lv0;                 /* long equivalent of v */
   int lastdot;
   long fudge;

   fudge = 1L << 16;


   if (transparent[0] || transparent[1])
      plot = normalplot = T_clipcolor;  /* Use transparent plot function */
   else                         /* Use the usual FRACTINT plot function with
                                 * clipping */
      plot = normalplot = clipcolor;

   currow = rowcount;           /* use separate variable to allow for
                                 * pot16bit files */
   if (pot16bit)
      currow >>= 1;

   /************************************************************************/
   /* This IF clause is executed ONCE per image. All precalculations are   */
   /* done here, with out any special concern about speed. DANGER -        */
   /* communication with the rest of the program is generally via static   */
   /* or global variables.                                                 */
   /************************************************************************/
   if (rowcount++ == 0)
   {
      int err;
      if ((err = first_time(linelen, v)) != 0)
         return (err);
      if(xdots > OLDMAXPIXELS)
         return(-1);
      tout = 0;
      crossavg[0] = 0;
      crossavg[1] = 0;
      crossavg[2] = 0;
      xcenter0 = (int) (xcenter = xdots / 2 + xshift);
      ycenter0 = (int) (ycenter = ydots / 2 - yshift);
   }
   /* make sure these pixel coordinates are out of range */
   old = bad;
   f_old = f_bad;

   /* copies pixels buffer to float type fraction buffer for fill purposes */
   if (pot16bit)
   {
      if (set_pixel_buff(pixels, fraction, linelen))
         return (0);
   }
   else if (grayflag)           /* convert color numbers to grayscale values */
      for (col = 0; col < (int) linelen; col++)
      {
         int pal, colornum;
         colornum = pixels[col];
         /* effectively (30*R + 59*G + 11*B)/100 scaled 0 to 255 */
         pal = ((int) g_dacbox[colornum][0] * 77 +
                (int) g_dacbox[colornum][1] * 151 +
                (int) g_dacbox[colornum][2] * 28);
         pal >>= 6;
         pixels[col] = (BYTE) pal;
      }
   crossnotinit = 1;
   col = 0;

   CO = 0;

   /*************************************************************************/
   /* This section of code allows the operation of a preview mode when the  */
   /* preview flag is set. Enabled, it allows the drawing of only the first */
   /* line of the source image, then every 10th line, until and including   */
   /* the last line. For the undrawn lines, only necessary calculations are */
   /* made. As a bonus, in non-sphere mode a box is drawn to help visualize */
   /* the effects of 3D transformations. Thanks to Marc Reinig for this idea*/
   /* and code -- BTW, Marc did NOT put the goto in, but WE did, to avoid   */
   /* copying code here, and to avoid a HUGE "if-then" construct. Besides,  */
   /* we have ALREADY sinned, so why not sin some more?                     */
   /*************************************************************************/
   lastdot = min(xdots - 1, (int) linelen - 1);
   if (FILLTYPE >= 5)
      if (haze && Targa_Out)
      {
         HAZE_MULT = (int) (haze * (
                                      (float) ((long) (ydots - 1 - currow) *
                                               (long) (ydots - 1 - currow)) /
                        (float) ((long) (ydots - 1) * (long) (ydots - 1))));
         HAZE_MULT = 100 - HAZE_MULT;
      }

   if (previewfactor >= ydots || previewfactor > lastdot)
      previewfactor = min(ydots - 1, lastdot);

   localpreviewfactor = ydots / previewfactor;

   tout = 0;
   /* Insure last line is drawn in preview and filltypes <0  */
   if ((RAY || preview || FILLTYPE < 0) && (currow != ydots - 1) &&
       (currow % localpreviewfactor) && /* Draw mod preview lines */
       !(!RAY && (FILLTYPE > 4) && (currow == 1)))
      /* Get init geometry in lightsource modes */
      goto reallythebottom;     /* skip over most of the line3d calcs */
   if (driver_diskp())
   {
      static char mapping[] = {"mapping to 3d, reading line "};
      char s[40];
#ifndef XFRACT
      sprintf(s, "%Fs%d", (char *)mapping, currow);
#else
      sprintf(s, "%s%d", mapping, currow);
#endif
      dvid_status(1, s);
   }

   if (!col && RAY && currow != 0)
      start_object();
      /* PROCESS ROW LOOP BEGINS HERE */
      while (col < (int) linelen)
      {
         if ((RAY || preview || FILLTYPE < 0) &&
             (col != lastdot) &&/* if this is not the last col */
         /* if not the 1st or mod factor col */
             (col % (int) (aspect * localpreviewfactor)) &&
             (!(!RAY && FILLTYPE > 4 && col == 1)))
            goto loopbottom;

         f_cur.color = (float) (cur.color = Real_Color = pixels[col]);

         if (RAY || preview || FILLTYPE < 0)
         {
            next = (int) (col + aspect * localpreviewfactor);
            if (next == col)
               next = col + 1;
         }
         else
            next = col + 1;
         if (next >= lastdot)
            next = lastdot;

         if (cur.color > 0 && cur.color < WATERLINE)
            f_cur.color = (float) (cur.color = Real_Color = (BYTE)WATERLINE); /* "lake" */
         else if (pot16bit)
            f_cur.color += ((float) fraction[col]) / (float) (1 << 8);

         if (SPHERE)            /* sphere case */
         {
            sintheta = sinthetaarray[col];
            costheta = costhetaarray[col];

            if (sinphi < 0 && !(RAY || FILLTYPE < 0))
            {
               cur = bad;
               f_cur = f_bad;
               goto loopbottom; /* another goto ! */
            }
            /************************************************************/
            /* KEEP THIS FOR DOCS - original formula --                 */
            /* if(rscale < 0.0)                                         */
            /* r = 1.0+((double)cur.color/(double)zcoord)*rscale;       */
            /* else                                                     */
            /* r = 1.0-rscale+((double)cur.color/(double)zcoord)*rscale;*/
            /* R = (double)ydots/2;                                     */
            /* r = r*R;                                                 */
            /* cur.x = xdots/2 + sclx*r*sintheta*aspect + xup ;         */
            /* cur.y = ydots/2 + scly*r*costheta*cosphi - yup ;         */
            /************************************************************/

            if (rscale < 0.0)
               r = R + Rfactor * (double) f_cur.color * costheta;
            else if (rscale > 0.0)
               r = R - rXrscale + Rfactor * (double) f_cur.color * costheta;
            else
               r = R;
            /* Allow Ray trace to go through so display ok */
            if (persp || RAY)
            {  /* mrr how do lv[] and cur and f_cur all relate */
               /* NOTE: fudge was pre-calculated above in r and R */
               /* (almost) guarantee negative */
               lv[2] = (long) (-R - r * costheta * sinphi);     /* z */
               if ((lv[2] > zcutoff) && !FILLTYPE < 0)
               {
                  cur = bad;
                  f_cur = f_bad;
                  goto loopbottom;      /* another goto ! */
               }
               lv[0] = (long) (xcenter + sintheta * sclx * r);  /* x */
               lv[1] = (long) (ycenter + costheta * cosphi * scly * r); /* y */

               if ((FILLTYPE >= 5) || RAY)
               {     /* calculate illumination normal before persp */

                  r0 = r / 65536L;
                  f_cur.x = (float) (xcenter0 + sintheta * sclx * r0);
                  f_cur.y = (float) (ycenter0 + costheta * cosphi * scly * r0);
                  f_cur.color = (float) (-r0 * costheta * sinphi);
               }
               if (!(usr_floatflag || RAY))
               {
                  if (longpersp(lv, lview, 16) == -1)
                  {
                     cur = bad;
                     f_cur = f_bad;
                     goto loopbottom;   /* another goto ! */
                  }
                  cur.x = (int) (((lv[0] + 32768L) >> 16) + xxadjust);
                  cur.y = (int) (((lv[1] + 32768L) >> 16) + yyadjust);
               }
               if (usr_floatflag || overflow || RAY)
               {
                  v[0] = lv[0];
                  v[1] = lv[1];
                  v[2] = lv[2];
                  v[0] /= fudge;
                  v[1] /= fudge;
                  v[2] /= fudge;
                  perspective(v);
                  cur.x = (int) (v[0] + .5 + xxadjust);
                  cur.y = (int) (v[1] + .5 + yyadjust);
               }
            }
            /* mrr Not sure how this an 3rd if above relate */
            else if (!(persp && RAY))
            {
               /* mrr Why the xx- and yyadjust here and not above? */
               cur.x = (int) (f_cur.x = (float) (xcenter
                                 + sintheta * sclx * r + xxadjust));
               cur.y = (int) (f_cur.y = (float) (ycenter
                                 + costheta * cosphi * scly * r + yyadjust));
               if (FILLTYPE >= 5 || RAY)        /* mrr why do we do this for
                                                 * filltype>5? */
                  f_cur.color = (float) (-r * costheta * sinphi * sclz);
               v[0] = v[1] = v[2] = 0;  /* MRR Why do we do this? */
            }
         }
         else
            /* non-sphere 3D */
         {
            if (!usr_floatflag && !RAY)
            {
               if (FILLTYPE >= 5)       /* flag to save vector before
                                         * perspective */
                  lv0[0] = 1;   /* in longvmultpersp calculation */
               else
                  lv0[0] = 0;

               /* use 32-bit multiply math to snap this out */
               lv[0] = col;
               lv[0] = lv[0] << 16;
               lv[1] = currow;
               lv[1] = lv[1] << 16;
               if (filetype || pot16bit)        /* don't truncate fractional
                                                 * part */
                  lv[2] = (long) (f_cur.color * 65536.0);
               else
                  /* there IS no fractaional part here! */
               {
                  lv[2] = (long) f_cur.color;
                  lv[2] = lv[2] << 16;
               }

               if (longvmultpersp(lv, llm, lv0, lv, lview, 16) == -1)
               {
                  cur = bad;
                  f_cur = f_bad;
                  goto loopbottom;
               }

               cur.x = (int) (((lv[0] + 32768L) >> 16) + xxadjust);
               cur.y = (int) (((lv[1] + 32768L) >> 16) + yyadjust);
               if (FILLTYPE >= 5 && !overflow)
               {
                  f_cur.x = (float) lv0[0];
                  f_cur.x /= (float)65536.0;
                  f_cur.y = (float) lv0[1];
                  f_cur.y /= (float)65536.0;
                  f_cur.color = (float) lv0[2];
                  f_cur.color /= (float)65536.0;
               }
            }

            if (usr_floatflag || overflow || RAY)
               /* do in float if integer math overflowed or doing Ray trace */
            {
               /* slow float version for comparison */
               v[0] = col;
               v[1] = currow;
               v[2] = f_cur.color;      /* Actually the z value */

               mult_vec(v);     /* matrix*vector routine */

               if (FILLTYPE > 4 || RAY)
               {
                  f_cur.x = (float) v[0];
                  f_cur.y = (float) v[1];
                  f_cur.color = (float) v[2];

                  if (RAY == 6)
                  {
                     f_cur.x = f_cur.x * ((float)2.0 / xdots) - (float)1.0;
                     f_cur.y = f_cur.y * ((float)2.0 / ydots) - (float)1.0;
                     f_cur.color = -f_cur.color * ((float)2.0 / numcolors) - (float)1.0;
                  }
               }

               if (persp && !RAY)
                  perspective(v);
               cur.x = (int) (v[0] + xxadjust + .5);
               cur.y = (int) (v[1] + yyadjust + .5);

               v[0] = 0;
               v[1] = 0;
               v[2] = WATERLINE;
               mult_vec(v);
               f_water = (float) v[2];
            }
         }

         if (RANDOMIZE)
            if (cur.color > WATERLINE)
            {
               RND = rand15() >> 8;     /* 7-bit number */
               RND = RND * RND >> rand_factor;  /* n-bit number */

               if (rand() & 1)
                  RND = -RND;   /* Make +/- n-bit number */

               if ((int) (cur.color) + RND >= colors)
                  cur.color = colors - 2;
               else if ((int) (cur.color) + RND <= WATERLINE)
                  cur.color = WATERLINE + 1;
               else
                  cur.color = cur.color + RND;
               Real_Color = (BYTE)cur.color;
            }

         if (RAY)
         {
            if (col && currow &&
                old.x > bad_check &&
                old.x < (xdots - bad_check) &&
                lastrow[col].x > bad_check &&
                lastrow[col].y > bad_check &&
                lastrow[col].x < (xdots - bad_check) &&
                lastrow[col].y < (ydots - bad_check))
            {
               /* Get rid of all the triangles in the plane at the base of
                * the object */

               if (f_cur.color == f_water &&
                   f_lastrow[col].color == f_water &&
                   f_lastrow[next].color == f_water)
                  goto loopbottom;

               if (RAY != 6)    /* Output the vertex info */
                  out_triangle(f_cur, f_old, f_lastrow[col],
                               cur.color, old.color, lastrow[col].color);

               tout = 1;

               driver_draw_line(old.x, old.y, cur.x, cur.y, old.color);
               driver_draw_line(old.x, old.y, lastrow[col].x,
                         lastrow[col].y, old.color);
               driver_draw_line(lastrow[col].x, lastrow[col].y,
                         cur.x, cur.y, cur.color);
               num_tris++;
            }

            if (col < lastdot && currow &&
                lastrow[col].x > bad_check &&
                lastrow[col].y > bad_check &&
                lastrow[col].x < (xdots - bad_check) &&
                lastrow[col].y < (ydots - bad_check) &&
                lastrow[next].x > bad_check &&
                lastrow[next].y > bad_check &&
                lastrow[next].x < (xdots - bad_check) &&
                lastrow[next].y < (ydots - bad_check))
            {
               /* Get rid of all the triangles in the plane at the base of
                * the object */

               if (f_cur.color == f_water &&
                   f_lastrow[col].color == f_water &&
                   f_lastrow[next].color == f_water)
                  goto loopbottom;

               if (RAY != 6)    /* Output the vertex info */
                  out_triangle(f_cur, f_lastrow[col], f_lastrow[next],
                        cur.color, lastrow[col].color, lastrow[next].color);

               tout = 1;

               driver_draw_line(lastrow[col].x, lastrow[col].y, cur.x, cur.y,
                         cur.color);
               driver_draw_line(lastrow[next].x, lastrow[next].y, cur.x, cur.y,
                         cur.color);
               driver_draw_line(lastrow[next].x, lastrow[next].y, lastrow[col].x,
                         lastrow[col].y, lastrow[col].color);
               num_tris++;
            }

            if (RAY == 6)       /* Output vertex info for Acrospin */
            {
               fprintf(File_Ptr1, "% #4.4f % #4.4f % #4.4f R%dC%d\n",
                       f_cur.x, f_cur.y, f_cur.color, RO, CO);
               if (CO > CO_MAX)
                  CO_MAX = CO;
               CO++;
            }
            goto loopbottom;
         }

         switch (FILLTYPE)
         {
         case -1:
            if (col &&
                old.x > bad_check &&
                old.x < (xdots - bad_check))
               driver_draw_line(old.x, old.y, cur.x, cur.y, cur.color);
            if (currow &&
                lastrow[col].x > bad_check &&
                lastrow[col].y > bad_check &&
                lastrow[col].x < (xdots - bad_check) &&
                lastrow[col].y < (ydots - bad_check))
               driver_draw_line(lastrow[col].x, lastrow[col].y, cur.x,
                         cur.y, cur.color);
            break;

         case 0:
            (*plot) (cur.x, cur.y, cur.color);
            break;

         case 1:                /* connect-a-dot */
            if ((old.x < xdots) && (col) &&
                old.x > bad_check &&
                old.y > bad_check)      /* Don't draw from old to cur on col
                                         * 0 */
               driver_draw_line(old.x, old.y, cur.x, cur.y, cur.color);
            break;

         case 2:                /* with interpolation */
         case 3:                /* no interpolation */
            /*************************************************************/
            /* "triangle fill" - consider four points: current point,    */
            /* previous point same row, point opposite current point in  */
            /* previous row, point after current point in previous row.  */
            /* The object is to fill all points inside the two triangles.*/
            /*                                                           */
            /* lastrow[col].x/y___ lastrow[next]                         */
            /* /        1                 /                              */
            /* /                1         /                              */
            /* /                       1  /                              */
            /* oldrow/col ________ trow/col                              */
            /*************************************************************/

            if (currow && !col)
               putatriangle(lastrow[next], lastrow[col], cur, cur.color);
            if (currow && col)  /* skip first row and first column */
            {
               if (col == 1)
                  putatriangle(lastrow[col], oldlast, old, old.color);

               if (col < lastdot)
                  putatriangle(lastrow[next], lastrow[col], cur, cur.color);
               putatriangle(old, lastrow[col], cur, cur.color);
            }
            break;

         case 4:                /* "solid fill" */
            if (SPHERE)
            {
               if (persp)
               {
                  old.x = (int) (xcenter >> 16);
                  old.y = (int) (ycenter >> 16);
               }
               else
               {
                  old.x = (int) xcenter;
                  old.y = (int) ycenter;
               }
            }
            else
            {
               lv[0] = col;
               lv[1] = currow;
               lv[2] = 0;

               /* apply fudge bit shift for integer math */
               lv[0] = lv[0] << 16;
               lv[1] = lv[1] << 16;
               /* Since 0, unnecessary lv[2] = lv[2] << 16; */

               if (longvmultpersp(lv, llm, lv0, lv, lview, 16))
               {
                  cur = bad;
                  f_cur = f_bad;
                  goto loopbottom;      /* another goto ! */
               }

               /* Round and fudge back to original  */
               old.x = (int) ((lv[0] + 32768L) >> 16);
               old.y = (int) ((lv[1] + 32768L) >> 16);
            }
            if (old.x < 0)
               old.x = 0;
            if (old.x >= xdots)
               old.x = xdots - 1;
            if (old.y < 0)
               old.y = 0;
            if (old.y >= ydots)
               old.y = ydots - 1;
            driver_draw_line(old.x, old.y, cur.x, cur.y, cur.color);
            break;

         case 5:
         case 6:
            /* light-source modulated fill */
            if (currow && col)  /* skip first row and first column */
            {
               if (f_cur.color < bad_check || f_old.color < bad_check ||
                   f_lastrow[col].color < bad_check)
                  break;

               v1[0] = f_cur.x - f_old.x;
               v1[1] = f_cur.y - f_old.y;
               v1[2] = f_cur.color - f_old.color;

               v2[0] = f_lastrow[col].x - f_cur.x;
               v2[1] = f_lastrow[col].y - f_cur.y;
               v2[2] = f_lastrow[col].color - f_cur.color;

               cross_product(v1, v2, cross);

               /* normalize cross - and check if non-zero */
               if (normalize_vector(cross))
               {
                  if (debugflag)
                  {
                     static char msg[] = {"debug, cur.color=bad"};
                     stopmsg(0, msg);
                  }
                  cur.color = (int)(f_cur.color = (float) bad.color);
               }
               else
               {
                  /* line-wise averaging scheme */
                  if (LIGHTAVG > 0)
                  {
                     if (crossnotinit)
                     {
                        /* initialize array of old normal vectors */
                        crossavg[0] = cross[0];
                        crossavg[1] = cross[1];
                        crossavg[2] = cross[2];
                        crossnotinit = 0;
                     }
                     tmpcross[0] = (crossavg[0] * LIGHTAVG + cross[0]) /
                        (LIGHTAVG + 1);
                     tmpcross[1] = (crossavg[1] * LIGHTAVG + cross[1]) /
                        (LIGHTAVG + 1);
                     tmpcross[2] = (crossavg[2] * LIGHTAVG + cross[2]) /
                        (LIGHTAVG + 1);
                     cross[0] = tmpcross[0];
                     cross[1] = tmpcross[1];
                     cross[2] = tmpcross[2];
                     if (normalize_vector(cross))
                     {
                        /* this shouldn't happen */
                        if (debugflag)
                        {
                           static char msg[] = {"debug, normal vector err2"};
                           stopmsg(0, msg);
                           /* use next instead if you ever need details:
                            * static char tmp[] = {"debug, vector err"};
                            * char msg[200]; #ifndef XFRACT
                            * sprintf(msg,"%Fs\n%f %f %f\n%f %f %f\n%f %f
                            * %f", #else sprintf(msg,"%s\n%f %f %f\n%f %f
                            * %f\n%f %f %f", #endif tmp, f_cur.x, f_cur.y,
                            * f_cur.color, f_lastrow[col].x,
                            * f_lastrow[col].y, f_lastrow[col].color,
                            * f_lastrow[col-1].x,
                            * f_lastrow[col-1].y,f_lastrow[col-1].color);
                            * stopmsg(0,msg); */
                        }
                        cur.color = (int)(f_cur.color = (float) colors);
                     }
                  }
                  crossavg[0] = tmpcross[0];
                  crossavg[1] = tmpcross[1];
                  crossavg[2] = tmpcross[2];

                  /* dot product of unit vectors is cos of angle between */
                  /* we will use this value to shade surface */

                  cur.color = (int) (1 + (colors - 2) *
                               (1.0 - dot_product(cross, light_direction)));
               }
               /* if colors out of range, set them to min or max color index
                * but avoid background index. This makes colors "opaque" so
                * SOMETHING plots. These conditions shouldn't happen but just
                * in case                                        */
               if (cur.color < 1)       /* prevent transparent colors */
                  cur.color = 1;/* avoid background */
               if (cur.color > colors - 1)
                  cur.color = colors - 1;

               /* why "col < 2"? So we have sufficient geometry for the fill */
               /* algorithm, which needs previous point in same row to have  */
               /* already been calculated (variable old)                 */
               /* fix ragged left margin in preview */
               if (col == 1 && currow > 1)
                  putatriangle(lastrow[next], lastrow[col], cur, cur.color);

               if (col < 2 || currow < 2)       /* don't have valid colors
                                                 * yet */
                  break;

               if (col < lastdot)
                  putatriangle(lastrow[next], lastrow[col], cur, cur.color);
               putatriangle(old, lastrow[col], cur, cur.color);

               plot = standardplot;
            }
            break;
         }                      /* End of CASE statement for fill type  */
       loopbottom:
         if (RAY || (FILLTYPE != 0 && FILLTYPE != 4))
         {
            /* for triangle and grid fill purposes */
            oldlast = lastrow[col];
            old = lastrow[col] = cur;

            /* for illumination model purposes */
            f_old = f_lastrow[col] = f_cur;
            if (currow && RAY && col >= lastdot)
               /* if we're at the end of a row, close the object */
            {
               end_object(tout);
               tout = 0;
               if (ferror(File_Ptr1))
               {
                  fclose(File_Ptr1);
                  remove(light_name);
                  File_Error(ray_name, 2);
                  return (-1);
               }
            }
         }
         col++;
      }                         /* End of while statement for plotting line  */
   RO++;
 reallythebottom:

   /* stuff that HAS to be done, even in preview mode, goes here */
   if (SPHERE)
   {
      /* incremental sin/cos phi calc */
      if (currow == 0)
      {
         sinphi = oldsinphi2;
         cosphi = oldcosphi2;
      }
      else
      {
         sinphi = twocosdeltaphi * oldsinphi2 - oldsinphi1;
         cosphi = twocosdeltaphi * oldcosphi2 - oldcosphi1;
         oldsinphi1 = oldsinphi2;
         oldsinphi2 = sinphi;
         oldcosphi1 = oldcosphi2;
         oldcosphi2 = cosphi;
      }
   }
   return (0);                  /* decoder needs to know all is well !!! */
}

/* vector version of line draw */
static void _fastcall vdraw_line(double *v1, double *v2, int color)
{
   int x1, y1, x2, y2;
   x1 = (int) v1[0];
   y1 = (int) v1[1];
   x2 = (int) v2[0];
   y2 = (int) v2[1];
   driver_draw_line(x1, y1, x2, y2, color);
}

static void corners(MATRIX m, int show, double *pxmin, double *pymin, double *pzmin, double *pxmax, double *pymax, double *pzmax)
{
   int i, j;
   VECTOR S[2][4];              /* Holds the top an bottom points,
                                 * S[0][]=bottom */

   /* define corners of box fractal is in in x,y,z plane "b" stands for
    * "bottom" - these points are the corners of the screen in the x-y plane.
    * The "t"'s stand for Top - they are the top of the cube where 255 color
    * points hit. */

   *pxmin = *pymin = *pzmin = (int) INT_MAX;
   *pxmax = *pymax = *pzmax = (int) INT_MIN;

   for (j = 0; j < 4; ++j)
      for (i = 0; i < 3; i++)
         S[0][j][i] = S[1][j][i] = 0;

   S[0][1][0] = S[0][2][0] = S[1][1][0] = S[1][2][0] = xdots - 1;
   S[0][2][1] = S[0][3][1] = S[1][2][1] = S[1][3][1] = ydots - 1;
   S[1][0][2] = S[1][1][2] = S[1][2][2] = S[1][3][2] = zcoord - 1;

   for (i = 0; i < 4; ++i)
   {
      /* transform points */
      vmult(S[0][i], m, S[0][i]);
      vmult(S[1][i], m, S[1][i]);

      /* update minimums and maximums */
      if (S[0][i][0] <= *pxmin)
         *pxmin = S[0][i][0];
      if (S[0][i][0] >= *pxmax)
         *pxmax = S[0][i][0];
      if (S[1][i][0] <= *pxmin)
         *pxmin = S[1][i][0];
      if (S[1][i][0] >= *pxmax)
         *pxmax = S[1][i][0];
      if (S[0][i][1] <= *pymin)
         *pymin = S[0][i][1];
      if (S[0][i][1] >= *pymax)
         *pymax = S[0][i][1];
      if (S[1][i][1] <= *pymin)
         *pymin = S[1][i][1];
      if (S[1][i][1] >= *pymax)
         *pymax = S[1][i][1];
      if (S[0][i][2] <= *pzmin)
         *pzmin = S[0][i][2];
      if (S[0][i][2] >= *pzmax)
         *pzmax = S[0][i][2];
      if (S[1][i][2] <= *pzmin)
         *pzmin = S[1][i][2];
      if (S[1][i][2] >= *pzmax)
         *pzmax = S[1][i][2];
   }

   if (show)
   {
      if (persp)
      {
         for (i = 0; i < 4; i++)
         {
            perspective(S[0][i]);
            perspective(S[1][i]);
         }
      }

      /* Keep the box surrounding the fractal */
      for (j = 0; j < 2; j++)
         for (i = 0; i < 4; ++i)
         {
            S[j][i][0] += xxadjust;
            S[j][i][1] += yyadjust;
         }

      draw_rect(S[0][0], S[0][1], S[0][2], S[0][3], 2, 1);      /* Bottom */

      draw_rect(S[0][0], S[1][0], S[0][1], S[1][1], 5, 0);      /* Sides */
      draw_rect(S[0][2], S[1][2], S[0][3], S[1][3], 6, 0);

      draw_rect(S[1][0], S[1][1], S[1][2], S[1][3], 8, 1);      /* Top */
   }
}

/* This function draws a vector from origin[] to direct[] and a box
        around it. The vector and box are transformed or not depending on
        FILLTYPE.
*/

static void draw_light_box(double *origin, double *direct, MATRIX light_m)
{
   VECTOR S[2][4];
   int i, j;
   double temp;

   S[1][0][0] = S[0][0][0] = origin[0];
   S[1][0][1] = S[0][0][1] = origin[1];

   S[1][0][2] = direct[2];

   for (i = 0; i < 2; i++)
   {
      S[i][1][0] = S[i][0][0];
      S[i][1][1] = direct[1];
      S[i][1][2] = S[i][0][2];
      S[i][2][0] = direct[0];
      S[i][2][1] = S[i][1][1];
      S[i][2][2] = S[i][0][2];
      S[i][3][0] = S[i][2][0];
      S[i][3][1] = S[i][0][1];
      S[i][3][2] = S[i][0][2];
   }

   /* transform the corners if necessary */
   if (FILLTYPE == 6)
      for (i = 0; i < 4; i++)
      {
         vmult(S[0][i], light_m, S[0][i]);
         vmult(S[1][i], light_m, S[1][i]);
      }

   /* always use perspective to aid viewing */
   temp = view[2];              /* save perspective distance for a later
                                 * restore */
   view[2] = -P * 300.0 / 100.0;

   for (i = 0; i < 4; i++)
   {
      perspective(S[0][i]);
      perspective(S[1][i]);
   }
   view[2] = temp;              /* Restore perspective distance */

   /* Adjust for aspect */
   for (i = 0; i < 4; i++)
   {
      S[0][i][0] = S[0][i][0] * aspect;
      S[1][i][0] = S[1][i][0] * aspect;
   }

   /* draw box connecting transformed points. NOTE order and COLORS */
   draw_rect(S[0][0], S[0][1], S[0][2], S[0][3], 2, 1);

   vdraw_line(S[0][0], S[1][2], 8);

   /* sides */
   draw_rect(S[0][0], S[1][0], S[0][1], S[1][1], 4, 0);
   draw_rect(S[0][2], S[1][2], S[0][3], S[1][3], 5, 0);

   draw_rect(S[1][0], S[1][1], S[1][2], S[1][3], 3, 1);

   /* Draw the "arrow head" */
   for (i = -3; i < 4; i++)
      for (j = -3; j < 4; j++)
         if (abs(i) + abs(j) < 6)
            plot((int) (S[1][2][0] + i), (int) (S[1][2][1] + j), 10);
}

static void draw_rect(VECTOR V0, VECTOR V1, VECTOR V2, VECTOR V3, int color, int rect)
{
   VECTOR V[4];
   int i;

   /* Since V[2] is not used by vdraw_line don't bother setting it */
   for (i = 0; i < 2; i++)
   {
      V[0][i] = V0[i];
      V[1][i] = V1[i];
      V[2][i] = V2[i];
      V[3][i] = V3[i];
   }
   if (rect)                    /* Draw a rectangle */
   {
      for (i = 0; i < 4; i++)
         if (fabs(V[i][0] - V[(i + 1) % 4][0]) < -2 * bad_check &&
             fabs(V[i][1] - V[(i + 1) % 4][1]) < -2 * bad_check)
            vdraw_line(V[i], V[(i + 1) % 4], color);
   }
   else
      /* Draw 2 lines instead */
   {
      for (i = 0; i < 3; i += 2)
         if (fabs(V[i][0] - V[i + 1][0]) < -2 * bad_check &&
             fabs(V[i][1] - V[i + 1][1]) < -2 * bad_check)
            vdraw_line(V[i], V[i + 1], color);
   }
   return;
}

/* replacement for plot - builds a table of min and max x's instead of plot */
/* called by draw_line as part of triangle fill routine */
static void _fastcall putminmax(int x, int y, int color)
{
   color = 0; /* to supress warning only */
   if (y >= 0 && y < ydots)
   {
      if (x < minmax_x[y].minx)
         minmax_x[y].minx = x;
      if (x > minmax_x[y].maxx)
         minmax_x[y].maxx = x;
   }
}

/*
        This routine fills in a triangle. Extreme left and right values for
        each row are calculated by calling the line function for the sides.
        Then rows are filled in with horizontal lines
*/
#define MAXOFFSCREEN  2    /* allow two of three points to be off screen */

static void _fastcall putatriangle(struct point pt1, struct point pt2, struct point pt3, int color)
{
   int miny, maxy;
   int x, y, xlim;

   /* Too many points off the screen? */
   if ((offscreen(pt1) + offscreen(pt2) + offscreen(pt3)) > MAXOFFSCREEN)
      return;

   p1 = pt1;                    /* needed by interpcolor */
   p2 = pt2;
   p3 = pt3;

   /* fast way if single point or single line */
   if (p1.y == p2.y && p1.x == p2.x)
   {
      plot = fillplot;
      if (p1.y == p3.y && p1.x == p3.x)
         (*plot) (p1.x, p1.y, color);
      else
         driver_draw_line(p1.x, p1.y, p3.x, p3.y, color);
      plot = normalplot;
      return;
   }
   else if ((p3.y == p1.y && p3.x == p1.x) || (p3.y == p2.y && p3.x == p2.x))
   {
      plot = fillplot;
      driver_draw_line(p1.x, p1.y, p2.x, p2.y, color);
      plot = normalplot;
      return;
   }

   /* find min max y */
   miny = maxy = p1.y;
   if (p2.y < miny)
      miny = p2.y;
   else
      maxy = p2.y;
   if (p3.y < miny)
      miny = p3.y;
   else if (p3.y > maxy)
      maxy = p3.y;

   /* only worried about values on screen */
   if (miny < 0)
      miny = 0;
   if (maxy >= ydots)
      maxy = ydots - 1;

   for (y = miny; y <= maxy; y++)
   {
      minmax_x[y].minx = (int) INT_MAX;
      minmax_x[y].maxx = (int) INT_MIN;
   }

   /* set plot to "fake" plot function */
   plot = putminmax;

   /* build table of extreme x's of triangle */
   driver_draw_line(p1.x, p1.y, p2.x, p2.y, 0);
   driver_draw_line(p2.x, p2.y, p3.x, p3.y, 0);
   driver_draw_line(p3.x, p3.y, p1.x, p1.y, 0);

   for (y = miny; y <= maxy; y++)
   {
      xlim = minmax_x[y].maxx;
      for (x = minmax_x[y].minx; x <= xlim; x++)
         (*fillplot) (x, y, color);
   }
   plot = normalplot;
}

static int _fastcall offscreen(struct point pt)
{
   if (pt.x >= 0)
      if (pt.x < xdots)
         if (pt.y >= 0)
            if (pt.y < ydots)
               return (0);      /* point is ok */
   if (abs(pt.x) > 0 - bad_check || abs(pt.y) > 0 - bad_check)
      return (99);              /* point is bad */
   return (1);                  /* point is off the screen */
}

static void _fastcall clipcolor(int x, int y, int color)
{
   if (0 <= x && x < xdots &&
       0 <= y && y < ydots &&
       0 <= color && color < filecolors)
   {
      standardplot(x, y, color);

      if (Targa_Out)
         /* standardplot modifies color in these types */
         if (!(glassestype == 1 || glassestype == 2))
            targa_color(x, y, color);
   }
}

/*********************************************************************/
/* This function is the same as clipcolor but checks for color being */
/* in transparent range. Intended to be called only if transparency  */
/* has been enabled.                                                 */
/*********************************************************************/

static void _fastcall T_clipcolor(int x, int y, int color)
{
   if (0 <= x && x < xdots &&   /* is the point on screen?  */
       0 <= y && y < ydots &&   /* Yes?  */
       0 <= color && color < colors &&  /* Colors in valid range?  */
   /* Lets make sure its not a transparent color  */
       (transparent[0] > color || color > transparent[1]))
   {
      standardplot(x, y, color);/* I guess we can plot then  */
      if (Targa_Out)
         /* standardplot modifies color in these types */
         if (!(glassestype == 1 || glassestype == 2))
            targa_color(x, y, color);
   }
}

/************************************************************************/
/* A substitute for plotcolor that interpolates the colors according    */
/* to the x and y values of three points (p1,p2,p3) which are static in */
/* this routine                                                         */
/*                                                                      */
/*      In Light source modes, color is light value, not actual color   */
/*      Real_Color always contains the actual color                     */
/************************************************************************/

static void _fastcall interpcolor(int x, int y, int color)
{
   int D, d1, d2, d3;

   /* this distance formula is not the usual one - but it has the virtue that
    * it uses ONLY additions (almost) and it DOES go to zero as the points
    * get close. */

   d1 = abs(p1.x - x) + abs(p1.y - y);
   d2 = abs(p2.x - x) + abs(p2.y - y);
   d3 = abs(p3.x - x) + abs(p3.y - y);

   D = (d1 + d2 + d3) << 1;
   if (D)
   {  /* calculate a weighted average of colors long casts prevent integer
         overflow. This can evaluate to zero */
      color = (int) (((long) (d2 + d3) * (long) p1.color +
                      (long) (d1 + d3) * (long) p2.color +
                      (long) (d1 + d2) * (long) p3.color) / D);
   }

   if (0 <= x && x < xdots &&
       0 <= y && y < ydots &&
       0 <= color && color < colors &&
       (transparent[1] == 0 || (int) Real_Color > transparent[1] ||
        transparent[0] > (int) Real_Color))
   {
      if (Targa_Out)
         /* standardplot modifies color in these types */
         if (!(glassestype == 1 || glassestype == 2))
            D = targa_color(x, y, color);

      if (FILLTYPE >= 5) {
         if (Real_V && Targa_Out)
            color = D;
         else
         {
            color = (1 + (unsigned) color * IAmbient) / 256;
            if (color == 0)
               color = 1;
         }
      }
      standardplot(x, y, color);
   }
}

/*
        In non light source modes, both color and Real_Color contain the
        actual pixel color. In light source modes, color contains the
        light value, and Real_Color contains the origninal color

        This routine takes a pixel modifies it for lightshading if appropriate
        and plots it in a Targa file. Used in plot3d.c
*/

int _fastcall targa_color(int x, int y, int color)
{
   unsigned long H, S, V;
   BYTE RGB[3];

   if (FILLTYPE == 2 || glassestype == 1 || glassestype == 2 || truecolor)
      Real_Color = (BYTE)color;       /* So Targa gets interpolated color */

   switch (truemode)
   {
      case 0:
      default:
      {
         RGB[0] = (BYTE)(g_dacbox[Real_Color][0] << 2); /* Move color space to */
         RGB[1] = (BYTE)(g_dacbox[Real_Color][1] << 2); /* 256 color primaries */
         RGB[2] = (BYTE)(g_dacbox[Real_Color][2] << 2); /* from 64 colors */
         break;
      }
      case 1:
      {
         RGB[0] = (BYTE)((realcoloriter >> 16) & 0xff);  /* red   */
         RGB[1] = (BYTE)((realcoloriter >> 8 ) & 0xff);  /* green */
         RGB[2] = (BYTE)((realcoloriter      ) & 0xff);  /* blue  */
         break;
      }
   }

   /* Now lets convert it to HSV */
   R_H(RGB[0], RGB[1], RGB[2], &H, &S, &V);

   /* Modify original S and V components */
   if (FILLTYPE > 4 && !(glassestype == 1 || glassestype == 2))
      /* Adjust for Ambient */
      V = (V * (65535L - (unsigned) (color * IAmbient))) / 65535L;

   if (haze)
   {
      /* Haze lowers sat of colors */
      S = (unsigned long) (S * HAZE_MULT) / 100;
      if (V >= 32640)           /* Haze reduces contrast */
      {
         V = V - 32640;
         V = (unsigned long) ((V * HAZE_MULT) / 100);
         V = V + 32640;
      }
      else
      {
         V = 32640 - V;
         V = (unsigned long) ((V * HAZE_MULT) / 100);
         V = 32640 - V;
      }
   }
   /* Now lets convert it back to RGB. Original Hue, modified Sat and Val */
   H_R(&RGB[0], &RGB[1], &RGB[2], H, S, V);

   if (Real_V)
      V = (35 * (int) RGB[0] + 45 * (int) RGB[1] + 20 * (int) RGB[2]) / 100;

   /* Now write the color triple to its transformed location */
   /* on the disk. */
   targa_writedisk(x + sxoffs, y + syoffs, RGB[0], RGB[1], RGB[2]);

   return ((int) (255 - V));
}

static int set_pixel_buff(BYTE * pixels, BYTE * fraction, unsigned linelen)
{
   int i;
   if ((evenoddrow++ & 1) == 0) /* even rows are color value */
   {
      for (i = 0; i < (int) linelen; i++)       /* add the fractional part in
                                                 * odd row */
         fraction[i] = pixels[i];
      return (1);
   }
   else
      /* swap */
   {
      BYTE tmp;
      for (i = 0; i < (int) linelen; i++)       /* swap so pixel has color */
      {
         tmp = pixels[i];
         pixels[i] = fraction[i];
         fraction[i] = tmp;
      }
   }
   return (0);
}

/**************************************************************************

  Common routine for printing error messages to the screen for Targa
    and other files

**************************************************************************/

#ifndef XFRACT
static char s_f[] = "%Fs%Fs";
static char s_fff[] = "%Fs%Fs%Fs";
#else
static char s_f[] = "%s%s";
static char s_fff[] = "%s%s%s";
#endif
static char OOPS[] = {"OOPS, "};
static char E1[] = {"can't handle this type of file.\n"};
static char str1[] = {"couldn't open  < "};
static char str3[] = {"image wrong size\n"};
static char outofdisk[] = {"ran out of disk space. < "};

static void File_Error(char *File_Name1, int ERROR)
{
   char msgbuf[200];

   error = ERROR;
   switch (ERROR)
   {
   case 1:                      /* Can't Open */
#ifndef XFRACT
      sprintf(msgbuf, "%Fs%Fs%s >", (char *)OOPS, (char *)str1, File_Name1);
#else
      sprintf(msgbuf, "%s%s%s >", OOPS, str1, File_Name1);
#endif
      break;
   case 2:                      /* Not enough room */
#ifndef XFRACT
      sprintf(msgbuf, "%Fs%Fs%s >", (char *)OOPS, (char *)outofdisk, File_Name1);
#else
      sprintf(msgbuf, "%s%s%s >", OOPS, outofdisk, File_Name1);
#endif
      break;
   case 3:                      /* Image wrong size */
      sprintf(msgbuf, s_f, (char *)OOPS, (char *)str3);
      break;
   case 4:                      /* Wrong file type */
      sprintf(msgbuf, s_f, (char *)OOPS, (char *)E1);
      break;
   }
   stopmsg(0, msgbuf);
   return;
}


/************************************************************************/
/*                                                                      */
/*   This function opens a TARGA_24 file for reading and writing. If    */
/*   its a new file, (overlay == 0) it writes a header. If it is to     */
/*   overlay an existing file (overlay == 1) it copies the original     */
/*   header whose lenght and validity was determined in                 */
/*   Targa_validate.                                                    */
/*                                                                      */
/*   It Verifies there is enough disk space, and leaves the file        */
/*   at the start of the display data area.                             */
/*                                                                      */
/*   If this is an overlay, closes source and copies to "targa_temp"    */
/*   If there is an error close the file.                               */
/*                                                                      */
/* **********************************************************************/

int startdisk1(char *File_Name2, FILE * Source, int overlay)
{
   int i, j, k, inc;
   FILE *fps;

   /* Open File for both reading and writing */
   if ((fps = dir_fopen(workdir,File_Name2, "w+b")) == NULL)
   {
      File_Error(File_Name2, 1);
      return (-1);              /* Oops, somethings wrong! */
   }

   inc = 1;                     /* Assume we are overlaying a file */

   /* Write the header */
   if (overlay)                 /* We are overlaying a file */
      for (i = 0; i < T_header_24; i++) /* Copy the header from the Source */
         fputc(fgetc(Source), fps);
   else
   {                            /* Write header for a new file */
      /* ID field size = 0, No color map, Targa type 2 file */
      for (i = 0; i < 12; i++)
      {
         if (i == 0 && truecolor != 0)
         {
            set_upr_lwr();
            fputc(4, fps); /* make room to write an extra number */
            T_header_24 = 18 + 4;
         }
         else if (i == 2)
            fputc(i, fps);
         else
            fputc(0, fps);
      }
      /* Write image size  */
      for (i = 0; i < 4; i++)
         fputc(upr_lwr[i], fps);
      fputc(T24, fps);          /* Targa 24 file */
      fputc(T32, fps);          /* Image at upper left */
      inc = 3;
   }

   if(truecolor) /* write maxit */
   {
      fputc((BYTE)(maxit       & 0xff), fps);
      fputc((BYTE)((maxit>>8 ) & 0xff), fps);
      fputc((BYTE)((maxit>>16) & 0xff), fps);
      fputc((BYTE)((maxit>>24) & 0xff), fps);
   }

   /* Finished with the header, now lets work on the display area  */
   for (i = 0; i < ydots; i++)  /* "clear the screen" (write to the disk) */
   {
      for (j = 0; j < line_length1; j = j + inc)
      {
         if (overlay)
            fputc(fgetc(Source), fps);
         else
            for (k = 2; k > -1; k--)
               fputc(back_color[k], fps);       /* Targa order (B, G, R) */
      }
      if (ferror(fps))
      {
         /* Almost certainly not enough disk space  */
         fclose(fps);
         if(overlay)
            fclose(Source);
         dir_remove(workdir,File_Name2);
         File_Error(File_Name2, 2);
         return (-2);
      }
      if (driver_key_pressed())
         return (-3);
   }

   if (targa_startdisk(fps, T_header_24) != 0)
   {
      enddisk();
      dir_remove(workdir,File_Name2);
      return (-4);
   }
   return (0);
}

int targa_validate(char *File_Name)
{
   FILE *fp;
   int i;
#if 0
   int j = 0;
#endif

   /* Attempt to open source file for reading */
   if ((fp = dir_fopen(workdir,File_Name, "rb")) == NULL)
   {
      File_Error(File_Name, 1);
      return (-1);              /* Oops, file does not exist */
   }

   T_header_24 += fgetc(fp);    /* Check ID field and adjust header size */

   if (fgetc(fp))               /* Make sure this is an unmapped file */
   {
      File_Error(File_Name, 4);
      return (-1);
   }

   if (fgetc(fp) != 2)          /* Make sure it is a type 2 file */
   {
      File_Error(File_Name, 4);
      return (-1);
   }

   /* Skip color map specification */
   for (i = 0; i < 5; i++)
      fgetc(fp);

   for (i = 0; i < 4; i++)
   {
      /* Check image origin */
      fgetc(fp);
#if 0
      if (j != 0)
      {
         File_Error(File_Name, 4);
         return (-1);
      }
#endif
   }
   /* Check Image specs */
   for (i = 0; i < 4; i++)
      if (fgetc(fp) != (int) upr_lwr[i])
      {
         File_Error(File_Name, 3);
         return (-1);
      }

   if (fgetc(fp) != (int) T24)
      error = 4;                /* Is it a targa 24 file? */
   if (fgetc(fp) != (int) T32)
      error = 4;                /* Is the origin at the upper left? */
   if (error == 4)
   {
      File_Error(File_Name, 4);
      return (-1);
   }
   rewind(fp);

   /* Now that we know its a good file, create a working copy */
   if (startdisk1(targa_temp, fp, 1))
      return (-1);

   fclose(fp);                  /* Close the source */

   T_Safe = 1;                  /* Original file successfully copied to
                                 * targa_temp */
   return (0);
}

static int R_H(BYTE R, BYTE G, BYTE B, unsigned long *H, unsigned long *S, unsigned long *V)
{
   unsigned long R1, G1, B1, DENOM;
   BYTE MIN;

   *V = R;
   MIN = G;
   if (R < G)
   {
      *V = G;
      MIN = R;
      if (G < B)
         *V = B;
      if (B < R)
         MIN = B;
   }
   else
   {
      if (B < G)
         MIN = B;
      if (R < B)
         *V = B;
   }
   DENOM = *V - MIN;
   if (*V != 0 && DENOM != 0)
   {
      *S = ((DENOM << 16) / *V) - 1;
   }
   else
      *S = 0;      /* Color is black! and Sat has no meaning */
   if (*S == 0)    /* R=G=B => shade of grey and Hue has no meaning */
   {
      *H = 0;
      *V = *V << 8;
      return (1);               /* v or s or both are 0 */
   }
   if (*V == MIN)
   {
      *H = 0;
      *V = *V << 8;
      return (0);
   }
   R1 = (((*V - R) * 60) << 6) / DENOM; /* distance of color from red   */
   G1 = (((*V - G) * 60) << 6) / DENOM; /* distance of color from green */
   B1 = (((*V - B) * 60) << 6) / DENOM; /* distance of color from blue  */
   if (*V == R) {
      if (MIN == G)
         *H = (300 << 6) + B1;
      else
         *H = (60 << 6) - G1;
   }
   if (*V == G) {
      if (MIN == B)
         *H = (60 << 6) + R1;
      else
         *H = (180 << 6) - B1;
   }
   if (*V == B) {
      if (MIN == R)
         *H = (180 << 6) + G1;
      else
         *H = (300 << 6) - R1;
    }
   *V = *V << 8;
   return (0);
}

static int H_R(BYTE *R, BYTE *G, BYTE *B, unsigned long H, unsigned long S, unsigned long V)
{
   unsigned long P1, P2, P3;
   int RMD, I;

   if (H >= 23040)
      H = H % 23040;            /* Makes h circular  */
   I = (int) (H / 3840);
   RMD = (int) (H % 3840);      /* RMD = fractional part of H    */

   P1 = ((V * (65535L - S)) / 65280L) >> 8;
   P2 = (((V * (65535L - (S * RMD) / 3840)) / 65280L) - 1) >> 8;
   P3 = (((V * (65535L - (S * (3840 - RMD)) / 3840)) / 65280L)) >> 8;
   V = V >> 8;
   switch (I)
   {
   case 0:
      *R = (BYTE) V;
      *G = (BYTE) P3;
      *B = (BYTE) P1;
      break;
   case 1:
      *R = (BYTE) P2;
      *G = (BYTE) V;
      *B = (BYTE) P1;
      break;
   case 2:
      *R = (BYTE) P1;
      *G = (BYTE) V;
      *B = (BYTE) P3;
      break;
   case 3:
      *R = (BYTE) P1;
      *G = (BYTE) P2;
      *B = (BYTE) V;
      break;
   case 4:
      *R = (BYTE) P3;
      *G = (BYTE) P1;
      *B = (BYTE) V;
      break;
   case 5:
      *R = (BYTE) V;
      *G = (BYTE) P1;
      *B = (BYTE) P2;
      break;
   }
   return (0);
}


/***************************************************************************/
/*                                                                         */
/* EB & DG fiddled with outputs for Rayshade so they work. with v4.x.      */
/* EB == eli brandt.     ebrandt@jarthur.claremont.edu                     */
/* DG == dan goldwater.  daniel_goldwater@brown.edu & dgold@math.umass.edu */
/*  (NOTE: all the stuff we fiddled with is commented with "EB & DG" )     */
/* general raytracing code info/notes:                                     */
/*                                                                         */
/*  ray == 0 means no raytracer output  ray == 7 is for dxf                */
/*  ray == 1 is for dkb/pov             ray == 4 is for mtv                */
/*  ray == 2 is for vivid               ray == 5 is for rayshade           */
/*  ray == 3 is for raw                 ray == 6 is for acrospin           */
/*                                                                         */
/*  rayshade needs counterclockwise triangles.  raytracers that support    */
/*  the 'heightfield' primitive include rayshade and pov.  anyone want to  */
/*  write code to make heightfields?  they are *MUCH* faster to trace than */
/*  triangles when doing landscapes...                                     */
/*                                                                         */
/*  stuff EB & DG changed:                                                 */
/*  made the rayshade output create a "grid" aggregate object (one of      */
/*  rayshade's primitives), instead  of a global grid.  as a result, the   */
/*  grid can be optimized based on the number of triangles.                */
/*  the z component of the grid can always be 1 since the surface formed   */
/*  by the triangles is flat                                               */
/*  (ie, it doesnt curve over itself).  this is a major optimization.      */
/*  the x and y grid size is also optimized for a 4:3 aspect ratio image,  */
/*  to get the fewest possible traingles in each grid square.              */
/*  also, we fixed the rayshade code so it actually produces output that   */
/*  works with rayshade.                                                   */
/*  (maybe the old code was for a really old version of rayshade?).        */
/*                                                                         */
/***************************************************************************/

/********************************************************************/
/*                                                                  */
/*  This routine writes a header to a ray tracer data file. It      */
/*  Identifies the version of FRACTINT which created it an the      */
/*  key 3D parameters in effect at the time.                        */
/*                                                                  */
/********************************************************************/

static char declare[] = {"DECLARE       "};
static char frac_default[] = {"F_Dflt"};
static char s_color[] = {"COLOR  "};
static char dflt[] = {"RED 0.8 GREEN 0.4 BLUE 0.1\n"};
static char d_color[] = {"0.8 0.4 0.1"};
static char r_surf[] = {"0.95 0.05 5 0 0\n"};
static char surf[] = {"surf={diff="};
/* EB & DG: changed "surface T" to "applysurf" and "diff" to "diffuse" */
static char rs_surf[] = {"applysurf diffuse "};
static char end[] = {"END_"};
static char plane[] = {"PLANE"};
static char m1[] = {"-1.0 "};
static char one[] = {" 1.0 "};
static char z[] = {" 0.0 "};
static char bnd_by[] = {" BOUNDED_BY\n"};
static char end_bnd[] = {" END_BOUND\n"};
static char inter[] = {"INTERSECTION\n"};
#ifndef XFRACT
static char fmt[] = "   %Fs <%Fs%Fs%Fs> % #4.3f %Fs%Fs\n";
#else
static char fmt[] = "   %s <%s%s%s> % #4.3f %s%s\n";
#endif
static char dxf_begin[] =
{"  0\nSECTION\n  2\nTABLES\n  0\nTABLE\n  2\nLAYER\n\
 70\n     2\n  0\nLAYER\n  2\n0\n 70\n     0\n 62\n     7\n  6\nCONTINUOUS\n\
  0\nLAYER\n  2\nFRACTAL\n 70\n    64\n 62\n     1\n  6\nCONTINUOUS\n  0\n\
ENDTAB\n  0\nENDSEC\n  0\nSECTION\n  2\nENTITIES\n"};
static char dxf_3dface[] = {"  0\n3DFACE\n  8\nFRACTAL\n 62\n%3d\n"};
static char dxf_vertex[] = {"%3d\n%g\n"};
static char dxf_end[] = {"  0\nENDSEC\n  0\nEOF\n"};
static char composite[] = {"COMPOSITE"};
static char object[] = {"OBJECT"};
static char triangle[] = {"TRIANGLE "};
static char l_tri[] = {"triangle"};
static char texture[] = {"TEXTURE\n"};
/* static char end_texture[] = {" END_TEXTURE\n"}; */
static char red[] = {"RED"};
static char green[] = {"GREEN"};
static char blue[] = {"BLUE"};
static char frac_texture[] = {"      AMBIENT 0.25 DIFFUSE 0.75"};
static char polygon[] = {"polygon={points=3;"};
static char vertex[] = {" vertex =  "};
static char d_vert[] = {"      <"};
static char f1[] = "% #4.4f ";
/* EB & DG: changed this to much better values */
static char grid[] =
{"screen 640 480\neyep 0 2.1 0.8\nlookp 0 0 -0.95\nlight 1 point -2 1 1.5\n"};
static char grid2[] = {"background .3 0 0\nreport verbose\n"};

static char s_n[] = "\n";
static char f2[] = "R%dC%d R%dC%d\n";
static char ray_comment1[] =
   {"/* make a gridded aggregate. this size grid is fast for landscapes. */\n"};
static char ray_comment2[] =
   {"/* make z grid = 1 always for landscapes. */\n\n"};
static char grid3[] = {"grid 33 25 1\n"};

static int _fastcall RAY_Header(void)
{
   /* Open the ray tracing output file */
   check_writefile(ray_name, ".ray");
   if ((File_Ptr1 = fopen(ray_name, "w")) == NULL)
      return (-1);              /* Oops, somethings wrong! */

   if (RAY == 2)
      fprintf(File_Ptr1, "//");
   if (RAY == 4)
      fprintf(File_Ptr1, "#");
   if (RAY == 5)
      fprintf(File_Ptr1, "/*\n");
   if (RAY == 6)
      fprintf(File_Ptr1, "--");
   if (RAY == 7)
      fprintf(File_Ptr1, dxf_begin);

   if (RAY != 7)
      fprintf(File_Ptr1, banner, (char *)s3, g_release / 100., (char *)s3a);

   if (RAY == 5)
      fprintf(File_Ptr1, "*/\n");


   /* Set the default color */
   if (RAY == 1)
   {
      fprintf(File_Ptr1, s_f, (char *)declare, (char *)frac_default);
      fprintf(File_Ptr1, " = ");
      fprintf(File_Ptr1, s_f, (char *)s_color, (char *)dflt);
   }
   if (BRIEF)
   {
      if (RAY == 2)
      {
         fprintf(File_Ptr1, s_f, (char *)surf, (char *)d_color);
         fprintf(File_Ptr1, ";}\n");
      }
      if (RAY == 4)
      {
         fprintf(File_Ptr1, "f ");
         fprintf(File_Ptr1, s_f, (char *)d_color, (char *)r_surf);
      }
      if (RAY == 5)
         fprintf(File_Ptr1, s_f, (char *)rs_surf, (char *)d_color);
   }
   if (RAY != 7)
      fprintf(File_Ptr1, s_n);

   /* EB & DG: open "grid" opject, a speedy way to do aggregates in rayshade */
   if (RAY == 5)
      fprintf(File_Ptr1, s_fff, (char *)ray_comment1, (char *)ray_comment2, (char *)grid3);

   if (RAY == 6)
#ifndef XFRACT
      fprintf(File_Ptr1, "%Fs", (char *)acro_s1);
#else
      fprintf(File_Ptr1, "%s", acro_s1);
#endif

   return (0);
}


/********************************************************************/
/*                                                                  */
/*  This routine describes the triangle to the ray tracer, it       */
/*  sets the color of the triangle to the average of the color      */
/*  of its verticies and sets the light parameters to arbitrary     */
/*  values.                                                         */
/*                                                                  */
/*  Note: numcolors (number of colors in the source                 */
/*  file) is used instead of colors (number of colors avail. with   */
/*  display) so you can generate ray trace files with your LCD      */
/*  or monochrome display                                           */
/*                                                                  */
/********************************************************************/

static int _fastcall out_triangle(struct f_point pt1, struct f_point pt2, struct f_point pt3, int c1, int c2, int c3)
{
   int i, j;
   float c[3];
   float pt_t[3][3];

   /* Normalize each vertex to screen size and adjust coordinate system */
   pt_t[0][0] = 2 * pt1.x / xdots - 1;
   pt_t[0][1] = (2 * pt1.y / ydots - 1);
   pt_t[0][2] = -2 * pt1.color / numcolors - 1;
   pt_t[1][0] = 2 * pt2.x / xdots - 1;
   pt_t[1][1] = (2 * pt2.y / ydots - 1);
   pt_t[1][2] = -2 * pt2.color / numcolors - 1;
   pt_t[2][0] = 2 * pt3.x / xdots - 1;
   pt_t[2][1] = (2 * pt3.y / ydots - 1);
   pt_t[2][2] = -2 * pt3.color / numcolors - 1;

   /* Color of triangle is average of colors of its verticies */
   if (!BRIEF)
      for (i = 0; i <= 2; i++)
#ifdef __SVR4
         c[i] = (float) ((int)(g_dacbox[c1][i] + g_dacbox[c2][i] + g_dacbox[c3][i])
            / (3 * 63));
#else
         c[i] = (float) (g_dacbox[c1][i] + g_dacbox[c2][i] + g_dacbox[c3][i])
            / (3 * 63);
#endif

   /* get rid of degenerate triangles: any two points equal */
   if ((pt_t[0][0] == pt_t[1][0] &&
        pt_t[0][1] == pt_t[1][1] &&
        pt_t[0][2] == pt_t[1][2]) ||

       (pt_t[0][0] == pt_t[2][0] &&
        pt_t[0][1] == pt_t[2][1] &&
        pt_t[0][2] == pt_t[2][2]) ||

       (pt_t[2][0] == pt_t[1][0] &&
        pt_t[2][1] == pt_t[1][1] &&
        pt_t[2][2] == pt_t[1][2]))
      return (0);

   /* Describe the triangle */
#ifndef XFRACT
   if (RAY == 1)
      fprintf(File_Ptr1, " %Fs\n  %Fs", (char *)object, (char *)triangle);
   if (RAY == 2 && !BRIEF)
      fprintf(File_Ptr1, "%Fs", (char *)surf);
#else
   if (RAY == 1)
      fprintf(File_Ptr1, " %s\n  %s", object, triangle);
   if (RAY == 2 && !BRIEF)
      fprintf(File_Ptr1, "%s", surf);
#endif
   if (RAY == 4 && !BRIEF)
      fprintf(File_Ptr1, "f");
   if (RAY == 5 && !BRIEF)
#ifndef XFRACT
      fprintf(File_Ptr1, "%Fs", (char *)rs_surf);
#else
      fprintf(File_Ptr1, "%s", rs_surf);
#endif

   if (!BRIEF && RAY != 1 && RAY != 7)
      for (i = 0; i <= 2; i++)
         fprintf(File_Ptr1, f1, c[i]);

   if (RAY == 2)
   {
      if (!BRIEF)
         fprintf(File_Ptr1, ";}\n");
#ifndef XFRACT
      fprintf(File_Ptr1, "%Fs", (char *)polygon);
#else
      fprintf(File_Ptr1, "%s", polygon);
#endif
   }
   if (RAY == 4)
   {
      if (!BRIEF)
#ifndef XFRACT
         fprintf(File_Ptr1, "%Fs", (char *)r_surf);
#else
         fprintf(File_Ptr1, "%s", r_surf);
#endif
      fprintf(File_Ptr1, "p 3");
   }
   if (RAY == 5)
   {
      if (!BRIEF)
         fprintf(File_Ptr1, s_n);
      /* EB & DG: removed "T" after "triangle" */
#ifndef XFRACT
      fprintf(File_Ptr1, "%Fs", (char *)l_tri);
#else
      fprintf(File_Ptr1, "%s", l_tri);
#endif
   }

   if (RAY == 7)
      fprintf(File_Ptr1, dxf_3dface, min(255, max(1, c1)));

   for (i = 0; i <= 2; i++)     /* Describe each  Vertex  */
   {
      if (RAY != 7)
         fprintf(File_Ptr1, s_n);

#ifndef XFRACT
      if (RAY == 1)
         fprintf(File_Ptr1, "%Fs", (char *)d_vert);
      if (RAY == 2)
         fprintf(File_Ptr1, "%Fs", (char *)vertex);
#else
      if (RAY == 1)
         fprintf(File_Ptr1, "%s", d_vert);
      if (RAY == 2)
         fprintf(File_Ptr1, "%s", vertex);
#endif
      if (RAY > 3 && RAY != 7)
         fprintf(File_Ptr1, " ");

      for (j = 0; j <= 2; j++)
      {
         if (RAY == 7)
         {
            /* write 3dface entity to dxf file */
            fprintf(File_Ptr1, dxf_vertex, 10 * (j + 1) + i, pt_t[i][j]);
            if (i == 2)         /* 3dface needs 4 vertecies */
               fprintf(File_Ptr1, dxf_vertex, 10 * (j + 1) + i + 1,
                  pt_t[i][j]);
         }
         else if (!(RAY == 4 || RAY == 5))
            fprintf(File_Ptr1, f1, pt_t[i][j]); /* Right handed */
         else
            fprintf(File_Ptr1, f1, pt_t[2 - i][j]);     /* Left handed */
      }

      if (RAY == 1)
         fprintf(File_Ptr1, ">");
      if (RAY == 2)
         fprintf(File_Ptr1, ";");
   }

   if (RAY == 1)
   {
#ifndef XFRACT
      fprintf(File_Ptr1, " %Fs%Fs\n", (char *)end, (char *)triangle);
#else
      fprintf(File_Ptr1, " %s%s\n", end, triangle);
#endif
      if (!BRIEF)
      {
#ifndef XFRACT
         fprintf(File_Ptr1, "  %Fs"
                 "      %Fs%Fs% #4.4f %Fs% #4.4f %Fs% #4.4f\n"
                 "%Fs"
                 " %Fs%Fs",
#else
         fprintf(File_Ptr1,
                 "  %s   %s%s% #4.4f %s% #4.4f %s% #4.4f\n%s %s%s",
#endif
                 (char *)texture,
                 (char *)s_color,
                 (char *)red,   c[0],
                 (char *)green, c[1],
                 (char *)blue,  c[2],
                 (char *)frac_texture,
                 (char *)end,
                 (char *)texture);
      }
#ifndef XFRACT
      fprintf(File_Ptr1, "  %Fs%Fs  %Fs%Fs",
#else
      fprintf(File_Ptr1, "  %s%s  %s%s",
#endif
              (char *)s_color, (char *)frac_default,
              (char *)end, (char *)object);
      triangle_bounds(pt_t);    /* update bounding info */
   }
   if (RAY == 2)
      fprintf(File_Ptr1, "}");
   if (RAY == 3 && !BRIEF)
      fprintf(File_Ptr1, s_n);

   if (RAY != 7)
      fprintf(File_Ptr1, s_n);

   return (0);
}

/********************************************************************/
/*                                                                  */
/*  This routine calculates the min and max values of a triangle    */
/*  for use in creating ray tracer data files. The values of min    */
/*  and max x, y, and z are assumed to be global.                   */
/*                                                                  */
/********************************************************************/

static void _fastcall triangle_bounds(float pt_t[3][3])
{
   int i, j;

   for (i = 0; i <= 2; i++)
      for (j = 0; j <= 2; j++)
      {
         if (pt_t[i][j] < min_xyz[j])
            min_xyz[j] = pt_t[i][j];
         if (pt_t[i][j] > max_xyz[j])
            max_xyz[j] = pt_t[i][j];
      }
   return;
}

/********************************************************************/
/*                                                                  */
/*  This routine starts a composite object for ray trace data files */
/*                                                                  */
/********************************************************************/

static int _fastcall start_object(void)
{
   if (RAY != 1)
      return (0);

   /* Reset the min/max values, for bounding box  */
   min_xyz[0] = min_xyz[1] = min_xyz[2] = (float)999999.0;
   max_xyz[0] = max_xyz[1] = max_xyz[2] = (float)-999999.0;

#ifndef XFRACT
   fprintf(File_Ptr1, "%Fs\n", (char *)composite);
#else
   fprintf(File_Ptr1, "%s\n", composite);
#endif
   return (0);
}

/********************************************************************/
/*                                                                  */
/*  This routine adds a bounding box for the triangles drawn        */
/*  in the last block and completes the composite object created.   */
/*  It uses the globals min and max x,y and z calculated in         */
/*  z calculated in Triangle_Bounds().                              */
/*                                                                  */
/********************************************************************/

static int _fastcall end_object(int triout)
{
   if (RAY == 7)
      return (0);
   if (RAY == 1)
   {
      if (triout)
      {
         /* Make sure the bounding box is slightly larger than the object */
         int i;
         for (i = 0; i <= 2; i++)
         {
            if (min_xyz[i] == max_xyz[i])
            {
               min_xyz[i] -= (float)0.01;
               max_xyz[i] += (float)0.01;
            }
            else
            {
               min_xyz[i] -= (max_xyz[i] - min_xyz[i]) * (float)0.01;
               max_xyz[i] += (max_xyz[i] - min_xyz[i]) * (float)0.01;
            }
         }

         /* Add the bounding box info */
#ifndef XFRACT
         fprintf(File_Ptr1, "%Fs  %Fs", (char *)bnd_by, (char *)inter);
#else
         fprintf(File_Ptr1, "%s  %s", bnd_by, inter);
#endif
         fprintf(File_Ptr1, fmt, (char *)plane, (char *)m1, (char *)z, (char *)z, -min_xyz[0], (char *)end, (char *)plane);
         fprintf(File_Ptr1, fmt, (char *)plane, (char *)one, (char *)z, (char *)z, max_xyz[0], (char *)end, (char *)plane);
         fprintf(File_Ptr1, fmt, (char *)plane, (char *)z, (char *)m1, (char *)z, -min_xyz[1], (char *)end, (char *)plane);
         fprintf(File_Ptr1, fmt, (char *)plane, (char *)z, (char *)one, (char *)z, max_xyz[1], (char *)end, (char *)plane);
         fprintf(File_Ptr1, fmt, (char *)plane, (char *)z, (char *)z, (char *)m1, -min_xyz[2], (char *)end, (char *)plane);
         fprintf(File_Ptr1, fmt, (char *)plane, (char *)z, (char *)z, (char *)one, max_xyz[2], (char *)end, (char *)plane);
#ifndef XFRACT
         fprintf(File_Ptr1, "  %Fs%Fs%Fs", (char *)end,
                (char *)inter, (char *)end_bnd);
#else
         fprintf(File_Ptr1, "  %s%s%s", end, inter, end_bnd);
#endif
      }

      /* Complete the composite object statement */
#ifndef XFRACT
      fprintf(File_Ptr1, "%Fs%Fs\n", (char *)end, (char *)composite);
#else
      fprintf(File_Ptr1, "%s%s\n", end, composite);
#endif
   }

   if (RAY != 6 && RAY != 5)
      fprintf(File_Ptr1, s_n);    /* EB & DG: too many newlines */

   return (0);
}

static void line3d_cleanup(void)
{
   int i, j;
   if (RAY && File_Ptr1)
   {                            /* Finish up the ray tracing files */
      static char n_ta[] = {"{ No. Of Triangles = "};
      if (RAY != 5 && RAY != 7)
         fprintf(File_Ptr1, s_n); /* EB & DG: too many newlines */
      if (RAY == 2)
         fprintf(File_Ptr1, "\n\n//");
      if (RAY == 4)
         fprintf(File_Ptr1, "\n\n#");

      if (RAY == 5)
#ifndef XFRACT
         /* EB & DG: end grid aggregate */
         fprintf(File_Ptr1, "end\n\n/*good landscape:*/\n%Fs%Fs\n/*",
            (char *)grid, (char *)grid2);
#else
         /* EB & DG: end grid aggregate */
         fprintf(File_Ptr1, "end\n\n/*good landscape:*/\n%s%s\n/*",
            grid, grid2);
#endif
      if (RAY == 6)
      {
#ifndef XFRACT
         fprintf(File_Ptr1, "%Fs", (char *)acro_s2);
#else
         fprintf(File_Ptr1, "%s", acro_s2);
#endif
         for (i = 0; i < RO; i++)
            for (j = 0; j <= CO_MAX; j++)
            {
               if (j < CO_MAX)
                  fprintf(File_Ptr1, f2, i, j, i, j + 1);
               if (i < RO - 1)
                  fprintf(File_Ptr1, f2, i, j, i + 1, j);
               if (i && i < RO && j < CO_MAX)
                  fprintf(File_Ptr1, f2, i, j, i - 1, j + 1);
            }
         fprintf(File_Ptr1, "\n\n--");
      }
      if (RAY != 7)
#ifndef XFRACT
         fprintf(File_Ptr1, "%Fs%ld }*/\n\n", (char *)n_ta, num_tris);
#else
         fprintf(File_Ptr1, "%s%ld }*/\n\n", n_ta, num_tris);
#endif
      if (RAY == 7)
         fprintf(File_Ptr1, dxf_end);
      fclose(File_Ptr1);
      File_Ptr1 = NULL;
   }
   if (Targa_Out)
   {                            /* Finish up targa files */
      T_header_24 = 18;         /* Reset Targa header size */
      enddisk();
      if (!debugflag && (!T_Safe || error) && Targa_Overlay)
      {
         dir_remove(workdir, light_name);
         rename(targa_temp, light_name);
      }
      if (!debugflag && Targa_Overlay)
         dir_remove(workdir, targa_temp);
   }
   usr_floatflag &= 1;          /* strip second bit */
   error = T_Safe = 0;
}

static void set_upr_lwr(void)
{
   upr_lwr[0] = (BYTE)(xdots & 0xff);
   upr_lwr[1] = (BYTE)(xdots >> 8);
   upr_lwr[2] = (BYTE)(ydots & 0xff);
   upr_lwr[3] = (BYTE)(ydots >> 8);
   line_length1 = 3 * xdots;    /* line length @ 3 bytes per pixel  */
}

static int first_time(int linelen, VECTOR v)
{
   int err;
   MATRIX lightm;               /* m w/no trans, keeps obj. on screen */
   float twocosdeltatheta;
   double xval, yval, zval;     /* rotation values */
   /* corners of transformed xdotx by ydots x colors box */
   double xmin, ymin, zmin, xmax, ymax, zmax;
   int i, j;
   double v_length;
   VECTOR origin, direct, tmp;
   float theta, theta1, theta2; /* current,start,stop latitude */
   float phi1, phi2;            /* current start,stop longitude */
   float deltatheta;            /* increment of latitude */
   outln_cleanup = line3d_cleanup;

   calctime = evenoddrow = 0;
   /* mark as in-progress, and enable <tab> timer display */
   calc_status = 1;

   IAmbient = (unsigned int) (255 * (float) (100 - Ambient) / 100.0);
   if (IAmbient < 1)
      IAmbient = 1;

   num_tris = 0;

   /* Open file for RAY trace output and write header */
   if (RAY)
   {
      RAY_Header();
      xxadjust = yyadjust = 0;  /* Disable shifting in ray tracing */
      xshift = yshift = 0;
   }

   CO_MAX = CO = RO = 0;

   set_upr_lwr();
   error = 0;

   if (whichimage < 2)
      T_Safe = 0; /* Not safe yet to mess with the source image */

   if (Targa_Out && !((glassestype == 1 || glassestype == 2)
                 && whichimage == 2))
   {
      if (Targa_Overlay)
      {
         /* Make sure target file is a supportable Targa File */
         if (targa_validate(light_name))
            return (-1);
      }
      else
      {
         check_writefile(light_name, ".tga");
         if (startdisk1(light_name, NULL, 0))   /* Open new file */
            return (-1);
      }
   }

   rand_factor = 14 - RANDOMIZE;

   zcoord = filecolors;

   if((err=line3dmem()) != 0)
      return(err);


   /* get scale factors */
   sclx = XSCALE / 100.0;
   scly = YSCALE / 100.0;
   if (ROUGH)
      sclz = -ROUGH / 100.0;
   else
      rscale = sclz = -0.0001;  /* if rough=0 make it very flat but plot
                                 * something */

   /* aspect ratio calculation - assume screen is 4 x 3 */
   aspect = (double) xdots *.75 / (double) ydots;

   if (SPHERE == FALSE)         /* skip this slow stuff in sphere case */
   {
      /*********************************************************************/
      /* What is done here is to create a single matrix, m, which has      */
      /* scale, rotation, and shift all combined. This allows us to use    */
      /* a single matrix to transform any point. Additionally, we create   */
      /* two perspective vectors.                                          */
      /*                                                                   */
      /* Start with a unit matrix. Add scale and rotation. Then calculate  */
      /* the perspective vectors. Finally add enough translation to center */
      /* the final image plus whatever shift the user has set.             */
      /*********************************************************************/

      /* start with identity */
      identity(m);
      identity(lightm);

      /* translate so origin is in center of box, so that when we rotate */
      /* it, we do so through the center */
      trans((double) xdots / (-2.0), (double) ydots / (-2.0),
            (double) zcoord / (-2.0), m);
      trans((double) xdots / (-2.0), (double) ydots / (-2.0),
            (double) zcoord / (-2.0), lightm);

      /* apply scale factors */
      scale(sclx, scly, sclz, m);
      scale(sclx, scly, sclz, lightm);

      /* rotation values - converting from degrees to radians */
      xval = XROT / 57.29577;
      yval = YROT / 57.29577;
      zval = ZROT / 57.29577;

      if (RAY)
      {
         xval = yval = zval = 0;
      }

      xrot(xval, m);
      xrot(xval, lightm);
      yrot(yval, m);
      yrot(yval, lightm);
      zrot(zval, m);
      zrot(zval, lightm);

      /* Find values of translation that make all x,y,z negative */
      /* m current matrix */
      /* 0 means don't show box */
      /* returns minimum and maximum values of x,y,z in fractal */
      corners(m, 0, &xmin, &ymin, &zmin, &xmax, &ymax, &zmax);
   }

   /* perspective 3D vector - lview[2] == 0 means no perspective */

   /* set perspective flag */
   persp = 0;
   if (ZVIEWER != 0)
   {
      persp = 1;
      if (ZVIEWER < 80)         /* force float */
         usr_floatflag |= 2;    /* turn on second bit */
   }

   /* set up view vector, and put viewer in center of screen */
   lview[0] = xdots >> 1;
   lview[1] = ydots >> 1;

   /* z value of user's eye - should be more negative than extreme negative
    * part of image */
   if (SPHERE)                  /* sphere case */
      lview[2] = -(long) ((double) ydots * (double) ZVIEWER / 100.0);
   else                         /* non-sphere case */
      lview[2] = (long) ((zmin - zmax) * (double) ZVIEWER / 100.0);

   view[0] = lview[0];
   view[1] = lview[1];
   view[2] = lview[2];
   lview[0] = lview[0] << 16;
   lview[1] = lview[1] << 16;
   lview[2] = lview[2] << 16;

   if (SPHERE == FALSE)         /* sphere skips this */
   {
      /* translate back exactly amount we translated earlier plus enough to
       * center image so maximum values are non-positive */
      trans(((double) xdots - xmax - xmin) / 2,
            ((double) ydots - ymax - ymin) / 2, -zmax, m);

      /* Keep the box centered and on screen regardless of shifts */
      trans(((double) xdots - xmax - xmin) / 2,
            ((double) ydots - ymax - ymin) / 2, -zmax, lightm);

      trans((double) (xshift), (double) (-yshift), 0.0, m);

      /* matrix m now contains ALL those transforms composed together !!
       * convert m to long integers shifted 16 bits */
      for (i = 0; i < 4; i++)
         for (j = 0; j < 4; j++)
            llm[i][j] = (long) (m[i][j] * 65536.0);

   }
   else
      /* sphere stuff goes here */
   {
      /* Sphere is on side - north pole on right. Top is -90 degrees
       * latitude; bottom 90 degrees */

      /* Map X to this LATITUDE range */
      theta1 = (float) (THETA1 * PI / 180.0);
      theta2 = (float) (THETA2 * PI / 180.0);

      /* Map Y to this LONGITUDE range */
      phi1 = (float) (PHI1 * PI / 180.0);
      phi2 = (float) (PHI2 * PI / 180.0);

      theta = theta1;

      /*********************************************************************/
      /* Thanks to Hugh Bray for the following idea: when calculating      */
      /* a table of evenly spaced sines or cosines, only a few initial     */
      /* values need be calculated, and the remaining values can be        */
      /* gotten from a derivative of the sine/cosine angle sum formula     */
      /* at the cost of one multiplication and one addition per value!     */
      /*                                                                   */
      /* This idea is applied once here to get a complete table for        */
      /* latitude, and near the bottom of this routine to incrementally    */
      /* calculate longitude.                                              */
      /*                                                                   */
      /* Precalculate 2*cos(deltaangle), sin(start) and sin(start+delta).  */
      /* Then apply recursively:                                           */
      /* sin(angle+2*delta) = sin(angle+delta) * 2cosdelta - sin(angle)    */
      /*                                                                   */
      /* Similarly for cosine. Neat!                                       */
      /*********************************************************************/

      deltatheta = (float) (theta2 - theta1) / (float) linelen;

      /* initial sin,cos theta */
      sinthetaarray[0] = (float) sin((double) theta);
      costhetaarray[0] = (float) cos((double) theta);
      sinthetaarray[1] = (float) sin((double) (theta + deltatheta));
      costhetaarray[1] = (float) cos((double) (theta + deltatheta));

      /* sin,cos delta theta */
      twocosdeltatheta = (float) (2.0 * cos((double) deltatheta));

      /* build table of other sin,cos with trig identity */
      for (i = 2; i < (int) linelen; i++)
      {
         sinthetaarray[i] = sinthetaarray[i - 1] * twocosdeltatheta -
            sinthetaarray[i - 2];
         costhetaarray[i] = costhetaarray[i - 1] * twocosdeltatheta -
            costhetaarray[i - 2];
      }

      /* now phi - these calculated as we go - get started here */
      deltaphi = (float) (phi2 - phi1) / (float) height;

      /* initial sin,cos phi */

      sinphi = oldsinphi1 = (float) sin((double) phi1);
      cosphi = oldcosphi1 = (float) cos((double) phi1);
      oldsinphi2 = (float) sin((double) (phi1 + deltaphi));
      oldcosphi2 = (float) cos((double) (phi1 + deltaphi));

      /* sin,cos delta phi */
      twocosdeltaphi = (float) (2.0 * cos((double) deltaphi));


      /* affects how rough planet terrain is */
      if (ROUGH)
         rscale = .3 * ROUGH / 100.0;

      /* radius of planet */
      R = (double) (ydots) / 2;

      /* precalculate factor */
      rXrscale = R * rscale;

      sclz = sclx = scly = RADIUS / 100.0;      /* Need x,y,z for RAY */

      /* adjust x scale factor for aspect */
      sclx *= aspect;

      /* precalculation factor used in sphere calc */
      Rfactor = rscale * R / (double) zcoord;

      if (persp)                /* precalculate fudge factor */
      {
         double radius;
         double zview;
         double angle;

         xcenter = xcenter << 16;
         ycenter = ycenter << 16;

         Rfactor *= 65536.0;
         R *= 65536.0;

         /* calculate z cutoff factor attempt to prevent out-of-view surfaces
          * from being written */
         zview = -(long) ((double) ydots * (double) ZVIEWER / 100.0);
         radius = (double) (ydots) / 2;
         angle = atan(-radius / (zview + radius));
         zcutoff = -radius - sin(angle) * radius;
         zcutoff *= 1.1;        /* for safety */
         zcutoff *= 65536L;
      }
   }

   /* set fill plot function */
   if (FILLTYPE != 3)
      fillplot = interpcolor;
   else
   {
      fillplot = clipcolor;

      if (transparent[0] || transparent[1])
         /* If transparent colors are set */
         fillplot = T_clipcolor;/* Use the transparent plot function  */
   }

   /* Both Sphere and Normal 3D */
   direct[0] = light_direction[0] = XLIGHT;
   direct[1] = light_direction[1] = -YLIGHT;
   direct[2] = light_direction[2] = ZLIGHT;

   /* Needed because sclz = -ROUGH/100 and light_direction is transformed in
    * FILLTYPE 6 but not in 5. */
   if (FILLTYPE == 5)
      direct[2] = light_direction[2] = -ZLIGHT;

   if (FILLTYPE == 6)           /* transform light direction */
   {
      /* Think of light direction  as a vector with tail at (0,0,0) and head
       * at (light_direction). We apply the transformation to BOTH head and
       * tail and take the difference */

      v[0] = 0.0;
      v[1] = 0.0;
      v[2] = 0.0;
      vmult(v, m, v);
      vmult(light_direction, m, light_direction);

      for (i = 0; i < 3; i++)
         light_direction[i] -= v[i];
   }
   normalize_vector(light_direction);

   if (preview && showbox)
   {
      normalize_vector(direct);

      /* move light vector to be more clear with grey scale maps */
      origin[0] = (3 * xdots) / 16;
      origin[1] = (3 * ydots) / 4;
      if (FILLTYPE == 6)
         origin[1] = (11 * ydots) / 16;

      origin[2] = 0.0;

      v_length = min(xdots, ydots) / 2;
      if (persp && ZVIEWER <= P)
         v_length *= (long) (P + 600) / ((long) (ZVIEWER + 600) * 2);

      /* Set direct[] to point from origin[] in direction of untransformed
       * light_direction (direct[]). */
      for (i = 0; i < 3; i++)
         direct[i] = origin[i] + direct[i] * v_length;

      /* center light box */
      for (i = 0; i < 2; i++)
      {
         tmp[i] = (direct[i] - origin[i]) / 2;
         origin[i] -= tmp[i];
         direct[i] -= tmp[i];
      }

      /* Draw light source vector and box containing it, draw_light_box will
       * transform them if necessary. */
      draw_light_box(origin, direct, lightm);
      /* draw box around original field of view to help visualize effect of
       * rotations 1 means show box - xmin etc. do nothing here */
      if (!SPHERE)
         corners(m, 1, &xmin, &ymin, &zmin, &xmax, &ymax, &zmax);
   }

   /* bad has values caught by clipping */
   f_bad.x = (float) (bad.x = bad_value);
   f_bad.y = (float) (bad.y = bad_value);
   f_bad.color = (float) (bad.color = bad_value);
   for (i = 0; i < (int) linelen; i++)
   {
      lastrow[i] = bad;
      f_lastrow[i] = f_bad;
   }
   got_status = 3;
   return (0);
} /* end of once-per-image intializations */

/*
   This pragma prevents optimizer failure in MSC/C++ 7.0. Program compiles ok
   without pragma, but error message is real ugly, paraphrasing loosely,
   something like "optimizer screwed up big time, call Bill Gates collect ...
   (Note: commented out pragma because we removed the compiler "/Og" option
    in MAKEFRACT.BAT - left these notes as a warning...
*/
#ifdef _MSC_VER
#if (_MSC_VER >= 600)
/* #pragma optimize( "g", off ) */
#endif
#endif

static int line3dmem(void)
{
   /*********************************************************************/
   /*  Memory allocation - assumptions - a 64K segment starting at      */
   /*  extraseg has been grabbed. It may have other purposes elsewhere, */
   /*  but it is assumed that it is totally free for use here. Our      */
   /*  strategy is to assign all the pointers needed here to various*/
   /*  spots in the extra segment, depending on the pixel dimensions of */
   /*  the video mode, and check whether we have run out. There is      */
   /*  basically one case where the extra segment is not big enough     */
   /*  -- SPHERE mode with a fill type that uses putatriangle() (array  */
   /*  minmax_x) at the largest legal resolution of MAXPIXELSxMAXPIXELS */
   /*  or thereabouts. In that case we use farmemalloc to grab memory   */
   /*  for minmax_x. This memory is never freed.                        */
   /*********************************************************************/
   long check_extra;  /* keep track ofd extraseg array use */

   /* lastrow stores the previous row of the original GIF image for
      the purpose of filling in gaps with triangle procedure */
   /* first 8k of extraseg now used in decoder TW 3/95 */
   /* TODO: allocate real memory, not reuse shared segment */
   lastrow = extraseg;

   check_extra = sizeof(*lastrow) * xdots;
   if (SPHERE)
   {
      sinthetaarray = (float *) (lastrow + xdots);
      check_extra += sizeof(*sinthetaarray) * xdots;
      costhetaarray = (float *) (sinthetaarray + xdots);
      check_extra += sizeof(*costhetaarray) * xdots;
      f_lastrow = (struct f_point *) (costhetaarray + xdots);
   }
   else
      f_lastrow = (struct f_point *) (lastrow + xdots);
   check_extra += sizeof(*f_lastrow) * (xdots);
   if (pot16bit)
   {
      fraction = (BYTE *) (f_lastrow + xdots);
      check_extra += sizeof(*fraction) * xdots;
   }
   minmax_x = (struct minmax *) NULL;

   /* these fill types call putatriangle which uses minmax_x */
   if (FILLTYPE == 2 || FILLTYPE == 3 || FILLTYPE == 5 || FILLTYPE == 6)
   {
      /* end of arrays if we use extra segement */
      check_extra += sizeof(struct minmax) * ydots;
      if (check_extra > (1L << 16))     /* run out of extra segment? */
      {
         static char msg[] = {"malloc minmax"};
         static struct minmax *got_mem = NULL;
         if(debugflag == 2222)
            stopmsg(0,msg);
         /* not using extra segment so decrement check_extra */
         check_extra -= sizeof(struct minmax) * ydots;
         if (got_mem == NULL)
            got_mem = (struct minmax *) (malloc(OLDMAXPIXELS *
                                                    sizeof(struct minmax)));
         if (got_mem)
            minmax_x = got_mem;
         else
            return (-1);
      }
      else /* ok to use extra segment */
      {
         if (pot16bit)
            minmax_x = (struct minmax *) (fraction + xdots);
         else
            minmax_x = (struct minmax *) (f_lastrow + xdots);
      }
   }
   if (debugflag == 2222 || check_extra > (1L << 16))
   {
      char tmpmsg[70];
      static char extramsg[] = {" of extra segment"};
#ifndef XFRACT
      sprintf(tmpmsg, "used %ld%Fs", check_extra, (char *)extramsg);
#else
      sprintf(tmpmsg, "used %ld%s", check_extra, extramsg);
#endif
      stopmsg(STOPMSG_NO_BUZZER, tmpmsg);
   }
   return(0);
}

#ifdef _MSC_VER
#if (_MSC_VER >= 600)
#pragma optimize( "g", on )
#endif
#endif
