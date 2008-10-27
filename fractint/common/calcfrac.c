/*
CALCFRAC.C contains the high level ("engine") code for calculating the
fractal images (well, SOMEBODY had to do it!).
Original author Tim Wegner, but just about ALL the authors have contributed
SOME code to this routine at one time or another, or contributed to one of
the many massive restructurings.
The following modules work very closely with CALCFRAC.C:
  FRACTALS.C    the fractal-specific code for escape-time fractals.
  FRACSUBR.C    assorted subroutines belonging mainly to calcfrac.
  CALCMAND.ASM  fast Mandelbrot/Julia integer implementation
Additional fractal-specific modules are also invoked from CALCFRAC:
  LORENZ.C      engine level and fractal specific code for attractors.
  JB.C          julibrot logic
  PARSER.C      formula fractals
  and more
 -------------------------------------------------------------------- */

#include <string.h>
#include <limits.h>
#include <time.h>
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "targa_lc.h"


/* routines in this module      */
static void perform_worklist(void);
static int  OneOrTwoPass(void);
static int  _fastcall StandardCalc(int);
static int  _fastcall potential(double,long);
static void decomposition(void);
static int  bound_trace_main(void);
static void step_col_row(void);
static int  solidguess(void);
static int  _fastcall guessrow(int,int,int);
static void _fastcall plotblock(int,int,int,int);
static void _fastcall setsymmetry(int,int);
static int  _fastcall xsym_split(int,int);
static int  _fastcall ysym_split(int,int);
static void _fastcall puttruecolor_disk(int,int,int);
static int diffusion_engine (void);
static int sticky_orbits(void);

/**CJLT new function prototypes: */
static int tesseral(void);
static int _fastcall tesschkcol(int,int,int);
static int _fastcall tesschkrow(int,int,int);
static int _fastcall tesscol(int,int,int);
static int _fastcall tessrow(int,int,int);

/* new drawing method by HB */
static int diffusion_scan(void);

/* lookup tables to avoid too much bit fiddling : */
char far dif_la[] = {
0, 8, 0, 8,4,12,4,12,0, 8, 0, 8,4,12,4,12, 2,10, 2,10,6,14,6,14,2,10,
2,10, 6,14,6,14,0, 8,0, 8, 4,12,4,12,0, 8, 0, 8, 4,12,4,12,2,10,2,10,
6,14, 6,14,2,10,2,10,6,14, 6,14,1, 9,1, 9, 5,13, 5,13,1, 9,1, 9,5,13,
5,13, 3,11,3,11,7,15,7,15, 3,11,3,11,7,15, 7,15, 1, 9,1, 9,5,13,5,13,
1, 9, 1, 9,5,13,5,13,3,11, 3,11,7,15,7,15, 3,11, 3,11,7,15,7,15,0, 8,
0, 8, 4,12,4,12,0, 8,0, 8, 4,12,4,12,2,10, 2,10, 6,14,6,14,2,10,2,10,
6,14, 6,14,0, 8,0, 8,4,12, 4,12,0, 8,0, 8, 4,12, 4,12,2,10,2,10,6,14,
6,14, 2,10,2,10,6,14,6,14, 1, 9,1, 9,5,13, 5,13, 1, 9,1, 9,5,13,5,13,
3,11, 3,11,7,15,7,15,3,11, 3,11,7,15,7,15, 1, 9, 1, 9,5,13,5,13,1, 9,
1, 9, 5,13,5,13,3,11,3,11, 7,15,7,15,3,11, 3,11, 7,15,7,15
};

char far dif_lb[] = {
 0, 8, 8, 0, 4,12,12, 4, 4,12,12, 4, 8, 0, 0, 8, 2,10,10, 2, 6,14,14,
 6, 6,14,14, 6,10, 2, 2,10, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2,
 2,10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 1, 9, 9, 1, 5,
13,13, 5, 5,13,13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,
11, 3, 3,11, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13,
 5, 9, 1, 1, 9, 9, 1, 1, 9,13, 5, 5,13, 1, 9, 9, 1, 5,13,13, 5, 5,13,
13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 3,
11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13, 5, 9, 1, 1, 9,
 9, 1, 1, 9,13, 5, 5,13, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2, 2,
10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 4,12,12, 4, 8, 0,
 0, 8, 8, 0, 0, 8,12, 4, 4,12, 6,14,14, 6,10, 2, 2,10,10, 2, 2,10,14,
 6, 6,14
};

/* added for testing autologmap() */
static long autologmap(void);

_LCMPLX linitorbit;
long lmagnitud, llimit, llimit2, lclosenuff, l16triglim;
_CMPLX init,tmp,old,new,saved;
int color;
long coloriter, oldcoloriter, realcoloriter;
int row, col, passes;
int iterations, invert;
double f_radius,f_xcenter, f_ycenter; /* for inversion */
void (_fastcall *putcolor)(int,int,int) = putcolor_a;
void (_fastcall *plot)(int,int,int) = putcolor_a;
typedef void (_fastcall *PLOTC)(int,int,int);
typedef void (_fastcall *GETC)(int,int,int);

double magnitude, rqlim, rqlim2, rqlim_save;
int no_mag_calc = 0;
int use_old_period = 0;
int use_old_distest = 0;
int old_demm_colors = 0;
int (*calctype)(void);
int (*calctypetmp)(void);
int quick_calc = 0;
double closeprox = 0.01;

double closenuff;
int pixelpi; /* value of pi in pixels */
unsigned long lm;               /* magnitude limit (CALCMAND) */

/* ORBIT variables */
int     show_orbit;                     /* flag to turn on and off */
int     orbit_ptr;                      /* pointer into save_orbit array */
int far *save_orbit;                    /* array to save orbit values */
int     orbit_color=15;                 /* XOR color */

int     ixstart, ixstop, iystart, iystop;       /* start, stop here */
int     symmetry;          /* symmetry flag */
int     reset_periodicity; /* nonzero if escape time pixel rtn to reset */
int     kbdcount, max_kbdcount;    /* avoids checking keyboard too often */

U16 resume_info = 0;                    /* handle to resume info if allocated */
int resuming;                           /* nonzero if resuming after interrupt */
int num_worklist;                       /* resume worklist for standard engine */
WORKLIST worklist[MAXCALCWORK];
int xxstart,xxstop,xxbegin;             /* these are same as worklist, */
int yystart,yystop,yybegin;             /* declared as separate items  */
int workpass,worksym;                   /* for the sake of calcmand    */

VOIDFARPTR typespecific_workarea = NULL;

static double dem_delta, dem_width;     /* distance estimator variables */
static double dem_toobig;
static int dem_mandel;
#define DEM_BAILOUT 535.5  /* (pb: not sure if this is special or arbitrary) */

/* variables which must be visible for tab_display */
int got_status; /* -1 if not, 0 for 1or2pass, 1 for ssg, */
	      /* 2 for btm, 3 for 3d, 4 for tesseral, 5 for diffusion_scan */
              /* 6 for orbits */
int curpass,totpasses;
int currow,curcol;

/* static vars for diffusion scan */
unsigned bits=0; 		/* number of bits in the counter */
unsigned long dif_counter; 	/* the diffusion counter */
unsigned long dif_limit; 	/* the diffusion counter */

/* static vars for solidguess & its subroutines */
char three_pass;
static int maxblock,halfblock;
static int guessplot;                   /* paint 1st pass row at a time?   */
static int right_guess,bottom_guess;
#define maxyblk 7    /* maxxblk*maxyblk*2 <= 4096, the size of "prefix" */
#define maxxblk 202  /* each maxnblk is oversize by 2 for a "border" */
                     /* maxxblk defn must match fracsubr.c */
/* next has a skip bit for each maxblock unit;
   1st pass sets bit  [1]... off only if block's contents guessed;
   at end of 1st pass [0]... bits are set if any surrounding block not guessed;
   bits are numbered [..][y/16+1][x+1]&(1<<(y&15)) */

/* Original array */
/* extern unsigned int prefix[2][maxyblk][maxxblk]; */

typedef int (*TPREFIX)[2][maxyblk][maxxblk];
#define tprefix   (*((TPREFIX)prefix))

/* size of next puts a limit of MAXPIXELS pixels across on solid guessing logic */
#ifdef XFRACT
BYTE dstack[4096];              /* common temp, two put_line calls */
unsigned int prefix[2][maxyblk][maxxblk]; /* common temp */
#endif

int nxtscreenflag; /* for cellular next screen generation */
int     attractors;                 /* number of finite attractors  */
_CMPLX  attr[N_ATTR];       /* finite attractor vals (f.p)  */
_LCMPLX lattr[N_ATTR];      /* finite attractor vals (int)  */
int    attrperiod[N_ATTR];          /* period of the finite attractor */

/***** vars for new btm *****/
enum direction {North,East,South,West};
enum direction going_to;
int trail_row, trail_col;

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#ifndef lsqr
#define lsqr(x) (multiply((x),(x),bitshift))
#endif

/* -------------------------------------------------------------------- */
/*              These variables are external for speed's sake only      */
/* -------------------------------------------------------------------- */

int periodicitycheck;

/* For periodicity testing, only in StandardFractal() */
int nextsavedincr;
long firstsavedand;

static BYTE *savedots = NULL;
static BYTE *fillbuff;
static int savedotslen;
static int showdotcolor;
int atan_colors = 180;

static int showdot_width = 0;
#define SAVE    1
#define RESTORE 2

#define JUST_A_POINT 0
#define LOWER_RIGHT  1
#define UPPER_RIGHT  2
#define LOWER_LEFT   3
#define UPPER_LEFT   4

/* FMODTEST routine. */
/* Makes the test condition for the FMOD coloring type
   that of the current bailout method. 'or' and 'and' 
   methods are not used - in these cases a normal 
   modulus test is used                              */

double fmodtest(void)
{
   double result;
   if (inside==FMODI && save_release <= 2000) /* for backwards compatibility */
   {
      if (magnitude == 0.0 || no_mag_calc == 0 || integerfractal)
         result=sqr(new.x)+sqr(new.y);
      else
         result=magnitude; /* don't recalculate */
      return (result);
   }

   switch(bailoutest)
   {
   case (Mod):
      {
      if (magnitude == 0.0 || no_mag_calc == 0 || integerfractal)
         result=sqr(new.x)+sqr(new.y);
      else
         result=magnitude; /* don't recalculate */
      }break;
   case (Real):
      {
      result=sqr(new.x);
      }break;
   case (Imag):
      {
      result=sqr(new.y);
      }break;
   case (Or):
      {
      double tmpx, tmpy;
      if ((tmpx = sqr(new.x)) > (tmpy = sqr(new.y)))
         result=tmpx;
      else
         result=tmpy;
      }break;
   case (Manh):
      {
      result=sqr(fabs(new.x)+fabs(new.y));
      }break;
   case (Manr):
      {
      result=sqr(new.x+new.y);
      }break;
   default:
      {
      result=sqr(new.x)+sqr(new.y);
      }break;
   }
   return (result);
}

/*
   The sym_fill_line() routine was pulled out of the boundary tracing 
   code for re-use with showdot. It's purpose is to fill a line with a
   solid color. This assumes that BYTE *str is already filled
   with the color. The routine does write the line using symmetry
   in all cases, however the symmetry logic assumes that the line
   is one color; it is not general enough to handle a row of
   pixels of different colors. 
*/  
static void sym_fill_line(int row, int left, int right, BYTE *str)
{
   int i,j,k, length;
   length = right-left+1;
   put_line(row,left,right,str);
   /* here's where all the symmetry goes */
   if (plot == putcolor)
      kbdcount -= length >> 4; /* seems like a reasonable value */
   else if (plot == symplot2) /* X-axis symmetry */
   {
      i = yystop-(row-yystart);
      if (i > iystop && i < ydots)
      {
         put_line(i,left,right,str);
         kbdcount -= length >> 3;
      }   
   }
   else if (plot == symplot2Y) /* Y-axis symmetry */
   {
      put_line(row,xxstop-(right-xxstart),xxstop-(left-xxstart),str);
      kbdcount -= length >> 3;
   }
   else if (plot == symplot2J)  /* Origin symmetry */
   {
      i = yystop-(row-yystart);
      j = min(xxstop-(right-xxstart),xdots-1);
      k = min(xxstop-(left -xxstart),xdots-1);
      if (i > iystop && i < ydots && j <= k)
         put_line(i,j,k,str);
      kbdcount -= length >> 3;
   }
   else if (plot == symplot4) /* X-axis and Y-axis symmetry */
   {
      i = yystop-(row-yystart);
      j = min(xxstop-(right-xxstart),xdots-1);
      k = min(xxstop-(left -xxstart),xdots-1);
      if (i > iystop && i < ydots)
      {
         put_line(i,left,right,str);
         if(j <= k)
            put_line(i,j,k,str);
      }
      if(j <= k)
         put_line(row,j,k,str);
      kbdcount -= length >> 2;
   }
   else    /* cheap and easy way out */
   {
      for (i = left; i <= right; i++)  /* DG */
         (*plot)(i,row,str[i-left]);
      kbdcount -= length >> 1;
   }
}

/*
  The sym_put_line() routine is the symmetry-aware version of put_line().
  It only works efficiently in the no symmetry or XAXIS symmetry case,
  otherwise it just writes the pixels one-by-one.
*/    
static void sym_put_line(int row, int left, int right, BYTE *str)
{
   int length,i;
   length = right-left+1;
   put_line(row,left,right,str);
   if (plot == putcolor)
      kbdcount -= length >> 4; /* seems like a reasonable value */
   else if (plot == symplot2) /* X-axis symmetry */
   {
      i = yystop-(row-yystart);
      if (i > iystop && i < ydots)
         put_line(i,left,right,str);
      kbdcount -= length >> 3;
   }
   else
   {
      for (i = left; i <= right; i++)  /* DG */
         (*plot)(i,row,str[i-left]);
      kbdcount -= length >> 1;
   }
}

void showdotsaverestore(int startx, int stopx, int starty, int stopy, int direction, int action)
{
   int j,ct;
   ct = 0;
   if(direction != JUST_A_POINT)
   {
      if(savedots == NULL)
      {
         stopmsg(0,"savedots NULL");
         exit(0);
      }
      if(fillbuff == NULL)
      {
         stopmsg(0,"fillbuff NULL");
         exit(0);
      }
   }
   switch(direction)
   {
      case LOWER_RIGHT:
         for(j=starty;   j<=stopy; startx++,j++)
         {
            if(action==SAVE)
            {
               get_line(j,startx,stopx,savedots+ct);
               sym_fill_line(j,startx,stopx,fillbuff);
            }
            else
               sym_put_line(j,startx,stopx,savedots+ct);
            ct += stopx-startx+1;
         }
         break;
      case UPPER_RIGHT:
         for(j=starty;   j>=stopy; startx++,j--)
         {
            if(action==SAVE)
            {
               get_line(j,startx,stopx,savedots+ct);
               sym_fill_line(j,startx,stopx,fillbuff);
            }
            else
               sym_put_line(j,startx,stopx,savedots+ct);
            ct += stopx-startx+1;
         }
         break;
      case LOWER_LEFT:
         for(j=starty; j<=stopy; stopx--,j++)
         {
            if(action==SAVE)
            {
               get_line(j,startx,stopx,savedots+ct);
               sym_fill_line(j,startx,stopx,fillbuff);
            }
            else
               sym_put_line(j,startx,stopx,savedots+ct);
            ct += stopx-startx+1;
         }
         break;
      case UPPER_LEFT:
         for(j=starty; j>=stopy; stopx--,j--)
         {
            if(action==SAVE)
            {
               get_line(j,startx,stopx,savedots+ct);
               sym_fill_line(j,startx,stopx,fillbuff);
            }
            else
               sym_put_line(j,startx,stopx,savedots+ct);
            ct += stopx-startx+1;
         }
         break;
   }
   if(action == SAVE)
      (*plot) (col,row, showdotcolor); 
}

int calctypeshowdot(void)
{
   int out, startx, starty, stopx, stopy, direction, width;
   direction = JUST_A_POINT;   
   startx = stopx = col;
   starty = stopy = row;
   width = showdot_width+1;
   if(width > 0) {
      if(col+width <= ixstop && row+width <= iystop)
      {
         /* preferred showdot shape */
         direction = UPPER_LEFT;
         startx = col;
         stopx  = col+width;
         starty = row+width;
         stopy  = row+1;
      }
      else if(col-width >= ixstart && row+width <= iystop)
      {
         /* second choice */
         direction = UPPER_RIGHT;
         startx = col-width;
         stopx  = col;
         starty = row+width;
         stopy  = row+1;
      }
      else if(col-width >= ixstart && row-width >= iystart)
      {
         direction = LOWER_RIGHT;
         startx = col-width;
         stopx  = col;
         starty = row-width;
         stopy  = row-1;
      }
      else if(col+width <= ixstop && row-width >= iystart)
      {
         direction = LOWER_LEFT;
         startx = col;
         stopx  = col+width;
         starty = row-width;
         stopy  = row-1;
      }
   }
   showdotsaverestore(startx, stopx, starty, stopy, direction, SAVE);
   if(orbit_delay > 0) sleepms(orbit_delay);
   out = (*calctypetmp)();
   showdotsaverestore(startx, stopx, starty, stopy, direction, RESTORE);
   return(out);
}

/* use top of extraseg for LogTable if room */
int logtable_in_extra_ok(void)
{
   if(((2L*(long)(xdots+ydots)*sizeof(double)+MaxLTSize+1) < (1L<<16))
      && (bf_math==0)
      && (fractype != FORMULA)
      && (fractype != FFORMULA))
      return(1);
   else
      return(0);
}

/******* calcfract - the top level routine for generating an image *******/

#if (_MSC_VER >= 700)
#pragma code_seg ("calcfra1_text")     /* place following in an overlay */
#endif

