/* calmanfp.c
 * This file contains routines to replace calmanfp.asm.
 *
 * This file Copyright 1992 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */

#include "port.h"
#include "prototyp.h"
#include "fractype.h"

extern int atan_colors;

static int inside_color, periodicity_color;

void calcmandfpasmstart(void) {
    if (inside<0) {
	inside_color = maxit;
    } else {
	inside_color = inside;
    }

    if (periodicitycheck < 0) {
	periodicity_color = 7;
    } else {
	periodicity_color = inside_color;
    }
    oldcoloriter = 0;
}

#define ABS(x) ((x)>0?(x):-(x))
#define close 0.01

long calcmandfpasm_c(void) {
    int cx;
    int savedand;
    double x,y,x2, y2, xy, Cx, Cy, savedx, savedy;
    int savedincr;

    if (periodicitycheck==0) {
	oldcoloriter = 0;
    } else if (reset_periodicity==0) {
	oldcoloriter = maxit-250;
    }

    /* initparms */
    savedx = 0;
    savedy = 0;
    savedand = 1;
    savedincr = 1;
    orbit_ptr = 0;
    kbdcount--; /* Only check the keyboard sometimes */
    if (kbdcount<0) {
	int key;
	kbdcount = 1000;
	key = fkeypressed();
	if (key) {
	    if (key=='o' || key=='O') {
		getakey();
		show_orbit = 1-show_orbit;
	    } else {
		coloriter = -1;
		return -1;
	    }
	}
    }

    cx = maxit;
    cx--;
    if (fractype != JULIAFP) {
	/* Mandelbrot_87 */
	Cx = init.x;
	Cy = init.y;
	x = parm.x+Cx;
	y = parm.y+Cy;
    } else {
	/* dojulia_87 */
	Cx = parm.x;
	Cy = parm.y;
	x = init.x;
	y = init.y;
     x2 = x*x;
     y2 = y*y;
     xy = x*y;
	x = x2-y2+Cx;
	y = 2*xy+Cy;
    }
    x2 = x*x;
    y2 = y*y;
    xy = x*y;

    /* top_of_cs_loop_87 */
    do {
	x = x2-y2+Cx;
	y = 2*xy+Cy;
   /* no_save_new_xy_87 */
	if (cx<oldcoloriter) {
	    if (savedand==0) {
		savedx = x;
		savedy = y;
		savedincr--;
		if (savedincr==0) {
		    savedand = (savedand<<1) + 1;
		    savedincr = nextsavedincr;
		} else {
		    if (ABS(x-savedx)<closenuff && ABS(y-savedy)<closenuff) {
/*			oldcoloriter = 65535;  */
			oldcoloriter = -1;
			realcoloriter = maxit;
			kbdcount = kbdcount-(maxit-cx);
			coloriter = periodicity_color;
			goto pop_stack;
		    }
		}
	    }
	}
	/* no_periodicity_check_87 */
	if (show_orbit != 0) {
	    plot_orbit(x,y,-1);
	}
	/* no_show_orbit_87 */
	x2 = x*x;
	y2 = y*y;
	xy = x*y;
	magnitude = x2+y2;

   if (magnitude > rqlim) {
	    goto over_bailout_87;
	}

	cx--;
    } while (cx>0);

    /* reached maxit */
/*    oldcoloriter = 65535;  */
    oldcoloriter = -1;
    kbdcount -= maxit;
    realcoloriter = maxit;

    coloriter = inside_color;

pop_stack:

    if (orbit_ptr) {
	scrub_orbit();
    }

    return coloriter;

over_bailout_87:

   if (outside<=-2) {
	    new.x = x;
	    new.y = y;
	}
    if (cx-10>=0) {
	oldcoloriter = cx-10;
    } else {
	oldcoloriter = 0;
    }
    coloriter = realcoloriter = maxit-cx;
/*    if (realcoloriter==0) realcoloriter = 1; */
    kbdcount -= realcoloriter;
    if (outside==-1) {
    } else if (outside>-2) {
	coloriter = outside;
    } else {
	/* special_outside */
	if (outside==REAL) {
	    coloriter += (long)new.x + 7;
	} else if (outside==IMAG) {
	    coloriter += (long)new.y + 7;
	} else if (outside==MULT && new.y!=0.0) {
          coloriter = (long)((double)coloriter * (new.x/new.y));
	} else if (outside==SUM) {
	    coloriter +=  new.x + new.y;
	} else if (outside==ATAN) {
            coloriter = (long)fabs(atan2(new.y,new.x)*atan_colors/PI);
        }
	/* check_color */
      if ((coloriter <= 0 || coloriter > maxit) && outside!=FMOD)
         {
         if (save_release < 1961)
             coloriter = 0;
         else
             coloriter = 1;
         }
    }

    goto pop_stack;

}
