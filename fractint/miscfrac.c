/*

Miscellaneous fractal-specific code (formerly in CALCFRAC.C)

*/

#include <string.h>
#include <limits.h>
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "targa_lc.h"

/* routines in this module      */

static void set_Plasma_palette(void);
static U16 _fastcall adjust(int xa,int ya,int x,int y,int xb,int yb);
static void _fastcall subDivide(int x1,int y1,int x2,int y2);
static int _fastcall new_subD (int x1,int y1,int x2,int y2, int recur);
static void verhulst(void);
static void Bif_Period_Init(void);
static int  _fastcall Bif_Periodic(long);
static void set_Cellular_palette(void);

U16  (_fastcall *getpix)(int,int)  = (U16(_fastcall *)(int,int))getcolor;

typedef void (_fastcall *PLOT)(int,int,int);

/***************** standalone engine for "test" ********************/

int test(void)
{
   int startrow,startpass,numpasses;
   startrow = startpass = 0;
   if (resuming)
   {
      start_resume();
      get_resume(sizeof(startrow),&startrow,sizeof(startpass),&startpass,0);
      end_resume();
   }
   if(teststart()) /* assume it was stand-alone, doesn't want passes logic */
      return(0);
   numpasses = (stdcalcmode == '1') ? 0 : 1;
   for (passes=startpass; passes <= numpasses ; passes++)
   {
      for (row = startrow; row <= iystop; row=row+1+numpasses)
      {
         for (col = 0; col <= ixstop; col++)       /* look at each point on screen */
         {
            register int color;
            init.x = dxpixel();
            init.y = dypixel();
            if(keypressed())
            {
               testend();
               alloc_resume(20,1);
               put_resume(sizeof(row),&row,sizeof(passes),&passes,0);
               return(-1);
            }
            color = testpt(init.x,init.y,parm.x,parm.y,maxit,inside);
            if (color >= colors) { /* avoid trouble if color is 0 */
               if (colors < 16)
                  color &= andcolor;
               else
                  color = ((color-1) % andcolor) + 1; /* skip color zero */
            }
            (*plot)(col,row,color);
            if(numpasses && (passes == 0))
               (*plot)(col,row+1,color);
         }
      }
      startrow = passes + 1;
   }
   testend();
   return(0);
}

/***************** standalone engine for "plasma" ********************/

static int iparmx;      /* iparmx = parm.x * 8 */
static int shiftvalue;  /* shift based on #colors */
static int recur1=1;
static int pcolors;
static int recur_level = 0;
U16 max_plasma;

/* returns a random 16 bit value that is never 0 */
U16 rand16(void)
{
   U16 value;
   value = (U16)rand15();
   value <<= 1;
   value = (U16)(value + (rand15()&1));
   if(value < 1)
      value = 1;
   return(value);
}

void _fastcall putpot(int x, int y, U16 color)
{
   if(color < 1)
      color = 1;
   putcolor(x, y, color >> 8 ? color >> 8 : 1);  /* don't write 0 */
   /* we don't write this if dotmode==11 because the above putcolor
         was already a "writedisk" in that case */
   if (dotmode != 11)
      writedisk(x+sxoffs,y+syoffs,color >> 8);    /* upper 8 bits */
   writedisk(x+sxoffs,y+sydots+syoffs,color&255); /* lower 8 bits */
}

/* fixes border */
void _fastcall putpotborder(int x, int y, U16 color)
{
   if((x==0) || (y==0) || (x==xdots-1) || (y==ydots-1))
      color = (U16)outside;
   putpot(x,y,color);
}

/* fixes border */
void _fastcall putcolorborder(int x, int y, int color)
{
   if((x==0) || (y==0) || (x==xdots-1) || (y==ydots-1))
      color = outside;
   if(color < 1)
      color = 1;
   putcolor(x,y,color);
}

U16 _fastcall getpot(int x, int y)
{
   U16 color;

   color = (U16)readdisk(x+sxoffs,y+syoffs);
   color = (U16)((color << 8) + (U16) readdisk(x+sxoffs,y+sydots+syoffs));
   return(color);
}

static int plasma_check;                        /* to limit kbd checking */

static U16 _fastcall adjust(int xa,int ya,int x,int y,int xb,int yb)
{
   S32 pseudorandom;
   pseudorandom = ((S32)iparmx)*((rand15()-16383));
/*   pseudorandom = pseudorandom*(abs(xa-xb)+abs(ya-yb));*/
   pseudorandom = pseudorandom * recur1;
   pseudorandom = pseudorandom >> shiftvalue;
   pseudorandom = (((S32)getpix(xa,ya)+(S32)getpix(xb,yb)+1)>>1)+pseudorandom;
   if(max_plasma == 0)
   {
      if (pseudorandom >= pcolors)
         pseudorandom = pcolors-1;
   }
   else if (pseudorandom >= (S32)max_plasma)
      pseudorandom = max_plasma;
   if(pseudorandom < 1)
      pseudorandom = 1;
   plot(x,y,(U16)pseudorandom);
   return((U16)pseudorandom);
}


static int _fastcall new_subD (int x1,int y1,int x2,int y2, int recur)
{
   int x,y;
   int nx1;
   int nx;
   int ny1, ny;
   S32 i, v;

   struct sub {
      BYTE t; /* top of stack */
      int v[16]; /* subdivided value */
      BYTE r[16];  /* recursion level */
   };

   static struct sub subx, suby;

   /*
   recur1=1;
   for (i=1;i<=recur;i++)
      recur1 = recur1 * 2;
   recur1=320/recur1;
   */
   recur1 = (int)(320L >> recur);
   suby.t = 2;
   ny   = suby.v[0] = y2;
   ny1 = suby.v[2] = y1;
   suby.r[0] = suby.r[2] = 0;
   suby.r[1] = 1;
   y = suby.v[1] = (ny1 + ny) >> 1;

   while (suby.t >= 1)
   {
      if ((++plasma_check & 0x0f) == 1)
         if(keypressed())
         {
            plasma_check--;
            return(1);
         }
      while (suby.r[suby.t-1] < (BYTE)recur)
      {
         /*     1.  Create new entry at top of the stack  */
         /*     2.  Copy old top value to new top value.  */
         /*            This is largest y value.           */
         /*     3.  Smallest y is now old mid point       */
         /*     4.  Set new mid point recursion level     */
         /*     5.  New mid point value is average        */
         /*            of largest and smallest            */

         suby.t++;
         ny1  = suby.v[suby.t] = suby.v[suby.t-1];
         ny   = suby.v[suby.t-2];
         suby.r[suby.t] = suby.r[suby.t-1];
         y    = suby.v[suby.t-1]   = (ny1 + ny) >> 1;
         suby.r[suby.t-1]   = (BYTE)(max(suby.r[suby.t], suby.r[suby.t-2])+1);
      }
      subx.t = 2;
      nx  = subx.v[0] = x2;
      nx1 = subx.v[2] = x1;
      subx.r[0] = subx.r[2] = 0;
      subx.r[1] = 1;
      x = subx.v[1] = (nx1 + nx) >> 1;

      while (subx.t >= 1)
      {
         while (subx.r[subx.t-1] < (BYTE)recur)
         {
            subx.t++; /* move the top ofthe stack up 1 */
            nx1  = subx.v[subx.t] = subx.v[subx.t-1];
            nx   = subx.v[subx.t-2];
            subx.r[subx.t] = subx.r[subx.t-1];
            x    = subx.v[subx.t-1]   = (nx1 + nx) >> 1;
            subx.r[subx.t-1]   = (BYTE)(max(subx.r[subx.t],
                subx.r[subx.t-2])+1);
         }

         if ((i = getpix(nx, y)) == 0)
            i = adjust(nx,ny1,nx,y ,nx,ny);
         v = i;
         if ((i = getpix(x, ny)) == 0)
            i = adjust(nx1,ny,x ,ny,nx,ny);
         v += i;
         if(getpix(x,y) == 0)
         {
            if ((i = getpix(x, ny1)) == 0)
               i = adjust(nx1,ny1,x ,ny1,nx,ny1);
            v += i;
            if ((i = getpix(nx1, y)) == 0)
               i = adjust(nx1,ny1,nx1,y ,nx1,ny);
            v += i;
            plot(x,y,(U16)((v + 2) >> 2));
         }

         if (subx.r[subx.t-1] == (BYTE)recur) subx.t = (BYTE)(subx.t - 2);
      }

      if (suby.r[suby.t-1] == (BYTE)recur) suby.t = (BYTE)(suby.t - 2);
   }
   return(0);
}

static void _fastcall subDivide(int x1,int y1,int x2,int y2)
{
   int x,y;
   S32 v,i;
   if ((++plasma_check & 0x7f) == 1)
      if(keypressed())
      {
         plasma_check--;
         return;
      }
   if(x2-x1<2 && y2-y1<2)
      return;
   recur_level++;
   recur1 = (int)(320L >> recur_level);

   x = (x1+x2)>>1;
   y = (y1+y2)>>1;
   if((v=getpix(x,y1)) == 0)
      v=adjust(x1,y1,x ,y1,x2,y1);
   i=v;
   if((v=getpix(x2,y)) == 0)
      v=adjust(x2,y1,x2,y ,x2,y2);
   i+=v;
   if((v=getpix(x,y2)) == 0)
      v=adjust(x1,y2,x ,y2,x2,y2);
   i+=v;
   if((v=getpix(x1,y)) == 0)
      v=adjust(x1,y1,x1,y ,x1,y2);
   i+=v;

   if(getpix(x,y) == 0)
      plot(x,y,(U16)((i+2)>>2));

   subDivide(x1,y1,x ,y);
   subDivide(x ,y1,x2,y);
   subDivide(x ,y ,x2,y2);
   subDivide(x1,y ,x ,y2);
   recur_level--;
}


