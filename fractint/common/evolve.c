#include <string.h>
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

/* px and py are coordinates in the parameter grid (small images on screen) */
/* evolving = flag, gridsz = dimensions of image grid (gridsz x gridsz) */
int px, py, evolving, gridsz;
#define MAXGRIDSZ 51  /* This is arbitrary, = 1024/20 */
static int ecountbox[MAXGRIDSZ][MAXGRIDSZ];

unsigned int this_gen_rseed;
/* used to replay random sequences to obtain correct values when selecting a
   seed image for next generation */

double opx,opy,newopx,newopy,paramrangex,paramrangey,dpx,dpy,fiddlefactor;
double fiddle_reduction;
double parmzoom;
char odpx,odpy,newodpx,newodpy;
/* offset for discrete parameters x and y..*/
/* used for things like inside or outside types, bailout tests, trig fn etc */
/* variation factors, opx,opy, paramrangex/y dpx, dpy.. used in field mapping
   for smooth variation across screen. opx =offset param x, dpx = delta param
   per image, paramrangex = variation across grid of param ...likewise for py */ 
/* fiddlefactor is amount of random mutation used in random modes ,
   fiddle_reduction is used to decrease fiddlefactor from one generation to the
   next to eventually produce a stable population */

static int *prmbox = NULL;
static int *imgbox = NULL;
int prmboxcount = 0;
static int imgboxcount;

struct phistory_info      /* for saving evolution data of center image */
{
   double param0;
   double param1;
   double param2;
   double param3;
   double param4;
   double param5;
   double param6;
   double param7;
   double param8;
   double param9;
   int inside;
   int outside;
   int decomp0;
   double invert0;
   double invert1;
   double invert2;
   BYTE trigndx0;
   BYTE trigndx1;
   BYTE trigndx2;
   BYTE trigndx3;
   int bailoutest;
};

typedef struct phistory_info    PARAMHIST;

static PARAMHIST oldhistory = { 0 };

void param_history(int mode);
void varydbl(GENEBASE gene[],int randval,int i);
int varyint(int randvalue, int limit, int mode);
int wrapped_positive_varyint(int randvalue, int limit, int mode);
void varyinside(GENEBASE gene[], int randval, int i);
void varyoutside(GENEBASE gene[], int randval, int i);
void varypwr2(GENEBASE gene[], int randval, int i);
void varytrig(GENEBASE gene[], int randval, int i);
void varybotest(GENEBASE gene[], int randval, int i);
void varyinv(GENEBASE gene[], int randval, int i);
int explore_check(void);
void spiralmap(int);
static void set_random(int);
void set_mutation_level(int);
void SetupParamBox(void);
void ReleaseParamBox(void);

GENEBASE g_genes[NUMGENES] =
{
	{ &param[0],   varydbl,     5, "Param 1 real", 1 },
	{ &param[1],   varydbl,     5, "Param 1 imag", 1 },
	{ &param[2],   varydbl,     0, "Param 2 real", 1 },
	{ &param[3],   varydbl,     0, "Param 2 imag", 1 },
	{ &param[4],   varydbl,     0, "Param 3 real", 1 },
	{ &param[5],   varydbl,     0, "Param 3 imag", 1 },
	{ &param[6],   varydbl,     0, "Param 4 real", 1 },
	{ &param[7],   varydbl,     0, "Param 4 imag", 1 },
	{ &param[8],   varydbl,     0, "Param 5 real", 1 },
	{ &param[9],   varydbl,     0, "Param 5 imag", 1 },
	{ &inside,     varyinside,  0, "inside color", 2 },
	{ &outside,    varyoutside, 0, "outside color", 3 },
	{ &decomp[0],  varypwr2,    0, "decomposition", 4 },
	{ &inversion[0],varyinv,    0, "invert radius", 7 },
	{ &inversion[1],varyinv,    0, "invert center x", 7 },
	{ &inversion[2],varyinv,    0, "invert center y", 7 },
	{ &trigndx[0], varytrig,    0, "trig function 1", 5 },
	{ &trigndx[1], varytrig,    0, "trig fn 2", 5 },
	{ &trigndx[2], varytrig,    0, "trig fn 3", 5 },
	{ &trigndx[3], varytrig,    0, "trig fn 4", 5 },
	{ &bailoutest, varybotest,  0, "bailout test", 6 }
};

