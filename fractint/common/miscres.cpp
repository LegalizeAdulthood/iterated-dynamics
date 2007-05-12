/*
		Resident odds and ends that don't fit anywhere else.
*/

#include <string.h>
#include <ctype.h>
#include <time.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif

#ifndef XFRACT
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
#include "fihelp.h"
#include "EscapeTime.h"
#include "MathUtil.h"

extern long g_bn_max_stack;
extern long maxstack;
extern long startstack;
/* routines in this module      */

static  void trigdetails(char *);
static void area();

#ifndef XFRACT
/* return full pathnames */
void findpath(const char *filename, char *fullpathname)
{
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	char temp_path[FILE_MAX_PATH];

	split_path(filename , NULL, NULL, fname, ext);
	make_path(temp_path, ""   , "" , fname, ext);

	if (g_check_current_dir != 0 && access(temp_path, 0) == 0)   /* file exists */
	{
		strcpy(fullpathname, temp_path);
		return;
	}

	strcpy(temp_path, filename);   /* avoid side effect changes to filename */

	if (temp_path[0] == SLASHC || (temp_path[0] && temp_path[1] == ':'))
	{
		if (access(temp_path, 0) == 0)   /* file exists */
		{
			strcpy(fullpathname, temp_path);
			return;
		}
		else
		{
			split_path(temp_path , NULL, NULL, fname, ext);
			make_path(temp_path, ""   , "" , fname, ext);
		}
	}
	fullpathname[0] = 0;                         /* indicate none found */
	_searchenv(temp_path, "PATH", fullpathname);
	if (!fullpathname[0])
	{
		_searchenv(temp_path, "FRACTDIR", fullpathname);
	}
	if (fullpathname[0] != 0)                    /* found it! */
	{
		if (strncmp(&fullpathname[2], SLASHSLASH, 2) == 0) /* stupid klooge! */
		{
			strcpy(&fullpathname[3], temp_path);
		}
	}
}
#endif


void not_disk_message()
{
	stop_message(0,
		"This type may be slow using a real-disk based 'video' mode, but may not \n"
		"be too bad if you have enough expanded or extended memory. Press <Esc> to \n"
		"abort if it appears that your disk drive is working too hard.");
}

/* Wrapping version of driver_put_string for long numbers                         */
/* row     -- pointer to row variable, internally incremented if needed   */
/* col1    -- starting column                                             */
/* col2    -- last column                                                 */
/* color   -- attribute (same as for driver_put_string)                           */
/* maxrow -- max number of rows to write                                 */
/* returns 0 if success, 1 if hit maxrow before done                      */
int putstringwrap(int *row, int col1, int col2, int color, char *str, int maxrow)
{
	char save1, save2;
	int length, decpt, padding, startrow, done;
	done = 0;
	startrow = *row;
	length = (int) strlen(str);
	padding = 3; /* space between col1 and decimal. */
	/* find decimal point */
	for (decpt = 0; decpt < length; decpt++)
	{
		if (str[decpt] == '.')
		{
			break;
		}
	}
	if (decpt >= length)
	{
		decpt = 0;
	}
	if (decpt < padding)
	{
		padding -= decpt;
	}
	else
	{
		padding = 0;
	}
	col1 += padding;
	decpt += col1 + 1; /* column just past where decimal is */
	while (length > 0)
	{
		if (col2-col1 < length)
		{
			if ((*row - startrow + 1) >= maxrow)
			{
				done = 1;
			}
			else
			{
				done = 0;
			}
			save1 = str[col2-col1 + 1];
			save2 = str[col2-col1 + 2];
			if (done)
			{
				str[col2-col1 + 1]   = '+';
			}
			else
			{
				str[col2-col1 + 1]   = '\\';
			}
			str[col2-col1 + 2] = 0;
			driver_put_string(*row, col1, color, str);
			if (done == 1)
			{
				break;
			}
			str[col2-col1 + 1] = save1;
			str[col2-col1 + 2] = save2;
			str += col2-col1;
			(*row)++;
		}
		else
		{
			driver_put_string(*row, col1, color, str);
		}
		length -= col2-col1;
		col1 = decpt; /* align with decimal */
	}
	return done;
}

/*
convert corners to center/mag
Rotation angles indicate how much the IMAGE has been rotated, not the
zoom box.  Same goes for the Skew angles
*/

#ifdef _MSC_VER
#pragma optimize("", off)
#endif

void convert_center_mag(double *Xctr, double *Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew)
{
	double Width, Height;
	double a, b; /* bottom, left, diagonal */
	double a2, b2, c2; /* squares of above */
	double tmpx1, tmpx2, tmpy1, tmpy2, tmpa; /* temporary x, y, angle */

	/* simple normal case first */
	if (g_escape_time_state.m_grid_fp.x_3rd() == g_escape_time_state.m_grid_fp.x_min() && g_escape_time_state.m_grid_fp.y_3rd() == g_escape_time_state.m_grid_fp.y_min())
	{ /* no rotation or skewing, but stretching is allowed */
		Width  = g_escape_time_state.m_grid_fp.width();
		Height = g_escape_time_state.m_grid_fp.height();
		*Xctr = g_escape_time_state.m_grid_fp.x_center();
		*Yctr = g_escape_time_state.m_grid_fp.y_center();
		*Magnification  = 2.0/Height;
		*Xmagfactor =  Height / (DEFAULT_ASPECT_RATIO*Width);
		*Rotation = 0.0;
		*Skew = 0.0;
	}
	else
	{
		/* set up triangle ABC, having sides abc */
		/* side a = bottom, b = left, c = diagonal not containing (x3rd, y3rd) */
		tmpx1 = g_escape_time_state.m_grid_fp.width();
		tmpy1 = g_escape_time_state.m_grid_fp.height();
		c2 = tmpx1*tmpx1 + tmpy1*tmpy1;

		tmpx1 = g_escape_time_state.m_grid_fp.x_max() - g_escape_time_state.m_grid_fp.x_3rd();
		tmpy1 = g_escape_time_state.m_grid_fp.y_min() - g_escape_time_state.m_grid_fp.y_3rd();
		a2 = tmpx1*tmpx1 + tmpy1*tmpy1;
		a = sqrt(a2);
		*Rotation = -MathUtil::RadiansToDegrees(atan2(tmpy1, tmpx1)); /* negative for image rotation */

		tmpx2 = g_escape_time_state.m_grid_fp.x_min() - g_escape_time_state.m_grid_fp.x_3rd();
		tmpy2 = g_escape_time_state.m_grid_fp.y_max() - g_escape_time_state.m_grid_fp.y_3rd();
		b2 = tmpx2*tmpx2 + tmpy2*tmpy2;
		b = sqrt(b2);

		tmpa = acos((a2 + b2-c2)/(2*a*b)); /* save tmpa for later use */
		*Skew = 90.0 - MathUtil::RadiansToDegrees(tmpa);

		*Xctr = g_escape_time_state.m_grid_fp.x_center();
		*Yctr = g_escape_time_state.m_grid_fp.y_center();

		Height = b*sin(tmpa);

		*Magnification  = 2.0/Height; /* 1/(h/2) */
		*Xmagfactor = Height / (DEFAULT_ASPECT_RATIO*a);

		/* if vector_a cross vector_b is negative */
		/* then adjust for left-hand coordinate system */
		if (tmpx1*tmpy2 - tmpx2*tmpy1 < 0 && g_debug_flag != DEBUGFLAG_PRE193_CENTERMAG)
		{
			*Skew = -*Skew;
			*Xmagfactor = -*Xmagfactor;
			*Magnification = -*Magnification;
		}
	}
	/* just to make par file look nicer */
	if (*Magnification < 0)
	{
		*Magnification = -*Magnification;
		*Rotation += 180;
	}
#ifdef DEBUG
	{
		double txmin, txmax, tx3rd, tymin, tymax, ty3rd;
		double error;
		txmin = g_escape_time_state.m_grid_fp.x_min();
		txmax = g_escape_time_state.m_grid_fp.x_max();
		tx3rd = g_escape_time_state.m_grid_fp.x_3rd();
		tymin = g_escape_time_state.m_grid_fp.y_min();
		tymax = g_escape_time_state.m_grid_fp.y_max();
		ty3rd = g_escape_time_state.m_grid_fp.y_3rd();
		convert_corners(*Xctr, *Yctr, *Magnification, *Xmagfactor, *Rotation, *Skew);
		error = sqr(txmin - g_escape_time_state.m_grid_fp.x_min()) +
			sqr(txmax - g_escape_time_state.m_grid_fp.x_max()) +
			sqr(tx3rd - g_escape_time_state.m_grid_fp.x_3rd()) +
			sqr(tymin - g_escape_time_state.m_grid_fp.y_min()) +
			sqr(tymax - g_escape_time_state.m_grid_fp.y_max()) +
			sqr(ty3rd - g_escape_time_state.m_grid_fp.y_3rd());
		if (error > .001)
		{
			show_corners_dbl("convert_center_mag problem");
		}
		g_escape_time_state.m_grid_fp.x_min() = txmin;
		g_escape_time_state.m_grid_fp.x_max() = txmax;
		g_escape_time_state.m_grid_fp.x_3rd() = tx3rd;
		g_escape_time_state.m_grid_fp.y_min() = tymin;
		g_escape_time_state.m_grid_fp.y_max() = tymax;
		g_escape_time_state.m_grid_fp.y_3rd() = ty3rd;
	}
#endif
	return;
}


