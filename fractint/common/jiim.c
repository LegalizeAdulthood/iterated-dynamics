/*
 * JIIM.C
 *
 * Generates Inverse Julia in real time, lets move a cursor which determines
 * the J-set.
 *
 *  The J-set is generated in a fixed-size window, a third of the screen.
 *
 * The routines used to set/move the cursor and to save/restore the
 * window were "borrowed" from editpal.c (TW - now we *use* the editpal code)
 *     (if you don't know how to write good code, look for someone who does)
 *
 *    JJB  [jbuhler@gidef.edu.ar]
 *    TIW  Tim Wegner
 *    MS   Michael Snyder
 *    KS   Ken Shirriff
 * Revision History:
 *
 *        7-28-92       JJB  Initial release out of editpal.c
 *        7-29-92       JJB  Added SaveRect() & RestoreRect() - now the
 *                           screen is restored after leaving.
 *        7-30-92       JJB  Now, if the cursor goes into the window, the
 *                           window is moved to the other side of the screen.
 *                           Worked from the first time!
 *        10-09-92      TIW  A major rewrite that merged cut routines duplicated
 *                           in EDITPAL.C and added orbits feature.
 *        11-02-92      KS   Made cursor blink
 *        11-18-92      MS   Altered Inverse Julia to use MIIM method.
 *        11-25-92      MS   Modified MIIM support routines to better be
 *                           shared with stand-alone inverse julia in
 *                           LORENZ.C, and to use DISKVID for swap space.
 *        05-05-93      TIW  Boy this change file really got out of date.
 *                           Added orbits capability, various commands, some
 *                           of Dan Farmer's ideas like circles and lines
 *                           connecting orbits points.
 *        12-18-93      TIW  Removed use of float only for orbits, fixed a
 *                           helpmode bug.
 *
 */

#include <string.h>

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef __TURBOC__
#   include <mem.h>   /* to get mem...() declarations */
#endif

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"

#define FAR_RESERVE     8192L     /* amount of far mem we will leave avail. */
#define MAXRECT         1024      /* largest width of SaveRect/RestoreRect */

#define newx(size)     mem_alloc(size)
#define delete(block)  block=NULL

int show_numbers =0;              /* toggle for display of coords */
U16 memory_handle = 0;
FILE *file;
int windows = 0;               /* windows management system */

int xc, yc;                       /* corners of the window */
int xd, yd;                       /* dots in the window    */
double xcjul = BIG;
double ycjul = BIG;

void displays(int x, int y, int fg, int bg, char *str, int len)
{
   int i;
   for(i=0;i<len; i++)
      displayc(x+i*8, y, fg, bg, str[i]);
}

/* circle routines from Dr. Dobbs June 1990 */
int xbase, ybase;
unsigned int xAspect, yAspect;

void SetAspect(double aspect)
{
   xAspect = 0;
   yAspect = 0;
   aspect = fabs(aspect);
   if (aspect != 1.0) {
      if (aspect > 1.0)
         yAspect = (unsigned int)(65536.0 / aspect);
      else
         xAspect = (unsigned int)(65536.0 * aspect);
   }
}

void _fastcall c_putcolor(int x, int y, int color)
   {
   /* avoid writing outside window */
   if ( x < xc || y < yc || x >= xc + xd || y >= yc + yd )
      return ;
   if(y >= sydots - show_numbers) /* avoid overwriting coords */
      return;
   if(windows == 2) /* avoid overwriting fractal */
      if (0 <= x && x < xdots && 0 <= y && y < ydots)
         return;
   putcolor(x, y, color);
   }


int  c_getcolor(int x, int y)
   {
   /* avoid reading outside window */
   if ( x < xc || y < yc || x >= xc + xd || y >= yc + yd )
      return 1000;
   if(y >= sydots - show_numbers) /* avoid overreading coords */
      return 1000;
   if(windows == 2) /* avoid overreading fractal */
      if (0 <= x && x < xdots && 0 <= y && y < ydots)
         return 1000;
   return getcolor(x, y);
   }

void circleplot(int x, int y, int color)
{
   if (xAspect == 0)
      if (yAspect == 0)
         c_putcolor(x+xbase, y+ybase,color);
      else
         c_putcolor(x+xbase, (short)(ybase + (((long) y * (long) yAspect) >> 16)),color);
   else
      c_putcolor((int)(xbase + (((long) x * (long) xAspect) >> 16)), y+ybase, color);
}

