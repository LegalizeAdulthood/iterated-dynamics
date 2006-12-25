#include <string.h>
#include <time.h>

#ifndef XFRACT
#include <io.h>
#endif

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <ctype.h>
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"

#if 0
/* makes a handly list of jul-man pairs, not for release */
static void julman()
{
   FILE *fp;
   int i;
   fp = dir_fopen(workdir,"toggle.txt","w");
   i = -1;
   while(fractalspecific[++i].name)
   {
      if(fractalspecific[i].tojulia != NOFRACTAL && fractalspecific[i].name[0] != '*')
         fprintf(fp,"%s  %s\n",fractalspecific[i].name,
             fractalspecific[fractalspecific[i].tojulia].name);
   }
   fclose(fp);
}
#endif

/* routines in this module      */

int main_menu_switch(int*,int*,int*,char*,int);
int evolver_menu_switch(int*,int*,int*,char*);
int big_while_loop(int *kbdmore, char *stacked, int resumeflag);
static void move_zoombox(int);
char fromtext_flag = 0;         /* = 1 if we're in graphics mode */
static int call_line3d(BYTE *pixels, int linelen);
static  void note_zoom(void);
static  void restore_zoom(void);
static  void move_zoombox(int keynum);
static  void cmp_line_cleanup(void);
static void _fastcall restore_history_info(int);
static void _fastcall save_history_info(void);

U16 evolve_handle = 0;
char old_stdcalcmode;
static char *savezoom;
static  int        historyptr = -1;     /* user pointer into history tbl  */
static  int        saveptr = 0;         /* save ptr into history tbl      */
static  int        historyflag;         /* are we backing off in history? */
void (*outln_cleanup) (void);