/* convert center/mag to corners */
void convert_corners(double Xctr, double Yctr, LDBL Magnification, double Xmagfactor, double Rotation, double Skew)
{
	double x, y;
	double h, w; /* half height, width */
	double tanskew, sinrot, cosrot;

	if (Xmagfactor == 0.0)
	{
		Xmagfactor = 1.0;
	}

	h = (double)(1/Magnification);
	w = h / (DEFAULT_ASPECT_RATIO*Xmagfactor);

	if (Rotation == 0.0 && Skew == 0.0)
		{ /* simple, faster case */
		g_escape_time_state.m_grid_fp.x_3rd() = g_escape_time_state.m_grid_fp.x_min() = Xctr - w;
		g_escape_time_state.m_grid_fp.x_max() = Xctr + w;
		g_escape_time_state.m_grid_fp.y_3rd() = g_escape_time_state.m_grid_fp.y_min() = Yctr - h;
		g_escape_time_state.m_grid_fp.y_max() = Yctr + h;
		return;
		}

	/* in unrotated, untranslated coordinate system */
	tanskew = tan(MathUtil::DegreesToRadians(Skew));
	g_escape_time_state.m_grid_fp.x_min() = -w + h*tanskew;
	g_escape_time_state.m_grid_fp.x_max() =  w - h*tanskew;
	g_escape_time_state.m_grid_fp.x_3rd() = -w - h*tanskew;
	g_escape_time_state.m_grid_fp.y_max() = h;
	g_escape_time_state.m_grid_fp.y_3rd() = g_escape_time_state.m_grid_fp.y_min() = -h;

	/* rotate coord system and then translate it */
	Rotation = MathUtil::DegreesToRadians(Rotation);
	sinrot = sin(Rotation);
	cosrot = cos(Rotation);

	/* top left */
	x = g_escape_time_state.m_grid_fp.x_min()*cosrot + g_escape_time_state.m_grid_fp.y_max()*sinrot;
	y = -g_escape_time_state.m_grid_fp.x_min()*sinrot + g_escape_time_state.m_grid_fp.y_max()*cosrot;
	g_escape_time_state.m_grid_fp.x_min() = x + Xctr;
	g_escape_time_state.m_grid_fp.y_max() = y + Yctr;

	/* bottom right */
	x = g_escape_time_state.m_grid_fp.x_max()*cosrot + g_escape_time_state.m_grid_fp.y_min()*sinrot;
	y = -g_escape_time_state.m_grid_fp.x_max()*sinrot + g_escape_time_state.m_grid_fp.y_min()*cosrot;
	g_escape_time_state.m_grid_fp.x_max() = x + Xctr;
	g_escape_time_state.m_grid_fp.y_min() = y + Yctr;

	/* bottom left */
	x = g_escape_time_state.m_grid_fp.x_3rd()*cosrot + g_escape_time_state.m_grid_fp.y_3rd()*sinrot;
	y = -g_escape_time_state.m_grid_fp.x_3rd()*sinrot + g_escape_time_state.m_grid_fp.y_3rd()*cosrot;
	g_escape_time_state.m_grid_fp.x_3rd() = x + Xctr;
	g_escape_time_state.m_grid_fp.y_3rd() = y + Yctr;

	return;
}

/* convert corners to center/mag using bf */
void convert_center_mag_bf(bf_t Xctr, bf_t Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew)
{
	/* needs to be LDBL or won't work past 307 (-DBL_MIN_10_EXP) or so digits */
	LDBL Width, Height;
	LDBL a, b; /* bottom, left, diagonal */
	LDBL a2, b2, c2; /* squares of above */
	LDBL tmpx1, tmpx2, tmpy = 0.0, tmpy1, tmpy2;
	double tmpa; /* temporary x, y, angle */
	bf_t bfWidth, bfHeight;
	bf_t bftmpx, bftmpy;
	int saved;
	int signx;

	saved = save_stack();

	/* simple normal case first */
	/* if (x3rd == xmin && y3rd == ymin) */
	if (!cmp_bf(g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_min()) && !cmp_bf(g_escape_time_state.m_grid_bf.y_3rd(), g_escape_time_state.m_grid_bf.y_min()))
	{ /* no rotation or skewing, but stretching is allowed */
		bfWidth  = alloc_stack(bflength + 2);
		bfHeight = alloc_stack(bflength + 2);
		/* Width  = xmax - xmin; */
		sub_bf(bfWidth, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min());
		Width  = bftofloat(bfWidth);
		/* Height = ymax - ymin; */
		sub_bf(bfHeight, g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_min());
		Height = bftofloat(bfHeight);
		/* *Xctr = (xmin + xmax)/2; */
		add_bf(Xctr, g_escape_time_state.m_grid_bf.x_min(), g_escape_time_state.m_grid_bf.x_max());
		half_a_bf(Xctr);
		/* *Yctr = (ymin + ymax)/2; */
		add_bf(Yctr, g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_max());
		half_a_bf(Yctr);
		*Magnification  = 2/Height;
		*Xmagfactor =  (double)(Height / (DEFAULT_ASPECT_RATIO*Width));
		*Rotation = 0.0;
		*Skew = 0.0;
	}
	else
	{
		bftmpx = alloc_stack(bflength + 2);
		bftmpy = alloc_stack(bflength + 2);

		/* set up triangle ABC, having sides abc */
		/* side a = bottom, b = left, c = diagonal not containing (x3rd, y3rd) */
		/* IMPORTANT: convert from bf AFTER subtracting */

		/* tmpx = xmax - xmin; */
		sub_bf(bftmpx, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min());
		tmpx1 = bftofloat(bftmpx);
		/* tmpy = ymax - ymin; */
		sub_bf(bftmpy, g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_min());
		tmpy1 = bftofloat(bftmpy);
		c2 = tmpx1*tmpx1 + tmpy1*tmpy1;

		/* tmpx = xmax - x3rd; */
		sub_bf(bftmpx, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_3rd());
		tmpx1 = bftofloat(bftmpx);

		/* tmpy = ymin - y3rd; */
		sub_bf(bftmpy, g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_3rd());
		tmpy1 = bftofloat(bftmpy);
		a2 = tmpx1*tmpx1 + tmpy1*tmpy1;
		a = sqrtl(a2);

		/* divide tmpx and tmpy by |tmpx| so that double version of atan2() can be used */
		/* atan2() only depends on the ratio, this puts it in double's range */
		signx = sign(tmpx1);
		if (signx)
		{
			tmpy = tmpy1/tmpx1*signx;    /* tmpy = tmpy / |tmpx| */
		}
		*Rotation = (double)(-MathUtil::RadiansToDegrees(atan2((double)tmpy, signx))); /* negative for image rotation */

		/* tmpx = xmin - x3rd; */
		sub_bf(bftmpx, g_escape_time_state.m_grid_bf.x_min(), g_escape_time_state.m_grid_bf.x_3rd());
		tmpx2 = bftofloat(bftmpx);
		/* tmpy = ymax - y3rd; */
		sub_bf(bftmpy, g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_3rd());
		tmpy2 = bftofloat(bftmpy);
		b2 = tmpx2*tmpx2 + tmpy2*tmpy2;
		b = sqrtl(b2);

		tmpa = acos((double)((a2 + b2-c2)/(2*a*b))); /* save tmpa for later use */
		*Skew = 90 - MathUtil::RadiansToDegrees(tmpa);

		/* these are the only two variables that must use big precision */
		/* *Xctr = (xmin + xmax)/2; */
		add_bf(Xctr, g_escape_time_state.m_grid_bf.x_min(), g_escape_time_state.m_grid_bf.x_max());
		half_a_bf(Xctr);
		/* *Yctr = (ymin + ymax)/2; */
		add_bf(Yctr, g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_max());
		half_a_bf(Yctr);

		Height = b*sin(tmpa);
		*Magnification  = 2/Height; /* 1/(h/2) */
		*Xmagfactor = (double)(Height / (DEFAULT_ASPECT_RATIO*a));

		/* if vector_a cross vector_b is negative */
		/* then adjust for left-hand coordinate system */
		if (tmpx1*tmpy2 - tmpx2*tmpy1 < 0 && g_debug_flag != DEBUGFLAG_PRE193_CENTERMAG)
		{
			*Skew = -*Skew;
			*Xmagfactor = -*Xmagfactor;
			*Magnification = -*Magnification;
		}
	}
	if (*Magnification < 0)
	{
		*Magnification = -*Magnification;
		*Rotation += 180;
	}
	restore_stack(saved);
	return;
}