void param_history(int mode)
{ /* mode = 0 for save old history,
     mode = 1 for restore old history */
   if (mode == 0) { /* save the old parameter history */
      oldhistory.param0 = param[0];
      oldhistory.param1 = param[1];
      oldhistory.param2 = param[2];
      oldhistory.param3 = param[3];
      oldhistory.param4 = param[4];
      oldhistory.param5 = param[5];
      oldhistory.param6 = param[6];
      oldhistory.param7 = param[7];
      oldhistory.param8 = param[8];
      oldhistory.param9 = param[9];
      oldhistory.inside = inside;
      oldhistory.outside = outside;
      oldhistory.decomp0 = decomp[0];
      oldhistory.invert0 = inversion[0];
      oldhistory.invert1 = inversion[1];
      oldhistory.invert2 = inversion[2];
      oldhistory.trigndx0 = trigndx[0];
      oldhistory.trigndx1 = trigndx[1];
      oldhistory.trigndx2 = trigndx[2];
      oldhistory.trigndx3 = trigndx[3];
      oldhistory.bailoutest = bailoutest;
   }

   if (mode == 1) { /* restore the old parameter history */
      param[0] = oldhistory.param0;
      param[1] = oldhistory.param1;
      param[2] = oldhistory.param2;
      param[3] = oldhistory.param3;
      param[4] = oldhistory.param4;
      param[5] = oldhistory.param5;
      param[6] = oldhistory.param6;
      param[7] = oldhistory.param7;
      param[8] = oldhistory.param8;
      param[9] = oldhistory.param9;
      inside = oldhistory.inside;
      outside = oldhistory.outside;
      decomp[0] = oldhistory.decomp0;
      inversion[0] = oldhistory.invert0;
      inversion[1] = oldhistory.invert1;
      inversion[2] = oldhistory.invert2;
      invert = (inversion[0] == 0.0) ? 0 : 3 ;
      trigndx[0] = oldhistory.trigndx0;
      trigndx[1] = oldhistory.trigndx1;
      trigndx[2] = oldhistory.trigndx2;
      trigndx[3] = oldhistory.trigndx3;
      bailoutest = oldhistory.bailoutest;
   }
}

void varydbl(GENEBASE gene[],int randval,int i) /* routine to vary doubles */
{
int lclpy = gridsz - py - 1;
   switch (gene[i].mutate) {
    default:
    case 0:
       break;
    case 1:
       *(double *)gene[i].addr = px * dpx + opx; /*paramspace x coord * per view delta px + offset */
       break;
    case 2: 
       *(double *)gene[i].addr = lclpy * dpy + opy; /*same for y */
       break;
    case   3:
       *(double *)gene[i].addr = px*dpx + opx +(lclpy*dpy) + opy; /*and x + y */
       break;
    case 4:
       *(double *)gene[i].addr = (px*dpx + opx)-(lclpy*dpy + opy); /*and x-y*/
       break;
    case 5:
       *(double *)gene[i].addr += (((double)randval / RAND_MAX) * 2 * fiddlefactor) - fiddlefactor;
       break;
    case 6:  /* weighted random mutation, further out = further change */
       {
       int mid = gridsz /2;
       double radius =  sqrt(sqr(px - mid) + sqr(lclpy - mid));
       *(double *)gene[i].addr += ((((double)randval / RAND_MAX) * 2 * fiddlefactor) - fiddlefactor) * radius;
       }
       break;
   }
return;
}

int varyint(int randvalue, int limit, int mode)
{
int ret = 0;
int lclpy = gridsz - py - 1;
 switch (mode) {
   default:
   case 0:
     break;
   case 1: /* vary with x */
     ret = (odpx + px)%limit;
     break;
   case 2: /* vary with y */
     ret = (odpy + lclpy)%limit;
     break;
   case 3: /* vary with x + y */
     ret = (odpx + px + odpy + lclpy)%limit;
     break;
   case 4: /* vary with x-y */
     ret = (odpx + px)-(odpy + lclpy)%limit;
     break;
   case 5: /* random mutation */
     ret = randvalue % limit;
     break;
   case 6:  /* weighted random mutation, further out = further change */
     {
     int mid = gridsz /2;
     double radius =  sqrt(sqr(px - mid) + sqr(lclpy - mid));
     ret = (int)((((randvalue / RAND_MAX) * 2 * fiddlefactor) - fiddlefactor) * radius);
     ret %= limit;
     }
       break;
 }
 return ret;
}

int wrapped_positive_varyint(int randvalue, int limit, int mode)
{
   int i;
   i = varyint(randvalue,limit,mode);
   if (i < 0)
      return limit + i;
   else
      return i;
}