int calcfract(void)
{
   matherr_ct = 0;
   attractors = 0;          /* default to no known finite attractors  */
   display3d = 0;
   basin = 0;
   /* added yet another level of indirection to putcolor!!! TW */
   putcolor = putcolor_a;
   if (istruecolor && truemode)
      /* Have to force passes=1 */
      usr_stdcalcmode = stdcalcmode = '1';
   if(truecolor)
   {
      check_writefile(light_name, ".tga");
      if(startdisk1(light_name,NULL,0)==0)
      {
         /* Have to force passes=1 */
         usr_stdcalcmode = stdcalcmode = '1';
         putcolor = puttruecolor_disk;
      }
      else
         truecolor = 0;
   }
   if(!use_grid || (xdots > OLDMAXPIXELS || ydots > OLDMAXPIXELS))
   {
      if (usr_stdcalcmode != 'o')
         usr_stdcalcmode = stdcalcmode = '1';
   }
      
   init_misc();  /* set up some variables in parser.c */
   reset_clock();
   
   /* following delta values useful only for types with rotation disabled */
   /* currently used only by bifurcation */
   if (integerfractal)
      distest = 0;
   parm.x   = param[0];
   parm.y   = param[1];
   parm2.x  = param[2];
   parm2.y  = param[3];

   if (LogFlag && colors < 16) {
      static FCODE msg[]={"Need at least 16 colors to use logmap"};
      stopmsg(0,msg);
      LogFlag = 0;
      }

   if (use_old_period == 1) {
      nextsavedincr = 1;
      firstsavedand = 1;
   }
   else {
      nextsavedincr = (int)log10(maxit); /* works better than log() */
      if(nextsavedincr < 4) nextsavedincr = 4; /* maintains image with low iterations */
      firstsavedand = (long)((nextsavedincr*2) + 1);
   }

   LogTable = NULL;
   MaxLTSize = maxit;
   Log_Calc = 0;
/* below, INT_MAX=32767 only when an integer is two bytes.  Which is not true for Xfractint. */
/* Since 32767 is what was meant, replaced the instances of INT_MAX with 32767. */
   if(LogFlag && (((maxit > 32767) && (save_release > 1920))
      || Log_Fly_Calc == 1)) {
      Log_Calc = 1; /* calculate on the fly */
      SetupLogTable();
   }
   else if(LogFlag && (((maxit > 32767) && (save_release <= 1920))
           || Log_Fly_Calc == 2)) {
      MaxLTSize = 32767;
      Log_Calc = 0; /* use logtable */
   }
   else if(rangeslen && (maxit >= 32767)) {
      MaxLTSize = 32766;
   }

   if ((LogFlag || rangeslen) && !Log_Calc)
   {
      if(logtable_in_extra_ok())
         LogTable = (BYTE far *)(dx0 + 2*(xdots+ydots));
      else
         LogTable = (BYTE far *)farmemalloc((long)MaxLTSize + 1);

      if(LogTable == NULL)
      {
         if (rangeslen || Log_Fly_Calc == 2) {
           static FCODE msg[]={"Insufficient memory for logmap/ranges with this maxiter"};
           stopmsg(0,msg);
         }
         else {
            static FCODE msg[]={"Insufficient memory for logTable, using on-the-fly routine"};
            stopmsg(0,msg);
            Log_Fly_Calc = 1;
            Log_Calc = 1; /* calculate on the fly */
            SetupLogTable();
         }
      }
      else if (rangeslen) { /* Can't do ranges if MaxLTSize > 32767 */
         int i,k,l,m,numval,flip,altern;
         i = k = l = 0;
         LogFlag = 0; /* ranges overrides logmap */
         while (i < rangeslen) {
            m = flip = 0;
            altern = 32767;
            if ((numval = ranges[i++]) < 0) {
               altern = ranges[i++];    /* sub-range iterations */
               numval = ranges[i++];
               }
            if (numval > (int)MaxLTSize || i >= rangeslen)
               numval = (int)MaxLTSize;
            while (l <= numval)  {
               LogTable[l++] = (BYTE)(k + flip);
               if (++m >= altern) {
                  flip ^= 1;            /* Alternate colors */
                  m = 0;
                  }
               }
            ++k;
            if (altern != 32767) ++k;
            }
         }
      else
         SetupLogTable();
   }
   lm = 4L << bitshift;                 /* CALCMAND magnitude limit */

   if (save_release > 2002)
      atan_colors = colors;
   else
      atan_colors = 180;

   /* ORBIT stuff */
   save_orbit = (int far *)((double huge *)dx0 + 4*OLDMAXPIXELS);
   show_orbit = start_showorbit;
   orbit_ptr = 0;
   orbit_color = 15;
   if(colors < 16)
      orbit_color = 1;

   if(inversion[0] != 0.0)
   {
      f_radius    = inversion[0];
      f_xcenter   = inversion[1];
      f_ycenter   = inversion[2];

      if (inversion[0] == AUTOINVERT)  /*  auto calc radius 1/6 screen */
      {
         inversion[0] = min(fabs(xxmax - xxmin),
             fabs(yymax - yymin)) / 6.0;
         fix_inversion(&inversion[0]);
         f_radius = inversion[0];
      }

      if (invert < 2 || inversion[1] == AUTOINVERT)  /* xcenter not already set */
      {
         inversion[1] = (xxmin + xxmax) / 2.0;
         fix_inversion(&inversion[1]);
         f_xcenter = inversion[1];
         if (fabs(f_xcenter) < fabs(xxmax-xxmin) / 100)
            inversion[1] = f_xcenter = 0.0;
      }

      if (invert < 3 || inversion[2] == AUTOINVERT)  /* ycenter not already set */
      {
         inversion[2] = (yymin + yymax) / 2.0;
         fix_inversion(&inversion[2]);
         f_ycenter = inversion[2];
         if (fabs(f_ycenter) < fabs(yymax-yymin) / 100)
            inversion[2] = f_ycenter = 0.0;
      }

      invert = 3; /* so values will not be changed if we come back */
   }

   closenuff = ddelmin*pow(2.0,-(double)(abs(periodicitycheck)));
   rqlim_save = rqlim;
   rqlim2 = sqrt(rqlim);
   if (integerfractal)          /* for integer routines (lambda) */
   {
      lparm.x = (long)(parm.x * fudge);    /* real portion of Lambda */
      lparm.y = (long)(parm.y * fudge);    /* imaginary portion of Lambda */
      lparm2.x = (long)(parm2.x * fudge);  /* real portion of Lambda2 */
      lparm2.y = (long)(parm2.y * fudge);  /* imaginary portion of Lambda2 */
      llimit = (long)(rqlim * fudge);      /* stop if magnitude exceeds this */
      if (llimit <= 0) llimit = 0x7fffffffL; /* klooge for integer math */
      llimit2 = (long)(rqlim2 * fudge);    /* stop if magnitude exceeds this */
      lclosenuff = (long)(closenuff * fudge); /* "close enough" value */
      l16triglim = 8L<<16;         /* domain limit of fast trig functions */
      linitorbit.x = (long)(initorbit.x * fudge);
      linitorbit.y = (long)(initorbit.y * fudge);
   }
   resuming = (calc_status == 2);
   if (!resuming) /* free resume_info memory if any is hanging around */
   {
      end_resume();
      if (resave_flag) {
         updatesavename(savename); /* do the pending increment */
         resave_flag = started_resaves = 0;
         }
      calctime = 0;
   }

   if (curfractalspecific->calctype != StandardFractal
       && curfractalspecific->calctype != calcmand
       && curfractalspecific->calctype != calcmandfp
       && curfractalspecific->calctype != lyapunov
       && curfractalspecific->calctype != calcfroth)
   {
      calctype = curfractalspecific->calctype; /* per_image can override */
      symmetry = curfractalspecific->symmetry; /*   calctype & symmetry  */
      plot = putcolor; /* defaults when setsymmetry not called or does nothing */
      iystart = ixstart = yystart = xxstart = yybegin = xxbegin = 0;
      iystop = yystop = ydots -1;
      ixstop = xxstop = xdots -1;
      calc_status = 1; /* mark as in-progress */
      distest = 0; /* only standard escape time engine supports distest */
      /* per_image routine is run here */
      if (curfractalspecific->per_image())
      { /* not a stand-alone */
         /* next two lines in case periodicity changed */
         closenuff = ddelmin*pow(2.0,-(double)(abs(periodicitycheck)));
         lclosenuff = (long)(closenuff * fudge); /* "close enough" value */
         setsymmetry(symmetry,0);
         timer(0,calctype); /* non-standard fractal engine */
      }
      if (check_key())
      {
         if (calc_status == 1) /* calctype didn't set this itself, */
            calc_status = 3;   /* so mark it interrupted, non-resumable */
      }
      else
         calc_status = 4; /* no key, so assume it completed */
   }
   else /* standard escape-time engine */
   {
      if(stdcalcmode == '3')  /* convoluted 'g' + '2' hybrid */
      {
         int oldcalcmode;
         oldcalcmode = stdcalcmode;
         if(!resuming || three_pass)
         {
            stdcalcmode = 'g';
            three_pass = 1;
            timer(0,(int(*)())perform_worklist);
            if(calc_status == 4)
            {
               if(xdots >= 640)  /* '2' is silly after 'g' for low rez */
                  stdcalcmode = '2';
               else
                  stdcalcmode = '1';

               timer(0,(int(*)())perform_worklist);
               three_pass = 0;
            }
         }
         else /* resuming '2' pass */
         {
            if(xdots >= 640)
               stdcalcmode = '2';
            else
               stdcalcmode = '1';
            timer(0,(int(*)())perform_worklist);
         }
         stdcalcmode = (char)oldcalcmode;
      }
      else /* main case, much nicer! */
      {
         three_pass = 0;
         timer(0,(int(*)())perform_worklist);
      }
   }
   calctime += timer_interval;

   if(LogTable && !Log_Calc)
   {
      if(!logtable_in_extra_ok())
         farmemfree(LogTable);   /* free if not using extraseg */
      LogTable = NULL;
   }
   if(typespecific_workarea)
   {
      free_workarea();
   }

   if (curfractalspecific->calctype == calcfroth)
      froth_cleanup();
   if((soundflag&7)>1) /* close sound write file */
      close_snd();
   if(truecolor)
      enddisk();
   return((calc_status == 4) ? 0 : -1);
}

/* locate alternate math record */
int find_alternate_math(int type, int math)
{
   int i,ret,curtype /* ,curmath=0 */;
   /* unsigned umath; */
   ret = -1;
   if(math==0)
      return(ret);
   i= -1;
#if 0  /* for now at least, the only alternatemath is bignum and bigflt math */
   umath = math;
   umath <<= 14;  /* BF_MATH or DL_MATH */

   /* this depends on last two bits of flags */
   if(fractalspecific[type].flags & umath)
   {
      while(((curtype=alternatemath[++i].type ) != type
          || (curmath=alternatemath[i].math) != math) && curtype != -1);
      if(curtype == type && curmath == math)
        ret = i;
   }
#else
   while ((curtype=alternatemath[++i].type) != type && curtype != -1)
      ;
   if(curtype == type && alternatemath[i].math)
     ret = i;
#endif
   return(ret);
}


/**************** general escape-time engine routines *********************/

static void perform_worklist()
{
   int (*sv_orbitcalc)(void) = NULL;  /* function that calculates one orbit */
   int (*sv_per_pixel)(void) = NULL;  /* once-per-pixel init */
   int (*sv_per_image)(void) = NULL;  /* once-per-image setup */
   int i, alt;

   if((alt=find_alternate_math(fractype,bf_math)) > -1)
   {
      sv_orbitcalc = curfractalspecific->orbitcalc;
      sv_per_pixel = curfractalspecific->per_pixel;
      sv_per_image = curfractalspecific->per_image;
      curfractalspecific->orbitcalc = alternatemath[alt].orbitcalc;
      curfractalspecific->per_pixel = alternatemath[alt].per_pixel;
      curfractalspecific->per_image = alternatemath[alt].per_image;
   }
   else
      bf_math = 0;

   if (potflag && pot16bit)
   {
      int tmpcalcmode = stdcalcmode;

      stdcalcmode = '1'; /* force 1 pass */
      if (resuming == 0)
         if (pot_startdisk() < 0)
         {
            pot16bit = 0;       /* startdisk failed or cancelled */
            stdcalcmode = (char)tmpcalcmode;    /* maybe we can carry on??? */
         }
   }
   if (stdcalcmode == 'b' && (curfractalspecific->flags & NOTRACE))
      stdcalcmode = '1';
   if (stdcalcmode == 'g' && (curfractalspecific->flags & NOGUESS))
      stdcalcmode = '1';
   if (stdcalcmode == 'o' && (curfractalspecific->calctype != StandardFractal))
      stdcalcmode = '1';

   /* default setup a new worklist */
   num_worklist = 1;
   worklist[0].xxstart = worklist[0].xxbegin = 0;
   worklist[0].yystart = worklist[0].yybegin = 0;
   worklist[0].xxstop = xdots - 1;
   worklist[0].yystop = ydots - 1;
   worklist[0].pass = worklist[0].sym = 0;
   if (resuming) /* restore worklist, if we can't the above will stay in place */
   {
      int vsn;
      vsn = start_resume();
      get_resume(sizeof(num_worklist),&num_worklist,sizeof(worklist),worklist,0);
      end_resume();
      if (vsn < 2)
         xxbegin = 0;
   }

   if (distest) /* setup stuff for distance estimator */
   {
      double ftemp,ftemp2,delxx,delyy2,delyy,delxx2,dxsize,dysize;
      double aspect;
      if(pseudox && pseudoy)
      {
         aspect = (double)pseudoy/(double)pseudox;
         dxsize = pseudox-1;
         dysize = pseudoy-1;
      }
      else
      {
         aspect = (double)ydots/(double)xdots;
         dxsize = xdots-1;
         dysize = ydots-1;
      }

      delxx  = (xxmax - xx3rd) / dxsize; /* calculate stepsizes */
      delyy  = (yymax - yy3rd) / dysize;
      delxx2 = (xx3rd - xxmin) / dysize;
      delyy2 = (yy3rd - yymin) / dxsize;

      if (save_release < 1827) /* in case it's changed with <G> */
         use_old_distest = 1;
      else
         use_old_distest = 0;
      rqlim = rqlim_save; /* just in case changed to DEM_BAILOUT earlier */
      if (distest != 1 || colors == 2) /* not doing regular outside colors */
         if (rqlim < DEM_BAILOUT)         /* so go straight for dem bailout */
            rqlim = DEM_BAILOUT;
      if (curfractalspecific->tojulia != NOFRACTAL || use_old_distest
          || fractype == FORMULA || fractype == FFORMULA)
         dem_mandel = 1; /* must be mandel type, formula, or old PAR/GIF */
      else
         dem_mandel = 0;
      dem_delta = sqr(delxx) + sqr(delyy2);
      if ((ftemp = sqr(delyy) + sqr(delxx2)) > dem_delta)
         dem_delta = ftemp;
      if (distestwidth == 0)
         distestwidth = 1;
      ftemp = distestwidth;
      if (distestwidth > 0)
         dem_delta *= sqr(ftemp)/10000; /* multiply by thickness desired */
      else
         dem_delta *= 1/(sqr(ftemp)*10000); /* multiply by thickness desired */
      dem_width = ( sqrt( sqr(xxmax-xxmin) + sqr(xx3rd-xxmin) ) * aspect
          + sqrt( sqr(yymax-yymin) + sqr(yy3rd-yymin) ) ) / distest;
      ftemp = (rqlim < DEM_BAILOUT) ? DEM_BAILOUT : rqlim;
      ftemp += 3; /* bailout plus just a bit */
      ftemp2 = log(ftemp);
      if(use_old_distest)
         dem_toobig = sqr(ftemp) * sqr(ftemp2) * 4 / dem_delta;
      else
         dem_toobig = fabs(ftemp) * fabs(ftemp2) * 2 / sqrt(dem_delta);
   }

   while (num_worklist > 0)
   {
      /* per_image can override */
      calctype = curfractalspecific->calctype;
      symmetry = curfractalspecific->symmetry; /*   calctype & symmetry  */
      plot = putcolor; /* defaults when setsymmetry not called or does nothing */

      /* pull top entry off worklist */
      ixstart = xxstart = worklist[0].xxstart;
      ixstop  = xxstop  = worklist[0].xxstop;
      xxbegin  = worklist[0].xxbegin;
      iystart = yystart = worklist[0].yystart;
      iystop  = yystop  = worklist[0].yystop;
      yybegin  = worklist[0].yybegin;
      workpass = worklist[0].pass;
      worksym  = worklist[0].sym;
      --num_worklist;
      for (i=0; i<num_worklist; ++i)
         worklist[i] = worklist[i+1];

      calc_status = 1; /* mark as in-progress */

      curfractalspecific->per_image();
      if(showdot >= 0)
      {
        find_special_colors();
        switch(autoshowdot)
        {
           case 'd':
              showdotcolor = color_dark%colors;
              break;
           case 'm':
              showdotcolor = color_medium%colors;
              break;
           case 'b':
           case 'a':
              showdotcolor = color_bright%colors;
              break;
           default:
              showdotcolor = showdot%colors;
              break;
        }
        if(sizedot <= 0)
            showdot_width = -1;
         else
         {
            double dshowdot_width;
            dshowdot_width = (double)sizedot*xdots/1024.0;
            /*
               Arbitrary sanity limit, however showdot_width will
               overflow if dshowdot width gets near 256.
            */
            if(dshowdot_width > 150.0)
               showdot_width = 150;
            else if(dshowdot_width > 0.0)
               showdot_width = (int)dshowdot_width;
            else
               showdot_width = -1;
         }
#ifdef SAVEDOTS_USES_MALLOC
         while(showdot_width >= 0)
         {
            /*
               We're using near memory, so get the amount down
               to something reasonable. The polynomial used to
               calculate savedotslen is exactly right for the
               triangular-shaped shotdot cursor. The that cursor
               is changed, this formula must match.
            */
            while((savedotslen=sqr(showdot_width)+5*showdot_width+4) > 1000)
               showdot_width--;
            if((savedots = (BYTE *)malloc(savedotslen)) != NULL)
            {
               savedotslen /= 2;
               fillbuff = savedots + savedotslen;
               memset(fillbuff,showdotcolor,savedotslen);
               break;
            }
            /*
               There's even less free memory than we thought, so reduce
               showdot_width still more
            */
            showdot_width--;
         }
         if(savedots == NULL)
            showdot_width = -1;
#else
         while((savedotslen=sqr(showdot_width)+5*showdot_width+4) > 2048)
            showdot_width--;
         savedots = (BYTE *)decoderline;
         savedotslen /= 2;
         fillbuff = savedots + savedotslen;
         memset(fillbuff,showdotcolor,savedotslen);
#endif
         calctypetmp = calctype;
         calctype    = calctypeshowdot;
      }

      /* some common initialization for escape-time pixel level routines */
      closenuff = ddelmin*pow(2.0,-(double)(abs(periodicitycheck)));
      lclosenuff = (long)(closenuff * fudge); /* "close enough" value */
      kbdcount=max_kbdcount;

      setsymmetry(symmetry,1);

      if (!(resuming)&&(labs(LogFlag) ==2 || (LogFlag && Log_Auto_Calc)))
       {  /* calculate round screen edges to work out best start for logmap */
         LogFlag = ( autologmap() * (LogFlag / labs(LogFlag)));
         SetupLogTable();
       }

      /* call the appropriate escape-time engine */
      switch (stdcalcmode)
      {
      case 's':
         if(debugflag==3444)
            soi_ldbl();
         else
            soi();
         break;
      case 't':
         tesseral();
         break;
      case 'b':
         bound_trace_main();
         break;
      case 'g':
         solidguess();
         break;
      case 'd':
         diffusion_scan();
         break;
      case 'o':
         sticky_orbits();
         break;
      default:
         OneOrTwoPass();
      }
#ifdef SAVEDOTS_USES_MALLOC
      if(savedots != NULL)
      {
         free(savedots);
         savedots = NULL;
         fillbuff = NULL;
      }
#endif
      if (check_key()) /* interrupted? */
         break;
   }

   if (num_worklist > 0)
   {  /* interrupted, resumable */
      alloc_resume(sizeof(worklist)+20,2);
      put_resume(sizeof(num_worklist),&num_worklist,sizeof(worklist),worklist,0);
   }
   else
      calc_status = 4; /* completed */
   if(sv_orbitcalc != NULL)
   {
      curfractalspecific->orbitcalc = sv_orbitcalc;
      curfractalspecific->per_pixel = sv_per_pixel;
      curfractalspecific->per_image = sv_per_image;
   }
}