int big_while_loop(int *kbdmore, char *stacked, int resumeflag)
{
   int     frommandel;                  /* if julia entered from mandel */
   int     axmode=0, bxmode, cxmode, dxmode; /* video mode (BIOS ##)    */
   double  ftemp;                       /* fp temp                      */
   int     i=0;                           /* temporary loop counters      */
   int kbdchar;
   int mms_value;
   frommandel = 0;
   if(resumeflag)
      goto resumeloop;
    for(;;) {                   /* eternal loop */
      if (calc_status != 2 || showfile == 0) {
#ifdef XFRACT
         if (resizeWindow()) {
             calc_status = -1;
         }
#endif
         memcpy((char *)&videoentry,(char *)&videotable[adapter],
                    sizeof(videoentry));
         axmode  = videoentry.videomodeax; /* video mode (BIOS call)   */
         bxmode  = videoentry.videomodebx; /* video mode (BIOS call)   */
         cxmode  = videoentry.videomodecx; /* video mode (BIOS call)   */
         dxmode  = videoentry.videomodedx; /* video mode (BIOS call)   */
         dotmode = videoentry.dotmode;     /* assembler dot read/write */
         xdots   = videoentry.xdots;       /* # dots across the screen */
         ydots   = videoentry.ydots;       /* # dots down the screen   */
         colors  = videoentry.colors;      /* # colors available */
         dotmode %= 1000;
         textsafe2 = dotmode / 100;
         dotmode  %= 100;
         sxdots  = xdots;
         sydots  = ydots;
         sxoffs = syoffs = 0;
         rotate_hi = (rotate_hi < colors) ? rotate_hi : colors - 1;

         diskvideo = 0;                 /* set diskvideo flag */
         if (driver_diskp())		/* default assumption is disk */
            diskvideo = 2;

         memcpy(olddacbox,g_dacbox,256*3); /* save the DAC */
         diskisactive = 1;              /* flag for disk-video routines */

         if (overlay3d && !initbatch) {
            driver_unstack_screen();            /* restore old graphics image */
            overlay3d = 0;
            }

         else {
            driver_set_video_mode(axmode, bxmode, cxmode, dxmode); /* switch video modes */
            if (goodmode == 0) {
               static char msg[] = {"That video mode is not available with your adapter."};
#ifndef XFRACT
               static char TPlusStr[] = "This video mode requires 'noninterlaced=yes'";

               if(TPlusErr) {
                  stopmsg(0, TPlusStr);
                  TPlusErr = 0;
                  }
               else
#endif	       
	       if (driver_diskp()) {
                  askvideo = TRUE;
                  }
               else {
                  stopmsg(0,msg);
                  askvideo = TRUE;
                  }
               initmode = -1;
               driver_set_for_text(); /* switch to text mode */
               /* goto restorestart; */
               return(RESTORESTART);
               }

            if (virtual_screens && (xdots > sxdots || ydots > sydots)) {
               char buf[120];
               static char msgxy1[] = {"Can't set virtual line that long, width cut down."};
               static char msgxy2[] = {"Not enough video memory for that many lines, height cut down."};
               if (xdots > sxdots && ydots > sydots) {
#ifndef XFRACT
                  sprintf(buf,"%Fs\n%Fs",(char *)msgxy1,(char *)msgxy2);
#else
                  sprintf(buf,"%s\n%s",(char *)msgxy1,(char *)msgxy2);
#endif
                  stopmsg(0,buf);
                }
                else if (ydots > sydots) {
                  stopmsg(0,msgxy2);
                }
                else {
                  stopmsg(0,msgxy1);
                }
            }
            xdots = sxdots;
            ydots = sydots;
            videoentry.xdots = xdots;
            videoentry.ydots = ydots;
         }

         diskisactive = 0;              /* flag for disk-video routines */
         if (savedac || colorpreloaded) {
            memcpy(g_dacbox,olddacbox,256*3); /* restore the DAC */
            spindac(0,1);
            colorpreloaded = 0;
            }
         else { /* reset DAC to defaults, which setvideomode has done for us */
            if (mapdacbox) { /* but there's a map=, so load that */
               memcpy((char *)g_dacbox,mapdacbox,768);
               spindac(0,1);
               }
            else if ((driver_diskp() && colors == 256) || !colors) {
               /* disk video, setvideomode via bios didn't get it right, so: */
#ifndef XFRACT
               ValidateLuts("default"); /* read the default palette file */
#endif
               }
            colorstate = 0;
            }
         if (viewwindow) {
            ftemp = finalaspectratio    /* bypass for VESA virtual screen */
                    * ((dotmode == 28 && ((vesa_xres && vesa_xres != sxdots)
                       || (vesa_yres && vesa_yres != sydots)))
                    ? 1 : (double)sydots / (double)sxdots / screenaspect);
            if ((xdots = viewxdots) != 0) { /* xdots specified */
               if ((ydots = viewydots) == 0) /* calc ydots? */
                  ydots = (int)((double)xdots * ftemp + 0.5);
            }
            else
               if (finalaspectratio <= screenaspect) {
                  xdots = (int)((double)sxdots / viewreduction + 0.5);
                  ydots = (int)((double)xdots * ftemp + 0.5);
               }
               else {
                  ydots = (int)((double)sydots / viewreduction + 0.5);
                  xdots = (int)((double)ydots / ftemp + 0.5);
               }
            if (xdots > sxdots || ydots > sydots) {
               static char msg[] = {"View window too large; using full screen."};
               stopmsg(0,msg);
               viewwindow = 0;
               xdots = viewxdots = sxdots;
               ydots = viewydots = sydots;
            }
            else if (((xdots <= 1) /* changed test to 1, so a 2x2 window will */
                  || (ydots <= 1)) /* work with the sound feature */
                  && !(evolving&1)) { /* so ssg works */
                  /* but no check if in evolve mode to allow lots of small views*/
               static char msg[] = {"View window too small; using full screen."};
               stopmsg(0,msg);
               viewwindow = 0;
               xdots = sxdots;
               ydots = sydots;
            }
            if ((evolving&1) && (curfractalspecific->flags&INFCALC)) {
               static char msg[] = {"Fractal doesn't terminate! switching off evolution."};
               stopmsg(0,msg);
               evolving = evolving -1;
               viewwindow = FALSE;
               xdots=sxdots;
               ydots=sydots;
            }
            if (evolving&1) {
               xdots = (sxdots / gridsz)-!((evolving & NOGROUT)/NOGROUT);
               xdots = xdots - (xdots % 4); /* trim to multiple of 4 for SSG */
               ydots = (sydots / gridsz)-!((evolving & NOGROUT)/NOGROUT);
               ydots = ydots - (ydots % 4);
            }
            else {
               sxoffs = (sxdots - xdots) / 2;
               syoffs = (sydots - ydots) / 3;
            }
         }
         dxsize = xdots - 1;            /* convert just once now */
         dysize = ydots - 1;
      }
      if(savedac == 0)
        savedac = 2;                    /* assume we save next time (except jb) */
      else
      savedac = 1;                      /* assume we save next time */
      if (initbatch == 0)
         lookatmouse = -PAGE_UP;        /* mouse left button == pgup */

      if(showfile == 0) {               /* loading an image */
         outln_cleanup = NULL;          /* outln routine can set this */
         if (display3d)                 /* set up 3D decoding */
            outln = call_line3d;
         else if(filetype >= 1)         /* old .tga format input file */
            outln = outlin16;
         else if(comparegif)            /* debug 50 */
            outln = cmp_line;
         else if(pot16bit) {            /* .pot format input file */
            if (pot_startdisk() < 0)
            {                           /* pot file failed?  */
               showfile = 1;
               potflag  = 0;
               pot16bit = 0;
               initmode = -1;
               calc_status = 2;         /* "resume" without 16-bit */
               driver_set_for_text();
               get_fracttype();
               /* goto imagestart; */
               return(IMAGESTART);
            }
            outln = pot_line;
         }
         else if((soundflag&7) > 1 && !evolving) /* regular gif/fra input file */
            outln = sound_line;      /* sound decoding */
         else
            outln = out_line;        /* regular decoding */
         if(filetype == 0)
         {
            if(debugflag==2224)
            {
               char msg[MSGLEN];
               sprintf(msg,"floatflag=%d",usr_floatflag);
               stopmsg(STOPMSG_NO_BUZZER,(char *)msg);
            }

            i = funny_glasses_call(gifview);
         }
         else
            i = funny_glasses_call(tgaview);
         if(outln_cleanup)              /* cleanup routine defined? */
            (*outln_cleanup)();
         if(i == 0)
            driver_buzzer(0);
         else {
            calc_status = -1;
            if (driver_key_pressed()) {
               static char msg[] = {"*** load incomplete ***"};
               driver_buzzer(1);
               while (driver_key_pressed()) driver_get_key();
               texttempmsg(msg);
               }
            }
         }

      zoomoff = 1;                      /* zooming is enabled */
      if (driver_diskp() || (curfractalspecific->flags&NOZOOM) != 0)
         zoomoff = 0;                   /* for these cases disable zooming */
      if (!evolving)
         calcfracinit();
      driver_schedule_alarm(1);

      sxmin = xxmin; /* save 3 corners for zoom.c ref points */
      sxmax = xxmax;
      sx3rd = xx3rd;
      symin = yymin;
      symax = yymax;
      sy3rd = yy3rd;

      if(bf_math)
      {
         copy_bf(bfsxmin,bfxmin);
         copy_bf(bfsxmax,bfxmax);
         copy_bf(bfsymin,bfymin);
         copy_bf(bfsymax,bfymax);
         copy_bf(bfsx3rd,bfx3rd);
         copy_bf(bfsy3rd,bfy3rd);
      }
      save_history_info();
      if (display3d || showfile) {      /* paranoia: these vars don't get set */
         save_system  = active_system;  /*   unless really doing some work,   */
         }                              /*   so simple <r> + <s> keeps number */

      if(showfile == 0) {               /* image has been loaded */
         showfile = 1;
         if (initbatch == 1 && calc_status == 2)
            initbatch = -1; /* flag to finish calc before save */
         if (loaded3d)      /* 'r' of image created with '3' */
            display3d = 1;  /* so set flag for 'b' command */
         }
      else {                            /* draw an image */
         diskisactive = 1;              /* flag for disk-video routines */
         if (initsavetime != 0          /* autosave and resumable? */
           && (curfractalspecific->flags&NORESUME) == 0) {
            savebase = readticker(); /* calc's start time */
            saveticks = abs(initsavetime);
            saveticks *= 1092; /* bios ticks/minute */
            if ((saveticks & 65535L) == 0)
               ++saveticks; /* make low word nonzero */
            finishrow = -1;
            }
         browsing = FALSE;      /* regenerate image, turn off browsing */
 /*rb*/
      name_stack_ptr = -1;   /* reset pointer */
      browsename[0] = '\0';  /* null */
      if (viewwindow && (evolving&1) && (calc_status != 4))
 /*generate a set of images with varied parameters on each one*/
        {
        int grout,ecount,tmpxdots,tmpydots,gridsqr;
        struct evolution_info resume_e_info;
        GENEBASE gene[NUMGENES];
        /* get the gene array from memory */
        MoveFromMemory((BYTE *)&gene, (U16)sizeof(gene), 1L, 0L, gene_handle);
        if ((evolve_handle != 0) && (calc_status == 2)) {
           MoveFromMemory((BYTE *)&resume_e_info,(U16)sizeof(resume_e_info),1L,0L,evolve_handle);
           paramrangex  = resume_e_info.paramrangex;
           paramrangey  = resume_e_info.paramrangey;
           opx = newopx = resume_e_info.opx;
           opy = newopy = resume_e_info.opy;
           odpx = newodpx = (char)resume_e_info.odpx;
           odpy = newodpy = (char)resume_e_info.odpy;
           px           = resume_e_info.px;
           py           = resume_e_info.py;
           sxoffs       = resume_e_info.sxoffs;
           syoffs       = resume_e_info.syoffs;
           xdots        = resume_e_info.xdots;
           ydots        = resume_e_info.ydots;
           gridsz       = resume_e_info.gridsz;
           this_gen_rseed = resume_e_info.this_gen_rseed;
           fiddlefactor   = resume_e_info.fiddlefactor;
           evolving     = viewwindow = resume_e_info.evolving;
           ecount       = resume_e_info.ecount;
           MemoryRelease(evolve_handle);  /* We're done with it, release it. */
           evolve_handle = 0;
        }
        else { /* not resuming, start from the beginning */
           int mid = gridsz / 2;
           if ((px != mid) || (py != mid)) {
              this_gen_rseed = (unsigned int)clock_ticks(); /* time for new set */
           }
           param_history(0); /* save old history */
           ecount = 0;
           fiddlefactor = fiddlefactor * fiddle_reduction;
           opx = newopx; opy = newopy;
           odpx = newodpx; odpy = newodpy; /*odpx used for discrete parms like
                                             inside, outside, trigfn etc */
        }
        prmboxcount = 0;
        dpx=paramrangex/(gridsz-1);
        dpy=paramrangey/(gridsz-1);
        grout  = !((evolving & NOGROUT)/NOGROUT);
        tmpxdots = xdots+grout;
        tmpydots = ydots+grout;
        gridsqr = gridsz * gridsz;
        while ( ecount < gridsqr ) {
          spiralmap(ecount); /* sets px & py */
          sxoffs = tmpxdots * px;
          syoffs = tmpydots * py;
          param_history(1); /* restore old history */
          fiddleparms(gene, ecount);
          calcfracinit();
          if (calcfract() == -1)
             goto done;
          ecount ++;
        }
done:
        if (ecount == gridsqr) {
           i = 0;
           driver_buzzer(0); /* finished!! */
        }
        else { /* interrupted screen generation, save info */
			/* TODO: MemoryAlloc */
           if (evolve_handle == 0)
              evolve_handle = MemoryAlloc((U16)sizeof(resume_e_info),1L,FARMEM);
           resume_e_info.paramrangex     = paramrangex;
           resume_e_info.paramrangey     = paramrangey;
           resume_e_info.opx             = opx;
           resume_e_info.opy             = opy;
           resume_e_info.odpx            = (short)odpx;
           resume_e_info.odpy            = (short)odpy;
           resume_e_info.px              = (short)px;
           resume_e_info.py              = (short)py;
           resume_e_info.sxoffs          = (short)sxoffs;
           resume_e_info.syoffs          = (short)syoffs;
           resume_e_info.xdots           = (short)xdots;
           resume_e_info.ydots           = (short)ydots;
           resume_e_info.gridsz          = (short)gridsz;
           resume_e_info.this_gen_rseed  = (short)this_gen_rseed;
           resume_e_info.fiddlefactor    = fiddlefactor;
           resume_e_info.evolving        = (short)evolving;
           resume_e_info.ecount          = (short) ecount;
           MoveToMemory((BYTE *)&resume_e_info,(U16)sizeof(resume_e_info),1L,0L,evolve_handle);
        }
        sxoffs = syoffs = 0;
        xdots = sxdots;
        ydots = sydots; /* otherwise save only saves a sub image and boxes get clipped */

        /* set up for 1st selected image, this reuses px and py */
        px = py = gridsz/2;
        unspiralmap(); /* first time called, w/above line sets up array */
        param_history(1); /* restore old history */
        fiddleparms(gene, 0);
        /* now put the gene array back in memory */
        MoveToMemory((BYTE *)&gene, (U16)sizeof(gene), 1L, 0L, gene_handle);
      }
 /* end of evolution loop */
     else {
         i = calcfract();       /* draw the fractal using "C" */
         if (i == 0)
            driver_buzzer(0); /* finished!! */
     }

     saveticks = 0;                 /* turn off autosave timer */
     if (driver_diskp() && i == 0) /* disk-video */
     {
        static char o_msg[] = {"Image has been completed"};
        char msg[sizeof(o_msg)];
        strcpy(msg,o_msg);
        dvid_status(0,msg);
     }
     diskisactive = 0;              /* flag for disk-video routines */
}
#ifndef XFRACT
      boxcount = 0;                     /* no zoom box yet  */
      zwidth = 0;
#else
      if (!XZoomWaiting) {
          boxcount = 0;                 /* no zoom box yet  */
          zwidth = 0;
      }
#endif

      if (fractype == PLASMA && cpu > 88) {
         cyclelimit = 256;              /* plasma clouds need quick spins */
         daccount = 256;
         daclearn = 1;
         }

resumeloop:                             /* return here on failed overlays */

      *kbdmore = 1;
      while (*kbdmore == 1) {           /* loop through command keys */
         if (timedsave != 0) {
            if (timedsave == 1) {       /* woke up for timed save */
               driver_get_key();     /* eat the dummy char */
               kbdchar = 's'; /* do the save */
               resave_flag = 1;
               timedsave = 2;
               }
            else {                      /* save done, resume */
               timedsave = 0;
               resave_flag = 2;
               kbdchar = ENTER;
               }
            }
         else if (initbatch == 0) {     /* not batch mode */
#ifndef XFRACT
            lookatmouse = (zwidth == 0 && !video_scroll) ? -PAGE_UP : 3;
#else
            lookatmouse = (zwidth == 0) ? -PAGE_UP : 3;
#endif
            if (calc_status == 2 && zwidth == 0 && !driver_key_pressed()) {
                  kbdchar = ENTER ;  /* no visible reason to stop, continue */
            } else {     /* wait for a real keystroke */
              if (autobrowse && !no_sub_images) kbdchar = 'l';
               else
               {
               driver_wait_key_pressed(0);
               kbdchar = driver_get_key();
               }
               if (kbdchar == ESC || kbdchar == 'm' || kbdchar == 'M') {
                  if (kbdchar == ESC && escape_exit != 0)
                      /* don't ask, just get out */
                      goodbye();
                  driver_stack_screen();
#ifndef XFRACT
                  kbdchar = main_menu(1);
#else
                  if (XZoomWaiting) {
                      kbdchar = ENTER;
                  } else {
                      kbdchar = main_menu(1);
                      if (XZoomWaiting) {
                          kbdchar = ENTER;
                      }
                  }
#endif
                  if (kbdchar == '\\' || kbdchar == CTL_BACKSLASH ||
                      kbdchar == 'h' || kbdchar == 8 ||
                      check_vidmode_key(0,kbdchar) >= 0)
                     driver_discard_screen();
                  else if (kbdchar == 'x' || kbdchar == 'y' ||
                           kbdchar == 'z' || kbdchar == 'g' ||
                           kbdchar == 'v' || kbdchar == 2 ||
                           kbdchar == 5 || kbdchar == 6)
                     fromtext_flag = 1;
                  else
                     driver_unstack_screen();
                  }
               }
            }
         else {         /* batch mode, fake next keystroke */

/* initbatch == -1  flag to finish calc before save */
/* initbatch == 0   not in batch mode */
/* initbatch == 1   normal batch mode */
/* initbatch == 2   was 1, now do a save */
/* initbatch == 3   bailout with errorlevel == 2, error occurred, no save */
/* initbatch == 4   bailout with errorlevel == 1, interrupted, try to save */
/* initbatch == 5   was 4, now do a save */

            if (initbatch == -1) {      /* finish calc */
               kbdchar = ENTER;
               initbatch = 1;
               }
            else if (initbatch == 1 || initbatch == 4 ) {       /* save-to-disk */
/*
               while(driver_key_pressed())
                 driver_get_key();
*/
               if (debugflag == 50)
                  kbdchar = 'r';
               else
                  kbdchar = 's';
               if(initbatch == 1) initbatch = 2;
               if(initbatch == 4) initbatch = 5;
               }
            else {
               if(calc_status != 4) initbatch = 3; /* bailout with error */
               goodbye();               /* done, exit */
               }
            }

#ifndef XFRACT
         if ('A' <= kbdchar && kbdchar <= 'Z')
            kbdchar = tolower(kbdchar);
#endif
         if (evolving)
            mms_value = evolver_menu_switch(&kbdchar,&frommandel,kbdmore,stacked);
         else
            mms_value = main_menu_switch(&kbdchar,&frommandel,kbdmore,stacked,axmode);
         if (quick_calc && (mms_value == IMAGESTART ||
                            mms_value == RESTORESTART ||
                            mms_value == RESTART)) {
            quick_calc = 0;
            usr_stdcalcmode = old_stdcalcmode;
         }
         if (quick_calc && calc_status != 4)
            usr_stdcalcmode = '1';
         switch(mms_value)
         {
         case IMAGESTART:
            return(IMAGESTART);
         case RESTORESTART:
            return(RESTORESTART);
         case RESTART:
            return(RESTART);
         case CONTINUE:
            continue;
         default:
            break;
         }
         if (zoomoff == 1 && *kbdmore == 1) /* draw/clear a zoom box? */
            drawbox(1);
         if (driver_resize()) {
             calc_status = -1;
         }
         }
      }
/*  return(0); */
}