void varyinside(GENEBASE gene[], int randval, int i)
 {
   int choices[9]={-59,-60,-61,-100,-101,-102,-103,-104,-1};
   if (gene[i].mutate)
     *(int*)gene[i].addr=choices[wrapped_positive_varyint(randval,9,gene[i].mutate)];
   return;
 }

void varyoutside(GENEBASE gene[], int randval, int i)
 {
   int choices[8]={-1,-2,-3,-4,-5,-6,-7,-8};
   if (gene[i].mutate)
     *(int*)gene[i].addr=choices[wrapped_positive_varyint(randval,8,gene[i].mutate)];
   return;
 }

void varybotest(GENEBASE gene[], int randval, int i)
 {
   int choices[7]={Mod, Real, Imag, Or, And, Manh, Manr};
   if (gene[i].mutate) {
     *(int*)gene[i].addr=choices[wrapped_positive_varyint(randval,7,gene[i].mutate)];
     /* move this next bit to varybot where it belongs */
     setbailoutformula(bailoutest);
   }
   return;
 }

void varypwr2(GENEBASE gene[], int randval, int i)
 {
  int choices[9]={0,2,4,8,16,32,64,128,256}; 
  if (gene[i].mutate)
    *(int*)gene[i].addr=choices[wrapped_positive_varyint(randval,9,gene[i].mutate)];
  return;
}

void varytrig(GENEBASE gene[], int randval, int i)
 {
  if (gene[i].mutate)
  /* Changed following to BYTE since trigfn is an array of BYTEs and if one */
  /* of the functions isn't varied, it's value was being zeroed by the high */
  /* BYTE of the preceeding function.  JCO  2 MAY 2001 */
     *(BYTE*)gene[i].addr=(BYTE)wrapped_positive_varyint(randval,numtrigfn,gene[i].mutate);
     /* replaced '30' with numtrigfn, set in prompts1.c */
  set_trig_pointers(5); /*set all trig ptrs up*/
  return;
 } 

void varyinv(GENEBASE gene[], int randval, int i)
  {
   if (gene[i].mutate)
      varydbl(gene,randval,i);
   invert = (inversion[0] == 0.0) ? 0 : 3 ;
  }

/* --------------------------------------------------------------------- */
/*
    get_evolve_params() is called from FRACTINT.C whenever the 'ctrl_e' key
    is pressed.  Return codes are:
      -1  routine was ESCAPEd - no need to re-generate the images
     0  minor variable changed.  No need to re-generate the image.
       1  major parms changed.  Re-generate the images.
*/
int get_the_rest(void)
{
  char *evolvmodes[]={"no","x","y","x + y","x-y","random","spread"};
  int i,k,num, numtrig;
  char *choices[20];
  struct fullscreenvalues uvalues[20];

   numtrig = (curfractalspecific->flags >> 6) & 7;
   if (fractype == FORMULA || fractype == FFORMULA) {
      numtrig = maxfn;
      }

choose_vars_restart:

   k = -1;
   for (num = MAXPARAMS; num < (NUMGENES - 5); num++) {
      choices[++k]=g_genes[num].name;
      uvalues[k].type = 'l';
      uvalues[k].uval.ch.vlen = 7;
      uvalues[k].uval.ch.llen = 7;
      uvalues[k].uval.ch.list = evolvmodes;
      uvalues[k].uval.ch.val =  g_genes[num].mutate;
   }

   for (num = (NUMGENES - 5); num < (NUMGENES - 5 + numtrig); num++) {
      choices[++k]=g_genes[num].name;
      uvalues[k].type = 'l';
      uvalues[k].uval.ch.vlen = 7;
      uvalues[k].uval.ch.llen = 7;
      uvalues[k].uval.ch.list = evolvmodes;
      uvalues[k].uval.ch.val =  g_genes[num].mutate;
   }

   if (curfractalspecific->calctype == StandardFractal &&
       (curfractalspecific->flags & BAILTEST)) {
      choices[++k]=g_genes[NUMGENES - 1].name;
      uvalues[k].type = 'l';
      uvalues[k].uval.ch.vlen = 7;
      uvalues[k].uval.ch.llen = 7;
      uvalues[k].uval.ch.list = evolvmodes;
      uvalues[k].uval.ch.val =  g_genes[NUMGENES - 1].mutate;
   }

   choices[++k]= "";
   uvalues[k].type = '*';
   choices[++k]= "Press F2 to set all to off";
   uvalues[k].type ='*';
   choices[++k]= "Press F3 to set all on";
   uvalues[k].type = '*';
   choices[++k]= "Press F4 to randomize all";
   uvalues[k].type = '*';

   i = fullscreen_prompt("Variable tweak central 2 of 2",k + 1,choices,uvalues,28,NULL);

   switch (i) {
     case FIK_F2: /* set all off */
       for (num = MAXPARAMS; num < NUMGENES; num++)
          g_genes[num].mutate = 0;
       goto choose_vars_restart;
     case FIK_F3: /* set all on..alternate x and y for field map */
       for (num = MAXPARAMS; num < NUMGENES; num ++)
          g_genes[num].mutate = (char)((num % 2) + 1);
       goto choose_vars_restart;
     case FIK_F4: /* Randomize all */
       for (num =MAXPARAMS; num < NUMGENES; num ++)
          g_genes[num].mutate = (char)(rand() % 6);
       goto choose_vars_restart;
     case -1:
       return -1;
     default:
       break;
   }

   /* read out values */
   k = -1;
   for (num = MAXPARAMS; num < (NUMGENES - 5); num++)
      g_genes[num].mutate = (char)(uvalues[++k].uval.ch.val);

   for (num = (NUMGENES - 5); num < (NUMGENES - 5 + numtrig); num++) 
      g_genes[num].mutate = (char)(uvalues[++k].uval.ch.val);

   if (curfractalspecific->calctype == StandardFractal &&
       (curfractalspecific->flags & BAILTEST))
      g_genes[NUMGENES - 1].mutate = (char)(uvalues[++k].uval.ch.val);

   return 1; /* if you were here, you want to regenerate */
}