#if (_MSC_VER >= 700)
#pragma code_seg ()     /* back to normal segment */
#endif

static int diffusion_scan(void)
{
   double log2;

   log2 = (double) log (2.0);

   got_status = 5;

   /* note: the max size of 2048x2048 gives us a 22 bit counter that will */
   /* fit any 32 bit architecture, the maxinum limit for this case would  */
   /* be 65536x65536 (HB) */

   bits = (unsigned) (min ( log (iystop-iystart+1), log(ixstop-ixstart+1) )/log2 );
   bits <<= 1; /* double for two axes */
   dif_limit = 1l << bits;

   if (diffusion_engine() == -1)
   {
      add_worklist(xxstart,xxstop,xxstart,yystart,yystop,
       (int)(dif_counter >> 16),            /* high, */
       (int)(dif_counter & 0xffff),         /* low order words */
       worksym);
      return(-1);
   }

   return(0);
}

/* little macro that plots a filled square of color c, size s with
   top left cornet at (x,y) with optimization from sym_fill_line */
#define plot_block(x,y,s,c) \
    memset(dstack,(c),(s)); \
    for(ty=(y); ty<(y)+(s); ty++) \
       sym_fill_line(ty, (x), (x)+(s)-1, dstack)

/* macro that does the same as above, but checks the limits in x and y */
#define plot_block_lim(x,y,s,c) \
    memset(dstack,(c),(s)); \
    for(ty=(y); ty<min((y)+(s),iystop+1); ty++) \
       sym_fill_line(ty, (x), min((x)+(s)-1,ixstop), dstack)

/* macro: count_to_int(dif_counter, colo, rowo) */
/* (inlined  function:) */
#define count_to_int(C,x,y)\
   \
   tC = C; \
   \
   x =dif_la[tC&0xFF];         y =dif_lb[tC&0xFF]; tC>>=8;   \
   x <<=4; x+=dif_la[tC&0xFF]; y <<=4; y+=dif_lb[tC&0xFF]; tC>>=8; \
   x <<=4; x+=dif_la[tC&0xFF]; y <<=4; y+=dif_lb[tC&0xFF]; tC>>=8; \
   x >>= dif_offset;           y >>= dif_offset
/* end of inlined function */

/* REMOVED: counter byte 3 */                                                          \
/* (x) <<=4; (x)+=dif_la[tC&0(x)FF]; (y) <<=4; (y)+=dif_lb[tC&0(x)FF]; tC>>=8;
   --> eliminated this and made (*) because fractint user coordinates up to
   2048(x)2048 what means a counter of 24 bits or 3 bytes */

/* Calculate the point */
#define calculate \
        reset_periodicity = 1;   \
        if ((*calctype)() == -1) \
           return(-1);           \
        reset_periodicity = 0

static int diffusion_engine (void) {

   double log2 = (double) log (2.0);

   int i,j;

   int nx,ny; /* number of tyles to build in x and y dirs */
       /* made this to complete the area that is not */
       /* a square with sides like 2 ** n */
   int rem_x,rem_y; /* what is left on the last tile to draw */

   int ty;  /* temp for y */

   long unsigned tC; /* temp for dif_counter */

   int dif_offset; /* offset for adjusting looked-up values */

   int sqsz;  /* size of the block being filled */

   int colo, rowo; /* original col and row */

   int s = 1 << (bits/2); /* size of the square */

   nx = (int) floor( (ixstop-ixstart+1)/s );
   ny = (int) floor( (iystop-iystart+1)/s );

   rem_x = (ixstop-ixstart+1) - nx * s;
   rem_y = (iystop-iystart+1) - ny * s;

   if (yybegin == iystart && workpass == 0) { /* if restarting on pan: */
      dif_counter =0l;
   } else {
      /* yybegin and passes contain data for resuming the type: */
      dif_counter = (((long) ((unsigned)yybegin))<<16) | ((unsigned)workpass);
   }

   dif_offset = 12-(bits/2); /* offset to adjust coordinates */
            /* (*) for 4 bytes use 16 for 3 use 12 etc. */

   /*************************************/
   /* only the points (dithering only) :*/
   if ( fillcolor==0 ){
      while (dif_counter < (dif_limit>>1)) {
         count_to_int(dif_counter, colo, rowo);

         i=0;
         col = ixstart + colo; /* get the right tiles */
         do {
            j=0;
            row = iystart + rowo ;
            do {
               calculate;
               (*plot)(col,row,color);
               j++;
               row += s;                  /* next tile */
            } while (j < ny);
            /* in the last y tile we may not need to plot the point
             */
            if (rowo < rem_y) {
               calculate;
               (*plot)(col,row,color);
            }
            i++;
            col += s;
         } while (i < nx);
         /* in the last x tiles we may not need to plot the point */ if
         (colo < rem_x) {
            row = iystart + rowo;
            j=0;
            do {
               calculate;
               (*plot)(col,row,color);
               j++;
               row += s; /* next tile */
            } while (j < ny);
            if (rowo < rem_y) {
               calculate;
               (*plot)(col,row,color);
            }
         }
         dif_counter++;
      }
   } else {
   /*********************************/
   /* with progressive filling :    */
      while (dif_counter < (dif_limit>>1))
      {
       sqsz=1<<( (int)(bits-(int)( log(dif_counter+0.5)/log2 )-1)/2 );

       count_to_int(dif_counter, colo, rowo);

       i=0;
       do {
        j=0;
        do {
           col = ixstart + colo + i * s; /* get the right tiles */
           row = iystart + rowo + j * s;

           calculate;
           plot_block(col,row,sqsz,color);
           j++;
        } while (j < ny);
     /* in the last tile we may not need to plot the point */
     if (rowo < rem_y) {
        row = iystart + rowo + ny * s;

        calculate;
        plot_block_lim(col,row,sqsz,color);
     }
    i++;
       } while (i < nx);
       /* in the last tile we may not need to plot the point */
       if (colo < rem_x) {
     col = ixstart + colo + nx * s;
     j=0;
     do {
        row = iystart + rowo + j * s; /* get the right tiles */

        calculate;
        plot_block_lim(col,row,sqsz,color);
        j++;
     } while (j < ny);
     if (rowo < rem_y) {
        row = iystart + rowo + ny * s;

        calculate;
        plot_block_lim(col,row,sqsz,color);
     }
       }

       dif_counter++;
      }
   }
   /* from half dif_limit on we only plot 1x1 points :-) */
   while (dif_counter < dif_limit)
   {
      count_to_int(dif_counter, colo, rowo);

      i=0;
      do {
       j=0;
       do {
     col = ixstart + colo + i * s; /* get the right tiles */
     row = iystart + rowo + j * s;

     calculate;
     (*plot)(col,row,color);
     j++;
       } while (j < ny);
       /* in the last tile we may not need to plot the point */
       if (rowo < rem_y) {
     row = iystart + rowo + ny * s;

     calculate;
     (*plot)(col,row,color);
       }
      i++;
      } while (i < nx);
      /* in the last tile we may nnt need to plot the point */
      if (colo < rem_x) {
       col = ixstart + colo + nx * s;
       j=0;
       do {
     row = iystart + rowo + j * s; /* get the right tiles */

     calculate;
     (*plot)(col,row,color);
     j++;
       } while (j < ny);
       if (rowo < rem_y) {
     row = iystart + rowo + ny * s;

     calculate;
     (*plot)(col,row,color);
       }
      }
      dif_counter++;
   }
   return(0);
}

/* OLD function (less eficient than the lookup code above:
static void count_to_int (long unsigned C, int *r, int *l) {

   int i;

   *r = *l = 0;

   for (i = bits; i>0; i -= 2){
      *r <<=1; *r += C % 2; C >>= 1;
      *l <<=1; *l += C % 2; C >>= 1;
   }
   *l = (*l+*r)%(1<<(bits/2));  * a+b mod 2^k *

}
*/

char drawmode = 'r';

static int sticky_orbits(void)
{
   got_status = 6; /* for <tab> screen */
   totpasses = 1;

   if (plotorbits2dsetup() == -1) {
      stdcalcmode = 'g';
      return(-1);
   }

   switch (drawmode)
   {
   case 'r':
   default:
   /* draw a rectangle */
      row = yybegin;
      col = xxbegin;

      while (row <= iystop)
      {
         currow = row;
         while (col <= ixstop)
         {
            if (plotorbits2dfloat() == -1)
            {
               add_worklist(xxstart,xxstop,col,yystart,yystop,row,0,worksym);
               return(-1); /* interrupted */
            }
            ++col;
         }
         col = ixstart;
         ++row;
      }
      break;
   case 'l':
      {
      int dX, dY;                     /* vector components */
      int final,                      /* final row or column number */
          G,                  /* used to test for new row or column */
          inc1,           /* G increment when row or column doesn't change */
          inc2;               /* G increment when row or column changes */
      char pos_slope;

      dX = ixstop - ixstart;                   /* find vector components */
      dY = iystop - iystart;
      pos_slope = (char)(dX > 0);                   /* is slope positive? */
      if (dY < 0)
      pos_slope = (char)!pos_slope;
      if (abs (dX) > abs (dY))                /* shallow line case */
      {
         if (dX > 0)         /* determine start point and last column */
         {
            col = xxbegin;
            row = yybegin;
            final = ixstop;
         }
         else
         {
            col = ixstop;
            row = iystop;
            final = xxbegin;
         }
         inc1 = 2 * abs (dY);            /* determine increments and initial G */
         G = inc1 - abs (dX);
         inc2 = 2 * (abs (dY) - abs (dX));
         if (pos_slope)
            while (col <= final)    /* step through columns checking for new row */
            {
               if (plotorbits2dfloat() == -1)
               {
                 add_worklist(xxstart,xxstop,col,yystart,yystop,row,0,worksym);
                 return(-1); /* interrupted */
               }
               col++;
               if (G >= 0)             /* it's time to change rows */
               {
                  row++;      /* positive slope so increment through the rows */
                  G += inc2;
               }
               else                        /* stay at the same row */
                  G += inc1;
            }
         else
            while (col <= final)    /* step through columns checking for new row */
            {
               if (plotorbits2dfloat() == -1)
               {
                 add_worklist(xxstart,xxstop,col,yystart,yystop,row,0,worksym);
                 return(-1); /* interrupted */
               }
               col++;
               if (G > 0)              /* it's time to change rows */
               {
                  row--;      /* negative slope so decrement through the rows */
                  G += inc2;
               }
               else                        /* stay at the same row */
                  G += inc1;
            }
      }   /* if |dX| > |dY| */
      else                            /* steep line case */
      {
        if (dY > 0)             /* determine start point and last row */
        {
            col = xxbegin;
            row = yybegin;
            final = iystop;
        }
        else
        {
            col = ixstop;
            row = iystop;
            final = yybegin;
        }
        inc1 = 2 * abs (dX);            /* determine increments and initial G */
        G = inc1 - abs (dY);
        inc2 = 2 * (abs (dX) - abs (dY));
        if (pos_slope)
           while (row <= final)    /* step through rows checking for new column */
           {
              if (plotorbits2dfloat() == -1)
              {
                add_worklist(xxstart,xxstop,col,yystart,yystop,row,0,worksym);
                return(-1); /* interrupted */
              }
              row++;
              if (G >= 0)                 /* it's time to change columns */
              {
                  col++;  /* positive slope so increment through the columns */
                  G += inc2;
              }
              else                    /* stay at the same column */
                  G += inc1;
           }
        else
           while (row <= final)    /* step through rows checking for new column */
           {
              if (plotorbits2dfloat() == -1)
              {
                add_worklist(xxstart,xxstop,col,yystart,yystop,row,0,worksym);
                return(-1); /* interrupted */
              }
              row++;
              if (G > 0)                  /* it's time to change columns */
              {
                 col--;  /* negative slope so decrement through the columns */
                 G += inc2;
              }
              else                    /* stay at the same column */
                 G += inc1;
           }
        }
      } /* end case 'l' */
      break;
   case 'f':  /* this code does not yet work??? */
      {
      double Xctr,Yctr;
      LDBL Magnification; /* LDBL not really needed here, but used to match function parameters */
      double Xmagfactor,Rotation,Skew;
      int angle;
      double factor = PI / 180.0;
      double theta;
      double xfactor = xdots / 2.0;
      double yfactor = ydots / 2.0;

      angle = xxbegin;  /* save angle in x parameter */
      
      cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
      if (Rotation <= 0)
         Rotation += 360;

      while (angle < Rotation)
      {
	 theta = (double)angle * factor; 
         col = (int)(xfactor + (Xctr + Xmagfactor * cos(theta)));
         row = (int)(yfactor + (Yctr + Xmagfactor * sin(theta)));
         if (plotorbits2dfloat() == -1)
         {
            add_worklist(angle,0,0,0,0,0,0,worksym);
            return(-1); /* interrupted */
         }
         angle++;
      }


      }  /* end case 'f' */
      break;
   }  /* end switch */

   return(0);
}

static int OneOrTwoPass(void)
{
   int i;

   totpasses = 1;
   if (stdcalcmode == '2') totpasses = 2;
   if (stdcalcmode == '2' && workpass == 0) /* do 1st pass of two */
   {
      if (StandardCalc(1) == -1)
      {
         add_worklist(xxstart,xxstop,col,yystart,yystop,row,0,worksym);
         return(-1);
      }
      if (num_worklist > 0) /* worklist not empty, defer 2nd pass */
      {
         add_worklist(xxstart,xxstop,xxstart,yystart,yystop,yystart,1,worksym);
         return(0);
      }
      workpass = 1;
      xxbegin = xxstart;
      yybegin = yystart;
   }
   /* second or only pass */
   if (StandardCalc(2) == -1)
   {
      i = yystop;
      if (iystop != yystop) /* must be due to symmetry */
         i -= row - iystart;
      add_worklist(xxstart,xxstop,col,row,i,row,workpass,worksym);
      return(-1);
   }

   return(0);
}

static int _fastcall StandardCalc(int passnum)
{
   got_status = 0;
   curpass = passnum;
   row = yybegin;
   col = xxbegin;

   while (row <= iystop)
   {
      currow = row;
      reset_periodicity = 1;
      while (col <= ixstop)
      {
         /* on 2nd pass of two, skip even pts */
         if (quick_calc && !resuming)
            if ((color = getcolor(col,row)) != inside) {
               ++col;
               continue;
            }
         if (passnum == 1 || stdcalcmode == '1' || (row&1) != 0 || (col&1) != 0)
         {
            if ((*calctype)() == -1) /* StandardFractal(), calcmand() or calcmandfp() */
               return(-1); /* interrupted */
            resuming = 0; /* reset so quick_calc works */
            reset_periodicity = 0;
            if (passnum == 1) /* first pass, copy pixel and bump col */
            {
               if ((row&1) == 0 && row < iystop)
               {
                  (*plot)(col,row+1,color);
                  if ((col&1) == 0 && col < ixstop)
                     (*plot)(col+1,row+1,color);
               }
               if ((col&1) == 0 && col < ixstop)
                  (*plot)(++col,row,color);
            }
         }
         ++col;
      }
      col = ixstart;
      if (passnum == 1 && (row&1) == 0)
         ++row;
      ++row;
   }
   return(0);
}