int main_menu_switch(int *kbdchar, int *frommandel, int *kbdmore, char *stacked, int axmode)
{
   int i,k;
   static double  jxxmin, jxxmax, jyymin, jyymax; /* "Julia mode" entry point */
   static double  jxx3rd, jyy3rd;
   long old_maxit;
   /*
   char drive[FILE_MAX_DRIVE];
   char dir[FILE_MAX_DIR];
   char fname[FILE_MAX_FNAME];
   char ext[FILE_MAX_EXT];
   */
   if (quick_calc && calc_status == 4) {
      quick_calc = 0;
      usr_stdcalcmode = old_stdcalcmode;
   }
   if (quick_calc && calc_status != 4)
      usr_stdcalcmode = old_stdcalcmode;
   switch (*kbdchar)
   {
   case 't':                    /* new fractal type             */
      julibrot = 0;
      clear_zoombox();
      driver_stack_screen();
      if ((i = get_fracttype()) >= 0)
      {
         driver_discard_screen();
         savedac = 0;
         save_release = g_release;
         no_mag_calc = 0;
         use_old_period = 0;
         bad_outside = 0;
         ldcheck = 0;
         set_current_params();
         odpx=odpy=newodpx=newodpy=0;
         fiddlefactor = 1;           /* reset param evolution stuff */
         set_orbit_corners = 0;
         param_history(0); /* save history */
         if (i == 0)
         {
            initmode = adapter;
            *frommandel = 0;
         }
         else if (initmode < 0) /* it is supposed to be... */
            driver_set_for_text();     /* reset to text mode      */
         return(IMAGESTART);
      }
      driver_unstack_screen();
      break;
   case 24:                     /* Ctl-X, Ctl-Y, CTL-Z do flipping */
   case 25:
   case 26:
      flip_image(*kbdchar);
      break;
   case 'x':                    /* invoke options screen        */
   case 'y':
   case 'p':                    /* passes options      */
   case 'z':                    /* type specific parms */
   case 'g':
   case 'v':
   case 2:
   case 5:
   case 6:
      old_maxit = maxit;
      clear_zoombox();
      if (fromtext_flag == 1)
         fromtext_flag = 0;
      else
         driver_stack_screen();
      if (*kbdchar == 'x')
         i = get_toggles();
      else if (*kbdchar == 'y')
         i = get_toggles2();
      else if (*kbdchar == 'p')
         i = passes_options();
      else if (*kbdchar == 'z')
         i = get_fract_params(1);
      else if (*kbdchar == 'v')
         i = get_view_params(); /* get the parameters */
      else if (*kbdchar == 2)
         i = get_browse_params();
      else if (*kbdchar == 5) {
         i = get_evolve_Parms();
         if (i > 0) {
            start_showorbit = 0;
            soundflag &= 0xF9; /* turn off only x,y,z */
            Log_Auto_Calc = 0; /* turn it off */
         }
      }
      else if (*kbdchar == 6)
         i = get_sound_params();
      else
         i = get_cmd_string();
      driver_unstack_screen();
      if (evolving && truecolor)
         truecolor = 0; /* truecolor doesn't play well with the evolver */
      if (maxit > old_maxit && inside >= 0 && calc_status == 4 &&
         curfractalspecific->calctype == StandardFractal && !LogFlag &&
         !truecolor &&    /* recalc not yet implemented with truecolor */
         !(usr_stdcalcmode == 't' && fillcolor > -1) &&
               /* tesseral with fill doesn't work */
         !(usr_stdcalcmode == 'o') &&
         i == 1 && /* nothing else changed */
         outside != ATAN ) {
            quick_calc = 1;
            old_stdcalcmode = usr_stdcalcmode;
            usr_stdcalcmode = '1';
            *kbdmore = 0;
            calc_status = 2;
            i = 0;
         }
      else if (i > 0) {              /* time to redraw? */
         quick_calc = 0;
         param_history(0); /* save history */
         *kbdmore = calc_status = 0;
      }
      break;
#ifndef XFRACT
   case '@':                    /* execute commands */
   case '2':                    /* execute commands */
#else
   case F2:                     /* execute commands */
#endif
      driver_stack_screen();
      i = get_commands();
      if (initmode != -1)
      {                         /* video= was specified */
         adapter = initmode;
         initmode = -1;
         i |= 1;
         savedac = 0;
      }
      else if (colorpreloaded)
      {                         /* colors= was specified */
         spindac(0, 1);
         colorpreloaded = 0;
      }
      else if ((i & 8))         /* reset was specified */
         savedac = 0;
      if ((i & 4))
      {                         /* 3d = was specified */
         *kbdchar = '3';
         driver_unstack_screen();
         goto do_3d_transform;  /* pretend '3' was keyed */
      }
      if ((i & 1))
      {                         /* fractal parameter changed */
         driver_discard_screen();
         /* backwards_v18();*/  /* moved this to cmdfiles.c */
         /* backwards_v19();*/
         *kbdmore = calc_status = 0;
      }
      else
         driver_unstack_screen();
      break;
   case 'f':                    /* floating pt toggle           */
      if (usr_floatflag == 0)
         usr_floatflag = 1;
      else if (stdcalcmode != 'o') /* don't go there */
         usr_floatflag = 0;
      initmode = adapter;
      return(IMAGESTART);
   case 'i':                    /* 3d fractal parms */
      if (get_fract3d_params() >= 0)    /* get the parameters */
         calc_status = *kbdmore = 0;    /* time to redraw */
      break;
#if 0
   case 'w':
      /*chk_keys();*/
      /*julman();*/
      break;
#endif
   case 1:                     /* ^a Ant */
      clear_zoombox();
      {
         int oldtype, err, i;
         double oldparm[MAXPARAMS];
         oldtype = fractype;
         for(i=0;i<MAXPARAMS;i++)
            oldparm[i] = param[i];
         if (fractype != ANT)
         {
            fractype = ANT;
            curfractalspecific = &fractalspecific[fractype];
            load_params(fractype);
         }
         if (!fromtext_flag)
            driver_stack_screen();
         fromtext_flag = 0;
         if ((err = get_fract_params(2)) >= 0)
         {
            driver_unstack_screen();
            if (ant() >= 0)
               calc_status = 0;
         }
         else
            driver_unstack_screen();
         fractype = oldtype;
         for(i=0;i<MAXPARAMS;i++)
            param[i] = oldparm[i];
         if (err >= 0)
            return(CONTINUE);
      }
      break;
   case 'k':                    /* ^s is irritating, give user a single key */
   case 19:                     /* ^s RDS */
      clear_zoombox();
      if (get_rds_params() >= 0)
      {
         if (do_AutoStereo() >= 0)
            calc_status = 0;
         return(CONTINUE);
      }
      break;
   case 'a':                    /* starfield parms               */
      clear_zoombox();
      if (get_starfield_params() >= 0)
      {
         if (starfield() >= 0)
            calc_status = 0;
         return(CONTINUE);
      }
      break;
   case 15:                     /* ctrl-o */
   case 'o':
      /* must use standard fractal and have a float variant */
      if ((fractalspecific[fractype].calctype == StandardFractal
           || fractalspecific[fractype].calctype == calcfroth) &&
          (fractalspecific[fractype].isinteger == 0 ||
           fractalspecific[fractype].tofloat != NOFRACTAL) &&
           !bf_math && /* for now no arbitrary precision support */
           !(istruecolor && truemode) )
      {
         clear_zoombox();
         Jiim(ORBIT);
      }
      break;
   case FIK_SPACE:                  /* spacebar, toggle mand/julia   */
      if(bf_math || evolving)
         break;
      if (fractype == CELLULAR)
      {
         if (nxtscreenflag)
            nxtscreenflag = 0;  /* toggle flag to stop generation */
         else
            nxtscreenflag = 1;  /* toggle flag to generate next screen */
         calc_status = 2;
         *kbdmore = 0;
      }
      else
      {
         if(fractype == FORMULA || fractype == FFORMULA)
         {
            if(ismand)
            {
               fractalspecific[fractype].tojulia = fractype;
               fractalspecific[fractype].tomandel = NOFRACTAL;
               ismand = 0;
            }
            else
            {
               fractalspecific[fractype].tojulia = NOFRACTAL;
               fractalspecific[fractype].tomandel = fractype;
               ismand = 1;
            }
         }
         if (curfractalspecific->tojulia != NOFRACTAL
             && param[0] == 0.0 && param[1] == 0.0)
         {
            /* switch to corresponding Julia set */
            int key;
            if ((fractype == MANDEL || fractype == MANDELFP) && bf_math == 0)
                  hasinverse = 1;
            else
                  hasinverse = 0;
            clear_zoombox();
            Jiim(JIIM);
            key = driver_get_key();    /* flush keyboard buffer */
            if (key != FIK_SPACE)
            {
                  ungetakey(key);
                  break;
            }
            fractype = curfractalspecific->tojulia;
            curfractalspecific = &fractalspecific[fractype];
            if (xcjul == BIG || ycjul == BIG)
            {
               param[0] = (xxmax + xxmin) / 2;
               param[1] = (yymax + yymin) / 2;
            }
            else
            {
               param[0] = xcjul;
               param[1] = ycjul;
               xcjul = ycjul = BIG;
            }
            jxxmin = sxmin;
            jxxmax = sxmax;
            jyymax = symax;
            jyymin = symin;
            jxx3rd = sx3rd;
            jyy3rd = sy3rd;
            *frommandel = 1;
            xxmin = curfractalspecific->xmin;
            xxmax = curfractalspecific->xmax;
            yymin = curfractalspecific->ymin;
            yymax = curfractalspecific->ymax;
            xx3rd = xxmin;
            yy3rd = yymin;
            if (usr_distest == 0 && usr_biomorph != -1 && bitshift != 29)
            {
               xxmin *= 3.0;
               xxmax *= 3.0;
               yymin *= 3.0;
               yymax *= 3.0;
               xx3rd *= 3.0;
               yy3rd *= 3.0;
            }
            zoomoff = 1;
            calc_status = 0;
            *kbdmore = 0;
         }
         else if (curfractalspecific->tomandel != NOFRACTAL)
         {
            /* switch to corresponding Mandel set */
            fractype = curfractalspecific->tomandel;
            curfractalspecific = &fractalspecific[fractype];
            if (*frommandel)
            {
               xxmin = jxxmin;
               xxmax = jxxmax;
               yymin = jyymin;
               yymax = jyymax;
               xx3rd = jxx3rd;
               yy3rd = jyy3rd;
            }
            else
            {
               xxmin = xx3rd = curfractalspecific->xmin;
               xxmax = curfractalspecific->xmax;
               yymin = yy3rd = curfractalspecific->ymin;
               yymax = curfractalspecific->ymax;
            }
            SaveC.x = param[0];
            SaveC.y = param[1];
            param[0] = 0;
            param[1] = 0;
            zoomoff = 1;
            calc_status = 0;
            *kbdmore = 0;
         }
         else
            driver_buzzer(2);          /* can't switch */
      }                         /* end of else for if == cellular */
      break;
   case 'j':                    /* inverse julia toggle */
      /* if the inverse types proliferate, something more elegant will be
       * needed */
      if (fractype == JULIA || fractype == JULIAFP || fractype == INVERSEJULIA)
      {
         static int oldtype = -1;
         if (fractype == JULIA || fractype == JULIAFP)
         {
            oldtype = fractype;
            fractype = INVERSEJULIA;
         }
         else if (fractype == INVERSEJULIA)
         {
            if (oldtype != -1)
               fractype = oldtype;
            else
               fractype = JULIA;
         }
         curfractalspecific = &fractalspecific[fractype];
         zoomoff = 1;
         calc_status = 0;
         *kbdmore = 0;
      }
#if 0
      else if (fractype == MANDEL || fractype == MANDELFP)
      {
         clear_zoombox();
         Jiim(JIIM);
      }
#endif
      else
         driver_buzzer(2);
      break;
   case '\\':                   /* return to prev image    */
   case CTL_BACKSLASH:
   case 'h':
   case 8:
      if (name_stack_ptr >= 1)
      {
         /* go back one file if somewhere to go (ie. browsing) */
         name_stack_ptr--;
         while (file_name_stack[name_stack_ptr][0] == '\0' 
                && name_stack_ptr >= 0)
            name_stack_ptr--;
         if (name_stack_ptr < 0) /* oops, must have deleted first one */
            break;
         strcpy(browsename, file_name_stack[name_stack_ptr]);
         /*
         splitpath(browsename, NULL, NULL, fname, ext);
         splitpath(readname, drive, dir, NULL, NULL);
         makepath(readname, drive, dir, fname, ext);
         */
         merge_pathnames(readname,browsename,2);
         browsing = TRUE;
         no_sub_images = FALSE;
         showfile = 0;
         if (askvideo)
         {
            driver_stack_screen();      /* save graphics image */
            *stacked = 1;
         }
         return(RESTORESTART);
      }
      else if(maxhistory > 0 && bf_math == 0)
      {
         if(*kbdchar == '\\' || *kbdchar == 'h')
            if (--historyptr < 0)
               historyptr = maxhistory - 1;
         if(*kbdchar == CTL_BACKSLASH || *kbdchar == 8)
            if (++historyptr >= maxhistory)
               historyptr = 0;
         restore_history_info(historyptr);
         zoomoff = 1;
         initmode = adapter;
         if (curfractalspecific->isinteger != 0 &&
             curfractalspecific->tofloat != NOFRACTAL)
            usr_floatflag = 0;
         if (curfractalspecific->isinteger == 0 &&
             curfractalspecific->tofloat != NOFRACTAL)
            usr_floatflag = 1;
         historyflag = 1;       /* avoid re-store parms due to rounding errs */
         return(IMAGESTART);
      }
      break;
   case 'd':                    /* shell to MS-DOS              */
#ifndef XFRACT
      driver_stack_screen();
      if (75000L > fr_farfree()) {
         static char dosmsg[] = {"Not enough memory to Shell-to-DOS"};
         driver_unstack_screen();
         stopmsg(0, dosmsg);
         break;
      }
      if (axmode == 0 || axmode > 7)
      {
         static char dosmsg[] =
         {"\
Note:  Your graphics image is still squirreled away in your video\n\
adapter's memory.  Switching video modes will clobber part of that\n\
image.  Sorry - it's the best we could do."};
         driver_put_string(0, 0, 7, dosmsg);
         driver_move_cursor(6, 0);
      }
      driver_shell();
      driver_unstack_screen();
#else
      setclear();
      printf("\n\nShelling to Linux/Unix - type 'exit' to return\n\n");
      driver_shell();
      putprompt();
      return(CONTINUE);
#endif
/*             calc_status = 0; */
      break;
   case 'c':                    /* switch to color cycling      */
   case '+':                    /* rotate palette               */
   case '-':                    /* rotate palette               */
      clear_zoombox();
      memcpy(olddacbox, g_dacbox, 256 * 3);
      rotate((*kbdchar == 'c') ? 0 : ((*kbdchar == '+') ? 1 : -1));
      if (memcmp(olddacbox, g_dacbox, 256 * 3))
      {
         colorstate = 1;
         save_history_info();
      }
      return(CONTINUE);
   case 'e':                    /* switch to color editing      */
      if (istruecolor && !initbatch) { /* don't enter palette editor */
         if (load_palette() >= 0) {
            *kbdmore = calc_status = 0;
            break;
         } else
            return(CONTINUE);
      }
      clear_zoombox();
      if (g_dacbox[0][0] != 255 && !reallyega && colors >= 16
          && !driver_diskp())
      {
         int oldhelpmode;
         oldhelpmode = helpmode;
         memcpy(olddacbox, g_dacbox, 256 * 3);
         helpmode = HELPXHAIR;
         EditPalette();
         helpmode = oldhelpmode;
         if (memcmp(olddacbox, g_dacbox, 256 * 3))
         {
            colorstate = 1;
            save_history_info();
         }
      }
      return(CONTINUE);
   case 's':                    /* save-to-disk                 */
      if (driver_diskp() && disktarga == 1)
         return(CONTINUE);  /* disk video and targa, nothing to save */
      diskisactive = 1;         /* flag for disk-video routines */
      note_zoom();
      savetodisk(savename);
      restore_zoom();
      diskisactive = 0;         /* flag for disk-video routines */
      return(CONTINUE);
   case '#':                    /* 3D overlay                   */
#ifdef XFRACT
   case F3:                     /* 3D overlay                   */
#endif
      clear_zoombox();
      overlay3d = 1;
   case '3':                    /* restore-from (3d)            */
    do_3d_transform:
      if (overlay3d)
         display3d = 2;         /* for <b> command               */
      else
         display3d = 1;
   case 'r':                    /* restore-from                 */
      comparegif = 0;
      *frommandel = 0;
      if (browsing)
      {
         browsing = FALSE;
      }
      if (*kbdchar == 'r')
      {
         if (debugflag == 50)
         {
            comparegif = overlay3d = 1;
            if (initbatch == 2)
            {
               driver_stack_screen();   /* save graphics image */
               strcpy(readname, savename);
               showfile = 0;
               return(RESTORESTART);
            }
         }
         else
            comparegif = overlay3d = 0;
         display3d = 0;
      }
      driver_stack_screen();            /* save graphics image */
      if (overlay3d)
         *stacked = 0;
      else
         *stacked = 1;
      if (resave_flag)
      {
         updatesavename(savename);      /* do the pending increment */
         resave_flag = started_resaves = 0;
      }
      showfile = -1;
      return(RESTORESTART);
   case 'l':
   case 'L':                    /* Look for other files within this view */
      if ((zwidth == 0) && (!diskvideo))
         /* not zooming & no disk video */
      {
         int oldhelpmode;
         oldhelpmode = helpmode;
         helpmode = HELPBROWSE;
         switch (fgetwindow())
         {
         case ENTER:
         case ENTER_2:
            showfile = 0;       /* trigger load */
            browsing = TRUE;    /* but don't ask for the file name as it's
                                 * just been selected */
            if (name_stack_ptr == 15)
            {                   /* about to run off the end of the file
                                 * history stack so shift it all back one to
                                 * make room, lose the 1st one */
               int tmp;
               for (tmp = 1; tmp < 16; tmp++)
                  strcpy(file_name_stack[tmp - 1], file_name_stack[tmp]);
               name_stack_ptr = 14;
            }
            name_stack_ptr++;
            strcpy(file_name_stack[name_stack_ptr], browsename);
            /*
            splitpath(browsename, NULL, NULL, fname, ext);
            splitpath(readname, drive, dir, NULL, NULL);
            makepath(readname, drive, dir, fname, ext);
            */
            merge_pathnames(readname,browsename,2);
            if (askvideo)
            {
               driver_stack_screen();   /* save graphics image */
               *stacked = 1;
            }
            return(RESTORESTART);       /* hop off and do it!! */
         case '\\':
            if (name_stack_ptr >= 1)
            {
               /* go back one file if somewhere to go (ie. browsing) */
               name_stack_ptr--;
               while (file_name_stack[name_stack_ptr][0] == '\0' 
                      && name_stack_ptr >= 0)
                  name_stack_ptr--;
               if (name_stack_ptr < 0) /* oops, must have deleted first one */
                  break;
               strcpy(browsename, file_name_stack[name_stack_ptr]);
               /*
               splitpath(browsename, NULL, NULL, fname, ext);
               splitpath(readname, drive, dir, NULL, NULL);
               makepath(readname, drive, dir, fname, ext);
               */
               merge_pathnames(readname,browsename,2);
               browsing = TRUE;
               showfile = 0;
               if (askvideo)
               {
                  driver_stack_screen();/* save graphics image */
                  *stacked = 1;
               }
               return(RESTORESTART);
            }                   /* otherwise fall through and turn off
                                 * browsing */
         case ESC:
         case 'l':              /* turn it off */
         case 'L':
            browsing = FALSE;
            helpmode = oldhelpmode;
            break;
         case 's':
            browsing = FALSE;
            helpmode = oldhelpmode;
            savetodisk(savename);
            break;
         default:               /* or no files found, leave the state of
                                 * browsing */
            break;              /* alone */
         }
      }
      else
      {
         browsing = FALSE;
         driver_buzzer(2);             /* can't browse if zooming or diskvideo */
      }
      break;
   case 'b':                    /* make batch file              */
      make_batch_file();
      break;
   case 16:                    /* print current image          */
      note_zoom();
      Print_Screen();
      restore_zoom();
      if (!driver_key_pressed())
         driver_buzzer(0);
      else
      {
         driver_buzzer(1);
         driver_get_key();
      }
      return(CONTINUE);
   case ENTER:                  /* Enter                        */
   case ENTER_2:                /* Numeric-Keypad Enter         */
#ifdef XFRACT
      XZoomWaiting = 0;
#endif
      if (zwidth != 0.0)
      {                         /* do a zoom */
         init_pan_or_recalc(0);
         *kbdmore = 0;
      }
      if (calc_status != 4)     /* don't restart if image complete */
         *kbdmore = 0;
      break;
   case CTL_ENTER:              /* control-Enter                */
   case CTL_ENTER_2:            /* Control-Keypad Enter         */
      init_pan_or_recalc(1);
      *kbdmore = 0;
      zoomout();                /* calc corners for zooming out */
      break;
   case INSERT:         /* insert                       */
      driver_set_for_text();           /* force text mode */
      return(RESTART);
   case LEFT_ARROW:             /* cursor left                  */
   case RIGHT_ARROW:            /* cursor right                 */
   case UP_ARROW:               /* cursor up                    */
   case DOWN_ARROW:             /* cursor down                  */
        move_zoombox(*kbdchar);
        break;
   case LEFT_ARROW_2:           /* Ctrl-cursor left             */
   case RIGHT_ARROW_2:          /* Ctrl-cursor right            */
   case UP_ARROW_2:             /* Ctrl-cursor up               */
   case DOWN_ARROW_2:           /* Ctrl-cursor down             */
       move_zoombox(*kbdchar);
       break;
   case CTL_HOME:               /* Ctrl-home                    */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
      {
         i = key_count(CTL_HOME);
         if ((zskew -= 0.02 * i) < -0.48)
            zskew = -0.48;
      }
      break;
   case CTL_END:                /* Ctrl-end                     */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
      {
         i = key_count(CTL_END);
         if ((zskew += 0.02 * i) > 0.48)
            zskew = 0.48;
      }
      break;
   case CTL_PAGE_UP:            /* Ctrl-pgup                    */
      if (boxcount)
         chgboxi(0, -2 * key_count(CTL_PAGE_UP));
      break;
   case CTL_PAGE_DOWN:          /* Ctrl-pgdn                    */
      if (boxcount)
         chgboxi(0, 2 * key_count(CTL_PAGE_DOWN));
      break;

   case PAGE_UP:                /* page up                      */
      if (zoomoff == 1)
      {
         if (zwidth == 0)
         {                      /* start zoombox */
            zwidth = zdepth = 1;
            zskew = zrotate = 0;
            zbx = zby = 0;
            find_special_colors();
            boxcolor = color_bright;
            px = py = gridsz/2;
            moveboxf(0.0,0.0); /* force scrolling */
         }
         else
            resizebox(0 - key_count(PAGE_UP));
      }
      break;
   case PAGE_DOWN:              /* page down                    */
      if (boxcount)
      {
         if (zwidth >= .999 && zdepth >= 0.999) /* end zoombox */
            zwidth = 0;
         else
            resizebox(key_count(PAGE_DOWN));
      }
      break;
   case CTL_MINUS:              /* Ctrl-kpad-                  */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
         zrotate += key_count(CTL_MINUS);
      break;
   case CTL_PLUS:               /* Ctrl-kpad+               */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
         zrotate -= key_count(CTL_PLUS);
      break;
   case CTL_INSERT:             /* Ctrl-ins                 */
      boxcolor += key_count(CTL_INSERT);
      break;
   case CTL_DEL:                /* Ctrl-del                 */
      boxcolor -= key_count(CTL_DEL);
      break;

   case 1120: /* alt + number keys set mutation level and start evolution engine */
   case 1121: 
   case 1122: 
   case 1123:
   case 1124:
   case 1125:
   case 1126:
/*
   case 1127:
   case 1128:
*/
      viewwindow = evolving = 1;
      set_mutation_level(*kbdchar-1119);
      param_history(0); /* save parameter history */
      *kbdmore = calc_status = 0;
      break;

   case FIK_DELETE:         /* select video mode from list */
   {
      driver_stack_screen();
      *kbdchar = select_video_mode(adapter);
      if (check_vidmode_key(0, *kbdchar) >= 0)  /* picked a new mode? */
         driver_discard_screen();
      else
         driver_unstack_screen();
      /* fall through */
   }
   default:                     /* other (maybe a valid Fn key) */
      if ((k = check_vidmode_key(0, *kbdchar)) >= 0)
      {
         adapter = k;
/*                if (videotable[adapter].dotmode != 11       Took out so that */
/*                  || videotable[adapter].colors != colors)  DAC is not reset */
/*                   savedac = 0;                    when changing video modes */
         if (videotable[adapter].colors != colors)
            savedac = 0;
         calc_status = 0;
         *kbdmore = 0;
         return(CONTINUE);
      }
      break;
   }                            /* end of the big switch */
   return(0);
}