int get_variations(void)
{
  char *evolvmodes[]={"no","x","y","x+y","x-y","random","spread"};
  int i,k,num, numparams;
  char *choices[20];
  struct fullscreenvalues uvalues[20];
  int firstparm = 0;
  int lastparm  = MAXPARAMS;
  int chngd = -1;

   if (fractype == FORMULA || fractype == FFORMULA) {
      if (uses_p1)  /* set first parameter */
         firstparm = 0;
      else if (uses_p2)
         firstparm = 2;
      else if (uses_p3)
         firstparm = 4;
      else if (uses_p4)
         firstparm = 6;
      else
         firstparm = 8; /* uses_p5 or no parameter */

      if (uses_p5) /* set last parameter */
         lastparm = 10;
      else if (uses_p4)
         lastparm = 8;
      else if (uses_p3)
         lastparm = 6;
      else if (uses_p2)
         lastparm = 4;
      else
         lastparm = 2; /* uses_p1 or no parameter */
   }

   numparams = 0;
   for (i = firstparm; i < lastparm; i++)
   {
      if (typehasparm(julibrot?neworbittype:fractype,i,NULL) == 0) {
         if (fractype == FORMULA || fractype == FFORMULA)
           if (paramnotused(i))
              continue;
         break;
      }
      numparams++;
   }

   if (fractype != FORMULA && fractype != FFORMULA)
      lastparm = numparams;

choose_vars_restart:

   k = -1;
   for (num = firstparm; num < lastparm; num++) {
      if (fractype == FORMULA || fractype == FFORMULA)
        if (paramnotused(num))
           continue;
      choices[++k]=g_genes[num].name;
      uvalues[k].type = 'l';
      uvalues[k].uval.ch.vlen = 7;
      uvalues[k].uval.ch.llen = 7;
      uvalues[k].uval.ch.list = evolvmodes;
      uvalues[k].uval.ch.val =  g_genes[num].mutate;
   }

   choices[++k]= "";
   uvalues[k].type = '*';
   choices[++k]= "Press F2 to set all to off";
   uvalues[k].type ='*';
   choices[++k]= "Press F3 to set all on";
   uvalues[k].type = '*';
   choices[++k]= "Press F4 to randomize all";
   uvalues[k].type = '*';
   choices[++k]= "Press F6 for second page"; /* F5 gets eaten */
   uvalues[k].type = '*';

   i = fullscreen_prompt("Variable tweak central 1 of 2",k + 1,choices,uvalues,92,NULL);

   switch (i) {
     case FIK_F2: /* set all off */
       for (num = 0; num < MAXPARAMS; num++)
          g_genes[num].mutate = 0;
       goto choose_vars_restart;
     case FIK_F3: /* set all on..alternate x and y for field map */
       for (num = 0; num < MAXPARAMS; num ++)
          g_genes[num].mutate = (char)((num % 2) + 1);
       goto choose_vars_restart;
     case FIK_F4: /* Randomize all */
       for (num =0; num < MAXPARAMS; num ++)
          g_genes[num].mutate = (char)(rand() % 6);
       goto choose_vars_restart;
     case FIK_F6: /* go to second screen, put array away first */
		 {
			GENEBASE save[NUMGENES];
			int g;

			for (g = 0; g < NUMGENES; g++)
			{
				save[g] = g_genes[g];
			}
			chngd = get_the_rest();
			for (g = 0; g < NUMGENES; g++)
			{
				g_genes[g] = save[g];
			}
		 }
       goto choose_vars_restart;
     case -1:
       return chngd;
     default:
       break;
   }

   /* read out values */
   k = -1;
   for (num = firstparm; num < lastparm; num++) {
      if (fractype == FORMULA || fractype == FFORMULA)
        if (paramnotused(num))
           continue;
      g_genes[num].mutate = (char)(uvalues[++k].uval.ch.val);
   }

   return 1; /* if you were here, you want to regenerate */
}