int calcmand(void)              /* fast per pixel 1/2/b/g, called with row & col set */
{
   /* setup values from far array to avoid using es reg in calcmand.asm */
   linitx = lxpixel();
   linity = lypixel();
   if (calcmandasm() >= 0)
   {
      if ((LogTable || Log_Calc) /* map color, but not if maxit & adjusted for inside,etc */
      && (realcoloriter < maxit || (inside < 0 && coloriter == maxit)))
            coloriter = logtablecalc(coloriter);
      color = abs((int)coloriter);
      if (coloriter >= colors) { /* don't use color 0 unless from inside/outside */
         if (save_release <= 1950) {
            if (colors < 16)
               color &= andcolor;
            else
               color = ((color - 1) % andcolor) + 1;  /* skip color zero */
         }
         else {
            if (colors < 16)
               color = (int)(coloriter & andcolor);
            else
               color = (int)(((coloriter - 1) % andcolor) + 1);
         }
      }
      if(debugflag != 470)
         if(color <= 0 && stdcalcmode == 'b' )   /* fix BTM bug */
            color = 1;
      (*plot) (col, row, color);
   }
   else
      color = (int)coloriter;
   return (color);
}

long (*calcmandfpasm)(void);

/************************************************************************/
/* added by Wes Loewer - sort of a floating point version of calcmand() */
/* can also handle invert, any rqlim, potflag, zmag, epsilon cross,     */
/* and all the current outside options    -Wes Loewer 11/03/91          */
/************************************************************************/
int calcmandfp(void)
{
   if(invert)
      invertz2(&init);
   else
   {
      init.x = dxpixel();
      init.y = dypixel();
   }
   if (calcmandfpasm() >= 0)
   {
      if (potflag)
         coloriter = potential(magnitude, realcoloriter);
      if ((LogTable || Log_Calc) /* map color, but not if maxit & adjusted for inside,etc */
          && (realcoloriter < maxit || (inside < 0 && coloriter == maxit)))
            coloriter = logtablecalc(coloriter);
      color = abs((int)coloriter);
      if (coloriter >= colors) { /* don't use color 0 unless from inside/outside */
         if (save_release <= 1950) {
            if (colors < 16)
               color &= andcolor;
            else
               color = ((color - 1) % andcolor) + 1;  /* skip color zero */
         }
         else {
            if (colors < 16)
               color = (int)(coloriter & andcolor);
            else
               color = (int)(((coloriter - 1) % andcolor) + 1);
         }
      }
      if(debugflag != 470)
         if(color == 0 && stdcalcmode == 'b' )   /* fix BTM bug */
            color = 1;
      (*plot) (col, row, color);
   }
   else
      color = (int)coloriter;
   return (color);
}
#define STARTRAILMAX FLT_MAX   /* just a convenient large number */
#define green 2
#define yellow 6
#if 0
#define NUMSAVED 40     /* define this to save periodicity analysis to file */
#endif
#if 0
#define MINSAVEDAND 3   /* if not defined, old method used */
#endif
int StandardFractal(void)       /* per pixel 1/2/b/g, called with row & col set */
{
#ifdef NUMSAVED
   _CMPLX savedz[NUMSAVED];
   long caught[NUMSAVED];
   long changed[NUMSAVED];
   int zctr = 0;
#endif
   long savemaxit;
   double tantable[16];
   int hooper = 0;
   long lcloseprox;
   double memvalue = 0.0;
   double min_orbit = 100000.0; /* orbit value closest to origin */
   long   min_index = 0;        /* iteration of min_orbit */
   long cyclelen = -1;
   long savedcoloriter = 0;
   int caught_a_cycle;
   long savedand;
   int savedincr;       /* for periodicity checking */
   _LCMPLX lsaved;
   int i, attracted;
   _LCMPLX lat;
   _CMPLX  at;
   _CMPLX deriv;
   long dem_color = -1;
   _CMPLX dem_new;
   int check_freq;
   double totaldist = 0.0;
   _CMPLX lastz;

   lcloseprox = (long)(closeprox*fudge);
   savemaxit = maxit;
#ifdef NUMSAVED
   for(i=0;i<NUMSAVED;i++)
   {
      caught[i] = 0L;
      changed[i] = 0L;
   }
#endif
   if(inside == STARTRAIL)
   {
      int i;
      for(i=0;i<16;i++)
         tantable[i] = 0.0;
      if (save_release > 1824) maxit = 16;
   }
   if (periodicitycheck == 0 || inside == ZMAG || inside == STARTRAIL)
      oldcoloriter = 2147483647L;       /* don't check periodicity at all */
   else if (inside == PERIOD)   /* for display-periodicity */
      oldcoloriter = (maxit/5)*4;       /* don't check until nearly done */
   else if (reset_periodicity)
      oldcoloriter = 255;               /* don't check periodicity 1st 250 iterations */

   /* Jonathan - how about this idea ? skips first saved value which never works */
#ifdef MINSAVEDAND
   if(oldcoloriter < MINSAVEDAND)
      oldcoloriter = MINSAVEDAND;
#else
   if (oldcoloriter < firstsavedand) /* I like it! */
      oldcoloriter = firstsavedand;
#endif
   /* really fractal specific, but we'll leave it here */
   if (!integerfractal)
   {
      if (useinitorbit == 1)
         saved = initorbit;
      else {
         saved.x = 0;
         saved.y = 0;
      }
#ifdef NUMSAVED
      savedz[zctr++] = saved;
#endif
      if(bf_math)
      {
         if(decimals > 200)
            kbdcount = -1;
         if (bf_math == BIGNUM)
         {
            clear_bn(bnsaved.x);
            clear_bn(bnsaved.y);
         }
         else if (bf_math == BIGFLT)
         {
            clear_bf(bfsaved.x);
            clear_bf(bfsaved.y);
         }
      }
      init.y = dypixel();
      if (distest)
      {
         if (use_old_distest) {
           rqlim = rqlim_save;
           if (distest != 1 || colors == 2) /* not doing regular outside colors */
            if (rqlim < DEM_BAILOUT)   /* so go straight for dem bailout */
               rqlim = DEM_BAILOUT;
           dem_color = -1;
         }
         deriv.x = 1;
         deriv.y = 0;
         magnitude = 0;
      }
   }
   else
   {
      if (useinitorbit == 1)
         lsaved = linitorbit;
      else {
         lsaved.x = 0;
         lsaved.y = 0;
      }
      linit.y = lypixel();
   }
   orbit_ptr = 0;
   coloriter = 0;
   if(fractype==JULIAFP || fractype==JULIA)
      coloriter = -1;
   caught_a_cycle = 0;
   if (inside == PERIOD) {
       savedand = 16;           /* begin checking every 16th cycle */
   } else {
     /* Jonathan - don't understand such a low savedand -- how about this? */
#ifdef MINSAVEDAND
       savedand = MINSAVEDAND;
#else
       savedand = firstsavedand;                /* begin checking every other cycle */
#endif
   }
   savedincr = 1;               /* start checking the very first time */

   if (inside <= BOF60 && inside >= BOF61)
   {
      magnitude = lmagnitud = 0;
      min_orbit = 100000.0;
   }
   overflow = 0;                /* reset integer math overflow flag */

   curfractalspecific->per_pixel(); /* initialize the calculations */

   attracted = FALSE;

   if (outside == TDIS) {
      if(integerfractal)
      {
         old.x = ((double)lold.x) / fudge;
         old.y = ((double)lold.y) / fudge;
      }
      else if (bf_math == BIGNUM)
         old = cmplxbntofloat(&bnold);
      else if (bf_math==BIGFLT)
         old = cmplxbftofloat(&bfold);
      lastz.x = old.x;
      lastz.y = old.y;
   }

   if (((soundflag&7) > 2 || showdot >= 0) && orbit_delay > 0)
      check_freq = 16;
   else
      check_freq = 2048;

   if(show_orbit)
      snd_time_write();
   while (++coloriter < maxit)
   {
      /* calculation of one orbit goes here */
      /* input in "old" -- output in "new" */
   if (coloriter % check_freq == 0) {
      if (check_key())
         return (-1);
   }

      if (distest)
      {
         double ftemp;
         /* Distance estimator for points near Mandelbrot set */
         /* Original code by Phil Wilson, hacked around by PB */
         /* Algorithms from Peitgen & Saupe, Science of Fractal Images, p.198 */
         if (dem_mandel)
            ftemp = 2 * (old.x * deriv.x - old.y * deriv.y) + 1;
         else
            ftemp = 2 * (old.x * deriv.x - old.y * deriv.y);
         deriv.y = 2 * (old.y * deriv.x + old.x * deriv.y);
         deriv.x = ftemp;
         if (use_old_distest) {
            if (sqr(deriv.x)+sqr(deriv.y) > dem_toobig)
               break;
         }
         else if (save_release > 1950)
            if (max(fabs(deriv.x),fabs(deriv.y)) > dem_toobig)
               break;
         /* if above exit taken, the later test vs dem_delta will place this
                    point on the boundary, because mag(old)<bailout just now */

         if (curfractalspecific->orbitcalc() || (overflow && save_release > 1826))
         {
          if (use_old_distest) {
           if (dem_color < 0) {
              dem_color = coloriter;
              dem_new = new;
           }
           if (rqlim >= DEM_BAILOUT
           || magnitude >= (rqlim = DEM_BAILOUT)
           || magnitude == 0)
              break;
          }
          else
           break;
         }
         old = new;
      }

      /* the usual case */
      else if ((curfractalspecific->orbitcalc() && inside != STARTRAIL)
              || overflow)
            break;
      if (show_orbit) {
         if (!integerfractal)
         {
            if (bf_math == BIGNUM)
               new = cmplxbntofloat(&bnnew);
            else if (bf_math==BIGFLT)
               new = cmplxbftofloat(&bfnew);
            plot_orbit(new.x, new.y, -1);
         }
         else
            iplot_orbit(lnew.x, lnew.y, -1);
      }
      if( inside < -1)
      {
         if (bf_math == BIGNUM)
            new = cmplxbntofloat(&bnnew);
         else if (bf_math == BIGFLT)
            new = cmplxbftofloat(&bfnew);
         if(inside == STARTRAIL)
         {
            if(0 < coloriter && coloriter < 16)
            {
               if (integerfractal)
               {
                  new.x = lnew.x;
                  new.x /= fudge;
                  new.y = lnew.y;
                  new.y /= fudge;
               }

               if (save_release > 1824) {
                 if(new.x > STARTRAILMAX)
                    new.x = STARTRAILMAX;
                 if(new.x < -STARTRAILMAX)
                    new.x = -STARTRAILMAX;
                 if(new.y > STARTRAILMAX)
                    new.y = STARTRAILMAX;
                 if(new.y < -STARTRAILMAX)
                    new.y = -STARTRAILMAX;
                 tempsqrx = new.x * new.x;
                 tempsqry = new.y * new.y;
                 magnitude = tempsqrx + tempsqry;
                 old = new;
               }
               {
               int tmpcolor;
               tmpcolor = (int)(((coloriter - 1) % andcolor) + 1);
               tantable[tmpcolor-1] = new.y/(new.x+.000001);
               }
            }
         }
         else if(inside == EPSCROSS)
         {
            hooper = 0;
            if(integerfractal)
            {
               if(labs(lnew.x) < labs(lcloseprox))
               {
                  hooper = (lcloseprox>0? 1 : -1); /* close to y axis */
                  goto plot_inside;
               }
               else if(labs(lnew.y) < labs(lcloseprox))
               {
                  hooper = (lcloseprox>0 ? 2: -2); /* close to x axis */
                  goto plot_inside;
               }
            }
            else
            {
               if(fabs(new.x) < fabs(closeprox))
               {
                  hooper = (closeprox>0? 1 : -1); /* close to y axis */
                  goto plot_inside;
               }
               else if(fabs(new.y) < fabs(closeprox))
               {
                  hooper = (closeprox>0? 2 : -2); /* close to x axis */
                  goto plot_inside;
               }
            }
         }
         else if (inside == FMODI)
         {
            double mag;
            if(integerfractal)
            {
               new.x = ((double)lnew.x) / fudge;
               new.y = ((double)lnew.y) / fudge;
            }
            mag = fmodtest();
            if(mag < closeprox)
               memvalue = mag;
         }
         else if (inside <= BOF60 && inside >= BOF61)
         {
            if (integerfractal)
            {
               if (lmagnitud == 0 || no_mag_calc == 0)
                  lmagnitud = lsqr(lnew.x) + lsqr(lnew.y);
               magnitude = lmagnitud;
               magnitude = magnitude / fudge;
            }
            else
               if (magnitude == 0.0 || no_mag_calc == 0)
                  magnitude = sqr(new.x) + sqr(new.y);
            if (magnitude < min_orbit)
            {
               min_orbit = magnitude;
               min_index = coloriter + 1;
            }
         }
      }

      if (outside == TDIS || outside == FMOD)
      {
         if (bf_math == BIGNUM)
            new = cmplxbntofloat(&bnnew);
         else if (bf_math == BIGFLT)
            new = cmplxbftofloat(&bfnew);
         if (outside == TDIS)
         {
            if(integerfractal)
            {
               new.x = ((double)lnew.x) / fudge;
               new.y = ((double)lnew.y) / fudge;
            }
            totaldist += sqrt(sqr(lastz.x-new.x)+sqr(lastz.y-new.y));
            lastz.x = new.x;
            lastz.y = new.y;
         }
         else if (outside == FMOD)
         {
            double mag;
            if(integerfractal)
            {
               new.x = ((double)lnew.x) / fudge;
               new.y = ((double)lnew.y) / fudge;
            }
            mag = fmodtest();
            if(mag < closeprox)
               memvalue = mag;
         }
      }

      if (attractors > 0)       /* finite attractor in the list   */
      {                         /* NOTE: Integer code is UNTESTED */
         if (integerfractal)
         {
            for (i = 0; i < attractors; i++)
            {
                lat.x = lnew.x - lattr[i].x;
                lat.x = lsqr(lat.x);
                if (lat.x < l_at_rad)
                {
                   lat.y = lnew.y - lattr[i].y;
                   lat.y = lsqr(lat.y);
                   if (lat.y < l_at_rad)
                   {
                      if ((lat.x + lat.y) < l_at_rad)
                      {
                         attracted = TRUE;
                         if (finattract<0) coloriter = (coloriter%attrperiod[i])+1;
                         break;
                      }
                   }
                }
            }
         }
         else
         {
            for (i = 0; i < attractors; i++)
            {
                at.x = new.x - attr[i].x;
                at.x = sqr(at.x);
                if (at.x < f_at_rad)
                {
                   at.y = new.y - attr[i].y;
                   at.y = sqr(at.y);
                   if ( at.y < f_at_rad)
                   {
                      if ((at.x + at.y) < f_at_rad)
                      {
                         attracted = TRUE;
                         if (finattract<0) coloriter = (coloriter%attrperiod[i])+1;
                         break;
                      }
                   }
                }
            }
         }
         if (attracted)
            break;              /* AHA! Eaten by an attractor */
      }

      if (coloriter > oldcoloriter) /* check periodicity */
      {
         if ((coloriter & savedand) == 0)            /* time to save a new value */
         {
            savedcoloriter = coloriter;
            if (integerfractal)
               lsaved = lnew;/* integer fractals */
            else if (bf_math == BIGNUM)
            {
               copy_bn(bnsaved.x,bnnew.x);
               copy_bn(bnsaved.y,bnnew.y);
            }
            else if (bf_math == BIGFLT)
            {
               copy_bf(bfsaved.x,bfnew.x);
               copy_bf(bfsaved.y,bfnew.y);
            }
            else
            {
               saved = new;  /* floating pt fractals */
#ifdef NUMSAVED
               if(zctr < NUMSAVED)
               {
                  changed[zctr]  = coloriter;
                  savedz[zctr++] = saved;
               }
#endif
            }
            if (--savedincr == 0)    /* time to lengthen the periodicity? */
            {
               savedand = (savedand << 1) + 1;       /* longer periodicity */
               savedincr = nextsavedincr;/* restart counter */
            }
         }
         else                /* check against an old save */
         {
            if (integerfractal)     /* floating-pt periodicity chk */
            {
               if (labs(lsaved.x - lnew.x) < lclosenuff)
                  if (labs(lsaved.y - lnew.y) < lclosenuff)
                     caught_a_cycle = 1;
            }
            else if (bf_math == BIGNUM)
            {
               if (cmp_bn(abs_a_bn(sub_bn(bntmp,bnsaved.x,bnnew.x)), bnclosenuff) < 0)
                  if (cmp_bn(abs_a_bn(sub_bn(bntmp,bnsaved.y,bnnew.y)), bnclosenuff) < 0)
                     caught_a_cycle = 1;
            }
            else if (bf_math == BIGFLT)
            {
               if (cmp_bf(abs_a_bf(sub_bf(bftmp,bfsaved.x,bfnew.x)), bfclosenuff) < 0)
                  if (cmp_bf(abs_a_bf(sub_bf(bftmp,bfsaved.y,bfnew.y)), bfclosenuff) < 0)
                     caught_a_cycle = 1;
            }
            else
            {
               if (fabs(saved.x - new.x) < closenuff)
                  if (fabs(saved.y - new.y) < closenuff)
                     caught_a_cycle = 1;
#ifdef NUMSAVED
               int i;
                for(i=0;i<=zctr;i++)
                {
                   if(caught[i] == 0)
                   {
                      if (fabs(savedz[i].x - new.x) < closenuff)
                         if (fabs(savedz[i].y - new.y) < closenuff)
                             caught[i] = coloriter;
                   }
                }
#endif
            }
            if(caught_a_cycle)
            {
#ifdef NUMSAVED
                char msg[MSGLEN];
                static FILE *fp = NULL;
                static char c;
                if(fp==NULL)
                   fp = dir_fopen(workdir,"cycles.txt","w");
#endif
                cyclelen = coloriter-savedcoloriter;
#ifdef NUMSAVED
                fprintf(fp,"row %3d col %3d len %6ld iter %6ld savedand %6ld\n",
                    row,col,cyclelen,coloriter,savedand);
                if(zctr > 1 && zctr < NUMSAVED)
                {
                   int i;
                   for(i=0;i<zctr;i++)
                      fprintf(fp,"   caught %2d saved %6ld iter %6ld\n",i,changed[i],caught[i]);
                }
                fflush(fp);
#endif
                coloriter = maxit - 1;
            }

         }
      }
   }  /* end while (coloriter++ < maxit) */

   if (show_orbit)
      scrub_orbit();

   realcoloriter = coloriter;           /* save this before we start adjusting it */
   if (coloriter >= maxit)
      oldcoloriter = 0;         /* check periodicity immediately next time */
   else
   {
      oldcoloriter = coloriter + 10;    /* check when past this + 10 next time */
      if (coloriter == 0)
         coloriter = 1;         /* needed to make same as calcmand */
   }

   if (potflag)
   {
      if (integerfractal)       /* adjust integer fractals */
      {
         new.x = ((double)lnew.x) / fudge;
         new.y = ((double)lnew.y) / fudge;
      }
      else if (bf_math==BIGNUM)
      {
         new.x = (double)bntofloat(bnnew.x);
         new.y = (double)bntofloat(bnnew.y);
      }
      else if (bf_math==BIGFLT)
      {
         new.x = (double)bftofloat(bfnew.x);
         new.y = (double)bftofloat(bfnew.y);
      }
      magnitude = sqr(new.x) + sqr(new.y);
      coloriter = potential(magnitude, coloriter);
      if (LogTable || Log_Calc)
         coloriter = logtablecalc(coloriter);
      goto plot_pixel;          /* skip any other adjustments */
   }

   if (coloriter >= maxit)              /* an "inside" point */
      goto plot_inside;         /* distest, decomp, biomorph don't apply */


   if (outside < -1)  /* these options by Richard Hughes modified by TW */
   {
      if (integerfractal)
      {
         new.x = ((double)lnew.x) / fudge;
         new.y = ((double)lnew.y) / fudge;
      }
      else if(bf_math==1)
      {
         new.x = (double)bntofloat(bnnew.x);
         new.y = (double)bntofloat(bnnew.y);
      }
      /* Add 7 to overcome negative values on the MANDEL    */
      if (outside == REAL)               /* "real" */
         coloriter += (long)new.x + 7;
      else if (outside == IMAG)          /* "imag" */
         coloriter += (long)new.y + 7;
      else if (outside == MULT  && new.y)  /* "mult" */
          coloriter = (long)((double)coloriter * (new.x/new.y));
      else if (outside == SUM)           /* "sum" */
          coloriter += (long)(new.x + new.y);
      else if (outside == ATAN)          /* "atan" */
          coloriter = (long)fabs(atan2(new.y,new.x)*atan_colors/PI);
      else if (outside == FMOD)
          coloriter = (long)(memvalue * colors / closeprox);
      else if (outside == TDIS) {
          coloriter = (long)(totaldist);
}


      /* eliminate negative colors & wrap arounds */
      if ((coloriter <= 0 || coloriter > maxit) && outside!=FMOD) {
         if (save_release < 1961)
             coloriter = 0;
         else
             coloriter = 1;
      }
   }

   if (distest)
   {
      double dist,temp;
      dist = sqr(new.x) + sqr(new.y);
      if (dist == 0 || overflow)
         dist = 0;
      else {
         temp = log(dist);
         dist = dist * sqr(temp) / ( sqr(deriv.x) + sqr(deriv.y) );
      }
      if (dist < dem_delta)     /* point is on the edge */
      {
         if (distest > 0)
            goto plot_inside;   /* show it as an inside point */
         coloriter = 0 - distest;       /* show boundary as specified color */
         goto plot_pixel;       /* no further adjustments apply */
      }
      if (colors == 2)
      {
         coloriter = !inside;   /* the only useful distest 2 color use */
         goto plot_pixel;       /* no further adjustments apply */
      }
      if (distest > 1)          /* pick color based on distance */
      {
         if (old_demm_colors) /* this one is needed for old color scheme */
            coloriter = (long)sqrt(sqrt(dist) / dem_width + 1);
         else if (use_old_distest)
            coloriter = (long)sqrt(dist / dem_width + 1);
         else
            coloriter = (long)(dist / dem_width + 1);
         coloriter &= LONG_MAX;  /* oops - color can be negative */
         goto plot_pixel;       /* no further adjustments apply */
      }
      if (use_old_distest) {
         coloriter = dem_color;
         new = dem_new;
      }
      /* use pixel's "regular" color */
   }

   if (decomp[0] > 0)
      decomposition();
   else if (biomorph != -1)
   {
      if (integerfractal)
      {
         if (labs(lnew.x) < llimit2 || labs(lnew.y) < llimit2)
            coloriter = biomorph;
      }
      else
         if (fabs(new.x) < rqlim2 || fabs(new.y) < rqlim2)
            coloriter = biomorph;
   }

   if (outside >= 0 && attracted == FALSE) /* merge escape-time stripes */
      coloriter = outside;
   else if (LogTable || Log_Calc)
            coloriter = logtablecalc(coloriter);
   goto plot_pixel;

   plot_inside: /* we're "inside" */
   if (periodicitycheck < 0 && caught_a_cycle)
      coloriter = 7;           /* show periodicity */
   else if (inside >= 0)
      coloriter = inside;              /* set to specified color, ignore logpal */
   else
   {
      if(inside == STARTRAIL)
      {
         int i;
         double diff;
         coloriter = 0;
         for(i=1;i<16;i++)
         {
            diff = tantable[0] - tantable[i];
            if(fabs(diff) < .05)
            {
               coloriter = i;
               break;
            }
         }
      }
      else if(inside== PERIOD) {
          if (cyclelen>0) {
              coloriter = cyclelen;
          } else {
              coloriter = maxit;
          }
      }
      else if(inside == EPSCROSS)
      {
         if(hooper==1)
            coloriter = green;
         else if(hooper==2)
            coloriter = yellow;
         else if( hooper==0)
            coloriter = maxit;
         if (show_orbit)
            scrub_orbit();
      }
      else if(inside == FMODI)
      {
         coloriter = (long)(memvalue * colors / closeprox);
      }
      else if (inside == ATANI)          /* "atan" */
         if (integerfractal) {
            new.x = ((double)lnew.x) / fudge;
            new.y = ((double)lnew.y) / fudge;
            coloriter = (long)fabs(atan2(new.y,new.x)*atan_colors/PI);
         }
         else
            coloriter = (long)fabs(atan2(new.y,new.x)*atan_colors/PI);
      else if (inside == BOF60)
         coloriter = (long)(sqrt(min_orbit) * 75);
      else if (inside == BOF61)
         coloriter = min_index;
      else if (inside == ZMAG)
      {
         if (integerfractal)
         {
            /*
            new.x = ((double)lnew.x) / fudge;
            new.y = ((double)lnew.y) / fudge;
            coloriter = (long)((((double)lsqr(lnew.x))/fudge + ((double)lsqr(lnew.y))/fudge) * (maxit>>1) + 1);
            */
            coloriter = (long)(((double)lmagnitud/fudge) * (maxit>>1) + 1);
         }
         else
            coloriter = (long)((sqr(new.x) + sqr(new.y)) * (maxit>>1) + 1);
      }
      else /* inside == -1 */
         coloriter = maxit;
      if (LogTable || Log_Calc)
         coloriter = logtablecalc(coloriter);
   }

   plot_pixel:

   color = abs((int)coloriter);
   if (coloriter >= colors) { /* don't use color 0 unless from inside/outside */
      if (save_release <= 1950) {
         if (colors < 16)
            color &= andcolor;
         else
            color = ((color - 1) % andcolor) + 1;  /* skip color zero */
      }
      else {
         if (colors < 16)
            color = (int)(coloriter & andcolor);
         else
            color = (int)(((coloriter - 1) % andcolor) + 1);
      }
   }
   if(debugflag != 470)
      if(color <= 0 && stdcalcmode == 'b' )   /* fix BTM bug */
         color = 1;
   (*plot) (col, row, color);

   maxit = savemaxit;
   if ((kbdcount -= abs((int)realcoloriter)) <= 0)
   {
      if (check_key())
         return (-1);
      kbdcount = max_kbdcount;
   }
   return (color);
}
#undef green
#undef yellow