int evolver_menu_switch(int *kbdchar, int *frommandel, int *kbdmore, char *stacked)
{
   int i,k;

   switch (*kbdchar)
   {
   case 't':                    /* new fractal type             */
     julibrot = 0;
     clear_zoombox();
     driver_stack_screen();
      if ((i = get_fracttype()) >= 0)
      {
         driver_discard_screen();
         savedac = 0;
         save_release = g_release;
         no_mag_calc = 0;
         use_old_period = 0;
         bad_outside = 0;
         ldcheck = 0;
         set_current_params();
         odpx=odpy=newodpx=newodpy=0;
         fiddlefactor = 1;           /* reset param evolution stuff */
         set_orbit_corners = 0;
         param_history(0); /* save history */
         if (i == 0)
         {
            initmode = adapter;
            *frommandel = 0;
         }
         else if (initmode < 0) /* it is supposed to be... */
            driver_set_for_text();     /* reset to text mode      */
         return(IMAGESTART);
      }
      driver_unstack_screen();
      break;
   case 'x':                    /* invoke options screen        */
   case 'y':
   case 'p':                    /* passes options      */
   case 'z':                    /* type specific parms */
   case 'g':
   case 5:
   case FIK_SPACE:
      clear_zoombox();
      if (fromtext_flag == 1)
         fromtext_flag = 0;
      else
         driver_stack_screen();
      if (*kbdchar == 'x')
         i = get_toggles();
      else if (*kbdchar == 'y')
         i = get_toggles2();
      else if (*kbdchar == 'p')
         i = passes_options();
      else if (*kbdchar == 'z')
         i = get_fract_params(1);
      else if (*kbdchar == 5 || *kbdchar == FIK_SPACE)
         i = get_evolve_Parms();
      else
         i = get_cmd_string();
      driver_unstack_screen();
      if (evolving && truecolor)
         truecolor = 0; /* truecolor doesn't play well with the evolver */
      if (i > 0) {              /* time to redraw? */
         param_history(0); /* save history */
         *kbdmore = calc_status = 0;
      }
      break;
   case 'b': /* quick exit from evolve mode */
      evolving = viewwindow = 0;
      param_history(0); /* save history */
      *kbdmore = calc_status = 0;
      break;

   case 'f':                    /* floating pt toggle           */
      if (usr_floatflag == 0)
         usr_floatflag = 1;
      else if (stdcalcmode != 'o') /* don't go there */
         usr_floatflag = 0;
      initmode = adapter;
      return(IMAGESTART);
   case '\\':                   /* return to prev image    */
   case CTL_BACKSLASH:
   case 'h':
   case 8:
      if(maxhistory > 0 && bf_math == 0)
      {
         if(*kbdchar == '\\' || *kbdchar == 'h')
            if (--historyptr < 0)
               historyptr = maxhistory - 1;
         if(*kbdchar == CTL_BACKSLASH || *kbdchar == 8)
            if (++historyptr >= maxhistory)
               historyptr = 0;
         restore_history_info(historyptr);
         zoomoff = 1;
         initmode = adapter;
         if (curfractalspecific->isinteger != 0 &&
             curfractalspecific->tofloat != NOFRACTAL)
            usr_floatflag = 0;
         if (curfractalspecific->isinteger == 0 &&
             curfractalspecific->tofloat != NOFRACTAL)
            usr_floatflag = 1;
         historyflag = 1;       /* avoid re-store parms due to rounding errs */
         return(IMAGESTART);
      }
      break;
   case 'c':                    /* switch to color cycling      */
   case '+':                    /* rotate palette               */
   case '-':                    /* rotate palette               */
      clear_zoombox();
      memcpy(olddacbox, g_dacbox, 256 * 3);
      rotate((*kbdchar == 'c') ? 0 : ((*kbdchar == '+') ? 1 : -1));
      if (memcmp(olddacbox, g_dacbox, 256 * 3))
      {
         colorstate = 1;
         save_history_info();
      }
      return(CONTINUE);
   case 'e':                    /* switch to color editing      */
      if (istruecolor && !initbatch) { /* don't enter palette editor */
         if (load_palette() >= 0) {
            *kbdmore = calc_status = 0;
            break;
         } else
            return(CONTINUE);
      }
      clear_zoombox();
      if (g_dacbox[0][0] != 255 && !reallyega && colors >= 16
          && !driver_diskp())
      {
         int oldhelpmode;
         oldhelpmode = helpmode;
         memcpy(olddacbox, g_dacbox, 256 * 3);
         helpmode = HELPXHAIR;
         EditPalette();
         helpmode = oldhelpmode;
         if (memcmp(olddacbox, g_dacbox, 256 * 3))
         {
            colorstate = 1;
            save_history_info();
         }
      }
      return(CONTINUE);
   case 's':                    /* save-to-disk                 */
{     int oldsxoffs, oldsyoffs, oldxdots, oldydots, oldpx, oldpy;
      GENEBASE gene[NUMGENES];

      if (driver_diskp() && disktarga == 1)
         return(CONTINUE);  /* disk video and targa, nothing to save */
      /* get the gene array from memory */
      MoveFromMemory((BYTE *)&gene, (U16)sizeof(gene), 1L, 0L, gene_handle);
      oldsxoffs = sxoffs;
      oldsyoffs = syoffs;
      oldxdots = xdots;
      oldydots = ydots;
      oldpx = px;
      oldpy = py;
      sxoffs = syoffs = 0;
      xdots = sxdots;
      ydots = sydots; /* for full screen save and pointer move stuff */
      px = py = gridsz / 2;
      diskisactive = 1;         /* flag for disk-video routines */
      param_history(1); /* restore old history */
      fiddleparms(gene, 0);
      drawparmbox(1);
      savetodisk(savename);
      px = oldpx;
      py = oldpy;
      param_history(1); /* restore old history */
      fiddleparms(gene, unspiralmap());
      diskisactive = 0;         /* flag for disk-video routines */
      sxoffs = oldsxoffs;
      syoffs = oldsyoffs;
      xdots = oldxdots;
      ydots = oldydots;
      MoveToMemory((BYTE *)&gene, (U16)sizeof(gene), 1L, 0L, gene_handle);
}
      return(CONTINUE);
   case 'r':                    /* restore-from                 */
      comparegif = 0;
      *frommandel = 0;
      if (browsing)
      {
         browsing = FALSE;
      }
      if (*kbdchar == 'r')
      {
         if (debugflag == 50)
         {
            comparegif = overlay3d = 1;
            if (initbatch == 2)
            {
               driver_stack_screen();   /* save graphics image */
               strcpy(readname, savename);
               showfile = 0;
               return(RESTORESTART);
            }
         }
         else
            comparegif = overlay3d = 0;
         display3d = 0;
      }
      driver_stack_screen();            /* save graphics image */
      if (overlay3d)
         *stacked = 0;
      else
         *stacked = 1;
      if (resave_flag)
      {
         updatesavename(savename);      /* do the pending increment */
         resave_flag = started_resaves = 0;
      }
      showfile = -1;
      return(RESTORESTART);
   case ENTER:                  /* Enter                        */
   case ENTER_2:                /* Numeric-Keypad Enter         */
#ifdef XFRACT
      XZoomWaiting = 0;
#endif
      if (zwidth != 0.0)
      {                         /* do a zoom */
         init_pan_or_recalc(0);
         *kbdmore = 0;
      }
      if (calc_status != 4)     /* don't restart if image complete */
         *kbdmore = 0;
      break;
   case CTL_ENTER:              /* control-Enter                */
   case CTL_ENTER_2:            /* Control-Keypad Enter         */
      init_pan_or_recalc(1);
      *kbdmore = 0;
      zoomout();                /* calc corners for zooming out */
      break;
   case INSERT:         /* insert                       */
      driver_set_for_text();           /* force text mode */
      return(RESTART);
   case LEFT_ARROW:             /* cursor left                  */
   case RIGHT_ARROW:            /* cursor right                 */
   case UP_ARROW:               /* cursor up                    */
   case DOWN_ARROW:             /* cursor down                  */
        move_zoombox(*kbdchar);
        break;
   case LEFT_ARROW_2:           /* Ctrl-cursor left             */
   case RIGHT_ARROW_2:          /* Ctrl-cursor right            */
   case UP_ARROW_2:             /* Ctrl-cursor up               */
   case DOWN_ARROW_2:           /* Ctrl-cursor down             */
        /* borrow ctrl cursor keys for moving selection box */
        /* in evolver mode */
     if (boxcount) {
        int grout;
        GENEBASE gene[NUMGENES];
        /* get the gene array from memory */
        MoveFromMemory((BYTE *)&gene, (U16)sizeof(gene), 1L, 0L, gene_handle);
        if (evolving&1) {
             if (*kbdchar == LEFT_ARROW_2) {
                px--;
             }
             if (*kbdchar == RIGHT_ARROW_2) {
                px++;
             }
             if (*kbdchar == UP_ARROW_2) {
                py--;
             }
             if (*kbdchar == DOWN_ARROW_2) {
                py++;
             }
             if (px <0 ) px = gridsz-1;
             if (px >(gridsz-1)) px = 0;
             if (py <0) py = gridsz-1;
             if (py > (gridsz-1)) py = 0;
             grout = !((evolving & NOGROUT)/NOGROUT) ;
             sxoffs = px * (int)(dxsize+1+grout);
             syoffs = py * (int)(dysize+1+grout);

             param_history(1); /* restore old history */
             fiddleparms(gene, unspiralmap()); /* change all parameters */
                         /* to values appropriate to the image selected */
             set_evolve_ranges();
             chgboxi(0,0);
             drawparmbox(0);
        }
        /* now put the gene array back in memory */
        MoveToMemory((BYTE *)&gene, (U16)sizeof(gene), 1L, 0L, gene_handle);
     }
     else                       /* if no zoombox, scroll by arrows */
        move_zoombox(*kbdchar);
     break;
   case CTL_HOME:               /* Ctrl-home                    */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
      {
         i = key_count(CTL_HOME);
         if ((zskew -= 0.02 * i) < -0.48)
            zskew = -0.48;
      }
      break;
   case CTL_END:                /* Ctrl-end                     */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
      {
         i = key_count(CTL_END);
         if ((zskew += 0.02 * i) > 0.48)
            zskew = 0.48;
      }
      break;
   case CTL_PAGE_UP:
      if(prmboxcount) {
        parmzoom -= 1.0;
        if(parmzoom<1.0) parmzoom=1.0;
        drawparmbox(0);
        set_evolve_ranges();
      }
      break;
   case CTL_PAGE_DOWN:
      if(prmboxcount) {
        parmzoom += 1.0;
        if(parmzoom>(double)gridsz/2.0) parmzoom=(double)gridsz/2.0;
        drawparmbox(0);
        set_evolve_ranges();
      }
      break;

   case PAGE_UP:                /* page up                      */
      if (zoomoff == 1)
      {
         if (zwidth == 0)
         {                      /* start zoombox */
            zwidth = zdepth = 1;
            zskew = zrotate = 0;
            zbx = zby = 0;
            find_special_colors();
            boxcolor = color_bright;
     /*rb*/ if (evolving&1) {
              /* set screen view params back (previously changed to allow
                          full screen saves in viewwindow mode) */
                   int grout = !((evolving & NOGROUT) / NOGROUT);
                   sxoffs = px * (int)(dxsize+1+grout);
                   syoffs = py * (int)(dysize+1+grout);
                   SetupParamBox();
                   drawparmbox(0);
            }
            moveboxf(0.0,0.0); /* force scrolling */
         }
         else
            resizebox(0 - key_count(PAGE_UP));
      }
      break;
   case PAGE_DOWN:              /* page down                    */
      if (boxcount)
      {
         if (zwidth >= .999 && zdepth >= 0.999) { /* end zoombox */
            zwidth = 0;
            if (evolving&1) {
               drawparmbox(1); /* clear boxes off screen */
               ReleaseParamBox();
            }
         }
         else
            resizebox(key_count(PAGE_DOWN));
      }
      break;
   case CTL_MINUS:              /* Ctrl-kpad-                  */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
         zrotate += key_count(CTL_MINUS);
      break;
   case CTL_PLUS:               /* Ctrl-kpad+               */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
         zrotate -= key_count(CTL_PLUS);
      break;
   case CTL_INSERT:             /* Ctrl-ins                 */
      boxcolor += key_count(CTL_INSERT);
      break;
   case CTL_DEL:                /* Ctrl-del                 */
      boxcolor -= key_count(CTL_DEL);
      break;

   /* grabbed a couple of video mode keys, user can change to these using
       delete and the menu if necessary */

   case F2: /* halve mutation params and regen */
      fiddlefactor = fiddlefactor / 2;
      paramrangex = paramrangex / 2;
      newopx = opx + paramrangex / 2;
      paramrangey = paramrangey / 2;
      newopy = opy + paramrangey / 2;
      *kbdmore = calc_status = 0;
      break;

   case F3: /*double mutation parameters and regenerate */
   {
    double centerx, centery;
      fiddlefactor = fiddlefactor * 2;
      centerx = opx + paramrangex / 2;
      paramrangex = paramrangex * 2;
      newopx = centerx - paramrangex / 2;
      centery = opy + paramrangey / 2;
      paramrangey = paramrangey * 2;
      newopy = centery - paramrangey / 2;
      *kbdmore = calc_status = 0;
      break;
   }

   case F4: /*decrement  gridsize and regen */
      if (gridsz > 3) {
        gridsz = gridsz - 2;  /* gridsz must have odd value only */
        *kbdmore = calc_status = 0;
        }
      break;

   case F5: /* increment gridsize and regen */
      if (gridsz < (sxdots / (MINPIXELS<<1))) {
         gridsz = gridsz + 2;
         *kbdmore = calc_status = 0;
         }
      break;

   case F6: /* toggle all variables selected for random variation to
               center weighted variation and vice versa */
         {
          int i;
          GENEBASE gene[NUMGENES];
          /* get the gene array from memory */
          MoveFromMemory((BYTE *)&gene, (U16)sizeof(gene), 1L, 0L, gene_handle);
          for (i =0;i < NUMGENES; i++) {
            if (gene[i].mutate == 5) {
               gene[i].mutate = 6;
               continue;
            }
            if (gene[i].mutate == 6) gene[i].mutate = 5;
          }
          /* now put the gene array back in memory */
          MoveToMemory((BYTE *)&gene, (U16)sizeof(gene), 1L, 0L, gene_handle);
        }
      *kbdmore = calc_status = 0;
      break;

   case 1120: /* alt + number keys set mutation level */
   case 1121: 
   case 1122: 
   case 1123:
   case 1124:
   case 1125:
   case 1126:
/*
   case 1127:
   case 1128:
*/
      set_mutation_level(*kbdchar-1119);
      param_history(1); /* restore old history */
      *kbdmore = calc_status = 0;
      break;

   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
/*  add these in when more parameters can be varied
   case '8':
   case '9':
*/
      set_mutation_level(*kbdchar-(int)'0');
      param_history(1); /* restore old history */
      *kbdmore = calc_status = 0;
      break;
   case '0': /* mutation level 0 == turn off evolving */ 
      evolving = viewwindow = 0;
      *kbdmore = calc_status = 0;
      break;

   case FIK_DELETE:         /* select video mode from list */
      driver_stack_screen();
      *kbdchar = select_video_mode(adapter);
      if (check_vidmode_key(0, *kbdchar) >= 0)  /* picked a new mode? */
         driver_discard_screen();
      else
         driver_unstack_screen();
      /* fall through */
   default:             /* other (maybe valid Fn key */
      if ((k = check_vidmode_key(0, *kbdchar)) >= 0)
      {
         adapter = k;
         if (videotable[adapter].colors != colors)
            savedac = 0;
         calc_status = 0;
         *kbdmore = 0;
         return(CONTINUE);
      }
      break;
   }                            /* end of the big evolver switch */
   return(0);
}