void set_mutation_level(int strength)
{
/* scan through the gene array turning on random variation for all parms that */
/* are suitable for this level of mutation */
 int i;

 for (i = 0; i < NUMGENES; i++) {
   if (g_genes[i].level <= strength)
      g_genes[i].mutate = 5; /* 5 = random mutation mode */
   else
      g_genes[i].mutate = 0;
 }
 return;
}

int get_evolve_Parms(void)
{
   char *choices[20];
   int oldhelpmode;
   struct fullscreenvalues uvalues[20];
   int i,j, k, tmp;
   int old_evolving,old_gridsz;
   int old_variations = 0;
   double old_paramrangex,old_paramrangey,old_opx,old_opy,old_fiddlefactor;

   /* fill up the previous values arrays */
   old_evolving      = evolving;
   old_gridsz        = gridsz;
   old_paramrangex   = paramrangex;
   old_paramrangey   = paramrangey;
   old_opx           = opx;
   old_opy           = opy;
   old_fiddlefactor  = fiddlefactor;

get_evol_restart:

   /* TODO: allocate real memory, not reuse shared segment */
//   ptr = (char *) extraseg;
   if ((evolving & EVOLVE_RAND_WALK) || (evolving & EVOLVE_RAND_PARAM)) {
   /* adjust field param to make some sense when changing from random modes*/
   /* maybe should adjust for aspect ratio here? */
      paramrangex = paramrangey = fiddlefactor * 2;
      opx = param[0] - fiddlefactor;
      opy = param[1] - fiddlefactor;
      /* set middle image to last selected and edges to +- fiddlefactor */
   }

   k = -1;

   choices[++k]= "Evolution mode? (no for full screen)";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = evolving & EVOLVE_FIELD_MAP;

   choices[++k]= "Image grid size (odd numbers only)";
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = gridsz;

   if (explore_check()) {  /* test to see if any parms are set to linear */
                           /* variation 'explore mode' */
     choices[++k]= "Show parameter zoom box?";
     uvalues[k].type = 'y';
     uvalues[k].uval.ch.val = ((evolving & EVOLVE_PARM_BOX) / EVOLVE_PARM_BOX);

     choices[++k]= "x parameter range (across screen)";
     uvalues[k].type = 'f';
     uvalues[k].uval.dval = paramrangex;

     choices[++k]= "x parameter offset (left hand edge)";
     uvalues[k].type = 'f';
     uvalues[k].uval.dval = opx;

     choices[++k]= "y parameter range (up screen)";
     uvalues[k].type = 'f';
     uvalues[k].uval.dval = paramrangey;

     choices[++k]= "y parameter offset (lower edge)";
     uvalues[k].type = 'f';
     uvalues[k].uval.dval= opy;
   }

     choices[++k]= "Max random mutation";
     uvalues[k].type = 'f';
     uvalues[k].uval.dval = fiddlefactor;

     choices[++k]= "Mutation reduction factor (between generations)";
     uvalues[k].type = 'f';
     uvalues[k].uval.dval = fiddle_reduction;

   choices[++k]= "Grouting? ";
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = !((evolving & EVOLVE_NO_GROUT) / EVOLVE_NO_GROUT);

   choices[++k]= "";
   uvalues[k].type = '*';

   choices[++k]= "Press F4 to reset view parameters to defaults.";
   uvalues[k].type = '*';

   choices[++k]= "Press F2 to halve mutation levels";
   uvalues[k].type = '*';

   choices[++k]= "Press F3 to double mutation levels" ;
   uvalues[k].type ='*';

   choices[++k]= "Press F6 to control which parameters are varied";
   uvalues[k].type = '*';
   oldhelpmode = helpmode;     /* this prevents HELP from activating */
   helpmode = HELPEVOL; 
   i = fullscreen_prompt("Evolution Mode Options",k + 1,choices,uvalues,255,NULL);
   helpmode = oldhelpmode;     /* re-enable HELP */
   if (i < 0) {
   /* in case this point has been reached after calling sub menu with F6 */
   evolving      = old_evolving;
   gridsz        = old_gridsz;
   paramrangex   = old_paramrangex;
   paramrangey   = old_paramrangey;
   opx           = old_opx;
   opy           = old_opy;
   fiddlefactor  = old_fiddlefactor;

      return -1;
   }

   if (i == FIK_F4) {
      set_current_params();
      fiddlefactor = 1;
      fiddle_reduction = 1.0;
      goto get_evol_restart;
   }
   if (i == FIK_F2) {
      paramrangex = paramrangex / 2;
      opx = newopx = opx + paramrangex / 2;
      paramrangey = paramrangey / 2;
      opy = newopy = opy + paramrangey / 2;
      fiddlefactor = fiddlefactor / 2;
      goto get_evol_restart;
   }
   if (i == FIK_F3) {
    double centerx, centery;
      centerx = opx + paramrangex / 2;
      paramrangex = paramrangex * 2;
      opx = newopx = centerx - paramrangex / 2;
      centery = opy + paramrangey / 2;
      paramrangey = paramrangey * 2;
      opy = newopy = centery - paramrangey / 2;
      fiddlefactor = fiddlefactor * 2;
      goto get_evol_restart;
   }

   j = i;   

   /* now check out the results (*hopefully* in the same order <grin>) */

   k = -1;

   viewwindow = evolving = uvalues[++k].uval.ch.val;

   if (!evolving && i != FIK_F6)  /* don't need any of the other parameters JCO 12JUL2002 */
      return 1;              /* the following code can set evolving even if it's off */

   gridsz = uvalues[++k].uval.ival;
   tmp = sxdots / (MINPIXELS << 1);
   /* (sxdots / 20), max # of subimages @ 20 pixels per subimage */
   /* MAXGRIDSZ == 1024 / 20 == 51 */
   if (gridsz > MAXGRIDSZ)
      gridsz = MAXGRIDSZ;
   if (gridsz > tmp)
      gridsz = tmp;
   if (gridsz < 3)
      gridsz = 3;
   gridsz |= 1; /* make sure gridsz is odd */
   if (explore_check()) {
     tmp = (EVOLVE_PARM_BOX*uvalues[++k].uval.ch.val);
     if (evolving)
        evolving += tmp;
     paramrangex = uvalues[++k].uval.dval;
     newopx = opx = uvalues[++k].uval.dval;
     paramrangey = uvalues[++k].uval.dval;
     newopy = opy = uvalues[++k].uval.dval;
   }

     fiddlefactor = uvalues[++k].uval.dval;

     fiddle_reduction = uvalues[++k].uval.dval;

   if (!(uvalues[++k].uval.ch.val)) evolving = evolving + EVOLVE_NO_GROUT;

   viewxdots = (sxdots / gridsz)-2;
   viewydots = (sydots / gridsz)-2;
   if (!viewwindow) viewxdots=viewydots = 0;

   i = 0;

   if (evolving != old_evolving
    || (gridsz != old_gridsz) ||(paramrangex != old_paramrangex)
    || (opx != old_opx) || (paramrangey != old_paramrangey)
    || (opy != old_opy)  || (fiddlefactor != old_fiddlefactor)
    || (old_variations > 0))
      i = 1;

   if (evolving && !old_evolving)
      param_history(0); /* save old history */

   if (!evolving && (evolving == old_evolving)) i = 0;

if (j == FIK_F6) {
      old_variations = get_variations();
      set_current_params();
      if (old_variations > 0)
        {
          viewwindow = 1;
          evolving |= 1;   /* leave other settings alone */
        }
      fiddlefactor = 1;
      fiddle_reduction = 1.0;
      goto get_evol_restart;
   }
   return i;
}