/* convert center/mag to corners using bf */
void convert_corners_bf(bf_t Xctr, bf_t Yctr, LDBL Magnification, double Xmagfactor, double Rotation, double Skew)
{
	LDBL x, y;
	LDBL h, w; /* half height, width */
	LDBL x_min, y_min, x_max, y_max, x_3rd, y_3rd;
	double tanskew, sinrot, cosrot;
	bf_t bfh, bfw;
	bf_t bftmp;
	int saved;

	saved = save_stack();
	bfh = alloc_stack(bflength + 2);
	bfw = alloc_stack(bflength + 2);

	if (Xmagfactor == 0.0)
	{
		Xmagfactor = 1.0;
	}

	h = 1/Magnification;
	floattobf(bfh, h);
	w = h / (DEFAULT_ASPECT_RATIO*Xmagfactor);
	floattobf(bfw, w);

	if (Rotation == 0.0 && Skew == 0.0)
		{ /* simple, faster case */
		/* x3rd = xmin = Xctr - w; */
		sub_bf(g_escape_time_state.m_grid_bf.x_min(), Xctr, bfw);
		copy_bf(g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_min());
		/* xmax = Xctr + w; */
		add_bf(g_escape_time_state.m_grid_bf.x_max(), Xctr, bfw);
		/* y3rd = ymin = Yctr - h; */
		sub_bf(g_escape_time_state.m_grid_bf.y_min(), Yctr, bfh);
		copy_bf(g_escape_time_state.m_grid_bf.y_3rd(), g_escape_time_state.m_grid_bf.y_min());
		/* ymax = Yctr + h; */
		add_bf(g_escape_time_state.m_grid_bf.y_max(), Yctr, bfh);
		restore_stack(saved);
		return;
		}

	bftmp = alloc_stack(bflength + 2);
	/* in unrotated, untranslated coordinate system */
	tanskew = tan(MathUtil::DegreesToRadians(Skew));
	x_min = -w + h*tanskew;
	x_max =  w - h*tanskew;
	x_3rd = -w - h*tanskew;
	y_max = h;
	y_3rd = y_min = -h;

	/* rotate coord system and then translate it */
	Rotation = MathUtil::DegreesToRadians(Rotation);
	sinrot = sin(Rotation);
	cosrot = cos(Rotation);

	/* top left */
	x =  x_min*cosrot + y_max*sinrot;
	y = -x_min*sinrot + y_max*cosrot;
	/* xmin = x + Xctr; */
	floattobf(bftmp, x);
	add_bf(g_escape_time_state.m_grid_bf.x_min(), bftmp, Xctr);
	/* ymax = y + Yctr; */
	floattobf(bftmp, y);
	add_bf(g_escape_time_state.m_grid_bf.y_max(), bftmp, Yctr);

	/* bottom right */
	x =  x_max*cosrot + y_min*sinrot;
	y = -x_max*sinrot + y_min*cosrot;
	/* xmax = x + Xctr; */
	floattobf(bftmp, x);
	add_bf(g_escape_time_state.m_grid_bf.x_max(), bftmp, Xctr);
	/* ymin = y + Yctr; */
	floattobf(bftmp, y);
	add_bf(g_escape_time_state.m_grid_bf.y_min(), bftmp, Yctr);

	/* bottom left */
	x =  x_3rd*cosrot + y_3rd*sinrot;
	y = -x_3rd*sinrot + y_3rd*cosrot;
	/* x3rd = x + Xctr; */
	floattobf(bftmp, x);
	add_bf(g_escape_time_state.m_grid_bf.x_3rd(), bftmp, Xctr);
	/* y3rd = y + Yctr; */
	floattobf(bftmp, y);
	add_bf(g_escape_time_state.m_grid_bf.y_3rd(), bftmp, Yctr);

	restore_stack(saved);
	return;
}

#ifdef _MSC_VER
#pragma optimize("", on)
#endif

void update_save_name(char *filename) /* go to the next file name */
{
	char *save, *hold;
	char drive[FILE_MAX_DRIVE];
	char dir[FILE_MAX_DIR];
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];

	split_path(filename , drive, dir, fname, ext);

	hold = fname + strlen(fname) - 1; /* start at the end */
	while (hold >= fname && (*hold == ' ' || isdigit(*hold))) /* skip backwards */
	{
		hold--;
	}
	hold++;                      /* recover first digit */
	while (*hold == '0')         /* skip leading zeros */
	{
		hold++;
	}
	save = hold;
	while (*save)  /* check for all nines */
	{
		if (*save != '9')
		{
			break;
		}
		save++;
		}
	if (!*save)                  /* if the whole thing is nines then back */
	{
		save = hold - 1;          /* up one place. Note that this will eat */
	}
								/* your last letter if you go to far.    */
	else
	{
		save = hold;
	}
	sprintf(save, "%ld", atol(hold) + 1); /* increment the number */
	make_path(filename, drive, dir, fname, ext);
}

int check_write_file(char *name, const char *ext)
{
	/* TODO: change encoder.cpp to also use this routine */
nextname:
	char openfile[FILE_MAX_DIR];
	strcpy(openfile, name);
	char opentype[20];
	strcpy(opentype, ext);
	char *period = has_extension(openfile);
	if (period != NULL)
	{
		strcpy(opentype, period);
		*period = 0;
	}
	strcat(openfile, opentype);
	if (access(openfile, 0) != 0) /* file doesn't exist */
	{
		strcpy(name, openfile);
		return 0;
	}
	/* file already exists */
	if (!g_fractal_overwrite)
	{
		update_save_name(name);
		goto nextname;
	}
	return 1;
}

/* ('check_key()' was moved to FRACTINT.C for MSC7-overlay speed purposes) */
/* ('timer()'     was moved to FRACTINT.C for MSC7-overlay speed purposes) */

BYTE g_trig_index[] = {SIN, SQR, SINH, COSH};
#if !defined(XFRACT)
void (*g_trig0_l)() = lStkSin;
void (*g_trig1_l)() = lStkSqr;
void (*g_trig2_l)() = lStkSinh;
void (*g_trig3_l)() = lStkCosh;
void (*g_trig0_m)() = mStkSin;
void (*g_trig1_m)() = mStkSqr;
void (*g_trig2_m)() = mStkSinh;
void (*g_trig3_m)() = mStkCosh;
#endif
void (*g_trig0_d)() = dStkSin;
void (*g_trig1_d)() = dStkSqr;
void (*g_trig2_d)() = dStkSinh;
void (*g_trig3_d)() = dStkCosh;

/* struct trig_funct_lst trigfn[]  was moved to prompts1.c */

void show_trig(char *buf) /* return display form of active trig functions */
{
	char tmpbuf[30];
	*buf = 0; /* null string if none */
	trigdetails(tmpbuf);
	if (tmpbuf[0])
	{
		sprintf(buf, " function=%s", tmpbuf);
	}
}

static void trigdetails(char *buf)
{
	int i, numfn;
	char tmpbuf[20];
	if (g_fractal_type == FRACTYPE_JULIBROT || g_fractal_type == FRACTYPE_JULIBROT_FP)
	{
		numfn = (g_fractal_specific[g_new_orbit_type].flags >> 6) & 7;
	}
	else
	{
		numfn = (g_current_fractal_specific->flags >> 6) & 7;
	}
	if (g_current_fractal_specific == &g_fractal_specific[FRACTYPE_FORMULA] ||
		g_current_fractal_specific == &g_fractal_specific[FRACTYPE_FORMULA_FP])
	{
		numfn = g_max_fn;
	}
	*buf = 0; /* null string if none */
	if (numfn > 0)
	{
		strcpy(buf, trigfn[g_trig_index[0]].name);
		i = 0;
		while (++i < numfn)
		{
			sprintf(tmpbuf, "/%s", trigfn[g_trig_index[i]].name);
			strcat(buf, tmpbuf);
		}
	}
}