static int call_line3d(BYTE *pixels, int linelen)
{
   /* this routine exists because line3d might be in an overlay */
   return(line3d(pixels,linelen));
}

static void note_zoom()
{
   if (boxcount) { /* save zoombox stuff in mem before encode (mem reused) */
      if ((savezoom = (char *)malloc((long)(5*boxcount))) == NULL)
         clear_zoombox(); /* not enuf mem so clear the box */
      else {
         reset_zoom_corners(); /* reset these to overall image, not box */
         memcpy(savezoom,boxx,boxcount*2);
         memcpy(savezoom+boxcount*2,boxy,boxcount*2);
         memcpy(savezoom+boxcount*4,boxvalues,boxcount);
         }
      }
}

static void restore_zoom()
{
   if (boxcount) { /* restore zoombox arrays */
      memcpy(boxx,savezoom,boxcount*2);
      memcpy(boxy,savezoom+boxcount*2,boxcount*2);
      memcpy(boxvalues,savezoom+boxcount*4,boxcount);
      free(savezoom);
      drawbox(1); /* get the xxmin etc variables recalc'd by redisplaying */
      }
}

/* do all pending movement at once for smooth mouse diagonal moves */
static void move_zoombox(int keynum)
{  int vertical, horizontal, getmore;
   vertical = horizontal = 0;
   getmore = 1;
   while (getmore) {
      switch (keynum) {
         case LEFT_ARROW:               /* cursor left */
            --horizontal;
            break;
         case RIGHT_ARROW:              /* cursor right */
            ++horizontal;
            break;
         case UP_ARROW:                 /* cursor up */
            --vertical;
            break;
         case DOWN_ARROW:               /* cursor down */
            ++vertical;
            break;
         case LEFT_ARROW_2:             /* Ctrl-cursor left */
            horizontal -= 8;
            break;
         case RIGHT_ARROW_2:             /* Ctrl-cursor right */
            horizontal += 8;
            break;
         case UP_ARROW_2:               /* Ctrl-cursor up */
            vertical -= 8;
            break;
         case DOWN_ARROW_2:             /* Ctrl-cursor down */
            vertical += 8;
            break;                      /* += 8 needed by VESA scrolling */
         default:
            getmore = 0;
         }
      if (getmore) {
         if (getmore == 2)              /* eat last key used */
            driver_get_key();
         getmore = 2;
         keynum = driver_key_pressed();         /* next pending key */
         }
      }
   if (boxcount) {
/*
      if (horizontal != 0)
         moveboxf((double)horizontal/dxsize,0.0);
      if (vertical != 0)
         moveboxf(0.0,(double)vertical/dysize);
*/
      moveboxf((double)horizontal/dxsize,(double)vertical/dysize);
      }
#ifndef XFRACT
   else                                 /* if no zoombox, scroll by arrows */
      scroll_relative(horizontal,vertical);
#endif
}