void plot8(int x, int y, int color)
{
   circleplot(x,y,color);
   circleplot(-x,y,color);
   circleplot(x,-y,color);
   circleplot(-x,-y,color);
   circleplot(y,x,color);
   circleplot(-y,x,color);
   circleplot(y,-x,color);
   circleplot(-y,-x,color);
}

void circle(int radius, int color)
{
   int x,y,sum;

   x = 0;
   y = radius << 1;
   sum = 0;

   while (x <= y)
   {
      if ( !(x & 1) )   /* plot if x is even */
         plot8( x >> 1, (y+1) >> 1, color);
      sum += (x << 1) + 1;
      x++;
      if (sum > 0)
      {
         sum -= (y << 1) - 1;
         y--;
      }
   }
}


/*
 * MIIM section:
 *
 * Global variables and service functions used for computing
 * MIIM Julias will be grouped here (and shared by code in LORENZ.C)
 *
 */


 long   ListFront, ListBack, ListSize;  /* head, tail, size of MIIM Queue */
 long   lsize, lmax;                    /* how many in queue (now, ever) */
 int    maxhits = 1;
 int    OKtoMIIM;
 int    SecretExperimentalMode;
 float  luckyx = 0, luckyy = 0;

static void fillrect(int x, int y, int width, int depth, int color)
{
   /* fast version of fillrect */
   if(hasinverse == 0)
      return;
   memset(dstack, color % colors, width);
   while (depth-- > 0)
   {
      if(keypressed()) /* we could do this less often when in fast modes */
         return;
      putrow(x, y++, width, (char *)dstack);
   }
}

/*
 * Queue/Stack Section:
 *
 * Defines a buffer that can be used as a FIFO queue or LIFO stack.
 */

int
QueueEmpty()            /* True if NO points remain in queue */
{
   return (ListFront == ListBack);
}

#if 0 /* not used */
int
QueueFull()             /* True if room for NO more points in queue */
{
   return (((ListFront + 1) % ListSize) == ListBack);
}
#endif

int
QueueFullAlmost()       /* True if room for ONE more point in queue */
{
   return (((ListFront + 2) % ListSize) == ListBack);
}

void
ClearQueue()
{
   ListFront = ListBack = lsize = lmax = 0;
}


/*
 * Queue functions for MIIM julia:
 * move to JIIM.C when done
 */

int Init_Queue(unsigned long request)
{
   if (dotmode == 11)
   {
      static FCODE nono[] = "Don't try this in disk video mode, kids...\n";
      stopmsg(0, nono);
      ListSize = 0;
      return 0;
   }

#if 0
   if (xmmquery() && debugflag != 420)  /* use LARGEST extended mem */
      if ((largest = xmmlongest()) > request / 128)
         request   = (unsigned long) largest * 128L;
#endif

   for (ListSize = request; ListSize > 1024; ListSize /= 2)
      switch (common_startdisk(ListSize * 8, 1, 256))
      {
         case 0:                        /* success */
            ListFront = ListBack = 0;
            lsize = lmax = 0;
            return 1;
         case -1:
            continue;                   /* try smaller queue size */
         case -2:
            ListSize = 0;               /* cancelled by user      */
            return 0;
      }

   /* failed to get memory for MIIM Queue */
   ListSize = 0;
   return 0;
}

void
Free_Queue()
{
   enddisk();
   ListFront = ListBack = ListSize = lsize = lmax = 0;
}

int
PushLong(long x, long y)
{
   if (((ListFront + 1) % ListSize) != ListBack)
   {
      if (ToMemDisk(8*ListFront, sizeof(x), &x) &&
          ToMemDisk(8*ListFront +sizeof(x), sizeof(y), &y))
      {
         ListFront = (ListFront + 1) % ListSize;
         if (++lsize > lmax)
         {
            lmax   = lsize;
            luckyx = (float)x;
            luckyy = (float)y;
         }
         return 1;
      }
   }
   return 0;                    /* fail */
}

int
PushFloat(float x, float y)
{
   if (((ListFront + 1) % ListSize) != ListBack)
   {
      if (ToMemDisk(8*ListFront, sizeof(x), &x) &&
          ToMemDisk(8*ListFront +sizeof(x), sizeof(y), &y))
      {
         ListFront = (ListFront + 1) % ListSize;
         if (++lsize > lmax)
         {
            lmax   = lsize;
            luckyx = x;
            luckyy = y;
         }
         return 1;
      }
   }
   return 0;                    /* fail */
}