#define cos45  sin45
#define lcos45 lsin45

/**************** standardfractal doodad subroutines *********************/
static void decomposition(void)
{
/* static double cos45     = 0.70710678118654750; */ /* cos 45  degrees */
   static double sin45     = 0.70710678118654750; /* sin 45     degrees */
   static double cos22_5   = 0.92387953251128670; /* cos 22.5   degrees */
   static double sin22_5   = 0.38268343236508980; /* sin 22.5   degrees */
   static double cos11_25  = 0.98078528040323040; /* cos 11.25  degrees */
   static double sin11_25  = 0.19509032201612820; /* sin 11.25  degrees */
   static double cos5_625  = 0.99518472667219690; /* cos 5.625  degrees */
   static double sin5_625  = 0.09801714032956060; /* sin 5.625  degrees */
   static double tan22_5   = 0.41421356237309500; /* tan 22.5   degrees */
   static double tan11_25  = 0.19891236737965800; /* tan 11.25  degrees */
   static double tan5_625  = 0.09849140335716425; /* tan 5.625  degrees */
   static double tan2_8125 = 0.04912684976946725; /* tan 2.8125 degrees */
   static double tan1_4063 = 0.02454862210892544; /* tan 1.4063 degrees */
/* static long lcos45     ;*/ /* cos 45   degrees */
   static long lsin45     ; /* sin 45     degrees */
   static long lcos22_5   ; /* cos 22.5   degrees */
   static long lsin22_5   ; /* sin 22.5   degrees */
   static long lcos11_25  ; /* cos 11.25  degrees */
   static long lsin11_25  ; /* sin 11.25  degrees */
   static long lcos5_625  ; /* cos 5.625  degrees */
   static long lsin5_625  ; /* sin 5.625  degrees */
   static long ltan22_5   ; /* tan 22.5   degrees */
   static long ltan11_25  ; /* tan 11.25  degrees */
   static long ltan5_625  ; /* tan 5.625  degrees */
   static long ltan2_8125 ; /* tan 2.8125 degrees */
   static long ltan1_4063 ; /* tan 1.4063 degrees */
   static long reset_fudge = -1;
   int temp = 0;
   int save_temp = 0;
   int i;
   _LCMPLX lalt;
   _CMPLX alt;
   coloriter = 0;
   if (integerfractal) /* the only case */
   {
      if (reset_fudge != fudge)
      {
         reset_fudge = fudge;
         /* lcos45     = (long)(cos45 * fudge); */
         lsin45     = (long)(sin45 * fudge);
         lcos22_5   = (long)(cos22_5 * fudge);
         lsin22_5   = (long)(sin22_5 * fudge);
         lcos11_25  = (long)(cos11_25 * fudge);
         lsin11_25  = (long)(sin11_25 * fudge);
         lcos5_625  = (long)(cos5_625 * fudge);
         lsin5_625  = (long)(sin5_625 * fudge);
         ltan22_5   = (long)(tan22_5 * fudge);
         ltan11_25  = (long)(tan11_25 * fudge);
         ltan5_625  = (long)(tan5_625 * fudge);
         ltan2_8125 = (long)(tan2_8125 * fudge);
         ltan1_4063 = (long)(tan1_4063 * fudge);
      }
      if (lnew.y < 0)
      {
         temp = 2;
         lnew.y = -lnew.y;
      }

      if (lnew.x < 0)
      {
         ++temp;
         lnew.x = -lnew.x;
      }
      if (decomp[0] == 2 && save_release >= 1827)
      {
        save_temp = temp;
        if(temp==2) save_temp = 3;
        if(temp==3) save_temp = 2;
      }

      if (decomp[0] >= 8)
      {
         temp <<= 1;
         if (lnew.x < lnew.y)
         {
            ++temp;
            lalt.x = lnew.x; /* just */
            lnew.x = lnew.y; /* swap */
            lnew.y = lalt.x; /* them */
         }

         if (decomp[0] >= 16)
         {
            temp <<= 1;
            if (multiply(lnew.x,ltan22_5,bitshift) < lnew.y)
            {
               ++temp;
               lalt = lnew;
               lnew.x = multiply(lalt.x,lcos45,bitshift) +
                   multiply(lalt.y,lsin45,bitshift);
               lnew.y = multiply(lalt.x,lsin45,bitshift) -
                   multiply(lalt.y,lcos45,bitshift);
            }

            if (decomp[0] >= 32)
            {
               temp <<= 1;
               if (multiply(lnew.x,ltan11_25,bitshift) < lnew.y)
               {
                  ++temp;
                  lalt = lnew;
                  lnew.x = multiply(lalt.x,lcos22_5,bitshift) +
                      multiply(lalt.y,lsin22_5,bitshift);
                  lnew.y = multiply(lalt.x,lsin22_5,bitshift) -
                      multiply(lalt.y,lcos22_5,bitshift);
               }

               if (decomp[0] >= 64)
               {
                  temp <<= 1;
                  if (multiply(lnew.x,ltan5_625,bitshift) < lnew.y)
                  {
                     ++temp;
                     lalt = lnew;
                     lnew.x = multiply(lalt.x,lcos11_25,bitshift) +
                         multiply(lalt.y,lsin11_25,bitshift);
                     lnew.y = multiply(lalt.x,lsin11_25,bitshift) -
                         multiply(lalt.y,lcos11_25,bitshift);
                  }

                  if (decomp[0] >= 128)
                  {
                     temp <<= 1;
                     if (multiply(lnew.x,ltan2_8125,bitshift) < lnew.y)
                     {
                        ++temp;
                        lalt = lnew;
                        lnew.x = multiply(lalt.x,lcos5_625,bitshift) +
                            multiply(lalt.y,lsin5_625,bitshift);
                        lnew.y = multiply(lalt.x,lsin5_625,bitshift) -
                            multiply(lalt.y,lcos5_625,bitshift);
                     }

                     if (decomp[0] == 256)
                     {
                        temp <<= 1;
                        if (multiply(lnew.x,ltan1_4063,bitshift) < lnew.y)
                           if ((lnew.x*ltan1_4063 < lnew.y))
                              ++temp;
                     }
                  }
               }
            }
         }
      }
   }
   else /* double case */
   {
      if (new.y < 0)
      {
         temp = 2;
         new.y = -new.y;
      }
      if (new.x < 0)
      {
         ++temp;
         new.x = -new.x;
      }
      if (decomp[0] == 2 && save_release >= 1827)
      {
        save_temp = temp;
        if(temp==2) save_temp = 3;
        if(temp==3) save_temp = 2;
      }
      if (decomp[0] >= 8)
      {
         temp <<= 1;
         if (new.x < new.y)
         {
            ++temp;
            alt.x = new.x; /* just */
            new.x = new.y; /* swap */
            new.y = alt.x; /* them */
         }
         if (decomp[0] >= 16)
         {
            temp <<= 1;
            if (new.x*tan22_5 < new.y)
            {
               ++temp;
               alt = new;
               new.x = alt.x*cos45 + alt.y*sin45;
               new.y = alt.x*sin45 - alt.y*cos45;
            }

            if (decomp[0] >= 32)
            {
               temp <<= 1;
               if (new.x*tan11_25 < new.y)
               {
                  ++temp;
                  alt = new;
                  new.x = alt.x*cos22_5 + alt.y*sin22_5;
                  new.y = alt.x*sin22_5 - alt.y*cos22_5;
               }

               if (decomp[0] >= 64)
               {
                  temp <<= 1;
                  if (new.x*tan5_625 < new.y)
                  {
                     ++temp;
                     alt = new;
                     new.x = alt.x*cos11_25 + alt.y*sin11_25;
                     new.y = alt.x*sin11_25 - alt.y*cos11_25;
                  }

                  if (decomp[0] >= 128)
                  {
                     temp <<= 1;
                     if (new.x*tan2_8125 < new.y)
                     {
                        ++temp;
                        alt = new;
                        new.x = alt.x*cos5_625 + alt.y*sin5_625;
                        new.y = alt.x*sin5_625 - alt.y*cos5_625;
                     }

                     if (decomp[0] == 256)
                     {
                        temp <<= 1;
                        if ((new.x*tan1_4063 < new.y))
                           ++temp;
                     }
                  }
               }
            }
         }
      }
   }
   for (i = 1; temp > 0; ++i)
   {
      if (temp & 1)
         coloriter = (1 << i) - 1 - coloriter;
      temp >>= 1;
   }
   if (decomp[0] == 2 && save_release >= 1827) {
      if(save_temp & 2) coloriter = 1;
      else coloriter = 0;
      if (colors == 2)
          coloriter++;
   }
   else if (decomp[0] == 2 && save_release < 1827)
      coloriter &= 1;
   if (colors > decomp[0])
      coloriter++;
}

/******************************************************************/
/* Continuous potential calculation for Mandelbrot and Julia      */
/* Reference: Science of Fractal Images p. 190.                   */
/* Special thanks to Mark Peterson for his "MtMand" program that  */
/* beautifully approximates plate 25 (same reference) and spurred */
/* on the inclusion of similar capabilities in FRACTINT.          */
/*                                                                */
/* The purpose of this function is to calculate a color value     */
/* for a fractal that varies continuously with the screen pixels  */
/* locations for better rendering in 3D.                          */
/*                                                                */
/* Here "magnitude" is the modulus of the orbit value at          */
/* "iterations". The potparms[] are user-entered paramters        */
/* controlling the level and slope of the continuous potential    */
/* surface. Returns color.  - Tim Wegner 6/25/89                  */
/*                                                                */
/*                     -- Change history --                       */
/*                                                                */
/* 09/12/89   - added floatflag support and fixed float underflow */
/*                                                                */
/******************************************************************/