/* displays differences between current image file and new image */
static FILE *cmp_fp;
static int errcount;
int cmp_line(BYTE *pixels, int linelen)
{
   int row,col;
   int oldcolor;
   if((row = rowcount++) == 0) {
      errcount = 0;
      cmp_fp = dir_fopen(workdir,"cmperr",(initbatch)?"a":"w");
      outln_cleanup = cmp_line_cleanup;
      }
   if(pot16bit) { /* 16 bit info, ignore odd numbered rows */
      if((row & 1) != 0) return(0);
      row >>= 1;
      }
   for(col=0;col<linelen;col++) {
      oldcolor=getcolor(col,row);
      if(oldcolor==(int)pixels[col])
         putcolor(col,row,0);
      else {
         if(oldcolor==0)
            putcolor(col,row,1);
         ++errcount;
         if(initbatch == 0)
            fprintf(cmp_fp,"#%5d col %3d row %3d old %3d new %3d\n",
               errcount,col,row,oldcolor,pixels[col]);
         }
      }
   return(0);
}

static void cmp_line_cleanup(void)
{
   char *timestring;
   time_t ltime;
   if(initbatch) {
      time(&ltime);
      timestring = ctime(&ltime);
      timestring[24] = 0; /*clobber newline in time string */
      fprintf(cmp_fp,"%s compare to %s has %5d errs\n",
                     timestring,readname,errcount);
      }
   fclose(cmp_fp);
}

