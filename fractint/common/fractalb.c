/* -----------------------------------------------------------------

This file contains the "big number" high precision versions of the
fractal routines.

--------------------------------------------------------------------   */


#include <limits.h>
#include <string.h>
#if !defined(__386BSD__)
#if !defined(_WIN32)
#include <malloc.h>
#endif
#endif
/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"


int bf_math = 0;

#ifdef DEBUG

/**********************************************************************/
void show_var_bn(char *s, bn_t n)
	{
		char msg[200];
		strcpy(msg, s);
		strcat(msg, " ");
		bntostr(msg + strlen(s), 40, n);
		msg[79] = 0;
		stopmsg(0, (char *)msg);
	}

void showcornersdbl(char *s)
{
	char msg[400];
	sprintf(msg, "%s\n"
		"g_xx_min= %.20f g_xx_max= %.20f\n"
		"g_yy_min= %.20f g_yy_max= %.20f\n"
		"g_xx_3rd= %.20f g_yy_3rd= %.20f\n"
		"delxx= %.20Lf delta_y_fp= %.20Lf\n"
		"g_delta_x2= %.20Lf g_delta_y2= %.20Lf",
		s, g_xx_min, g_xx_max, g_yy_min, g_yy_max, g_xx_3rd, g_yy_3rd,
		delxx, delta_y_fp, g_delta_x2_fp, g_delta_y2_fp);
	if (stopmsg(0, msg) == -1)
	{
		goodbye();
	}
}

/* show floating point and bignumber corners */
void showcorners(char *s)
{
	int dec = 20;
	char msg[100], msg1[100], msg3[100];
	bntostr(msg, dec, bnxmin);
	sprintf(msg1, "bnxmin=%s\nxxmin= %.20f\n\n", msg, g_xx_min);
	strcpy(msg3, s);
	strcat(msg3, "\n");
	strcat(msg3, msg1);
	bntostr(msg, dec, bnxmax);
	sprintf(msg1, "bnxmax=%s\nxxmax= %.20f\n\n", msg, g_xx_max);
	strcat(msg3, msg1);
	bntostr(msg, dec, bnymin);
	sprintf(msg1, "bnymin=%s\nyymin= %.20f\n\n", msg, g_yy_min);
	strcat(msg3, msg1);
	bntostr(msg, dec, bnymax);
	sprintf(msg1, "bnymax=%s\nyymax= %.20f\n\n", msg, g_yy_max);
	strcat(msg3, msg1);
	bntostr(msg, dec, bnx3rd);
	sprintf(msg1, "bnx3rd=%s\nxx3rd= %.20f\n\n", msg, g_xx_3rd);
	strcat(msg3, msg1);
	bntostr(msg, dec, bny3rd);
	sprintf(msg1, "bny3rd=%s\nyy3rd= %.20f\n\n", msg, g_yy_3rd);
	strcat(msg3, msg1);
	if (stopmsg(0, msg3) == -1)
	{
		goodbye();
	}
}

/* show globals */
void showbfglobals(char *s)
{
	char msg[300];
	sprintf(msg, "%s\n"
		"bnstep=%d bnlength=%d intlength=%d rlength=%d padding=%d\n"
		"shiftfactor=%d g_decimals=%d bflength=%d rbflength=%d \n"
		"bfdecimals=%d ",
		s, bnstep, bnlength, intlength, rlength, padding,
		shiftfactor, g_decimals, bflength, rbflength,
		bfdecimals);
	if (stopmsg(0, msg) == -1)
	{
		goodbye();
	}
}

void showcornersbf(char *s)
{
	int dec = g_decimals;
	char msg[100], msg1[100], msg3[600];
	if (dec > 20)
	{
		dec = 20;
	}
	bftostr(msg, dec, bfxmin);
	sprintf(msg1, "bfxmin=%s\nxxmin= %.20f g_decimals %d bflength %d\n\n",
		msg, g_xx_min, g_decimals, bflength);
	strcpy(msg3, s);
	strcat(msg3, "\n");
	strcat(msg3, msg1);
	bftostr(msg, dec, bfxmax);
	sprintf(msg1, "bfxmax=%s\nxxmax= %.20f\n\n", msg, g_xx_max);
	strcat(msg3, msg1);
	bftostr(msg, dec, bfymin);
	sprintf(msg1, "bfymin=%s\nyymin= %.20f\n\n", msg, g_yy_min);
	strcat(msg3, msg1);
	bftostr(msg, dec, bfymax);
	sprintf(msg1, "bfymax=%s\nyymax= %.20f\n\n", msg, g_yy_max);
	strcat(msg3, msg1);
	bftostr(msg, dec, bfx3rd);
	sprintf(msg1, "bfx3rd=%s\nxx3rd= %.20f\n\n", msg, g_xx_3rd);
	strcat(msg3, msg1);
	bftostr(msg, dec, bfy3rd);
	sprintf(msg1, "bfy3rd=%s\nyy3rd= %.20f\n\n", msg, g_yy_3rd);
	strcat(msg3, msg1);
	if (stopmsg(0, msg3) == -1)
	{
		goodbye();
	}
}