/* set array of trig function indices according to "function=" command */
int set_trig_array(int k, const char *name)
{
	char trigname[10];
	int i;
	char *slash;
	strncpy(trigname, name, 6);
	trigname[6] = 0; /* safety first */

	slash = strchr(trigname, '/');
	if (slash != NULL)
	{
		*slash = 0;
	}

	strlwr(trigname);

	for (i = 0; i < g_num_trig_fn; i++)
	{
		if (strcmp(trigname, trigfn[i].name) == 0)
		{
			g_trig_index[k] = (BYTE)i;
			set_trig_pointers(k);
			break;
		}
	}
	return 0;
}
void set_trig_pointers(int which)
{
	/* set trig variable functions to avoid array lookup time */
	int i;
	switch (which)
	{
	case 0:
#if !defined(XFRACT) && !defined(_WIN32)
		g_trig0_l = trigfn[g_trig_index[0]].lfunct;
		g_trig0_m = trigfn[g_trig_index[0]].mfunct;
#endif
		g_trig0_d = trigfn[g_trig_index[0]].dfunct;
		break;
	case 1:
#if !defined(XFRACT) && !defined(_WIN32)
		g_trig1_l = trigfn[g_trig_index[1]].lfunct;
		g_trig1_m = trigfn[g_trig_index[1]].mfunct;
#endif
		g_trig1_d = trigfn[g_trig_index[1]].dfunct;
		break;
	case 2:
#if !defined(XFRACT) && !defined(_WIN32)
		g_trig2_l = trigfn[g_trig_index[2]].lfunct;
		g_trig2_m = trigfn[g_trig_index[2]].mfunct;
#endif
		g_trig2_d = trigfn[g_trig_index[2]].dfunct;
		break;
	case 3:
#if !defined(XFRACT) && !defined(_WIN32)
		g_trig3_l = trigfn[g_trig_index[3]].lfunct;
		g_trig3_m = trigfn[g_trig_index[3]].mfunct;
#endif
		g_trig3_d = trigfn[g_trig_index[3]].dfunct;
		break;
	default: /* do 'em all */
		for (i = 0; i < 4; i++)
		{
			set_trig_pointers(i);
		}
		break;
	}
}

#ifdef XFRACT
static char spressanykey[] = {"Press any key to continue, F6 for area, F7 for next page"};
#else
static char spressanykey[] = {"Press any key to continue, F6 for area, CTRL-TAB for next page"};
#endif

void get_calculation_time(char *msg, long ctime)
{
	if (ctime >= 0)
	{
		sprintf(msg, "%3ld:%02ld:%02ld.%02ld", ctime/360000L,
			(ctime % 360000L)/6000, (ctime % 6000)/100, ctime % 100);
	}
	else
	{
		strcpy(msg, "A long time! (> 24.855 days)");
	}
}

static void show_str_var(char *name, char *var, int *row, char *msg)
{
	if (var == NULL)
	{
		return;
	}
	if (*var != 0)
	{
		sprintf(msg, "%s=%s", name, var);
		driver_put_string((*row)++, 2, C_GENERAL_HI, msg);
	}
}

static void
write_row(int row, const char *format, ...)
{
	char text[78] = { 0 };
	va_list args;

	va_start(args, format);
	_vsnprintf(text, NUM_OF(text), format, args);
	va_end(args);

	driver_put_string(row, 2, C_GENERAL_HI, text);
}

int tab_display_2(char *msg)
{
	int row, key = 0;

	help_title();
	driver_set_attr(1, 0, C_GENERAL_MED, 24*80); /* init rest to background */

	row = 1;
	put_string_center(row++, 0, 80, C_PROMPT_HI, "Top Secret Developer's Screen");

	write_row(++row, "Version %d patch %d", g_release, g_patch_level);
	write_row(++row, "%ld of %ld bignum memory used", g_bn_max_stack, maxstack);
	write_row(++row, "   %ld used for bignum globals", startstack);
	write_row(++row, "   %ld stack used == %ld variables of length %d",
			g_bn_max_stack-startstack, (long)((g_bn_max_stack-startstack)/(rbflength + 2)), rbflength + 2);
	if (g_bf_math)
	{
		write_row(++row, "intlength %-d bflength %-d ", intlength, bflength);
	}
	row++;
	show_str_var("tempdir",     g_temp_dir,      &row, msg);
	show_str_var("workdir",     g_work_dir,      &row, msg);
	show_str_var("filename",    g_read_name,     &row, msg);
	show_str_var("formulafile", g_formula_filename, &row, msg);
	show_str_var("savename",    g_save_name,     &row, msg);
	show_str_var("parmfile",    g_command_file,  &row, msg);
	show_str_var("ifsfile",     g_ifs_filename,  &row, msg);
	show_str_var("autokeyname", g_autokey_name,     &row, msg);
	show_str_var("lightname",   g_light_name,   &row, msg);
	show_str_var("map",         g_map_name,     &row, msg);
	write_row(row++, "Sizeof g_fractal_specific array %d",
		g_num_fractal_types*(int)sizeof(FractalTypeSpecificData));
	write_row(row++, "CalculationStatus %d pixel [%d, %d]", g_calculation_status, g_col, g_row);
	if (g_fractal_type == FRACTYPE_FORMULA || g_fractal_type == FRACTYPE_FORMULA_FP)
	{
		write_row(row++, "TotalFormulaMem %ld MaxOps (posp) %u MaxArgs (vsp) %u",
			g_total_formula_mem, g_posp, g_parser_vsp);
		write_row(row++, "   Store ptr %d Loadptr %d MaxOps var %u MaxArgs var %u LastInitOp %d",
			g_store_ptr, g_lod_ptr, g_formula_max_ops, g_formula_max_args, g_last_init_op);
	}
	else if (g_rhombus_stack[0])
	{
		write_row(row++, "SOI Recursion %d stack free %d %d %d %d %d %d %d %d %d %d",
			g_max_rhombus_depth + 1,
			g_rhombus_stack[0], g_rhombus_stack[1], g_rhombus_stack[2],
			g_rhombus_stack[3], g_rhombus_stack[4], g_rhombus_stack[5],
			g_rhombus_stack[6], g_rhombus_stack[7], g_rhombus_stack[8],
			g_rhombus_stack[9]);
	}

	write_row(row++, "%dx%d dm=%d %s (%s)", g_x_dots, g_y_dots, g_dot_mode,
		driver_name(), driver_description());
	write_row(row++, "xx: start %d, stop %d; yy: start %d, stop %d %s UsesIsMand %d",
		g_xx_start, g_xx_stop, g_yy_start, g_yy_stop,
#if !defined(XFRACT) && !defined(_WIN32)
		g_current_fractal_specific->orbitcalc == fFormula ? "fast parser" :
#endif
		g_current_fractal_specific->orbitcalc ==  Formula ? "slow parser" :
		g_current_fractal_specific->orbitcalc ==  BadFormula ? "bad formula" :
		"", g_uses_is_mand);
	/*
	char message[80] = { 0 };
	extern void tile_message(char *message, int message_len);
	tile_message(message, NUM_OF(message));
	write_row(row++, message)
	write_row(row++, "ixstart %d g_x_stop %d iystart %d g_y_stop %d g_bit_shift %d",
	ixstart, g_x_stop, iystart, g_y_stop, g_bit_shift);
	*/
	write_row(row++, "g_minimum_stack_available %d g_limit2_l %ld g_use_grid %s",
		g_minimum_stack_available, g_limit2_l, g_escape_time_state.m_use_grid ? "true" : "false");
	put_string_center(24, 0, 80, C_GENERAL_LO, "Press Esc to continue, Backspace for first screen");
	*msg = 0;

	/* display keycodes while waiting for ESC, BACKSPACE or TAB */
	while ((key != FIK_ESC) && (key != FIK_BACKSPACE) && (key != FIK_TAB))
	{
		driver_put_string(row, 2, C_GENERAL_HI, msg);
		key = getakeynohelp();
		sprintf(msg, "%d (0x%04x)      ", key, key);
	}
	return (key != FIK_ESC);
}