int plasma()
{
   int i,k, n;
   U16 rnd[4];
   int OldPotFlag, OldPot16bit;

   OldPotFlag=OldPot16bit=plasma_check = 0;

   if(colors < 4) {
      static FCODE plasmamsg[]={
         "\
Plasma Clouds can currently only be run in a 4-or-more-color video\n\
mode (and color-cycled only on VGA adapters [or EGA adapters in their\n\
640x350x16 mode])."      };
      stopmsg(0,plasmamsg);
      return(-1);
   }
   iparmx = (int)(param[0] * 8);
   if (parm.x <= 0.0) iparmx = 0;
   if (parm.x >= 100) iparmx = 800;
   param[0] = (double)iparmx / 8.0;  /* let user know what was used */
   if (param[1] < 0) param[1] = 0;  /* limit parameter values */
   if (param[1] > 1) param[1] = 1;
   if (param[2] < 0) param[2] = 0;  /* limit parameter values */
   if (param[2] > 1) param[2] = 1;
   if (param[3] < 0) param[3] = 0;  /* limit parameter values */
   if (param[3] > 1) param[3] = 1;

   if ((!rflag) && param[2] == 1)
      --rseed;
   if (param[2] != 0 && param[2] != 1)
      rseed = (int)param[2];
   max_plasma = (U16)param[3];  /* max_plasma is used as a flag for potential */

   if(max_plasma != 0)
   {
      if (pot_startdisk() >= 0)
      {
         /* max_plasma = (U16)(1L << 16) -1; */
         max_plasma = 0xFFFF;
         if(outside >= 0)
            plot    = (PLOT)putpotborder;
         else
            plot    = (PLOT)putpot;
         getpix =  getpot;
         OldPotFlag = potflag;
         OldPot16bit = pot16bit;
      }
      else
      {
         max_plasma = 0;        /* can't do potential (startdisk failed) */
         param[3]   = 0;
         if(outside >= 0)
            plot    = putcolorborder;
         else
            plot    = putcolor;
         getpix  = (U16(_fastcall *)(int,int))getcolor;
      }
   }
   else
   {
      if(outside >= 0)
        plot    = putcolorborder;
       else
        plot    = putcolor;
      getpix  = (U16(_fastcall *)(int,int))getcolor;
   }
   srand(rseed);
   if (!rflag) ++rseed;

   if (colors == 256)                   /* set the (256-color) palette */
      set_Plasma_palette();             /* skip this if < 256 colors */

   if (colors > 16)
      shiftvalue = 18;
   else
   {
      if (colors > 4)
         shiftvalue = 22;
      else
      {
         if (colors > 2)
            shiftvalue = 24;
         else
            shiftvalue = 25;
      }
   }
   if(max_plasma != 0)
      shiftvalue = 10;

   if(max_plasma == 0)
   {
      pcolors = min(colors, max_colors);
      for(n = 0; n < 4; n++)
         rnd[n] = (U16)(1+(((rand15()/pcolors)*(pcolors-1))>>(shiftvalue-11)));
   }
   else
      for(n = 0; n < 4; n++)
         rnd[n] = rand16();
   if(debugflag==3600)
      for(n = 0; n < 4; n++)
         rnd[n] = 1;

   plot(      0,      0,  rnd[0]);
   plot(xdots-1,      0,  rnd[1]);
   plot(xdots-1,ydots-1,  rnd[2]);
   plot(      0,ydots-1,  rnd[3]);

   recur_level = 0;
   if (param[1] == 0)
      subDivide(0,0,xdots-1,ydots-1);
   else
   {
      recur1 = i = k = 1;
      while(new_subD(0,0,xdots-1,ydots-1,i)==0)
      {
         k = k * 2;
         if (k  >(int)max(xdots-1,ydots-1))
            break;
         if (keypressed())
         {
            n = 1;
            goto done;
         }
         i++;
      }
   }
   if (! keypressed())
      n = 0;
   else
      n = 1;
   done:
   if(max_plasma != 0)
   {
      potflag = OldPotFlag;
      pot16bit = OldPot16bit;
   }
   plot    = putcolor;
   getpix  = (U16(_fastcall *)(int,int))getcolor;
   return(n);
}

#define dac ((Palettetype *)dacbox)
static void set_Plasma_palette()
{
   static Palettetype Red    = { 63, 0, 0 };
   static Palettetype Green  = { 0, 63, 0 };
   static Palettetype Blue   = { 0,  0,63 };
   int i;

   if (mapdacbox || colorpreloaded) return;    /* map= specified */

   dac[0].red  = 0 ;
   dac[0].green= 0 ;
   dac[0].blue = 0 ;
   for(i=1;i<=85;i++)
   {
#ifdef __SVR4
      dac[i].red       = (BYTE)((i*(int)Green.red   + (86-i)*(int)Blue.red)/85);
      dac[i].green     = (BYTE)((i*(int)Green.green + (86-i)*(int)Blue.green)/85);
      dac[i].blue      = (BYTE)((i*(int)Green.blue  + (86-i)*(int)Blue.blue)/85);

      dac[i+85].red    = (BYTE)((i*(int)Red.red   + (86-i)*(int)Green.red)/85);
      dac[i+85].green  = (BYTE)((i*(int)Red.green + (86-i)*(int)Green.green)/85);
      dac[i+85].blue   = (BYTE)((i*(int)Red.blue  + (86-i)*(int)Green.blue)/85);

      dac[i+170].red   = (BYTE)((i*(int)Blue.red   + (86-i)*(int)Red.red)/85);
      dac[i+170].green = (BYTE)((i*(int)Blue.green + (86-i)*(int)Red.green)/85);
      dac[i+170].blue  = (BYTE)((i*(int)Blue.blue  + (86-i)*(int)Red.blue)/85);
#else
      dac[i].red       = (BYTE)((i*Green.red   + (86-i)*Blue.red)/85);
      dac[i].green     = (BYTE)((i*Green.green + (86-i)*Blue.green)/85);  
      dac[i].blue      = (BYTE)((i*Green.blue  + (86-i)*Blue.blue)/85);
 
      dac[i+85].red    = (BYTE)((i*Red.red   + (86-i)*Green.red)/85);
      dac[i+85].green  = (BYTE)((i*Red.green + (86-i)*Green.green)/85);   
      dac[i+85].blue   = (BYTE)((i*Red.blue  + (86-i)*Green.blue)/85); 
      dac[i+170].red   = (BYTE)((i*Blue.red   + (86-i)*Red.red)/85);
      dac[i+170].green = (BYTE)((i*Blue.green + (86-i)*Red.green)/85);
      dac[i+170].blue  = (BYTE)((i*Blue.blue  + (86-i)*Red.blue)/85);
#endif
   }
   SetTgaColors();      /* TARGA 3 June 89  j mclain */
   spindac(0,1);
}

/***************** standalone engine for "diffusion" ********************/

#define RANDOM(x)  (rand()%(x))