void showcornersbfs(char *s)
{
	int dec = 20;
	char msg[100], msg1[100], msg3[500];
	bftostr(msg, dec, bfsxmin);
	sprintf(msg1, "bfsxmin=%s\nxxmin= %.20f\n\n", msg, g_xx_min);
	strcpy(msg3, s);
	strcat(msg3, "\n");
	strcat(msg3, msg1);
	bftostr(msg, dec, bfsxmax);
	sprintf(msg1, "bfsxmax=%s\nxxmax= %.20f\n\n", msg, g_xx_max);
	strcat(msg3, msg1);
	bftostr(msg, dec, bfsymin);
	sprintf(msg1, "bfsymin=%s\nyymin= %.20f\n\n", msg, g_yy_min);
	strcat(msg3, msg1);
	bftostr(msg, dec, bfsymax);
	sprintf(msg1, "bfsymax=%s\nyymax= %.20f\n\n", msg, g_yy_max);
	strcat(msg3, msg1);
	bftostr(msg, dec, bfsx3rd);
	sprintf(msg1, "bfsx3rd=%s\nxx3rd= %.20f\n\n", msg, g_xx_3rd);
	strcat(msg3, msg1);
	bftostr(msg, dec, bfsy3rd);
	sprintf(msg1, "bfsy3rd=%s\nyy3rd= %.20f\n\n", msg, g_yy_3rd);
	strcat(msg3, msg1);
	if (stopmsg(0, msg3) == -1)
	{
		goodbye();
	}
}

void show_two_bf(char *s1, bf_t t1, char *s2, bf_t t2, int digits)
{
	char msg1[200], msg2[200], msg3[400];
	bftostr_e(msg1, digits, t1);
	bftostr_e(msg2, digits, t2);
	sprintf(msg3, "\n%s->%s\n%s->%s", s1, msg1, s2, msg2);
	if (stopmsg(0, msg3) == -1)
	{
		goodbye();
	}
}

void show_three_bf(char *s1, bf_t t1, char *s2, bf_t t2, char *s3, bf_t t3, int digits)
{
	char msg1[200], msg2[200], msg3[200], msg4[600];
	bftostr_e(msg1, digits, t1);
	bftostr_e(msg2, digits, t2);
	bftostr_e(msg3, digits, t3);
	sprintf(msg4, "\n%s->%s\n%s->%s\n%s->%s", s1, msg1, s2, msg2, s3, msg3);
	if (stopmsg(0, msg4) == -1)
	{
		goodbye();
	}
}

/* for aspect ratio debugging */
void showaspect(char *s)
{
	bf_t bt1, bt2, aspect;
	char msg[100], str[100];
	int saved; saved = save_stack();
	bt1    = alloc_stack(rbflength + 2);
	bt2    = alloc_stack(rbflength + 2);
	aspect = alloc_stack(rbflength + 2);
	sub_bf(bt1, bfxmax, bfxmin);
	sub_bf(bt2, bfymax, bfymin);
	div_bf(aspect, bt2, bt1);
	bftostr(str, 10, aspect);
	sprintf(msg, "aspect %s\nfloat %13.10f\nbf    %s\n\n",
				s,
				(g_yy_max-g_yy_min)/(g_xx_max-g_xx_min),
				str);
	if (stopmsg(0, msg) == -1)
	{
		goodbye();
	}
	restore_stack(saved);
}

/* compare a double and bignumber */
void comparevalues(char *s, LDBL x, bn_t bnx)
{
	int dec = 40;
	char msg[100], msg1[100];
	bntostr(msg, dec, bnx);
	sprintf(msg1, "%s\nbignum=%s\ndouble=%.20Lf\n\n", s, msg, x);
	if (stopmsg(0, msg1) == -1)
	{
		goodbye();
	}
}
/* compare a double and bignumber */
void comparevaluesbf(char *s, LDBL x, bf_t bfx)
{
	int dec = 40;
	char msg[300], msg1[300];
	bftostr_e(msg, dec, bfx);
	sprintf(msg1, "%s\nbignum=%s\ndouble=%.20Lf\n\n", s, msg, x);
	if (stopmsg(0, msg1) == -1)
	{
		goodbye();
	}
}

/**********************************************************************/
void show_var_bf(char *s, bf_t n)
	{
		char msg[200];
		strcpy(msg, s);
		strcat(msg, " ");
		bftostr_e(msg + strlen(s), 40, n);
		msg[79] = 0;
		if (stopmsg(0, msg) == -1)
		{
			goodbye();
		}
	}

#endif

void corners_bf_to_float(void)
{
	int i;
	if (bf_math)
	{
		g_xx_min = (double)bftofloat(bfxmin);
		g_yy_min = (double)bftofloat(bfymin);
		g_xx_max = (double)bftofloat(bfxmax);
		g_yy_max = (double)bftofloat(bfymax);
		g_xx_3rd = (double)bftofloat(bfx3rd);
		g_yy_3rd = (double)bftofloat(bfy3rd);
	}
	for (i = 0; i < MAXPARAMS; i++)
	{
		if (type_has_parameter(g_fractal_type, i, NULL))
		{
			g_parameters[i] = (double)bftofloat(bfparms[i]);
		}
	}
}