void SetupParamBox(void)
{
	int vidsize;
	prmboxcount = 0;
	parmzoom=((double)gridsz-1.0)/2.0;
	/* need to allocate 2 int arrays for boxx and boxy plus 1 byte array for values */  
	vidsize = (xdots + ydots)*4*sizeof(int);
	vidsize += xdots + ydots + 2;
	if (!prmbox)
	{
		prmbox = (int *) malloc(vidsize);
	}
	if (!prmbox)
	{
		texttempmsg("Sorry...can't allocate mem for parmbox");
		evolving = 0;
	}
	prmboxcount = 0;

	/* vidsize = (vidsize / gridsz) + 3 ; */ /* allocate less mem for smaller box */
	/* taken out above as *all* pixels get plotted in small boxes */
	if (!imgbox)
	{
		imgbox = (int *) malloc(vidsize);
	}
	if (!imgbox)
	{
		texttempmsg("Sorry...can't allocate mem for imagebox");
	}
}

void ReleaseParamBox(void)
{
	if (prmbox)
	{
		free(prmbox);
		prmbox = NULL;
	}
	if (imgbox)
	{
		free(imgbox);
		imgbox = NULL;
	}
}

void set_current_params(void)
{
   paramrangex = curfractalspecific->xmax - curfractalspecific->xmin;
   opx = newopx = - (paramrangex / 2);
   paramrangey = curfractalspecific->ymax - curfractalspecific->ymin;
   opy = newopy = - (paramrangey / 2);
   return;
}