int diffusion()
{
   int xmax,ymax,xmin,ymin;     /* Current maximum coordinates */
   int border;   /* Distance between release point and fractal */
   int mode;     /* Determines diffusion type:  0 = central (classic) */
                 /*                             1 = falling particles */
                 /*                             2 = square cavity     */
   int colorshift; /* If zero, select colors at random, otherwise shift */
                   /* the color every colorshift points */

   int colorcount,currentcolor;
  
   int i;
   double cosine,sine,angle;
   int x,y;
   float r, radius;

   if (diskvideo)
      notdiskmsg();
  
   x = y = -1;
   bitshift = 16;
   fudge = 1L << 16;

   border = (int)param[0];
   mode = (int)param[1];
   colorshift = (int)param[2];

   colorcount = colorshift; /* Counts down from colorshift */
   currentcolor = 1;  /* Start at color 1 (color 0 is probably invisible)*/

   if (mode > 2)
      mode=0;

   if (border <= 0)
      border = 10;

   srand(rseed);
   if (!rflag) ++rseed;

   if (mode == 0) {
      xmax = xdots / 2 + border;  /* Initial box */
      xmin = xdots / 2 - border;
      ymax = ydots / 2 + border;
      ymin = ydots / 2 - border;
   }
   if (mode == 1) {
      xmax = xdots / 2 + border;  /* Initial box */
      xmin = xdots / 2 - border;
      ymin = ydots - border;
   }
   if (mode == 2) {
      if (xdots > ydots)
         radius = ydots - border;
      else
         radius = xdots - border;
   }
   if (resuming) /* restore worklist, if we can't the above will stay in place */
   {
      start_resume();
      if (mode != 2)
         get_resume(sizeof(xmax),&xmax,sizeof(xmin),&xmin,sizeof(ymax),&ymax,
             sizeof(ymin),&ymin,0);
      else
         get_resume(sizeof(xmax),&xmax,sizeof(xmin),&xmin,sizeof(ymax),&ymax,
             sizeof(radius),&radius,0);
      end_resume();
   }

   switch (mode) {
   case 0: /* Single seed point in the center */
           putcolor(xdots / 2, ydots / 2,currentcolor);  
           break;
   case 1: /* Line along the bottom */
           for (i=0;i<=xdots;i++)
           putcolor(i,ydots-1,currentcolor);
           break;
   case 2: /* Large square that fills the screen */
           if (xdots > ydots)
              for (i=0;i<ydots;i++){
                 putcolor(xdots/2-ydots/2 , i , currentcolor);
                 putcolor(xdots/2+ydots/2 , i , currentcolor);
                 putcolor(xdots/2-ydots/2+i , 0 , currentcolor);
                 putcolor(xdots/2-ydots/2+i , ydots-1 , currentcolor);
              }
           else 
              for (i=0;i<xdots;i++){
                 putcolor(0 , ydots/2-xdots/2+i , currentcolor);
                 putcolor(xdots-1 , ydots/2-xdots/2+i , currentcolor);
                 putcolor(i , ydots/2-xdots/2 , currentcolor);
                 putcolor(i , ydots/2+xdots/2 , currentcolor);
              }
           break;
   }

   for(;;)
   {
      switch (mode) {
      case 0: /* Release new point on a circle inside the box */
               angle=2*(double)rand()/(RAND_MAX/PI);
               FPUsincos(&angle,&sine,&cosine);
               x = (int)(cosine*(xmax-xmin) + xdots);
               y = (int)(sine  *(ymax-ymin) + ydots);
               x = x >> 1; /* divide by 2 */
               y = y >> 1;
               break;
      case 1: /* Release new point on the line ymin somewhere between xmin
                 and xmax */
              y=ymin;
              x=RANDOM(xmax-xmin) + (xdots-xmax+xmin)/2;
              break;
      case 2: /* Release new point on a circle inside the box with radius
                 given by the radius variable */
               angle=2*(double)rand()/(RAND_MAX/PI);
               FPUsincos(&angle,&sine,&cosine);
               x = (int)(cosine*radius + xdots);
               y = (int)(sine  *radius + ydots);
               x = x >> 1;
               y = y >> 1;
               break;
      }

      /* Loop as long as the point (x,y) is surrounded by color 0 */
      /* on all eight sides                                       */

      while((getcolor(x+1,y+1) == 0) && (getcolor(x+1,y) == 0) &&
          (getcolor(x+1,y-1) == 0) && (getcolor(x  ,y+1) == 0) &&
          (getcolor(x  ,y-1) == 0) && (getcolor(x-1,y+1) == 0) &&
          (getcolor(x-1,y) == 0) && (getcolor(x-1,y-1) == 0))
      {
         /* Erase moving point */
         if (show_orbit)
            putcolor(x,y,0);

         if (mode==0){  /* Make sure point is inside the box */
            if (x==xmax)
               x--;
            else if (x==xmin)
               x++;
            if (y==ymax)
               y--;
            else if (y==ymin)
               y++;
         }

         if (mode==1) /* Make sure point is on the screen below ymin, but
                    we need a 1 pixel margin because of the next random step.*/
         {
            if (x>=xdots-1)
               x--;
            else if (x<=1)
               x++;
            if (y<ymin)
               y++;
         }

         /* Take one random step */
         x += RANDOM(3) - 1;
         y += RANDOM(3) - 1;

         /* Check keyboard */
         if ((++plasma_check & 0x7f) == 1)
            if(check_key())
            {
               alloc_resume(20,1);
               if (mode!=2)
                  put_resume(sizeof(xmax),&xmax,sizeof(xmin),&xmin,
                      sizeof(ymax),&ymax,sizeof(ymin),&ymin,0);
               else
                  put_resume(sizeof(xmax),&xmax,sizeof(xmin),&xmin,
                      sizeof(ymax),&ymax,sizeof(radius),&radius,0);

               plasma_check--;
               return 1;
            }

         /* Show the moving point */
         if (show_orbit)
            putcolor(x,y,RANDOM(colors-1)+1);

      } /* End of loop, now fix the point */

      /* If we're doing colorshifting then use currentcolor, otherwise 
         pick one at random */
      putcolor(x,y,colorshift?currentcolor:RANDOM(colors-1)+1);

      /* If we're doing colorshifting then check to see if we need to shift*/
      if (colorshift){
        if (!--colorcount){ /* If the counter reaches zero then shift*/
          currentcolor++;      /* Increase the current color and wrap */
          currentcolor%=colors;  /* around skipping zero */
          if (!currentcolor) currentcolor++;
          colorcount=colorshift;  /* and reset the counter */
        }
      }

      /* If the new point is close to an edge, we may need to increase
         some limits so that the limits expand to match the growing
         fractal. */
 
      switch (mode) {
      case 0: if (((x+border)>xmax) || ((x-border)<xmin)
                    || ((y-border)<ymin) || ((y+border)>ymax))
              {
                 /* Increase box size, but not past the edge of the screen */
                 ymin--;
                 ymax++;
                 xmin--;
                 xmax++;
                 if ((ymin==0) || (xmin==0))
                    return 0;
              }
              break;
      case 1: /* Decrease ymin, but not past top of screen */
              if (y-border < ymin)
                 ymin--;
              if (ymin==0)
                 return 0;
              break;
      case 2: /* Decrease the radius where points are released to stay away 
                 from the fractal.  It might be decreased by 1 or 2 */
              r = sqr((float)x-xdots/2) + sqr((float)y-ydots/2);
              if (r<=border*border) 
                return 0;
              while ((radius-border)*(radius-border) > r)
                 radius--;
              break;
      }
   }
}



/************ standalone engine for "bifurcation" types ***************/

/***************************************************************/
/* The following code now forms a generalised Fractal Engine   */
/* for Bifurcation fractal typeS.  By rights it now belongs in */
/* CALCFRACT.C, but it's easier for me to leave it here !      */

/* Original code by Phil Wilson, hacked around by Kev Allen.   */

/* Besides generalisation, enhancements include Periodicity    */
/* Checking during the plotting phase (AND halfway through the */
/* filter cycle, if possible, to halve calc times), quicker    */
/* floating-point calculations for the standard Verhulst type, */
/* and new bifurcation types (integer bifurcation, f.p & int   */
/* biflambda - the real equivalent of complex Lambda sets -    */
/* and f.p renditions of bifurcations of r*sin(Pi*p), which    */
/* spurred Mitchel Feigenbaum on to discover his Number).      */

/* To add further types, extend the fractalspecific[] array in */
/* usual way, with Bifurcation as the engine, and the name of  */
/* the routine that calculates the next bifurcation generation */
/* as the "orbitcalc" routine in the fractalspecific[] entry.  */

/* Bifurcation "orbitcalc" routines get called once per screen */
/* pixel column.  They should calculate the next generation    */
/* from the doubles Rate & Population (or the longs lRate &    */
/* lPopulation if they use integer math), placing the result   */
/* back in Population (or lPopulation).  They should return 0  */
/* if all is ok, or any non-zero value if calculation bailout  */
/* is desirable (eg in case of errors, or the series tending   */
/* to infinity).                Have fun !                     */
/***************************************************************/

#define DEFAULTFILTER 1000     /* "Beauty of Fractals" recommends using 5000
                               (p.25), but that seems unnecessary. Can
                               override this value with a nonzero param1 */

#define SEED 0.66               /* starting value for population */

static int far *verhulst_array;
unsigned long filter_cycles;
static unsigned int half_time_check;
static long   lPopulation, lRate;
double Population,  Rate;
static int    mono, outside_x;
static long   LPI;

int Bifurcation(void)
{
   unsigned long array_size;
   int row, column;
   column = 0;
   if (resuming)
   {
      start_resume();
      get_resume(sizeof(column),&column,0);
      end_resume();
   }
   array_size =  (iystop + 1) * sizeof(int); /* should be iystop + 1 */
   if ((verhulst_array = (int far *) farmemalloc(array_size)) == NULL)
   {
      static FCODE msg[]={"Insufficient free memory for calculation."};
      stopmsg(0,msg);
      return(-1);
   }

   LPI = (long)(PI * fudge);

   for (row = 0; row <= iystop; row++) /* should be iystop */
      verhulst_array[row] = 0;

   mono = 0;
   if (colors == 2)
      mono = 1;
   if (mono)
   {
      if (inside)
      {
         outside_x = 0;
         inside = 1;
      }
      else
         outside_x = 1;
   }

   filter_cycles = (parm.x <= 0) ? DEFAULTFILTER : (long)parm.x;
   half_time_check = FALSE;
   if (periodicitycheck && (unsigned long)maxit < filter_cycles)
   {
      filter_cycles = (filter_cycles - maxit + 1) / 2;
      half_time_check = TRUE;
   }

   if (integerfractal)
      linit.y = ymax - iystop*dely;            /* Y-value of    */
   else
      init.y = (double)(yymax - iystop*delyy); /* bottom pixels */

   while (column <= ixstop)
   {
      if(keypressed())
      {
         farmemfree((char far *)verhulst_array);
         alloc_resume(10,1);
         put_resume(sizeof(column),&column,0);
         return(-1);
      }

      if (integerfractal)
         lRate = xmin + column*delx;
      else
         Rate = (double)(xxmin + column*delxx);
      verhulst();        /* calculate array once per column */

      for (row = iystop; row >= 0; row--) /* should be iystop & >=0 */
      {
         int color;
         color = verhulst_array[row];
         if(color && mono)
            color = inside;
         else if((!color) && mono)
            color = outside_x;
         else if (color>=colors)
            color = colors-1;
         verhulst_array[row] = 0;
         (*plot)(column,row,color); /* was row-1, but that's not right? */
      }
      column++;
   }
   farmemfree((char far *)verhulst_array);
   return(0);
}

static void verhulst()          /* P. F. Verhulst (1845) */
{
   unsigned int pixel_row, errors;
   unsigned long counter;

    if (integerfractal)
       lPopulation = (parm.y == 0) ? (long)(SEED*fudge) : (long)(parm.y*fudge);
    else
       Population = (parm.y == 0 ) ? SEED : parm.y;

   errors = overflow = FALSE;

   for (counter=0 ; counter < filter_cycles ; counter++)
   {
      errors = curfractalspecific->orbitcalc();
      if (errors)
         return;
   }
   if (half_time_check) /* check for periodicity at half-time */
   {
      Bif_Period_Init();
      for (counter=0 ; counter < (unsigned long)maxit ; counter++)
      {
         errors = curfractalspecific->orbitcalc();
         if (errors) return;
         if (periodicitycheck && Bif_Periodic(counter)) break;
      }
      if (counter >= (unsigned long)maxit)   /* if not periodic, go the distance */
      {
         for (counter=0 ; counter < filter_cycles ; counter++)
         {
            errors = curfractalspecific->orbitcalc();
            if (errors) return;
         }
      }
   }

   if (periodicitycheck) Bif_Period_Init();
   for (counter=0 ; counter < (unsigned long)maxit ; counter++)
   {
      errors = curfractalspecific->orbitcalc();
      if (errors) return;

      /* assign population value to Y coordinate in pixels */
      if (integerfractal)
         pixel_row = iystop - (int)((lPopulation - linit.y) / dely); /* iystop */
      else
         pixel_row = iystop - (int)((Population - init.y) / delyy);

      /* if it's visible on the screen, save it in the column array */
      if (pixel_row <= (unsigned int)iystop) /* JCO 6/6/92 */
         verhulst_array[ pixel_row ] ++;
      if (periodicitycheck && Bif_Periodic(counter))
      {
         if (pixel_row <= (unsigned int)iystop) /* JCO 6/6/92 */
            verhulst_array[ pixel_row ] --;
         break;
      }
   }
}
static  long    lBif_closenuf, lBif_savedpop;   /* poss future use  */
static  double   Bif_closenuf,  Bif_savedpop;
static  int      Bif_savedinc;
static  long     Bif_savedand;