void clear_zoombox()
{
   zwidth = 0;
   drawbox(0);
   reset_zoom_corners();
}

void reset_zoom_corners()
{
   xxmin = sxmin;
   xxmax = sxmax;
   xx3rd = sx3rd;
   yymax = symax;
   yymin = symin;
   yy3rd = sy3rd;
   if(bf_math)
   {
      copy_bf(bfxmin,bfsxmin);
      copy_bf(bfxmax,bfsxmax);
      copy_bf(bfymin,bfsymin);
      copy_bf(bfymax,bfsymax);
      copy_bf(bfx3rd,bfsx3rd);
      copy_bf(bfy3rd,bfsy3rd);
   }
}

/*
   Function setup287code is called by main() when a 287
   or better fpu is detected.
*/
#define ORBPTR(x) fractalspecific[x].orbitcalc
void setup287code()
{
   ORBPTR(MANDELFP)       = ORBPTR(JULIAFP)      = FJuliafpFractal;
   ORBPTR(BARNSLEYM1FP)   = ORBPTR(BARNSLEYJ1FP) = FBarnsley1FPFractal;
   ORBPTR(BARNSLEYM2FP)   = ORBPTR(BARNSLEYJ2FP) = FBarnsley2FPFractal;
   ORBPTR(MANOWARFP)      = ORBPTR(MANOWARJFP)   = FManOWarfpFractal;
   ORBPTR(MANDELLAMBDAFP) = ORBPTR(LAMBDAFP)     = FLambdaFPFractal;
}

/* read keystrokes while = specified key, return 1+count;       */
/* used to catch up when moving zoombox is slower than keyboard */
int key_count(int keynum)
{  int ctr;
   ctr = 1;
   while (driver_key_pressed() == keynum) {
      driver_get_key();
      ++ctr;
      }
   return ctr;
}

static void _fastcall save_history_info()
{
   HISTORY current,last;
   if(maxhistory <= 0 || bf_math || history == 0)
      return;
   MoveFromMemory((BYTE *)&last,(U16)sizeof(HISTORY),1L,(long)saveptr,history);

   memset((void *)&current,0,sizeof(HISTORY));
   current.fractal_type			= (short)fractype                  ;
   current.xmin					= xxmin                     ;
   current.xmax					= xxmax                     ;
   current.ymin					= yymin                     ;
   current.ymax					= yymax                     ;
   current.creal				= param[0]                  ;
   current.cimag				= param[1]                  ;
   current.dparm3				= param[2]                  ;
   current.dparm4				= param[3]                  ;
   current.dparm5				= param[4]                  ;
   current.dparm6				= param[5]                  ;
   current.dparm7				= param[6]                  ;
   current.dparm8				= param[7]                  ;
   current.dparm9				= param[8]                  ;
   current.dparm10				= param[9]                  ;
   current.fillcolor			= (short)fillcolor                 ;
   current.potential[0]			= potparam[0]               ;
   current.potential[1]			= potparam[1]               ;
   current.potential[2]			= potparam[2]               ;
   current.rflag				= (short)rflag                     ;
   current.rseed				= (short)rseed                     ;
   current.inside				= (short)inside                    ;
   current.logmap				= LogFlag                   ;
   current.invert[0]			= inversion[0]              ;
   current.invert[1]			= inversion[1]              ;
   current.invert[2]			= inversion[2]              ;
   current.decomp				= (short)decomp[0];                ;
   current.biomorph				= (short)biomorph                  ;
   current.symmetry				= (short)forcesymmetry             ;
   current.init3d[0]			= (short)init3d[0]                 ;
   current.init3d[1]			= (short)init3d[1]                 ;
   current.init3d[2]			= (short)init3d[2]                 ;
   current.init3d[3]			= (short)init3d[3]                 ;
   current.init3d[4]			= (short)init3d[4]                 ;
   current.init3d[5]			= (short)init3d[5]                 ;
   current.init3d[6]			= (short)init3d[6]                 ;
   current.init3d[7]			= (short)init3d[7]                 ;
   current.init3d[8]			= (short)init3d[8]                 ;
   current.init3d[9]			= (short)init3d[9]                 ;
   current.init3d[10]			= (short)init3d[10]               ;
   current.init3d[11]			= (short)init3d[12]               ;
   current.init3d[12]			= (short)init3d[13]               ;
   current.init3d[13]			= (short)init3d[14]               ;
   current.init3d[14]			= (short)init3d[15]               ;
   current.init3d[15]			= (short)init3d[16]               ;
   current.previewfactor		= (short)previewfactor             ;
   current.xtrans				= (short)xtrans                    ;
   current.ytrans				= (short)ytrans                    ;
   current.red_crop_left		= (short)red_crop_left             ;
   current.red_crop_right		= (short)red_crop_right            ;
   current.blue_crop_left		= (short)blue_crop_left            ;
   current.blue_crop_right		= (short)blue_crop_right           ;
   current.red_bright			= (short)red_bright                ;
   current.blue_bright			= (short)blue_bright               ;
   current.xadjust				= (short)xadjust                   ;
   current.yadjust				= (short)yadjust                   ;
   current.eyeseparation		= (short)eyeseparation             ;
   current.glassestype			= (short)glassestype               ;
   current.outside				= (short)outside                   ;
   current.x3rd					= xx3rd                     ;
   current.y3rd					= yy3rd                     ;
   current.stdcalcmode			= usr_stdcalcmode               ;
   current.three_pass			= three_pass                ;
   current.stoppass				= (short)stoppass;
   current.distest				= distest                   ;
   current.trigndx[0]			= trigndx[0]                ;
   current.trigndx[1]			= trigndx[1]                ;
   current.trigndx[2]			= trigndx[2]                ;
   current.trigndx[3]			= trigndx[3]                ;
   current.finattract			= (short)finattract                ;
   current.initorbit[0]			= initorbit.x               ;
   current.initorbit[1]			= initorbit.y               ;
   current.useinitorbit			= useinitorbit              ;
   current.periodicity			= (short)periodicitycheck          ;
   current.pot16bit				= (short)disk16bit                 ;
   current.release				= (short)g_release                   ;
   current.save_release			= (short)save_release              ;
   current.flag3d				= (short)display3d                 ;
   current.ambient				= (short)Ambient                   ;
   current.randomize			= (short)RANDOMIZE                 ;
   current.haze					= (short)haze                      ;
   current.transparent[0]		= (short)transparent[0]            ;
   current.transparent[1]		= (short)transparent[1]            ;
   current.rotate_lo			= (short)rotate_lo                 ;
   current.rotate_hi			= (short)rotate_hi                 ;
   current.distestwidth			= (short)distestwidth              ;
   current.mxmaxfp				= mxmaxfp                   ;
   current.mxminfp				= mxminfp                   ;
   current.mymaxfp				= mymaxfp                   ;
   current.myminfp				= myminfp                   ;
   current.zdots				= (short)zdots                         ;
   current.originfp				= originfp                  ;
   current.depthfp				= depthfp                      ;
   current.heightfp				= heightfp                  ;
   current.widthfp				= widthfp                      ;
   current.distfp				= distfp                       ;
   current.eyesfp				= eyesfp                       ;
   current.orbittype			= (short)neworbittype              ;
   current.juli3Dmode			= (short)juli3Dmode                ;
   current.maxfn				= maxfn                     ;
   current.major_method			= (short)major_method              ;
   current.minor_method			= (short)minor_method              ;
   current.bailout				= bailout                   ;
   current.bailoutest			= (short)bailoutest                ;
   current.iterations			= maxit                     ;
   current.old_demm_colors		= (short)old_demm_colors;
   current.logcalc				= (short)Log_Fly_Calc;
   current.ismand				= (short)ismand;
   current.closeprox			= closeprox;
   current.nobof				= (short)nobof;
   current.orbit_delay			= (short)orbit_delay;
   current.orbit_interval		= orbit_interval;
   current.oxmin				= oxmin;
   current.oxmax				= oxmax;
   current.oymin				= oymin;
   current.oymax				= oymax;
   current.ox3rd				= ox3rd;
   current.oy3rd				= oy3rd;
   current.keep_scrn_coords		= (short)keep_scrn_coords;
   current.drawmode				= drawmode;
   memcpy(current.dac,g_dacbox,256*3);
   switch(fractype)
   {
   case FORMULA:
   case FFORMULA:
      strncpy(current.filename,FormFileName,FILE_MAX_PATH);
      strncpy(current.itemname,FormName,ITEMNAMELEN+1);
      break;
   case IFS:
   case IFS3D:
      strncpy(current.filename,IFSFileName,FILE_MAX_PATH);
      strncpy(current.itemname,IFSName,ITEMNAMELEN+1);
      break;
   case LSYSTEM:
      strncpy(current.filename,LFileName,FILE_MAX_PATH);
      strncpy(current.itemname,LName,ITEMNAMELEN+1);
      break;
   default:
      *(current.filename) = 0;
      *(current.itemname) = 0;
      break;
   }
   if (historyptr == -1)        /* initialize the history file */
   {
      int i;
      for (i = 0; i < maxhistory; i++)
         MoveToMemory((BYTE *)&current,(U16)sizeof(HISTORY),1L,(long)i,history);
      historyflag = saveptr = historyptr = 0;   /* initialize history ptr */
   }
   else if(historyflag == 1)
      historyflag = 0;   /* coming from user history command, don't save */
   else if (memcmp(&current,&last,sizeof(HISTORY)))
   {
      if(++saveptr >= maxhistory)  /* back to beginning of circular buffer */
         saveptr = 0;
      if(++historyptr >= maxhistory)  /* move user pointer in parallel */
         historyptr = 0;
      MoveToMemory((BYTE *)&current,(U16)sizeof(HISTORY),1L,(long)saveptr,history);
   }
}

