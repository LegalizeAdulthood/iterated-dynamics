/*
        Various routines that prompt for things.
*/

#include <string.h>
#include <ctype.h>
#ifndef XFRACT
#include <io.h>
#elif !defined(__386BSD__)
#include <sys/types.h>
#include <sys/stat.h>
#ifndef __SVR4
# include <sys/dir.h>
#else
# include <dirent.h>
#ifndef DIRENT
#define DIRENT
#endif
#endif
#endif
#ifdef __TURBOC__
#include <alloc.h>
#elif !defined(__386BSD__)
#include <malloc.h>
#endif

#ifdef XFRACT
#include <fcntl.h>
#endif

#ifdef __hpux
#include <sys/param.h>
#endif

#ifdef __SVR4
#include <sys/param.h>
#endif

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

/* Routines in this module      */

static  int check_f6_key(int curkey,int choice);
static  int expand_dirname(char *dirname,char *drive);
static  int filename_speedstr(int, int, int, char *, int);
static  int get_screen_corners(void);

/* speed key state values */
#define MATCHING         0      /* string matches list - speed key mode */
#define TEMPLATE        -2      /* wild cards present - buiding template */
#define SEARCHPATH      -3      /* no match - building path search name */

#define   FILEATTR       0x37      /* File attributes; select all but volume labels */
#define   HIDDEN         2
#define   SYSTEM         4
#define   SUBDIR         16
#define   MAXNUMFILES    2977L

struct DIR_SEARCH DTA;          /* Allocate DTA and define structure */

#define GETFORMULA 0
#define GETLSYS    1
#define GETIFS     2
#define GETPARM    3

char commandmask[13] = {"*.par"};

/* --------------------------------------------------------------------- */
/*
        get_toggles() is called from FRACTINT.C whenever the 'x' key
        is pressed.  This routine prompts for several options,
        sets the appropriate variables, and returns the following code
        to the calling routine:

        -1  routine was ESCAPEd - no need to re-generate the image.
         0  nothing changed, or minor variable such as "overwrite=".
            No need to re-generate the image.
         1  major variable changed (such as "inside=").  Re-generate
            the image.

        Finally, remember to insert variables in the list *and* check
        for them in the same order!!!
*/
#define LOADCHOICES(X)     {\
   static FCODE tmp[] = { X };\
   far_strcpy(ptr,(char far *)tmp);\
   choices[++k]= ptr;\
   ptr += sizeof(tmp);\
   }