_CMPLX
PopFloat()
{
   _CMPLX pop;
   float  popx, popy;

   if (!QueueEmpty())
   {
      ListFront--;
      if (ListFront < 0)
          ListFront = ListSize - 1;
      if (FromMemDisk(8*ListFront, sizeof(popx), &popx) &&
          FromMemDisk(8*ListFront +sizeof(popx), sizeof(popy), &popy))
      {
         pop.x = popx;
         pop.y = popy;
         --lsize;
      }
      return pop;
   }
   pop.x = 0;
   pop.y = 0;
   return pop;
}

LCMPLX
PopLong()
{
   LCMPLX pop;

   if (!QueueEmpty())
   {
      ListFront--;
      if (ListFront < 0)
          ListFront = ListSize - 1;
      if (FromMemDisk(8*ListFront, sizeof(pop.x), &pop.x) &&
          FromMemDisk(8*ListFront +sizeof(pop.x), sizeof(pop.y), &pop.y))
         --lsize;
      return pop;
   }
   pop.x = 0;
   pop.y = 0;
   return pop;
}

int
EnQueueFloat(float x, float y)
{
   return PushFloat(x, y);
}

int
EnQueueLong(long x, long y)
{
   return PushLong(x, y);
}

_CMPLX
DeQueueFloat()
{
   _CMPLX out;
   float outx, outy;

   if (ListBack != ListFront)
   {
      if (FromMemDisk(8*ListBack, sizeof(outx), &outx) &&
          FromMemDisk(8*ListBack +sizeof(outx), sizeof(outy), &outy))
      {
         ListBack = (ListBack + 1) % ListSize;
         out.x = outx;
         out.y = outy;
         lsize--;
      }
      return out;
   }
   out.x = 0;
   out.y = 0;
   return out;
}

LCMPLX
DeQueueLong()
{
   LCMPLX out;
   out.x = 0;
   out.y = 0;

   if (ListBack != ListFront)
   {
      if (FromMemDisk(8*ListBack, sizeof(out.x), &out.x) &&
          FromMemDisk(8*ListBack +sizeof(out.x), sizeof(out.y), &out.y))
      {
         ListBack = (ListBack + 1) % ListSize;
         lsize--;
      }
      return out;
   }
   out.x = 0;
   out.y = 0;
   return out;
}



/*
 * End MIIM section;
 */

static void SaveRect(int x, int y, int width, int depth)
{
   char buff[MAXRECT];
   int  yoff;
   if(hasinverse == 0)
      return;
   /* first, do any de-allocationg */

   if (memory_handle != 0)
      MemoryRelease(memory_handle);

   /* allocate space and store the rect */

   memset(dstack, color_dark, width);
   if ((memory_handle = MemoryAlloc( (U16)width, (long)depth, FARMEM)) != 0)
   {
      Cursor_Hide();
      for (yoff=0; yoff<depth; yoff++)
      {
         getrow(x, y+yoff, width, buff);
         putrow(x, y+yoff, width, (char *)dstack);
         MoveToMemory((BYTE *)buff, (U16)width, 1L, (long)(yoff), memory_handle);
      }
      Cursor_Show();
   }
}


static void RestoreRect(int x, int y, int width, int depth)
{
   char buff[MAXRECT];
   int  yoff;
   if(hasinverse == 0)
      return;

    Cursor_Hide();
    for (yoff=0; yoff<depth; yoff++)
       {
          MoveFromMemory((BYTE *)buff, (U16)width, 1L, (long)(yoff), memory_handle);
          putrow(x, y+yoff, width, buff);
       }
    Cursor_Show();
}

/*
 * interface to FRACTINT
 */

_CMPLX SaveC = {-3000.0, -3000.0};