static void Bif_Period_Init()
{
   Bif_savedinc = 1;
   Bif_savedand = 1;
   if (integerfractal)
   {
      lBif_savedpop = -1;
      lBif_closenuf = dely / 8;
   }
   else
   {
      Bif_savedpop = -1.0;
      Bif_closenuf = (double)delyy / 8.0;
   }
}

static int _fastcall Bif_Periodic (long time)  /* Bifurcation Population Periodicity Check */
/* Returns : 1 if periodicity found, else 0 */
{
   if ((time & Bif_savedand) == 0)      /* time to save a new value */
   {
      if (integerfractal) lBif_savedpop = lPopulation;
      else                   Bif_savedpop =  Population;
      if (--Bif_savedinc == 0)
      {
         Bif_savedand = (Bif_savedand << 1) + 1;
         Bif_savedinc = 4;
      }
   }
   else                         /* check against an old save */
   {
      if (integerfractal)
      {
         if (labs(lBif_savedpop-lPopulation) <= lBif_closenuf)
            return(1);
      }
      else
      {
         if (fabs(Bif_savedpop-Population) <= Bif_closenuf)
            return(1);
      }
   }
   return(0);
}

/**********************************************************************/
/*                                                                                                    */
/* The following are Bifurcation "orbitcalc" routines...              */
/*                                                                                                    */
/**********************************************************************/
#ifdef XFRACT
int BifurcLambda() /* Used by lyanupov */
  {
    Population = Rate * Population * (1 - Population);
    return (fabs(Population) > BIG);
  }
#endif

/* Modified formulas below to generalize bifurcations. JCO 7/3/92 */

#define LCMPLXtrig0(arg,out) Arg1->l = (arg); ltrig0(); (out)=Arg1->l
#define  CMPLXtrig0(arg,out) Arg1->d = (arg); dtrig0(); (out)=Arg1->d

int BifurcVerhulstTrig()
  {
/*  Population = Pop + Rate * fn(Pop) * (1 - fn(Pop)) */
    tmp.x = Population;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population += Rate * tmp.x * (1 - tmp.x);
    return (fabs(Population) > BIG);
  }

int LongBifurcVerhulstTrig()
  {
#ifndef XFRACT
    ltmp.x = lPopulation;
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    ltmp.y = ltmp.x - multiply(ltmp.x,ltmp.x,bitshift);
    lPopulation += multiply(lRate,ltmp.y,bitshift);
#endif
    return (overflow);
  }

int BifurcStewartTrig()
  {
/*  Population = (Rate * fn(Population) * fn(Population)) - 1.0 */
    tmp.x = Population;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population = (Rate * tmp.x * tmp.x) - 1.0;
    return (fabs(Population) > BIG);
  }

int LongBifurcStewartTrig()
  {
#ifndef XFRACT
    ltmp.x = lPopulation;
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    lPopulation = multiply(ltmp.x,ltmp.x,bitshift);
    lPopulation = multiply(lPopulation,lRate,      bitshift);
    lPopulation -= fudge;
#endif
    return (overflow);
  }

int BifurcSetTrigPi()
  {
    tmp.x = Population * PI;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population = Rate * tmp.x;
    return (fabs(Population) > BIG);
  }

int LongBifurcSetTrigPi()
  {
#ifndef XFRACT
    ltmp.x = multiply(lPopulation,LPI,bitshift);
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    lPopulation = multiply(lRate,ltmp.x,bitshift);
#endif
    return (overflow);
  }

int BifurcAddTrigPi()
  {
    tmp.x = Population * PI;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population += Rate * tmp.x;
    return (fabs(Population) > BIG);
  }

int LongBifurcAddTrigPi()
  {
#ifndef XFRACT
    ltmp.x = multiply(lPopulation,LPI,bitshift);
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    lPopulation += multiply(lRate,ltmp.x,bitshift);
#endif
    return (overflow);
  }

int BifurcLambdaTrig()
  {
/*  Population = Rate * fn(Population) * (1 - fn(Population)) */
    tmp.x = Population;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population = Rate * tmp.x * (1 - tmp.x);
    return (fabs(Population) > BIG);
  }

int LongBifurcLambdaTrig()
  {
#ifndef XFRACT
    ltmp.x = lPopulation;
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    ltmp.y = ltmp.x - multiply(ltmp.x,ltmp.x,bitshift);
    lPopulation = multiply(lRate,ltmp.y,bitshift);
#endif
    return (overflow);
  }

#define LCMPLXpwr(arg1,arg2,out)    Arg2->l = (arg1); Arg1->l = (arg2);\
         lStkPwr(); Arg1++; Arg2++; (out) = Arg2->l

long beta;

int BifurcMay()
  { /* X = (lambda * X) / (1 + X)^beta, from R.May as described in Pickover,
            Computers, Pattern, Chaos, and Beauty, page 153 */
    tmp.x = 1.0 + Population;
    tmp.x = pow(tmp.x, -beta); /* pow in math.h included with mpmath.h */
    Population = (Rate * Population) * tmp.x;
    return (fabs(Population) > BIG);
  }

int LongBifurcMay()
  {
#ifndef XFRACT
    ltmp.x = lPopulation + fudge;
    ltmp.y = 0;
    lparm2.x = beta * fudge;
    LCMPLXpwr(ltmp, lparm2, ltmp);
    lPopulation = multiply(lRate,lPopulation,bitshift);
    lPopulation = divide(lPopulation,ltmp.x,bitshift);
#endif
    return (overflow);
  }

int BifurcMaySetup()
  {

   beta = (long)param[2];
   if(beta < 2)
      beta = 2;
   param[2] = (double)beta;

   timer(0,curfractalspecific->calctype);
   return(0);
  }

/* Here Endeth the Generalised Bifurcation Fractal Engine   */

/* END Phil Wilson's Code (modified slightly by Kev Allen et. al. !) */


/******************* standalone engine for "popcorn" ********************/

int popcorn()   /* subset of std engine */
{
   int start_row;
   start_row = 0;
   if (resuming)
   {
      start_resume();
      get_resume(sizeof(start_row),&start_row,0);
      end_resume();
   }
   kbdcount=max_kbdcount;
   plot = noplot;
   tempsqrx = ltempsqrx = 0; /* PB added this to cover weird BAILOUTs */
   for (row = start_row; row <= iystop; row++)
   {
      reset_periodicity = 1;
      for (col = 0; col <= ixstop; col++)
      {
         if (StandardFractal() == -1) /* interrupted */
         {
            alloc_resume(10,1);
            put_resume(sizeof(row),&row,0);
            return(-1);
         }
         reset_periodicity = 0;
      }
   }
   calc_status = 4;
   return(0);
}

/******************* standalone engine for "lyapunov" *********************/
/*** Roy Murphy [76376,721]                                             ***/
/*** revision history:                                                  ***/
/*** initial version: Winter '91                                        ***/
/***    Fall '92 integration of Nicholas Wilt's ASM speedups            ***/
/***    Jan 93' integration with calcfrac() yielding boundary tracing,  ***/
/***    tesseral, and solid guessing, and inversion, inside=nnn         ***/
/*** save_release behavior:                                             ***/
/***    1730 & prior: ignores inside=, calcmode='1', (a,b)->(x,y)       ***/
/***    1731: other calcmodes and inside=nnn                            ***/
/***    1732: the infamous axis swap: (b,a)->(x,y),                     ***/
/***            the order parameter becomes a long int                  ***/
/**************************************************************************/
int lyaLength, lyaSeedOK;
int lyaRxy[34];

#define WES 1   /* define WES to be 0 to use Nick's lyapunov.obj */
#if WES
int lyapunov_cycles(double, double);
#else
int lyapunov_cycles(int, double, double, double);
#endif

int lyapunov_cycles_in_c(long, double, double);

int lyapunov () {
    double a, b;

    if (keypressed()) {
        return -1;
        }
    overflow=FALSE;
    if (param[1]==1) Population = (1.0+rand())/(2.0+RAND_MAX);
    else if (param[1]==0) {
        if (fabs(Population)>BIG || Population==0 || Population==1)
            Population = (1.0+rand())/(2.0+RAND_MAX);
        }
    else Population = param[1];
    (*plot)(col, row, 1);
    if (invert) {
        invertz2(&init);
        a = init.y;
        b = init.x;
        }
    else {
        a = dypixel();
        b = dxpixel();
        }
#ifndef XFRACT
    /*  the assembler routines don't work for a & b outside the
        ranges 0 < a < 4 and 0 < b < 4. So, fall back on the C
        routines if part of the image sticks out.
        */
#if WES
        color=lyapunov_cycles(a, b);
#else
    if (lyaSeedOK && a>0 && b>0 && a<=4 && b<=4)
        color=lyapunov_cycles(filter_cycles, Population, a, b);
    else
        color=lyapunov_cycles_in_c(filter_cycles, a, b);
#endif
#else
    color=lyapunov_cycles_in_c(filter_cycles, a, b);
#endif
    if (inside>0 && color==0)
        color = inside;
    else if (color>=colors)
        color = colors-1;
    (*plot)(col, row, color);
    return color;
}