int get_toggles()
{
   static FCODE o_hdg[]={"Basic Options\n(not all combinations make sense)"};
   char hdg[sizeof(o_hdg)];
   char far *choices[20];
   char far *ptr;
   int oldhelpmode;
   char prevsavename[FILE_MAX_DIR+1];
   char *savenameptr;
   struct fullscreenvalues uvalues[25];
   int i, j, k;
   char old_usr_stdcalcmode;
   long old_maxit,old_logflag;
   int old_inside,old_outside,old_soundflag;
   int old_biomorph,old_decomp;
   int old_fillcolor;
   int old_stoppass;
   double old_closeprox;
   char *calcmodes[] ={"1","2","3","g","g1","g2","g3","g4","g5","g6","b","s","t","d","o"};
   char *soundmodes[5]={s_off,s_beep,s_x,s_y,s_z};
   char *insidemodes[]={"numb",s_maxiter,s_zmag,s_bof60,s_bof61,s_epscross,
                         s_startrail,s_period,s_atan,s_fmod};
   char *outsidemodes[]={"numb",s_iter,s_real,s_imag,s_mult,s_sum,s_atan,
                         s_fmod,s_tdis};

   far_strcpy(hdg,o_hdg);
   ptr = (char far *)MK_FP(extraseg,0);

   k = -1;

   LOADCHOICES("Passes (1,2,3, g[uess], b[ound], t[ess], d[iffu], o[rbit])");
   uvalues[k].type = 'l';
   uvalues[k].uval.ch.vlen = 3;
   uvalues[k].uval.ch.llen = sizeof(calcmodes)/sizeof(*calcmodes);
   uvalues[k].uval.ch.list = calcmodes;
   uvalues[k].uval.ch.val = (usr_stdcalcmode == '1') ? 0
                          : (usr_stdcalcmode == '2') ? 1
                          : (usr_stdcalcmode == '3') ? 2
                          : (usr_stdcalcmode == 'g' && stoppass == 0) ? 3
                          : (usr_stdcalcmode == 'g' && stoppass == 1) ? 4
                          : (usr_stdcalcmode == 'g' && stoppass == 2) ? 5
                          : (usr_stdcalcmode == 'g' && stoppass == 3) ? 6
                          : (usr_stdcalcmode == 'g' && stoppass == 4) ? 7
                          : (usr_stdcalcmode == 'g' && stoppass == 5) ? 8
                          : (usr_stdcalcmode == 'g' && stoppass == 6) ? 9
                          : (usr_stdcalcmode == 'b') ? 10 
			  : (usr_stdcalcmode == 's') ? 11
			  : (usr_stdcalcmode == 't') ? 12
			  : (usr_stdcalcmode == 'd') ? 13
                          :        /* "o"rbits */      14;
   old_usr_stdcalcmode = usr_stdcalcmode;
   old_stoppass = stoppass;
#ifndef XFRACT
   LOADCHOICES("Floating Point Algorithm");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = usr_floatflag;
#endif
   LOADCHOICES("Maximum Iterations (2 to 2,147,483,647)");
   uvalues[k].type = 'L';
   uvalues[k].uval.Lval = old_maxit = maxit;

   LOADCHOICES("Inside Color (0-# of colors, if Inside=numb)");
   uvalues[k].type = 'i';
   if (inside >= 0)
      uvalues[k].uval.ival = inside;
   else
      uvalues[k].uval.ival = 0;

   LOADCHOICES("Inside (numb,maxit,zmag,bof60,bof61,epscr,star,per,atan,fmod)");
   uvalues[k].type = 'l';
   uvalues[k].uval.ch.vlen = 12;
   uvalues[k].uval.ch.llen = sizeof(insidemodes)/sizeof(*insidemodes);
   uvalues[k].uval.ch.list = insidemodes;
   if(inside >= 0)  /* numb */
      uvalues[k].uval.ch.val = 0;
   else if(inside == -1)  /* maxiter */
      uvalues[k].uval.ch.val = 1;
   else if(inside == ZMAG)
      uvalues[k].uval.ch.val = 2;
   else if(inside == BOF60)
      uvalues[k].uval.ch.val = 3;
   else if(inside == BOF61)
      uvalues[k].uval.ch.val = 4;
   else if(inside == EPSCROSS)
      uvalues[k].uval.ch.val = 5;
   else if(inside == STARTRAIL)
      uvalues[k].uval.ch.val = 6;
   else if(inside == PERIOD)
      uvalues[k].uval.ch.val = 7;
   else if(inside == ATANI)
      uvalues[k].uval.ch.val = 8;
   else if(inside == FMODI)
      uvalues[k].uval.ch.val = 9;
   old_inside = inside;

   LOADCHOICES("Outside Color (0-# of colors, if Outside=numb)");
   uvalues[k].type = 'i';
   if (outside >= 0)
      uvalues[k].uval.ival = outside;
   else
      uvalues[k].uval.ival = 0;

   LOADCHOICES("Outside (numb,iter,real,imag,mult,summ,atan,fmod,tdis)");
   uvalues[k].type = 'l';
   uvalues[k].uval.ch.vlen = 4;
   uvalues[k].uval.ch.llen = sizeof(outsidemodes)/sizeof(*outsidemodes);
   uvalues[k].uval.ch.list = outsidemodes;
   if(outside >= 0)  /* numb */
      uvalues[k].uval.ch.val = 0;
   else
      uvalues[k].uval.ch.val = -outside;
   old_outside = outside;

   LOADCHOICES("Savename (.GIF implied)");
   uvalues[k].type = 's';
   strcpy(prevsavename,savename);
   savenameptr = strrchr(savename, SLASHC);
   if(savenameptr == NULL)
      savenameptr = savename;
   else
      savenameptr++; /* point past slash */
   strcpy(uvalues[k].uval.sval,savenameptr);

   LOADCHOICES("File Overwrite ('overwrite=')");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = overwrite;

   LOADCHOICES("Sound (off, beep, x, y, z)");
   uvalues[k].type = 'l';
   uvalues[k].uval.ch.vlen = 4;
   uvalues[k].uval.ch.llen = 5;
   uvalues[k].uval.ch.list = soundmodes;
   uvalues[k].uval.ch.val = (old_soundflag = soundflag)&7;

   if (rangeslen == 0) {
      LOADCHOICES("Log Palette (0=no,1=yes,-1=old,+n=cmprsd,-n=sqrt, 2=auto)");
      uvalues[k].type = 'L';
      }
   else {
      LOADCHOICES("Log Palette (n/a, ranges= parameter is in effect)");
      uvalues[k].type = '*';
      }
   uvalues[k].uval.Lval = old_logflag = LogFlag;

   LOADCHOICES("Biomorph Color (-1 means OFF)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_biomorph = usr_biomorph;

   LOADCHOICES("Decomp Option (2,4,8,..,256, 0=OFF)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_decomp = decomp[0];

   LOADCHOICES("Fill Color (normal,#) (works with passes=t, b and d)");
   uvalues[k].type = 's';
   if(fillcolor < 0)
      strcpy(uvalues[k].uval.sval,s_normal);
   else
      sprintf(uvalues[k].uval.sval,"%d",fillcolor);
   old_fillcolor = fillcolor;

   LOADCHOICES("Proximity value for inside=epscross and fmod");
   uvalues[k].type = 'f'; /* should be 'd', but prompts get messed up JCO */
   uvalues[k].uval.dval = old_closeprox = closeprox;

   oldhelpmode = helpmode;
   helpmode = HELPXOPTS;
   i = fullscreen_prompt(hdg,k+1,choices,uvalues,0,NULL);
   helpmode = oldhelpmode;
   if (i < 0) {
      return(-1);
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;
   j = 0;   /* return code */

   usr_stdcalcmode = calcmodes[uvalues[++k].uval.ch.val][0];
   stoppass = (int)calcmodes[uvalues[k].uval.ch.val][1] - (int)'0';

   if(stoppass < 0 || stoppass > 6 || usr_stdcalcmode != 'g')
      stoppass = 0;

   if(usr_stdcalcmode == 'o' && fractype == LYAPUNOV) /* Oops,lyapunov type */
                                       /* doesn't use 'new' & breaks orbits */
      usr_stdcalcmode = old_usr_stdcalcmode;

   if (old_usr_stdcalcmode != usr_stdcalcmode) j++;
   if (old_stoppass != stoppass) j++;
#ifndef XFRACT
   if (uvalues[++k].uval.ch.val != usr_floatflag) {
      usr_floatflag = (char)uvalues[k].uval.ch.val;
      j++;
      }
#endif
   ++k;
   maxit = uvalues[k].uval.Lval;
   if (maxit < 0) maxit = old_maxit;
   if (maxit < 2) maxit = 2;

   if (maxit != old_maxit) j++;

   inside = uvalues[++k].uval.ival;
   if (inside < 0) inside = -inside;
   if (inside >= colors) inside = (inside % colors) + (inside / colors);

   { int tmp;
   tmp = uvalues[++k].uval.ch.val;
   if (tmp > 0)
      switch (tmp)
      {
      case 1:
         inside = -1;  /* maxiter */
         break;
      case 2:
         inside = ZMAG;
         break;
      case 3:
         inside = BOF60;
         break;
      case 4:
         inside = BOF61;
         break;
      case 5:
         inside = EPSCROSS;
         break;
      case 6:
         inside = STARTRAIL;
         break;
      case 7:
         inside = PERIOD;
         break;
      case 8:
         inside = ATANI;
         break;
      case 9:
         inside = FMODI;
         break;
      }
   }
   if (inside != old_inside) j++;

   outside = uvalues[++k].uval.ival;
   if (outside < 0) outside = -outside;
   if (outside >= colors) outside = (outside % colors) + (outside / colors);

   { int tmp;
   tmp = uvalues[++k].uval.ch.val;
   if (tmp > 0)
      outside = -tmp;
   }
   if (outside != old_outside) j++;

   strcpy(savenameptr,uvalues[++k].uval.sval);
   if (strcmp(savename,prevsavename))
      resave_flag = started_resaves = 0; /* forget pending increment */
   overwrite = (char)uvalues[++k].uval.ch.val;

   soundflag = ((soundflag >> 3) << 3) | (uvalues[++k].uval.ch.val);
   if (soundflag != old_soundflag && ((soundflag&7) > 1 || (old_soundflag&7) > 1))
      j++;

   LogFlag = uvalues[++k].uval.Lval;
   if (LogFlag != old_logflag) {
      j++;
      Log_Auto_Calc = 0;  /* turn it off, use the supplied value */
   }

   usr_biomorph = uvalues[++k].uval.ival;
   if (usr_biomorph >= colors) usr_biomorph = (usr_biomorph % colors) + (usr_biomorph / colors);
   if (usr_biomorph != old_biomorph) j++;

   decomp[0] = uvalues[++k].uval.ival;
   if (decomp[0] != old_decomp) j++;

   if(strncmp(strlwr(uvalues[++k].uval.sval),s_normal,4)==0)
      fillcolor = -1;
   else
      fillcolor = atoi(uvalues[k].uval.sval);
   if (fillcolor < 0) fillcolor = -1;
   if (fillcolor >= colors) fillcolor = (fillcolor % colors) + (fillcolor / colors);
   if (fillcolor != old_fillcolor) j++;

   ++k;
   closeprox = uvalues[k].uval.dval;
   if (closeprox != old_closeprox) j++;

/* if (j >= 1) j = 1; need to know how many prompts changed for quick_calc JCO 6/23/2001 */

   return(j);
}

/*
        get_toggles2() is similar to get_toggles, invoked by 'y' key
*/

int get_toggles2()
{
   static FCODE o_hdg[]={"Extended Options\n\
(not all combinations make sense)"};
   char hdg[sizeof(o_hdg)];
   char far *ptr;
   char far *choices[18];
   int oldhelpmode;

   struct fullscreenvalues uvalues[23];
   int i, j, k;

   int old_rotate_lo,old_rotate_hi;
   int old_distestwidth;
   double old_potparam[3],old_inversion[3];
   long old_usr_distest;

   far_strcpy(hdg,o_hdg);
   ptr = (char far *)MK_FP(extraseg,0);

   /* fill up the choices (and previous values) arrays */
   k = -1;

   LOADCHOICES("Look for finite attractor (0=no,>0=yes,<0=phase)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ch.val = finattract;

   LOADCHOICES("Potential Max Color (0 means off)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = (int)(old_potparam[0] = potparam[0]);

   LOADCHOICES("          Slope");
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = old_potparam[1] = potparam[1];

   LOADCHOICES("          Bailout");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = (int)(old_potparam[2] = potparam[2]);

   LOADCHOICES("          16 bit values");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = pot16bit;

   LOADCHOICES("Distance Estimator (0=off, <0=edge, >0=on):");
   uvalues[k].type = 'L';
   uvalues[k].uval.Lval = old_usr_distest = usr_distest;

   LOADCHOICES("          width factor:");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_distestwidth = distestwidth;

   LOADCHOICES("Inversion radius or \"auto\" (0 means off)");
   LOADCHOICES("          center X coordinate or \"auto\"");
   LOADCHOICES("          center Y coordinate or \"auto\"");
   k = k - 3;
   for (i= 0; i < 3; i++) {
      uvalues[++k].type = 's';
      if ((old_inversion[i] = inversion[i]) == AUTOINVERT)
         sprintf(uvalues[k].uval.sval,"auto");
      else
         sprintf(uvalues[k].uval.sval,"%-1.15lg",inversion[i]);
      }
   LOADCHOICES("  (use fixed radius & center when zooming)");
   uvalues[k].type = '*';

   LOADCHOICES("Color cycling from color (0 ... 254)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_rotate_lo = rotate_lo;

   LOADCHOICES("              to   color (1 ... 255)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_rotate_hi = rotate_hi;

   oldhelpmode = helpmode;
   helpmode = HELPYOPTS;
   i = fullscreen_prompt(hdg,k+1,choices,uvalues,0,NULL);
   helpmode = oldhelpmode;
   if (i < 0) {
      return(-1);
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;
   j = 0;   /* return code */

   if (uvalues[++k].uval.ch.val != finattract) {
      finattract = uvalues[k].uval.ch.val;
      j = 1;
      }

   potparam[0] = uvalues[++k].uval.ival;
   if (potparam[0] != old_potparam[0]) j = 1;

   potparam[1] = uvalues[++k].uval.dval;
   if (potparam[0] != 0.0 && potparam[1] != old_potparam[1]) j = 1;

   potparam[2] = uvalues[++k].uval.ival;
   if (potparam[0] != 0.0 && potparam[2] != old_potparam[2]) j = 1;

   if (uvalues[++k].uval.ch.val != pot16bit) {
      pot16bit = uvalues[k].uval.ch.val;
      if (pot16bit) { /* turned it on */
         if (potparam[0] != 0.0) j = 1;
         }
      else /* turned it off */
         if (dotmode != 11) /* ditch the disk video */
            enddisk();
         else /* keep disk video, but ditch the fraction part at end */
            disk16bit = 0;
      }

   ++k;
/* usr_distest = (uvalues[k].uval.ival > 32000) ? 32000 : uvalues[k].uval.ival; */
   usr_distest = uvalues[k].uval.Lval;
   if (usr_distest != old_usr_distest) j = 1;
   ++k;
   distestwidth = uvalues[k].uval.ival;
   if (usr_distest && distestwidth != old_distestwidth) j = 1;

   for (i = 0; i < 3; i++) {
      if (uvalues[++k].uval.sval[0] == 'a' || uvalues[k].uval.sval[0] == 'A')
         inversion[i] = AUTOINVERT;
      else
         inversion[i] = atof(uvalues[k].uval.sval);
      if (old_inversion[i] != inversion[i]
        && (i == 0 || inversion[0] != 0.0))
         j = 1;
      }
   invert = (inversion[0] == 0.0) ? 0 : 3;
   ++k;

   rotate_lo = uvalues[++k].uval.ival;
   rotate_hi = uvalues[++k].uval.ival;
   if (rotate_lo < 0 || rotate_hi > 255 || rotate_lo > rotate_hi) {
      rotate_lo = old_rotate_lo;
      rotate_hi = old_rotate_hi;
      }

   return(j);
}


/*
     passes_options invoked by <p> key
*/

int passes_options(void)
{
   static FCODE o_hdg[]={"Passes Options\n\
(not all combinations make sense)"};
   static FCODE pressf2[] = {"\n(Press "FK_F2" for corner parameters)"};
   static FCODE pressf6[] = {"\n(Press "FK_F6" for calculation parameters)"};
   char hdg[sizeof(o_hdg)+sizeof(pressf2)+sizeof(pressf6)];
   char far *ptr;
   char far *choices[20];
   int oldhelpmode;
   char *passcalcmodes[] ={"rect","line","func"};

   struct fullscreenvalues uvalues[25];
   int i, j, k;
   int ret;

   int old_periodicity, old_orbit_delay, old_orbit_interval;
   int old_keep_scrn_coords;
   char old_drawmode;

   far_strcpy(hdg,o_hdg);
   far_strcat(hdg,pressf2);
   far_strcat(hdg,pressf6);
   ptr = (char far *)MK_FP(extraseg,0);
   ret = 0;

pass_option_restart:
   /* fill up the choices (and previous values) arrays */
   k = -1;

   LOADCHOICES("Periodicity (0=off, <0=show, >0=on, -255..+255)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_periodicity = usr_periodicitycheck;

   LOADCHOICES("Orbit delay (0 = none)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_orbit_delay = orbit_delay;

   LOADCHOICES("Orbit interval (1 ... 255)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = old_orbit_interval = (int)orbit_interval;

   LOADCHOICES("Maintain screen coordinates");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = old_keep_scrn_coords = keep_scrn_coords;

   LOADCHOICES("Orbit pass shape (rect,line,func)");
   uvalues[k].type = 'l';
   uvalues[k].uval.ch.vlen = 5;
   uvalues[k].uval.ch.llen = sizeof(passcalcmodes)/sizeof(*passcalcmodes);
   uvalues[k].uval.ch.list = passcalcmodes;
   uvalues[k].uval.ch.val = (drawmode == 'r') ? 0
                          : (drawmode == 'l') ? 1
			  :   /* function */    2;
   old_drawmode = drawmode;

   oldhelpmode = helpmode;
   helpmode = HELPPOPTS;
   i = fullscreen_prompt(hdg,k+1,choices,uvalues,0x44,NULL);
   helpmode = oldhelpmode;
   if (i < 0) {
      return(-1);
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;
   j = 0;   /* return code */

   usr_periodicitycheck = uvalues[++k].uval.ival;
   if (usr_periodicitycheck > 255) usr_periodicitycheck = 255;
   if (usr_periodicitycheck < -255) usr_periodicitycheck = -255;
   if (usr_periodicitycheck != old_periodicity) j = 1;


   orbit_delay = uvalues[++k].uval.ival;
   if (orbit_delay != old_orbit_delay) j = 1;


   orbit_interval = uvalues[++k].uval.ival;
   if (orbit_interval > 255) orbit_interval = 255;
   if (orbit_interval < 1) orbit_interval = 1;
   if (orbit_interval != old_orbit_interval) j = 1;

   keep_scrn_coords = uvalues[++k].uval.ch.val;
   if (keep_scrn_coords != old_keep_scrn_coords) j = 1;
   if (keep_scrn_coords == 0) set_orbit_corners = 0;

   { int tmp;
   tmp = uvalues[++k].uval.ch.val;
      switch (tmp)
      {
      default:
      case 0:
         drawmode = 'r';
         break;
      case 1:
         drawmode = 'l';
         break;
      case 2:
         drawmode = 'f';
         break;
      }
   }
   if (drawmode != old_drawmode) j = 1;

   if (i == F2) {
      if (get_screen_corners() > 0) {
         ret = 1;
      }
      goto pass_option_restart;
   }

   if (i == F6) {
      if (get_corners() > 0) {
         ret = 1;
      }
      goto pass_option_restart;
   }

   return(j + ret);
}


/* for videomodes added new options "virtual x/y" that change "sx/ydots" */
/* for diskmode changed "viewx/ydots" to "virtual x/y" that do as above  */
/* (since for diskmode they were updated by x/ydots that should be the   */
/* same as sx/ydots for that mode)                                       */
/* videotable and videoentry are now updated even for non-disk modes     */

/* --------------------------------------------------------------------- */
/*
    get_view_params() is called from FRACTINT.C whenever the 'v' key
    is pressed.  Return codes are:
        -1  routine was ESCAPEd - no need to re-generate the image.
         0  minor variable changed.  No need to re-generate the image.
         1  View changed.  Re-generate the image.
*/

int get_view_params()
{
   static FCODE o_hdg[]={"View Window Options"};
   char hdg[sizeof(o_hdg)];
   char far *choices[16];
   char far *ptr;

   int oldhelpmode;
   struct fullscreenvalues uvalues[25];
   int i, k;
   float old_viewreduction,old_aspectratio;
   int old_viewwindow,old_viewxdots,old_viewydots,old_sxdots,old_sydots;
   unsigned long estm_xmax=32767,estm_ymax=32767;
#ifndef XFRACT
   unsigned long vidmem = (unsigned long)video_vram << 16;
   int truebytes = videoentry.dotmode/1000;

   if (dotmode == 28)          /* setvideo might have changed mode 27 to 28 */
      dotmode = videoentry.dotmode%100;
#endif

   far_strcpy(hdg,o_hdg);
   ptr = (char far *)MK_FP(extraseg,0);

/*
   Because the scrolling (and virtual screen width) must be
   aligned to 8 bytes, that gives:

   8 pixels for 8 bit (truebytes == 0; ++truebytes == 1;)
   4 pixels for 15 bit (truebytes == 1; ++truebytes == 2;)
   4 pixels for 16 bit (truebytes == 2;)
   8 pixels for 24 bit (truebytes == 3;)
   2 pixels for 32 bit (truebytes == 4;)

   so for odd truebytes (8 and 24 bit) it does &= ..FFF8 (&= -8)
   if truebytes==2 (15 and 16 bit) it does &= ..FFFC (&= -4)
   if truebytes==4 (32 bit) it does &= ..FFFE (&= -2)
*/

#ifndef XFRACT
   if (dotmode == 28 && virtual) {
      /* virtual screen limits estimation */
      if (truebytes < 2)
         ++truebytes;
      vidmem /= truebytes;
      estm_xmax = vesa_yres ? vidmem/vesa_yres : 0;
      estm_ymax = vesa_xres ? vidmem/vesa_xres : 0;
      estm_xmax &= truebytes&1 ? -8 : truebytes - 6;
   }
#endif

   old_viewwindow    = viewwindow;
   old_viewreduction = viewreduction;
   old_aspectratio   = finalaspectratio;
   old_viewxdots     = viewxdots;
   old_viewydots     = viewydots;
   old_sxdots        = sxdots;
   old_sydots        = sydots;

get_view_restart:
   /* fill up the previous values arrays */
   k = -1;

   if (dotmode != 11) {
      LOADCHOICES("Preview display? (no for full screen)");
      uvalues[k].type = 'y';
      uvalues[k].uval.ch.val = viewwindow;

      LOADCHOICES("Auto window size reduction factor");
      uvalues[k].type = 'f';
      uvalues[k].uval.dval = viewreduction;

      LOADCHOICES("Final media overall aspect ratio, y/x");
      uvalues[k].type = 'f';
      uvalues[k].uval.dval = finalaspectratio;

      LOADCHOICES("Crop starting coordinates to new aspect ratio?");
      uvalues[k].type = 'y';
      uvalues[k].uval.ch.val = viewcrop;

      LOADCHOICES("Explicit size x pixels (0 for auto size)");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = viewxdots;

      LOADCHOICES("              y pixels (0 to base on aspect ratio)");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = viewydots;
   }

   LOADCHOICES("");
   uvalues[k].type = '*';

#ifndef XFRACT
   if (virtual && dotmode == 28 && chkd_vvs && !video_scroll) {
      LOADCHOICES("Your graphics card does NOT support virtual screens.");
      uvalues[k].type = '*';
   }
#endif

   if (dotmode == 11 || (virtual && dotmode == 28)) {
      LOADCHOICES("Virtual screen total x pixels");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = sxdots;

      if (dotmode == 11) {
         LOADCHOICES("                     y pixels");
      }
      else {
         LOADCHOICES("                     y pixels (0: by aspect ratio)");
      }
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = sydots;
   }

#ifndef XFRACT
   if (virtual && dotmode == 28) {
      char dim[50];
      static FCODE xmsg[] = {"Video memory limits: (for y = "};
      static FCODE ymsg[] = {"                     (for x = "};
      static FCODE midxmsg[] = {") x <= "};
      static FCODE midymsg[] = {") y <= "};
      char *scrolltypes[] ={"fixed","relaxed"};

      LOADCHOICES("Keep aspect? (cuts both x & y when either too big)");
      uvalues[k].type = 'y';
      uvalues[k].uval.ch.val = video_cutboth;

      LOADCHOICES("Zoombox scrolling (f[ixed], r[elaxed])");
      uvalues[k].type = 'l';
      uvalues[k].uval.ch.vlen = 7;
      uvalues[k].uval.ch.llen = sizeof(scrolltypes)/sizeof(*scrolltypes);
      uvalues[k].uval.ch.list = scrolltypes;
      uvalues[k].uval.ch.val = zscroll;

      LOADCHOICES("");
      uvalues[k].type = '*';

      sprintf(dim,"%Fs%4u%Fs%lu",(char far *)xmsg,vesa_yres,(char far *)midxmsg,estm_xmax);
      far_strcpy(ptr,(char far *)dim);
      choices[++k]= ptr;
      ptr += sizeof(dim);
      uvalues[k].type = '*';

      sprintf(dim,"%Fs%4u%Fs%lu",(char far *)ymsg,vesa_xres,(char far *)midymsg,estm_ymax);
      far_strcpy(ptr,(char far *)dim);
      choices[++k]= ptr;
      ptr += sizeof(dim);
      uvalues[k].type = '*';

      LOADCHOICES("");
      uvalues[k].type = '*';
   }
#endif

   if (dotmode != 11) {
      LOADCHOICES("Press "FK_F4" to reset view parameters to defaults.");
      uvalues[k].type = '*';
   }

   oldhelpmode = helpmode;     /* this prevents HELP from activating */
   helpmode = HELPVIEW;
   i = fullscreen_prompt(hdg,k+1,choices,uvalues,16,NULL);
   helpmode = oldhelpmode;     /* re-enable HELP */
   if (i < 0) {
      return(-1);
      }

   if (i == F4 && dotmode != 11) {
      viewwindow = viewxdots = viewydots = 0;
      viewreduction = (float)4.2;
      viewcrop = 1;
      finalaspectratio = screenaspect;
      if (dotmode == 28) {
         sxdots = vesa_xres ? vesa_xres : old_sxdots;
         sydots = vesa_yres ? vesa_yres : old_sydots;
         video_cutboth = 1;
         zscroll = 1;
      }
      goto get_view_restart;
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;

   if (dotmode != 11) {
      viewwindow = uvalues[++k].uval.ch.val;

      viewreduction = (float)uvalues[++k].uval.dval;

      finalaspectratio = (float)uvalues[++k].uval.dval;

      viewcrop = uvalues[++k].uval.ch.val;

      viewxdots = uvalues[++k].uval.ival;
      viewydots = uvalues[++k].uval.ival;
   }

   ++k;

   if (virtual && dotmode == 28 && chkd_vvs && !video_scroll)
      ++k;  /* add 1 if not supported line is inserted */

   if (dotmode == 11 || (virtual && dotmode == 28)) {
      sxdots = uvalues[++k].uval.ival;
      sydots = uvalues[++k].uval.ival;
#ifndef XFRACT
      video_cutboth = uvalues[++k].uval.ch.val;
      zscroll = uvalues[++k].uval.ch.val;
#endif

      if ((unsigned long)sxdots > estm_xmax)
         sxdots = (int)estm_xmax;
      if (sxdots < 2)
         sxdots = 2;
      if (sydots == 0 && dotmode == 28) { /* auto by aspect ratio request */
         if (finalaspectratio == 0.0) {
            if (viewwindow && viewxdots != 0 && viewydots != 0)
               finalaspectratio = (float)viewydots/viewxdots;
            else
               finalaspectratio = old_aspectratio;
         }
         sydots = (int)(finalaspectratio * sxdots + 0.5);
      }
      if ((unsigned long)sydots > estm_ymax)
         sydots = (int)estm_ymax;
      if (sydots < 2)
         sydots = 2;
   }

#ifndef XFRACT
   if (virtual && dotmode == 28) {

      /* virtual screen smaller than physical screen, use view window */
      if (sxdots < vesa_xres && sydots < vesa_yres) {
         viewwindow = 1;
         viewxdots = sxdots;
         viewydots = sydots;
         sxdots    = vesa_xres;
         sydots    = vesa_yres;
      } else {
         viewwindow = 0; /* make sure it is off */
         viewxdots = 0;
         viewydots = 0;
      }

      /* if virtual screen is too large */
      if( (unsigned long)((sxdots < vesa_xres) ? vesa_xres : sxdots)
            * ((sydots < vesa_yres) ? vesa_yres : sydots) > vidmem) {
        /* and we have to keep ratio */
        if (video_cutboth) {
           double tmp,virtaspect = (double)sydots / sxdots;

           /* we need vidmem >= x * y == x * x * virtaspect */
           tmp = sqrt(vidmem / virtaspect);
           sxdots = (tmp > (double)estm_xmax) ? (int)estm_xmax : (int)tmp;
           sxdots &= truebytes&1 ? -8 : truebytes - 6;
           if (sxdots < 2)
               sxdots = 2;
           tmp = sxdots * virtaspect;
           sydots = (tmp > (double)estm_ymax) ? (int)estm_ymax : (int)tmp;
           /* if sydots < 2 here, then sxdots > estm_xmax */
        }
        else { /* cut only the y value */
           sydots = (int)((double)vidmem / ((sxdots < vesa_xres) ? vesa_xres : sxdots));
        }
      }
   }
#endif

   if (dotmode == 11 || (virtual && dotmode == 28)) {
      videoentry.xdots = sxdots;
      videoentry.ydots = sydots;
      far_memcpy((char far *)&videotable[adapter],(char far *)&videoentry,
                    sizeof(videoentry));
      if (finalaspectratio == 0.0)
         finalaspectratio = (float)sydots/sxdots;
   }

   if (viewxdots != 0 && viewydots != 0 && viewwindow && finalaspectratio == 0.0)
      finalaspectratio = (float)viewydots/viewxdots;
   else if (finalaspectratio == 0.0 && (viewxdots == 0 || viewydots == 0))
      finalaspectratio = old_aspectratio;

   if (finalaspectratio != old_aspectratio && viewcrop)
      aspectratio_crop(old_aspectratio,finalaspectratio);

   i = 0;
   if (viewwindow != old_viewwindow
      || sxdots != old_sxdots || sydots != old_sydots
      || (viewwindow
         && (  viewreduction != old_viewreduction
            || finalaspectratio != old_aspectratio
            || viewxdots != old_viewxdots
            || (viewydots != old_viewydots && viewxdots) ) ) )
      i = 1;

   return(i);
}

/*
    get_cmd_string() is called from FRACTINT.C whenever the 'g' key
    is pressed.  Return codes are:
        -1  routine was ESCAPEd - no need to re-generate the image.
         0  parameter changed, no need to regenerate
        >0  parameter changed, regenerate
*/

int get_cmd_string()
{
   static FCODE o_msg[] = {"Enter command string to use."};
   char msg[sizeof(o_msg)];
   int oldhelpmode;
   int i;
   static char cmdbuf[61];

   far_strcpy(msg,o_msg);
   oldhelpmode = helpmode;
   helpmode = HELPCOMMANDS;
   i = field_prompt(0,msg,NULL,cmdbuf,60,NULL);
   helpmode = oldhelpmode;
   if (i >= 0 && cmdbuf[0] != 0) {
       i = cmdarg(cmdbuf, 2);
       if(debugflag == 98)
       {
          backwards_v18();
          backwards_v19();
          backwards_v20();
       }
   }

   return(i);
}


/* --------------------------------------------------------------------- */

int Distribution = 30, Offset = 0, Slope = 25;
long con;


       double starfield_values[4] = {
        30.0,100.0,5.0,0.0
        };

char GreyFile[] = "altern.map";

int starfield(void)
{
   int c;
   busy = 1;
   if (starfield_values[0] <   1.0) starfield_values[0] =   1.0;
   if (starfield_values[0] > 100.0) starfield_values[0] = 100.0;
   if (starfield_values[1] <   1.0) starfield_values[1] =   1.0;
   if (starfield_values[1] > 100.0) starfield_values[1] = 100.0;
   if (starfield_values[2] <   1.0) starfield_values[2] =   1.0;
   if (starfield_values[2] > 100.0) starfield_values[2] = 100.0;

   Distribution = (int)(starfield_values[0]);
   con  = (long)(((starfield_values[1]) / 100.0) * (1L << 16));
   Slope = (int)(starfield_values[2]);

   if (ValidateLuts(GreyFile) != 0) {
      static FCODE msg[]={"Unable to load ALTERN.MAP"};
      stopmsg(0,msg);
      busy = 0;
      return(-1);
      }
   spindac(0,1);                 /* load it, but don't spin */
   for(row = 0; row < ydots; row++) {
      for(col = 0; col < xdots; col++) {
         if(keypressed()) {
            buzzer(1);
            busy = 0;
            return(1);
            }
         c = getcolor(col, row);
         if(c == inside)
            c = colors-1;
         putcolor(col, row, GausianNumber(c, colors));
      }
   }
   buzzer(0);
   busy = 0;
   return(0);
}

int get_starfield_params(void) {
   static FCODE o_hdg[]={"Starfield Parameters"};
   static FCODE o_sf1[] = {"Star Density in Pixels per Star"};
   static FCODE o_sf2[] = {"Percent Clumpiness"};
   static FCODE o_sf3[] = {"Ratio of Dim stars to Bright"};
   char hdg[sizeof(o_hdg)];
   char sf1[sizeof(o_sf1)];
   char sf2[sizeof(o_sf2)];
   char sf3[sizeof(o_sf3)];
   struct fullscreenvalues uvalues[3];
   int oldhelpmode;
   int i;
   char far *starfield_prompts[3];
   far_strcpy(hdg,o_hdg);
   far_strcpy(sf1,o_sf1);
   far_strcpy(sf2,o_sf2);
   far_strcpy(sf3,o_sf3);
   starfield_prompts[0] = sf1;
   starfield_prompts[1] = sf2;
   starfield_prompts[2] = sf3;

   if(colors < 255) {
      static FCODE msg[]={"starfield requires 256 color mode"};
      stopmsg(0,msg);
      return(-1);
   }
   for (i = 0; i < 3; i++) {
      uvalues[i].uval.dval = starfield_values[i];
      uvalues[i].type = 'f';
   }
   stackscreen();
   oldhelpmode = helpmode;
   helpmode = HELPSTARFLD;
   i = fullscreen_prompt(hdg,3,starfield_prompts,uvalues,0,NULL);
   helpmode = oldhelpmode;
   unstackscreen();
   if (i < 0) {
      return(-1);
      }
   for (i = 0; i < 3; i++)
      starfield_values[i] = uvalues[i].uval.dval;

   return(0);
}

static char *masks[] = {"*.pot","*.gif"};

int get_rds_params(void) {
   static FCODE o_hdg[] =  {"Random Dot Stereogram Parameters"};
   static FCODE o_rds0[] = {"Depth Effect (negative reverses front and back)"};
   static FCODE o_rds1[] = {"Image width in inches"};
   static FCODE o_rds2[] = {"Use grayscale value for depth? (if \"no\" uses color number)"};
   static FCODE o_rds3[] = {"Calibration bars"};
   static FCODE o_rds4[] = {"Use image map? (if \"no\" uses random dots)"};
   static FCODE o_rds5[] = {"  If yes, use current image map name? (see below)"};

   char hdg[sizeof(o_hdg)];
   char rds0[sizeof(o_rds0)];
   char rds1[sizeof(o_rds1)];
   char rds2[sizeof(o_rds2)];
   char rds3[sizeof(o_rds3)];
   char rds4[sizeof(o_rds4)];
   char rds5[sizeof(o_rds5)];
   char rds6[60];
   char *stereobars[] = {"none", "middle", "top"};
   struct fullscreenvalues uvalues[7];
   char far *rds_prompts[7];
   int oldhelpmode;
   int i,k;
   int ret;
   static char reuse = 0;
   stackscreen();
   for(;;)
   {
      ret = 0;
      /* copy to make safe from overlay change */
      far_strcpy(hdg,o_hdg);
      far_strcpy(rds0,o_rds0);
      far_strcpy(rds1,o_rds1);
      far_strcpy(rds2,o_rds2);
      far_strcpy(rds3,o_rds3);
      far_strcpy(rds4,o_rds4);
      far_strcpy(rds5,o_rds5);
      rds_prompts[0] = rds0;
      rds_prompts[1] = rds1;
      rds_prompts[2] = rds2;
      rds_prompts[3] = rds3;
      rds_prompts[4] = rds4;
      rds_prompts[5] = rds5;
      rds_prompts[6] = rds6;

      k=0;
      uvalues[k].uval.ival = AutoStereo_depth;
      uvalues[k++].type = 'i';

      uvalues[k].uval.dval = AutoStereo_width;
      uvalues[k++].type = 'f';

      uvalues[k].uval.ch.val = grayflag;
      uvalues[k++].type = 'y';

      uvalues[k].type = 'l';
      uvalues[k].uval.ch.list = stereobars;
      uvalues[k].uval.ch.vlen = 6;
      uvalues[k].uval.ch.llen = 3;
      uvalues[k++].uval.ch.val  = calibrate;

      uvalues[k].uval.ch.val = image_map;
      uvalues[k++].type = 'y';


      if(*stereomapname != 0 && image_map)
      {
         char *p;
         uvalues[k].uval.ch.val = reuse;
         uvalues[k++].type = 'y';

         uvalues[k++].type = '*';
         for(i=0;i<sizeof(rds6);i++)
            rds6[i] = ' ';
         if((p = strrchr(stereomapname,SLASHC))==NULL ||
                 strlen(stereomapname) < sizeof(rds6)-2)
            p = strlwr(stereomapname);
         else
            p++;
         /* center file name */
         rds6[(sizeof(rds6)-strlen(p)+2)/2] = 0;
         strcat(rds6,"[");
         strcat(rds6,p);
         strcat(rds6,"]");
      }
      else
         *stereomapname = 0;
      oldhelpmode = helpmode;
      helpmode = HELPRDS;
      i = fullscreen_prompt(hdg,k,rds_prompts,uvalues,0,NULL);
      helpmode = oldhelpmode;
      if (i < 0) {
         ret = -1;
         break;
         }
      else
      {
         k=0;
         AutoStereo_depth = uvalues[k++].uval.ival;
         AutoStereo_width = uvalues[k++].uval.dval;
         grayflag         = (char)uvalues[k++].uval.ch.val;
         calibrate        = (char)uvalues[k++].uval.ch.val;
         image_map        = (char)uvalues[k++].uval.ch.val;
         if(*stereomapname && image_map)
            reuse         = (char)uvalues[k++].uval.ch.val;
         else
            reuse = 0;
         if(image_map && !reuse)
         {
            static FCODE tmp[] = {"Select an Imagemap File"};
            char tmp1[sizeof(tmp)];
            /* tmp1 only a convenient buffer */
            far_strcpy(tmp1,tmp);
            if(getafilename(tmp1,masks[1],stereomapname))
               continue;
         }
      }
      break;
   }
   unstackscreen();
   return(ret);
}

int get_a_number(double *x, double *y)
{
   static FCODE o_hdg[]={"Set Cursor Coordinates"};
   char hdg[sizeof(o_hdg)];
   char far *ptr;
   char far *choices[2];

   struct fullscreenvalues uvalues[2];
   int i, k;

   stackscreen();
   far_strcpy(hdg,o_hdg);
   ptr = (char far *)MK_FP(extraseg,0);

   /* fill up the previous values arrays */
   k = -1;

   LOADCHOICES("X coordinate at cursor");
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = *x;

   LOADCHOICES("Y coordinate at cursor");
   uvalues[k].type = 'd';
   uvalues[k].uval.dval = *y;

   i = fullscreen_prompt(hdg,k+1,choices,uvalues,25,NULL);
   if (i < 0) {
      unstackscreen();
      return(-1);
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;

   *x = uvalues[++k].uval.dval;
   *y = uvalues[++k].uval.dval;

   unstackscreen();
   return(i);
}

/* --------------------------------------------------------------------- */

int get_commands()              /* execute commands from file */
{
   int ret;
   FILE *parmfile;
   long point;
   int oldhelpmode;
   ret = 0;
   oldhelpmode = helpmode;
   helpmode = HELPPARMFILE;
   if ((point = get_file_entry(GETPARM,"Parameter Set",
                               commandmask,CommandFile,CommandName)) >= 0
     && (parmfile = fopen(CommandFile,"rb")) != NULL) {
      fseek(parmfile,point,SEEK_SET);
      ret = load_commands(parmfile);
      }
   helpmode = oldhelpmode;
   return(ret);
}

/* --------------------------------------------------------------------- */

void goodbye()                  /* we done.  Bail out */
{
   char goodbyemessage[40];
   int ret;
   static FCODE gbm[]={"   Thank You for using "FRACTINT};
#ifndef XFRACT
   union REGS r;
#endif
   if (resume_info != 0)
      end_resume();
   if (evolve_handle != 0)
      MemoryRelease(evolve_handle);
   if (gene_handle != 0)
      MemoryRelease(gene_handle);
   if (imgboxhandle != 0 || prmboxhandle != 0)
      ReleaseParamBox();
   if (history != 0)
      MemoryRelease(history);
   if (oldhistory_handle != 0)
      MemoryRelease(oldhistory_handle);
   enddisk();
   discardgraphics();
   ExitCheck();
   far_strcpy(goodbyemessage, gbm);
#ifdef WINFRACT
   return;
#endif
   if(*s_makepar != 0)
      setvideotext();
#ifdef XFRACT
   UnixDone();
   printf("\n\n\n%s\n",goodbyemessage); /* printf takes far pointer */
#else
   if(*s_makepar != 0)
   {
      r.h.al = (char)((mode7text == 0) ? exitmode : 7);
      r.h.ah = 0;
      int86(0x10, &r, &r);
      printf("\n\n\n%s\n",goodbyemessage); /* printf takes far pointer */
   }
#endif
   if(*s_makepar != 0)
   {
      movecursor(6,0);
      discardgraphics(); /* if any emm/xmm tied up there, release it */
   }
   stopslideshow();
   end_help();
   ret = 0;
   if (initbatch == 3) /* exit with error code for batch file */
     ret = 2;
   else if (initbatch == 4)
     ret = 1;
   exit(ret);
}


/* --------------------------------------------------------------------- */

#ifdef XFRACT
static char searchdir[FILE_MAX_DIR];
static char searchname[FILE_MAX_PATH];
static char searchext[FILE_MAX_EXT];
static DIR *currdir = NULL;
#endif
int  fr_findfirst(char *path)       /* Find 1st file (or subdir) meeting path/filespec */
{
#ifndef XFRACT
     union REGS regs;
     regs.h.ah = 0x1A;             /* Set DTA to filedata */
     regs.x.dx = (unsigned)&DTA;
     intdos(&regs, &regs);
     regs.h.ah = 0x4E;             /* Find 1st file meeting path */
     regs.x.dx = (unsigned)path;
     regs.x.cx = FILEATTR;
     intdos(&regs, &regs);
     return(regs.x.ax);            /* Return error code */
#else
     if (currdir != NULL) {
         closedir(currdir);
         currdir = NULL;
     }
     splitpath(path,NULL,searchdir,searchname,searchext);
     if (searchdir[0]=='\0') {
         currdir = opendir(".");
     } else {
         currdir = opendir(searchdir);
     }
     if (currdir==NULL) {
         return -1;
     } else {
         return fr_findnext();
     }
#endif
}

int  fr_findnext()              /* Find next file (or subdir) meeting above path/filespec */
{
#ifndef XFRACT
     union REGS regs;
     regs.h.ah = 0x4F;             /* Find next file meeting path */
     regs.x.dx = (unsigned)&DTA;
     intdos(&regs, &regs);
     return(regs.x.ax);
#else
#ifdef DIRENT
     struct dirent *dirEntry;
#else
     struct direct *dirEntry;
#endif
     struct stat sbuf;
     char thisname[FILE_MAX_PATH];
     char tmpname[FILE_MAX_PATH];
     char thisext[FILE_MAX_EXT];
     for(;;) {
         dirEntry = readdir(currdir);
         if (dirEntry == NULL) {
             closedir(currdir);
             currdir = NULL;
             return -1;
         } else if (dirEntry->d_ino != 0) {
             splitpath(dirEntry->d_name,NULL,NULL,thisname,thisext);
             strncpy(DTA.filename,dirEntry->d_name,13);
             DTA.filename[12]='\0';
             strcpy(tmpname,searchdir);
             strcat(tmpname,dirEntry->d_name);
             stat(tmpname,&sbuf);
             DTA.size = sbuf.st_size;
             if ((sbuf.st_mode&S_IFMT)==S_IFREG &&
                 (searchname[0]=='*' || strcmp(searchname,thisname)==0) &&
                 (searchext[0]=='*' || strcmp(searchext,thisext)==0)) {
                 DTA.attribute = 0;
                 return 0;
             }
	     else if (((sbuf.st_mode&S_IFMT)==S_IFDIR) &&
                 ((searchname[0]=='*' || searchext[0]=='*') ||
                 (strcmp(searchname,thisname)==0))) {
                 DTA.attribute = SUBDIR;
                 return 0;
             }
         }
     }
#endif
}


#if 0
void heap_sort(void far *ra1, int n, unsigned sz, int (__cdecl *fct)(VOIDFARPTR arg1,VOIDFARPTR arg2))
{
   int ll,j,ir,i;
   void far *rra;
   char far *ra;
   ra = (char far *)ra1;
   ra -= sz;
   ll=(n>>1)+1;
   ir=n;

   for(;;)
   {
      if(ll>1)
         rra = *((char far *far *)(ra+(--ll)*sz));
      else
      {
         rra = *((char far * far *)(ra+ir*sz));
         *((char far * far *)(ra+ir*sz))=*((char far * far *)(ra+sz));
         if(--ir == 1)
         {
            *((char far * far *)(ra+sz))=rra;
            return;
         }
      }
      i = ll;
      j = ll <<1;
      while (j <= ir)
      {
         if(j<ir && (fct(ra+j*sz,ra+(j+1)*sz) < 0))
            ++j;
         if(fct(&rra,ra+j*sz) < 0)
         {
            *((char far * far *)(ra+i*sz)) = *((char far * far *)(ra+j*sz));
            j += (i=j);
         }
         else
            j=ir+1;
      }
      *((char far * far *)(ra+i*sz))=rra;
   }
}
#endif

int lccompare(VOIDFARPTR arg1, VOIDFARPTR arg2) /* for sort */
{
   return(strncasecmp(*((char far * far *)arg1),*((char far *far *)arg2),40));
}


static int speedstate;
int getafilename(char *hdg,char *template,char *flname)
{
   int rds;  /* if getting an RDS image map */
   static FCODE o_instr[]={"Press "FK_F6" for default directory, "FK_F4" to toggle sort "};
   char far *instr;
   int masklen;
   char filename[FILE_MAX_PATH]; /* 13 is big enough for Fractint, but not Xfractint */
   char speedstr[81];
   char tmpmask[FILE_MAX_PATH];   /* used to locate next file in list */
   char old_flname[FILE_MAX_PATH];
   static int numtemplates = 1;
   int i,j;
   int out;
   int retried;
   static struct CHOICE
   {
      char name[13];
      char type;
   }
   far *far*choices;
   int far *attributes;
   int filecount;   /* how many files */
   int dircount;    /* how many directories */
   int notroot;     /* not the root directory */

   char drive[FILE_MAX_DRIVE];
   char dir[FILE_MAX_DIR];
   char fname[FILE_MAX_FNAME];
   char ext[FILE_MAX_EXT];
   static int dosort = 1;
   int options = 8;

   rds = (stereomapname == flname)?1:0;

   /* steal existing array for "choices" */
   choices = (struct CHOICE far *far*)MK_FP(extraseg,0);
   choices[0] = (struct CHOICE far *)(choices + MAXNUMFILES+1);
   attributes = (int far *)(choices[0] + MAXNUMFILES+1);
   instr = (char far *)(attributes + MAXNUMFILES +1);
   attributes[0] = 1;
   for(i=1;i<MAXNUMFILES+1;i++)
   {
      choices[i] = choices[i-1] + 1;
      attributes[i] = 1;
   }
   /* save filename */
   strcpy(old_flname,flname);
restart:  /* return here if template or directory changes */

   tmpmask[0] = 0;
   if(flname[0] == 0)
      strcpy(flname,DOTSLASH);
   splitpath(flname ,drive,dir,fname,ext);
   makepath(filename,""   ,"" ,fname,ext);
   retried = 0;
retry_dir:
   if (dir[0] == 0)
      strcpy(dir,".");
   expand_dirname(dir,drive);
   makepath(tmpmask,drive,dir,"","");
   fix_dirname(tmpmask);
   if (retried == 0 && strcmp(dir,SLASH) && strcmp(dir,DOTSLASH))
   {
      tmpmask[(j = strlen(tmpmask) - 1)] = 0; /* strip trailing \ */
      if (strchr(tmpmask,'*') || strchr(tmpmask,'?')
        || fr_findfirst(tmpmask) != 0
        || (DTA.attribute & SUBDIR) == 0)
      {
         strcpy(dir,DOTSLASH);
         ++retried;
         goto retry_dir;
      }
      tmpmask[j] = SLASHC;
   }
   if(template[0])
   {
      numtemplates = 1;
      splitpath(template,NULL,NULL,fname,ext);
   }
   else
      numtemplates = sizeof(masks)/sizeof(masks[0]);
   filecount = -1;
   dircount  = 0;
   notroot   = 0;
   j = 0;
   masklen = strlen(tmpmask);
   strcat(tmpmask,"*.*");
   out = fr_findfirst(tmpmask);
   while(out == 0 && filecount < MAXNUMFILES)
   {
      if((DTA.attribute & SUBDIR) && strcmp(DTA.filename,"."))
      {
#ifndef XFRACT
         strlwr(DTA.filename);
#endif
         if(strcmp(DTA.filename,".."))
            strcat(DTA.filename,SLASH);
         far_strncpy(choices[++filecount]->name,DTA.filename,13);
         choices[filecount]->name[12] = '\0';
         choices[filecount]->type = 1;
         dircount++;
         if(strcmp(DTA.filename,"..")==0)
            notroot = 1;
      }
      out = fr_findnext();
   }
   tmpmask[masklen] = 0;
   if(template[0])
      makepath(tmpmask,drive,dir,fname,ext);
   do
   {
      if(numtemplates > 1)
         strcpy(&(tmpmask[masklen]),masks[j]);
      out = fr_findfirst(tmpmask);
      while(out == 0 && filecount < MAXNUMFILES)
      {
         if(!(DTA.attribute & SUBDIR))
         {
            if(rds)
            {
               sprintf(speedstr,"%s",DTA.filename);
               putstringcenter(2,0,80,C_GENERAL_INPUT,speedstr);

               splitpath(DTA.filename,NULL,NULL,fname,ext);
               /* just using speedstr as a handy buffer */
               makepath(speedstr,drive,dir,fname,ext);
#ifndef XFRACT
               strlwr(DTA.filename);
#endif
               far_strncpy(choices[++filecount]->name,DTA.filename,13);
               choices[filecount]->type = 0;
            }
            else
            {
#ifndef XFRACT
               strlwr(DTA.filename);
#endif
               far_strncpy(choices[++filecount]->name,DTA.filename,13);
               choices[filecount]->type = 0;
            }
         }
         out = fr_findnext();
      }
   }
   while (++j < numtemplates);
   if (++filecount == 0)
   {
      far_strcpy(choices[filecount]->name,"*nofiles*");
      choices[filecount]->type = 0;
      ++filecount;
   }

   far_strcpy(instr,o_instr);
   if(dosort)
   {
      far_strcat(instr,"off");
      shell_sort((void far *far*)choices,filecount,sizeof(char far *),lccompare); /* sort file list */
   }
   else
      far_strcat(instr,"on");
   if(notroot == 0 && dir[0] && dir[0] != SLASHC) /* must be in root directory */
   {
      splitpath(tmpmask,drive,dir,fname,ext);
      strcpy(dir,SLASH);
      makepath(tmpmask,drive,dir,fname,ext);
   }
   if(numtemplates > 1)
   {
      strcat(tmpmask," ");
      strcat(tmpmask,masks[0]);
   }
   strcpy(temp1,hdg);
   strcat(temp1,"\nTemplate: ");
   strcat(temp1,tmpmask);
   strcpy(speedstr,filename);
   if (speedstr[0] == 0)
   {
      for (i=0; i<filecount; i++) /* find first file */
         if (choices[i]->type == 0)
            break;
      if (i >= filecount)
         i = 0;
   }
   if(dosort)
      options = 8;
   else
      options = 8+32;
   i = fullscreen_choice(options,temp1,NULL,instr,filecount,(char far *far*)choices,
          attributes,5,99,12,i,NULL,speedstr,filename_speedstr,check_f6_key);
   if (i==-F4)
   {
      dosort = 1 - dosort;
      goto restart;
   }
   if (i==-F6)
   {
      static int lastdir=0;
      if (lastdir==0)
      {
         strcpy(dir,fract_dir1);
      }
      else
      {
         strcpy(dir,fract_dir2);
      }
      fix_dirname(dir);
      makepath(flname,drive,dir,"","");
      lastdir = 1-lastdir;
      goto restart;
   }
   if (i < 0)
   {
      /* restore filename */
      strcpy(flname,old_flname);
      return(-1);
   }
   if(speedstr[0] == 0 || speedstate == MATCHING)
   {
      if(choices[i]->type)
      {
         if(far_strcmp(choices[i]->name,"..") == 0) /* go up a directory */
         {
            if(strcmp(dir,DOTSLASH) == 0)
               strcpy(dir,DOTDOTSLASH);
            else
            {
               char *s;
               if((s = strrchr(dir,SLASHC)) != NULL) /* trailing slash */
               {
                  *s = 0;
                  if((s = strrchr(dir,SLASHC)) != NULL)
                     *(s+1) = 0;
               }
            }
         }
         else  /* go down a directory */
            far_strcat(dir,choices[i]->name);
         fix_dirname(dir);
         makepath(flname,drive,dir,"","");
         goto restart;
      }
      splitpath(choices[i]->name,NULL,NULL,fname,ext);
      makepath(flname,drive,dir,fname,ext);
   }
   else
   {
      if (speedstate == SEARCHPATH
        && strchr(speedstr,'*') == 0 && strchr(speedstr,'?') == 0
        && ((fr_findfirst(speedstr) == 0
        && (DTA.attribute & SUBDIR))|| strcmp(speedstr,SLASH)==0)) /* it is a directory */
         speedstate = TEMPLATE;

      if(speedstate == TEMPLATE)
      {
         /* extract from tempstr the pathname and template information,
            being careful not to overwrite drive and directory if not
            newly specified */
         char drive1[FILE_MAX_DRIVE];
         char dir1[FILE_MAX_DIR];
         char fname1[FILE_MAX_FNAME];
         char ext1[FILE_MAX_EXT];
         splitpath(speedstr,drive1,dir1,fname1,ext1);
         if(drive1[0])
            strcpy(drive,drive1);
         if(dir1[0])
            strcpy(dir,dir1);
         makepath(flname,drive,dir,fname1,ext1);
         if(strchr(fname1,'*') || strchr(fname1,'?') ||
             strchr(ext1  ,'*') || strchr(ext1  ,'?'))
            makepath(template,"","",fname1,ext1);
         else if(isadirectory(flname))
            fix_dirname(flname);
         goto restart;
      }
      else /* speedstate == SEARCHPATH */
      {
         char fullpath[FILE_MAX_DIR];
         findpath(speedstr,fullpath);
         if(fullpath[0])
            strcpy(flname,fullpath);
         else
         {  /* failed, make diagnostic useful: */
            strcpy(flname,speedstr);
            if (strchr(speedstr,SLASHC) == NULL)
            {
               splitpath(speedstr,NULL,NULL,fname,ext);
               makepath(flname,drive,dir,fname,ext);
            }
         }
      }
   }
   makepath(browsename,"","",fname,ext);
   return(0);
}

#ifdef __CLINT__
#pragma argsused
#endif

static int check_f6_key(int curkey,int choice)
{ /* choice is dummy used by other routines called by fullscreen_choice() */
   choice = 0; /* to suppress warning only */
   if (curkey == F6)
      return 0-F6;
   else if (curkey == F4)
      return 0-F4;
   return 0;
}

static int filename_speedstr(int row, int col, int vid,
                             char *speedstring, int speed_match)
{
   char *prompt;
   if ( strchr(speedstring,':')
     || strchr(speedstring,'*') || strchr(speedstring,'*')
     || strchr(speedstring,'?')) {
      speedstate = TEMPLATE;  /* template */
      prompt = "File Template";
      }
   else if (speed_match) {
      speedstate = SEARCHPATH; /* does not match list */
      prompt = "Search Path for";
      }
   else {
      speedstate = MATCHING;
      prompt = speed_prompt;
      }
   putstring(row,col,vid,prompt);
   return(strlen(prompt));
}

int isadirectory(char *s)
{
   int len;
   char sv;
#ifdef _MSC_VER
   unsigned attrib = 0;
#endif
   despace(s);  /* scrunch out white space */
   if(strchr(s,'*') || strchr(s,'?'))
      return(0); /* for my purposes, not a directory */

   len = strlen(s);
   if(len > 0)
      sv = s[len-1];   /* last char */
   else
      sv = 0;

#ifdef _MSC_VER
   if(_dos_getfileattr(s, &attrib) == 0 && ((attrib&_A_SUBDIR) != 0))
   {
      return(1);  /* not a directory or doesn't exist */
   }
   else if(sv == SLASHC)
   {
      /* strip trailing slash and try again */
      s[len-1] = 0;
      if(_dos_getfileattr(s, &attrib) == 0 && ((attrib&_A_SUBDIR) != 0))
      {
         s[len-1] = sv;
         return(1);
      }
      s[len-1] = sv;
   }
   return(0);
#else
   if(fr_findfirst(s) != 0) /* couldn't find it */
   {
      /* any better ideas?? */
     if(sv == SLASHC) /* we'll guess it is a directory */
         return(1);
      else
         return(0);  /* no slashes - we'll guess it's a file */
   }
   else if((DTA.attribute & SUBDIR) != 0) {
      if(sv == SLASHC) {
      /* strip trailing slash and try again */
         s[len-1] = 0;
         if(fr_findfirst(s) != 0) /* couldn't find it */
             return(0);
         else if((DTA.attribute & SUBDIR) != 0)
             return(1);   /* we're SURE it's a directory */
         else
             return(0);
      } else
         return(1);   /* we're SURE it's a directory */
   }
   return(0);
#endif
}


#ifndef XFRACT  /* This routine moved to unix.c so we can use it in hc.c */

int splitpath(char far *template,char *drive,char *dir,char *fname,char *ext)
{
   int length;
   int len;
   int offset;
   char far *tmp;
   if(drive)
      drive[0] = 0;
   if(dir)
      dir[0]   = 0;
   if(fname)
      fname[0] = 0;
   if(ext)
      ext[0]   = 0;

   if((length = far_strlen(template)) == 0)
      return(0);

   offset = 0;

   /* get drive */
   if(length >= 2)
      if(template[1] == ':')
      {
         if(drive)
         {
            drive[0] = template[offset++];
            drive[1] = template[offset++];
            drive[2] = 0;
         }
         else
         {
            offset++;
            offset++;
         }
      }

   /* get dir */
   if(offset < length)
   {
      tmp = far_strrchr(template,SLASHC);
      if(tmp)
      {
         tmp++;  /* first character after slash */
         len = tmp - (char far *)&template[offset];
         if(len >= 0 && len < FILE_MAX_DIR && dir)
            far_strncpy(dir,&template[offset],min(len,FILE_MAX_DIR));
         if(len < FILE_MAX_DIR && dir)
            dir[len] = 0;
         offset += len;
      }
   }
   else
      return(0);

   /* get fname */
   if(offset < length)
   {
      tmp = far_strrchr(template,'.');
      if(tmp < far_strrchr(template,SLASHC) || tmp < far_strrchr(template,':'))
         tmp = 0; /* in this case the '.' must be a directory */
      if(tmp)
      {
         /* tmp++; */ /* first character past "." */
         len = tmp - (char far *)&template[offset];
         if((len > 0) && (offset+len < length) && fname)
         {
            far_strncpy(fname,&template[offset],min(len,FILE_MAX_FNAME));
            if(len < FILE_MAX_FNAME)
               fname[len] = 0;
            else
               fname[FILE_MAX_FNAME-1] = 0;
         }
         offset += len;
         if((offset < length) && ext)
         {
            far_strncpy(ext,&template[offset],FILE_MAX_EXT);
            ext[FILE_MAX_EXT-1] = 0;
         }
      }
      else if((offset < length) && fname)
      {
         far_strncpy(fname,&template[offset],FILE_MAX_FNAME);
         fname[FILE_MAX_FNAME-1] = 0;
      }
   }
   return(0);
}
#endif

int makepath(char *template,char *drive,char *dir,char *fname,char *ext)
{
   if(template)
      *template = 0;
   else
      return(-1);
#ifndef XFRACT
   if(drive)
      strcpy(template,drive);
#endif
   if(dir)
      strcat(template,dir);
   if(fname)
      strcat(template,fname);
   if(ext)
      strcat(template,ext);
   return(0);
}


/* fix up directory names */
void fix_dirname(char *dirname)
{
   int length;
   despace(dirname);
   length = strlen(dirname); /* index of last character */

   /* make sure dirname ends with a slash */
   if(length > 0)
      if(dirname[length-1] == SLASHC)
         return;
   strcat(dirname,SLASH);
}

static void dir_name(char *target, char *dir, char *name)
{
   *target = 0;
   if(*dir != 0)
      strcpy(target,dir);
   strcat(target,name);
}

/* opens file in dir directory */
int dir_open(char *dir, char *filename, int oflag, int pmode)
{
   char tmp[FILE_MAX_PATH];
   dir_name(tmp,dir,filename);
   return(open(tmp,oflag,pmode));
}

/* removes file in dir directory */
int dir_remove(char *dir,char *filename)
{
   char tmp[FILE_MAX_PATH];
   dir_name(tmp,dir,filename);
   return(remove(tmp));
}

/* fopens file in dir directory */
FILE *dir_fopen(char *dir, char *filename, char *mode )
{
   char tmp[FILE_MAX_PATH];
   dir_name(tmp,dir,filename);
   return(fopen(tmp,mode));
}

/* converts relative path to absolute path */
static int expand_dirname(char *dirname,char *drive)
{
   fix_dirname(dirname);
   if (dirname[0] != SLASHC) {
      char buf[FILE_MAX_DIR+1],curdir[FILE_MAX_DIR+1];
#ifndef XFRACT
      int i=0;
      union REGS regs;
      struct SREGS sregs;
      curdir[0] = 0;
      regs.h.ah = 0x47; /* get current directory */
      regs.h.dl = 0;
      if (drive[0] && drive[0] != ' ')
         regs.h.dl = (char)(tolower(drive[0])-'a'+1);
      regs.x.si = (unsigned int) &curdir[0];
      segread(&sregs);
      intdosx(&regs, &regs, &sregs);
#else
      getcwd(curdir,FILE_MAX_DIR);
#endif
      strcat(curdir,SLASH);
#ifndef XFRACT
      while (curdir[i] != 0) {
         curdir[i] = (char)tolower(curdir[i]);
         i++;
      }
#endif
      while (strncmp(dirname,DOTSLASH,2) == 0) {
         strcpy(buf,&dirname[2]);
         strcpy(dirname,buf);
         }
      while (strncmp(dirname,DOTDOTSLASH,3) == 0) {
         char *s;
         curdir[strlen(curdir)-1] = 0; /* strip trailing slash */
         if ((s = strrchr(curdir,SLASHC)) != NULL)
            *s = 0;
         strcat(curdir,SLASH);
         strcpy(buf,&dirname[3]);
         strcpy(dirname,buf);
         }
      strcpy(buf,dirname);
      dirname[0] = 0;
      if (curdir[0] != SLASHC)
         strcpy(dirname,SLASH);
      strcat(dirname,curdir);
      strcat(dirname,buf);
      }
   return(0);
}


/*
   See if double value was changed by input screen. Problem is that the
   conversion from double to string and back can make small changes
   in the value, so will it twill test as "different" even though it
   is not
*/
int cmpdbl(double old, double new)
{
   char buf[81];
   struct fullscreenvalues val;

   /* change the old value with the same torture the new value had */
   val.type = 'd'; /* most values on this screen are type d */
   val.uval.dval = old;
   prompt_valuestring(buf,&val);   /* convert "old" to string */

   old = atof(buf);                /* convert back */
   return(fabs(old-new)<DBL_EPSILON?0:1);  /* zero if same */
}

#define LOADPROMPTS(X)     {\
   static FCODE tmp[] = { X };\
   far_strcpy(ptr,(char far *)tmp);\
   prompts[++nump]= ptr;\
   ptr += sizeof(tmp);\
   }

int get_corners()
{
   char far *ptr;
   struct fullscreenvalues values[15];
   char far *prompts[15];
   static FCODE o_xprompt[]={"          X"};
   static FCODE o_yprompt[]={"          Y"};
   static FCODE o_zprompt[]={"          Z"};
   char xprompt[sizeof(o_xprompt)];
   char yprompt[sizeof(o_yprompt)];
   char zprompt[sizeof(o_zprompt)];
   int i,nump,prompt_ret;
   int cmag;
   double Xctr,Yctr;
   LDBL Magnification; /* LDBL not really needed here, but used to match function parameters */
   double Xmagfactor,Rotation,Skew;
   BYTE ousemag;
   double oxxmin,oxxmax,oyymin,oyymax,oxx3rd,oyy3rd;
   static FCODE hdg[]={"Image Coordinates"};
   int oldhelpmode;

   far_strcpy(xprompt,o_xprompt);
   far_strcpy(yprompt,o_yprompt);
   far_strcpy(zprompt,o_zprompt);
   ptr = (char far *)MK_FP(extraseg,0);
   oldhelpmode = helpmode;
   ousemag = usemag;
   oxxmin = xxmin; oxxmax = xxmax;
   oyymin = yymin; oyymax = yymax;
   oxx3rd = xx3rd; oyy3rd = yy3rd;

gc_loop:
   for (i = 0; i < 15; ++i)
      values[i].type = 'd'; /* most values on this screen are type d */
   cmag = usemag;
   cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);

   nump = -1;
   if (cmag) {
      LOADPROMPTS("Center X");
      values[nump].uval.dval = Xctr;
      LOADPROMPTS("Center Y");
      values[nump].uval.dval = Yctr;
      LOADPROMPTS("Magnification");
      values[nump].uval.dval = (double)Magnification;
      LOADPROMPTS("X Magnification Factor");
      values[nump].uval.dval = Xmagfactor;
      LOADPROMPTS("Rotation Angle (degrees)");
      values[nump].uval.dval = Rotation;
      LOADPROMPTS("Skew Angle (degrees)");
      values[nump].uval.dval = Skew;
      LOADPROMPTS("");
      values[nump].type = '*';
      LOADPROMPTS("Press "FK_F7" to switch to \"corners\" mode");
      values[nump].type = '*';
      }

   else {
      LOADPROMPTS("Top-Left Corner");
      values[nump].type = '*';
      prompts[++nump] = xprompt;
      values[nump].uval.dval = xxmin;
      prompts[++nump] = yprompt;
      values[nump].uval.dval = yymax;
      LOADPROMPTS("Bottom-Right Corner");
      values[nump].type = '*';
      prompts[++nump] = xprompt;
      values[nump].uval.dval = xxmax;
      prompts[++nump] = yprompt;
      values[nump].uval.dval = yymin;
      if (xxmin == xx3rd && yymin == yy3rd)
         xx3rd = yy3rd = 0;
      LOADPROMPTS("Bottom-left (zeros for top-left X, bottom-right Y)");
      values[nump].type = '*';
      prompts[++nump] = xprompt;
      values[nump].uval.dval = xx3rd;
      prompts[++nump] = yprompt;
      values[nump].uval.dval = yy3rd;
      LOADPROMPTS("Press "FK_F7" to switch to \"center-mag\" mode");
      values[nump].type = '*';
   }

   LOADPROMPTS("Press "FK_F4" to reset to type default values");
   values[nump].type = '*';

   oldhelpmode = helpmode;
   helpmode = HELPCOORDS;
   prompt_ret = fullscreen_prompt(hdg,nump+1, prompts, values, 0x90, NULL);
   helpmode = oldhelpmode;

   if (prompt_ret < 0) {
      usemag = ousemag;
      xxmin = oxxmin; xxmax = oxxmax;
      yymin = oyymin; yymax = oyymax;
      xx3rd = oxx3rd; yy3rd = oyy3rd;
      return(-1);
      }

   if (prompt_ret == F4) { /* reset to type defaults */
      xx3rd = xxmin = curfractalspecific->xmin;
      xxmax         = curfractalspecific->xmax;
      yy3rd = yymin = curfractalspecific->ymin;
      yymax         = curfractalspecific->ymax;
      if (viewcrop && finalaspectratio != screenaspect)
         aspectratio_crop(screenaspect,finalaspectratio);
      if(bf_math != 0)
         fractal_floattobf();
      goto gc_loop;
      }

   if (cmag) {
      if ( cmpdbl(Xctr         , values[0].uval.dval)
        || cmpdbl(Yctr         , values[1].uval.dval)
        || cmpdbl((double)Magnification, values[2].uval.dval)
        || cmpdbl(Xmagfactor   , values[3].uval.dval)
        || cmpdbl(Rotation     , values[4].uval.dval)
        || cmpdbl(Skew         , values[5].uval.dval))
      {
         Xctr          = values[0].uval.dval;
         Yctr          = values[1].uval.dval;
         Magnification = values[2].uval.dval;
         Xmagfactor    = values[3].uval.dval;
         Rotation      = values[4].uval.dval;
         Skew          = values[5].uval.dval;
         if (Xmagfactor == 0)
            Xmagfactor = 1;
         cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
      }
   }

   else {
      nump = 1;
      xxmin = values[nump++].uval.dval;
      yymax = values[nump++].uval.dval;
      nump++;
      xxmax = values[nump++].uval.dval;
      yymin = values[nump++].uval.dval;
      nump++;
      xx3rd = values[nump++].uval.dval;
      yy3rd = values[nump++].uval.dval;
      if (xx3rd == 0 && yy3rd == 0) {
         xx3rd = xxmin;
         yy3rd = yymin;
      }
   }

   if (prompt_ret == F7) { /* toggle corners/center-mag mode */
      if (usemag == 0)
      {
         cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            usemag = 1;
      }
      else
         usemag = 0;
      goto gc_loop;
      }

   if(!cmpdbl(oxxmin,xxmin) && !cmpdbl(oxxmax,xxmax) && !cmpdbl(oyymin,yymin) &&
      !cmpdbl(oyymax,yymax) && !cmpdbl(oxx3rd,xx3rd) && !cmpdbl(oyy3rd,yy3rd))
   {
     /* no change, restore values to avoid drift */
      xxmin = oxxmin; xxmax = oxxmax;
      yymin = oyymin; yymax = oyymax;
      xx3rd = oxx3rd; yy3rd = oyy3rd;
      return 0;
   }
   else
      return(1);
}

static int get_screen_corners(void)
{
   char far *ptr;
   struct fullscreenvalues values[15];
   char far *prompts[15];
   static FCODE o_xprompt[]={"          X"};
   static FCODE o_yprompt[]={"          Y"};
   static FCODE o_zprompt[]={"          Z"};
   char xprompt[sizeof(o_xprompt)];
   char yprompt[sizeof(o_yprompt)];
   char zprompt[sizeof(o_zprompt)];
   int i,nump,prompt_ret;
   int cmag = 0;
   double Xctr,Yctr;
   LDBL Magnification; /* LDBL not really needed here, but used to match function parameters */
   double Xmagfactor,Rotation,Skew;
   double oxxmin,oxxmax,oyymin,oyymax,oxx3rd,oyy3rd;
   double svxxmin,svxxmax,svyymin,svyymax,svxx3rd,svyy3rd;
   static FCODE hdg[]={"Screen Coordinates"};
   int oldhelpmode;

   far_strcpy(xprompt,o_xprompt);
   far_strcpy(yprompt,o_yprompt);
   far_strcpy(zprompt,o_zprompt);
   ptr = (char far *)MK_FP(extraseg,0);
   oldhelpmode = helpmode;

   svxxmin = xxmin;  /* save these for later since cvtcorners modifies them */
   svxxmax = xxmax;  /* and we need to set them for cvtcentermag to work */
   svxx3rd = xxmin;
   svyymin = yymin;
   svyymax = yymax;
   svyy3rd = yymin;

   if (!set_orbit_corners && !keep_scrn_coords) {
      oxmin = xxmin;
      oxmax = xxmax;
      ox3rd = xxmin;
      oymin = yymin;
      oymax = yymax;
      oy3rd = yymin;
   }

   oxxmin = oxmin; oxxmax = oxmax;
   oyymin = oymin; oyymax = oymax;
   oxx3rd = ox3rd; oyy3rd = oy3rd;

   xxmin = oxmin; xxmax = oxmax;
   yymin = oymin; yymax = oymax;
   xx3rd = ox3rd; yy3rd = oy3rd;

gsc_loop:
   for (i = 0; i < 15; ++i)
      values[i].type = 'd'; /* most values on this screen are type d */
   cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);

   nump = -1;
   if (cmag) {
      LOADPROMPTS("Center X");
      values[nump].uval.dval = Xctr;
      LOADPROMPTS("Center Y");
      values[nump].uval.dval = Yctr;
      LOADPROMPTS("Magnification");
      values[nump].uval.dval = (double)Magnification;
      LOADPROMPTS("X Magnification Factor");
      values[nump].uval.dval = Xmagfactor;
      LOADPROMPTS("Rotation Angle (degrees)");
      values[nump].uval.dval = Rotation;
      LOADPROMPTS("Skew Angle (degrees)");
      values[nump].uval.dval = Skew;
      LOADPROMPTS("");
      values[nump].type = '*';
      LOADPROMPTS("Press "FK_F7" to switch to \"corners\" mode");
      values[nump].type = '*';
      }

   else {
      LOADPROMPTS("Top-Left Corner");
      values[nump].type = '*';
      prompts[++nump] = xprompt;
      values[nump].uval.dval = oxmin;
      prompts[++nump] = yprompt;
      values[nump].uval.dval = oymax;
      LOADPROMPTS("Bottom-Right Corner");
      values[nump].type = '*';
      prompts[++nump] = xprompt;
      values[nump].uval.dval = oxmax;
      prompts[++nump] = yprompt;
      values[nump].uval.dval = oymin;
      if (oxmin == ox3rd && oymin == oy3rd)
         ox3rd = oy3rd = 0;
      LOADPROMPTS("Bottom-left (zeros for top-left X, bottom-right Y)");
      values[nump].type = '*';
      prompts[++nump] = xprompt;
      values[nump].uval.dval = ox3rd;
      prompts[++nump] = yprompt;
      values[nump].uval.dval = oy3rd;
      LOADPROMPTS("Press "FK_F7" to switch to \"center-mag\" mode");
      values[nump].type = '*';
   }

   LOADPROMPTS("Press "FK_F4" to reset to type default values");
   values[nump].type = '*';

   oldhelpmode = helpmode;
   helpmode = HELPSCRNCOORDS;
   prompt_ret = fullscreen_prompt(hdg,nump+1, prompts, values, 0x90, NULL);
   helpmode = oldhelpmode;

   if (prompt_ret < 0) {
      oxmin = oxxmin; oxmax = oxxmax;
      oymin = oyymin; oymax = oyymax;
      ox3rd = oxx3rd; oy3rd = oyy3rd;
      /* restore corners */
      xxmin = svxxmin; xxmax = svxxmax;
      yymin = svyymin; yymax = svyymax;
      xx3rd = svxx3rd; yy3rd = svyy3rd;
      return(-1);
      }

   if (prompt_ret == F4) { /* reset to type defaults */
      ox3rd = oxmin = curfractalspecific->xmin;
      oxmax         = curfractalspecific->xmax;
      oy3rd = oymin = curfractalspecific->ymin;
      oymax         = curfractalspecific->ymax;
      xxmin = oxmin; xxmax = oxmax;
      yymin = oymin; yymax = oymax;
      xx3rd = ox3rd; yy3rd = oy3rd;
      if (viewcrop && finalaspectratio != screenaspect)
         aspectratio_crop(screenaspect,finalaspectratio);

      oxmin = xxmin; oxmax = xxmax;
      oymin = yymin; oymax = yymax;
      ox3rd = xxmin; oy3rd = yymin;
      goto gsc_loop;
      }

   if (cmag) {
      if ( cmpdbl(Xctr         , values[0].uval.dval)
        || cmpdbl(Yctr         , values[1].uval.dval)
        || cmpdbl((double)Magnification, values[2].uval.dval)
        || cmpdbl(Xmagfactor   , values[3].uval.dval)
        || cmpdbl(Rotation     , values[4].uval.dval)
        || cmpdbl(Skew         , values[5].uval.dval))
      {
         Xctr          = values[0].uval.dval;
         Yctr          = values[1].uval.dval;
         Magnification = values[2].uval.dval;
         Xmagfactor    = values[3].uval.dval;
         Rotation      = values[4].uval.dval;
         Skew          = values[5].uval.dval;
         if (Xmagfactor == 0)
            Xmagfactor = 1;
         cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
         /* set screen corners */
         oxmin = xxmin; oxmax = xxmax;
         oymin = yymin; oymax = yymax;
         ox3rd = xx3rd; oy3rd = yy3rd;
      }
   }

   else {
      nump = 1;
      oxmin = values[nump++].uval.dval;
      oymax = values[nump++].uval.dval;
      nump++;
      oxmax = values[nump++].uval.dval;
      oymin = values[nump++].uval.dval;
      nump++;
      ox3rd = values[nump++].uval.dval;
      oy3rd = values[nump++].uval.dval;
      if (ox3rd == 0 && oy3rd == 0) {
         ox3rd = oxmin;
         oy3rd = oymin;
      }
   }

   if (prompt_ret == F7) { /* toggle corners/center-mag mode */
      if (cmag == 0)
      {
         cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            cmag = 1;
      }
      else
         cmag = 0;
      goto gsc_loop;
      }

   if(!cmpdbl(oxxmin,oxmin) && !cmpdbl(oxxmax,oxmax) && !cmpdbl(oyymin,oymin) &&
      !cmpdbl(oyymax,oymax) && !cmpdbl(oxx3rd,ox3rd) && !cmpdbl(oyy3rd,oy3rd))
   {
     /* no change, restore values to avoid drift */
      oxmin = oxxmin; oxmax = oxxmax;
      oymin = oyymin; oymax = oyymax;
      ox3rd = oxx3rd; oy3rd = oyy3rd;
      /* restore corners */
      xxmin = svxxmin; xxmax = svxxmax;
      yymin = svyymin; yymax = svyymax;
      xx3rd = svxx3rd; yy3rd = svyy3rd;
      return 0;
   }
   else {
      set_orbit_corners = 1;
      keep_scrn_coords = 1;
      /* restore corners */
      xxmin = svxxmin; xxmax = svxxmax;
      yymin = svyymin; yymax = svyymax;
      xx3rd = svxx3rd; yy3rd = svyy3rd;
      return(1);
   }
}

/* get browse parameters , called from fractint.c and loadfile.c
   returns 3 if anything changes.  code pinched from get_view_params */

int get_browse_params()
{
   static FCODE o_hdg[]={"Browse ('L'ook) Mode Options"};
   char hdg[sizeof(o_hdg)];
   char far *ptr;
   char far *choices[10];

   int oldhelpmode;
   struct fullscreenvalues uvalues[25];
   int i, k;
   int old_autobrowse,old_brwschecktype,old_brwscheckparms,old_doublecaution;
   int old_minbox;
   double old_toosmall;
   char old_browsemask[13];

   far_strcpy(hdg,o_hdg);
   ptr = (char far *)MK_FP(extraseg,0);
   old_autobrowse     = autobrowse;
   old_brwschecktype  = brwschecktype;
   old_brwscheckparms = brwscheckparms;
   old_doublecaution  = doublecaution;
   old_minbox         = minbox;
   old_toosmall       = toosmall;
   strcpy(old_browsemask,browsemask);

get_brws_restart:
   /* fill up the previous values arrays */
   k = -1;

   LOADCHOICES("Autobrowsing? (y/n)");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = autobrowse;

   LOADCHOICES("Ask about GIF video mode? (y/n)");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = askvideo;

   LOADCHOICES("Check fractal type? (y/n)");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = brwschecktype;

   LOADCHOICES("Check fractal parameters (y/n)");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = brwscheckparms;

   LOADCHOICES("Confirm file deletes (y/n)");
   uvalues[k].type='y';
   uvalues[k].uval.ch.val = doublecaution;

   LOADCHOICES("Smallest window to display (size in pixels)");
   uvalues[k].type = 'f';
   uvalues[k].uval.dval = toosmall;

   LOADCHOICES("Smallest box size shown before crosshairs used (pix)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = minbox;
    LOADCHOICES("Browse search filename mask ");
   uvalues[k].type = 's';
   strcpy(uvalues[k].uval.sval,browsemask);

   LOADCHOICES("");
   uvalues[k].type = '*';

   LOADCHOICES("Press "FK_F4" to reset browse parameters to defaults.");
   uvalues[k].type = '*';

   oldhelpmode = helpmode;     /* this prevents HELP from activating */
   helpmode = HELPBRWSPARMS;
   i = fullscreen_prompt(hdg,k+1,choices,uvalues,16,NULL);
   helpmode = oldhelpmode;     /* re-enable HELP */
   if (i < 0) {
      return(0);
      }

   if (i == F4) {
      toosmall = 6;
      autobrowse = FALSE;
      askvideo = TRUE;
      brwscheckparms = TRUE;
      brwschecktype  = TRUE;
      doublecaution  = TRUE;
      minbox = 3;
      strcpy(browsemask,"*.gif");
      goto get_brws_restart;
      }

   /* now check out the results (*hopefully* in the same order <grin>) */
   k = -1;

   autobrowse = uvalues[++k].uval.ch.val;

   askvideo = uvalues[++k].uval.ch.val;

   brwschecktype = (char)uvalues[++k].uval.ch.val;

   brwscheckparms = (char)uvalues[++k].uval.ch.val;

   doublecaution = uvalues[++k].uval.ch.val;

   toosmall  = uvalues[++k].uval.dval;
   if (toosmall < 0 ) toosmall = 0 ;

   minbox = uvalues[++k].uval.ival;
     if (minbox < 1 ) minbox = 1;
     if (minbox > 10) minbox = 10;

   strcpy(browsemask,uvalues[++k].uval.sval);

   i = 0;
   if (autobrowse != old_autobrowse ||
       brwschecktype != old_brwschecktype ||
       brwscheckparms != old_brwscheckparms ||
       doublecaution != old_doublecaution ||
       toosmall != old_toosmall ||
       minbox != old_minbox ||
       !stricmp(browsemask,old_browsemask) )
        i = -3;

   if (evolving) { /* can't browse */
      autobrowse = 0;
      i = 0;
   }

   return(i);
}

/* merge existing full path with new one  */
/* attempt to detect if file or directory */

#define ATFILENAME 0
#define SSTOOLSINI 1
#define ATCOMMANDINTERACTIVE 2
#define ATFILENAMESETNAME  3

#define GETPATH (mode < 2)

#ifndef XFRACT
#include <direct.h>
#endif

/* copies the proposed new filename to the fullpath variable */
/* does not copy directories for PAR files (modes 2 and 3)   */
/* attempts to extract directory and test for existence (modes 0 and 1) */
int merge_pathnames(char *oldfullpath, char *newfilename, int mode)
{
   int isadir = 0;
   int isafile = 0;
   int len;
   char drive[FILE_MAX_DRIVE];
   char dir[FILE_MAX_DIR];
   char fname[FILE_MAX_FNAME];
   char ext[FILE_MAX_EXT];
#ifndef XFRACT
   char temp_path[FILE_MAX_PATH];
#endif

   char drive1[FILE_MAX_DRIVE];
   char dir1[FILE_MAX_DIR];
   char fname1[FILE_MAX_FNAME];
   char ext1[FILE_MAX_EXT];

   /* no dot or slash so assume a file */
   if(strchr(newfilename,'.')==NULL && strchr(newfilename,SLASHC) == NULL)
      isafile=1;
   if((isadir = isadirectory(newfilename)) != 0)
      fix_dirname(newfilename);
#if 0
   /* if slash by itself, it's a directory */
   if(strcmp(newfilename,SLASH)==0)
      isadir = 1;
#endif
#ifndef XFRACT
   /* if drive, colon, slash, is a directory */
   if(strlen(newfilename) == 3 &&
           newfilename[1] == ':' &&
           newfilename[2] == SLASHC)
      isadir = 1;
   /* if drive, colon, with no slash, is a directory */
   if(strlen(newfilename) == 2 &&
           newfilename[1] == ':') {
      newfilename[2] = SLASHC;
      newfilename[3] = 0;
      isadir = 1;
      }
   /* if dot, slash, '0', its the current directory, set up full path */
   if(newfilename[0] == '.' &&
           newfilename[1] == SLASHC && newfilename[2] == 0) {
      temp_path[0] = (char)('a' + _getdrive() - 1);
      temp_path[1] = ':';
      temp_path[2] = 0;
      expand_dirname(newfilename,temp_path);
      strcat(temp_path,newfilename);
      strcpy(newfilename,temp_path);
      isadir = 1;
      }
   /* if dot, slash, its relative to the current directory, set up full path */
   if(newfilename[0] == '.' &&
           newfilename[1] == SLASHC) {
      int len, test_dir=0;
      temp_path[0] = (char)('a' + _getdrive() - 1);
      temp_path[1] = ':';
      temp_path[2] = 0;
      if (strrchr(newfilename,'.') == newfilename)
        test_dir = 1;  /* only one '.' assume its a directory */
      expand_dirname(newfilename,temp_path);
      strcat(temp_path,newfilename);
      strcpy(newfilename,temp_path);
      if (!test_dir) {
         len = strlen(newfilename);
         newfilename[len-1] = 0; /* get rid of slash added by expand_dirname */
      }
   }
#endif
   /* check existence */
   if(isadir==0 || isafile==1)
   {
       if(fr_findfirst(newfilename) == 0) {
          if(DTA.attribute & SUBDIR) /* exists and is dir */
          {
             fix_dirname(newfilename);  /* add trailing slash */
             isadir = 1;
             isafile = 0;
          }
          else
             isafile = 1;
       }
   }

   splitpath(newfilename,drive,dir,fname,ext);
   splitpath(oldfullpath,drive1,dir1,fname1,ext1);
   if(strlen(drive) != 0 && GETPATH)
      strcpy(drive1,drive);
   if(strlen(dir) != 0 && GETPATH)
      strcpy(dir1,dir);
   if(strlen(fname) != 0)
      strcpy(fname1,fname);
   if(strlen(ext) != 0)
      strcpy(ext1,ext);
   if(isadir == 0 && isafile == 0 && GETPATH)
   {
      makepath(oldfullpath,drive1,dir1,NULL,NULL);
      len = strlen(oldfullpath);
      if(len > 0)
      {
         char save;
         /* strip trailing slash */
         save = oldfullpath[len-1];
         if(save == SLASHC)
            oldfullpath[len-1] = 0;
         if(access(oldfullpath,0))
            isadir = -1;
         oldfullpath[len-1] = save;
      }
   }
   makepath(oldfullpath,drive1,dir1,fname1,ext1);
   return(isadir);
}

/* extract just the filename/extension portion of a path */
void extract_filename(char *target, char *source)
{
   char fname[FILE_MAX_FNAME];
   char ext[FILE_MAX_EXT];
   splitpath(source,NULL,NULL,fname,ext);
   makepath(target,"","",fname,ext);
}

/* tells if filename has extension */
/* returns pointer to period or NULL */
char *has_ext(char *source)
{
   char fname[FILE_MAX_FNAME];
   char ext[FILE_MAX_EXT];
   char *ret = NULL;
   splitpath(source,NULL,NULL,fname,ext);
   if(ext != NULL)
      if(*ext != 0)
         ret = strrchr(source,'.');
   return(ret);
}


/* I tried heap sort also - this is faster! */
void shell_sort(void far *v1, int n, unsigned sz, int (__cdecl *fct)(VOIDFARPTR arg1,VOIDFARPTR arg2))
{
   int gap,i,j;
   void far *temp;
   char far *v;
   v = (char far *)v1;
   for(gap = n/2; gap > 0; gap /= 2)
      for(i = gap; i<n; i++)
         for(j=i-gap;j>=0; j -= gap)
         {
            if(fct((char far *far*)(v+j*sz),(char far *far*)(v+(j+gap)*sz)) <= 0)
               break;
            temp = *(char far *far*)(v+j*sz);
            *(char far *far*)(v+j*sz) = *(char far *far*)(v+(j+gap)*sz);
            *(char far *far*)(v+(j+gap)*sz) = temp;
         }
}

#if (_MSC_VER >= 700)
#pragma code_seg ("prompts3_text")     /* place following in an overlay */
#endif

void far_strncpy(char far *t, char far *s, int len)
{
   while((len-- && (*t++ = *s++) != 0));
}

char far *far_strchr(char far *str, char c)
{
   int len,i;
   len = far_strlen(str);
   i= -1;
   while (++i < len && c != str[i]);
   if(i == len)
      return(NULL);
   else
      return(&str[i]);
}

char far *far_strrchr(char far *str, char c)
{
   int len;
   len = far_strlen(str);
   while (--len > -1 && c != str[len]);
   if(len == -1)
      return(NULL);
   else
      return(&str[len]);
}

#if (_MSC_VER >= 700)
#pragma code_seg ("")
#endif
