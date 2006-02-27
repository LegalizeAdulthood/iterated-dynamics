/*
   non-windows main driver of Fractint for Windows -
*/

void win_cmdfiles(void);

#include "port.h"
#include "prototyp.h"

#include <windows.h>
#include <search.h>
#include <string.h>
#include <float.h>
#include <ctype.h>
#include <time.h>
#include <malloc.h>

#include "winfract.h"
#include "fractype.h"

struct videoinfo videoentry;
int helpmode;

LPSTR win_lpCmdLine;

extern int time_to_act;
extern int time_to_restart;
extern int time_to_reinit;
extern int time_to_resume;
extern int time_to_quit;
extern int time_to_load;
extern int time_to_save;
extern int time_to_print;
extern int time_to_cycle;
extern int time_to_starfield;
extern int time_to_orbit;

extern char FileName[];

extern long maxiter;
extern int ytop, ybottom, xleft, xright;

int fractype;

double xxmin, xxmax, yymin, yymax;

long maxit;
int boxcount;
int bitshift;
extern        int colorpreloaded;        /* comments in cmdfiles */

int calc_status; /* -1 no fractal                    */
                 /*  0 parms changed, recalc reqd   */
                 /*  1 actively calculating            */
                 /*  2 interrupted, resumable            */
                 /*  3 interrupted, not resumable   */
                 /*  4 completed                    */

char usr_stdcalcmode;
long  usr_distest;

long delx, dely, delx2, dely2, delmin;
long xmin, xmax, ymin, ymax, x3rd, y3rd;
double  dxsize, dysize;                /* xdots-1, ydots-1            */
LDBL delxx, delyy, delxx2, delyy2;
double ddelmin, xx3rd, yy3rd;
double param[MAXPARAMS];
int diskvideo, savedac;

#define MAXLINE  2048

long   far *lx0, far *ly0, far *lx1, far *ly1;
double far *dx0, far *dy0, far *dx1, far *dy1;
extern double far *temp_array;

extern unsigned char far win_dacbox[256][3];
BYTE dacbox[256][3];
int dotmode;
int andcolor, resave_flag;

/*      HISTORY  far *history = NULL; */
U16 history = 0;
int maxhistory = 10;

int extraseg;
extern int initbatch;
extern char readname[];
int rowcount;
extern int showfile;
extern int initmode;
extern int overlay3d;
extern int display3d;
extern int filetype;
int comparegif = 0;
int diskisactive = 0;
extern int initsavetime;
long saveticks = 0;
long savebase = 0;
double zwidth = 0;
extern int (*outln)();
extern int out_line();
extern int outlin16();
static int call_line3d();
extern int line3d();

int     sxdots,sydots;
int     sxoffs=0,syoffs=0;
float   finalaspectratio;
extern int filexdots, fileydots, filecolors;
int frommandel;

int onthelist[100];                /* list of available fractal types */
int CountFractalList;           /* how many are on the list? */
extern int CurrentFractal;                /* which one is current? */

extern int win_display3d, win_overlay3d;

int release;
extern int win_release;
extern int save_release;
void        (*outln_cleanup)();
extern int fpu;

int compare_fractalnames( const void *element1, const void *element2)
{
int i, j, k;
    j = *(int*)element1;
    k = *(int*)element2;
for (i = 0; i < 100; i++) {
    if (fractalspecific[j].name[i] < fractalspecific[k].name[i])
        return(-1);
    if (fractalspecific[j].name[i] > fractalspecific[k].name[i])
        return(1);
    if (fractalspecific[j].name[i] == 0)
        return(0);
    }
return(0);
}