int lya_setup () {
    /* This routine sets up the sequence for forcing the Rate parameter
        to vary between the two values.  It fills the array lyaRxy[] and
        sets lyaLength to the length of the sequence.

        The sequence is coded in the bit pattern in an integer.
        Briefly, the sequence starts with an A the leading zero bits
        are ignored and the remaining bit sequence is decoded.  The
        sequence ends with a B.  Not all possible sequences can be
        represented in this manner, but every possible sequence is
        either represented as itself, as a rotation of one of the
        representable sequences, or as the inverse of a representable
        sequence (swapping 0s and 1s in the array.)  Sequences that
        are the rotation and/or inverses of another sequence will generate
        the same lyapunov exponents.

        A few examples follow:
            number    sequence
                0       ab
                1       aab
                2       aabb
                3       aaab
                4       aabbb
                5       aabab
                6       aaabb (this is a duplicate of 4, a rotated inverse)
                7       aaaab
                8       aabbbb  etc.
         */

    long i;
    int t;

    if ((filter_cycles=(long)param[2])==0)
        filter_cycles=maxit/2;
    lyaSeedOK = param[1]>0 && param[1]<=1 && debugflag!=90;
    lyaLength = 1;

    i = (long)param[0];
#ifndef XFRACT
    if (save_release<1732) i &= 0x0FFFFL; /* make it a short to reporduce prior stuff*/
#endif
    lyaRxy[0] = 1;
    for (t=31; t>=0; t--)
        if (i & (1<<t)) break;
    for (; t>=0; t--)
        lyaRxy[lyaLength++] = (i & (1<<t)) != 0;
    lyaRxy[lyaLength++] = 0;
    if (save_release<1732)              /* swap axes prior to 1732 */
        for (t=lyaLength; t>=0; t--)
            lyaRxy[t] = !lyaRxy[t];
    if (save_release<1731) {            /* ignore inside=, stdcalcmode */
        stdcalcmode='1';
        if (inside==1) inside = 0;
        }
    if (inside<0) {
        static FCODE msg[]=
            {"Sorry, inside options other than inside=nnn are not supported by the lyapunov"};
        stopmsg(0,(char far *)msg);
        inside=1;
        }
    if (usr_stdcalcmode == 'o') { /* Oops,lyapunov type */
        usr_stdcalcmode = '1';  /* doesn't use new & breaks orbits */
        stdcalcmode = '1';
        }
    return 1;
}

int lyapunov_cycles_in_c(long filter_cycles, double a, double b) {
    int color, count, lnadjust;
    long i;
    double lyap, total, temp;
    /* e10=22026.4657948  e-10=0.0000453999297625 */

    total = 1.0;
    lnadjust = 0;
    for (i=0; i<filter_cycles; i++) {
        for (count=0; count<lyaLength; count++) {
            Rate = lyaRxy[count] ? a : b;
            if (curfractalspecific->orbitcalc()) {
                overflow = TRUE;
                goto jumpout;
                }
            }
        }
    for (i=0; i < maxit/2; i++) {
        for (count = 0; count < lyaLength; count++) {
            Rate = lyaRxy[count] ? a : b;
            if (curfractalspecific->orbitcalc()) {
                overflow = TRUE;
                goto jumpout;
                }
            temp = fabs(Rate-2.0*Rate*Population);
                if ((total *= (temp))==0) {
                overflow = TRUE;
                goto jumpout;
                }
            }
        while (total > 22026.4657948) {
            total *= 0.0000453999297625;
            lnadjust += 10;
            }
        while (total < 0.0000453999297625) {
            total *= 22026.4657948;
            lnadjust -= 10;
            }
        }

jumpout:
    if (overflow || total <= 0 || (temp = log(total) + lnadjust) > 0)
        color = 0;
    else {
        if (LogFlag)
        lyap = -temp/((double) lyaLength*i);
    else
        lyap = 1 - exp(temp/((double) lyaLength*i));
        color = 1 + (int)(lyap * (colors-1));
        }
    return color;
}


/******************* standalone engine for "cellular" ********************/
/* Originally coded by Ken Shirriff.
   Modified beyond recognition by Jonathan Osuch.
     Original or'd the neighborhood, changed to sum the neighborhood
     Changed prompts and error messages
     Added CA types
     Set the palette to some standard? CA colors
     Changed *cell_array to near and used dstack so put_line and get_line
       could be used all the time
     Made space bar generate next screen
     Increased string/rule size to 16 digits and added CA types 9/20/92
*/

#define BAD_T         1
#define BAD_MEM       2
#define STRING1       3
#define STRING2       4
#define TABLEK        5
#define TYPEKR        6
#define RULELENGTH    7
#define INTERUPT      8

#define CELLULAR_DONE 10

#ifndef XFRACT
static BYTE *cell_array[2];
#else
static BYTE far *cell_array[2];
#endif

S16 r, k_1, rule_digits;
int lstscreenflag;

void abort_cellular(int err, int t)
{
   int i;
   switch (err)
   {
      case BAD_T:
         {
         char msg[30];
         sprintf(msg,"Bad t=%d, aborting\n", t);
         stopmsg(0,(char far *)msg);
         }
         break;
      case BAD_MEM:
         {
         static FCODE msg[]={"Insufficient free memory for calculation" };
         stopmsg(0,msg);
         }
         break;
      case STRING1:
         {
         static FCODE msg[]={"String can be a maximum of 16 digits" };
         stopmsg(0,msg);
         }
         break;
      case STRING2:
         {
         static FCODE msg[]={"Make string of 0's through  's" };
         msg[27] = (char)(k_1 + 48); /* turn into a character value */
         stopmsg(0,msg);
         }
         break;
      case TABLEK:
         {
         static FCODE msg[]={"Make Rule with 0's through  's" };
         msg[27] = (char)(k_1 + 48); /* turn into a character value */
         stopmsg(0,msg);
         }
         break;
      case TYPEKR:
         {
         static FCODE msg[]={"Type must be 21, 31, 41, 51, 61, 22, 32, \
42, 23, 33, 24, 25, 26, 27" };
         stopmsg(0,msg);
         }
         break;
      case RULELENGTH:
         {
         static FCODE msg[]={"Rule must be    digits long" };
         i = rule_digits / 10;
         if(i==0)
            msg[14] = (char)(rule_digits + 48);
         else {
            msg[13] = (char)(i+48);
            msg[14] = (char)((rule_digits % 10) + 48);
         }
         stopmsg(0,msg);
         }
         break;
      case INTERUPT:
         {
         static FCODE msg[]={"Interrupted, can't resume" };
         stopmsg(0,msg);
         }
         break;
      case CELLULAR_DONE:
         break;
   }
   if(cell_array[0] != NULL)
#ifndef XFRACT
      cell_array[0] = NULL;
#else
      farmemfree((char far *)cell_array[0]);
#endif
   if(cell_array[1] != NULL)
#ifndef XFRACT
      cell_array[1] = NULL;
#else
      farmemfree((char far *)cell_array[1]);
#endif
}

