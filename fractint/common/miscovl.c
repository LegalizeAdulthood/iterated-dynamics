/*
        Overlayed odds and ends that don't fit anywhere else.
*/

#include <string.h>
#include <ctype.h>
#include <time.h>

#ifndef XFRACT
#include <malloc.h>
#include <process.h>
#include <io.h>
#endif

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"

/* routines in this module      */

void write_batch_parms(char *colorinf,int colorsonly, int maxcolor,int i, int j);
void expand_comments(char *target, char *source);

#ifndef USE_VARARGS
static void put_parm(char *parm,...);
#else
static void put_parm();
#endif

static void put_parm_line(void);
static int getprec(double,double,double);
int getprecbf(int);
static void put_float(int,double,int);
static void put_bf(int slash,bf_t r, int prec);
static void put_filename(char *keyword,char *fname);
#ifndef XFRACT
static int check_modekey(int curkey,int choice);
#endif
static int entcompare(VOIDCONSTPTR p1,VOIDCONSTPTR p2);
static void update_fractint_cfg(void);
static void strip_zeros(char *buf);

/* fullscreen_choice options */
#define CHOICERETURNKEY 1
#define CHOICEMENU      2
#define CHOICEHELP      4

char par_comment[4][MAXCMT];

char s_yes[]      = "yes";
char s_no[]       = "no";
char s_seqs[]     = " %s=%s";
char s_seqd[]     = " %s=%d";
char s_seqdd[]    = " %s=%d/%d";
char s_seqddd[]   = " %s=%d/%d/%d";
char s_seqldddd[]  = " %s=%ld/%d/%d/%d";
char s_seqd12[]   = " %s=%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d";
char s_seqy[]     = " %s=y";
char s_x[]        = "x";
char s_y[]        = "y";
char s_z[]        = "z";

/* JIIM */

FILE *parmfile;

#define PAR_KEY(x)  ( x < 10 ? '0' + x : 'a' - 10 + x)

#ifdef _MSC_VER
#pragma optimize("e",off)  /* MSC 6.00A messes up next rtn with "e" on */
#endif

#define LOADBATCHPROMPTS(X)     {\
   static FCODE tmp[] = { X };\
   strcpy(ptr,tmp);\
   choices[promptnum]= ptr;\
   ptr += sizeof(tmp);\
   }

void make_batch_file()
{
#define MAXPROMPTS 18
   int colorsonly = 0;
   static char hdg[]={"Save Current Parameters"};
   /** added for pieces feature **/
   double pdelx = 0.0;
   double pdely = 0.0;
   double pdelx2 = 0.0;
   double pdely2 = 0.0;
   unsigned int pxdots, pydots, xm, ym;
   double pxxmin = 0.0, pyymax = 0.0;
   char vidmde[5];
   int promptnum;
   int piecespromts;
   int have3rd = 0;
   /****/

   int i,j;
   char *inpcommandfile, *inpcommandname;
   char *inpcomment[4];
   struct fullscreenvalues paramvalues[18];
   char * choices[MAXPROMPTS];
   char *ptr;
   int gotinfile;
   char outname[FILE_MAX_PATH+1], buf[256], buf2[128];
   FILE *infile = NULL;
   FILE *fpbat = NULL;
   char colorspec[14];
   int maxcolor;
   int maxcolorindex = 0;
   char *sptr = NULL, *sptr2;
   int oldhelpmode;

   if(s_makepar[1] == 0) /* makepar map case */
      colorsonly = 1;

   /* put comment storage in extraseg */
   /* TODO: allocate real memory, not reuse shared segment */
   inpcommandfile = MK_FP(extraseg,0);
   inpcommandname = inpcommandfile+80;
   inpcomment[0]    = inpcommandname+(ITEMNAMELEN + 1);
   inpcomment[1]    = inpcomment[0] + MAXCMT;
   inpcomment[2]    = inpcomment[1] + MAXCMT;
   inpcomment[3]    = inpcomment[2] + MAXCMT;

   /* steal existing array for "choices" */
   ptr = (char *)(inpcomment[3] + MAXCMT);
   driver_stack_screen();
   oldhelpmode = helpmode;
   helpmode = HELPPARMFILE;

   maxcolor = colors;
   strcpy(colorspec,"y");
#ifndef XFRACT
   if ((gotrealdac && !reallyega) || (istruecolor && !truemode))
#else
   if ((gotrealdac && !reallyega) || (istruecolor && !truemode) || fake_lut)
#endif
   {
      --maxcolor;
/*    if (maxit < maxcolor)  remove 2 lines */
/*       maxcolor = maxit;   so that whole palette is always saved */
      if (inside > 0 && inside > maxcolor)
         maxcolor = inside;
      if (outside > 0 && outside > maxcolor)
         maxcolor = outside;
      if (distest < 0 && 0 - distest > maxcolor)
         maxcolor = (int)(0 - distest);
      if (decomp[0] > maxcolor)
         maxcolor = decomp[0] - 1;
      if (potflag && potparam[0] >= maxcolor)
         maxcolor = (int)potparam[0];
      if (++maxcolor > 256)
         maxcolor = 256;
      if (colorstate == 0)
      {                         /* default colors */
         if (mapdacbox)
         {
            colorspec[0] = '@';
            sptr = MAP_name;
         }
      }
      else if (colorstate == 2)
      {                         /* colors match colorfile */
         colorspec[0] = '@';
         sptr = colorfile;
      }
      else                      /* colors match no .map that we know of */
         strcpy (colorspec,"y");

      if (colorspec[0] == '@')
      {
         if ((sptr2 = strrchr(sptr, SLASHC)) != NULL)
            sptr = sptr2 + 1;
         if ((sptr2 = strrchr(sptr, ':')) != NULL)
            sptr = sptr2 + 1;
         strncpy(&colorspec[1], sptr, 12);
         colorspec[13] = 0;
      }
   }
   strcpy(inpcommandfile, CommandFile);
   strcpy(inpcommandname, CommandName);
   for(i=0;i<4;i++)
   {
      expand_comments(CommandComment[i], par_comment[i]);
      strcpy(inpcomment[i], CommandComment[i]);
   }
   
   if (CommandName[0] == 0)
      strcpy(inpcommandname, "test");
   /* TW added these  - and Bert moved them */
   pxdots = xdots;
   pydots = ydots;
   xm = ym = 1;
   if(*s_makepar == 0)
      goto skip_UI;

   vidmode_keyname(videoentry.keynum, vidmde);
   for(;;)
   {
prompt_user:
      promptnum = 0;
      LOADBATCHPROMPTS("Parameter file");
      paramvalues[promptnum].type = 0x100 + MAXCMT - 1;
      paramvalues[promptnum++].uval.sbuf = inpcommandfile;
      LOADBATCHPROMPTS("Name");
      paramvalues[promptnum].type = 0x100 + ITEMNAMELEN;
      paramvalues[promptnum++].uval.sbuf = inpcommandname;
      LOADBATCHPROMPTS("Main comment");
      paramvalues[promptnum].type = 0x100 + MAXCMT - 1;
      paramvalues[promptnum++].uval.sbuf = inpcomment[0];
      LOADBATCHPROMPTS("Second comment");
      paramvalues[promptnum].type = 0x100 + MAXCMT - 1;
      paramvalues[promptnum++].uval.sbuf = inpcomment[1];
      LOADBATCHPROMPTS("Third comment");
      paramvalues[promptnum].type = 0x100 + MAXCMT - 1;
      paramvalues[promptnum++].uval.sbuf = inpcomment[2];
      LOADBATCHPROMPTS("Fourth comment");
      paramvalues[promptnum].type = 0x100 + MAXCMT - 1;
      paramvalues[promptnum++].uval.sbuf = inpcomment[3];
#ifndef XFRACT
      if ((gotrealdac && !reallyega) || (istruecolor && !truemode))
#else
      if ((gotrealdac && !reallyega) || (istruecolor && !truemode) || fake_lut)
#endif
      {
         LOADBATCHPROMPTS("Record colors?");
         paramvalues[promptnum].type = 0x100 + 13;
         paramvalues[promptnum++].uval.sbuf = colorspec;
         LOADBATCHPROMPTS("    (no | yes | only for full info | @filename to point to a map file)");
         paramvalues[promptnum++].type = '*';
         LOADBATCHPROMPTS("# of colors");
         maxcolorindex = promptnum;
         paramvalues[promptnum].type = 'i';
         paramvalues[promptnum++].uval.ival = maxcolor;
         LOADBATCHPROMPTS("    (if recording full color info)");
         paramvalues[promptnum++].type = '*';
      }
      LOADBATCHPROMPTS("Maximum line length");
      paramvalues[promptnum].type = 'i';
      paramvalues[promptnum++].uval.ival = maxlinelength;
      LOADBATCHPROMPTS("");
      paramvalues[promptnum++].type = '*';
      LOADBATCHPROMPTS("    **** The following is for generating images in pieces ****");
      paramvalues[promptnum++].type = '*';
      LOADBATCHPROMPTS("X Multiples");
      piecespromts = promptnum;
      paramvalues[promptnum].type = 'i';
      paramvalues[promptnum++].uval.ival = xm;
      LOADBATCHPROMPTS("Y Multiples");
      paramvalues[promptnum].type = 'i';
      paramvalues[promptnum++].uval.ival = ym;
#ifndef XFRACT
      LOADBATCHPROMPTS("Video mode");
      paramvalues[promptnum].type = 0x100 + 4;
      paramvalues[promptnum++].uval.sbuf = vidmde;
#endif

      if (fullscreen_prompt(hdg,promptnum, choices, paramvalues, 0, NULL) < 0)
         break;

      if(*colorspec == 'o' || s_makepar[1] == 0)
      {
         strcpy(colorspec,"y");
         colorsonly = 1;
      }

      strcpy(CommandFile, inpcommandfile);
      if (has_ext(CommandFile) == NULL)
         strcat(CommandFile, ".par");   /* default extension .par */
      strcpy(CommandName, inpcommandname);
      for(i=0;i<4;i++)
         strncpy(CommandComment[i], inpcomment[i], MAXCMT);
#ifndef XFRACT
      if ((gotrealdac && !reallyega) || (istruecolor && !truemode))
#else
      if ((gotrealdac && !reallyega) || (istruecolor && !truemode) || fake_lut)
#endif
         if (paramvalues[maxcolorindex].uval.ival > 0 &&
             paramvalues[maxcolorindex].uval.ival <= 256)
            maxcolor = paramvalues[maxcolorindex].uval.ival;
      promptnum = piecespromts;
      {
         int newmaxlinelength;
         newmaxlinelength = paramvalues[promptnum-3].uval.ival;
         if(maxlinelength != newmaxlinelength &&
              newmaxlinelength >= MINMAXLINELENGTH &&
              newmaxlinelength <= MAXMAXLINELENGTH)
            maxlinelength = newmaxlinelength;    
      }
      xm = paramvalues[promptnum++].uval.ival;

      ym = paramvalues[promptnum++].uval.ival;

      /* sanity checks */
      {
      long xtotal, ytotal;
#ifndef XFRACT
      int i;

      /* get resolution from the video name (which must be valid) */
      pxdots = pydots = 0;
      if ((i = check_vidmode_keyname(vidmde)) > 0)
          if ((i = check_vidmode_key(0, i)) >= 0) {
              /* get the resolution of this video mode */
              pxdots = videotable[i].xdots;
              pydots = videotable[i].ydots;
              }
      if (pxdots == 0 && (xm > 1 || ym > 1)) {
          /* no corresponding video mode! */
          static FCODE msg[] = {"Invalid video mode entry!"};
          stopmsg(0,msg);
          goto prompt_user;
          }
#endif

      /* bounds range on xm, ym */
      if (xm < 1 || xm > 36 || ym < 1 || ym > 36) {
          static FCODE msg[] = {"X and Y components must be 1 to 36"};
          stopmsg(0,msg);
          goto prompt_user;
          }

      /* another sanity check: total resolution cannot exceed 65535 */
      xtotal = xm;  ytotal = ym;
      xtotal *= pxdots;  ytotal *= pydots;
      if (xtotal > 65535L || ytotal > 65535L) {
      static FCODE msg[] = {"Total resolution (X or Y) cannot exceed 65535"};
          stopmsg(0,msg);
          goto prompt_user;
          }
      }
skip_UI:
      if(*s_makepar == 0)
      {
         if(filecolors > 0)
            strcpy(colorspec, "y");
         else
            strcpy(colorspec, "n");
         if(s_makepar[1] == 0)
            maxcolor = 256;
         else   
            maxcolor = filecolors;
      }
      strcpy(outname, CommandFile);
      gotinfile = 0;
      if (access(CommandFile, 0) == 0)
      {                         /* file exists */
         gotinfile = 1;
         if (access(CommandFile, 6))
         {
            sprintf(buf, s_cantwrite, CommandFile);
            stopmsg(0, buf);
            continue;
         }
         i = (int) strlen(outname);
         while (--i >= 0 && outname[i] != SLASHC)
            outname[i] = 0;
         strcat(outname, "fractint.tmp");
         infile = fopen(CommandFile, "rt");
#ifndef XFRACT
         setvbuf(infile, tstack, _IOFBF, 4096); /* improves speed */
#endif
      }
      if ((parmfile = fopen(outname, "wt")) == NULL)
      {
         sprintf(buf, s_cantcreate, outname);
         stopmsg(0, buf);
         if (gotinfile)
            fclose(infile);
         continue;
      }

      if (gotinfile)
      {
         while (file_gets(buf, 255, infile) >= 0)
         {
            if (strchr(buf, '{')/* entry heading? */
                && sscanf(buf, " %40[^ \t({]", buf2)
                && stricmp(buf2, CommandName) == 0)
            {                   /* entry with same name */
               static FCODE s1[] = {"File already has an entry named "};
               static FCODE s2[] = {"\n\
Continue to replace it, Cancel to back out"};
               static FCODE s2a[] = {"... Replacing ..."};
               strcpy(buf2,s1);
               strcat(buf2,CommandName);
               if(*s_makepar == 0)
                   strcat(buf2,s2a);
               else
                   strcat(buf2,s2);
               if (stopmsg(18, buf2) < 0)
               {                /* cancel */
                  fclose(infile);
                  fclose(parmfile);
                  unlink(outname);
                  goto prompt_user;
               }
               while (strchr(buf, '}') == NULL
                      && file_gets(buf, 255, infile) > 0)
                  ; /* skip to end of set */
               break;
            }
            fputs(buf, parmfile);
            fputc('\n', parmfile);
         }
      }
/***** start here*/
      if (xm > 1 || ym > 1)
      {
         if (xxmin != xx3rd || yymin != yy3rd)
            have3rd = 1;
         else
            have3rd = 0;
         if ((fpbat = dir_fopen(workdir,"makemig.bat", "w")) == NULL)
            xm = ym = 0;
         pdelx  = (xxmax - xx3rd) / (xm * pxdots - 1);   /* calculate stepsizes */
         pdely  = (yymax - yy3rd) / (ym * pydots - 1);
         pdelx2 = (xx3rd - xxmin) / (ym * pydots - 1);
         pdely2 = (yy3rd - yymin) / (xm * pxdots - 1);

         /* save corners */
         pxxmin = xxmin;
         pyymax = yymax;
      }
      for (i = 0; i < (int)xm; i++)  /* columns */
      for (j = 0; j < (int)ym; j++)  /* rows    */
      {
         if (xm > 1 || ym > 1)
         {
            int w;
            char c;
            char PCommandName[80];
            w=0;
            while(w < (int)strlen(CommandName))
            {
               c = CommandName[w];
               if(isspace(c) || c == 0)
                  break;
               PCommandName[w] = c;
               w++;
            }
            PCommandName[w] = 0;
            {
               char buf[20];
               sprintf(buf,"_%c%c",PAR_KEY(i),PAR_KEY(j));
               strcat(PCommandName,buf);
            }
            fprintf(parmfile, "%-19s{",PCommandName);
            xxmin = pxxmin + pdelx*(i*pxdots) + pdelx2*(j*pydots);
            xxmax = pxxmin + pdelx*((i+1)*pxdots - 1) + pdelx2*((j+1)*pydots - 1);
            yymin = pyymax - pdely*((j+1)*pydots - 1) - pdely2*((i+1)*pxdots - 1);
            yymax = pyymax - pdely*(j*pydots) - pdely2*(i*pxdots);
            if (have3rd)
            {
               xx3rd = pxxmin + pdelx*(i*pxdots) + pdelx2*((j+1)*pydots - 1);
               yy3rd = pyymax - pdely*((j+1)*pydots - 1) - pdely2*(i*pxdots);
            }
            else
            {
               xx3rd = xxmin;
               yy3rd = yymin;
            }
            fprintf(fpbat,"Fractint batch=yes overwrite=yes @%s/%s\n",CommandFile,PCommandName);
            fprintf(fpbat,"If Errorlevel 2 goto oops\n");
         }
         else
            fprintf(parmfile, "%-19s{", CommandName);
         {
            /* guarantee that there are no blank comments above the last
               non-blank par_comment */
            int i, last;
            for(last=-1,i=0;i<4;i++)
               if(*par_comment[i])
                  last=i;
            for(i=0;i<last;i++)
               if(*CommandComment[i]=='\0')
                  strcpy(CommandComment[i],";");
         }
         if (CommandComment[0][0])
            fprintf(parmfile, " ; %s", CommandComment[0]);
         fputc('\n', parmfile);
         {
            int k;
            char buf[25];
            memset(buf, ' ', 23);
            buf[23] = 0;
            buf[21] = ';';
            for(k=1;k<4;k++)
               if (CommandComment[k][0])
                  fprintf(parmfile, "%s%s\n", buf, CommandComment[k]);
            if (patchlevel != 0 && colorsonly == 0)
               fprintf(parmfile, "%s %s Version %d Patchlevel %d\n", buf,
                  Fractint, release, patchlevel); 
         }
         write_batch_parms(colorspec, colorsonly, maxcolor, i, j);
         if(xm > 1 || ym > 1)
         {
            fprintf(parmfile,"  video=%s", vidmde);
            fprintf(parmfile," savename=frmig_%c%c\n", PAR_KEY(i), PAR_KEY(j));
         }
         fprintf(parmfile, "  }\n\n");
      }
      if(xm > 1 || ym > 1)
         {
         fprintf(fpbat,"Fractint makemig=%d/%d\n",xm,ym);
         fprintf(fpbat,"Rem Simplgif fractmig.gif simplgif.gif  in case you need it\n");
         fprintf(fpbat,":oops\n");
         fclose(fpbat);
         }
/*******end here */

      if (gotinfile)
      {                         /* copy the rest of the file */
         while ((i = file_gets(buf, 255, infile)) == 0)
            ; /* skip blanks */
         while (i >= 0)
         {
            fputs(buf, parmfile);
            fputc('\n', parmfile);
            i = file_gets(buf, 255, infile);
         }
         fclose(infile);
      }
      fclose(parmfile);
      if (gotinfile)
      {                         /* replace the original file with the new */
         unlink(CommandFile);   /* success assumed on these lines       */
         rename(outname, CommandFile);  /* since we checked earlier with
                                         * access */
      }
      break;
   }
   helpmode = oldhelpmode;
   driver_unstack_screen();
}