void fiddleparms(GENEBASE gene[], int ecount)
{
/* call with px,py ... parameter set co-ords*/
/* set random seed then call rnd enough times to get to px,py */ 
/* 5/2/96 adding in indirection */ 
/* 26/2/96 adding in multiple methods and field map */
/* 29/4/96 going for proper handling of the whole gene array */
/*         bung in a pile of switches to allow for expansion to any
           future variable types */
/* 11/6/96 scrapped most of switches above and used function pointers 
           instead */
/* 4/1/97  picking it up again after the last step broke it all horribly! */

 int i;

/* when writing routines to vary param types make sure that rand() gets called
the same number of times whether gene[].mutate is set or not to allow
user to change it between generations without screwing up the duplicability
of the sequence and starting from the wrong point */

/* this function has got simpler and simpler throughout the construction of the
 evolver feature and now consists of just these few lines to loop through all
 the variables referenced in the gene array and call the functions required
 to vary them, aren't pointers marvellous! */

 if ((px == gridsz / 2) && (py == gridsz / 2)) /* return if middle image */
    return;

 set_random(ecount);   /* generate the right number of pseudo randoms */

 for (i = 0; i < NUMGENES; i++)
    (*(gene[i].varyfunc))(gene,rand(),i);

}

static void set_random(int ecount)
{ /* This must be called with ecount set correctly for the spiral map. */
  /* Call this routine to set the random # to the proper value */
  /* if it may have changed, before fiddleparms() is called. */
  /* Now called by fiddleparms(). */
 int index,i;

 srand(this_gen_rseed);
 for (index = 0; index < ecount; index++)
   for (i = 0; i < NUMGENES; i++)
     rand();
}

int explore_check(void)
{
/* checks through gene array to see if any of the parameters are set to */
/* one of the non random variation modes. Used to see if parmzoom box is */
/* needed */
   int nonrandom = FALSE;
   int i;

   for (i = 0; i < NUMGENES && !(nonrandom); i++)
      if ((g_genes[i].mutate > 0) && (g_genes[i].mutate < 5)) nonrandom = TRUE;
   return nonrandom;
}