static int _fastcall potential(double mag, long iterations)
{
   float f_mag,f_tmp,pot;
   double d_tmp;
   int i_pot;
   long l_pot;

   if(iterations < maxit)
   {
      pot = (float)(l_pot = iterations+2);
      if(l_pot <= 0 || mag <= 1.0)
         pot = (float)0.0;
      else /* pot = log(mag) / pow(2.0, (double)pot); */
      {
         if(l_pot < 120 && !floatflag) /* empirically determined limit of fShift */
         {
            f_mag = (float)mag;
            fLog14(f_mag,f_tmp); /* this SHOULD be non-negative */
            fShift(f_tmp,(char)-l_pot,pot);
         }
         else
         {
            d_tmp = log(mag)/(double)pow(2.0,(double)pot);
            if(d_tmp > FLT_MIN) /* prevent float type underflow */
               pot = (float)d_tmp;
            else
               pot = (float)0.0;
         }
      }
      /* following transformation strictly for aesthetic reasons */
      /* meaning of parameters:
            potparam[0] -- zero potential level - highest color -
            potparam[1] -- slope multiplier -- higher is steeper
            potparam[2] -- rqlim value if changeable (bailout for modulus) */

      if(pot > 0.0)
      {
         if(floatflag)
            pot = (float)sqrt((double)pot);
         else
         {
            fSqrt14(pot,f_tmp);
            pot = f_tmp;
         }
         pot = (float)(potparam[0] - pot*potparam[1] - 1.0);
      }
      else
         pot = (float)(potparam[0] - 1.0);
      if(pot < 1.0)
         pot = (float)1.0; /* avoid color 0 */
   }
   else if(inside >= 0)
      pot = inside;
   else /* inside < 0 implies inside=maxit, so use 1st pot param instead */
      pot = (float)potparam[0];

   i_pot = (int)((l_pot = (long)(pot * 256)) >> 8);
   if(i_pot >= colors)
   {
      i_pot = colors - 1;
      l_pot = 255;
   }

   if(pot16bit)
   {
      if (dotmode != 11) /* if putcolor won't be doing it for us */
         writedisk(col+sxoffs,row+syoffs,i_pot);
      writedisk(col+sxoffs,row+sydots+syoffs,(int)l_pot);
   }

   return(i_pot);
}


/******************* boundary trace method ***************************
Fractint's original btm was written by David Guenther.  There were a few
rare circumstances in which the original btm would not trace or fill
correctly, even on Mandelbrot Sets.  The code below was adapted from
"Mandelbrot Sets by Wesley Loewer" (see calmanfp.asm) which was written
before I was introduced to Fractint.  It should be noted that without
David Guenther's implimentation of a btm, I doubt that I would have been
able to impliment my own code into Fractint.  There are several things in
the following code that are not original with me but came from David
Guenther's code.  I've noted these places with the initials DG.

                                        Wesley Loewer 3/8/92
*********************************************************************/
#define bkcolor 0  /* I have some ideas for the future with this. -Wes */
#define advance_match()     coming_from = ((going_to = (going_to - 1) & 0x03) - 1) & 0x03
#define advance_no_match()  going_to = (going_to + 1) & 0x03

static
int  bound_trace_main(void)
    {
    enum direction coming_from;
    unsigned int match_found, continue_loop;
    int trail_color, fillcolor_used, last_fillcolor_used = -1;
    int max_putline_length;
    int right, left, length;
    static FCODE btm_cantbeused[]={"Boundary tracing cannot be used with "};
    if (inside == 0 || outside == 0)
        {
        static FCODE inside_outside[] = {"inside=0 or outside=0"};
        char msg[MSGLEN];
        far_strcpy(msg,btm_cantbeused);
        far_strcat(msg,inside_outside);
        stopmsg(0,msg);
        return(-1);
        }
    if (colors < 16)
        {
        char msg[MSGLEN];
        static FCODE lessthansixteen[] = {"< 16 colors"};
        far_strcpy(msg,btm_cantbeused);
        far_strcat(msg,lessthansixteen);
        stopmsg(0,msg);
        return(-1);
        }

    got_status = 2;
    max_putline_length = 0; /* reset max_putline_length */
    for (currow = iystart; currow <= iystop; currow++)
        {
        reset_periodicity = 1; /* reset for a new row */
        color = bkcolor;
        for (curcol = ixstart; curcol <= ixstop; curcol++)
            {
            if (getcolor(curcol, currow) != bkcolor)
                continue;

            trail_color = color;
            row = currow;
            col = curcol;
            if ((*calctype)()== -1) /* color, row, col are global */
                {
                if (showdot != bkcolor) /* remove showdot pixel */
                   (*plot)(col,row,bkcolor);
                if (iystop != yystop)  /* DG */
                   iystop = yystop - (currow - yystart); /* allow for sym */
                add_worklist(xxstart,xxstop,curcol,currow,iystop,currow,0,worksym);
                return -1;
                }
            reset_periodicity = 0; /* normal periodicity checking */

            /*
            This next line may cause a few more pixels to be calculated,
            but at the savings of quite a bit of overhead
            */
            if (color != trail_color)  /* DG */
                continue;

            /* sweep clockwise to trace outline */
            trail_row = currow;
            trail_col = curcol;
            trail_color = color;
            fillcolor_used = fillcolor > 0 ? fillcolor : trail_color;
            coming_from = West;
            going_to = East;
            match_found = 0;
            continue_loop = TRUE;
            do
                {
                step_col_row();
                if (row >= currow
                        && col >= ixstart
                        && col <= ixstop
                        && row <= iystop)
                    {
                    /* the order of operations in this next line is critical */
                    if ((color = getcolor(col, row)) == bkcolor && (*calctype)()== -1)
                                /* color, row, col are global for (*calctype)() */
                        {
                        if (showdot != bkcolor) /* remove showdot pixel */
                           (*plot)(col,row,bkcolor);
                        if (iystop != yystop)  /* DG */
                           iystop = yystop - (currow - yystart); /* allow for sym */
                        add_worklist(xxstart,xxstop,curcol,currow,iystop,currow,0,worksym);
                        return -1;
                        }
                    else if (color == trail_color)
                        {
                        if (match_found < 4) /* to keep it from overflowing */
                                match_found++;
                        trail_row = row;
                        trail_col = col;
                        advance_match();
                        }
                    else
                        {
                        advance_no_match();
                        continue_loop = going_to != coming_from || match_found;
                        }
                    }
                else
                    {
                    advance_no_match();
                    continue_loop = going_to != coming_from || match_found;
                    }
                } while (continue_loop && (col != curcol || row != currow));

            if (match_found <= 3)  /* DG */
                { /* no hole */
                color = bkcolor;
                reset_periodicity = 1;
                continue;
                }

/*
Fill in region by looping around again, filling lines to the left
whenever going_to is South or West
*/
            trail_row = currow;
            trail_col = curcol;
            coming_from = West;
            going_to = East;
            do
                {
                match_found = FALSE;
                do
                    {
                    step_col_row();
                    if (row >= currow
                            && col >= ixstart
                            && col <= ixstop
                            && row <= iystop
                            && getcolor(col,row) == trail_color)
                              /* getcolor() must be last */
                        {
                        if (going_to == South
                                || (going_to == West && coming_from != East))
                            { /* fill a row, but only once */
                            right = col;
                            while (--right >= ixstart && (color = getcolor(right,row)) == trail_color)
                                ; /* do nothing */
                            if (color == bkcolor) /* check last color */
                                {
                                left = right;
                                while (getcolor(--left,row) == bkcolor)
                                      /* Should NOT be possible for left < ixstart */
                                    ; /* do nothing */
                                left++; /* one pixel too far */
                                if (right == left) /* only one hole */
                                    (*plot)(left,row,fillcolor_used);
                                else
                                    { /* fill the line to the left */
                                    length=right-left+1;
                                    if (fillcolor_used != last_fillcolor_used || length > max_putline_length)
                                        { /* only reset dstack if necessary */
                                        memset(dstack,fillcolor_used,length);
                                        last_fillcolor_used = fillcolor_used;
                                        max_putline_length = length;
                                        }
                                    sym_fill_line(row, left, right, dstack);
                                    }
                                } /* end of fill line */

#if 0 /* don't interupt with a check_key() during fill */
                            if(--kbdcount<=0)
                                {
                                if(check_key())
                                    {
                                    if (iystop != yystop)
                                       iystop = yystop - (currow - yystart); /* allow for sym */
                                    add_worklist(xxstart,xxstop,curcol,currow,iystop,currow,0,worksym);
                                    return(-1);
                                    }
                                kbdcount=max_kbdcount;
                                }
#endif
                            }
                        trail_row = row;
                        trail_col = col;
                        advance_match();
                        match_found = TRUE;
                        }
                    else
                        advance_no_match();
                    } while (!match_found && going_to != coming_from);

                if (!match_found)
                    { /* next one has to be a match */
                    step_col_row();
                    trail_row = row;
                    trail_col = col;
                    advance_match();
                    }
                } while (trail_col != curcol || trail_row != currow);
            reset_periodicity = 1; /* reset after a trace/fill */
            color = bkcolor;
            }
        }
    return 0;
    }

/*******************************************************************/
/* take one step in the direction of going_to */
static void step_col_row()
    {
    switch (going_to)
        {
        case North:
            col = trail_col;
            row = trail_row - 1;
            break;
        case East:
            col = trail_col + 1;
            row = trail_row;
            break;
        case South:
            col = trail_col;
            row = trail_row + 1;
            break;
        case West:
            col = trail_col - 1;
            row = trail_row;
            break;
        }
    }

/******************* end of boundary trace method *******************/


/************************ super solid guessing *****************************/

/*
   I, Timothy Wegner, invented this solidguessing idea and implemented it in
   more or less the overall framework you see here.  I am adding this note
   now in a possibly vain attempt to secure my place in history, because
   Pieter Branderhorst has totally rewritten this routine, incorporating
   a *MUCH* more sophisticated algorithm.  His revised code is not only
   faster, but is also more accurate. Harrumph!
*/

static int solidguess(void)
{
   int i,x,y,xlim,ylim,blocksize;
   unsigned int *pfxp0,*pfxp1;
   unsigned int u;

   guessplot=(plot!=putcolor && plot!=symplot2 && plot!=symplot2J);
   /* check if guessing at bottom & right edges is ok */
   bottom_guess = (plot == symplot2 || (plot == putcolor && iystop+1 == ydots));
   right_guess  = (plot == symplot2J
       || ((plot == putcolor || plot == symplot2) && ixstop+1 == xdots));

   /* there seems to be a bug in solid guessing at bottom and side */
   if(debugflag != 472)
      bottom_guess = right_guess = 0;  /* TIW march 1995 */

   i = maxblock = blocksize = ssg_blocksize();
   totpasses = 1;
   while ((i >>= 1) > 1) ++totpasses;

   /* ensure window top and left are on required boundary, treat window
         as larger than it really is if necessary (this is the reason symplot
         routines must check for > xdots/ydots before plotting sym points) */
   ixstart &= -1 - (maxblock-1);
   iystart = yybegin;
   iystart &= -1 - (maxblock-1);

   got_status = 1;

   if (workpass == 0) /* otherwise first pass already done */
   {
      /* first pass, calc every blocksize**2 pixel, quarter result & paint it */
      curpass = 1;
      if (iystart <= yystart) /* first time for this window, init it */
      {
         currow = 0;
         memset(&tprefix[1][0][0],0,maxxblk*maxyblk*2); /* noskip flags off */
         reset_periodicity = 1;
         row=iystart;
         for(col=ixstart; col<=ixstop; col+=maxblock)
         { /* calc top row */
            if((*calctype)()== -1)
            {
               add_worklist(xxstart,xxstop,xxbegin,yystart,yystop,yybegin,0,worksym);
               goto exit_solidguess;
            }
            reset_periodicity = 0;
         }
      }
      else
         memset(&tprefix[1][0][0],-1,maxxblk*maxyblk*2); /* noskip flags on */
      for(y=iystart; y<=iystop; y+=blocksize)
      {
         currow = y;
         i = 0;
         if(y+blocksize<=iystop)
         { /* calc the row below */
            row=y+blocksize;
            reset_periodicity = 1;
            for(col=ixstart; col<=ixstop; col+=maxblock)
            {
               if((i=(*calctype)()) == -1)
                  break;
               reset_periodicity = 0;
            }
         }
         reset_periodicity = 0;
         if (i == -1 || guessrow(1,y,blocksize) != 0) /* interrupted? */
         {
            if (y < yystart)
               y = yystart;
            add_worklist(xxstart,xxstop,xxstart,yystart,yystop,y,0,worksym);
            goto exit_solidguess;
         }
      }

      if (num_worklist) /* work list not empty, just do 1st pass */
      {
         add_worklist(xxstart,xxstop,xxstart,yystart,yystop,yystart,1,worksym);
         goto exit_solidguess;
      }
      ++workpass;
      iystart = yystart & (-1 - (maxblock-1));

      /* calculate skip flags for skippable blocks */
      xlim=(ixstop+maxblock)/maxblock+1;
      ylim=((iystop+maxblock)/maxblock+15)/16+1;
      if(right_guess==0) /* no right edge guessing, zap border */
         for(y=0;y<=ylim;++y)
            tprefix[1][y][xlim]= 0xffff;
      if(bottom_guess==0) /* no bottom edge guessing, zap border */
      {
         i=(iystop+maxblock)/maxblock+1;
         y=i/16+1;
         i=1<<(i&15);
         for(x=0;x<=xlim;++x)
            tprefix[1][y][x]|=i;
      }
      /* set each bit in tprefix[0] to OR of it & surrounding 8 in tprefix[1] */
      for(y=0;++y<ylim;)
      {
         pfxp0= (unsigned int *)&tprefix[0][y][0];
         pfxp1= (unsigned int *)&tprefix[1][y][0];
         for(x=0;++x<xlim;)
         {
            ++pfxp1;
            u= *(pfxp1-1)|*pfxp1|*(pfxp1+1);
            *(++pfxp0)=u|(u>>1)|(u<<1)
               |((*(pfxp1-(maxxblk+1))|*(pfxp1-maxxblk)|*(pfxp1-(maxxblk-1)))>>15)
                  |((*(pfxp1+(maxxblk-1))|*(pfxp1+maxxblk)|*(pfxp1+(maxxblk+1)))<<15);
         }
      }
   }
   else /* first pass already done */
      memset(&tprefix[0][0][0],-1,maxxblk*maxyblk*2); /* noskip flags on */
   if(three_pass)
      goto exit_solidguess;

   /* remaining pass(es), halve blocksize & quarter each blocksize**2 */
   i = workpass;
   while (--i > 0) /* allow for already done passes */
      blocksize = blocksize>>1;
   reset_periodicity = 0;
   while((blocksize=blocksize>>1)>=2)
   {
      if(stoppass > 0)
         if(workpass >= stoppass)
            goto exit_solidguess;
      curpass = workpass + 1;
      for(y=iystart; y<=iystop; y+=blocksize)
      {
         currow = y;
         if(guessrow(0,y,blocksize) != 0)
         {
            if (y < yystart)
               y = yystart;
            add_worklist(xxstart,xxstop,xxstart,yystart,yystop,y,workpass,worksym);
            goto exit_solidguess;
         }
      }
      ++workpass;
      if (num_worklist /* work list not empty, do one pass at a time */
      && blocksize>2) /* if 2, we just did last pass */
      {
         add_worklist(xxstart,xxstop,xxstart,yystart,yystop,yystart,workpass,worksym);
         goto exit_solidguess;
      }
      iystart = yystart & (-1 - (maxblock-1));
   }

   exit_solidguess:
   return(0);
}

#define calcadot(c,x,y) { col=x; row=y; if((c=(*calctype)())== -1) return -1; }