int fractint_main(void)
{
int i, k;
double temp1, temp2;
double dtemp;

outln_cleanup = NULL;                /* outln routine can set this */

CountFractalList = 0;
for (k = 0; fractalspecific[k].name != NULL; k++)
   if (fractalspecific[k].name[0] != '*' &&
       (fractalspecific[k].flags & WINFRAC) != 0 &&
       CountFractalList < 100)
            onthelist[CountFractalList++] = k;
qsort(onthelist,CountFractalList,2,compare_fractalnames);
CurrentFractal = fractype;

lx0 = (long far *)&temp_array[0];
ly0 = (long far *)&lx0[MAXLINE];
lx1 = (long far *)&ly0[MAXLINE];
ly1 = (long far *)&ly0[MAXLINE];
dx0 = (double far *)&temp_array[0];
dy0 = (double far *)&dx0[MAXLINE];
dx1 = (double far *)&dy0[MAXLINE];
dy1 = (double far *)&dy0[MAXLINE];

extraseg = FP_SEG(dx0);

    {
    /* wierd logic to initialize the palette */
    int iLoop, jLoop;
    if (ValidateLuts("defaultw.map") == 0) {
        for (iLoop = 0; iLoop < 256; iLoop++)
            for (jLoop = 0; jLoop < 3; jLoop++)
                win_dacbox[iLoop][jLoop] = dacbox[iLoop][jLoop];
            spindac(0,1);
        }
    }

restoredac();                /* ensure that the palette has been initialized */

win_cmdfiles();                                /* SSTOOLS.INI processing */

initmode = 1;                           /* override SSTOOLS.INI */

release = win_release;
save_release = win_release;

dotmode = 1;
diskvideo = 0;
usr_distest = 0;

max_kbdcount=(cpu==386) ? 80 : 30; /* check the keyboard less often */

/* ----- */

calc_status = -1;
resave_flag = 1;
strcpy(FileName,"Fract001");
if (showfile != 0) {
    strcpy(readname, FileName);
    }
else {
    if (strchr(readname,'.') == NULL)
         strcat(readname,".gif");
    strcpy(FileName,readname);
    time_to_load = 1;
    }

if (debugflag == 17500) {   /* force the release number for screen-shots */
    extern int win_release;
/*    win_release = 1750; */
    win_set_title_text();
    }

if (debugflag == 10000) {   /* check for free memory */
   char temp[50];
   char *tempptr;
   unsigned int i,i2;

   sprintf(temp," %d bytes of free stack space",stackavail());
   stopmsg(0,temp);

   i = 0;
   i2 = 0x8000;
   while ((i2 >>= 1) != 0)
      if ((tempptr = malloc(i+i2)) != NULL) {
         free(tempptr);
         i += i2;
         }
   sprintf(temp," %d NEAR bytes free", i);
   stopmsg(0,temp);
   }

if (debugflag == 70) fpu = 0;

init_help();

/* ----- */

time_to_quit = 0;
if (debugflag == 23232) /* give the Windows stuff control first */
    getakey();
if (time_to_quit) {
    end_help();
    return(0);
    }

reinit:
    time_to_reinit = 0;

    savedac = 0;                         /* don't save the VGA DAC */

    for (i = 0; i < 4; i++) {
        if(param[i] == FLT_MAX)
            param[i] = fractalspecific[fractype].paramvalue[i];
/*
        else
            fractalspecific[fractype].paramvalue[i] = param[i];
*/
        }

/*  Not used, MCP 8-6-91
    ccreal = param[0]; ccimag = param[1]; */ /* default C-values */
    frommandel = 0;

    if (xxmin > xxmax) {
       dtemp = xxmin; xxmin = xxmax; xxmax = dtemp;}
    if (yymin > yymax) {
       dtemp = yymin; yymin = yymax; yymax = dtemp;}

    ytop    = 0;
    ybottom = ydots-1;
    xleft   = 0;
    xright  = xdots-1;
    filexdots = xdots;
    fileydots = ydots;
    filecolors = colors;

restart:
    time_to_restart = 0;
    time_to_resume = 0;
    time_to_orbit = 0;

    win_title_text(1);

    if (calc_status == -99)
        calc_status = 2;                        /* force a recalc */
    else
        calc_status = 0;                        /* force a restart */

    maxit = maxiter;

    if (colors == 2 && (fractype == PLASMA || usr_stdcalcmode == 'b'))
        colors = 16;         /* 2-color mode just doesn't work on these */

    andcolor = colors-1;

    /* compute the (new) screen co-ordinates */
    /* correct a possibly munged-up zoom-box outside the image range */
    if (ytop    >= ydots) ytop    = ydots-1;
    if (ybottom >= ydots) ybottom = ydots-1;
    if (xleft   >= xdots) xleft   = xdots-1;
    if (xright  >= xdots) xright  = xdots-1;
    if (xleft == xright || ytop == ybottom) {
        }
    if (xleft > xright)
        { i = xleft; xleft = xright; xright = i;}
    if (ytop > ybottom)
        { i = ybottom; ybottom = ytop; ytop = i;}
    temp1 = xxmin;
    temp2 = xxmax - xxmin;
    xxmin = temp1 + (temp2 * xleft )/(xdots-1);
    xxmax = temp1 + (temp2 * xright)/(xdots-1);
    temp1 = yymin;
    temp2 = yymax - yymin;
    yymin = temp1 + (temp2 * (ydots - 1 - ybottom)/(ydots-1));
    yymax = temp1 + (temp2 * (ydots - 1 - ytop   )/(ydots-1));
    xx3rd = xxmin;
    yy3rd = yymin;
    xleft   = 0;
    xright  = xdots-1;
    ytop    = 0;
    ybottom = ydots-1;

/*
    delxx = (xxmax - xxmin) / (xdots-1);
    delyy = (yymax - yymin) / (ydots-1);
    delxx2 = delyy2 = 0.0;
    ddelmin = fabs(delxx);
    if (fabs(delyy) < ddelmin)
        ddelmin = fabs(delyy);
*/

    dxsize = xdots - 1;  dysize = ydots - 1;

    if (calc_status != 2 && !overlay3d)
        if (!clear_screen(1)) {
            stopmsg(0,"Can't free and re-allocate the image");
            return(0);
            }

    if (savedac || colorpreloaded) {
        memcpy(dacbox,olddacbox,256*3); /* restore the DAC */
        spindac(0,1);
        colorpreloaded = 0;
        }

    dxsize = xdots - 1;  dysize = ydots - 1;
    sxdots = xdots;  sydots = ydots;
    finalaspectratio = ((float)ydots)/xdots;

    calcfracinit();

    bitshiftless1 = bitshift - 1;

    if (time_to_load)
        goto wait_loop;

      if(showfile == 0) {                /* loading an image */
         if (display3d)                 /* set up 3D decoding */
            outln = call_line3d;
         else if(filetype >= 1)         /* old .tga format input file */
            outln = outlin16;
         else if(comparegif)                /* debug 50 */
            outln = cmp_line;
         else if(pot16bit) {                /* .pot format input file */
            pot_startdisk();
            outln = pot_line;
            }
         else                                /* regular gif/fra input file */
            outln = out_line;
         if(filetype == 0)
            i = funny_glasses_call(gifview);
         else
            i = funny_glasses_call(tgaview);
         if(i == 0)
            buzzer(0);
         else {
            calc_status = -1;
            }
         }

      if(showfile == 0) {                /* image has been loaded */
         showfile = 1;
         if (initbatch == 1 && calc_status == 2)
            initbatch = -1; /* flag to finish calc before save */
         if (calc_status == 2) goto try_to_resume;
         }
      else {                                /* draw an image */

try_to_resume:

         diskisactive = 1;                /* flag for disk-video routines */
         if (initsavetime != 0                /* autosave and resumable? */
           && (fractalspecific[fractype].flags&NORESUME) == 0) {
            savebase = readticker(); /* calc's start time */
            saveticks = (long)initsavetime * 1092; /* bios ticks/minute */
            if ((saveticks & 65535) == 0)
               ++saveticks; /* make low word nonzero */
            }
         kbdcount = 30;                 /* ensure that we check the keyboard */
         if ((i = calcfract()) == 0)        /* draw the fractal using "C" */
            buzzer(0); /* finished!! */
         saveticks = 0;                 /* turn off autosave timer */
         diskisactive = 0;                /* flag for disk-video routines */
         }

    overlay3d = 0;      /* turn off overlay3d */
    display3d = 0;      /* turn off display3d */

    zwidth = 0;

    if (!keypressed()) {
        flush_screen();
        buzzer(3);
        win_title_text(0);
        getakey();
        }

wait_loop:

    win_title_text(0);

for (;;) {
    if (time_to_act) {
         /* we bailed out of the main loop to take some secondary action */
         time_to_act = 0;
         SecondaryWndProc();
         if (! keypressed())
                 time_to_resume = 1;
         }
    if (time_to_quit) {
         end_help();
         return(0);
         }
    if (time_to_starfield) {
         time_to_starfield = 0;
         win_title_text(4);
         starfield();
         win_title_text(0);
         flush_screen();
         }
    if (time_to_load) {
        strcpy(readname, FileName);
        showfile = 1;
        time_to_load = 0;
        time_to_restart = 0;
        overlay3d = win_overlay3d;
        display3d = win_display3d;
        if (win_load() >= 0) {
            showfile = 0;
            rowcount = 0;
            ytop    = 0;                /* reset the zoom-box */
            ybottom = ydots-1;
            xleft   = 0;
            xright  = xdots-1;
            maxiter = maxit;
            time_to_load = 0;
            time_to_restart = 1;
            if (calc_status == 2) {
                calc_status = -99;      /* special klooge for restart */
                }
            }
        win_overlay3d = 0;
        win_display3d = 0;
        }
    if (time_to_save) {
        strcpy(readname, FileName);
        if (readname[0] != 0)
            win_save();
        time_to_save = 0;
        if (calc_status == 2) {
            calc_status = -99;
            time_to_restart = 1;
            }
        }
    if (time_to_print) {
        PrintFile();
        time_to_print = 0;
        }
    if (time_to_cycle) {
       win_cycle();
       }
    if (time_to_reinit == 2) {
       win_cmdfiles();
       maxiter = maxit;
       }
    if (time_to_reinit)
         goto reinit;
    if(time_to_restart)
         goto restart;
    if(time_to_resume)
        if (calc_status == 2) {
            calc_status = -99;
            time_to_restart = 1;
            goto restart;
            }
    getakey();
    }

}