int cellular () {
   S16 start_row;
   S16 filled, notfilled;
   U16 cell_table[32];
   U16 init_string[16];
   U16 kr, k;
   U32 lnnmbr;
   U16 i, twor;
   S16 t, t2;
   S32 randparam;
   double n;
   char buf[30];

   set_Cellular_palette();

   randparam = (S32)param[0];
   lnnmbr = (U32)param[3];
   kr = (U16)param[2];
   switch (kr) {
     case 21:
     case 31:
     case 41:
     case 51:
     case 61:
     case 22:
     case 32:
     case 42:
     case 23:
     case 33:
     case 24:
     case 25:
     case 26:
     case 27:
        break;
     default:
        abort_cellular(TYPEKR, 0);
        return -1;
        /* break; */
   }

   r = (S16)(kr % 10); /* Number of nearest neighbors to sum */
   k = (U16)(kr / 10); /* Number of different states, k=3 has states 0,1,2 */
   k_1 = (S16)(k - 1); /* Highest state value, k=3 has highest state value of 2 */
   rule_digits = (S16)((r * 2 + 1) * k_1 + 1); /* Number of digits in the rule */

   if ((!rflag) && randparam == -1)
       --rseed;
   if (randparam != 0 && randparam != -1) {
      n = param[0];
      sprintf(buf,"%.16g",n); /* # of digits in initial string */
      t = (S16)strlen(buf);
      if (t>16 || t <= 0) {
         abort_cellular(STRING1, 0);
         return -1;
      }
      for (i=0;i<16;i++)
         init_string[i] = 0; /* zero the array */
      t2 = (S16) ((16 - t)/2);
      for (i=0;i<(U16)t;i++) { /* center initial string in array */
         init_string[i+t2] = (U16)(buf[i] - 48); /* change character to number */
         if (init_string[i+t2]>(U16)k_1) {
            abort_cellular(STRING2, 0);
            return -1;
         }
      }
   }

   srand(rseed);
   if (!rflag) ++rseed;

/* generate rule table from parameter 1 */
#ifndef XFRACT
   n = param[1];
#else
   /* gcc can't manage to convert a big double to an unsigned long properly. */
   if (param[1]>0x7fffffff) {
       n = (param[1]-0x7fffffff);
       n += 0x7fffffff;
   } else {
       n = param[1];
   }
#endif
   if (n == 0) { /* calculate a random rule */
      n = rand()%(int)k;
      for (i=1;i<(U16)rule_digits;i++) {
         n *= 10;
         n += rand()%(int)k;
      }
      param[1] = n;
   }
   sprintf(buf,"%.*g",rule_digits ,n);
   t = (S16)strlen(buf);
   if (rule_digits < t || t < 0) { /* leading 0s could make t smaller */
      abort_cellular(RULELENGTH, 0);
      return -1;
   }
   for (i=0;i<(U16)rule_digits;i++) /* zero the table */
      cell_table[i] = 0;
   for (i=0;i<(U16)t;i++) { /* reverse order */
      cell_table[i] = (U16)(buf[t-i-1] - 48); /* change character to number */
      if (cell_table[i]>(U16)k_1) {
         abort_cellular(TABLEK, 0);
         return -1;
      }
   }


   start_row = 0;
#ifndef XFRACT
  /* two 4096 byte arrays, at present at most 2024 + 1 bytes should be */
  /* needed in each array (max screen width + 1) */
   cell_array[0] = (BYTE *)&dstack[0]; /* dstack is in general.asm */
   cell_array[1] = (BYTE *)&boxy[0]; /* boxy is in general.asm */
#else
   cell_array[0] = (BYTE far *)farmemalloc(ixstop+1);
   cell_array[1] = (BYTE far *)farmemalloc(ixstop+1);
#endif
   if (cell_array[0]==NULL || cell_array[1]==NULL) {
      abort_cellular(BAD_MEM, 0);
      return(-1);
   }

/* nxtscreenflag toggled by space bar in fractint.c, 1 for continuous */
/* 0 to stop on next screen */

   filled = 0;
   notfilled = (S16)(1-filled);
   if (resuming && !nxtscreenflag && !lstscreenflag) {
      start_resume();
      get_resume(sizeof(start_row),&start_row,0);
      end_resume();
      get_line(start_row,0,ixstop,cell_array[filled]);
   }
   else if (nxtscreenflag && !lstscreenflag) {
      start_resume();
      end_resume();
      get_line(iystop,0,ixstop,cell_array[filled]);
      param[3] += iystop + 1;
      start_row = -1; /* after 1st iteration its = 0 */
   }
   else {
    if(rflag || randparam==0 || randparam==-1){
      for (col=0;col<=ixstop;col++) {
         cell_array[filled][col] = (BYTE)(rand()%(int)k);
      }
    } /* end of if random */

    else {
      for (col=0;col<=ixstop;col++) { /* Clear from end to end */
         cell_array[filled][col] = 0;
      }
      i = 0;
      for (col=(ixstop-16)/2;col<(ixstop+16)/2;col++) { /* insert initial */
         cell_array[filled][col] = (BYTE)init_string[i++];    /* string */
      }
    } /* end of if not random */
    if (lnnmbr != 0)
      lstscreenflag = 1;
    else
      lstscreenflag = 0;
    put_line(start_row,0,ixstop,cell_array[filled]);
   }
   start_row++;

/* This section calculates the starting line when it is not zero */
/* This section can't be resumed since no screen output is generated */
/* calculates the (lnnmbr - 1) generation */
   if (lstscreenflag) { /* line number != 0 & not resuming & not continuing */
     U32 big_row;
     for (big_row = (U32)start_row; big_row < lnnmbr; big_row++) {
      static FCODE msg[]={"Cellular thinking (higher start row takes longer)"};

      thinking(1,msg);
      if(rflag || randparam==0 || randparam==-1){
       /* Use a random border */
       for (i=0;i<=(U16)r;i++) {
         cell_array[notfilled][i]=(BYTE)(rand()%(int)k);
         cell_array[notfilled][ixstop-i]=(BYTE)(rand()%(int)k);
       }
      }
      else {
       /* Use a zero border */
       for (i=0;i<=(U16)r;i++) {
         cell_array[notfilled][i]=0;
         cell_array[notfilled][ixstop-i]=0;
       }
      }

       t = 0; /* do first cell */
       for (twor=(U16)(r+r),i=0;i<=twor;i++)
           t = (S16)(t + (S16)cell_array[filled][i]);
       if (t>rule_digits || t<0) {
         thinking(0, NULL);
         abort_cellular(BAD_T, t);
         return(-1);
       }
       cell_array[notfilled][r] = (BYTE)cell_table[t];

           /* use a rolling sum in t */
       for (col=r+1;col<ixstop-r;col++) { /* now do the rest */
         t = (S16)(t + cell_array[filled][col+r] - cell_array[filled][col-r-1]);
         if (t>rule_digits || t<0) {
           thinking(0, NULL);
           abort_cellular(BAD_T, t);
           return(-1);
         }
         cell_array[notfilled][col] = (BYTE)cell_table[t];
       }

       filled = notfilled;
       notfilled = (S16)(1-filled);
       if (keypressed()) {
          thinking(0, NULL);
          abort_cellular(INTERUPT, 0);
          return -1;
       }
     }
   start_row = 0;
   thinking(0, NULL);
   lstscreenflag = 0;
   }

/* This section does all the work */
contloop:
   for (row = start_row; row <= iystop; row++) {

      if(rflag || randparam==0 || randparam==-1){
       /* Use a random border */
       for (i=0;i<=(U16)r;i++) {
         cell_array[notfilled][i]=(BYTE)(rand()%(int)k);
         cell_array[notfilled][ixstop-i]=(BYTE)(rand()%(int)k);
       }
      }
      else {
       /* Use a zero border */
       for (i=0;i<=(U16)r;i++) {
         cell_array[notfilled][i]=0;
         cell_array[notfilled][ixstop-i]=0;
       }
      }

       t = 0; /* do first cell */
       for (twor=(U16)(r+r),i=0;i<=twor;i++)
           t = (S16)(t + (S16)cell_array[filled][i]);
       if (t>rule_digits || t<0) {
         thinking(0, NULL);
         abort_cellular(BAD_T, t);
         return(-1);
       }
       cell_array[notfilled][r] = (BYTE)cell_table[t];

           /* use a rolling sum in t */
       for (col=r+1;col<ixstop-r;col++) { /* now do the rest */
         t = (S16)(t + cell_array[filled][col+r] - cell_array[filled][col-r-1]);
         if (t>rule_digits || t<0) {
           thinking(0, NULL);
           abort_cellular(BAD_T, t);
           return(-1);
         }
         cell_array[notfilled][col] = (BYTE)cell_table[t];
       }

       filled = notfilled;
       notfilled = (S16)(1-filled);
       put_line(row,0,ixstop,cell_array[filled]);
       if (keypressed()) {
          abort_cellular(CELLULAR_DONE, 0);
          alloc_resume(10,1);
          put_resume(sizeof(row),&row,0);
          return -1;
       }
   }
   if(nxtscreenflag) {
     param[3] += iystop + 1;
     start_row = -1; /* after 1st iteration its = 0 */
     goto contloop;
   }
  abort_cellular(CELLULAR_DONE, 0);
  return 1;
}

int CellularSetup(void)
{
   if (!resuming) {
      nxtscreenflag = 0; /* initialize flag */
   }
   timer(0,curfractalspecific->calctype);
   return(0);
}

static void set_Cellular_palette()
{
   static Palettetype Red    = { 42, 0, 0 };
   static Palettetype Green  = { 10,35,10 };
   static Palettetype Blue   = { 13,12,29 };
   static Palettetype Yellow = { 60,58,18 };
   static Palettetype Brown  = { 42,21, 0 };

   if (mapdacbox) return;               /* map= specified */

   dac[0].red  = 0 ;
   dac[0].green= 0 ;
   dac[0].blue = 0 ;

   dac[1].red    = Red.red;
   dac[1].green = Red.green;
   dac[1].blue  = Red.blue;

   dac[2].red   = Green.red;
   dac[2].green = Green.green;
   dac[2].blue  = Green.blue;

   dac[3].red   = Blue.red;
   dac[3].green = Blue.green;
   dac[3].blue  = Blue.blue;

   dac[4].red   = Yellow.red;
   dac[4].green = Yellow.green;
   dac[4].blue  = Yellow.blue;

   dac[5].red   = Brown.red;
   dac[5].green = Brown.green;
   dac[5].blue  = Brown.blue;

   SetTgaColors();
   spindac(0,1);
}

/* frothy basin routines */

#define FROTH_BITSHIFT      28
#define FROTH_D_TO_L(x)     ((long)((x)*(1L<<FROTH_BITSHIFT)))
#define FROTH_CLOSE         1e-6      /* seems like a good value */
#define FROTH_LCLOSE        FROTH_D_TO_L(FROTH_CLOSE)
#define SQRT3               1.732050807568877193
#define FROTH_SLOPE         SQRT3
#define FROTH_LSLOPE        FROTH_D_TO_L(FROTH_SLOPE)
#define FROTH_CRITICAL_A    1.028713768218725  /* 1.0287137682187249127 */
#define froth_top_x_mapping(x)  ((x)*(x)-(x)-3*fsp->fl.f.a*fsp->fl.f.a/4)


static char froth3_256c[] = "froth3.map";
static char froth6_256c[] = "froth6.map";
static char froth3_16c[] =  "froth316.map";
static char froth6_16c[] =  "froth616.map";

struct froth_double_struct {
    double a;
    double halfa;
    double top_x1;
    double top_x2;
    double top_x3;
    double top_x4;
    double left_x1;
    double left_x2;
    double left_x3;
    double left_x4;
    double right_x1;
    double right_x2;
    double right_x3;
    double right_x4;
    };

struct froth_long_struct {
    long a;
    long halfa;
    long top_x1;
    long top_x2;
    long top_x3;
    long top_x4;
    long left_x1;
    long left_x2;
    long left_x3;
    long left_x4;
    long right_x1;
    long right_x2;
    long right_x3;
    long right_x4;
    };

struct froth_struct {
    int repeat_mapping;
    int altcolor;
    int attractors;
    int shades;
    union { /* This was made into a union to save 56 malloc()'ed bytes. */
        struct froth_double_struct f;
        struct froth_long_struct l;
        } fl;
    };