static int _fastcall guessrow(int firstpass,int y,int blocksize)
{
   int x,i,j,color;
   int xplushalf,xplusblock;
   int ylessblock,ylesshalf,yplushalf,yplusblock;
   int     c21,c31,c41;         /* cxy is the color of pixel at (x,y) */
   int c12,c22,c32,c42;         /* where c22 is the topleft corner of */
   int c13,c23,c33;             /* the block being handled in current */
   int     c24,    c44;         /* iteration                          */
   int guessed23,guessed32,guessed33,guessed12,guessed13;
   int prev11,fix21,fix31;
   unsigned int *pfxptr,pfxmask;

   c44 = c41 = c42 = 0;  /* just for warning */

   halfblock=blocksize>>1;
   i=y/maxblock;
   pfxptr= (unsigned int *)&tprefix[firstpass][(i>>4)+1][ixstart/maxblock];
   pfxmask=1<<(i&15);
   ylesshalf=y-halfblock;
   ylessblock=y-blocksize; /* constants, for speed */
   yplushalf=y+halfblock;
   yplusblock=y+blocksize;
   prev11= -1;
   c24=c12=c13=c22=getcolor(ixstart,y);
   c31=c21=getcolor(ixstart,(y>0)?ylesshalf:0);
   if(yplusblock<=iystop)
      c24=getcolor(ixstart,yplusblock);
   else if(bottom_guess==0)
      c24= -1;
   guessed12=guessed13=0;

   for(x=ixstart; x<=ixstop;)  /* increment at end, or when doing continue */
   {
      if((x&(maxblock-1))==0)  /* time for skip flag stuff */
      {
         ++pfxptr;
         if(firstpass==0 && (*pfxptr&pfxmask)==0)  /* check for fast skip */
         {
            /* next useful in testing to make skips visible */
            /*
            if(halfblock==1)
            {
               (*plot)(x+1,y,0); (*plot)(x,y+1,0); (*plot)(x+1,y+1,0);
            }
            */
            x+=maxblock;
            prev11=c31=c21=c24=c12=c13=c22;
            guessed12=guessed13=0;
            continue;
         }
      }

      if(firstpass)  /* 1st pass, paint topleft corner */
         plotblock(0,x,y,c22);
      /* setup variables */
      xplusblock=(xplushalf=x+halfblock)+halfblock;
      if(xplushalf>ixstop)
      {
         if(right_guess==0)
            c31= -1;
      }
      else if(y>0)
         c31=getcolor(xplushalf,ylesshalf);
      if(xplusblock<=ixstop)
      {
         if(yplusblock<=iystop)
            c44=getcolor(xplusblock,yplusblock);
         c41=getcolor(xplusblock,(y>0)?ylesshalf:0);
         c42=getcolor(xplusblock,y);
      }
      else if(right_guess==0)
         c41=c42=c44= -1;
      if(yplusblock>iystop)
         c44=(bottom_guess)?c42:-1;

      /* guess or calc the remaining 3 quarters of current block */
      guessed23=guessed32=guessed33=1;
      c23=c32=c33=c22;
      if(yplushalf>iystop)
      {
         if(bottom_guess==0)
            c23=c33= -1;
         guessed23=guessed33= -1;
         guessed13=0; /* fix for ydots not divisible by four bug TW 2/16/97 */
      }
      if(xplushalf>ixstop)
      {
         if(right_guess==0)
            c32=c33= -1;
         guessed32=guessed33= -1;
      }
      for(;;) /* go around till none of 23,32,33 change anymore */
      {
         if(guessed33>0
             && (c33!=c44 || c33!=c42 || c33!=c24 || c33!=c32 || c33!=c23))
         {
            calcadot(c33,xplushalf,yplushalf);
            guessed33=0;
         }
         if(guessed32>0
             && (c32!=c33 || c32!=c42 || c32!=c31 || c32!=c21
             || c32!=c41 || c32!=c23))
         {
            calcadot(c32,xplushalf,y);
            guessed32=0;
            continue;
         }
         if(guessed23>0
             && (c23!=c33 || c23!=c24 || c23!=c13 || c23!=c12 || c23!=c32))
         {
            calcadot(c23,x,yplushalf);
            guessed23=0;
            continue;
         }
         break;
      }

      if(firstpass) /* note whether any of block's contents were calculated */
         if(guessed23==0 || guessed32==0 || guessed33==0)
            *pfxptr|=pfxmask;

      if(halfblock>1) { /* not last pass, check if something to display */
         if(firstpass)  /* display guessed corners, fill in block */
         {
            if(guessplot)
            {
               if(guessed23>0)
                  (*plot)(x,yplushalf,c23);
               if(guessed32>0)
                  (*plot)(xplushalf,y,c32);
               if(guessed33>0)
                  (*plot)(xplushalf,yplushalf,c33);
            }
            plotblock(1,x,yplushalf,c23);
            plotblock(0,xplushalf,y,c32);
            plotblock(1,xplushalf,yplushalf,c33);
         }
         else  /* repaint changed blocks */
         {
            if(c23!=c22)
               plotblock(-1,x,yplushalf,c23);
            if(c32!=c22)
               plotblock(-1,xplushalf,y,c32);
            if(c33!=c22)
               plotblock(-1,xplushalf,yplushalf,c33);
         }
      }

      /* check if some calcs in this block mean earlier guesses need fixing */
      fix21=((c22!=c12 || c22!=c32)
          && c21==c22 && c21==c31 && c21==prev11
          && y>0
          && (x==ixstart || c21==getcolor(x-halfblock,ylessblock))
          && (xplushalf>ixstop || c21==getcolor(xplushalf,ylessblock))
          && c21==getcolor(x,ylessblock));
      fix31=(c22!=c32
          && c31==c22 && c31==c42 && c31==c21 && c31==c41
          && y>0 && xplushalf<=ixstop
          && c31==getcolor(xplushalf,ylessblock)
          && (xplusblock>ixstop || c31==getcolor(xplusblock,ylessblock))
          && c31==getcolor(x,ylessblock));
      prev11=c31; /* for next time around */
      if(fix21)
      {
         calcadot(c21,x,ylesshalf);
         if(halfblock>1 && c21!=c22)
            plotblock(-1,x,ylesshalf,c21);
      }
      if(fix31)
      {
         calcadot(c31,xplushalf,ylesshalf);
         if(halfblock>1 && c31!=c22)
            plotblock(-1,xplushalf,ylesshalf,c31);
      }
      if(c23!=c22)
      {
         if(guessed12)
         {
            calcadot(c12,x-halfblock,y);
            if(halfblock>1 && c12!=c22)
               plotblock(-1,x-halfblock,y,c12);
         }
         if(guessed13)
         {
            calcadot(c13,x-halfblock,yplushalf);
            if(halfblock>1 && c13!=c22)
               plotblock(-1,x-halfblock,yplushalf,c13);
         }
      }
      c22=c42;
      c24=c44;
      c13=c33;
      c31=c21=c41;
      c12=c32;
      guessed12=guessed32;
      guessed13=guessed33;
      x+=blocksize;
   } /* end x loop */

   if(firstpass==0 || guessplot) return 0;

   /* paint rows the fast way */
   for(i=0;i<halfblock;++i)
   {
      if((j=y+i)<=iystop)
         put_line(j,xxstart,ixstop,&dstack[xxstart]);
      if((j=y+i+halfblock)<=iystop)
         put_line(j,xxstart,ixstop,&dstack[xxstart+OLDMAXPIXELS]);
      if(keypressed()) return -1;
   }
   if(plot!=putcolor)  /* symmetry, just vertical & origin the fast way */
   {
      if(plot==symplot2J) /* origin sym, reverse lines */
         for(i=(ixstop+xxstart+1)/2;--i>=xxstart;)
         {
            color=dstack[i];
            dstack[i]=dstack[j=ixstop-(i-xxstart)];
            dstack[j]=(BYTE)color;
            j+=OLDMAXPIXELS;
            color=dstack[i+OLDMAXPIXELS];
            dstack[i+OLDMAXPIXELS]=dstack[j];
            dstack[j]=(BYTE)color;
         }
      for(i=0;i<halfblock;++i)
      {
         if((j=yystop-(y+i-yystart))>iystop && j<ydots)
            put_line(j,xxstart,ixstop,&dstack[xxstart]);
         if((j=yystop-(y+i+halfblock-yystart))>iystop && j<ydots)
            put_line(j,xxstart,ixstop,&dstack[xxstart+OLDMAXPIXELS]);
         if(keypressed()) return -1;
      }
   }
   return 0;
}

static void _fastcall plotblock(int buildrow,int x,int y,int color)
{
   int i,xlim,ylim;
   if((xlim=x+halfblock)>ixstop)
      xlim=ixstop+1;
   if(buildrow>=0 && guessplot==0) /* save it for later put_line */
   {
      if(buildrow==0)
         for(i=x;i<xlim;++i)
            dstack[i]=(BYTE)color;
      else
         for(i=x;i<xlim;++i)
            dstack[i+OLDMAXPIXELS]=(BYTE)color;
      if (x>=xxstart) /* when x reduced for alignment, paint those dots too */
         return; /* the usual case */
   }
   /* paint it */
   if((ylim=y+halfblock)>iystop)
   {
      if(y>iystop)
         return;
      ylim=iystop+1;
   }
   for(i=x;++i<xlim;)
      (*plot)(i,y,color); /* skip 1st dot on 1st row */
   while(++y<ylim)
      for(i=x;i<xlim;++i)
         (*plot)(i,y,color);
}


/************************* symmetry plot setup ************************/

static int _fastcall xsym_split(int xaxis_row,int xaxis_between)
{
   int i;
   if ((worksym&0x11) == 0x10) /* already decided not sym */
      return(1);
   if ((worksym&1) != 0) /* already decided on sym */
      iystop = (yystart+yystop)/2;
   else /* new window, decide */
   {
      worksym |= 0x10;
      if (xaxis_row <= yystart || xaxis_row >= yystop)
         return(1); /* axis not in window */
      i = xaxis_row + (xaxis_row - yystart);
      if (xaxis_between)
         ++i;
      if (i > yystop) /* split into 2 pieces, bottom has the symmetry */
      {
         if (num_worklist >= MAXCALCWORK-1) /* no room to split */
            return(1);
         iystop = xaxis_row - (yystop - xaxis_row);
         if (!xaxis_between)
            --iystop;
         add_worklist(xxstart,xxstop,xxstart,iystop+1,yystop,iystop+1,workpass,0);
         yystop = iystop;
         return(1); /* tell set_symmetry no sym for current window */
      }
      if (i < yystop) /* split into 2 pieces, top has the symmetry */
      {
         if (num_worklist >= MAXCALCWORK-1) /* no room to split */
            return(1);
         add_worklist(xxstart,xxstop,xxstart,i+1,yystop,i+1,workpass,0);
         yystop = i;
      }
      iystop = xaxis_row;
      worksym |= 1;
   }
   symmetry = 0;
   return(0); /* tell set_symmetry its a go */
}

static int _fastcall ysym_split(int yaxis_col,int yaxis_between)
{
   int i;
   if ((worksym&0x22) == 0x20) /* already decided not sym */
      return(1);
   if ((worksym&2) != 0) /* already decided on sym */
      ixstop = (xxstart+xxstop)/2;
   else /* new window, decide */
   {
      worksym |= 0x20;
      if (yaxis_col <= xxstart || yaxis_col >= xxstop)
         return(1); /* axis not in window */
      i = yaxis_col + (yaxis_col - xxstart);
      if (yaxis_between)
         ++i;
      if (i > xxstop) /* split into 2 pieces, right has the symmetry */
      {
         if (num_worklist >= MAXCALCWORK-1) /* no room to split */
            return(1);
         ixstop = yaxis_col - (xxstop - yaxis_col);
         if (!yaxis_between)
            --ixstop;
         add_worklist(ixstop+1,xxstop,ixstop+1,yystart,yystop,yystart,workpass,0);
         xxstop = ixstop;
         return(1); /* tell set_symmetry no sym for current window */
      }
      if (i < xxstop) /* split into 2 pieces, left has the symmetry */
      {
         if (num_worklist >= MAXCALCWORK-1) /* no room to split */
            return(1);
         add_worklist(i+1,xxstop,i+1,yystart,yystop,yystart,workpass,0);
         xxstop = i;
      }
      ixstop = yaxis_col;
      worksym |= 2;
   }
   symmetry = 0;
   return(0); /* tell set_symmetry its a go */
}

#ifdef _MSC_VER
#pragma optimize ("ea", off)
#endif

#if (_MSC_VER >= 700)
#pragma code_seg ("calcfra1_text")     /* place following in an overlay */
#endif

static void _fastcall setsymmetry(int sym, int uselist) /* set up proper symmetrical plot functions */
{
   int i;
   int parmszero, parmsnoreal, parmsnoimag;
   int xaxis_row, yaxis_col;         /* pixel number for origin */
   int xaxis_between=0, yaxis_between=0; /* if axis between 2 pixels, not on one */
   int xaxis_on_screen=0, yaxis_on_screen=0;
   double ftemp;
   bf_t bft1;
   int saved=0;
   symmetry = 1;
   if(stdcalcmode == 's' || stdcalcmode == 'o')
      return;
   if(sym == NOPLOT && forcesymmetry == 999)
   {
      plot = noplot;
      return;
   }
   /* NOTE: 16-bit potential disables symmetry */
   /* also any decomp= option and any inversion not about the origin */
   /* also any rotation other than 180deg and any off-axis stretch */
   if(bf_math)
      if(cmp_bf(bfxmin,bfx3rd) || cmp_bf(bfymin,bfy3rd))
         return;
   if ((potflag && pot16bit) || (invert && inversion[2] != 0.0)
       || decomp[0] != 0
       || xxmin!=xx3rd || yymin!=yy3rd)
      return;
   if(sym != XAXIS && sym != XAXIS_NOPARM && inversion[1] != 0.0 && forcesymmetry == 999)
      return;
   if(forcesymmetry < 999)
      sym = forcesymmetry;
   else if(forcesymmetry == 1000)
      forcesymmetry = sym;  /* for backwards compatibility */
   else if(outside==REAL || outside==IMAG || outside==MULT || outside==SUM
          || outside==ATAN || bailoutest==Manr || outside==FMOD)
      return;
   else if(inside==FMODI || outside==TDIS)
      return;
   parmszero = (parm.x == 0.0 && parm.y == 0.0 && useinitorbit != 1);
   parmsnoreal = (parm.x == 0.0 && useinitorbit != 1);
   parmsnoimag = (parm.y == 0.0 && useinitorbit != 1);
   switch (fractype)
   { case LMANLAMFNFN:      /* These need only P1 checked. */
     case FPMANLAMFNFN:     /* P2 is used for a switch value */
     case LMANFNFN:         /* These have NOPARM set in fractalp.c, */
     case FPMANFNFN:        /* but it only applies to P1. */
     case FPMANDELZPOWER:   /* or P2 is an exponent */
     case LMANDELZPOWER:
     case FPMANZTOZPLUSZPWR:
     case MARKSMANDEL:
     case MARKSMANDELFP:
     case MARKSJULIA:
     case MARKSJULIAFP:
       break;
     case FORMULA:  /* Check P2, P3, P4 and P5 */
     case FFORMULA:
       parmszero = (parmszero && param[2] == 0.0 && param[3] == 0.0
                    && param[4] == 0.0 && param[5] == 0.0
                    && param[6] == 0.0 && param[7] == 0.0
                    && param[8] == 0.0 && param[9] == 0.0);
       parmsnoreal = (parmsnoreal && param[2] == 0.0 && param[4] == 0.0
                      && param[6] == 0.0 && param[8] == 0.0);
       parmsnoimag = (parmsnoimag && param[3] == 0.0 && param[5] == 0.0
                      && param[7] == 0.0 && param[9] == 0.0);
       break;
     default:   /* Check P2 for the rest */
       parmszero = (parmszero && parm2.x == 0.0 && parm2.y == 0.0);
   }
   xaxis_row = yaxis_col = -1;
   if(bf_math)
   {
      saved = save_stack();
      bft1    = alloc_stack(rbflength+2);
      xaxis_on_screen = (sign_bf(bfymin) != sign_bf(bfymax));
      yaxis_on_screen = (sign_bf(bfxmin) != sign_bf(bfxmax));
   }
   else
   {
      xaxis_on_screen = (sign(yymin) != sign(yymax));
      yaxis_on_screen = (sign(xxmin) != sign(xxmax));
   }
   if (xaxis_on_screen) /* axis is on screen */
   {
      if(bf_math)
      {
         /* ftemp = (0.0-yymax) / (yymin-yymax); */
         sub_bf(bft1,bfymin,bfymax);
         div_bf(bft1,bfymax,bft1);
         neg_a_bf(bft1);
         ftemp = (double)bftofloat(bft1);
      }
      else
         ftemp = (0.0-yymax) / (yymin-yymax);
      ftemp *= (ydots-1);
      ftemp += 0.25;
      xaxis_row = (int)ftemp;
      xaxis_between = (ftemp - xaxis_row >= 0.5);
      if (uselist == 0 && (!xaxis_between || (xaxis_row+1)*2 != ydots))
         xaxis_row = -1; /* can't split screen, so dead center or not at all */
   }
   if (yaxis_on_screen) /* axis is on screen */
   {
      if(bf_math)
      {
         /* ftemp = (0.0-xxmin) / (xxmax-xxmin); */
         sub_bf(bft1,bfxmax,bfxmin);
         div_bf(bft1,bfxmin,bft1);
         neg_a_bf(bft1);
         ftemp = (double)bftofloat(bft1);
      }
      else
         ftemp = (0.0-xxmin) / (xxmax-xxmin);
      ftemp *= (xdots-1);
      ftemp += 0.25;
      yaxis_col = (int)ftemp;
      yaxis_between = (ftemp - yaxis_col >= 0.5);
      if (uselist == 0 && (!yaxis_between || (yaxis_col+1)*2 != xdots))
         yaxis_col = -1; /* can't split screen, so dead center or not at all */
   }
   switch(sym)       /* symmetry switch */
   {
   case XAXIS_NOREAL:    /* X-axis Symmetry (no real param) */
      if (!parmsnoreal) break;
      goto xsym;
   case XAXIS_NOIMAG:    /* X-axis Symmetry (no imag param) */
      if (!parmsnoimag) break;
      goto xsym;
   case XAXIS_NOPARM:                        /* X-axis Symmetry  (no params)*/
      if (!parmszero)
         break;
      xsym:
   case XAXIS:                       /* X-axis Symmetry */
      if (xsym_split(xaxis_row,xaxis_between) == 0) {
         if(basin)
            plot = symplot2basin;
         else
            plot = symplot2;
      }
      break;
   case YAXIS_NOPARM:                        /* Y-axis Symmetry (No Parms)*/
      if (!parmszero)
         break;
   case YAXIS:                       /* Y-axis Symmetry */
      if (ysym_split(yaxis_col,yaxis_between) == 0)
         plot = symplot2Y;
      break;
   case XYAXIS_NOPARM:                       /* X-axis AND Y-axis Symmetry (no parms)*/
      if(!parmszero)
         break;
   case XYAXIS:                      /* X-axis AND Y-axis Symmetry */
      xsym_split(xaxis_row,xaxis_between);
      ysym_split(yaxis_col,yaxis_between);
      switch (worksym & 3)
      {
      case 1: /* just xaxis symmetry */
         if(basin)
            plot = symplot2basin;
         else
            plot = symplot2 ;
         break;
      case 2: /* just yaxis symmetry */
         if (basin) /* got no routine for this case */
         {
            ixstop = xxstop; /* fix what split should not have done */
            symmetry = 1;
         }
         else
            plot = symplot2Y;
         break;
      case 3: /* both axes */
         if(basin)
            plot = symplot4basin;
         else
            plot = symplot4 ;
      }
      break;
   case ORIGIN_NOPARM:                       /* Origin Symmetry (no parms)*/
      if (!parmszero)
         break;
   case ORIGIN:                      /* Origin Symmetry */
      originsym:
      if ( xsym_split(xaxis_row,xaxis_between) == 0
          && ysym_split(yaxis_col,yaxis_between) == 0)
      {
         plot = symplot2J;
         ixstop = xxstop; /* didn't want this changed */
      }
      else
      {
         iystop = yystop; /* in case first split worked */
         symmetry = 1;
         worksym = 0x30; /* let it recombine with others like it */
      }
      break;
   case PI_SYM_NOPARM:
      if (!parmszero)
         break;
   case PI_SYM:                      /* PI symmetry */
      if(bf_math)
      {
         if((double)bftofloat(abs_a_bf(sub_bf(bft1,bfxmax,bfxmin))) < PI/4)
            break; /* no point in pi symmetry if values too close */
      }
      else
      {
         if(fabs(xxmax - xxmin) < PI/4)
            break; /* no point in pi symmetry if values too close */
      }
      if(invert && forcesymmetry == 999)
         goto originsym;
      plot = symPIplot ;
      symmetry = 0;
      if ( xsym_split(xaxis_row,xaxis_between) == 0
          && ysym_split(yaxis_col,yaxis_between) == 0)
         if(parm.y == 0.0)
            plot = symPIplot4J; /* both axes */
         else
            plot = symPIplot2J; /* origin */
      else
      {
         iystop = yystop; /* in case first split worked */
         worksym = 0x30;  /* don't mark pisym as ysym, just do it unmarked */
      }
      if(bf_math)
      {
         sub_bf(bft1,bfxmax,bfxmin);
         abs_a_bf(bft1);
         pixelpi = (int)((PI/(double)bftofloat(bft1)*xdots)); /* PI in pixels */
      }
      else
         pixelpi = (int)((PI/fabs(xxmax-xxmin))*xdots); /* PI in pixels */

      if ( (ixstop = xxstart+pixelpi-1 ) > xxstop)
         ixstop = xxstop;
      if (plot == symPIplot4J && ixstop > (i = (xxstart+xxstop)/2))
         ixstop = i;
      break;
   default:                  /* no symmetry */
      break;
   }
   if(bf_math)
      restore_stack(saved);
}