static void _fastcall restore_history_info(int i)
{
   HISTORY last;
   if(maxhistory <= 0 || bf_math || history == 0)
      return;
   MoveFromMemory((BYTE *)&last,(U16)sizeof(HISTORY),1L,(long)i,history);
   invert = 0;
   calc_status = 0;
   resuming = 0;
   fractype              = last.fractal_type   ;
   xxmin                 = last.xmin           ;
   xxmax                 = last.xmax           ;
   yymin                 = last.ymin           ;
   yymax                 = last.ymax           ;
   param[0]              = last.creal          ;
   param[1]              = last.cimag          ;
   param[2]              = last.dparm3         ;
   param[3]              = last.dparm4         ;
   param[4]              = last.dparm5         ;
   param[5]              = last.dparm6         ;
   param[6]              = last.dparm7         ;
   param[7]              = last.dparm8         ;
   param[8]              = last.dparm9         ;
   param[9]              = last.dparm10        ;
   fillcolor             = last.fillcolor      ;
   potparam[0]           = last.potential[0]   ;
   potparam[1]           = last.potential[1]   ;
   potparam[2]           = last.potential[2]   ;
   rflag                 = last.rflag          ;
   rseed                 = last.rseed          ;
   inside                = last.inside         ;
   LogFlag               = last.logmap         ;
   inversion[0]          = last.invert[0]      ;
   inversion[1]          = last.invert[1]      ;
   inversion[2]          = last.invert[2]      ;
   decomp[0]             = last.decomp         ;
   usr_biomorph          = last.biomorph       ;
   biomorph              = last.biomorph       ;
   forcesymmetry         = last.symmetry       ;
   init3d[0]             = last.init3d[0]      ;
   init3d[1]             = last.init3d[1]      ;
   init3d[2]             = last.init3d[2]      ;
   init3d[3]             = last.init3d[3]      ;
   init3d[4]             = last.init3d[4]      ;
   init3d[5]             = last.init3d[5]      ;
   init3d[6]             = last.init3d[6]      ;
   init3d[7]             = last.init3d[7]      ;
   init3d[8]             = last.init3d[8]      ;
   init3d[9]             = last.init3d[9]      ;
   init3d[10]            = last.init3d[10]     ;
   init3d[12]            = last.init3d[11]     ;
   init3d[13]            = last.init3d[12]     ;
   init3d[14]            = last.init3d[13]     ;
   init3d[15]            = last.init3d[14]     ;
   init3d[16]            = last.init3d[15]     ;
   previewfactor         = last.previewfactor  ;
   xtrans                = last.xtrans         ;
   ytrans                = last.ytrans         ;
   red_crop_left         = last.red_crop_left  ;
   red_crop_right        = last.red_crop_right ;
   blue_crop_left        = last.blue_crop_left ;
   blue_crop_right       = last.blue_crop_right;
   red_bright            = last.red_bright     ;
   blue_bright           = last.blue_bright    ;
   xadjust               = last.xadjust        ;
   yadjust               = last.yadjust        ;
   eyeseparation         = last.eyeseparation  ;
   glassestype           = last.glassestype    ;
   outside               = last.outside        ;
   xx3rd                 = last.x3rd           ;
   yy3rd                 = last.y3rd           ;
   usr_stdcalcmode       = last.stdcalcmode    ;
   stdcalcmode           = last.stdcalcmode    ;
   three_pass            = last.three_pass     ;
   stoppass              = last.stoppass       ;
   distest               = last.distest        ;
   usr_distest           = last.distest        ;
   trigndx[0]            = last.trigndx[0]     ;
   trigndx[1]            = last.trigndx[1]     ;
   trigndx[2]            = last.trigndx[2]     ;
   trigndx[3]            = last.trigndx[3]     ;
   finattract            = last.finattract     ;
   initorbit.x           = last.initorbit[0]   ;
   initorbit.y           = last.initorbit[1]   ;
   useinitorbit          = last.useinitorbit   ;
   periodicitycheck      = last.periodicity    ;
   usr_periodicitycheck  = last.periodicity    ;
   disk16bit             = last.pot16bit       ;
   g_release             = last.release        ;
   save_release          = last.save_release   ;
   display3d             = last.flag3d         ;
   Ambient               = last.ambient        ;
   RANDOMIZE             = last.randomize      ;
   haze                  = last.haze           ;
   transparent[0]        = last.transparent[0] ;
   transparent[1]        = last.transparent[1] ;
   rotate_lo             = last.rotate_lo      ;
   rotate_hi             = last.rotate_hi      ;
   distestwidth          = last.distestwidth   ;
   mxmaxfp               = last.mxmaxfp        ;
   mxminfp               = last.mxminfp        ;
   mymaxfp               = last.mymaxfp        ;
   myminfp               = last.myminfp        ;
   zdots                 = last.zdots          ;
   originfp              = last.originfp       ;
   depthfp               = last.depthfp        ;
   heightfp              = last.heightfp       ;
   widthfp               = last.widthfp        ;
   distfp                = last.distfp         ;
   eyesfp                = last.eyesfp         ;
   neworbittype          = last.orbittype      ;
   juli3Dmode            = last.juli3Dmode     ;
   maxfn                 = last.maxfn          ;
   major_method          = (enum Major)last.major_method   ;
   minor_method          = (enum Minor)last.minor_method   ;
   bailout               = last.bailout        ;
   bailoutest            = (enum bailouts)last.bailoutest     ;
   maxit                 = last.iterations     ;
   old_demm_colors       = last.old_demm_colors;
   curfractalspecific    = &fractalspecific[fractype];
   potflag               = (potparam[0] != 0.0);
   if (inversion[0] != 0.0)
      invert = 3;
   Log_Fly_Calc = last.logcalc;
   ismand = last.ismand;
   closeprox = last.closeprox;
   nobof = last.nobof;
   orbit_delay = last.orbit_delay;
   orbit_interval = last.orbit_interval;
   oxmin = last.oxmin;
   oxmax = last.oxmax;
   oymin = last.oymin;
   oymax = last.oymax;
   ox3rd = last.ox3rd;
   oy3rd = last.oy3rd;
   keep_scrn_coords = last.keep_scrn_coords;
   if (keep_scrn_coords) set_orbit_corners = 1;
   drawmode = last.drawmode;
   usr_floatflag = (char)((curfractalspecific->isinteger) ? 0 : 1);
   memcpy(g_dacbox,last.dac,256*3);
   memcpy(olddacbox,last.dac,256*3);
   if(mapdacbox)
      memcpy(mapdacbox,last.dac,256*3);
   spindac(0,1);
   if(fractype == JULIBROT || fractype == JULIBROTFP)
      savedac = 0;
   else
      savedac = 1;
   switch(fractype)
   {
   case FORMULA:
   case FFORMULA:
      strncpy(FormFileName,last.filename,FILE_MAX_PATH);
      strncpy(FormName,    last.itemname,ITEMNAMELEN+1);
      break;
   case IFS:
   case IFS3D:
      strncpy(IFSFileName,last.filename,FILE_MAX_PATH);
      strncpy(IFSName    ,last.itemname,ITEMNAMELEN+1);
      break;
   case LSYSTEM:
      strncpy(LFileName,last.filename,FILE_MAX_PATH);
      strncpy(LName    ,last.itemname,ITEMNAMELEN+1);
      break;
   default:
      break;
   }
}

void checkfreemem(int secondpass)
{
	int oldmaxhistory;
	char *tmp;
	static char msg[] =
		{" I'm sorry, but you don't have enough free memory \n to run this program.\n\n"};
	static char msg2[] = {"To save memory, reduced maxhistory to "};
	tmp = (char *)malloc(4096L);
	oldmaxhistory = maxhistory;
	if (secondpass && !history)
	{
		while (maxhistory > 0) /* decrease history if necessary */
		{
			/* TODO: MemoryAlloc */
			history = MemoryAlloc((U16)sizeof(HISTORY),(long)maxhistory,EXPANDED);
			if (history)
			{
				break;
			}
			maxhistory--;
		}
	}
	if (extraseg == 0 || tmp == NULL)
	{
		driver_buzzer(2);
#ifndef XFRACT
		printf("%Fs",(char *)msg);
#else
		printf("%s",msg);
#endif
		exit(1);
	}
	free(tmp); /* was just to check for min space */
	if (secondpass && (maxhistory < oldmaxhistory || (history == 0 && oldmaxhistory != 0)))
	{
#ifndef XFRACT
		printf("%Fs%d\n%Fs\n",(char *)msg2,maxhistory,s_pressanykeytocontinue);
#else
		printf("%s%d\n%s\n",(char *)msg2,maxhistory,s_pressanykeytocontinue);
#endif
		driver_get_key();
	}
}