struct froth_struct *fsp=NULL; /* froth_struct pointer */

/* color maps which attempt to replicate the images of James Alexander. */
static void set_Froth_palette(void)
   {
   char *mapname;

   if (colorstate != 0) /* 0 means dacbox matches default */
      return;
   if (colors >= 16)
      {
      if (colors >= 256)
         {
         if (fsp->attractors == 6)
            mapname = froth6_256c;
         else
            mapname = froth3_256c;
         }
      else /* colors >= 16 */
         {
         if (fsp->attractors == 6)
            mapname = froth6_16c;
         else
            mapname = froth3_16c;
         }
      if (ValidateLuts(mapname) != 0)
         return;
      colorstate = 0; /* treat map it as default */
      spindac(0,1);
      }
   }

int froth_setup(void)
   {
   double sin_theta, cos_theta, x0, y0;

   sin_theta = SQRT3/2; /* sin(2*PI/3) */
   cos_theta = -0.5;    /* cos(2*PI/3) */

   /* check for NULL as safety net */
   if (fsp == NULL)
      fsp = (struct froth_struct *)malloc(sizeof (struct froth_struct));
   if (fsp == NULL)
      {
      static FCODE msg[]=
          {"Sorry, not enough memory to run the frothybasin fractal type"};
      stopmsg(0,(char far *)msg);
      return 0;
      }

   /* for the all important backwards compatibility */
   if (save_release <= 1821)   /* book version is 18.21 */
      {
      /* use old release parameters */

      fsp->repeat_mapping = ((int)param[0] == 6 || (int)param[0] == 2); /* map 1 or 2 times (3 or 6 basins)  */
      fsp->altcolor = (int)param[1];
      param[2] = 0; /* throw away any value used prior to 18.20 */

      fsp->attractors = !fsp->repeat_mapping ? 3 : 6;

      /* use old values */                /* old names */
      fsp->fl.f.a = 1.02871376822;          /* A     */
      fsp->fl.f.halfa = fsp->fl.f.a/2;      /* A/2   */

      fsp->fl.f.top_x1 = -1.04368901270;    /* X1MIN */
      fsp->fl.f.top_x2 =  1.33928675524;    /* X1MAX */
      fsp->fl.f.top_x3 = -0.339286755220;   /* XMIDT */
      fsp->fl.f.top_x4 = -0.339286755220;   /* XMIDT */

      fsp->fl.f.left_x1 =  0.07639837810;   /* X3MAX2 */
      fsp->fl.f.left_x2 = -1.11508950586;   /* X2MIN2 */
      fsp->fl.f.left_x3 = -0.27580275066;   /* XMIDL  */
      fsp->fl.f.left_x4 = -0.27580275066;   /* XMIDL  */

      fsp->fl.f.right_x1 =  0.96729063460;  /* X2MAX1 */
      fsp->fl.f.right_x2 = -0.22419724936;  /* X3MIN1 */
      fsp->fl.f.right_x3 =  0.61508950585;  /* XMIDR  */
      fsp->fl.f.right_x4 =  0.61508950585;  /* XMIDR  */

      }
   else /* use new code */
      {
      if (param[0] != 2)
         param[0] = 1;
      fsp->repeat_mapping = (int)param[0] == 2;
      if (param[1] != 0)
         param[1] = 1;
      fsp->altcolor = (int)param[1];
      fsp->fl.f.a = param[2];

      fsp->attractors = fabs(fsp->fl.f.a) <= FROTH_CRITICAL_A ? (!fsp->repeat_mapping ? 3 : 6)
                                                              : (!fsp->repeat_mapping ? 2 : 3);

      /* new improved values */
      /* 0.5 is the value that causes the mapping to reach a minimum */
      x0 = 0.5;
      /* a/2 is the value that causes the y value to be invariant over the mappings */
      y0 = fsp->fl.f.halfa = fsp->fl.f.a/2;
      fsp->fl.f.top_x1 = froth_top_x_mapping(x0);
      fsp->fl.f.top_x2 = froth_top_x_mapping(fsp->fl.f.top_x1);
      fsp->fl.f.top_x3 = froth_top_x_mapping(fsp->fl.f.top_x2);
      fsp->fl.f.top_x4 = froth_top_x_mapping(fsp->fl.f.top_x3);

      /* rotate 120 degrees counter-clock-wise */
      fsp->fl.f.left_x1 = fsp->fl.f.top_x1 * cos_theta - y0 * sin_theta;
      fsp->fl.f.left_x2 = fsp->fl.f.top_x2 * cos_theta - y0 * sin_theta;
      fsp->fl.f.left_x3 = fsp->fl.f.top_x3 * cos_theta - y0 * sin_theta;
      fsp->fl.f.left_x4 = fsp->fl.f.top_x4 * cos_theta - y0 * sin_theta;

      /* rotate 120 degrees clock-wise */
      fsp->fl.f.right_x1 = fsp->fl.f.top_x1 * cos_theta + y0 * sin_theta;
      fsp->fl.f.right_x2 = fsp->fl.f.top_x2 * cos_theta + y0 * sin_theta;
      fsp->fl.f.right_x3 = fsp->fl.f.top_x3 * cos_theta + y0 * sin_theta;
      fsp->fl.f.right_x4 = fsp->fl.f.top_x4 * cos_theta + y0 * sin_theta;

      }

   /* if 2 attractors, use same shades as 3 attractors */
   fsp->shades = (colors-1) / max(3,fsp->attractors);

   /* rqlim needs to be at least sq(1+sqrt(1+sq(a))), */
   /* which is never bigger than 6.93..., so we'll call it 7.0 */
   if (rqlim < 7.0)
      rqlim=7.0;
   set_Froth_palette();
   /* make the best of the .map situation */
   orbit_color = fsp->attractors != 6 && colors >= 16 ? (fsp->shades<<1)+1 : colors-1;

   if (integerfractal)
      {
      struct froth_long_struct tmp_l;

      tmp_l.a        = FROTH_D_TO_L(fsp->fl.f.a);
      tmp_l.halfa    = FROTH_D_TO_L(fsp->fl.f.halfa);

      tmp_l.top_x1   = FROTH_D_TO_L(fsp->fl.f.top_x1);
      tmp_l.top_x2   = FROTH_D_TO_L(fsp->fl.f.top_x2);
      tmp_l.top_x3   = FROTH_D_TO_L(fsp->fl.f.top_x3);
      tmp_l.top_x4   = FROTH_D_TO_L(fsp->fl.f.top_x4);

      tmp_l.left_x1  = FROTH_D_TO_L(fsp->fl.f.left_x1);
      tmp_l.left_x2  = FROTH_D_TO_L(fsp->fl.f.left_x2);
      tmp_l.left_x3  = FROTH_D_TO_L(fsp->fl.f.left_x3);
      tmp_l.left_x4  = FROTH_D_TO_L(fsp->fl.f.left_x4);

      tmp_l.right_x1 = FROTH_D_TO_L(fsp->fl.f.right_x1);
      tmp_l.right_x2 = FROTH_D_TO_L(fsp->fl.f.right_x2);
      tmp_l.right_x3 = FROTH_D_TO_L(fsp->fl.f.right_x3);
      tmp_l.right_x4 = FROTH_D_TO_L(fsp->fl.f.right_x4);

      fsp->fl.l = tmp_l;
      }
   return 1;
   }

void froth_cleanup(void)
   {
   if (fsp != NULL)
      free(fsp);
   /* set to NULL as a flag that froth_cleanup() has been called */
   fsp = NULL;
   }