void drawparmbox(int mode)
{
/* draws parameter zoom box in evolver mode */
/* clears boxes off screen if mode = 1, otherwise, redraws boxes */
struct coords tl,tr,bl,br;
int grout;
 if (!(evolving & EVOLVE_PARM_BOX)) return; /* don't draw if not asked to! */
 grout = !((evolving & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
 imgboxcount = boxcount;
 if (boxcount) {
   /* stash normal zoombox pixels */
	 memcpy(&imgbox[0], &boxx[0], boxcount*sizeof(boxx[0]));
	 memcpy(&imgbox[boxcount], &boxy[0], boxcount*sizeof(boxy[0]));
	 memcpy(&imgbox[boxcount*2], &boxvalues[0], boxcount*sizeof(boxvalues[0]));
   clearbox(); /* to avoid probs when one box overlaps the other */
 }
 if (prmboxcount != 0)  { /* clear last parmbox */
   boxcount=prmboxcount;
   memcpy(&boxx[0], &prmbox[0], boxcount*sizeof(boxx[0]));
   memcpy(&boxy[0], &prmbox[boxcount], boxcount*sizeof(boxy[0]));
   memcpy(&boxvalues[0], &prmbox[boxcount*2], boxcount*sizeof(boxvalues[0]));
   clearbox();
 }

 if (mode == 1) {
    boxcount = imgboxcount;
    prmboxcount = 0;
    return;
 }

 boxcount =0;
 /*draw larger box to show parm zooming range */
 tl.x = bl.x = ((px -(int)parmzoom) * (int)(dxsize + 1 + grout))-sxoffs-1;
 tl.y = tr.y = ((py -(int)parmzoom) * (int)(dysize + 1 + grout))-syoffs-1; 
 br.x = tr.x = ((px +1 + (int)parmzoom) * (int)(dxsize + 1 + grout))-sxoffs;
 br.y = bl.y = ((py +1 + (int)parmzoom) * (int)(dysize + 1 + grout))-syoffs;
#ifndef XFRACT
 addbox(br); addbox(tr); addbox(bl); addbox(tl);
 drawlines(tl,tr,bl.x-tl.x,bl.y-tl.y);
 drawlines(tl,bl,tr.x-tl.x,tr.y-tl.y);
#else
 boxx[0] = tl.x + sxoffs;
 boxy[0] = tl.y + syoffs;
 boxx[1] = tr.x + sxoffs;
 boxy[1] = tr.y + syoffs;
 boxx[2] = br.x + sxoffs;
 boxy[2] = br.y + syoffs;
 boxx[3] = bl.x + sxoffs;
 boxy[3] = bl.y + syoffs;
 boxcount = 8;
#endif
 if (boxcount) {
   dispbox();
   /* stash pixel values for later */
   memcpy(&prmbox[0], &boxx[0], boxcount*sizeof(boxx[0]));
   memcpy(&prmbox[boxcount], &boxy[0], boxcount*sizeof(boxy[0]));
   memcpy(&prmbox[boxcount*2], &boxvalues[0], boxcount*sizeof(boxvalues[0]));
   }
 prmboxcount = boxcount;
 boxcount = imgboxcount;
 if (imgboxcount) {
   /* and move back old values so that everything can proceed as normal */
	memcpy(&boxx[0], &imgbox[0], boxcount*sizeof(boxx[0]));
	memcpy(&boxy[0], &imgbox[boxcount], boxcount*sizeof(boxy[0]));
	memcpy(&boxvalues[0], &imgbox[boxcount*2], boxcount*sizeof(boxvalues[0]));
   dispbox();
 }
 return;
}

void set_evolve_ranges(void)
{
int lclpy = gridsz - py - 1;
  /* set up ranges and offsets for parameter explorer/evolver */
  paramrangex=dpx*(parmzoom*2.0);
  paramrangey=dpy*(parmzoom*2.0);
  newopx=opx + (((double)px-parmzoom)*dpx);
  newopy=opy + (((double)lclpy-parmzoom)*dpy);

  newodpx=(char)(odpx + (px-gridsz/2));
  newodpy=(char)(odpy + (lclpy-gridsz/2));
  return;
}

void spiralmap(int count)
{
 /* maps out a clockwise spiral for a prettier and possibly   */
 /* more intuitively useful order of drawing the sub images.  */
 /* All the malarky with count is to allow resuming */

  int i,mid,offset;
  i = 0;
  mid = gridsz / 2;
  if (count == 0) { /* start in the middle */
    px = py = mid;
    return;
  }
  for (offset = 1; offset <= mid; offset ++) {
    /* first do the top row */
    py = (mid - offset);
    for (px = (mid - offset) + 1; px <mid + offset; px++) {
      i++;
      if (i == count) return;
    }
    /* then do the right hand column */
    for (; py < mid + offset; py++) {
      i++;
      if (i == count) return;
    }
    /* then reverse along the bottom row */
    for (; px > mid - offset; px--) {
      i++;
      if (i == count) return;
    }
    /* then up the left to finish */
    for (; py >= mid - offset; py--) {
      i++;
      if (i == count) return;
    }
  }
}

int unspiralmap(void)
{
 /* unmaps the clockwise spiral */
 /* All this malarky is to allow selecting different subimages */
 /* Returns the count from the center subimage to the current px & py */
  int mid;
  static int oldgridsz = 0;

  mid = gridsz / 2;
  if ((px == mid && py == mid) || (oldgridsz != gridsz)) {
    int i, gridsqr;
   /* set up array and return */
     gridsqr = gridsz * gridsz;
     ecountbox[px][py] = 0;  /* we know the first one, do the rest */
     for (i = 1; i < gridsqr; i++) {
         spiralmap(i);
         ecountbox[px][py] = i;
     }
     oldgridsz = gridsz;
     px = py = mid;
     return 0;
  }
  return ecountbox[px][py];
}

  /* points to ponder.....
   watch out for near mem.....            (TW) It's better now.
   try and keep gene array in overlay?    (TW) No, but try dynamic alloc.
   how to preserve 'what to mutate ' choices through overlay swaps in
   prompts... (TW) Needs checking, seems OK. LOADCHOICES copies, so is
   safe.
   */