/* displays differences between current image file and new image */
/* Bert - suggest add this to video.asm */
int cmp_line(unsigned char *pixels, int linelen)
{
   static errcount;
   static FILE *fp = NULL;
   extern int rowcount;
   int row,col;
   int oldcolor;
   char *timestring;
   time_t ltime;
   if(fp == NULL)
      fp = fopen("cmperr",(initbatch)?"a":"w");
   if((row = rowcount++) == 0)
      errcount = 0;
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
            fprintf(fp,"#%5d col %3d row %3d old %3d new %3d\n",
               errcount,col,row,oldcolor,pixels[col]);
         }
      }
   if(row+1 == ydots && initbatch) {
      time(&ltime);
      timestring = ctime(&ltime);
      timestring[24] = 0; /*clobber newline in time string */
      fprintf(fp,"%s compare to %s has %5d errs\n",timestring,readname,errcount);
      }
   return(0);
}

#if 0
int pot_line(unsigned char *pixels, int linelen)
{
   extern int rowcount;
   int row,col,saverowcount;
   if (rowcount == 0)
      pot_startdisk();
   saverowcount = rowcount;
   row = (rowcount >>= 1);
   if ((saverowcount & 1) != 0) /* odd line */
      row += ydots;
   else                         /* even line */
      if (dotmode != 11) /* display the line too */
         out_line(pixels,linelen);
   for (col = 0; col < xdots; ++col)
      writedisk(col+sxoffs,row+syoffs,*(pixels+col));
   rowcount = saverowcount + 1;
   return(0);
}
#endif

static int call_line3d(unsigned char *pixels, int linelen)
{
   /* this routine exists because line3d might be in an overlay */
   return(line3d(pixels,linelen));
}

void win_cmdfiles(void)   /* convert lpCmdLine into argc, argv */
{
int i, k;
int argc;
char *argv[10];
unsigned char arg[501];   /* max 10 args, 450 chars total */

arg[0] = 0;
for (i = 0; i < 10; i++)
   argv[i] = &arg[0];
argc = 1;
strcpy(&arg[1],"winfract.exe");
argv[argc-1] = &arg[1];

for (i = 0; i < 460 && win_lpCmdLine[i] != 0; i++)
   arg[20+i] = win_lpCmdLine[i];
arg[20+i] = 0;
arg[21+i] = 0;

for (k = 20; arg[k] != 0; k++) {
    while(arg[k] <= ' ' && arg[k] != 0) k++;
    if (arg[k] == 0) break;
    if (argc >= 10) break;
    argc++;
    argv[argc-1] = &arg[k];
    while(arg[k] > ' ')k++;
    arg[k] = 0;
    }

cmdfiles(argc,argv);

}