int tab_display()       /* display the status of the current image */
{
	int s_row, i, j, addrow = 0;
	double Xctr, Yctr;
	LDBL Magnification;
	double Xmagfactor, Rotation, Skew;
	bf_t bfXctr = NULL, bfYctr = NULL;
	char msg[350];
	char *msgptr;
	int key;
	int saved = 0;
	int dec;
	int k;
	int hasformparam = 0;

	if (g_calculation_status < CALCSTAT_PARAMS_CHANGED)        /* no active fractal image */
	{
		return 0;                /* (no TAB on the credits screen) */
	}
	if (g_calculation_status == CALCSTAT_IN_PROGRESS)        /* next assumes CLK_TCK is 10^n, n >= 2 */
	{
		g_calculation_time += (clock_ticks() - g_timer_start) / (CLK_TCK/100);
	}
	driver_stack_screen();
	if (g_bf_math)
	{
		saved = save_stack();
		bfXctr = alloc_stack(bflength + 2);
		bfYctr = alloc_stack(bflength + 2);
	}
	if (g_fractal_type == FRACTYPE_FORMULA || g_fractal_type == FRACTYPE_FORMULA_FP)
	{
		for (i = 0; i < MAX_PARAMETERS; i += 2)
		{
			if (!parameter_not_used(i))
			{
				hasformparam++;
			}
		}
	}

top:
	k = 0; /* initialize here so parameter line displays correctly on return
				from control-tab */
	help_title();
	driver_set_attr(1, 0, C_GENERAL_MED, 24*80); /* init rest to background */
	s_row = 2;
	driver_put_string(s_row, 2, C_GENERAL_MED, "Fractal type:");
	if (g_display_3d > DISPLAY3D_NONE)
	{
		driver_put_string(s_row, 16, C_GENERAL_HI, "3D Transform");
	}
	else
	{
		driver_put_string(s_row, 16, C_GENERAL_HI,
			g_current_fractal_specific->name[0] == '*' ?
				&g_current_fractal_specific->name[1] : g_current_fractal_specific->name);
		i = 0;
		if (g_fractal_type == FRACTYPE_FORMULA || g_fractal_type == FRACTYPE_FORMULA_FP)
		{
			driver_put_string(s_row + 1, 3, C_GENERAL_MED, "Item name:");
			driver_put_string(s_row + 1, 16, C_GENERAL_HI, g_formula_name);
			i = (int) strlen(g_formula_name) + 1;
			driver_put_string(s_row + 2, 3, C_GENERAL_MED, "Item file:");
			if ((int) strlen(g_formula_filename) >= 29)
			{
				addrow = 1;
			}
			driver_put_string(s_row + 2 + addrow, 16, C_GENERAL_HI, g_formula_filename);
		}
		trigdetails(msg);
		driver_put_string(s_row + 1, 16 + i, C_GENERAL_HI, msg);
		if (g_fractal_type == FRACTYPE_L_SYSTEM)
		{
			driver_put_string(s_row + 1, 3, C_GENERAL_MED, "Item name:");
			driver_put_string(s_row + 1, 16, C_GENERAL_HI, g_l_system_name);
			driver_put_string(s_row + 2, 3, C_GENERAL_MED, "Item file:");
			if ((int) strlen(g_l_system_filename) >= 28)
			{
				addrow = 1;
			}
			driver_put_string(s_row + 2 + addrow, 16, C_GENERAL_HI, g_l_system_filename);
		}
		if (g_fractal_type == FRACTYPE_IFS || g_fractal_type == FRACTYPE_IFS_3D)
		{
			driver_put_string(s_row + 1, 3, C_GENERAL_MED, "Item name:");
			driver_put_string(s_row + 1, 16, C_GENERAL_HI, g_ifs_name);
			driver_put_string(s_row + 2, 3, C_GENERAL_MED, "Item file:");
			if ((int) strlen(g_ifs_filename) >= 28)
			{
				addrow = 1;
			}
			driver_put_string(s_row + 2 + addrow, 16, C_GENERAL_HI, g_ifs_filename);
		}
	}

	switch (g_calculation_status)
	{
	case CALCSTAT_PARAMS_CHANGED:	msgptr = "Parms chgd since generated"; break;
	case CALCSTAT_IN_PROGRESS:		msgptr = "Still being generated"; break;
	case CALCSTAT_RESUMABLE:		msgptr = "Interrupted, resumable"; break;
	case CALCSTAT_NON_RESUMABLE:	msgptr = "Interrupted, non-resumable"; break;
	case CALCSTAT_COMPLETED:		msgptr = "Image completed"; break;
	default:						msgptr = "";
	}
	driver_put_string(s_row, 45, C_GENERAL_HI, msgptr);
	if (g_initialize_batch && g_calculation_status != CALCSTAT_PARAMS_CHANGED)
	{
		driver_put_string(-1, -1, C_GENERAL_HI, " (Batch mode)");
	}

	if (get_help_mode() == HELPCYCLING)
	{
		driver_put_string(s_row + 1, 45, C_GENERAL_HI, "You are in color-cycling mode");
	}
	++s_row;
	/* if (g_bf_math == 0) */
	++s_row;

	i = j = 0;
	if (g_display_3d > DISPLAY3D_NONE)
	{
		if (g_user_float_flag)
		{
			j = 1;
		}
	}
	else if (g_float_flag)
	{
		j = (g_user_float_flag) ? 1 : 2;
	}

	if (g_bf_math == 0)
	{
		if (j)
		{
			driver_put_string(s_row, 45, C_GENERAL_HI, "Floating-point");
			driver_put_string(-1, -1, C_GENERAL_HI,
				(j == 1) ? " flag is activated" : " in use (required)");
		}
		else
		{
			driver_put_string(s_row, 45, C_GENERAL_HI, "Integer math is in use");
		}
	}
	else
	{
		sprintf(msg, "(%-d decimals)", g_decimals /*get_precision_bf(CURRENTREZ)*/);
		driver_put_string(s_row, 45, C_GENERAL_HI, "Arbitrary precision ");
		driver_put_string(-1, -1, C_GENERAL_HI, msg);
	}
	i = 1;

	s_row += i;

	if (g_calculation_status == CALCSTAT_IN_PROGRESS || g_calculation_status == CALCSTAT_RESUMABLE)
	{
		if (g_current_fractal_specific->flags & FRACTALFLAG_NOT_RESUMABLE)
		{
			driver_put_string(s_row++, 2, C_GENERAL_HI,
				"Note: can't resume this type after interrupts other than <tab> and <F1>");
		}
	}
	s_row += addrow;
	driver_put_string(s_row, 2, C_GENERAL_MED, "Savename: ");
	driver_put_string(s_row, -1, C_GENERAL_HI, g_save_name);

	++s_row;

	if ((g_got_status >= GOT_STATUS_12PASS) &&
		(g_calculation_status == CALCSTAT_IN_PROGRESS || g_calculation_status == CALCSTAT_RESUMABLE))
	{
		switch (g_got_status)
		{
		case GOT_STATUS_12PASS:
			sprintf(msg, "%d Pass Mode", g_total_passes);
			driver_put_string(s_row, 2, C_GENERAL_HI, msg);
			if (g_user_standard_calculation_mode == '3')
			{
				driver_put_string(s_row, -1, C_GENERAL_HI, " (threepass)");
			}
			break;
		case GOT_STATUS_GUESSING:
			driver_put_string(s_row, 2, C_GENERAL_HI, "Solid Guessing");
			if (g_user_standard_calculation_mode == '3')
			{
				driver_put_string(s_row, -1, C_GENERAL_HI, " (threepass)");
			}
			break;
		case GOT_STATUS_BOUNDARY_TRACE:
			driver_put_string(s_row, 2, C_GENERAL_HI, "Boundary Tracing");
			break;
		case GOT_STATUS_3D:
			sprintf(msg, "Processing row %d (of %d) of input image", g_current_row, g_file_y_dots);
			driver_put_string(s_row, 2, C_GENERAL_HI, msg);
			break;
		case GOT_STATUS_TESSERAL:
			driver_put_string(s_row, 2, C_GENERAL_HI, "Tesseral");
			break;
		case GOT_STATUS_DIFFUSION:
			driver_put_string(s_row, 2, C_GENERAL_HI, "Diffusion");
			break;
		case GOT_STATUS_ORBITS:
			driver_put_string(s_row, 2, C_GENERAL_HI, "Orbits");
			break;
		}
		++s_row;
		if (g_got_status == GOT_STATUS_DIFFUSION)
		{
			sprintf(msg, "%2.2f%% done, counter at %lu of %lu (%u bits)",
					(100.0*g_diffusion_counter)/g_diffusion_limit,
					g_diffusion_counter, g_diffusion_limit, g_bits);
			driver_put_string(s_row, 2, C_GENERAL_MED, msg);
			++s_row;
		}
		else
		if (g_got_status != GOT_STATUS_3D)
		{
			sprintf(msg, "Working on block (y, x) [%d, %d]...[%d, %d], ",
					g_yy_start, g_xx_start, g_yy_stop, g_xx_stop);
			driver_put_string(s_row, 2, C_GENERAL_MED, msg);
			if (g_got_status == GOT_STATUS_BOUNDARY_TRACE || g_got_status == GOT_STATUS_TESSERAL)  /* btm or tesseral */
			{
				driver_put_string(-1, -1, C_GENERAL_MED, "at ");
				sprintf(msg, "[%d, %d]", g_current_row, g_current_col);
				driver_put_string(-1, -1, C_GENERAL_HI, msg);
			}
			else
			{
				if (g_total_passes > 1)
				{
					driver_put_string(-1, -1, C_GENERAL_MED, "pass ");
					sprintf(msg, "%d", g_current_pass);
					driver_put_string(-1, -1, C_GENERAL_HI, msg);
					driver_put_string(-1, -1, C_GENERAL_MED, " of ");
					sprintf(msg, "%d", g_total_passes);
					driver_put_string(-1, -1, C_GENERAL_HI, msg);
					driver_put_string(-1, -1, C_GENERAL_MED, ", ");
				}
				driver_put_string(-1, -1, C_GENERAL_MED, "at row ");
				sprintf(msg, "%d", g_current_row);
				driver_put_string(-1, -1, C_GENERAL_HI, msg);
				driver_put_string(-1, -1, C_GENERAL_MED, " col ");
				sprintf(msg, "%d", g_col);
				driver_put_string(-1, -1, C_GENERAL_HI, msg);
			}
			++s_row;
		}
	}
	driver_put_string(s_row, 2, C_GENERAL_MED, "Calculation time:");
	get_calculation_time(msg, g_calculation_time);
	driver_put_string(-1, -1, C_GENERAL_HI, msg);
	if ((g_got_status == GOT_STATUS_DIFFUSION) && (g_calculation_status == CALCSTAT_IN_PROGRESS))  /* estimate total time */
	{
		driver_put_string(-1, -1, C_GENERAL_MED, " estimated total time: ");
		get_calculation_time(msg, (long)(g_calculation_time*((g_diffusion_limit*1.0)/g_diffusion_counter)));
		driver_put_string(-1, -1, C_GENERAL_HI, msg);
	}

	if ((g_current_fractal_specific->flags&FRACTALFLAG_INFINITE_CALCULATION) && (g_color_iter != 0))
	{
		driver_put_string(s_row, -1, C_GENERAL_MED, " 1000's of points:");
		sprintf(msg, " %ld of %ld", g_color_iter-2, g_max_count);
		driver_put_string(s_row, -1, C_GENERAL_HI, msg);
	}

	++s_row;
	if (g_bf_math == 0)
	{
		++s_row;
	}
	_snprintf(msg, NUM_OF(msg), "driver: %s, %s", driver_name(), driver_description());
	driver_put_string(s_row++, 2, C_GENERAL_MED, msg);
	if (g_video_entry.x_dots && g_bf_math == 0)
	{
		sprintf(msg, "Video: %dx%dx%d %s %s",
				g_video_entry.x_dots, g_video_entry.y_dots, g_video_entry.colors,
				g_video_entry.name, g_video_entry.comment);
		driver_put_string(s_row++, 2, C_GENERAL_MED, msg);
	}
	if (!(g_current_fractal_specific->flags & FRACTALFLAG_NO_ZOOM))
	{
		adjust_corner(); /* make bottom left exact if very near exact */
		if (g_bf_math)
		{
			int truncate, truncaterow;
			dec = min(320, g_decimals);
			adjust_corner_bf(); /* make bottom left exact if very near exact */
			convert_center_mag_bf(bfXctr, bfYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
			/* find alignment information */
			msg[0] = 0;
			truncate = 0;
			if (dec < g_decimals)
			{
				truncate = 1;
			}
			truncaterow = g_row;
			driver_put_string(++s_row, 2, C_GENERAL_MED, "Ctr");
			driver_put_string(s_row, 8, C_GENERAL_MED, "x");
			bftostr(msg, dec, bfXctr);
			if (putstringwrap(&s_row, 10, 78, C_GENERAL_HI, msg, 5) == 1)
			{
				truncate = 1;
			}
			driver_put_string(++s_row, 8, C_GENERAL_MED, "y");
			bftostr(msg, dec, bfYctr);
			if (putstringwrap(&s_row, 10, 78, C_GENERAL_HI, msg, 5) == 1 || truncate)
			{
				driver_put_string(truncaterow, 2, C_GENERAL_MED, "(Center values shown truncated to 320 decimals)");
			}
			driver_put_string(++s_row, 2, C_GENERAL_MED, "Mag");
#ifdef USE_LONG_DOUBLE
			sprintf(msg, "%10.8Le", Magnification);
#else
			sprintf(msg, "%10.8le", Magnification);
#endif
			driver_put_string(-1, 11, C_GENERAL_HI, msg);
			driver_put_string(++s_row, 2, C_GENERAL_MED, "X-Mag-Factor");
			sprintf(msg, "%11.4f   ", Xmagfactor);
			driver_put_string(-1, -1, C_GENERAL_HI, msg);
			driver_put_string(-1, -1, C_GENERAL_MED, "Rotation");
			sprintf(msg, "%9.3f   ", Rotation);
			driver_put_string(-1, -1, C_GENERAL_HI, msg);
			driver_put_string(-1, -1, C_GENERAL_MED, "Skew");
			sprintf(msg, "%9.3f", Skew);
			driver_put_string(-1, -1, C_GENERAL_HI, msg);
		}
		else /* bf != 1 */
		{
			driver_put_string(s_row, 2, C_GENERAL_MED, "Corners:                X                     Y");
			driver_put_string(++s_row, 3, C_GENERAL_MED, "Top-l");
			sprintf(msg, "%20.16f  %20.16f", g_escape_time_state.m_grid_fp.x_min(), g_escape_time_state.m_grid_fp.y_max());
			driver_put_string(-1, 17, C_GENERAL_HI, msg);
			driver_put_string(++s_row, 3, C_GENERAL_MED, "Bot-r");
			sprintf(msg, "%20.16f  %20.16f", g_escape_time_state.m_grid_fp.x_max(), g_escape_time_state.m_grid_fp.y_min());
			driver_put_string(-1, 17, C_GENERAL_HI, msg);

			if (g_escape_time_state.m_grid_fp.x_min() != g_escape_time_state.m_grid_fp.x_3rd() || g_escape_time_state.m_grid_fp.y_min() != g_escape_time_state.m_grid_fp.y_3rd())
			{
				driver_put_string(++s_row, 3, C_GENERAL_MED, "Bot-l");
				sprintf(msg, "%20.16f  %20.16f", g_escape_time_state.m_grid_fp.x_3rd(), g_escape_time_state.m_grid_fp.y_3rd());
				driver_put_string(-1, 17, C_GENERAL_HI, msg);
			}
			convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
			driver_put_string(s_row += 2, 2, C_GENERAL_MED, "Ctr");
			sprintf(msg, "%20.16f %20.16f  ", Xctr, Yctr);
			driver_put_string(-1, -1, C_GENERAL_HI, msg);
			driver_put_string(-1, -1, C_GENERAL_MED, "Mag");
#ifdef USE_LONG_DOUBLE
			sprintf(msg, " %10.8Le", Magnification);
#else
			sprintf(msg, " %10.8le", Magnification);
#endif
			driver_put_string(-1, -1, C_GENERAL_HI, msg);
			driver_put_string(++s_row, 2, C_GENERAL_MED, "X-Mag-Factor");
			sprintf(msg, "%11.4f   ", Xmagfactor);
			driver_put_string(-1, -1, C_GENERAL_HI, msg);
			driver_put_string(-1, -1, C_GENERAL_MED, "Rotation");
			sprintf(msg, "%9.3f   ", Rotation);
			driver_put_string(-1, -1, C_GENERAL_HI, msg);
			driver_put_string(-1, -1, C_GENERAL_MED, "Skew");
			sprintf(msg, "%9.3f", Skew);
			driver_put_string(-1, -1, C_GENERAL_HI, msg);
		}
	}

	if (type_has_parameter(g_fractal_type, 0, msg) || hasformparam)
	{
		for (i = 0; i < MAX_PARAMETERS; i++)
		{
			int col;
			char p[50];
			if (type_has_parameter(g_fractal_type, i, p))
			{
				if (k % 4 == 0)
				{
					s_row++;
					col = 9;
				}
				else
				{
					col = -1;
				}
				if (k == 0) /* only true with first displayed parameter */
				{
					driver_put_string(++s_row, 2, C_GENERAL_MED, "Params ");
				}
				sprintf(msg, "%3d: ", i + 1);
				driver_put_string(s_row, col, C_GENERAL_MED, msg);
				if (*p == '+')
				{
					sprintf(msg, "%-12d", (int)g_parameters[i]);
				}
				else if (*p == '#')
				{
					sprintf(msg, "%-12lu", (U32)g_parameters[i]);
				}
				else
				{
					sprintf(msg, "%-12.9f", g_parameters[i]);
				}
				driver_put_string(-1, -1, C_GENERAL_HI, msg);
				k++;
			}
		}
	}
	driver_put_string(s_row += 2, 2, C_GENERAL_MED, "Current (Max) Iteration: ");
	sprintf(msg, "%ld (%ld)", g_color_iter, g_max_iteration);
	driver_put_string(-1, -1, C_GENERAL_HI, msg);
	driver_put_string(-1, -1, C_GENERAL_MED, "     Effective bailout: ");
	sprintf(msg, "%f", g_rq_limit);
	driver_put_string(-1, -1, C_GENERAL_HI, msg);

	if (g_fractal_type == FRACTYPE_PLASMA || g_fractal_type == FRACTYPE_ANT || g_fractal_type == FRACTYPE_CELLULAR)
	{
		driver_put_string(++s_row, 2, C_GENERAL_MED, "Current 'rseed': ");
		sprintf(msg, "%d", g_random_seed);
		driver_put_string(-1, -1, C_GENERAL_HI, msg);
	}

	if (g_invert)
	{
		driver_put_string(++s_row, 2, C_GENERAL_MED, "Inversion radius: ");
		sprintf(msg, "%12.9f", g_f_radius);
		driver_put_string(-1, -1, C_GENERAL_HI, msg);
		driver_put_string(-1, -1, C_GENERAL_MED, "  xcenter: ");
		sprintf(msg, "%12.9f", g_f_x_center);
		driver_put_string(-1, -1, C_GENERAL_HI, msg);
		driver_put_string(-1, -1, C_GENERAL_MED, "  ycenter: ");
		sprintf(msg, "%12.9f", g_f_y_center);
		driver_put_string(-1, -1, C_GENERAL_HI, msg);
	}

	s_row += 2;
	if (s_row < 23)
	{
		++s_row;
	}
	/*waitforkey:*/
	put_string_center(/*s_row*/24, 0, 80, C_GENERAL_LO, spressanykey);
	driver_hide_text_cursor();
#ifdef XFRACT
	while (driver_key_pressed())
	{
		driver_get_key();
	}
#endif
	key = getakeynohelp();
	if (key == FIK_F6)
	{
		driver_stack_screen();
		area();
		driver_unstack_screen();
		goto top;
	}
	else if (key == FIK_CTL_TAB || key == FIK_SHF_TAB || key == FIK_F7)
	{
		if (tab_display_2(msg))
		{
			goto top;
		}
	}
	driver_unstack_screen();
	g_timer_start = clock_ticks(); /* tab display was "time out" */
	if (g_bf_math)
	{
		restore_stack(saved);
	}
	return 0;
}

static void area()
{
	/* apologies to UNIX folks, we PC guys have to save near space */
	char *msg;
	int x, y;
	char buf[160];
	long cnt = 0;
	if (g_inside < 0)
	{
		stop_message(0, "Need solid inside to compute area");
		return;
	}
	for (y = 0; y < g_y_dots; y++)
	{
		for (x = 0; x < g_x_dots; x++)
		{
			if (getcolor(x, y) == g_inside)
			{
				cnt++;
			}
		}
	}
	if (g_inside > 0 && g_outside < 0 && g_max_iteration > g_inside)
	{
		msg = "Warning: inside may not be unique\n";
	}
	else
	{
		msg = "";
	}
	sprintf(buf, "%s%ld inside pixels of %ld%s%f",
			msg, cnt, (long)g_x_dots*(long)g_y_dots, ".  Total area ",
			cnt/((float)g_x_dots*(float)g_y_dots)*(g_escape_time_state.m_grid_fp.x_max()-g_escape_time_state.m_grid_fp.x_min())*(g_escape_time_state.m_grid_fp.y_max()-g_escape_time_state.m_grid_fp.y_min()));
	stop_message(STOPMSG_NO_BUZZER, buf);
}

int ends_with_slash(char *fl)
{
	int len;
	len = (int) strlen(fl);
	if (len)
	{
		if (fl[--len] == SLASHC)
		{
			return 1;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------- */
static char seps[] = {" \t\n\r"};
char *get_ifs_token(char *buf, FILE *ifsfile)
{
	char *bufptr;
	while (1)
	{
		if (file_gets(buf, 200, ifsfile) < 0)
		{
			return NULL;
		}
		else
		{
			bufptr = strchr(buf, ';');
			if (bufptr != NULL) /* use ';' as comment to eol */
			{
				*bufptr = 0;
			}
			bufptr = strtok(buf, seps);
			if (bufptr != NULL)
			{
				return bufptr;
			}
		}
	}
}

char g_insufficient_ifs_memory[] = {"Insufficient memory for IFS"};
int g_num_affine;
int ifs_load()                   /* read in IFS parameters */
{
	int i;
	FILE *ifsfile;
	char buf[201];
	char *bufptr;
	int ret, rowsize;

	if (g_ifs_definition)  /* release prior parms */
	{
		free((char *)g_ifs_definition);
		g_ifs_definition = NULL;
		}

	g_ifs_type = IFSTYPE_2D;
	rowsize = IFSPARM;
	if (find_file_item(g_ifs_filename, g_ifs_name, &ifsfile, 3) < 0)
	{
		return -1;
	}

	file_gets(buf, 200, ifsfile);
	bufptr = strchr(buf, ';');
	if (bufptr != NULL) /* use ';' as comment to eol */
	{
		*bufptr = 0;
	}

	strlwr(buf);
	bufptr = &buf[0];
	while (*bufptr)
	{
		if (strncmp(bufptr, "(3d)", 4) == 0)
		{
			g_ifs_type = IFSTYPE_3D;
			rowsize = IFS3DPARM;
		}
		++bufptr;
	}

	for (i = 0; i < (NUMIFS + 1)*IFS3DPARM; ++i)
	{
		((float *)g_text_stack)[i] = 0;
	}
	i = ret = 0;
	bufptr = get_ifs_token(buf, ifsfile);
	while (bufptr != NULL)
	{
		if (sscanf(bufptr, " %f ", &((float *)g_text_stack)[i]) != 1)
		{
			break;
		}
		if (++i >= NUMIFS*rowsize)
		{
				stop_message(0, "IFS definition has too many lines");
				ret = -1;
				break;
		}
		bufptr = strtok(NULL, seps);
		if (bufptr == NULL)
		{
			bufptr = get_ifs_token(buf, ifsfile);
			if (bufptr == NULL)
			{
				ret = -1;
				break;
			}
		}
		if (ret == -1)
		{
			break;
		}
		if (*bufptr == '}')
		{
			break;
		}
	}

	if ((i % rowsize) != 0 || *bufptr != '}')
	{
		stop_message(0, "invalid IFS definition");
		ret = -1;
		}
	if (i == 0 && ret == 0)
	{
		stop_message(0, "Empty IFS definition");
		ret = -1;
		}
	fclose(ifsfile);

	if (ret == 0)
	{
		g_num_affine = i/rowsize;
		g_ifs_definition = (float *) malloc((long) ((NUMIFS + 1)*IFS3DPARM*sizeof(float)));
		if (g_ifs_definition == NULL)
		{
			stop_message(0, g_insufficient_ifs_memory);
			ret = -1;
		}
		else
		{
			for (i = 0; i < (NUMIFS + 1)*IFS3DPARM; ++i)
			{
				g_ifs_definition[i] = ((float *)g_text_stack)[i];
			}
		}
	}
	return ret;
}
/* TW 5-31-94 - added search of current directory for entry files if
	entry item not found */

int find_file_item(char *filename, char *itemname, FILE **fileptr, int itemtype)
{
	FILE *infile = NULL;
	int found = 0;
	char parsearchname[ITEMNAMELEN + 6];
	char drive[FILE_MAX_DRIVE];
	char dir[FILE_MAX_DIR];
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	char fullpath[FILE_MAX_PATH];
	char defaultextension[5];


	split_path(filename, drive, dir, fname, ext);
	make_path(fullpath, "", "", fname, ext);
	if (stricmp(filename, g_command_file))
	{
		infile = fopen(filename, "rb");
		if (infile != NULL)
		{
			if (scan_entries(infile, NULL, itemname) == -1)
			{
				found = 1;
			}
			else
			{
				fclose(infile);
				infile = NULL;
			}
		}

		if (!found && g_check_current_dir)
		{
			make_path(fullpath, "", DOTSLASH, fname, ext);
			infile = fopen(fullpath, "rb");
			if (infile != NULL)
			{
				if (scan_entries(infile, NULL, itemname) == -1)
				{
					strcpy(filename, fullpath);
					found = 1;
				}
				else
				{
					fclose(infile);
					infile = NULL;
				}
			}
		}
	}

	switch (itemtype)
	{
	case ITEMTYPE_FORMULA:
		strcpy(parsearchname, "frm:");
		strcat(parsearchname, itemname);
		parsearchname[ITEMNAMELEN + 5] = (char) 0; /*safety*/
		strcpy(defaultextension, ".frm");
		split_path(g_search_for.frm, drive, dir, NULL, NULL);
		break;
	case ITEMTYPE_L_SYSTEM:
		strcpy(parsearchname, "lsys:");
		strcat(parsearchname, itemname);
		parsearchname[ITEMNAMELEN + 5] = (char) 0; /*safety*/
		strcpy(defaultextension, ".l");
		split_path(g_search_for.lsys, drive, dir, NULL, NULL);
		break;
	case ITEMTYPE_IFS:
		strcpy(parsearchname, "ifs:");
		strcat(parsearchname, itemname);
		parsearchname[ITEMNAMELEN + 5] = (char) 0; /*safety*/
		strcpy(defaultextension, ".ifs");
		split_path(g_search_for.ifs, drive, dir, NULL, NULL);
		break;
	case ITEMTYPE_PARAMETER:
		strcpy(parsearchname, itemname);
		parsearchname[ITEMNAMELEN + 5] = (char) 0; /*safety*/
		strcpy(defaultextension, ".par");
		split_path(g_search_for.par, drive, dir, NULL, NULL);
		break;
	}

	if (!found)
	{
		infile = fopen(g_command_file, "rb");
		if (infile != NULL)
		{
			if (scan_entries(infile, NULL, parsearchname) == -1)
			{
				strcpy(filename, g_command_file);
				found = 1;
			}
			else
			{
				fclose(infile);
				infile = NULL;
			}
		}
	}

	if (!found)
	{
		make_path(fullpath, drive, dir, fname, ext);
		infile = fopen(fullpath, "rb");
		if (infile != NULL)
		{
			if (scan_entries(infile, NULL, itemname) == -1)
			{
				strcpy(filename, fullpath);
				found = 1;
			}
			else
			{
				fclose(infile);
				infile = NULL;
			}
		}
	}

	if (!found)  /* search for file */
	{
		int out;
		make_path(fullpath, drive, dir, "*", defaultextension);
		out = fr_find_first(fullpath);
		while (out == 0)
		{
			char msg[200];
			g_dta.filename[FILE_MAX_FNAME + FILE_MAX_EXT-2] = 0;
			sprintf(msg, "Searching %13s for %s      ", g_dta.filename, itemname);
			show_temp_message(msg);
			if (!(g_dta.attribute & SUBDIR) &&
				strcmp(g_dta.filename, ".")&&
				strcmp(g_dta.filename, ".."))
			{
				split_path(g_dta.filename, NULL, NULL, fname, ext);
				make_path(fullpath, drive, dir, fname, ext);
				infile = fopen(fullpath, "rb");
				if (infile != NULL)
				{
					if (scan_entries(infile, NULL, itemname) == -1)
					{
						strcpy(filename, fullpath);
						found = 1;
						break;
					}
					else
					{
						fclose(infile);
						infile = NULL;
					}
				}
			}
			out = fr_find_next();
		}
		clear_temp_message();
	}

	if (!found && g_organize_formula_search && itemtype == 1)
	{
		split_path(g_organize_formula_dir, drive, dir, NULL, NULL);
		fname[0] = '_';
		fname[1] = (char) 0;
		if (isalpha(itemname[0]))
		{
			if (strnicmp(itemname, "carr", 4))
			{
				fname[1] = itemname[0];
				fname[2] = (char) 0;
			}
			else if (isdigit(itemname[4]))
			{
				strcat(fname, "rc");
				fname[3] = itemname[4];
				fname[4] = (char) 0;
			}
			else
			{
				strcat(fname, "rc");
			}
		}
		else if (isdigit(itemname[0]))
		{
			strcat(fname, "num");
		}
		else
		{
			strcat(fname, "chr");
		}
		make_path(fullpath, drive, dir, fname, defaultextension);
		infile = fopen(fullpath, "rb");
		if (infile != NULL)
		{
			if (scan_entries(infile, NULL, itemname) == -1)
			{
				strcpy(filename, fullpath);
				found = 1;
			}
			else
			{
				fclose(infile);
				infile = NULL;
			}
		}
	}

	if (!found)
	{
		sprintf(fullpath, "'%s' file entry item not found", itemname);
		stop_message(0, fullpath);
		return -1;
	}
	/* found file */
	if (fileptr != NULL)
	{
		*fileptr = infile;
	}
	else if (infile != NULL)
	{
		fclose(infile);
	}
	return 0;
}


int file_gets(char *buf, int maxlen, FILE *infile)
{
	int len, c;
	/* similar to 'fgets', but file may be in either text or binary mode */
	/* returns -1 at eof, length of string otherwise */
	if (feof(infile))
	{
		return -1;
	}
	len = 0;
	while (len < maxlen)
	{
		c = getc(infile);
		if (c == EOF || c == '\032')
		{
			if (len)
			{
				break;
			}
			return -1;
			}
		if (c == '\n') /* linefeed is end of line  */
		{
			break;
		}
		if (c != '\r') /* ignore c/r  */
		{
			buf[len++] = (char)c;
		}
	}
	buf[len] = 0;
	return len;
}

int g_math_error_count = 0;

#if !defined(_WIN32)
#ifndef XFRACT
#ifdef WINFRACT
/* call this something else to dodge the QC4WIN bullet... */
int win_matherr(struct exception *except)
#else
int _cdecl _matherr(struct exception *except)
#endif
{
	if (g_debug_flag)
	{
		static FILE *fp = NULL;
		if (g_math_error_count++ == 0)
		{
			if (DEBUGFLAG_SHOW_MATH_ERRORS == g_debug_flag || DEBUGFLAG_NO_BIG_TO_FLOAT == g_debug_flag)
			{
				stop_message(0, "Math error, but we'll try to keep going");
			}
		}
		if (fp == NULL)
		{
			fp = fopen("matherr.txt", "wt");
		}
		if (g_math_error_count < 100)
		{
			fprintf(fp, "err #%d:  %d\nname: %s\narg:  %e\n",
					g_math_error_count, except->type, except->name, except->arg1);
			fflush(fp);
		}
		else
		{
			g_math_error_count = 100;
		}
	}
	if (except->type == DOMAIN)
	{
		char buf[40];
		sprintf(buf, "%e", except->arg1);
		/* This test may be unnecessary - from my experiments if the
			argument is too large or small the error is TLOSS not DOMAIN */
		if (strstr(buf, "IN") || strstr(buf, "NAN"))  /* trashed arg? */
									/* "IND" with MSC, "INF" with BC++ */
		{
			if (strcmp(except->name, "sin") == 0)
			{
				except->retval = 0.0;
				return 1;
			}
			else if (strcmp(except->name, "cos") == 0)
			{
				except->retval = 1.0;
				return 1;
			}
			else if (strcmp(except->name, "log") == 0)
			{
				except->retval = 1.0;
				return 1;
			}
		}
	}
	if (except->type == TLOSS)
	{
		/* try valiantly to keep going */
		if (strcmp(except->name, "sin") == 0)
		{
			except->retval = 0.5;
			return 1;
		}
		else if (strcmp(except->name, "cos") == 0)
		{
			except->retval = 0.5;
			return 1;
		}
	}
	/* shucks, no idea what went wrong, but our motto is "keep going!" */
	except->retval = 1.0;
	return 1;
}
#endif
#endif

void round_float_d(double *x) /* make double converted from float look ok */
{
	char buf[30];
	sprintf(buf, "%-10.7g", *x);
	*x = atof(buf);
}

void fix_inversion(double *x) /* make double converted from string look ok */
{
	char buf[30];
	sprintf(buf, "%-1.15lg", *x);
	*x = atof(buf);
}