void Jiim(int which)         /* called by fractint */
{
   struct affine cvt;
   int exact = 0;
   int oldhelpmode;
   int count = 0;            /* coloring julia */
   static int mode = 0;      /* point, circle, ... */
   int       oldlookatmouse = lookatmouse;
   double cr, ci, r;
   int xfactor, yfactor;             /* aspect ratio          */

   int xoff, yoff;                   /* center of the window  */
   int x, y;
   int still, kbdchar= -1;

   long iter;
   int color;
   float zoom;
   int oldsxoffs, oldsyoffs;
   int savehasinverse;
   int (*oldcalctype)(void);
   int old_x, old_y;
   double aspect;
   static int randir = 0;
   static int rancnt = 0;
   int actively_computing = 1;
   int first_time = 1;
   int old_debugflag;

   old_debugflag = debugflag;
   /* must use standard fractal or be calcfroth */
   if(fractalspecific[fractype].calctype != StandardFractal
       && fractalspecific[fractype].calctype != calcfroth)
       return;
   oldhelpmode = helpmode;
   if(which == JIIM)
      helpmode = HELP_JIIM;
   else
   {
      helpmode = HELP_ORBITS;
      hasinverse = 1;

#if 0 /* I don't think this is needed any more */
      /* Earth to Chuck Ebbert - remove this code when your code supports
         my changes to PARSER.C */
      if(fractype == FFORMULA)
      {
         debugflag = 90;
      }
#endif
   }
   oldsxoffs = sxoffs;
   oldsyoffs = syoffs;
   oldcalctype = calctype;
   show_numbers = 0;
   using_jiim = 1;
   mem_init(strlocn, 10*1024);
   line_buff = newx(max(sxdots,sydots));
   aspect = ((double)xdots*3)/((double)ydots*4);  /* assumes 4:3 */
         actively_computing = 1;
   SetAspect(aspect);
   lookatmouse = 3;

   /* this code moved from just below to avoid boxx/strlocn conflict */
   if(which == ORBIT)
      (*PER_IMAGE)();
   else
      color = color_bright;
   /* end moved code */

   Cursor_Construct();

/*
 * MIIM code:
 * Grab far memory for Queue/Stack before SaveRect gets it.
 */
   OKtoMIIM  = 0;
   if (which == JIIM && debugflag != 300)
      OKtoMIIM = Init_Queue((long)8*1024); /* Queue Set-up Successful? */

   maxhits = 1;
   if (which == ORBIT)
      plot = c_putcolor;                /* for line with clipping */

/*
 * end MIIM code.
 */

   if (!video_scroll) {
      vesa_xres = sxdots;
      vesa_yres = sydots;
   }

   if(sxoffs != 0 || syoffs != 0) /* we're in view windows */
   {
      savehasinverse = hasinverse;
      hasinverse = 1;
      SaveRect(0,0,xdots,ydots);
      sxoffs = video_startx;
      syoffs = video_starty;
      RestoreRect(0,0,xdots,ydots);
      hasinverse = savehasinverse;
   }

   if(xdots == vesa_xres || ydots == vesa_yres ||
       vesa_xres-xdots < vesa_xres/3 ||
       vesa_yres-ydots < vesa_yres/3 ||
       xdots >= MAXRECT )
   {
      /* this mode puts orbit/julia in an overlapping window 1/3 the size of
         the physical screen */
      windows = 0; /* full screen or large view window */
      xd = vesa_xres / 3;
      yd = vesa_yres / 3;
      xc = video_startx + xd * 2;
      yc = video_starty + yd * 2;
      xoff = video_startx + xd * 5 / 2;
      yoff = video_starty + yd * 5 / 2;
   }
   else if(xdots > vesa_xres/3 && ydots > vesa_yres/3)
   {
      /* Julia/orbit and fractal don't overlap */
      windows = 1;
      xd = vesa_xres - xdots;
      yd = vesa_yres - ydots;
      xc = video_startx + xdots;
      yc = video_starty + ydots;
      xoff = xc + xd/2;
      yoff = yc + yd/2;

   }
   else
   {
      /* Julia/orbit takes whole screen */
      windows = 2;
      xd = vesa_xres;
      yd = vesa_yres;
      xc = video_startx;
      yc = video_starty;
      xoff = video_startx + xd/2;
      yoff = video_starty + yd/2;
   }

   xfactor = (int)(xd/5.33);
   yfactor = (int)(-yd/4);

   if(windows == 0)
      SaveRect(xc,yc,xd,yd);
   else if(windows == 2)  /* leave the fractal */
   {
      fillrect(xdots, yc, xd-xdots, yd, color_dark);
      fillrect(xc   , ydots, xdots, yd-ydots, color_dark);
   }
   else  /* blank whole window */
      fillrect(xc, yc, xd, yd, color_dark);

   setup_convert_to_screen(&cvt);

   /* reuse last location if inside window */
   col = (int)(cvt.a*SaveC.x + cvt.b*SaveC.y + cvt.e + .5);
   row = (int)(cvt.c*SaveC.x + cvt.d*SaveC.y + cvt.f + .5);
   if(col < 0 || col >= xdots ||
      row < 0 || row >= ydots)
   {
      cr = (xxmax + xxmin) / 2.0;
      ci = (yymax + yymin) / 2.0;
   }
   else
   {
      cr = SaveC.x;
      ci = SaveC.y;
   }

   old_x = old_y = -1;

   col = (int)(cvt.a*cr + cvt.b*ci + cvt.e + .5);
   row = (int)(cvt.c*cr + cvt.d*ci + cvt.f + .5);

   /* possible extraseg arrays have been trashed, so set up again */
   if(integerfractal)
      fill_lx_array();
   else
      fill_dx_array();

   Cursor_SetPos(col, row);
   Cursor_Show();
   color = color_bright;

   iter = 1;
   still = 1;
   zoom = 1;

#ifdef XFRACT
   Cursor_StartMouseTracking();
#endif

   while (still)
   {
      int dcol, drow;
      if (actively_computing) {
          Cursor_CheckBlink();
      } else {
          Cursor_WaitKey();
      }
      if(keypressed() || first_time) /* prevent burning up UNIX CPU */
      {
         first_time = 0;
         while(keypressed())
         {
            Cursor_WaitKey();
            kbdchar = getakey();

            dcol = drow = 0;
            xcjul = BIG;
            ycjul = BIG;
            switch (kbdchar)
            {
            case 1143:    /* ctrl - keypad 5 */
            case 1076:    /* keypad 5        */
               break;     /* do nothing */
            case CTL_PAGE_UP:
               dcol = 4;
               drow = -4;
               break;
            case CTL_PAGE_DOWN:
               dcol = 4;
               drow = 4;
               break;
            case CTL_HOME:
               dcol = -4;
               drow = -4;
               break;
            case CTL_END:
               dcol = -4;
               drow = 4;
               break;
            case PAGE_UP:
               dcol = 1;
               drow = -1;
               break;
            case PAGE_DOWN:
               dcol = 1;
               drow = 1;
               break;
            case HOME:
               dcol = -1;
               drow = -1;
               break;
            case END:
               dcol = -1;
               drow = 1;
               break;
            case UP_ARROW:
               drow = -1;
               break;
            case DOWN_ARROW:
               drow = 1;
               break;
            case LEFT_ARROW:
               dcol = -1;
               break;
            case RIGHT_ARROW:
               dcol = 1;
               break;
            case UP_ARROW_2:
               drow = -4;
               break;
            case DOWN_ARROW_2:
               drow = 4;
               break;
            case LEFT_ARROW_2:
               dcol = -4;
               break;
            case RIGHT_ARROW_2:
               dcol = 4;
               break;
            case 'z':
            case 'Z':
               zoom = (float)1.0;
               break;
            case '<':
            case ',':
               zoom /= (float)1.15;
               break;
            case '>':
            case '.':
               zoom *= (float)1.15;
               break;
            case SPACE:
               xcjul = cr;
               ycjul = ci;
               goto finish;
               /* break; */
            case 'c':   /* circle toggle */
            case 'C':   /* circle toggle */
               mode = mode ^ 1;
               break;
            case 'l':
            case 'L':
               mode = mode ^ 2;
               break;
            case 'n':
            case 'N':
               show_numbers = 8 - show_numbers;
               if(windows == 0 && show_numbers == 0)
               {
                  Cursor_Hide();
                  cleartempmsg();
                  Cursor_Show();
               }
               break;
            case 'p':
            case 'P':
               get_a_number(&cr,&ci);
               exact = 1;
               col = (int)(cvt.a*cr + cvt.b*ci + cvt.e + .5);
               row = (int)(cvt.c*cr + cvt.d*ci + cvt.f + .5);
               dcol = drow = 0;
               break;
            case 'h':   /* hide fractal toggle */
            case 'H':   /* hide fractal toggle */
               if(windows == 2)
                  windows = 3;
               else if(windows == 3 && xd == vesa_xres)
               {
                  RestoreRect(video_startx, video_starty, xdots, ydots);
                  windows = 2;
               }
               break;
#ifdef XFRACT
            case ENTER:
                break;
#endif
            case '0':
            case '1':
            case '2':
/*          case '3': */  /* don't use '3', it's already meaningful */
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
               if (which == JIIM)
               {
                  SecretExperimentalMode = kbdchar - '0';
                  break;
               }
            default:
               still = 0;
/*             ismand = (short)(1 - ismand); */
            }  /* switch */
            if(kbdchar == 's' || kbdchar == 'S')
               goto finish;
            if(dcol > 0 || drow > 0)
               exact = 0;
            col += dcol;
            row += drow;
#ifdef XFRACT
            if (kbdchar == ENTER) {
                /* We want to use the position of the cursor */
                exact=0;
                col = Cursor_GetX();
                row = Cursor_GetY();
            }
#endif

            /* keep cursor in logical screen */
           if(col >= xdots) {
              col = xdots -1; exact = 0;
           }
           if(row >= ydots) {
              row = ydots -1; exact = 0;
           }
           if(col < 0) {
              col = 0; exact = 0;
           }
           if(row < 0) {
              row = 0; exact = 0;
           }

            Cursor_SetPos(col,row);
         }  /* end while (keypressed) */

         if(exact == 0)
         {
            if(integerfractal)
            {
               cr = lxpixel();
               ci = lypixel();
               cr /= (1L<<bitshift);
               ci /= (1L<<bitshift);
            }
            else
            {
               cr = dxpixel();
               ci = dypixel();
            }
         }
         actively_computing = 1;
         if(show_numbers) /* write coordinates on screen */
         {
            char str[41];
            sprintf(str,"%16.14f %16.14f %3d",cr,ci,getcolor(col,row));
            if(windows == 0)
            {
               /* show temp msg will clear self if new msg is a
                  different length - pad to length 40*/
               while((int)strlen(str) < 40)
                  strcat(str," ");
               str[40] = 0;
               Cursor_Hide();
               actively_computing = 1;
               showtempmsg(str);
               Cursor_Show();
            }
            else
               displays(5, vesa_yres-show_numbers, WHITE, BLACK, str,strlen(str));
         }
         iter = 1;
         old.x = old.y = lold.x = lold.y = 0;
         SaveC.x = init.x =  cr;
         SaveC.y = init.y =  ci;
         linit.x = (long)(init.x*fudge);
         linit.y = (long)(init.y*fudge);

         old_x = old_y = -1;
/*
 * MIIM code:
 * compute fixed points and use them as starting points of JIIM
 */
         if (which == JIIM && OKtoMIIM)
         {
            _CMPLX f1, f2, Sqrt;        /* Fixed points of Julia */

            Sqrt = ComplexSqrtFloat(1 - 4 * cr, -4 * ci);
            f1.x = (1 + Sqrt.x) / 2;
            f2.x = (1 - Sqrt.x) / 2;
            f1.y =  Sqrt.y / 2;
            f2.y = -Sqrt.y / 2;

            ClearQueue();
            maxhits = 1;
            EnQueueFloat((float)f1.x, (float)f1.y);
            EnQueueFloat((float)f2.x, (float)f2.y);
         }
/*
 * End MIIM code.
 */
         if(which == ORBIT)
         {
           PER_PIXEL();
         }  
         /* move window if bumped */
         if(windows==0 && col>xc && col < xc+xd && row>yc && row < yc+yd)
         {
            RestoreRect(xc,yc,xd,yd);
            if (xc == video_startx + xd*2)
               xc = video_startx + 2;
            else
               xc = video_startx + xd*2;
            xoff = xc + xd /  2;
            SaveRect(xc,yc,xd,yd);
         }
         if(windows == 2)
         {
            fillrect(xdots, yc, xd-xdots, yd-show_numbers, color_dark);
            fillrect(xc   , ydots, xdots, yd-ydots-show_numbers, color_dark);
         }
         else
            fillrect(xc, yc, xd, yd, color_dark);

      } /* end if (keypressed) */

      if(which == JIIM)
      {
         if(hasinverse == 0)
            continue;
/*
 * MIIM code:
 * If we have MIIM queue allocated, then use MIIM method.
 */
         if (OKtoMIIM)
         {
            if (QueueEmpty())
            {
               if (maxhits < colors - 1 && maxhits < 5 &&
                  (luckyx != 0.0 || luckyy != 0.0))
               {
                  int i;

                  lsize  = lmax   = 0;
                  old.x  = new.x  = luckyx;
                  old.y  = new.y  = luckyy;
                  luckyx = luckyy = (float)0.0;
                  for (i=0; i<199; i++)
                  {
                     old = ComplexSqrtFloat(old.x - cr, old.y - ci);
                     new = ComplexSqrtFloat(new.x - cr, new.y - ci);
                     EnQueueFloat( (float)new.x,  (float)new.y);
                     EnQueueFloat((float)-old.x, (float)-old.y);
                  }
                  maxhits++;
               }
               else
                  continue;             /* loop while (still) */
            }

            old = DeQueueFloat();

#if 0 /* try a different new method */
            if (lsize < (lmax / 8) && maxhits < 5)      /* NEW METHOD */
               if (maxhits < colors - 1)
                   maxhits++;
#endif
            x = (int)(old.x * xfactor * zoom + xoff);
            y = (int)(old.y * yfactor * zoom + yoff);
            color = c_getcolor(x, y);
            if (color < maxhits)
            {
               c_putcolor(x, y, color + 1);
               new = ComplexSqrtFloat(old.x - cr, old.y - ci);
               EnQueueFloat( (float)new.x,  (float)new.y);
               EnQueueFloat((float)-new.x, (float)-new.y);
            }
         }
         else
         {
/*
 * end Msnyder code, commence if not MIIM code.
 */
         old.x -= cr;
         old.y -= ci;
         r = old.x*old.x + old.y*old.y;
         if(r > 10.0)
         {
             old.x = old.y = 0.0; /* avoids math error */
             iter = 1;
             r = 0;
         }
         iter++;
         color = ((count++)>>5)%colors; /* chg color every 32 pts */
         if(color==0)
          color = 1;

/*       r = sqrt(old.x*old.x + old.y*old.y); calculated above */
         r = sqrt(r);
         new.x = sqrt(fabs((r + old.x)/2));
         if (old.y < 0)
            new.x = -new.x;

         new.y = sqrt(fabs((r - old.x)/2));


         switch (SecretExperimentalMode) {
            case 0:                     /* unmodified random walk */
            default:
                if (rand() % 2)
                {
                   new.x = -new.x;
                   new.y = -new.y;
                }
                x = (int)(new.x * xfactor * zoom + xoff);
                y = (int)(new.y * yfactor * zoom + yoff);
                break;
            case 1:                     /* always go one direction */
                if (SaveC.y < 0)
                {
                   new.x = -new.x;
                   new.y = -new.y;
                }
                x = (int)(new.x * xfactor * zoom + xoff);
                y = (int)(new.y * yfactor * zoom + yoff);
                break;
            case 2:                     /* go one dir, draw the other */
                if (SaveC.y < 0)
                {
                   new.x = -new.x;
                   new.y = -new.y;
                }
                x = (int)(-new.x * xfactor * zoom + xoff);
                y = (int)(-new.y * yfactor * zoom + yoff);
                break;
            case 4:                     /* go negative if max color */
                x = (int)(new.x * xfactor * zoom + xoff);
                y = (int)(new.y * yfactor * zoom + yoff);
                if (c_getcolor(x, y) == colors - 1)
                {
                   new.x = -new.x;
                   new.y = -new.y;
                   x = (int)(new.x * xfactor * zoom + xoff);
                   y = (int)(new.y * yfactor * zoom + yoff);
                }
                break;
            case 5:                     /* go positive if max color */
                new.x = -new.x;
                new.y = -new.y;
                x = (int)(new.x * xfactor * zoom + xoff);
                y = (int)(new.y * yfactor * zoom + yoff);
                if (c_getcolor(x, y) == colors - 1)
                {
                   x = (int)(new.x * xfactor * zoom + xoff);
                   y = (int)(new.y * yfactor * zoom + yoff);
                }
                break;
            case 7:
                if (SaveC.y < 0)
                {
                   new.x = -new.x;
                   new.y = -new.y;
                }
                x = (int)(-new.x * xfactor * zoom + xoff);
                y = (int)(-new.y * yfactor * zoom + yoff);
                if(iter > 10)
                {
                   if(mode == 0)                        /* pixels  */
                      c_putcolor(x, y, color);
                   else if (mode & 1)            /* circles */
                   {
                      xbase = x;
                      ybase = y;
                      circle((int)(zoom*(xd >> 1)/iter),color);
                   }
                   if ((mode & 2) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
                   {
                      draw_line(x, y, old_x, old_y, color);
                   }
                   old_x = x;
                   old_y = y;
                }
                x = (int)(new.x * xfactor * zoom + xoff);
                y = (int)(new.y * yfactor * zoom + yoff);
                break;
            case 8:                     /* go in long zig zags */
                if (rancnt >= 300)
                    rancnt = -300;
                if (rancnt < 0)
                {
                    new.x = -new.x;
                    new.y = -new.y;
                }
                x = (int)(new.x * xfactor * zoom + xoff);
                y = (int)(new.y * yfactor * zoom + yoff);
                break;
            case 9:                     /* "random run" */
                switch (randir) {
                    case 0:             /* go random direction for a while */
                        if (rand() % 2)
                        {
                            new.x = -new.x;
                            new.y = -new.y;
                        }
                        if (++rancnt > 1024)
                        {
                            rancnt = 0;
                            if (rand() % 2)
                                randir =  1;
                            else
                                randir = -1;
                        }
                        break;
                    case 1:             /* now go negative dir for a while */
                        new.x = -new.x;
                        new.y = -new.y;
                        /* fall through */
                    case -1:            /* now go positive dir for a while */
                        if (++rancnt > 512)
                            randir = rancnt = 0;
                        break;
                }
                x = (int)(new.x * xfactor * zoom + xoff);
                y = (int)(new.y * yfactor * zoom + yoff);
                break;
         } /* end switch SecretMode (sorry about the indentation) */
         } /* end if not MIIM */
      }
      else /* orbits */
      {
         if(iter < maxit)
         {
            color = (int)iter%colors;
            if(integerfractal)
            {
               old.x = lold.x; old.x /= fudge;
               old.y = lold.y; old.y /= fudge;
            }
            x = (int)((old.x - init.x) * xfactor * 3 * zoom + xoff);
            y = (int)((old.y - init.y) * yfactor * 3 * zoom + yoff);
            if((*ORBITCALC)())
               iter = maxit;
            else
               iter++;
         }
         else
         {
            x = y = -1;
            actively_computing = 0;
         }
      }
      if(which == ORBIT || iter > 10)
      {
         if(mode == 0)                  /* pixels  */
            c_putcolor(x, y, color);
         else if (mode & 1)            /* circles */
         {
            xbase = x;
            ybase = y;
            circle((int)(zoom*(xd >> 1)/iter),color);
         }
         if ((mode & 2) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
         {
            draw_line(x, y, old_x, old_y, color);
         }
         old_x = x;
         old_y = y;
      }
      old = new;
      lold = lnew;
   } /* end while(still) */
finish:

/*
 * Msnyder code:
 * free MIIM queue
 */

   Free_Queue();
/*
 * end Msnyder code.
 */

   if(kbdchar != 's'&& kbdchar != 'S')
   {
      Cursor_Hide();
      if(windows == 0)
         RestoreRect(xc,yc,xd,yd);
      else if(windows >= 2 )
      {
         if(windows == 2)
         {
            fillrect(xdots, yc, xd-xdots, yd, color_dark);
            fillrect(xc   , ydots, xdots, yd-ydots, color_dark);
         }
         else
            fillrect(xc, yc, xd, yd, color_dark);
         if(windows == 3 && xd == vesa_xres) /* unhide */
         {
            RestoreRect(0, 0, xdots, ydots);
            windows = 2;
         }
         Cursor_Hide();
         savehasinverse = hasinverse;
         hasinverse = 1;
         SaveRect(0,0,xdots,ydots);
         sxoffs = oldsxoffs;
         syoffs = oldsyoffs;
         RestoreRect(0,0,xdots,ydots);
         hasinverse = savehasinverse;
      }
   }
   Cursor_Destroy();
#ifdef XFRACT
   Cursor_EndMouseTracking();
#endif
   delete(line_buff);

   if (memory_handle != 0) {
      MemoryRelease(memory_handle);
      memory_handle = 0;
   }
#if 0
   if (memory)                  /* done with memory, free it */
   {
      farmemfree(memory);
      memory = NULL;
   }
#endif

   lookatmouse = oldlookatmouse;
   using_jiim = 0;
   calctype = oldcalctype;
   debugflag = old_debugflag; /* yo Chuck! */
   helpmode = oldhelpmode;
   if(kbdchar == 's' || kbdchar == 'S')
   {
      viewwindow = viewxdots = viewydots = 0;
      viewreduction = (float)4.2;
      viewcrop = 1;
      finalaspectratio = screenaspect;
      xdots = sxdots;
      ydots = sydots;
      dxsize = xdots - 1;
      dysize = ydots - 1;
      sxoffs = 0;
      syoffs = 0;
      freetempmsg();
   }
   else
      cleartempmsg();
   if (file != NULL)
      {
      fclose(file);
      file = NULL;
      dir_remove(tempdir,scrnfile);
      }
   show_numbers = 0;
   ungetakey(kbdchar);

   if (curfractalspecific->calctype == calcfroth)
      froth_cleanup();
}