#ifdef _MSC_VER
#pragma optimize ("ea", on)
#endif

#if (_MSC_VER >= 700)
#pragma code_seg ()       /* back to normal segment */
#endif

/**************** tesseral method by CJLT begins here*********************/
/*  reworked by PB for speed and resumeability */

struct tess { /* one of these per box to be done gets stacked */
   int x1,x2,y1,y2;      /* left/right top/bottom x/y coords  */
   int top,bot,lft,rgt;  /* edge colors, -1 mixed, -2 unknown */
};

static int tesseral(void)
{
   struct tess *tp;

   guessplot = (plot != putcolor && plot != symplot2);
   tp = (struct tess *)&dstack[0];
   tp->x1 = ixstart;                              /* set up initial box */
   tp->x2 = ixstop;
   tp->y1 = iystart;
   tp->y2 = iystop;

   if (workpass == 0) { /* not resuming */
      tp->top = tessrow(ixstart,ixstop,iystart);     /* Do top row */
      tp->bot = tessrow(ixstart,ixstop,iystop);      /* Do bottom row */
      tp->lft = tesscol(ixstart,iystart+1,iystop-1); /* Do left column */
      tp->rgt = tesscol(ixstop,iystart+1,iystop-1);  /* Do right column */
      if (check_key()) { /* interrupt before we got properly rolling */
         add_worklist(xxstart,xxstop,xxstart,yystart,yystop,yystart,0,worksym);
         return(-1);
      }
   }

   else { /* resuming, rebuild work stack */
      int i,mid,curx,cury,xsize,ysize;
      struct tess *tp2;
      tp->top = tp->bot = tp->lft = tp->rgt = -2;
      cury = yybegin & 0xfff;
      ysize = 1;
      i = (unsigned)yybegin >> 12;
      while (--i >= 0) ysize <<= 1;
      curx = workpass & 0xfff;
      xsize = 1;
      i = (unsigned)workpass >> 12;
      while (--i >= 0) xsize <<= 1;
      for(;;) {
         tp2 = tp;
         if (tp->x2 - tp->x1 > tp->y2 - tp->y1) { /* next divide down middle */
            if (tp->x1 == curx && (tp->x2 - tp->x1 - 2) < xsize)
               break;
            mid = (tp->x1 + tp->x2) >> 1;                /* Find mid point */
            if (mid > curx) { /* stack right part */
               memcpy(++tp,tp2,sizeof(*tp));
               tp->x2 = mid;
            }
            tp2->x1 = mid;
         }
         else {                                   /* next divide across */
            if (tp->y1 == cury && (tp->y2 - tp->y1 - 2) < ysize) break;
            mid = (tp->y1 + tp->y2) >> 1;                /* Find mid point */
            if (mid > cury) { /* stack bottom part */
               memcpy(++tp,tp2,sizeof(*tp));
               tp->y2 = mid;
            }
            tp2->y1 = mid;
         }
      }
   }

   got_status = 4; /* for tab_display */

   while (tp >= (struct tess *)&dstack[0]) { /* do next box */
      curcol = tp->x1; /* for tab_display */
      currow = tp->y1;

      if (tp->top == -1 || tp->bot == -1 || tp->lft == -1 || tp->rgt == -1)
         goto tess_split;
      /* for any edge whose color is unknown, set it */
      if (tp->top == -2)
         tp->top = tesschkrow(tp->x1,tp->x2,tp->y1);
      if (tp->top == -1)
         goto tess_split;
      if (tp->bot == -2)
         tp->bot = tesschkrow(tp->x1,tp->x2,tp->y2);
      if (tp->bot != tp->top)
         goto tess_split;
      if (tp->lft == -2)
         tp->lft = tesschkcol(tp->x1,tp->y1,tp->y2);
      if (tp->lft != tp->top)
         goto tess_split;
      if (tp->rgt == -2)
         tp->rgt = tesschkcol(tp->x2,tp->y1,tp->y2);
      if (tp->rgt != tp->top)
         goto tess_split;

      {
      int mid,midcolor;
      if (tp->x2 - tp->x1 > tp->y2 - tp->y1) { /* divide down the middle */
         mid = (tp->x1 + tp->x2) >> 1;           /* Find mid point */
         midcolor = tesscol(mid, tp->y1+1, tp->y2-1); /* Do mid column */
         if (midcolor != tp->top) goto tess_split;
         }
      else {                              /* divide across the middle */
         mid = (tp->y1 + tp->y2) >> 1;           /* Find mid point */
         midcolor = tessrow(tp->x1+1, tp->x2-1, mid); /* Do mid row */
         if (midcolor != tp->top) goto tess_split;
         }
      }

      {  /* all 4 edges are the same color, fill in */
         int i,j;
         i = 0;
         if(fillcolor != 0)
         {
         if(fillcolor > 0)
            tp->top = fillcolor %colors;
         if (guessplot || (j = tp->x2 - tp->x1 - 1) < 2) { /* paint dots */
            for (col = tp->x1 + 1; col < tp->x2; col++)
               for (row = tp->y1 + 1; row < tp->y2; row++) {
                  (*plot)(col,row,tp->top);
                  if (++i > 500) {
                     if (check_key()) goto tess_end;
                     i = 0;
                  }
               }
         }
         else { /* use put_line for speed */
            memset(&dstack[OLDMAXPIXELS],tp->top,j);
            for (row = tp->y1 + 1; row < tp->y2; row++) {
               put_line(row,tp->x1+1,tp->x2-1,&dstack[OLDMAXPIXELS]);
               if (plot != putcolor) /* symmetry */
                  if ((j = yystop-(row-yystart)) > iystop && j < ydots)
                     put_line(j,tp->x1+1,tp->x2-1,&dstack[OLDMAXPIXELS]);
               if (++i > 25) {
                  if (check_key()) goto tess_end;
                  i = 0;
               }
            }
         }
         }
         --tp;
      }
      continue;

      tess_split:
      {  /* box not surrounded by same color, sub-divide */
         int mid,midcolor;
         struct tess *tp2;
         if (tp->x2 - tp->x1 > tp->y2 - tp->y1) { /* divide down the middle */
            mid = (tp->x1 + tp->x2) >> 1;                /* Find mid point */
            midcolor = tesscol(mid, tp->y1+1, tp->y2-1); /* Do mid column */
            if (midcolor == -3) goto tess_end;
            if (tp->x2 - mid > 1) {    /* right part >= 1 column */
               if (tp->top == -1) tp->top = -2;
               if (tp->bot == -1) tp->bot = -2;
               tp2 = tp;
               if (mid - tp->x1 > 1) { /* left part >= 1 col, stack right */
                  memcpy(++tp,tp2,sizeof(*tp));
                  tp->x2 = mid;
                  tp->rgt = midcolor;
               }
               tp2->x1 = mid;
               tp2->lft = midcolor;
            }
            else
               --tp;
         }
         else {                                   /* divide across the middle */
            mid = (tp->y1 + tp->y2) >> 1;                /* Find mid point */
            midcolor = tessrow(tp->x1+1, tp->x2-1, mid); /* Do mid row */
            if (midcolor == -3) goto tess_end;
            if (tp->y2 - mid > 1) {    /* bottom part >= 1 column */
               if (tp->lft == -1) tp->lft = -2;
               if (tp->rgt == -1) tp->rgt = -2;
               tp2 = tp;
               if (mid - tp->y1 > 1) { /* top also >= 1 col, stack bottom */
                  memcpy(++tp,tp2,sizeof(*tp));
                  tp->y2 = mid;
                  tp->bot = midcolor;
               }
               tp2->y1 = mid;
               tp2->top = midcolor;
            }
            else
               --tp;
         }
      }

   }

   tess_end:
   if (tp >= (struct tess *)&dstack[0]) { /* didn't complete */
      int i,xsize,ysize;
      xsize = ysize = 1;
      i = 2;
      while (tp->x2 - tp->x1 - 2 >= i) {
         i <<= 1;
         ++xsize;
      }
      i = 2;
      while (tp->y2 - tp->y1 - 2 >= i) {
         i <<= 1;
         ++ysize;
      }
      add_worklist(xxstart,xxstop,xxstart,yystart,yystop,
          (ysize<<12)+tp->y1,(xsize<<12)+tp->x1,worksym);
      return(-1);
   }
   return(0);

} /* tesseral */

static int _fastcall tesschkcol(int x,int y1,int y2)
{
   int i;
   i = getcolor(x,++y1);
   while (--y2 > y1)
      if (getcolor(x,y2) != i) return -1;
   return i;
}

static int _fastcall tesschkrow(int x1,int x2,int y)
{
   int i;
   i = getcolor(x1,y);
   while (x2 > x1) {
      if (getcolor(x2,y) != i) return -1;
      --x2;
   }
   return i;
}

static int _fastcall tesscol(int x,int y1,int y2)
{
   int colcolor,i;
   col = x;
   row = y1;
   reset_periodicity = 1;
   colcolor = (*calctype)();
   reset_periodicity = 0;
   while (++row <= y2) { /* generate the column */
      if ((i = (*calctype)()) < 0) return -3;
      if (i != colcolor) colcolor = -1;
   }
   return colcolor;
}

static int _fastcall tessrow(int x1,int x2,int y)
{
   int rowcolor,i;
   row = y;
   col = x1;
   reset_periodicity = 1;
   rowcolor = (*calctype)();
   reset_periodicity = 0;
   while (++col <= x2) { /* generate the row */
      if ((i = (*calctype)()) < 0) return -3;
      if (i != rowcolor) rowcolor = -1;
   }
   return rowcolor;
}

/* added for testing autologmap() */ /* CAE 9211 fixed missing comment */
/* insert at end of CALCFRAC.C */

static long autologmap(void)   /*RB*/
{  /* calculate round screen edges to avoid wasted colours in logmap */
 long mincolour;
 int lag;
 int xstop = xdots - 1; /* don't use symetry */
 int ystop = ydots - 1; /* don't use symetry */
 long old_maxit;
 mincolour=LONG_MAX;
 row=0;
 reset_periodicity = 0;
 old_maxit = maxit;
 for (col=0;col<xstop;col++) /* top row */
    {
      color=(*calctype)();
      if (color == -1) goto ack; /* key pressed, bailout */
      if (realcoloriter < mincolour) {
        mincolour=realcoloriter ;
        maxit = max(2,mincolour); /*speedup for when edges overlap lakes */
        }
      if ( col >=32 ) (*plot)(col-32,row,0);
    }                                    /* these lines tidy up for BTM etc */
    for (lag=32;lag>0;lag--) (*plot)(col-lag,row,0);

 col=xstop;
 for (row=0;row<ystop;row++) /* right  side */
    {
      color=(*calctype)();
      if (color == -1) goto ack; /* key pressed, bailout */
      if (realcoloriter < mincolour) {
        mincolour=realcoloriter ;
        maxit = max(2,mincolour); /*speedup for when edges overlap lakes */
        }
      if ( row >=32 ) (*plot)(col,row-32,0);
    }
    for (lag=32;lag>0;lag--) (*plot)(col,row-lag,0);

 col=0;
 for (row=0;row<ystop;row++) /* left  side */
    {
      color=(*calctype)();
      if (color == -1) goto ack; /* key pressed, bailout */
      if (realcoloriter < mincolour) {
        mincolour=realcoloriter ;
        maxit = max(2,mincolour); /*speedup for when edges overlap lakes */
        }
      if ( row >=32 ) (*plot)(col,row-32,0);
    }
    for (lag=32;lag>0;lag--) (*plot)(col,row-lag,0);

 row=ystop ;
 for (col=0;col<xstop;col++) /* bottom row */
    {
      color=(*calctype)();
      if (color == -1) goto ack; /* key pressed, bailout */
      if (realcoloriter < mincolour) {
        mincolour=realcoloriter ;
        maxit = max(2,mincolour); /*speedup for when edges overlap lakes */
        }
      if ( col >=32 ) (*plot)(col-32,row,0);
    }
    for (lag=32;lag>0;lag--) (*plot)(col-lag,row,0);

 ack: /* bailout here if key is pressed */
 if (mincolour==2) resuming=1; /* insure autologmap not called again */
 maxit = old_maxit;

 return ( mincolour );
}

/* Symmetry plot for period PI */
void _fastcall symPIplot(int x, int y, int color)
{
   while(x <= xxstop)
   {
      putcolor(x, y, color) ;
      x += pixelpi;
   }
}
/* Symmetry plot for period PI plus Origin Symmetry */
void _fastcall symPIplot2J(int x, int y, int color)
{
   int i,j;
   while(x <= xxstop)
   {
      putcolor(x, y, color) ;
      if ((i=yystop-(y-yystart)) > iystop && i < ydots
          && (j=xxstop-(x-xxstart)) < xdots)
         putcolor(j, i, color) ;
      x += pixelpi;
   }
}
/* Symmetry plot for period PI plus Both Axis Symmetry */
void _fastcall symPIplot4J(int x, int y, int color)
{
   int i,j;
   while(x <= (xxstart+xxstop)/2)
   {
      j = xxstop-(x-xxstart);
      putcolor(       x , y , color) ;
      if (j < xdots)
         putcolor(j , y , color) ;
      if ((i=yystop-(y-yystart)) > iystop && i < ydots)
      {
         putcolor(x , i , color) ;
         if (j < xdots)
            putcolor(j , i , color) ;
      }
      x += pixelpi;
   }
}

/* Symmetry plot for X Axis Symmetry */
void _fastcall symplot2(int x, int y, int color)
{
   int i;
   putcolor(x, y, color) ;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots)
      putcolor(x, i, color) ;
}

/* Symmetry plot for Y Axis Symmetry */
void _fastcall symplot2Y(int x, int y, int color)
{
   int i;
   putcolor(x, y, color) ;
   if ((i=xxstop-(x-xxstart)) < xdots)
      putcolor(i, y, color) ;
}

/* Symmetry plot for Origin Symmetry */
void _fastcall symplot2J(int x, int y, int color)
{
   int i,j;
   putcolor(x, y, color) ;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots
       && (j=xxstop-(x-xxstart)) < xdots)
      putcolor(j, i, color) ;
}

/* Symmetry plot for Both Axis Symmetry */
void _fastcall symplot4(int x, int y, int color)
{
   int i,j;
   j = xxstop-(x-xxstart);
   putcolor(       x , y, color) ;
   if (j < xdots)
      putcolor(    j , y, color) ;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots)
   {
      putcolor(    x , i, color) ;
      if (j < xdots)
         putcolor(j , i, color) ;
   }
}

/* Symmetry plot for X Axis Symmetry - Striped Newtbasin version */
void _fastcall symplot2basin(int x, int y, int color)
{
   int i,stripe;
   putcolor(x, y, color) ;
   if(basin==2 && color > 8)
      stripe=8;
   else
      stripe = 0;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots)
   {
      color -= stripe;                    /* reconstruct unstriped color */
      color = (degree+1-color)%degree+1;  /* symmetrical color */
      color += stripe;                    /* add stripe */
      putcolor(x, i,color)  ;
   }
}

/* Symmetry plot for Both Axis Symmetry  - Newtbasin version */
void _fastcall symplot4basin(int x, int y, int color)
{
   int i,j,color1,stripe;
   if(color == 0) /* assumed to be "inside" color */
   {
      symplot4(x, y, color);
      return;
   }
   if(basin==2 && color > 8)
      stripe = 8;
   else
      stripe = 0;
   color -= stripe;               /* reconstruct unstriped color */
   color1 = degree/2+degree+2 - color;
   if(color < degree/2+2)
      color1 = degree/2+2 - color;
   else
      color1 = degree/2+degree+2 - color;
   j = xxstop-(x-xxstart);
   putcolor(       x, y, color+stripe) ;
   if (j < xdots)
      putcolor(    j, y, color1+stripe) ;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots)
   {
      putcolor(    x, i, stripe + (degree+1 - color)%degree+1) ;
      if (j < xdots)
         putcolor(j, i, stripe + (degree+1 - color1)%degree+1) ;
   }
}

static void _fastcall puttruecolor_disk(int x, int y, int color)
{
   putcolor_a(x,y,color);

   targa_color(x, y, color);
/* writedisk(x*=3,y,(BYTE)((realcoloriter      ) & 0xff)); */ /* blue  */
/* writedisk(++x, y,(BYTE)((realcoloriter >> 8 ) & 0xff)); */ /* green */
/* writedisk(x+1, y,(BYTE)((realcoloriter >> 16) & 0xff)); */ /* red   */

}

/* Do nothing plot!!! */
#ifdef __CLINT__
#pragma argsused
#endif

void _fastcall noplot(int x,int y,int color)
{
   x = y = color = 0;  /* just for warning */
}