#ifdef C6
#pragma optimize("e",on)  /* back to normal */
#endif

static struct write_batch_data { /* buffer for parms to break lines nicely */
   int len;
   char *buf;
   } *wbdata;

void write_batch_parms(char *colorinf, int colorsonly, int maxcolor, int ii, int jj)
{
   char *saveshared;
   int i,j,k;
   double Xctr, Yctr;
   LDBL Magnification;
   double Xmagfactor, Rotation, Skew;
   struct write_batch_data wb_data;
   char *sptr;
   char buf[81];
   bf_t bfXctr=NULL, bfYctr=NULL;
   int saved;
   saved = save_stack();
   if(bf_math)
   {
      bfXctr = alloc_stack(bflength+2);
      bfYctr = alloc_stack(bflength+2);
   }
   wbdata = &wb_data;
   wb_data.len = 0; /* force first parm to start on new line */

   /* Using near string boxx for buffer after saving to extraseg */

   /* TODO: allocate real memory, not reuse shared segment */
   saveshared = MK_FP(extraseg,0);
   memcpy(saveshared,boxx,10000);
   memset(boxx,0,10000);
   wb_data.buf = (char *)boxx;
   if(colorsonly)
      goto docolors;
   if (display3d <= 0) { /* a fractal was generated */

      /****** fractal only parameters in this section *******/
      put_parm(" reset");
      if (check_back())
        put_parm("=%d",min(save_release,release));
      else
        put_parm("=%d",release);

      if (*(sptr = curfractalspecific->name) == '*') ++sptr;
      put_parm( s_seqs,s_type,sptr);

      if (fractype == JULIBROT || fractype == JULIBROTFP)
      {
         put_parm(" %s=%.15g/%.15g/%.15g/%.15g",
             s_julibrotfromto,mxmaxfp,mxminfp,mymaxfp,myminfp);
         /* these rarely change */
         if(originfp != 8 || heightfp != 7 || widthfp != 10 || distfp != 24
                          || depthfp != 8 || zdots != 128)
            put_parm(" %s=%d/%g/%g/%g/%g/%g",s_julibrot3d,
                zdots, originfp, depthfp, heightfp, widthfp,distfp);
         if(eyesfp != 0)
            put_parm(" %s=%g",s_julibroteyes,eyesfp);
         if(neworbittype != JULIA)
         {
            char *name;
            name = fractalspecific[neworbittype].name;
            if(*name=='*')
               name++;
            put_parm(s_seqs,s_orbitname,name);
         }
         if(juli3Dmode != 0)
            put_parm(s_seqs,s_3dmode,juli3Doptions[juli3Dmode]);
      }
      if (fractype == FORMULA || fractype == FFORMULA)
      {
         put_filename(s_formulafile,FormFileName);
         put_parm( s_seqs,s_formulaname,FormName);
         if (uses_ismand)
            put_parm(" %s=%c",s_ismand,ismand?'y':'n');
      }
      if (fractype == LSYSTEM)
      {
         put_filename(s_lfile,LFileName);
         put_parm( s_seqs,s_lname,LName);
      }
      if (fractype == IFS || fractype == IFS3D)
      {
         put_filename(s_ifsfile,IFSFileName);
         put_parm( s_seqs,s_ifs,IFSName);
      }
      if (fractype == INVERSEJULIA || fractype == INVERSEJULIAFP)
         put_parm( " %s=%s/%s",s_miim,JIIMmethod[major_method], JIIMleftright[minor_method]);

      showtrig(buf); /* this function is in miscres.c */
      if (buf[0])
         put_parm(buf);

      if (usr_stdcalcmode != 'g')
         put_parm(" %s=%c",s_passes,usr_stdcalcmode);


      if (stoppass != 0)
         put_parm(" %s=%c%c",s_passes,usr_stdcalcmode,(char)stoppass + '0');

      if (usemag)
      {
         if (bf_math)
         {
            int digits;
            cvtcentermagbf(bfXctr, bfYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            digits = getprecbf(MAXREZ);
            put_parm(" %s=",s_centermag);
            put_bf(0,bfXctr,digits);
            put_bf(1,bfYctr,digits);
         }
         else /* !bf_math */
         {
            cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            put_parm(" %s=",s_centermag);
/*          convert 1000 fudged long to double, 1000/1<<24 = 6e-5 */
            put_parm(ddelmin > 6e-5 ? "%g/%g" : "%+20.17lf/%+20.17lf", Xctr, Yctr);
         }
#ifdef USE_LONG_DOUBLE
         put_parm("/%.7Lg",Magnification); /* precision of magnification not critical, but magnitude is */
#else
         put_parm("/%.7lg",Magnification); /* precision of magnification not critical, but magnitude is */
#endif
         /* Round to avoid ugly decimals, precision here is not critical */
         /* Don't round Xmagfactor if it's small */
         if (fabs(Xmagfactor) > 0.5) /* or so, exact value isn't important */
            Xmagfactor = (sign(Xmagfactor) * (long)(fabs(Xmagfactor) * 1e4 + 0.5)) / 1e4;
         /* Just truncate these angles.  Who cares about 1/1000 of a degree */
         /* Somebody does.  Some rotated and/or skewed images are slightly */
         /* off when recreated from a PAR using 1/1000. */
         /* JCO 08052001 */
#if 0
         Rotation   = (long)(Rotation   * 1e3)/1e3;
         Skew       = (long)(Skew       * 1e3)/1e3;
#endif
         if (Xmagfactor != 1 || Rotation != 0 || Skew != 0)
         { /* Only put what is necessary */
            /* The difference with Xmagfactor is that it is normally */
            /* near 1 while the others are normally near 0 */
            if (fabs(Xmagfactor) >= 1)
               put_float(1,Xmagfactor,5); /* put_float() uses %g */
            else /* abs(Xmagfactor) is < 1 */
               put_float(1,Xmagfactor,4); /* put_float() uses %g */
            if (Rotation != 0 || Skew != 0)
            {
               /* Use precision=6 here.  These angle have already been rounded        */
               /* to 3 decimal places, but angles like 123.456 degrees need 6         */
               /* sig figs to get 3 decimal places.  Trailing 0's are dropped anyway. */
               /* Changed to 18 to address rotated and skewed problem w/ PARs */
               /* JCO 08052001 */
               put_float(1,Rotation,18);
               if (Skew != 0)
               {
                  put_float(1,Skew,18);
               }
            }
         }
      }
      else /* not usemag */
      {
         put_parm( " %s=",s_corners);
         if(bf_math)
         {
            int digits;
            digits = getprecbf(MAXREZ);
            put_bf(0,bfxmin,digits);
            put_bf(1,bfxmax,digits);
            put_bf(1,bfymin,digits);
            put_bf(1,bfymax,digits);
            if (cmp_bf(bfx3rd,bfxmin) || cmp_bf(bfy3rd,bfymin))
            {
               put_bf(1,bfx3rd,digits);
               put_bf(1,bfy3rd,digits);
            }
         }
         else
         {
            int xdigits,ydigits;
            xdigits = getprec(xxmin,xxmax,xx3rd);
            ydigits = getprec(yymin,yymax,yy3rd);
            put_float(0,xxmin,xdigits);
            put_float(1,xxmax,xdigits);
            put_float(1,yymin,ydigits);
            put_float(1,yymax,ydigits);
            if (xx3rd != xxmin || yy3rd != yymin)
            {
               put_float(1,xx3rd,xdigits);
               put_float(1,yy3rd,ydigits);
            }
         }
      }

      for(i = (MAXPARAMS-1); i >= 0; --i)
          if(typehasparm((fractype==JULIBROT || fractype==JULIBROTFP)
                          ?neworbittype:fractype,i,NULL)) break;

      if (i >= 0) {
        if (fractype == CELLULAR || fractype == ANT)
          put_parm(" %s=%.1f",s_params,param[0]);
        else
        {
#ifdef USE_LONG_DOUBLE
          if(debugflag == 750)
             put_parm(" %s=%.17Lg",s_params,(long double)param[0]);
          else
#endif
          put_parm(" %s=%.17g",s_params,param[0]);
        }
        for (j = 1; j <= i; ++j)
        if (fractype == CELLULAR || fractype == ANT)
          put_parm("/%.1f",param[j]);
        else
        {
#ifdef USE_LONG_DOUBLE
          if(debugflag == 750)
             put_parm("/%.17Lg",(long double)param[j]);
          else
#endif
          put_parm("/%.17g",param[j]);
        }
      }

      if(useinitorbit == 2)
         put_parm( " %s=pixel",s_initorbit);
      else if(useinitorbit == 1)
         put_parm( " %s=%.15g/%.15g",s_initorbit,initorbit.x,initorbit.y);

      if (floatflag)
         put_parm( s_seqy,s_float);

      if (maxit != 150)
         put_parm(" %s=%ld",s_maxiter,maxit);

      if(bailout && (potflag == 0 || potparam[2] == 0.0))
         put_parm(" %s=%ld",s_bailout,bailout);

      if(bailoutest != Mod) {
         put_parm(" %s=",s_bailoutest);
         if (bailoutest == Real)
            put_parm( s_real);
         else if (bailoutest == Imag)
            put_parm(s_imag);
         else if (bailoutest == Or)
            put_parm(s_or);
         else if (bailoutest == And)
            put_parm(s_and);
         else if (bailoutest == Manh)
            put_parm(s_manh);
         else if (bailoutest == Manr)
            put_parm(s_manr);
         else
            put_parm(s_mod); /* default, just in case */
      }
      if(fillcolor != -1) {
         put_parm(" %s=",s_fillcolor);
        put_parm( "%d",fillcolor);
      }
      if (inside != 1) {
         put_parm(" %s=",s_inside);
         if (inside == -1)
            put_parm( s_maxiter);
         else if (inside == ZMAG)
            put_parm(s_zmag);
         else if (inside == BOF60)
            put_parm(s_bof60);
         else if (inside == BOF61)
            put_parm(s_bof61);
         else if (inside == EPSCROSS)
            put_parm(s_epscross);
         else if (inside == STARTRAIL)
            put_parm(s_startrail);
         else if (inside == PERIOD)
            put_parm(s_period);
         else if (inside == FMODI)
            put_parm(s_fmod);
         else if (inside == ATANI)
            put_parm(s_atan);
         else
            put_parm( "%d",inside);
         }
      if (closeprox != 0.01 && (inside == EPSCROSS || inside == FMODI
          || outside==FMOD) ) {
         put_parm(" %s=%.15g",s_prox,closeprox);
         }
      if (outside != -1)
      {
         put_parm(" %s=",s_outside);
         if (outside == REAL)
            put_parm(s_real);
         else if (outside == IMAG)
            put_parm(s_imag);
         else if (outside == MULT)
            put_parm(s_mult);
         else if (outside == SUM)
            put_parm(s_sum);
         else if (outside == ATAN)
            put_parm(s_atan);
         else if (outside == FMOD)
            put_parm(s_fmod);
         else if (outside == TDIS)
            put_parm(s_tdis);
         else
            put_parm( "%d",outside);
          }

      if(LogFlag && !rangeslen) {
         put_parm( " %s=",s_logmap);
         if(LogFlag == -1)
            put_parm( "old");
         else if(LogFlag == 1)
            put_parm( s_yes);
         else
            put_parm( "%ld", LogFlag);
         }

      if(Log_Fly_Calc && LogFlag && !rangeslen) {
         put_parm( " %s=",s_logmode);
         if(Log_Fly_Calc == 1)
            put_parm( "fly");
         else if(Log_Fly_Calc == 2)
            put_parm( "table");
         }

      if (potflag) {
       put_parm( " %s=%d/%g/%d",s_potential,
           (int)potparam[0],potparam[1],(int)potparam[2]);
       if(pot16bit)
            put_parm( "/%s",s_16bit);
         }
      if (invert)
         put_parm( " %s=%-1.15lg/%-1.15lg/%-1.15lg",s_invert,
             inversion[0], inversion[1], inversion[2]);
      if (decomp[0])
         put_parm( s_seqd,s_decomp, decomp[0]);
      if (distest) {
         put_parm( s_seqldddd,s_distest, distest, distestwidth,
                     pseudox?pseudox:xdots,pseudoy?pseudoy:ydots);
      }
      if (old_demm_colors)
         put_parm( s_seqy,s_olddemmcolors);
      if (usr_biomorph != -1)
         put_parm( s_seqd,s_biomorph, usr_biomorph);
      if (finattract)
         put_parm(s_seqy,s_finattract);

      if (forcesymmetry != 999) {
         static FCODE msg[] =
            {"Regenerate before <b> to get correct symmetry"};
         if(forcesymmetry == 1000 && ii == 1 && jj == 1)
            stopmsg(0,msg);
         put_parm( " %s=",s_symmetry);
         if (forcesymmetry==XAXIS)
            put_parm(s_xaxis);
         else if(forcesymmetry==YAXIS)
            put_parm(s_yaxis);
         else if(forcesymmetry==XYAXIS)
            put_parm(s_xyaxis);
         else if(forcesymmetry==ORIGIN)
            put_parm(s_origin);
         else if(forcesymmetry==PI_SYM)
            put_parm(s_pi);
         else
            put_parm(s_none);
         }

      if (periodicitycheck != 1)
         put_parm( s_seqd,s_periodicity,periodicitycheck);

      if (rflag)
         put_parm( s_seqd,s_rseed,rseed);

      if (rangeslen) {
         put_parm(" %s=",s_ranges);
         i = 0;
         while (i < rangeslen) {
            if (i)
               put_parm("/");
            if (ranges[i] == -1) {
               put_parm("-%d/",ranges[++i]);
               ++i;
               }
            put_parm("%d",ranges[i++]);
            }
         }
      }

   if (display3d >= 1) {
      /***** 3d transform only parameters in this section *****/
      if(display3d == 2)
         put_parm( s_seqs,s_3d,s_overlay);
      else
      put_parm( s_seqs,s_3d,s_yes);
      if (loaded3d == 0)
         put_filename(s_filename,readname);
      if (SPHERE) {
         put_parm( s_seqy,s_sphere);
         put_parm( s_seqdd,s_latitude, THETA1, THETA2);
         put_parm( s_seqdd,s_longitude, PHI1, PHI2);
         put_parm( s_seqd,s_radius, RADIUS);
         }
      put_parm( s_seqdd,s_scalexyz, XSCALE, YSCALE);
      put_parm( s_seqd,s_roughness, ROUGH);
      put_parm( s_seqd,s_waterline, WATERLINE);
      if (FILLTYPE)
         put_parm( s_seqd,s_filltype, FILLTYPE);
      if (transparent[0] || transparent[1])
         put_parm( s_seqdd,s_transparent, transparent[0],transparent[1]);
      if (preview) {
         put_parm( s_seqs,s_preview,s_yes);
         if (showbox)
            put_parm( s_seqs,s_showbox,s_yes);
         put_parm( s_seqd,s_coarse,previewfactor);
         }
      if (RAY) {
         put_parm( s_seqd,s_ray,RAY);
         if (BRIEF)
            put_parm(s_seqy,s_brief);
         }
      if (FILLTYPE > 4) {
         put_parm( s_seqddd,s_lightsource, XLIGHT, YLIGHT, ZLIGHT);
         if (LIGHTAVG)
            put_parm(s_seqd,s_smoothing, LIGHTAVG);
         }
      if (RANDOMIZE)
         put_parm( s_seqd,s_randomize,RANDOMIZE);
      if (Targa_Out)
         put_parm( s_seqy,s_fullcolor);
      if (grayflag)
         put_parm( s_seqy,s_usegrayscale);
      if (Ambient)
         put_parm( s_seqd,s_ambient,Ambient);
      if (haze)
         put_parm( s_seqd,s_haze,haze);
      if (back_color[0] != 51 || back_color[1] != 153 || back_color[2] != 200)
         put_parm( s_seqddd,s_background,back_color[0],back_color[1],
                   back_color[2]);
      }

   if (display3d) {             /* universal 3d */
      /***** common (fractal & transform) 3d parameters in this section *****/
      if (!SPHERE || display3d < 0)
         put_parm( s_seqddd,s_rotation, XROT, YROT, ZROT);
      put_parm( s_seqd,s_perspective, ZVIEWER);
      put_parm( s_seqdd,s_xyshift, XSHIFT, YSHIFT);
      if(xtrans || ytrans)
         put_parm( s_seqdd,s_xyadjust,xtrans,ytrans);
      if(glassestype) {
         put_parm( s_seqd,s_stereo,glassestype);
         put_parm( s_seqd,s_interocular,eyeseparation);
         put_parm( s_seqd,s_converge,xadjust);
         put_parm( " %s=%d/%d/%d/%d",s_crop,
             red_crop_left,red_crop_right,blue_crop_left,blue_crop_right);
         put_parm( s_seqdd,s_bright,
             red_bright,blue_bright);
         }
      }

   /***** universal parameters in this section *****/

   if(viewwindow == 1)
   {
      put_parm(" %s=%g/%g",s_viewwindows,viewreduction,finalaspectratio);
      if(viewcrop)
         put_parm("/%s",s_yes);
      else
         put_parm("/%s",s_no);
      put_parm("/%d/%d",viewxdots,viewydots);
   }

   if(colorsonly == 0)
   {
   if (rotate_lo != 1 || rotate_hi != 255)
      put_parm( s_seqdd,s_cyclerange,rotate_lo,rotate_hi);

   if(basehertz != 440)
      put_parm(s_seqd,s_hertz,basehertz);

  if(soundflag != 9) {
   if((soundflag&7) == 0)
      put_parm(s_seqs,s_sound,s_off);
   else if((soundflag&7) == 1)
      put_parm(s_seqs,s_sound,s_beep);
   else if((soundflag&7) == 2)
      put_parm(s_seqs,s_sound,s_x);
   else if((soundflag&7) == 3)
      put_parm(s_seqs,s_sound,s_y);
   else if((soundflag&7) == 4)
      put_parm(s_seqs,s_sound,s_z);
#ifndef XFRACT
   if((soundflag&7) && (soundflag&7) <=4) {
      if(soundflag&8)
         put_parm("/pc");
      if(soundflag&16)
         put_parm("/fm");
      if(soundflag&32)
         put_parm("/midi");
      if(soundflag&64)
         put_parm("/quant");
   }
#endif
  }

#ifndef XFRACT
   if(fm_vol != 63)
     put_parm(s_seqd,s_volume,fm_vol);

   if(hi_atten != 0) {
     if(hi_atten == 1)
        put_parm(s_seqs,s_atten,s_low);
     else if(hi_atten == 2)
        put_parm(s_seqs,s_atten,s_mid);
     else if(hi_atten == 3)
        put_parm(s_seqs,s_atten,s_high);
     else   /* just in case */
        put_parm(s_seqs,s_atten,s_none);
   }

   if(polyphony != 0)
     put_parm(s_seqd,s_polyphony,polyphony+1);
   
   if(fm_wavetype !=0)
     put_parm(s_seqd,s_wavetype,fm_wavetype);
   
   if(fm_attack != 5)
      put_parm(s_seqd,s_attack,fm_attack);

   if(fm_decay != 10)
      put_parm(s_seqd,s_decay,fm_decay);

   if(fm_sustain != 13)
      put_parm(s_seqd,s_sustain,fm_sustain);

   if(fm_release != 5)
      put_parm(s_seqd,s_srelease,fm_release);

   if(soundflag&64) { /* quantize turned on */
      for(i=0;i<=11;i++) if(scale_map[i] != i+1) i=15;
      if(i>12) 
         put_parm(s_seqd12,s_scalemap,scale_map[0],scale_map[1],scale_map[2],scale_map[3]
            ,scale_map[4],scale_map[5],scale_map[6],scale_map[7],scale_map[8]
            ,scale_map[9],scale_map[10],scale_map[11]);
   }

#endif

   if(nobof > 0)
      put_parm(s_seqs,s_nobof,s_yes);

   if(orbit_delay > 0)
      put_parm(s_seqd,s_orbitdelay,orbit_delay);

   if(orbit_interval != 1)
      put_parm(s_seqd,s_orbitinterval,orbit_interval);

   if(start_showorbit > 0)
      put_parm(s_seqs,s_showorbit,s_yes);

   if (keep_scrn_coords)
      put_parm(s_seqs,s_screencoords,s_yes);

   if (usr_stdcalcmode == 'o' && set_orbit_corners && keep_scrn_coords)
      {
         int xdigits,ydigits;
         put_parm( " %s=",s_orbitcorners);
         xdigits = getprec(oxmin,oxmax,ox3rd);
         ydigits = getprec(oymin,oymax,oy3rd);
         put_float(0,oxmin,xdigits);
         put_float(1,oxmax,xdigits);
         put_float(1,oymin,ydigits);
         put_float(1,oymax,ydigits);
         if (ox3rd != oxmin || oy3rd != oymin)
         {
            put_float(1,ox3rd,xdigits);
            put_float(1,oy3rd,ydigits);
         }
      }

   if (drawmode != 'r')
      put_parm(" %s=%c",s_orbitdrawmode, drawmode);

   if (math_tol[0] != 0.05 || math_tol[1] != 0.05)
      put_parm(" %s=%g/%g",s_mathtolerance,math_tol[0],math_tol[1]);

   }

   if (*colorinf != 'n')
   {
      if(recordcolors=='c' && *colorinf == '@')
      {
         put_parm_line();
         put_parm("; %s=",s_colors);
         put_parm(colorinf);
         put_parm_line();
      }
docolors:
      put_parm(" %s=",s_colors);
      if (recordcolors !='c' && recordcolors != 'y' && *colorinf == '@')
         put_parm(colorinf);
      else {
         int curc,scanc,force,diffmag = -1;
         int delta,diff1[4][3],diff2[4][3];
         curc = force = 0;
#ifdef XFRACT
         if (fake_lut && !truemode) loaddac(); /* stupid kludge JCO 6/23/2001 */
#endif
         for(;;) {
            /* emit color in rgb 3 char encoded form */
            for (j = 0; j < 3; ++j) {
               if ((k = dacbox[curc][j]) < 10) k += '0';
               else if (k < 36)                k += ('A' - 10);
               else                            k += ('_' - 36);
               buf[j] = (char)k;
               }
            buf[3] = 0;
            put_parm(buf);
            if (++curc >= maxcolor)      /* quit if done last color */
               break;
            if(debugflag == 920)  /* lossless compression */
               continue;
            /* Next a P Branderhorst special, a tricky scan for smooth-shaded
               ranges which can be written as <nn> to compress .par file entry.
               Method used is to check net change in each color value over
               spans of 2 to 5 color numbers.  First time for each span size
               the value change is noted.  After first time the change is
               checked against noted change.  First time it differs, a
               a difference of 1 is tolerated and noted as an alternate
               acceptable change.  When change is not one of the tolerated
               values, loop exits. */
            if (force) {
               --force;
               continue;
               }
            scanc = curc;
            while (scanc < maxcolor) {   /* scan while same diff to next */
               if ((i = scanc - curc) > 3) /* check spans up to 4 steps */
                  i = 3;
               for (k = 0; k <= i; ++k) {
                  for (j = 0; j < 3; ++j) { /* check pattern of chg per color */
                     /* Sylvie Gallet's fix */
                     if (debugflag != 910 && scanc > (curc+4) && scanc < maxcolor-5)
                        if (abs(2*dacbox[scanc][j] - dacbox[scanc-5][j]
                                - dacbox[scanc+5][j]) >= 2)
                           break;
                     /* end Sylvie's fix */       
                     delta = (int)dacbox[scanc][j] - (int)dacbox[scanc-k-1][j];
                     if (k == scanc - curc)
                        diff1[k][j] = diff2[k][j] = delta;
                     else
                        if (delta != diff1[k][j] && delta != diff2[k][j]) {
                           diffmag = abs(delta - diff1[k][j]);
                           if (diff1[k][j] != diff2[k][j] || diffmag != 1)
                              break;
                           diff2[k][j] = delta;
                           }
                     }
                  if (j < 3) break; /* must've exited from inner loop above */
                  }
               if (k <= i) break;   /* must've exited from inner loop above */
               ++scanc;
               }
            /* now scanc-1 is next color which must be written explicitly */
            if (scanc - curc > 2) { /* good, we have a shaded range */
               if (scanc != maxcolor) {
                  if (diffmag < 3) {  /* not a sharp slope change? */
                     force = 2;       /* force more between ranges, to stop  */
                     --scanc;         /* "drift" when load/store/load/store/ */
                     }
                  if (k) {            /* more of the same                    */
                     force += k;
                     --scanc;
                     }
                  }
               if (--scanc - curc > 1) {
                  put_parm("<%d>",scanc-curc);
                  curc = scanc;
                  }
               else                /* changed our mind */
                  force = 0;
               }
            }
         }
      }

   while (wbdata->len) /* flush the buffer */
      put_parm_line();
   /* restore previous boxx data from extraseg */
   memcpy(boxx, saveshared, 10000);
   restore_stack(saved);
}

static void put_filename(char *keyword,char *fname)
{
   char *p;
   if (*fname && !endswithslash(fname)) {
      if ((p = strrchr(fname, SLASHC)) != NULL)
         if (*(fname = p+1) == 0) return;
      put_parm(s_seqs,keyword,fname);
      }
}

#ifndef USE_VARARGS
static void put_parm(char *parm,...)
#else
static void put_parm(va_alist)
va_dcl
#endif
{
   char *bufptr;
   va_list args;

#ifndef USE_VARARGS
   va_start(args,parm);
#else
   char * parm;

   va_start(args);
   parm = va_arg(args,char *);
#endif
   if (*parm == ' '             /* starting a new parm */
     && wbdata->len == 0)       /* skip leading space */
      ++parm;
   bufptr = wbdata->buf + wbdata->len;
   vsprintf(bufptr,parm,args);
   while (*(bufptr++))
      ++wbdata->len;
   while (wbdata->len > 200)
      put_parm_line();
}

int maxlinelength=72;
#define MAXLINELEN  maxlinelength
#define NICELINELEN (MAXLINELEN-4)

static void put_parm_line()
{
   int len,c;
   if ((len = wbdata->len) > NICELINELEN) {
      len = NICELINELEN+1;
      while (--len != 0 && wbdata->buf[len] != ' ') { }
      if (len == 0) {
         len = NICELINELEN-1;
         while (++len < MAXLINELEN
           && wbdata->buf[len] && wbdata->buf[len] != ' ') { }
         }
      }
   c = wbdata->buf[len];
   wbdata->buf[len] = 0;
   fputs("  ",parmfile);
   fputs(wbdata->buf,parmfile);
   if (c && c != ' ')
      fputc('\\',parmfile);
   fputc('\n',parmfile);
   if ((wbdata->buf[len] = (char)c) == ' ')
      ++len;
   wbdata->len -= len;
   strcpy(wbdata->buf,wbdata->buf+len);
}

int getprecbf_mag()
{
   double Xmagfactor, Rotation, Skew;
   LDBL Magnification;
   bf_t bXctr, bYctr;
   int saved,dec;

   saved = save_stack();
   bXctr            = alloc_stack(bflength+2);
   bYctr            = alloc_stack(bflength+2);
   /* this is just to find Magnification */
   cvtcentermagbf(bXctr, bYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
   restore_stack(saved);

   /* I don't know if this is portable, but something needs to */
   /* be used in case compiler's LDBL_MAX is not big enough    */
   if (Magnification > LDBL_MAX || Magnification < -LDBL_MAX)
      return(-1);

   dec = getpower10(Magnification) + 4; /* 4 digits of padding sounds good */
   return(dec);
}

static int getprec(double a,double b,double c)
{
   double diff,temp;
   int digits;
   double highv = 1.0E20;
   if ((diff = fabs(a - b)) == 0.0) diff = highv;
   if ((temp = fabs(a - c)) == 0.0) temp = highv;
   if (temp < diff) diff = temp;
   if ((temp = fabs(b - c)) == 0.0) temp = highv;
   if (temp < diff) diff = temp;
   digits = 7;
   if(debugflag >= 700 && debugflag < 720 )
      digits =  debugflag - 700;
   while (diff < 1.0 && digits <= DBL_DIG+1) {
      diff *= 10;
      ++digits;
      }
   return(digits);
}

/* This function calculates the precision needed to distiguish adjacent
   pixels at Fractint's maximum resolution of MAXPIXELS by MAXPIXELS
   (if rez==MAXREZ) or at current resolution (if rez==CURRENTREZ)    */
int getprecbf(int rezflag)
{
   bf_t del1,del2, one, bfxxdel, bfxxdel2, bfyydel, bfyydel2;
   int digits,dec;
   int saved;
   int rez;
   saved    = save_stack();
   del1     = alloc_stack(bflength+2);
   del2     = alloc_stack(bflength+2);
   one      = alloc_stack(bflength+2);
   bfxxdel   = alloc_stack(bflength+2);
   bfxxdel2  = alloc_stack(bflength+2);
   bfyydel   = alloc_stack(bflength+2);
   bfyydel2  = alloc_stack(bflength+2);
   floattobf(one,1.0);
   if(rezflag == MAXREZ)
      rez = OLDMAXPIXELS -1;
   else
      rez = xdots-1;

   /* bfxxdel = (bfxmax - bfx3rd)/(xdots-1) */
   sub_bf(bfxxdel, bfxmax, bfx3rd);
   div_a_bf_int(bfxxdel, (U16)rez);

   /* bfyydel2 = (bfy3rd - bfymin)/(xdots-1) */
   sub_bf(bfyydel2, bfy3rd, bfymin);
   div_a_bf_int(bfyydel2, (U16)rez);

   if(rezflag == CURRENTREZ)
      rez = ydots-1;

   /* bfyydel = (bfymax - bfy3rd)/(ydots-1) */
   sub_bf(bfyydel, bfymax, bfy3rd);
   div_a_bf_int(bfyydel, (U16)rez);

   /* bfxxdel2 = (bfx3rd - bfxmin)/(ydots-1) */
   sub_bf(bfxxdel2, bfx3rd, bfxmin);
   div_a_bf_int(bfxxdel2, (U16)rez);

   abs_a_bf(add_bf(del1,bfxxdel,bfxxdel2));
   abs_a_bf(add_bf(del2,bfyydel,bfyydel2));
   if(cmp_bf(del2,del1) < 0)
       copy_bf(del1, del2);
   if(cmp_bf(del1,clear_bf(del2)) == 0)
   {
      restore_stack(saved);
      return(-1);
   }
   digits = 1;
   while(cmp_bf(del1,one) < 0)
   {
      digits++;
      mult_a_bf_int(del1,10);
   }
   digits = max(digits,3);
   restore_stack(saved);
   dec = getprecbf_mag();
   return(max(digits,dec));
}

#ifdef _MSC_VER
#pragma optimize("e",off)  /* MSC 7.00 messes up next with "e" on */
#endif

/* This function calculates the precision needed to distiguish adjacent
   pixels at Fractint's maximum resolution of MAXPIXELS by MAXPIXELS
   (if rez==MAXREZ) or at current resolution (if rez==CURRENTREZ)    */
int getprecdbl(int rezflag)
{
   LDBL del1,del2, xdel, xdel2, ydel, ydel2;
   int digits;
   LDBL rez;
   if(rezflag == MAXREZ)
      rez = OLDMAXPIXELS -1;
   else
      rez = xdots-1;

   xdel =  ((LDBL)xxmax - (LDBL)xx3rd)/rez;
   ydel2 = ((LDBL)yy3rd - (LDBL)yymin)/rez;

   if(rezflag == CURRENTREZ)
      rez = ydots-1;

   ydel = ((LDBL)yymax - (LDBL)yy3rd)/rez;
   xdel2 = ((LDBL)xx3rd - (LDBL)xxmin)/rez;

   del1 = fabsl(xdel) + fabsl(xdel2);
   del2 = fabsl(ydel) + fabsl(ydel2);
   if(del2 < del1)
       del1 = del2;
   if(del1 == 0)
   {
#ifdef DEBUG
      showcornersdbl("getprecdbl");
#endif
      return(-1);
   }
   digits = 1;
   while(del1 < 1.0)
   {
      digits++;
      del1 *= 10;
   }
   digits = max(digits,3);
   return(digits);
}

#ifdef _MSC_VER
#pragma optimize("e",on)
#endif

/*
   Strips zeros from the non-exponent part of a number. This logic
   was originally in put_bf(), but is split into this routine so it can be
   shared with put_float(), which had a bug in Fractint 19.2 (used to strip
   zeros from the exponent as well.)
*/

static void strip_zeros(char *buf)
{
   char *dptr, *bptr, *exptr;
   strlwr(buf);
   if ((dptr = strchr(buf,'.')) != 0) {
      ++dptr;
      if ((exptr = strchr(buf,'e')) !=0)  /* scientific notation with 'e'? */
         bptr = exptr;
      else
         bptr = buf + strlen(buf);
      while (--bptr > dptr && *bptr == '0')
         *bptr = 0;
      if(exptr && bptr < exptr -1)
        strcat(buf,exptr);
   }
}

static void put_float(int slash,double fnum,int prec)
{  char buf[40];
   char *bptr;
   bptr = buf;
   if (slash)
      *(bptr++) = '/';
/*   sprintf(bptr,"%1.*f",prec,fnum); */
#ifdef USE_LONG_DOUBLE
   /* Idea of long double cast is to squeeze out another digit or two
      which might be needed (we have found cases where this digit makes
      a difference.) But lets not do this at lower precision */
   if(prec > 15)
      sprintf(bptr,"%1.*Lg",prec,(long double)fnum);
   else
#endif
      sprintf(bptr,"%1.*g",prec,(double)fnum);
   strip_zeros(bptr);
   put_parm(buf);
}

static void put_bf(int slash,bf_t r, int prec)
{
   char *buf; /* "/-1.xxxxxxE-1234" */
   char *bptr;
   /* buf = malloc(decimals+11); */
   buf = wbdata->buf+5000;  /* end of use suffix buffer, 5000 bytes safe */
   bptr = buf;
   if (slash)
      *(bptr++) = '/';
   bftostr(bptr, prec, r);
   strip_zeros(bptr);
   put_parm(buf);
}

#ifndef XFRACT
#include <direct.h>
void shell_to_dos()
{
   int drv;
   char *comspec;
   char curdir[FILE_MAX_DIR],*s;
   if ((comspec = getenv("COMSPEC")) == NULL)
      printf("Cannot find COMMAND.COM.\n");
   else {
      putenv("PROMPT='EXIT' returns to FRACTINT.$_$p$g");
      s = getcwd(curdir,100);
      drv = _getdrive();
      spawnl(P_WAIT, comspec, NULL);
      if(drv)
         _chdrive(drv);
      if(s)
         chdir(s);
      }
}

size_t showstack(void)
{
   return(stackavail());
}

long fr_farfree(void)
{
   long j,j2;
   BYTE huge *fartempptr;
   j = 0;
   j2 = 0x80000L;
   while ((j2 >>= 1) != 0)
      if ((fartempptr = (BYTE huge *)malloc(j+j2)) != NULL) {
         free((void *)fartempptr);
         j += j2;
         }
   return(j);
}

void showfreemem(void)
{
   char *tempptr;
   unsigned i,i2;

   char adapter_name[8];        /* entry lenth from VIDEO.ASM */
   char *adapter_ptr;

   printf("\n CPU type: %d  FPU type: %d  Video: %d",
          cpu, fpu, video_type);

   adapter_ptr = &supervga_list;

   for(i = 0 ; ; i++) {         /* find the SuperVGA entry */
       int j;
       memcpy(adapter_name , adapter_ptr, 8);
       adapter_ptr += 8;
       if (adapter_name[0] == ' ') break;       /* end-of-the-list */
       if (adapter_name[6] == 0) continue;      /* not our adapter */
       adapter_name[6] = ' ';
       for (j = 0; j < 8; j++)
           if(adapter_name[j] == ' ')
               adapter_name[j] = 0;
       printf("  Video chip: %d (%s)",i+1,adapter_name);
       }
   printf("\n\n");

   i = 0;
   i2 = 0x8000;
   while ((i2 >>= 1) != 0)
      if ((tempptr = malloc(i+i2)) != NULL) {
         free(tempptr);
         i += i2;
         }
   printf(" %d NEAR bytes free \n", i);

   printf(" %ld FAR bytes free ", fr_farfree());
   {
      size_t stack;
      stack = showstack();
/*      if(stack >= 0) */ /* stack is unsigned */
         printf("\n %u STACK bytes free",stack);
   }
   printf("\n %ld used by HISTORY structure",
      sizeof(HISTORY)*(unsigned long)maxhistory);
   printf("\n %d video table used",showvidlength());
   printf("\n\n %Fs...\n",s_pressanykeytocontinue);
   getakey();
}
#endif

int edit_text_colors()
{
   int save_debugflag,save_lookatmouse;
   int row,col,bkgrd;
   int rowf,colf,rowt,colt;
   char *vidmem;
   char *savescreen;
   char *farp1; char *farp2;
   int i,j,k;
   save_debugflag = debugflag;
   save_lookatmouse = lookatmouse;
   debugflag = 0;   /* don't get called recursively */
   lookatmouse = 2; /* text mouse sensitivity */
   row = col = bkgrd = rowt = rowf = colt = colf = 0;
   /* TODO: allocate real memory, not reuse shared segment */
   vidmem = MK_FP(0xB800,0);
   for(;;) {
      if (row < 0)  row = 0;
      if (row > 24) row = 24;
      if (col < 0)  col = 0;
      if (col > 79) col = 79;
      driver_move_cursor(row,col);
      i = getakey();
      if (i >= 'a' && i <= 'z') i -= 32; /* uppercase */
      switch (i) {
         case 27: /* esc */
            debugflag = save_debugflag;
            lookatmouse = save_lookatmouse;
            driver_hide_text_cursor();
            return 0;
         case '/':
            farp1 = savescreen = (char *)malloc(4000L);
            farp2 = vidmem;
            for (i = 0; i < 4000; ++i) { /* save and blank */
               *(farp1++) = *farp2;
               *(farp2++) = 0;
               }
            for (i = 0; i < 8; ++i)       /* 8 bkgrd attrs */
               for (j = 0; j < 16; ++j) { /* 16 fgrd attrs */
                  k = i*16 + j;
                  farp1 = vidmem + i*320 + j*10;
                  *(farp1++) = ' '; *(farp1++) = (char)k;
                  *(farp1++) = (char)(i+'0'); *(farp1++) = (char)k;
                  *(farp1++) = (char)((j < 10) ? j+'0' : j+'A'-10); *(farp1++) = (char)k;
                  *(farp1++) = ' '; *(farp1++) = (char)k;
                  }
            getakey();
            farp1 = vidmem;
            farp2 = savescreen;
            for (i = 0; i < 4000; ++i) /* restore */
               *(farp1++) = *(farp2++);
            free(savescreen);
            break;
         case ',':
            rowf = row; colf = col; break;
         case '.':
            rowt = row; colt = col; break;
         case ' ': /* next color is background */
            bkgrd = 1; break;
         case 1075: /* cursor left  */
            --col; break;
         case 1077: /* cursor right */
            ++col; break;
         case 1072: /* cursor up    */
            --row; break;
         case 1080: /* cursor down  */
            ++row; break;
         case 13:   /* enter */
            *(vidmem + row*160 + col*2) = (char)getakey();
            break;
         default:
            if (i >= '0' && i <= '9')      i -= '0';
            else if (i >= 'A' && i <= 'F') i -= 'A'-10;
            else break;
            for (j = rowf; j <= rowt; ++j)
               for (k = colf; k <= colt; ++k) {
                  farp1 = vidmem + j*160 + k*2 + 1;
                  if (bkgrd) *farp1 = (char)((*farp1 & 15) + i * 16);
                  else       *farp1 = (char)((*farp1 & 0xf0) + i);
                  }
            bkgrd = 0;
         }
      }
}

static int *entsptr;
static int modes_changed;

int select_video_mode(int curmode)
{
   static FCODE o_hdg2[]={"key...name.......................xdot..ydot.colr.comment.................."};
   static FCODE o_hdg1[]={"Select Video Mode"};
   char hdg2[sizeof(o_hdg2)];
   char hdg1[sizeof(o_hdg1)];

   int entnums[MAXVIDEOMODES];
   int attributes[MAXVIDEOMODES];
   int i,k,ret;
#ifndef XFRACT
   int j;
   int oldtabmode,oldhelpmode;
#endif

   load_fractint_cfg(0);        /* load fractint.cfg to extraseg */

   strcpy(hdg1,o_hdg1);
   strcpy(hdg2,o_hdg2);

   for (i = 0; i < vidtbllen; ++i) { /* init tables */
      entnums[i] = i;
      attributes[i] = 1;
      }
   entsptr = entnums;           /* for indirectly called subroutines */

   qsort(entnums,vidtbllen,sizeof(entnums[0]),entcompare); /* sort modes */

   /* pick default mode */
   if (curmode < 0) {
      switch (video_type) { /* set up a reasonable default (we hope) */
         case 1:  videoentry.videomodeax = 8;   /* hgc */
                  videoentry.colors = 2;
                  break;
         case 2:  videoentry.videomodeax = 4;   /* cga */
                  videoentry.colors = 4;
                  break;
         case 3:  videoentry.videomodeax = 16;  /* ega */
                  videoentry.colors = 16;
                  if (mode7text) {              /* egamono */
                     videoentry.videomodeax = 15;
                     videoentry.colors = 2;
                     }
                  break;
         default: videoentry.videomodeax = 19;  /* mcga/vga? */
                  videoentry.colors = 256;
                  break;
         }
      }
   else
      memcpy((char *)&videoentry,(char *)&videotable[curmode],
                 sizeof(videoentry));
#ifndef XFRACT
   for (i = 0; i < vidtbllen; ++i) { /* find default mode */
      if ( videoentry.videomodeax == vidtbl[entnums[i]].videomodeax
        && videoentry.colors      == vidtbl[entnums[i]].colors
        && (curmode < 0
            || memcmp((char *)&videoentry,(char *)&vidtbl[entnums[i]],
                          sizeof(videoentry)) == 0))
         break;
      }
   if (i >= vidtbllen) /* no match, default to first entry */
      i = 0;

   oldtabmode = tabmode;
   oldhelpmode = helpmode;
   modes_changed = 0;
   tabmode = 0;
   helpmode = HELPVIDSEL;
   i = fullscreen_choice(CHOICEHELP,hdg1,hdg2,NULL,vidtbllen,NULL,attributes,
                         1,16,74,i,format_vid_table,NULL,NULL,check_modekey);
   tabmode = oldtabmode;
   helpmode = oldhelpmode;
   if (i == -1) {
   static FCODE msg[]={"Save new function key assignments or cancel changes?"};
      if (modes_changed /* update fractint.cfg for new key assignments */
        && badconfig == 0
        && stopmsg(22,msg) == 0)
         update_fractint_cfg();
      return(-1);
      }
   if (i < 0)   /* picked by function key */
      i = -1 - i;
   else         /* picked by Enter key */
      i = entnums[i];
#endif
   memcpy((char *)&videoentry,(char *)&vidtbl[i],
              sizeof(videoentry));  /* the selected entry now in videoentry */

#ifndef XFRACT
   /* copy fractint.cfg table to resident table, note selected entry */
   j = k = 0;
   memset((char *)videotable,0,sizeof(*vidtbl)*MAXVIDEOTABLE);
   for (i = 0; i < vidtbllen; ++i) {
      if (vidtbl[i].keynum > 0) {
         memcpy((char *)&videotable[j],(char *)&vidtbl[i],
                    sizeof(*vidtbl));
         if (memcmp((char *)&videoentry,(char *)&vidtbl[i],
                        sizeof(videoentry)) == 0)
            k = vidtbl[i].keynum;
         if (++j >= MAXVIDEOTABLE-1)
            break;
         }
      }
#else
    k = vidtbl[0].keynum;
#endif
   if ((ret = k) == 0) { /* selected entry not a copied (assigned to key) one */
      memcpy((char *)&videotable[MAXVIDEOTABLE-1],
                 (char *)&videoentry,sizeof(*vidtbl));
      ret = 1400; /* special value for check_vidmode_key */
      }

   if (modes_changed /* update fractint.cfg for new key assignments */
     && badconfig == 0)
      update_fractint_cfg();

   return(ret);
}

void format_vid_table(int choice,char *buf)
{
   char local_buf[81];
   char kname[5];
   char biosflag;
   int truecolorbits;
   memcpy((char *)&videoentry,(char *)&vidtbl[entsptr[choice]],
              sizeof(videoentry));
   vidmode_keyname(videoentry.keynum,kname);
   biosflag = (char)((videoentry.dotmode % 100 == 1) ? 'B' : ' ');
   sprintf(buf,"%-5s %-25s %5d %5d ",  /* 44 chars */
           kname, videoentry.name, videoentry.xdots, videoentry.ydots);
   if((truecolorbits = videoentry.dotmode/1000) == 0)
      sprintf(local_buf,"%s%3d",  /* 47 chars */
           buf, videoentry.colors);
   else 
      sprintf(local_buf,"%s%3s",  /* 47 chars */
           buf, (truecolorbits == 4)?" 4g":
                (truecolorbits == 3)?"16m":
                (truecolorbits == 2)?"64k":
                (truecolorbits == 1)?"32k":"???");
   sprintf(buf,"%s%c %-25s",  /* 74 chars */
           local_buf, biosflag, videoentry.comment);
}

#ifndef XFRACT
static int check_modekey(int curkey,int choice)
{
   int i,j,k,ret;
   if ((i = check_vidmode_key(1,curkey)) >= 0)
      return(-1-i);
   i = entsptr[choice];
   ret = 0;
   if ( (curkey == '-' || curkey == '+')
     && (vidtbl[i].keynum == 0 || vidtbl[i].keynum >= 1084)) {
      static FCODE msg[]={"Missing or bad FRACTINT.CFG file. Can't reassign keys."};
      if (badconfig)
         stopmsg(0,msg);
      else {
         if (curkey == '-') {                   /* deassign key? */
            if (vidtbl[i].keynum >= 1084) {
               vidtbl[i].keynum = 0;
               modes_changed = 1;
               }
            }
         else {                                 /* assign key? */
            j = getakeynohelp();
            if (j >= 1084 && j <= 1113) {
               for (k = 0; k < vidtbllen; ++k) {
                  if (vidtbl[k].keynum == j) {
                     vidtbl[k].keynum = 0;
                     ret = -1; /* force redisplay */
                     }
                  }
               vidtbl[i].keynum = j;
               modes_changed = 1;
               }
            }
         }
      }
   return(ret);
}
#endif

static int entcompare(VOIDCONSTPTR p1,VOIDCONSTPTR p2)
{
   int i,j;
   if ((i = vidtbl[*((int *)p1)].keynum) == 0) i = 9999;
   if ((j = vidtbl[*((int *)p2)].keynum) == 0) j = 9999;
   if (i < j || (i == j && *((int *)p1) < *((int *)p2)))
      return(-1);
   return(1);
}

static void update_fractint_cfg()
{
#ifndef XFRACT
   char cfgname[100],outname[100],buf[121],kname[5];
   FILE *cfgfile,*outfile;
   int *cfglinenums;
   int i,j,linenum,nextlinenum,nextmode;
   struct videoinfo vident;

   findpath("fractint.cfg",cfgname);

   if (access(cfgname,6)) {
      sprintf(buf,s_cantwrite,cfgname);
      stopmsg(0,buf);
      return;
      }
   strcpy(outname,cfgname);
   i = (int) strlen(outname);
   while (--i >= 0 && outname[i] != SLASHC)
   outname[i] = 0;
   strcat(outname,"fractint.tmp");
   if ((outfile = fopen(outname,"w")) == NULL) {
      sprintf(buf,s_cantcreate,outname);
      stopmsg(0,buf);
      return;
      }
   cfgfile = fopen(cfgname,"r");

   cfglinenums = (int *)(&vidtbl[MAXVIDEOMODES]);
   linenum = nextmode = 0;
   nextlinenum = cfglinenums[0];
   while (fgets(buf,120,cfgfile)) {
      int truecolorbits;
      char colorsbuf[10];
      ++linenum;
      if (linenum == nextlinenum) { /* replace this line */
         memcpy((char *)&vident,(char *)&vidtbl[nextmode],
                    sizeof(videoentry));
         vidmode_keyname(vident.keynum,kname);
         strcpy(buf,vident.name);
         i = (int) strlen(buf);
         while (i && buf[i-1] == ' ') /* strip trailing spaces to compress */
            --i;
         j = i + 5;
         while (j < 32) {               /* tab to column 33 */
            buf[i++] = '\t';
            j += 8;
            }
         buf[i] = 0;
         if((truecolorbits = vident.dotmode/1000) == 0)
            sprintf(colorsbuf,"%3d",vident.colors);
         else 
            sprintf(colorsbuf,"%3s",
                    (truecolorbits == 4)?" 4g":
                    (truecolorbits == 3)?"16m":
                    (truecolorbits == 2)?"64k":
                    (truecolorbits == 1)?"32k":"???");
         fprintf(outfile,"%-4s,%s,%4x,%4x,%4x,%4x,%4d,%5d,%5d,%s,%s\n",
                kname,
                buf,
                vident.videomodeax,
                vident.videomodebx,
                vident.videomodecx,
                vident.videomodedx,
                vident.dotmode%1000, /* remove true-color flag, keep textsafe */
                vident.xdots,
                vident.ydots,
                colorsbuf,
                vident.comment);
         if (++nextmode >= vidtbllen)
            nextlinenum = 32767;
         else
            nextlinenum = cfglinenums[nextmode];
         }
      else
         fputs(buf,outfile);
      }

   fclose(cfgfile);
   fclose(outfile);
   unlink(cfgname);         /* success assumed on these lines       */
   rename(outname,cfgname); /* since we checked earlier with access */
#endif
}

/* make_mig() takes a collection of individual GIF images (all
   presumably the same resolution and all presumably generated
   by Fractint and its "divide and conquer" algorithm) and builds
   a single multiple-image GIF out of them.  This routine is
   invoked by the "batch=stitchmode/x/y" option, and is called
   with the 'x' and 'y' parameters
*/

void make_mig(unsigned int xmult, unsigned int ymult)
{
unsigned int xstep, ystep;
unsigned int xres, yres;
unsigned int allxres, allyres, xtot, ytot;
unsigned int xloc, yloc;
unsigned char ichar;
unsigned int allitbl, itbl;
unsigned int i;
char gifin[15], gifout[15];
int errorflag, inputerrorflag;
unsigned char *temp;
FILE *out, *in;
char msgbuf[81];

errorflag = 0;                          /* no errors so */
inputerrorflag = 0;
allxres = allyres = allitbl = 0;
out = in = NULL;

strcpy(gifout,"fractmig.gif");

temp= &olddacbox[0][0];                 /* a safe place for our temp data */

gif87a_flag = 1;                        /* for now, force this */

/* process each input image, one at a time */
for (ystep = 0; ystep < ymult; ystep++) {
    for (xstep = 0; xstep < xmult; xstep++) {

if (xstep == 0 && ystep == 0) {         /* first time through? */
    static FCODE msg1[] = "Cannot create output file %s!\n";
    static FCODE msg2[] = " \n Generating multi-image GIF file %s using";
    static FCODE msg3[] = " %d X and %d Y components\n\n";
    strcpy(msgbuf, msg2);
    printf(msgbuf, gifout);
    strcpy(msgbuf, msg3);
    printf(msgbuf, xmult, ymult);
    /* attempt to create the output file */
    if ((out = fopen(gifout,"wb")) == NULL) {
        strcpy(msgbuf, msg1);
        printf(msgbuf, gifout);
        exit(1);
        }
    }

        sprintf(gifin, "frmig_%c%c.gif", PAR_KEY(xstep), PAR_KEY(ystep));

        if ((in = fopen(gifin,"rb")) == NULL) {
            static FCODE msg1[] = "Can't open file %s!\n";
            strcpy(msgbuf, msg1);
            printf(msgbuf, gifin);
            exit(1);
            }

        /* (read, but only copy this if it's the first time through) */
        if (fread(temp,13,1,in) != 1)   /* read the header and LDS */
            inputerrorflag = 1;
        memcpy(&xres, &temp[6], 2);     /* X-resolution */
        memcpy(&yres, &temp[8], 2);     /* Y-resolution */

        if (xstep == 0 && ystep == 0) { /* first time through? */
            allxres = xres;             /* save the "master" resolution */
            allyres = yres;
            xtot = xres * xmult;        /* adjust the image size */
            ytot = yres * ymult;
            memcpy(&temp[6], &xtot, 2);
            memcpy(&temp[8], &ytot, 2);
            if (gif87a_flag) {
                temp[3] = '8';
                temp[4] = '7';
                temp[5] = 'a';
                }
            temp[12] = 0; /* reserved */
            if (fwrite(temp,13,1,out) != 1)     /* write out the header */
                errorflag = 1;
            }                           /* end of first-time-through */


        ichar = (char)(temp[10] & 0x07);        /* find the color table size */
        itbl = 1 << (++ichar);
        ichar = (char)(temp[10] & 0x80);        /* is there a global color table? */
        if (xstep == 0 && ystep == 0)   /* first time through? */
            allitbl = itbl;             /* save the color table size */
        if (ichar != 0) {               /* yup */
            /* (read, but only copy this if it's the first time through) */
            if(fread(temp,3*itbl,1,in) != 1)    /* read the global color table */
                inputerrorflag = 2;
            if (xstep == 0 && ystep == 0)       /* first time through? */
                if (fwrite(temp,3*itbl,1,out) != 1)     /* write out the GCT */
                    errorflag = 2;
            }

        if (xres != allxres || yres != allyres || itbl != allitbl) {
            /* Oops - our pieces don't match */
            static FCODE msg1[] = "File %s doesn't have the same resolution as its predecessors!\n";
            strcpy(msgbuf, msg1);
            printf(msgbuf, gifin);
            exit(1);
            }

        for (;;) {                      /* process each information block */
        memset(temp,0,10);
        if (fread(temp,1,1,in) != 1)    /* read the block identifier */
            inputerrorflag = 3;

        if (temp[0] == 0x2c) {          /* image descriptor block */
            if (fread(&temp[1],9,1,in) != 1)    /* read the Image Descriptor */
                inputerrorflag = 4;
            memcpy(&xloc, &temp[1], 2); /* X-location */
            memcpy(&yloc, &temp[3], 2); /* Y-location */
            xloc += (xstep * xres);     /* adjust the locations */
            yloc += (ystep * yres);
            memcpy(&temp[1], &xloc, 2);
            memcpy(&temp[3], &yloc, 2);
            if (fwrite(temp,10,1,out) != 1)     /* write out the Image Descriptor */
                errorflag = 4;

            ichar = (char)(temp[9] & 0x80);     /* is there a local color table? */
            if (ichar != 0) {           /* yup */
                if (fread(temp,3*itbl,1,in) != 1)       /* read the local color table */
                    inputerrorflag = 5;
                if (fwrite(temp,3*itbl,1,out) != 1)     /* write out the LCT */
                    errorflag = 5;
                }

            if (fread(temp,1,1,in) != 1)        /* LZH table size */
                inputerrorflag = 6;
            if (fwrite(temp,1,1,out) != 1)
                errorflag = 6;
            for(;;) {
                if (errorflag != 0 || inputerrorflag != 0)      /* oops - did something go wrong? */
                    break;
                if (fread(temp,1,1,in) != 1)    /* block size */
                    inputerrorflag = 7;
                if (fwrite(temp,1,1,out) != 1)
                    errorflag = 7;
                if ((i = temp[0]) == 0)
                    break;
                if (fread(temp,i,1,in) != 1)    /* LZH data block */
                    inputerrorflag = 8;
                if (fwrite(temp,i,1,out) != 1)
                    errorflag = 8;
                }
            }

        if (temp[0] == 0x21) {          /* extension block */
            /* (read, but only copy this if it's the last time through) */
            if (fread(&temp[2],1,1,in) != 1)    /* read the block type */
                inputerrorflag = 9;
            if ((!gif87a_flag) && xstep == xmult-1 && ystep == ymult-1)
                if (fwrite(temp,2,1,out) != 1)
                    errorflag = 9;
            for(;;) {
                if (errorflag != 0 || inputerrorflag != 0)      /* oops - did something go wrong? */
                    break;
                if (fread(temp,1,1,in) != 1)    /* block size */
                    inputerrorflag = 10;
                if ((!gif87a_flag) && xstep == xmult-1 && ystep == ymult-1)
                    if (fwrite(temp,1,1,out) != 1)
                        errorflag = 10;
                if ((i = temp[0]) == 0)
                    break;
                if (fread(temp,i,1,in) != 1)    /* data block */
                    inputerrorflag = 11;
                if ((!gif87a_flag) && xstep == xmult-1 && ystep == ymult-1)
                    if (fwrite(temp,i,1,out) != 1)
                        errorflag = 11;
                }
            }

        if (temp[0] == 0x3b) {          /* end-of-stream indicator */
            break;                      /* done with this file */
            }

        if (errorflag != 0 || inputerrorflag != 0)      /* oops - did something go wrong? */
            break;

        }
        fclose(in);                     /* done with an input GIF */

        if (errorflag != 0 || inputerrorflag != 0)      /* oops - did something go wrong? */
            break;
        }

    if (errorflag != 0 || inputerrorflag != 0)  /* oops - did something go wrong? */
        break;
    }

temp[0] = 0x3b;                 /* end-of-stream indicator */
if (fwrite(temp,1,1,out) != 1)
    errorflag = 12;
fclose(out);                    /* done with the output GIF */

if (inputerrorflag != 0) {      /* uh-oh - something failed */
    static FCODE msg1[] = "\007 Process failed = early EOF on input file %s\n";
    strcpy(msgbuf, msg1);
    printf(msgbuf, gifin);
/* following line was for debugging
    printf("inputerrorflag = %d\n", inputerrorflag);
*/
    }

if (errorflag != 0) {           /* uh-oh - something failed */
    static FCODE msg1[] = "\007 Process failed = out of disk space?\n";
    strcpy(msgbuf, msg1);
    printf(msgbuf);
/* following line was for debugging
    printf("errorflag = %d\n", errorflag);
*/
    }

/* now delete each input image, one at a time */
if (errorflag == 0 && inputerrorflag == 0)
  for (ystep = 0; ystep < ymult; ystep++) {
    for (xstep = 0; xstep < xmult; xstep++) {
        sprintf(gifin, "frmig_%c%c.gif", PAR_KEY(xstep), PAR_KEY(ystep));
        remove(gifin);
        }
    }

/* tell the world we're done */
if (errorflag == 0 && inputerrorflag == 0) {
    static FCODE msg1[] = "File %s has been created (and its component files deleted)\n";
    strcpy(msgbuf, msg1);
    printf(msgbuf, gifout);
    }
}

/* This routine copies the current screen to by flipping x-axis, y-axis,
   or both. Refuses to work if calculation in progress or if fractal
   non-resumable. Clears zoombox if any. Resets corners so resulting fractal
   is still valid. */
void flip_image(int key)
{
   int i, j, ixhalf, iyhalf, tempdot;

   /* fractal must be rotate-able and be finished */
   if ((curfractalspecific->flags&NOROTATE) != 0
       || calc_status == 1
       || calc_status == 2)
      return;
   if(bf_math)
       clear_zoombox(); /* clear, don't copy, the zoombox */
   ixhalf = xdots / 2;
   iyhalf = ydots / 2;
   switch(key)
   {
   case 24:            /* control-X - reverse X-axis */
      for (i = 0; i < ixhalf; i++)
      {
         if(keypressed())
            break;
         for (j = 0; j < ydots; j++)
         {
            tempdot=getcolor(i,j);
            putcolor(i, j, getcolor(xdots-1-i,j));
            putcolor(xdots-1-i, j, tempdot);
         }
      }
      sxmin = xxmax + xxmin - xx3rd;
      symax = yymax + yymin - yy3rd;
      sxmax = xx3rd;
      symin = yy3rd;
      sx3rd = xxmax;
      sy3rd = yymin;
      if(bf_math)
      {
         add_bf(bfsxmin, bfxmax, bfxmin); /* sxmin = xxmax + xxmin - xx3rd; */
         sub_a_bf(bfsxmin, bfx3rd);
         add_bf(bfsymax, bfymax, bfymin); /* symax = yymax + yymin - yy3rd; */
         sub_a_bf(bfsymax, bfy3rd);
         copy_bf(bfsxmax, bfx3rd);        /* sxmax = xx3rd; */
         copy_bf(bfsymin, bfy3rd);        /* symin = yy3rd; */
         copy_bf(bfsx3rd, bfxmax);        /* sx3rd = xxmax; */
         copy_bf(bfsy3rd, bfymin);        /* sy3rd = yymin; */
      }
      break;
   case 25:            /* control-Y - reverse Y-aXis */
      for (j = 0; j < iyhalf; j++)
      {
         if(keypressed())
            break;
         for (i = 0; i < xdots; i++)
         {
            tempdot=getcolor(i,j);
            putcolor(i, j, getcolor(i,ydots-1-j));
            putcolor(i,ydots-1-j, tempdot);
         }
      }
      sxmin = xx3rd;
      symax = yy3rd;
      sxmax = xxmax + xxmin - xx3rd;
      symin = yymax + yymin - yy3rd;
      sx3rd = xxmin;
      sy3rd = yymax;
      if(bf_math)
      {
         copy_bf(bfsxmin, bfx3rd);        /* sxmin = xx3rd; */
         copy_bf(bfsymax, bfy3rd);        /* symax = yy3rd; */
         add_bf(bfsxmax, bfxmax, bfxmin); /* sxmax = xxmax + xxmin - xx3rd; */
         sub_a_bf(bfsxmax, bfx3rd);
         add_bf(bfsymin, bfymax, bfymin); /* symin = yymax + yymin - yy3rd; */
         sub_a_bf(bfsymin, bfy3rd);
         copy_bf(bfsx3rd, bfxmin);        /* sx3rd = xxmin; */
         copy_bf(bfsy3rd, bfymax);        /* sy3rd = yymax; */
      }
      break;
   case 26:            /* control-Z - reverse X and Y aXis */
      for (i = 0; i < ixhalf; i++)
      {
         if(keypressed())
            break;
         for (j = 0; j < ydots; j++)
         {
            tempdot=getcolor(i,j);
            putcolor(i, j, getcolor(xdots-1-i,ydots-1-j));
            putcolor(xdots-1-i, ydots-1-j, tempdot);
         }
      }
      sxmin = xxmax;
      symax = yymin;
      sxmax = xxmin;
      symin = yymax;
      sx3rd = xxmax + xxmin - xx3rd;
      sy3rd = yymax + yymin - yy3rd;
      if(bf_math)
      {
         copy_bf(bfsxmin, bfxmax);        /* sxmin = xxmax; */
         copy_bf(bfsymax, bfymin);        /* symax = yymin; */
         copy_bf(bfsxmax, bfxmin);        /* sxmax = xxmin; */
         copy_bf(bfsymin, bfymax);        /* symin = yymax; */
         add_bf(bfsx3rd, bfxmax, bfxmin); /* sx3rd = xxmax + xxmin - xx3rd; */
         sub_a_bf(bfsx3rd, bfx3rd);
         add_bf(bfsy3rd, bfymax, bfymin); /* sy3rd = yymax + yymin - yy3rd; */
         sub_a_bf(bfsy3rd, bfy3rd);
      }
      break;
   }
   reset_zoom_corners();
   calc_status = 0;
}
static char *expand_var(char *var, char *buf)
{
   static FCODE s_year    [] = {"year"    };
   static FCODE s_month   [] = {"month"   };
   static FCODE s_day     [] = {"day"     };
   static FCODE s_hour    [] = {"hour"    };
   static FCODE s_min     [] = {"min"     };
   static FCODE s_sec     [] = {"sec"     };
   static FCODE s_time    [] = {"time"    };
   static FCODE s_date    [] = {"date"    };
   static FCODE s_calctime[] = {"calctime"};
   static FCODE s_version [] = {"version" };
   static FCODE s_patch   [] = {"patch"   };
   static FCODE s_xdots   [] = {"xdots"   };
   static FCODE s_ydots   [] = {"ydots"   };
   static FCODE s_vidkey  [] = {"vidkey"  };
   
   time_t ltime;
   char *str, *out;
   
   time( &ltime );
   str = ctime(&ltime);

   /* ctime format             */
   /* Sat Aug 17 21:34:14 1996 */
   /* 012345678901234567890123 */
   /*           1         2    */   
   if(strcmp(var,s_year) == 0)       /* 4 chars */
   {
      str[24] = '\0';
      out = &str[20];
   }
   else if(strcmp(var,s_month) == 0) /* 3 chars */
   {
      str[7] = '\0';
      out = &str[4];
   }
   else if(strcmp(var,s_day) == 0)   /* 2 chars */
   {
      str[10] = '\0';
      out = &str[8];
   }
   else if(strcmp(var,s_hour) == 0)  /* 2 chars */
   {
      str[13] = '\0';
      out = &str[11];
   }
   else if(strcmp(var,s_min) == 0)   /* 2 chars */
   {
      str[16] = '\0';
      out = &str[14];
   }
   else if(strcmp(var,s_sec) == 0)   /* 2 chars */
   {
      str[19] = '\0';
      out = &str[17];
   }
   else if(strcmp(var,s_time) == 0)  /* 8 chars */
   {
      str[19] = '\0';
      out = &str[11];
   }
   else if(strcmp(var,s_date) == 0)
   {
      str[10] = '\0';
      str[24] = '\0';
      out = &str[4];
      strcat(out,", ");
      strcat(out,&str[20]);
   }
   else if(strcmp(var,s_calctime) == 0)
   {
      get_calculation_time(buf,calctime);
      out = buf;
   }
   else if(strcmp(var,s_version) == 0)  /* 4 chars */
   {
      sprintf(buf,"%d",release);
      out = buf;
   }
   else if(strcmp(var,s_patch) == 0)   /* 1 or 2 chars */
   {
      sprintf(buf,"%d",patchlevel);
      out = buf;
   }
   else if(strcmp(var,s_xdots) == 0)   /* 2 to 4 chars */
   {
      sprintf(buf,"%d",xdots);
      out = buf;
   }
   else if(strcmp(var,s_ydots) == 0)   /* 2 to 4 chars */
   {
      sprintf(buf,"%d",ydots);
      out = buf;
   }
   else if(strcmp(var,s_vidkey) == 0)   /* 2 to 3 chars */
   {
      char vidmde[5];
      vidmode_keyname(videoentry.keynum, vidmde);
      sprintf(buf,"%s",vidmde);
      out = buf;
   }
   else
   {
      static char msg[] = {"Unknown comment variable xxxxxxxxxxxxxxx"};
      msg[25] = '\0';
      strcat(msg,var);
      stopmsg(0,msg);
      out = "";
   }
   return(out);
}

#define MAXVNAME  13

static const char esc_char = '$';

/* extract comments from the comments= command */
void expand_comments(char *target, char *source)
{
   int i,j, k, escape = 0;
   char c, oldc, varname[MAXVNAME];
   i=j=k=0;
   c = oldc = 0;
   while(i < MAXCMT && j < MAXCMT && (c = *(source+i++)) != '\0')
   {
      if(c == '\\' && oldc != '\\')
      {
         oldc = c;
         continue;
      }
      /* expand underscores to blanks */
      if(c == '_' && oldc != '\\')
         c = ' ';
      /* esc_char marks start and end of variable names */
      if(c == esc_char && oldc != '\\')
         escape = 1 - escape;
      if(c != esc_char && escape != 0) /* if true, building variable name */
      {
         if(k < MAXVNAME-1)
            varname[k++] = c;
      }
      /* got variable name */
      else if(c == esc_char && escape == 0 && oldc != '\\')
      {
         char buf[100];
         char *varstr;
         varname[k] = 0;
         varstr = expand_var(varname,buf);
         strncpy(target+j,varstr,MAXCMT-j-1);
         j += (int) strlen(varstr);
      }
      else if (c == esc_char && escape != 0 && oldc != '\\')
         k = 0;
      else if ((c != esc_char || oldc == '\\') && escape == 0)
         *(target+j++) = c;
      oldc = c;
   }   
   if(*source != '\0')
      *(target+min(j,MAXCMT-1)) = '\0';
}

/* extract comments from the comments= command */
void parse_comments(char *value)
{
   int i;
   char *next,save;
   for(i=0;i<4;i++)
   {
      save = '\0';
      if (*value == 0) 
         break;
      next = strchr(value,'/');
      if (*value != '/') 
      {
         if(next != NULL)
         {
            save = *next;
            *next = '\0';
         }
         strncpy(par_comment[i],value, MAXCMT);
      }
      if(next == NULL)
         break;
      if(save != '\0')
         *next = save;
      value = next+1;
   }
}
   
void init_comments()
{
   int i;
   for(i=0;i<4;i++)
      par_comment[i][0] = '\0';
}