/* Froth Fractal type */
int calcfroth(void)   /* per pixel 1/2/g, called with row & col set */
     {
     int found_attractor=0;

   if (check_key()) {
        return -1;
        }

   if (fsp == NULL)
      { /* error occured allocating memory for fsp */
      return 0;
      }

   orbit_ptr = 0;
   coloriter = 0;
   if(showdot>0)
      (*plot) (col, row, showdot%colors);
   if (!integerfractal) /* fp mode */
      {
      if(invert)
         {
         invertz2(&tmp);
         old = tmp;
         }
      else
         {
         old.x = dxpixel();
         old.y = dypixel();
         }

      while (!found_attractor
             && ((tempsqrx=sqr(old.x)) + (tempsqry=sqr(old.y)) < rqlim)
             && (coloriter < maxit))
         {
         /* simple formula: z = z^2 + conj(z*(-1+ai)) */
         /* but it's the attractor that makes this so interesting */
         new.x = tempsqrx - tempsqry - old.x - fsp->fl.f.a*old.y;
         old.y += (old.x+old.x)*old.y - fsp->fl.f.a*old.x;
         old.x = new.x;
         if (fsp->repeat_mapping)
            {
            new.x = sqr(old.x) - sqr(old.y) - old.x - fsp->fl.f.a*old.y;
            old.y += (old.x+old.x)*old.y - fsp->fl.f.a*old.x;
            old.x = new.x;
            }

         coloriter++;

         if (show_orbit) {
            if (keypressed())
               break;
            plot_orbit(old.x, old.y, -1);
         }

         if (fabs(fsp->fl.f.halfa-old.y) < FROTH_CLOSE
                && old.x >= fsp->fl.f.top_x1 && old.x <= fsp->fl.f.top_x2)
            {
            if ((!fsp->repeat_mapping && fsp->attractors == 2)
                || (fsp->repeat_mapping && fsp->attractors == 3))
               found_attractor = 1;
            else if (old.x <= fsp->fl.f.top_x3)
               found_attractor = 1;
            else if (old.x >= fsp->fl.f.top_x4) {
               if (!fsp->repeat_mapping)
                  found_attractor = 1;
               else
                  found_attractor = 2;
             }
            }
         else if (fabs(FROTH_SLOPE*old.x - fsp->fl.f.a - old.y) < FROTH_CLOSE
                  && old.x <= fsp->fl.f.right_x1 && old.x >= fsp->fl.f.right_x2)
            {
            if (!fsp->repeat_mapping && fsp->attractors == 2)
               found_attractor = 2;
            else if (fsp->repeat_mapping && fsp->attractors == 3)
               found_attractor = 3;
            else if (old.x >= fsp->fl.f.right_x3) {
               if (!fsp->repeat_mapping)
                  found_attractor = 2;
               else
                  found_attractor = 4;
            }
            else if (old.x <= fsp->fl.f.right_x4) {
               if (!fsp->repeat_mapping)
                  found_attractor = 3;
               else
                  found_attractor = 6;
             }
            }
         else if (fabs(-FROTH_SLOPE*old.x - fsp->fl.f.a - old.y) < FROTH_CLOSE
                  && old.x <= fsp->fl.f.left_x1 && old.x >= fsp->fl.f.left_x2)
            {
            if (!fsp->repeat_mapping && fsp->attractors == 2)
               found_attractor = 2;
            else if (fsp->repeat_mapping && fsp->attractors == 3)
               found_attractor = 2;
            else if (old.x >= fsp->fl.f.left_x3) {
               if (!fsp->repeat_mapping)
                  found_attractor = 3;
               else
                  found_attractor = 5;
            }
            else if (old.x <= fsp->fl.f.left_x4) {
               if (!fsp->repeat_mapping)
                  found_attractor = 2;
               else
                  found_attractor = 3;
             }
            }
         }
      }
   else /* integer mode */
      {
      if(invert)
         {
         invertz2(&tmp);
         lold.x = (long)(tmp.x * fudge);
         lold.y = (long)(tmp.y * fudge);
         }
      else
         {
         lold.x = lxpixel();
         lold.y = lypixel();
         }

      while (!found_attractor && ((lmagnitud = (ltempsqrx=lsqr(lold.x)) + (ltempsqry=lsqr(lold.y))) < llimit)
             && (lmagnitud >= 0) && (coloriter < maxit))
         {
         /* simple formula: z = z^2 + conj(z*(-1+ai)) */
         /* but it's the attractor that makes this so interesting */
         lnew.x = ltempsqrx - ltempsqry - lold.x - multiply(fsp->fl.l.a,lold.y,bitshift);
         lold.y += (multiply(lold.x,lold.y,bitshift)<<1) - multiply(fsp->fl.l.a,lold.x,bitshift);
         lold.x = lnew.x;
         if (fsp->repeat_mapping)
            {
            lmagnitud = (ltempsqrx=lsqr(lold.x)) + (ltempsqry=lsqr(lold.y));
            if ((lmagnitud > llimit) || (lmagnitud < 0))
               break;
            lnew.x = ltempsqrx - ltempsqry - lold.x - multiply(fsp->fl.l.a,lold.y,bitshift);
            lold.y += (multiply(lold.x,lold.y,bitshift)<<1) - multiply(fsp->fl.l.a,lold.x,bitshift);
            lold.x = lnew.x;
            }
         coloriter++;

         if (show_orbit) {
            if (keypressed())
               break;
            iplot_orbit(lold.x, lold.y, -1);
         }

         if (labs(fsp->fl.l.halfa-lold.y) < FROTH_LCLOSE
              && lold.x > fsp->fl.l.top_x1 && lold.x < fsp->fl.l.top_x2)
            {
            if ((!fsp->repeat_mapping && fsp->attractors == 2)
                || (fsp->repeat_mapping && fsp->attractors == 3))
               found_attractor = 1;
            else if (lold.x <= fsp->fl.l.top_x3)
               found_attractor = 1;
            else if (lold.x >= fsp->fl.l.top_x4) {
               if (!fsp->repeat_mapping)
                  found_attractor = 1;
               else
                  found_attractor = 2;
             }
            }
         else if (labs(multiply(FROTH_LSLOPE,lold.x,bitshift)-fsp->fl.l.a-lold.y) < FROTH_LCLOSE
                  && lold.x <= fsp->fl.l.right_x1 && lold.x >= fsp->fl.l.right_x2)
            {
            if (!fsp->repeat_mapping && fsp->attractors == 2)
               found_attractor = 2;
            else if (fsp->repeat_mapping && fsp->attractors == 3)
               found_attractor = 3;
            else if (lold.x >= fsp->fl.l.right_x3) {
               if (!fsp->repeat_mapping)
                  found_attractor = 2;
               else
                  found_attractor = 4;
            }
            else if (lold.x <= fsp->fl.l.right_x4) {
               if (!fsp->repeat_mapping)
                  found_attractor = 3;
               else
                  found_attractor = 6;
             }
            }
         else if (labs(multiply(-FROTH_LSLOPE,lold.x,bitshift)-fsp->fl.l.a-lold.y) < FROTH_LCLOSE)
            {
            if (!fsp->repeat_mapping && fsp->attractors == 2)
               found_attractor = 2;
            else if (fsp->repeat_mapping && fsp->attractors == 3)
               found_attractor = 2;
            else if (lold.x >= fsp->fl.l.left_x3) {
               if (!fsp->repeat_mapping)
                  found_attractor = 3;
               else
                  found_attractor = 5;
            }
            else if (lold.x <= fsp->fl.l.left_x4) {
               if (!fsp->repeat_mapping)
                  found_attractor = 2;
               else
                  found_attractor = 3;
             }
            }
         }
      }
   if (show_orbit)
      scrub_orbit();

   realcoloriter = coloriter;
   if ((kbdcount -= abs((int)realcoloriter)) <= 0)
      {
      if (check_key())
         return (-1);
      kbdcount = max_kbdcount;
      }

/* inside - Here's where non-palette based images would be nice.  Instead, */
/* we'll use blocks of (colors-1)/3 or (colors-1)/6 and use special froth  */
/* color maps in attempt to replicate the images of James Alexander.       */
   if (found_attractor)
      {
      if (colors >= 256)
         {
         if (!fsp->altcolor)
            {
            if (coloriter > fsp->shades)
                coloriter = fsp->shades;
            }
         else
            coloriter = fsp->shades * coloriter / maxit;
         if (coloriter == 0)
            coloriter = 1;
         coloriter += fsp->shades * (found_attractor-1);
         }
      else if (colors >= 16)
         { /* only alternate coloring scheme available for 16 colors */
         long lshade;

/* Trying to make a better 16 color distribution. */
/* Since their are only a few possiblities, just handle each case. */
/* This is a mostly guess work here. */
         lshade = (coloriter<<16)/maxit;
         if (fsp->attractors != 6) /* either 2 or 3 attractors */
            {
            if (lshade < 2622)       /* 0.04 */
               coloriter = 1;
            else if (lshade < 10486) /* 0.16 */
               coloriter = 2;
            else if (lshade < 23593) /* 0.36 */
               coloriter = 3;
            else if (lshade < 41943L) /* 0.64 */
               coloriter = 4;
            else
               coloriter = 5;
            coloriter += 5 * (found_attractor-1);
            }
         else /* 6 attractors */
            {
            if (lshade < 10486)      /* 0.16 */
               coloriter = 1;
            else
               coloriter = 2;
            coloriter += 2 * (found_attractor-1);
            }
         }
      else /* use a color corresponding to the attractor */
         coloriter = found_attractor;
      oldcoloriter = coloriter;
      }
   else /* outside, or inside but didn't get sucked in by attractor. */
      coloriter = 0;

   color = abs((int)(coloriter));

   (*plot)(col, row, color);

   return color;
   }

/*
These last two froth functions are for the orbit-in-window feature.
Normally, this feature requires StandardFractal, but since it is the
attractor that makes the frothybasin type so unique, it is worth
putting in as a stand-alone.
*/

int froth_per_pixel(void)
   {
   if (!integerfractal) /* fp mode */
      {
      old.x = dxpixel();
      old.y = dypixel();
      tempsqrx=sqr(old.x);
      tempsqry=sqr(old.y);
      }
   else  /* integer mode */
      {
      lold.x = lxpixel();
      lold.y = lypixel();
      ltempsqrx = multiply(lold.x, lold.x, bitshift);
      ltempsqry = multiply(lold.y, lold.y, bitshift);
      }
   return 0;
   }

int froth_per_orbit(void)
   {
   if (!integerfractal) /* fp mode */
      {
      new.x = tempsqrx - tempsqry - old.x - fsp->fl.f.a*old.y;
      new.y = 2.0*old.x*old.y - fsp->fl.f.a*old.x + old.y;
      if (fsp->repeat_mapping)
        {
        old = new;
        new.x = sqr(old.x) - sqr(old.y) - old.x - fsp->fl.f.a*old.y;
        new.y = 2.0*old.x*old.y - fsp->fl.f.a*old.x + old.y;
        }

      if ((tempsqrx=sqr(new.x)) + (tempsqry=sqr(new.y)) >= rqlim)
         return 1;
      old = new;
      }
   else  /* integer mode */
      {
      lnew.x = ltempsqrx - ltempsqry - lold.x - multiply(fsp->fl.l.a,lold.y,bitshift);
      lnew.y = lold.y + (multiply(lold.x,lold.y,bitshift)<<1) - multiply(fsp->fl.l.a,lold.x,bitshift);
      if (fsp->repeat_mapping)
         {
         if ((ltempsqrx=lsqr(lnew.x)) + (ltempsqry=lsqr(lnew.y)) >= llimit)
            return 1;
         lold = lnew;
         lnew.x = ltempsqrx - ltempsqry - lold.x - multiply(fsp->fl.l.a,lold.y,bitshift);
         lnew.y = lold.y + (multiply(lold.x,lold.y,bitshift)<<1) - multiply(fsp->fl.l.a,lold.x,bitshift);
         }
      if ((ltempsqrx=lsqr(lnew.x)) + (ltempsqry=lsqr(lnew.y)) >= llimit)
         return 1;
      lold = lnew;
      }
   return 0;
   }