/* -------------------------------------------------------------------- */
/*    Bignumber Bailout Routines                                        */
/* -------------------------------------------------------------------- */

/* Note:                                             */
/* No need to set magnitude                          */
/* as color schemes that need it calculate it later. */
int  bail_out_mod_bn()
{
	long longmagnitude;

	square_bn(bntmpsqrx, bnnew.x);
	square_bn(bntmpsqry, bnnew.y);
	add_bn(bntmp, bntmpsqrx + shiftfactor, bntmpsqry + shiftfactor);

	longmagnitude = bntoint(bntmp);  /* works with any fractal type */
	if (longmagnitude >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bn(bnold.x, bnnew.x);
	copy_bn(bnold.y, bnnew.y);
	return 0;
}

int  bail_out_real_bn()
{
	long longtempsqrx;

	square_bn(bntmpsqrx, bnnew.x);
	square_bn(bntmpsqry, bnnew.y);
	longtempsqrx = bntoint(bntmpsqrx + shiftfactor);
	if (longtempsqrx >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bn(bnold.x, bnnew.x);
	copy_bn(bnold.y, bnnew.y);
	return 0;
}


int  bail_out_imag_bn()
{
	long longtempsqry;

	square_bn(bntmpsqrx, bnnew.x);
	square_bn(bntmpsqry, bnnew.y);
	longtempsqry = bntoint(bntmpsqry + shiftfactor);
	if (longtempsqry >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bn(bnold.x, bnnew.x);
	copy_bn(bnold.y, bnnew.y);
	return 0;
}

int  bail_out_or_bn()
{
	long longtempsqrx, longtempsqry;

	square_bn(bntmpsqrx, bnnew.x);
	square_bn(bntmpsqry, bnnew.y);
	longtempsqrx = bntoint(bntmpsqrx + shiftfactor);
	longtempsqry = bntoint(bntmpsqry + shiftfactor);
	if (longtempsqrx >= (long)g_rq_limit || longtempsqry >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bn(bnold.x, bnnew.x);
	copy_bn(bnold.y, bnnew.y);
	return 0;
}

int  bail_out_and_bn()
{
	long longtempsqrx, longtempsqry;

	square_bn(bntmpsqrx, bnnew.x);
	square_bn(bntmpsqry, bnnew.y);
	longtempsqrx = bntoint(bntmpsqrx + shiftfactor);
	longtempsqry = bntoint(bntmpsqry + shiftfactor);
	if (longtempsqrx >= (long)g_rq_limit && longtempsqry >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bn(bnold.x, bnnew.x);
	copy_bn(bnold.y, bnnew.y);
	return 0;
}

int  bail_out_manhattan_bn()
{
	long longtempmag;

	square_bn(bntmpsqrx, bnnew.x);
	square_bn(bntmpsqry, bnnew.y);
	/* note: in next five lines, bnold is just used as a temporary variable */
	abs_bn(bnold.x, bnnew.x);
	abs_bn(bnold.y, bnnew.y);
	add_bn(bntmp, bnold.x, bnold.y);
	square_bn(bnold.x, bntmp);
	longtempmag = bntoint(bnold.x + shiftfactor);
	if (longtempmag >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bn(bnold.x, bnnew.x);
	copy_bn(bnold.y, bnnew.y);
	return 0;
}

int  bail_out_manhattan_r_bn()
{
	long longtempmag;

	square_bn(bntmpsqrx, bnnew.x);
	square_bn(bntmpsqry, bnnew.y);
	add_bn(bntmp, bnnew.x, bnnew.y); /* don't need abs since we square it next */
	/* note: in next two lines, bnold is just used as a temporary variable */
	square_bn(bnold.x, bntmp);
	longtempmag = bntoint(bnold.x + shiftfactor);
	if (longtempmag >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bn(bnold.x, bnnew.x);
	copy_bn(bnold.y, bnnew.y);
	return 0;
}

int  bail_out_mod_bf()
{
	long longmagnitude;

	square_bf(bftmpsqrx, bfnew.x);
	square_bf(bftmpsqry, bfnew.y);
	add_bf(bftmp, bftmpsqrx, bftmpsqry);

	longmagnitude = bftoint(bftmp);
	if (longmagnitude >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bf(bfold.x, bfnew.x);
	copy_bf(bfold.y, bfnew.y);
	return 0;
}

int  bail_out_real_bf()
{
	long longtempsqrx;

	square_bf(bftmpsqrx, bfnew.x);
	square_bf(bftmpsqry, bfnew.y);
	longtempsqrx = bftoint(bftmpsqrx);
	if (longtempsqrx >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bf(bfold.x, bfnew.x);
	copy_bf(bfold.y, bfnew.y);
	return 0;
}


int  bail_out_imag_bf()
{
	long longtempsqry;

	square_bf(bftmpsqrx, bfnew.x);
	square_bf(bftmpsqry, bfnew.y);
	longtempsqry = bftoint(bftmpsqry);
	if (longtempsqry >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bf(bfold.x, bfnew.x);
	copy_bf(bfold.y, bfnew.y);
	return 0;
}

int  bail_out_or_bf()
{
	long longtempsqrx, longtempsqry;

	square_bf(bftmpsqrx, bfnew.x);
	square_bf(bftmpsqry, bfnew.y);
	longtempsqrx = bftoint(bftmpsqrx);
	longtempsqry = bftoint(bftmpsqry);
	if (longtempsqrx >= (long)g_rq_limit || longtempsqry >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bf(bfold.x, bfnew.x);
	copy_bf(bfold.y, bfnew.y);
	return 0;
}

int  bail_out_and_bf()
{
	long longtempsqrx, longtempsqry;

	square_bf(bftmpsqrx, bfnew.x);
	square_bf(bftmpsqry, bfnew.y);
	longtempsqrx = bftoint(bftmpsqrx);
	longtempsqry = bftoint(bftmpsqry);
	if (longtempsqrx >= (long)g_rq_limit && longtempsqry >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bf(bfold.x, bfnew.x);
	copy_bf(bfold.y, bfnew.y);
	return 0;
}

int  bail_out_manhattan_bf()
{
	long longtempmag;

	square_bf(bftmpsqrx, bfnew.x);
	square_bf(bftmpsqry, bfnew.y);
	/* note: in next five lines, bfold is just used as a temporary variable */
	abs_bf(bfold.x, bfnew.x);
	abs_bf(bfold.y, bfnew.y);
	add_bf(bftmp, bfold.x, bfold.y);
	square_bf(bfold.x, bftmp);
	longtempmag = bftoint(bfold.x);
	if (longtempmag >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bf(bfold.x, bfnew.x);
	copy_bf(bfold.y, bfnew.y);
	return 0;
}

int  bail_out_manhattan_r_bf()
{
	long longtempmag;

	square_bf(bftmpsqrx, bfnew.x);
	square_bf(bftmpsqry, bfnew.y);
	add_bf(bftmp, bfnew.x, bfnew.y); /* don't need abs since we square it next */
	/* note: in next two lines, bfold is just used as a temporary variable */
	square_bf(bfold.x, bftmp);
	longtempmag = bftoint(bfold.x);
	if (longtempmag >= (long)g_rq_limit)
	{
		return 1;
	}
	copy_bf(bfold.x, bfnew.x);
	copy_bf(bfold.y, bfnew.y);
	return 0;
}

int mandelbrot_setup_bn()
{
	/* this should be set up dynamically based on corners */
	bn_t bntemp1, bntemp2;
	int saved; saved = save_stack();
	bntemp1 = alloc_stack(bnlength);
	bntemp2 = alloc_stack(bnlength);

	bftobn(bnxmin, bfxmin);
	bftobn(bnxmax, bfxmax);
	bftobn(bnymin, bfymin);
	bftobn(bnymax, bfymax);
	bftobn(bnx3rd, bfx3rd);
	bftobn(bny3rd, bfy3rd);

	bf_math = BIGNUM;

	/* bnxdel = (bnxmax - bnx3rd)/(g_x_dots-1) */
	sub_bn(bnxdel, bnxmax, bnx3rd);
	div_a_bn_int(bnxdel, (U16)(g_x_dots - 1));

	/* bnydel = (bnymax - bny3rd)/(g_y_dots-1) */
	sub_bn(bnydel, bnymax, bny3rd);
	div_a_bn_int(bnydel, (U16)(g_y_dots - 1));

	/* bnxdel2 = (bnx3rd - bnxmin)/(g_y_dots-1) */
	sub_bn(bnxdel2, bnx3rd, bnxmin);
	div_a_bn_int(bnxdel2, (U16)(g_y_dots - 1));

	/* bnydel2 = (bny3rd - bnymin)/(g_x_dots-1) */
	sub_bn(bnydel2, bny3rd, bnymin);
	div_a_bn_int(bnydel2, (U16)(g_x_dots - 1));

	abs_bn(bnclosenuff, bnxdel);
	if (cmp_bn(abs_bn(bntemp1, bnxdel2), bnclosenuff) > 0)
	{
		copy_bn(bnclosenuff, bntemp1);
	}
	if (cmp_bn(abs_bn(bntemp1, bnydel), abs_bn(bntemp2, bnydel2)) > 0)
	{
		if (cmp_bn(bntemp1, bnclosenuff) > 0)
		{
			copy_bn(bnclosenuff, bntemp1);
		}
	}
	else if (cmp_bn(bntemp2, bnclosenuff) > 0)
	{
		copy_bn(bnclosenuff, bntemp2);
	}
	{
		int t;
		t = abs(g_periodicity_check);
		while (t--)
		{
			half_a_bn(bnclosenuff);
		}
	}

	g_c_exp = (int)g_parameters[2];
	switch (g_fractal_type)
	{
	case JULIAFP:
		bftobn(bnparm.x, bfparms[0]);
		bftobn(bnparm.y, bfparms[1]);
		break;
	case FPMANDELZPOWER:
		init_big_pi();
		if ((double)g_c_exp == g_parameters[2] && (g_c_exp & 1)) /* odd exponents */
		{
			g_symmetry = XYAXIS_NOPARM;
		}
		if (g_parameters[3] != 0)
		{
			g_symmetry = NOSYM;
		}
		break;
	case FPJULIAZPOWER:
		init_big_pi();
		bftobn(bnparm.x, bfparms[0]);
		bftobn(bnparm.y, bfparms[1]);
		if ((g_c_exp & 1) || g_parameters[3] != 0.0 || (double)g_c_exp != g_parameters[2])
		{
			g_symmetry = NOSYM;
		}
		break;
	}

	/* at the present time, parameters are kept in float, but want to keep
		the arbitrary precision logic intact. The next two lines, if used,
		would disguise and breaking of the arbitrary precision logic */
	/*
	floattobn(bnparm.x, g_parameters[0]);
	floattobn(bnparm.y, g_parameters[1]);
	*/
	restore_stack(saved);
	return 1;
}

int mandelbrot_setup_bf()
{
	/* this should be set up dynamically based on corners */
	bf_t bftemp1, bftemp2;
	int saved; saved = save_stack();
	bftemp1 = alloc_stack(bflength + 2);
	bftemp2 = alloc_stack(bflength + 2);

	bf_math = BIGFLT;

	/* bfxdel = (bfxmax - bfx3rd)/(g_x_dots-1) */
	sub_bf(bfxdel, bfxmax, bfx3rd);
	div_a_bf_int(bfxdel, (U16)(g_x_dots - 1));

	/* bfydel = (bfymax - bfy3rd)/(g_y_dots-1) */
	sub_bf(bfydel, bfymax, bfy3rd);
	div_a_bf_int(bfydel, (U16)(g_y_dots - 1));

	/* bfxdel2 = (bfx3rd - bfxmin)/(g_y_dots-1) */
	sub_bf(bfxdel2, bfx3rd, bfxmin);
	div_a_bf_int(bfxdel2, (U16)(g_y_dots - 1));

	/* bfydel2 = (bfy3rd - bfymin)/(g_x_dots-1) */
	sub_bf(bfydel2, bfy3rd, bfymin);
	div_a_bf_int(bfydel2, (U16)(g_x_dots - 1));

	abs_bf(bfclosenuff, bfxdel);
	if (cmp_bf(abs_bf(bftemp1, bfxdel2), bfclosenuff) > 0)
	{
		copy_bf(bfclosenuff, bftemp1);
	}
	if (cmp_bf(abs_bf(bftemp1, bfydel), abs_bf(bftemp2, bfydel2)) > 0)
	{
		if (cmp_bf(bftemp1, bfclosenuff) > 0)
		{
			copy_bf(bfclosenuff, bftemp1);
		}
	}
	else if (cmp_bf(bftemp2, bfclosenuff) > 0)
	{
		copy_bf(bfclosenuff, bftemp2);
	}
	{
		int t;
		t = abs(g_periodicity_check);
		while (t--)
		{
			half_a_bf(bfclosenuff);
		}
	}

	g_c_exp = (int)g_parameters[2];
	switch (g_fractal_type)
	{
	case JULIAFP:
		copy_bf(bfparm.x, bfparms[0]);
		copy_bf(bfparm.y, bfparms[1]);
		break;
	case FPMANDELZPOWER:
		init_big_pi();
		if ((double)g_c_exp == g_parameters[2] && (g_c_exp & 1)) /* odd exponents */
		{
			g_symmetry = XYAXIS_NOPARM;
		}
		if (g_parameters[3] != 0)
		{
			g_symmetry = NOSYM;
		}
		break;
	case FPJULIAZPOWER:
		init_big_pi();
		copy_bf(bfparm.x, bfparms[0]);
		copy_bf(bfparm.y, bfparms[1]);
		if ((g_c_exp & 1) || g_parameters[3] != 0.0 || (double)g_c_exp != g_parameters[2])
		{
			g_symmetry = NOSYM;
		}
		break;
	}

	restore_stack(saved);
	return 1;
}

int mandelbrot_per_pixel_bn()
{
	/* g_parameter.x = g_xx_min + col*delx + row*g_delta_x2 */
	mult_bn_int(bnparm.x, bnxdel, (U16)g_col);
	mult_bn_int(bntmp, bnxdel2, (U16)g_row);

	add_a_bn(bnparm.x, bntmp);
	add_a_bn(bnparm.x, bnxmin);

	/* g_parameter.y = g_yy_max - row*dely - col*g_delta_y2; */
	/* note: in next four lines, bnold is just used as a temporary variable */
	mult_bn_int(bnold.x, bnydel,  (U16)g_row);
	mult_bn_int(bnold.y, bnydel2, (U16)g_col);
	add_a_bn(bnold.x, bnold.y);
	sub_bn(bnparm.y, bnymax, bnold.x);

	copy_bn(bnold.x, bnparm.x);
	copy_bn(bnold.y, bnparm.y);

	if ((g_inside == BOF60 || g_inside == BOF61) && !g_no_bof)
	{
		/* kludge to match "Beauty of Fractals" picture since we start
			Mandelbrot iteration with init rather than 0 */
		floattobn(bnold.x, g_parameters[0]); /* initial pertubation of parameters set */
		floattobn(bnold.y, g_parameters[1]);
		g_color_iter = -1;
	}
	else
	{
		floattobn(bnnew.x, g_parameters[0]);
		floattobn(bnnew.y, g_parameters[1]);
		add_a_bn(bnold.x, bnnew.x);
		add_a_bn(bnold.y, bnnew.y);
	}

	/* square has side effect - must copy first */
	copy_bn(bnnew.x, bnold.x);
	copy_bn(bnnew.y, bnold.y);

	/* Square these to rlength bytes of precision */
	square_bn(bntmpsqrx, bnnew.x);
	square_bn(bntmpsqry, bnnew.y);

	return 1;                  /* 1st iteration has been done */
}

int mandelbrot_per_pixel_bf()
{
	/* g_parameter.x = g_xx_min + col*delx + row*g_delta_x2 */
	mult_bf_int(bfparm.x, bfxdel, (U16)g_col);
	mult_bf_int(bftmp, bfxdel2, (U16)g_row);

	add_a_bf(bfparm.x, bftmp);
	add_a_bf(bfparm.x, bfxmin);

	/* g_parameter.y = g_yy_max - row*dely - col*g_delta_y2; */
	/* note: in next four lines, bfold is just used as a temporary variable */
	mult_bf_int(bfold.x, bfydel,  (U16)g_row);
	mult_bf_int(bfold.y, bfydel2, (U16)g_col);
	add_a_bf(bfold.x, bfold.y);
	sub_bf(bfparm.y, bfymax, bfold.x);

	copy_bf(bfold.x, bfparm.x);
	copy_bf(bfold.y, bfparm.y);

	if ((g_inside == BOF60 || g_inside == BOF61) && !g_no_bof)
	{
		/* kludge to match "Beauty of Fractals" picture since we start
			Mandelbrot iteration with g_initial_z rather than 0 */
		floattobf(bfold.x, g_parameters[0]); /* initial pertubation of parameters set */
		floattobf(bfold.y, g_parameters[1]);
		g_color_iter = -1;
	}
	else
	{
		floattobf(bfnew.x, g_parameters[0]);
		floattobf(bfnew.y, g_parameters[1]);
		add_a_bf(bfold.x, bfnew.x);
		add_a_bf(bfold.y, bfnew.y);
	}

	/* square has side effect - must copy first */
	copy_bf(bfnew.x, bfold.x);
	copy_bf(bfnew.y, bfold.y);

	/* Square these to rbflength bytes of precision */
	square_bf(bftmpsqrx, bfnew.x);
	square_bf(bftmpsqry, bfnew.y);

	return 1;                  /* 1st iteration has been done */
}

int julia_per_pixel_bn(void)
{
	/* old.x = g_xx_min + col*delx + row*g_delta_x2 */
	mult_bn_int(bnold.x, bnxdel, (U16)g_col);
	mult_bn_int(bntmp, bnxdel2, (U16)g_row);

	add_a_bn(bnold.x, bntmp);
	add_a_bn(bnold.x, bnxmin);

	/* old.y = g_yy_max - row*dely - col*g_delta_y2; */
	/* note: in next four lines, bnnew is just used as a temporary variable */
	mult_bn_int(bnnew.x, bnydel,  (U16)g_row);
	mult_bn_int(bnnew.y, bnydel2, (U16)g_col);
	add_a_bn(bnnew.x, bnnew.y);
	sub_bn(bnold.y, bnymax, bnnew.x);

	/* square has side effect - must copy first */
	copy_bn(bnnew.x, bnold.x);
	copy_bn(bnnew.y, bnold.y);

	/* Square these to rlength bytes of precision */
	square_bn(bntmpsqrx, bnnew.x);
	square_bn(bntmpsqry, bnnew.y);

	return 1;                  /* 1st iteration has been done */
}

int julia_per_pixel_bf(void)
{
	/* old.x = g_xx_min + col*delx + row*g_delta_x2 */
	mult_bf_int(bfold.x, bfxdel, (U16)g_col);
	mult_bf_int(bftmp, bfxdel2, (U16)g_row);

	add_a_bf(bfold.x, bftmp);
	add_a_bf(bfold.x, bfxmin);

	/* old.y = g_yy_max - row*dely - col*g_delta_y2; */
	/* note: in next four lines, bfnew is just used as a temporary variable */
	mult_bf_int(bfnew.x, bfydel,  (U16)g_row);
	mult_bf_int(bfnew.y, bfydel2, (U16)g_col);
	add_a_bf(bfnew.x, bfnew.y);
	sub_bf(bfold.y, bfymax, bfnew.x);

	/* square has side effect - must copy first */
	copy_bf(bfnew.x, bfold.x);
	copy_bf(bfnew.y, bfold.y);

	/* Square these to rbflength bytes of precision */
	square_bf(bftmpsqrx, bfnew.x);
	square_bf(bftmpsqry, bfnew.y);

	return 1;                  /* 1st iteration has been done */
}

int julia_orbit_bn()
{
	/* Don't forget, with bn_t numbers, after multiplying or squaring */
	/* you must shift over by shiftfactor to get the bn number.          */

	/* bntmpsqrx and bntmpsqry were previously squared before getting to */
	/* this function, so they must be shifted.                           */

	/* new.x = tmpsqrx - tmpsqry + g_parameter.x;   */
	sub_a_bn(bntmpsqrx + shiftfactor, bntmpsqry + shiftfactor);
	add_bn(bnnew.x, bntmpsqrx + shiftfactor, bnparm.x);

	/* new.y = 2*bnold.x*bnold.y + g_parameter.y; */
	mult_bn(bntmp, bnold.x, bnold.y); /* ok to use unsafe here */
	double_a_bn(bntmp + shiftfactor);
	add_bn(bnnew.y, bntmp + shiftfactor, bnparm.y);

	return g_bail_out_bn();
}

int julia_orbit_bf()
{
	/* new.x = tmpsqrx - tmpsqry + g_parameter.x;   */
	sub_a_bf(bftmpsqrx, bftmpsqry);
	add_bf(bfnew.x, bftmpsqrx, bfparm.x);

	/* new.y = 2*bfold.x*bfold.y + g_parameter.y; */
	mult_bf(bftmp, bfold.x, bfold.y); /* ok to use unsafe here */
	double_a_bf(bftmp);
	add_bf(bfnew.y, bftmp, bfparm.y);
	return g_bail_out_bf();
}

int julia_z_power_orbit_bn()
{
	_BNCMPLX parm2;
	int saved; saved = save_stack();

	parm2.x = alloc_stack(bnlength);
	parm2.y = alloc_stack(bnlength);

	floattobn(parm2.x, g_parameters[2]);
	floattobn(parm2.y, g_parameters[3]);
	complex_power_bn(&bnnew, &bnold, &parm2);
	add_bn(bnnew.x, bnparm.x, bnnew.x + shiftfactor);
	add_bn(bnnew.y, bnparm.y, bnnew.y + shiftfactor);
	restore_stack(saved);
	return g_bail_out_bn();
}

int julia_z_power_orbit_bf()
{
	_BFCMPLX parm2;
	int saved; saved = save_stack();

	parm2.x = alloc_stack(bflength + 2);
	parm2.y = alloc_stack(bflength + 2);

	floattobf(parm2.x, g_parameters[2]);
	floattobf(parm2.y, g_parameters[3]);
	ComplexPower_bf(&bfnew, &bfold, &parm2);
	add_bf(bfnew.x, bfparm.x, bfnew.x);
	add_bf(bfnew.y, bfparm.y, bfnew.y);
	restore_stack(saved);
	return g_bail_out_bf();
}


#if 0
/*
the following is an example of how you can take advantage of the bn_t
format to squeeze a little more precision out of the calculations.
*/
int
julia_orbit_bn()
{
	int oldbnlength;
	bn_t mod;
	/* using partial precision multiplications */

	/* bnnew.x = bntmpsqrx - bntmpsqry + bnparm.x;   */
	/*
	* Since tmpsqrx and tmpsqry where just calculated to rlength bytes of
	* precision, we might as well keep that extra precision in this next
	* subtraction.  Therefore, use rlength as the length.
	*/

	oldbnlength = bnlength;
	bnlength = rlength; sub_a_bn(bntmpsqrx, bntmpsqry); bnlength = oldbnlength;

	/*
	* Now that bntmpsqry has been sutracted from bntmpsqrx, we need to treat
	* tmpsqrx as a single width bignumber, so shift to bntmpsqrx + shiftfactor.
	*/
	add_bn(bnnew.x, bntmpsqrx + shiftfactor, bnparm.x);

	/* new.y = 2*bnold.x*bnold.y + old.y; */
	/* Multiply bnold.x*bnold.y to rlength precision. */
	mult_bn(bntmp, bnold.x, bnold.y);

	/*
	* Double bnold.x*bnold.y by shifting bits, including one of those bits
	* calculated in the previous mult_bn().  Therefore, use rlength.
	*/
	bnlength = rlength; double_a_bn(bntmp); bnlength = oldbnlength;

	/* Convert back to a single width bignumber and add bnparm.y */
	add_bn(bnnew.y, bntmp + shiftfactor, bnparm.y);

	copy_bn(bnold.x, bnnew.x);
	copy_bn(bnold.y, bnnew.y);

	/* Square these to rlength bytes of precision */
	square_bn(bntmpsqrx, bnold.x);
	square_bn(bntmpsqry, bnold.y);

	/* And add the full rlength precision to get those extra bytes */
	bnlength = rlength;
	add_bn(bntmp, bntmpsqrx, bntmpsqry);
	bnlength = oldbnlength;

	mod = bntmp + (rlength) - (intlength << 1);  /* where int part starts
													* after mult */
	/*
	* equivalent to, but faster than, mod = bn_int(tmp + shiftfactor);
	*/

	magnitude = *mod;
	if (magnitude >= g_rq_limit)
	{
		return 1;
	}
	return 0;
}
#endif

_CMPLX complex_bn_to_float(_BNCMPLX *s)
{
	_CMPLX t;
	t.x = (double)bntofloat(s->x);
	t.y = (double)bntofloat(s->y);
	return t;
}

_CMPLX complex_bf_to_float(_BFCMPLX *s)
{
	_CMPLX t;
	t.x = (double)bftofloat(s->x);
	t.y = (double)bftofloat(s->y);
	return t;
}

_BFCMPLX *complex_log_bf(_BFCMPLX *t, _BFCMPLX *s)
{
	square_bf(t->x, s->x);
	square_bf(t->y, s->y);
	add_a_bf(t->x, t->y);
	ln_bf(t->x, t->x);
	half_a_bf(t->x);
	atan2_bf(t->y, s->y, s->x);
	return t;
}

_BFCMPLX *cplxmul_bf(_BFCMPLX *t, _BFCMPLX *x, _BFCMPLX *y)
{
	bf_t tmp1;
	int saved; saved = save_stack();
	tmp1 = alloc_stack(rbflength + 2);
	mult_bf(t->x, x->x, y->x);
	mult_bf(t->y, x->y, y->y);
	sub_bf(t->x, t->x, t->y);

	mult_bf(tmp1, x->x, y->y);
	mult_bf(t->y, x->y, y->x);
	add_bf(t->y, tmp1, t->y);
	restore_stack(saved);
	return t;
}

_BFCMPLX *ComplexPower_bf(_BFCMPLX *t, _BFCMPLX *xx, _BFCMPLX *yy)
{
	_BFCMPLX tmp;
	bf_t e2x, siny, cosy;
	int saved; saved = save_stack();
	e2x  = alloc_stack(rbflength + 2);
	siny = alloc_stack(rbflength + 2);
	cosy = alloc_stack(rbflength + 2);
	tmp.x = alloc_stack(rbflength + 2);
	tmp.y = alloc_stack(rbflength + 2);

	/* 0 raised to anything is 0 */
	if (is_bf_zero(xx->x) && is_bf_zero(xx->y))
		{
		clear_bf(t->x);
		clear_bf(t->y);
		return t;
		}

	complex_log_bf(t, xx);
	cplxmul_bf(&tmp, t, yy);
	exp_bf(e2x, tmp.x);
	sincos_bf(siny, cosy, tmp.y);
	mult_bf(t->x, e2x, cosy);
	mult_bf(t->y, e2x, siny);
	restore_stack(saved);
	return t;
}

_BNCMPLX *complex_log_bn(_BNCMPLX *t, _BNCMPLX *s)
{
	square_bn(t->x, s->x);
	square_bn(t->y, s->y);
	add_a_bn(t->x + shiftfactor, t->y + shiftfactor);
	ln_bn(t->x, t->x + shiftfactor);
	half_a_bn(t->x);
	atan2_bn(t->y, s->y, s->x);
	return t;
}

_BNCMPLX *complex_multiply_bn(_BNCMPLX *t, _BNCMPLX *x, _BNCMPLX *y)
{
	bn_t tmp1;
	int saved; saved = save_stack();
	tmp1 = alloc_stack(rlength);
	mult_bn(t->x, x->x, y->x);
	mult_bn(t->y, x->y, y->y);
	sub_bn(t->x, t->x + shiftfactor, t->y + shiftfactor);

	mult_bn(tmp1, x->x, y->y);
	mult_bn(t->y, x->y, y->x);
	add_bn(t->y, tmp1 + shiftfactor, t->y + shiftfactor);
	restore_stack(saved);
	return t;
}

/* note: complex_power_bn() returns need to be +shiftfactor'ed */
_BNCMPLX *complex_power_bn(_BNCMPLX *t, _BNCMPLX *xx, _BNCMPLX *yy)
{
	_BNCMPLX tmp;
	bn_t e2x, siny, cosy;
	int saved; saved = save_stack();
	e2x  = alloc_stack(bnlength);
	siny = alloc_stack(bnlength);
	cosy = alloc_stack(bnlength);
	tmp.x = alloc_stack(rlength);
	tmp.y = alloc_stack(rlength);

	/* 0 raised to anything is 0 */
	if (is_bn_zero(xx->x) && is_bn_zero(xx->y))
		{
		clear_bn(t->x);
		clear_bn(t->y);
		return t;
		}

	complex_log_bn(t, xx);
	complex_multiply_bn(&tmp, t, yy);
	exp_bn(e2x, tmp.x);
	sincos_bn(siny, cosy, tmp.y);
	mult_bn(t->x, e2x, cosy);
	mult_bn(t->y, e2x, siny);
	restore_stack(saved);
	return t;
}
