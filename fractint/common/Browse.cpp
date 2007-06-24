#include <string.h>
#include <time.h>
#include <errno.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#include "Browse.h"
#include "drivers.h"
#include "EscapeTime.h"
#include "fihelp.h"
#include "filesystem.h"
#include "Formula.h"
#include "framain2.h"
#include "fracsubr.h"
#include "loadfile.h"
#include "lorenz.h"
#include "miscres.h"
#include "prompts2.h"
#include "realdos.h"
#include "zoom.h"

#define MAX_WINDOWS_OPEN 450

BrowseState g_browse_state;

struct window  /* for look_get_window on screen browser */
{
	Coordinate itl; /* screen coordinates */
	Coordinate ibl;
	Coordinate itr;
	Coordinate ibr;
	double win_size;   /* box size for drawindow() */
	char name[FILE_MAX_FNAME];     /* for filename */
	int box_count;      /* bytes of saved screen info */
};

static int oldbf_math;
/* here because must be visible inside several routines */
static bf_t bt_a;
static bf_t bt_b;
static bf_t bt_c;
static bf_t bt_d;
static bf_t bt_e;
static bf_t bt_f;
static bf_t n_a;
static bf_t n_b;
static bf_t n_c;
static bf_t n_d;
static bf_t n_e;
static bf_t n_f;
static struct affine *cvt;
static struct window browse_windows[MAX_WINDOWS_OPEN] = { 0 };
static int *boxx_storage = NULL;
static int *boxy_storage = NULL;
static int *boxvalues_storage = NULL;

/* prototypes */
static void check_history(const char *, const char *);
static void transform(CoordinateD *);
static void transform_bf(bf_t, bf_t, CoordinateD *);
static void drawindow(int color, struct window *info);
static bool is_visible_window(struct window *, fractal_info *, struct ext_blk_mp_info *);
static void bfsetup_convert_to_screen();
static char typeOK(fractal_info *, struct ext_blk_formula_info *);
static char functionOK(fractal_info *, int);
static char paramsOK(fractal_info *);

void BrowseState::extract_read_name()
{
	::extract_filename(m_name, g_read_name);
}

void BrowseState::make_path(const char *fname, const char *ext)
{
	::make_path(m_name, "", "", fname, ext);
}

void BrowseState::merge_path_names(char *read_name)
{
	::merge_path_names(read_name, m_name, 2);
}


/* maps points onto view screen*/
static void transform(CoordinateD *point)
{
	double tmp_pt_x = cvt->a*point->x + cvt->b*point->y + cvt->e;
	point->y = cvt->c*point->x + cvt->d*point->y + cvt->f;
	point->x = tmp_pt_x;
}

/* maps points onto view screen*/
static void transform_bf(bf_t bt_x, bf_t bt_y, CoordinateD *point)
{
	int saved = save_stack();
	bf_t bt_tmp1 = alloc_stack(rbflength + 2);
	bf_t bt_tmp2 = alloc_stack(rbflength + 2);

	/*  point->x = cvt->a*point->x + cvt->b*point->y + cvt->e; */
	mult_bf(bt_tmp1, n_a, bt_x);
	mult_bf(bt_tmp2, n_b, bt_y);
	add_a_bf(bt_tmp1, bt_tmp2);
	add_a_bf(bt_tmp1, n_e);
	point->x = (double) bftofloat(bt_tmp1);

	/*  point->y = cvt->c*point->x + cvt->d*point->y + cvt->f; */
	mult_bf(bt_tmp1, n_c, bt_x);
	mult_bf(bt_tmp2, n_d, bt_y);
	add_a_bf(bt_tmp1, bt_tmp2);
	add_a_bf(bt_tmp1, n_f);
	point->y = (double) bftofloat(bt_tmp1);

	restore_stack(saved);
}

static void is_visible_window_corner(const fractal_info &info,
	bf_t bt_x, bf_t bt_y,
	bf_t bt_xmin, bf_t bt_xmax,
	bf_t bt_ymin, bf_t bt_ymax,
	bf_t bt_x3rd, bf_t bt_y3rd,
	CoordinateD &corner)
{
	if (oldbf_math || info.bf_math)
	{
		if (!info.bf_math)
		{
			floattobf(bt_x, (info.x_max)-(info.x_3rd-info.x_min));
			floattobf(bt_y, (info.y_max) + (info.y_min-info.y_3rd));
		}
		else
		{
			neg_a_bf(sub_bf(bt_x, bt_x3rd, bt_xmin));
			add_a_bf(bt_x, bt_xmax);
			sub_bf(bt_y, bt_ymin, bt_y3rd);
			add_a_bf(bt_y, bt_ymax);
		}
		transform_bf(bt_x, bt_y, &corner);
	}
	else
	{
		corner.x = (info.x_max)-(info.x_3rd-info.x_min);
		corner.y = (info.y_max) + (info.y_min-info.y_3rd);
		transform(&corner);
	}
}

static bool is_visible_window(struct window *list, fractal_info *info,
	struct ext_blk_mp_info *mp_info)
{
	double toobig = sqrt(sqr((double)g_screen_width) + sqr((double)g_screen_height))*1.5;
	int saved = save_stack();

	/* Save original values. */
	int orig_bflength = bflength;
	int orig_bnlength = bnlength;
	int orig_padding = padding;
	int orig_rlength = rlength;
	int orig_shiftfactor = shiftfactor;
	int orig_rbflength = rbflength;

	int two_len = bflength + 2;
	bf_t bt_x = alloc_stack(two_len);
	bf_t bt_y = alloc_stack(two_len);
	bf_t bt_xmin = alloc_stack(two_len);
	bf_t bt_xmax = alloc_stack(two_len);
	bf_t bt_ymin = alloc_stack(two_len);
	bf_t bt_ymax = alloc_stack(two_len);
	bf_t bt_x3rd = alloc_stack(two_len);
	bf_t bt_y3rd = alloc_stack(two_len);

	if (info->bf_math)
	{
		int di_bflength = info->bflength + bnstep;
		int two_di_len = di_bflength + 2;
		int two_rbf = rbflength + 2;

		n_a = alloc_stack(two_rbf);
		n_b = alloc_stack(two_rbf);
		n_c = alloc_stack(two_rbf);
		n_d = alloc_stack(two_rbf);
		n_e = alloc_stack(two_rbf);
		n_f = alloc_stack(two_rbf);

		convert_bf(n_a, bt_a, rbflength, orig_rbflength);
		convert_bf(n_b, bt_b, rbflength, orig_rbflength);
		convert_bf(n_c, bt_c, rbflength, orig_rbflength);
		convert_bf(n_d, bt_d, rbflength, orig_rbflength);
		convert_bf(n_e, bt_e, rbflength, orig_rbflength);
		convert_bf(n_f, bt_f, rbflength, orig_rbflength);

		bf_t bt_t1 = alloc_stack(two_di_len);
		bf_t bt_t2 = alloc_stack(two_di_len);
		bf_t bt_t3 = alloc_stack(two_di_len);
		bf_t bt_t4 = alloc_stack(two_di_len);
		bf_t bt_t5 = alloc_stack(two_di_len);
		bf_t bt_t6 = alloc_stack(two_di_len);

		memcpy((char *)bt_t1, mp_info->apm_data, (two_di_len));
		memcpy((char *)bt_t2, mp_info->apm_data + two_di_len, (two_di_len));
		memcpy((char *)bt_t3, mp_info->apm_data + 2*two_di_len, (two_di_len));
		memcpy((char *)bt_t4, mp_info->apm_data + 3*two_di_len, (two_di_len));
		memcpy((char *)bt_t5, mp_info->apm_data + 4*two_di_len, (two_di_len));
		memcpy((char *)bt_t6, mp_info->apm_data + 5*two_di_len, (two_di_len));

		convert_bf(bt_xmin, bt_t1, two_len, two_di_len);
		convert_bf(bt_xmax, bt_t2, two_len, two_di_len);
		convert_bf(bt_ymin, bt_t3, two_len, two_di_len);
		convert_bf(bt_ymax, bt_t4, two_len, two_di_len);
		convert_bf(bt_x3rd, bt_t5, two_len, two_di_len);
		convert_bf(bt_y3rd, bt_t6, two_len, two_di_len);
	}

	/* tranform maps real plane co-ords onto the current screen view
		see above */
	CoordinateD tl;
	is_visible_window_corner(*info, bt_x, bt_y, bt_xmin, bt_xmax, bt_ymin, bt_ymax, bt_x3rd, bt_y3rd, tl);
	list->itl.x = (int)(tl.x + 0.5);
	list->itl.y = (int)(tl.y + 0.5);

	CoordinateD tr;
	is_visible_window_corner(*info, bt_x, bt_y, bt_xmin, bt_xmax, bt_ymin, bt_ymax, bt_x3rd, bt_y3rd, tr);
	list->itr.x = (int)(tr.x + 0.5);
	list->itr.y = (int)(tr.y + 0.5);

	CoordinateD bl;
	is_visible_window_corner(*info, bt_x, bt_y, bt_xmin, bt_xmax, bt_ymin, bt_ymax, bt_x3rd, bt_y3rd, bl);
	list->ibl.x = (int)(bl.x + 0.5);
	list->ibl.y = (int)(bl.y + 0.5);

	CoordinateD br;
	is_visible_window_corner(*info, bt_x, bt_y, bt_xmin, bt_xmax, bt_ymin, bt_ymax, bt_x3rd, bt_y3rd, br);
	list->ibr.x = (int)(br.x + 0.5);
	list->ibr.y = (int)(br.y + 0.5);

	double tmp_sqrt = sqrt(sqr(tr.x-bl.x) + sqr(tr.y-bl.y));
	list->win_size = tmp_sqrt; /* used for box vs crosshair in drawindow() */
	/* arbitrary value... stops browser zooming out too far */
	bool cant_see = false;
	if (tmp_sqrt < g_too_small)
	{
		cant_see = true;
	}
	/* reject anything too small onscreen */
	if (tmp_sqrt > toobig)
	{
		cant_see = true;
	}
	/* or too big... */

	/* restore original values */
	bflength = orig_bflength;
	bnlength = orig_bnlength;
	padding = orig_padding;
	rlength = orig_rlength;
	shiftfactor = orig_shiftfactor;
	rbflength = orig_rbflength;

	restore_stack(saved);
	if (cant_see) /* do it this way so bignum stack is released */
	{
		return false;
	}

	/* now see how many corners are on the screen, accept if one or more */
	int cornercount = 0;
	if (tl.x >= -g_sx_offset && tl.x <= (g_screen_width-g_sx_offset) && tl.y >= (0-g_sy_offset) && tl.y <= (g_screen_height-g_sy_offset))
	{
		cornercount++;
	}
	if (bl.x >= -g_sx_offset && bl.x <= (g_screen_width-g_sx_offset) && bl.y >= (0-g_sy_offset) && bl.y <= (g_screen_height-g_sy_offset))
	{
		cornercount++;
	}
	if (tr.x >= -g_sx_offset && tr.x <= (g_screen_width-g_sx_offset) && tr.y >= (0-g_sy_offset) && tr.y <= (g_screen_height-g_sy_offset))
	{
		cornercount++;
	}
	if (br.x >= -g_sx_offset && br.x <= (g_screen_width-g_sx_offset) && br.y >= (0-g_sy_offset) && br.y <= (g_screen_height-g_sy_offset))
	{
		cornercount++;
	}

	return (cornercount >= 1);
}

static void drawindow(int color, struct window *info)
{
#ifndef XFRACT
	int cross_size;
	Coordinate ibl;
	Coordinate itr;
#endif

	g_box_color = color;
	g_box_count = 0;
	if (info->win_size >= g_cross_hair_box_size)
	{
		/* big enough on screen to show up as a box so draw it */
		/* corner pixels */
#ifndef XFRACT
		add_box(info->itl);
		add_box(info->itr);
		add_box(info->ibl);
		add_box(info->ibr);
		draw_lines(info->itl, info->itr, info->ibl.x-info->itl.x, info->ibl.y-info->itl.y); /* top & bottom lines */
		draw_lines(info->itl, info->ibl, info->itr.x-info->itl.x, info->itr.y-info->itl.y); /* left & right lines */
#else
		g_box_x[0] = info->itl.x + g_sx_offset;
		g_box_y[0] = info->itl.y + g_sy_offset;
		g_box_x[1] = info->itr.x + g_sx_offset;
		g_box_y[1] = info->itr.y + g_sy_offset;
		g_box_x[2] = info->ibr.x + g_sx_offset;
		g_box_y[2] = info->ibr.y + g_sy_offset;
		g_box_x[3] = info->ibl.x + g_sx_offset;
		g_box_y[3] = info->ibl.y + g_sy_offset;
		g_box_count = 4;
#endif
		display_box();
	}
	else  /* draw crosshairs */
	{
#ifndef XFRACT
		cross_size = g_y_dots / 45;
		if (cross_size < 2)
		{
			cross_size = 2;
		}
		itr.x = info->itl.x - cross_size;
		itr.y = info->itl.y;
		ibl.y = info->itl.y - cross_size;
		ibl.x = info->itl.x;
		draw_lines(info->itl, itr, ibl.x-itr.x, 0); /* top & bottom lines */
		draw_lines(info->itl, ibl, 0, itr.y-ibl.y); /* left & right lines */
		display_box();
#endif
	}
}

static void bfsetup_convert_to_screen()
{
	/* setup_convert_to_screen() in LORENZ.C, converted to bf_math */
	/* Call only from within look_get_window() */
	int saved = save_stack();
	bf_t bt_inter1 = alloc_stack(rbflength + 2);
	bf_t bt_inter2 = alloc_stack(rbflength + 2);
	bf_t bt_det = alloc_stack(rbflength + 2);
	bf_t bt_xd = alloc_stack(rbflength + 2);
	bf_t bt_yd = alloc_stack(rbflength + 2);
	bf_t bt_tmp1 = alloc_stack(rbflength + 2);
	bf_t bt_tmp2 = alloc_stack(rbflength + 2);

	/* x3rd-xmin */
	sub_bf(bt_inter1, g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_min());
	/* ymin-ymax */
	sub_bf(bt_inter2, g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_max());
	/* (x3rd-xmin)*(ymin-ymax) */
	mult_bf(bt_tmp1, bt_inter1, bt_inter2);

	/* ymax-y3rd */
	sub_bf(bt_inter1, g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_3rd());
	/* xmax-xmin */
	sub_bf(bt_inter2, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min());
	/* (ymax-y3rd)*(xmax-xmin) */
	mult_bf(bt_tmp2, bt_inter1, bt_inter2);

	/* det = (x3rd-xmin)*(ymin-ymax) + (ymax-y3rd)*(xmax-xmin) */
	add_bf(bt_det, bt_tmp1, bt_tmp2);

	/* xd = g_dx_size/det */
	floattobf(bt_tmp1, g_dx_size);
	div_bf(bt_xd, bt_tmp1, bt_det);

	/* a = xd*(ymax-y3rd) */
	sub_bf(bt_inter1, g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_3rd());
	mult_bf(bt_a, bt_xd, bt_inter1);

	/* b = xd*(x3rd-xmin) */
	sub_bf(bt_inter1, g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_min());
	mult_bf(bt_b, bt_xd, bt_inter1);

	/* e = -(a*xmin + b*ymax) */
	mult_bf(bt_tmp1, bt_a, g_escape_time_state.m_grid_bf.x_min());
	mult_bf(bt_tmp2, bt_b, g_escape_time_state.m_grid_bf.y_max());
	neg_a_bf(add_bf(bt_e, bt_tmp1, bt_tmp2));

	/* x3rd-xmax */
	sub_bf(bt_inter1, g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_max());
	/* ymin-ymax */
	sub_bf(bt_inter2, g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_max());
	/* (x3rd-xmax)*(ymin-ymax) */
	mult_bf(bt_tmp1, bt_inter1, bt_inter2);

	/* ymin-y3rd */
	sub_bf(bt_inter1, g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_3rd());
	/* xmax-xmin */
	sub_bf(bt_inter2, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min());
	/* (ymin-y3rd)*(xmax-xmin) */
	mult_bf(bt_tmp2, bt_inter1, bt_inter2);

	/* det = (x3rd-xmax)*(ymin-ymax) + (ymin-y3rd)*(xmax-xmin) */
	add_bf(bt_det, bt_tmp1, bt_tmp2);

	/* yd = g_dy_size/det */
	floattobf(bt_tmp2, g_dy_size);
	div_bf(bt_yd, bt_tmp2, bt_det);

	/* c = yd*(ymin-y3rd) */
	sub_bf(bt_inter1, g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_3rd());
	mult_bf(bt_c, bt_yd, bt_inter1);

	/* d = yd*(x3rd-xmax) */
	sub_bf(bt_inter1, g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_max());
	mult_bf(bt_d, bt_yd, bt_inter1);

	/* f = -(c*xmin + d*ymax) */
	mult_bf(bt_tmp1, bt_c, g_escape_time_state.m_grid_bf.x_min());
	mult_bf(bt_tmp2, bt_d, g_escape_time_state.m_grid_bf.y_max());
	neg_a_bf(add_bf(bt_f, bt_tmp1, bt_tmp2));

	restore_stack(saved);
}

static char typeOK(fractal_info *info, struct ext_blk_formula_info *formula_info)
{
	int numfn;
	if ((g_fractal_type == FRACTYPE_FORMULA || g_fractal_type == FRACTYPE_FORMULA_FP) &&
		(info->fractal_type == FRACTYPE_FORMULA || info->fractal_type == FRACTYPE_FORMULA_FP))
	{
		if (!stricmp(formula_info->form_name, g_formula_name))
		{
			numfn = g_formula_state.max_fn();
			return (numfn > 0) ? functionOK(info, numfn) : 1;
		}
		else
		{
			return 0; /* two formulas but names don't match */
		}
	}
	else if (info->fractal_type == g_fractal_type ||
			info->fractal_type == g_current_fractal_specific->tofloat)
	{
		numfn = (g_current_fractal_specific->flags >> 6) & 7;
		return (numfn > 0) ? functionOK(info, numfn) : 1;
	}
	else
	{
		return 0; /* no match */
	}
}

static char functionOK(fractal_info *info, int numfn)
{
	int mzmatch = 0;
	for (int i = 0; i < numfn; i++)
	{
		if (info->function_index[i] != g_function_index[i])
		{
			mzmatch++;
		}
	}
	return (mzmatch > 0) ? 0 : 1;
}

static char paramsOK(fractal_info *info)
{
#define MINDIF 0.001

	double tmpparm3;
	double tmpparm4;
	if (info->version > 6)
	{
		tmpparm3 = info->dparm3;
		tmpparm4 = info->dparm4;
	}
	else
	{
		tmpparm3 = info->parm3;
		round_float_d(&tmpparm3);
		tmpparm4 = info->parm4;
		round_float_d(&tmpparm4);
	}

	double tmpparm5;
	double tmpparm6;
	double tmpparm7;
	double tmpparm8;
	double tmpparm9;
	double tmpparm10;
	if (info->version > 8)
	{
		tmpparm5 = info->dparm5;
		tmpparm6 = info->dparm6;
		tmpparm7 = info->dparm7;
		tmpparm8 = info->dparm8;
		tmpparm9 = info->dparm9;
		tmpparm10 = info->dparm10;
	}
	else
	{
		tmpparm5 = 0.0;
		tmpparm6 = 0.0;
		tmpparm7 = 0.0;
		tmpparm8 = 0.0;
		tmpparm9 = 0.0;
		tmpparm10 = 0.0;
	}
	/* parameters are in range? */
	return
		(fabs(info->c_real - g_parameters[0]) < MINDIF &&
		fabs(info->c_imag - g_parameters[1]) < MINDIF &&
		fabs(tmpparm3 - g_parameters[2]) < MINDIF &&
		fabs(tmpparm4 - g_parameters[3]) < MINDIF &&
		fabs(tmpparm5 - g_parameters[4]) < MINDIF &&
		fabs(tmpparm6 - g_parameters[5]) < MINDIF &&
		fabs(tmpparm7 - g_parameters[6]) < MINDIF &&
		fabs(tmpparm8 - g_parameters[7]) < MINDIF &&
		fabs(tmpparm9 - g_parameters[8]) < MINDIF &&
		fabs(tmpparm10 - g_parameters[9]) < MINDIF &&
		info->invert[0] - g_inversion[0] < MINDIF)
		? 1 : 0;
}


static void check_history(const char *oldname, const char *newname)
{
	/* g_file_name_stack[] is maintained in framain2.c.  It is the history */
	/*  file for the browser and holds a maximum of 16 images.  The history */
	/*  file needs to be adjusted if the rename or delete functions of the */
	/*  browser are used. */
	/* g_name_stack_ptr is also maintained in framain2.c.  It is the index into */
	/*  g_file_name_stack[]. */
	for (int i = 0; i < g_name_stack_ptr; i++)
	{
		if (stricmp(g_file_name_stack[i], oldname) == 0) /* we have a match */
		{
			strcpy(g_file_name_stack[i], newname);    /* insert the new name */
		}
	}
}

/* look_get_window reads all .GIF files and draws window outlines on the screen */
int look_get_window()
{
	struct affine stack_cvt;
	fractal_info read_info;
	struct ext_blk_resume_info resume_info_blk;
	struct ext_blk_formula_info formula_info;
	struct ext_blk_ranges_info ranges_info;
	struct ext_blk_mp_info mp_info;
	struct ext_blk_evolver_info evolver_info;
	struct ext_blk_orbits_info orbits_info;
	time_t thistime;
	time_t lastime;
	char mesg[40];
	char newname[60];
	char oldname[60];
	int index;
	int done;
	int wincount;
	int toggle;
	int color_of_box;
	window winlist;
	char drive[FILE_MAX_DRIVE];
	char dir[FILE_MAX_DIR];
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	char tmpmask[FILE_MAX_PATH];
	int vid_too_big = 0;
	int no_memory = 0;
	int vidlength;
	int saved;
#ifdef XFRACT
	U32 blinks;
#endif

	HelpModeSaver saved_help(HELPBROWSE);
	oldbf_math = g_bf_math;
	g_bf_math = BIGFLT;
	if (!oldbf_math)
	{
		int oldcalc_status = g_calculation_status; /* kludge because next sets it = 0 */
		fractal_float_to_bf();
		g_calculation_status = oldcalc_status;
	}
	saved = save_stack();
	bt_a = alloc_stack(rbflength + 2);
	bt_b = alloc_stack(rbflength + 2);
	bt_c = alloc_stack(rbflength + 2);
	bt_d = alloc_stack(rbflength + 2);
	bt_e = alloc_stack(rbflength + 2);
	bt_f = alloc_stack(rbflength + 2);

	vidlength = g_screen_width + g_screen_height;
	if (vidlength > 4096)
	{
		vid_too_big = 2;
	}
	/* 4096 based on 4096B in g_box_x... max 1/4 pixels plotted, and need words */
	/* 4096 = 10240/2.5 based on size of g_box_x + g_box_y + g_box_values */
#ifdef XFRACT
	vidlength = 4; /* Xfractint only needs the 4 corners saved. */
#endif
	boxx_storage = (int *) malloc(vidlength*MAX_WINDOWS_OPEN*sizeof(int));
	boxy_storage = (int *) malloc(vidlength*MAX_WINDOWS_OPEN*sizeof(int));
	boxvalues_storage = (int *) malloc(vidlength/2*MAX_WINDOWS_OPEN*sizeof(int));
	if (!boxx_storage || !boxy_storage || !boxvalues_storage)
	{
		no_memory = 1;
	}

	/* set up complex-plane-to-screen transformation */
	if (oldbf_math)
	{
		bfsetup_convert_to_screen();
	}
	else
	{
		cvt = &stack_cvt; /* use stack */
		setup_convert_to_screen(cvt);
		/* put in bf variables */
		floattobf(bt_a, cvt->a);
		floattobf(bt_b, cvt->b);
		floattobf(bt_c, cvt->c);
		floattobf(bt_d, cvt->d);
		floattobf(bt_e, cvt->e);
		floattobf(bt_f, cvt->f);
	}
	find_special_colors();
	color_of_box = g_color_medium;

rescan:  /* entry for changed browse parms */
	time(&lastime);
	toggle = 0;
	wincount = 0;
	g_no_sub_images = false;
	split_path(g_read_name, drive, dir, NULL, NULL);
	split_path(g_browse_state.mask(), NULL, NULL, fname, ext);
	make_path(tmpmask, drive, dir, fname, ext);
	done = (vid_too_big == 2) || no_memory || fr_find_first(tmpmask);
								/* draw all visible windows */
	while (!done)
	{
		if (driver_key_pressed())
		{
			driver_get_key();
			break;
		}
		split_path(g_dta.filename, NULL, NULL, fname, ext);
		make_path(tmpmask, drive, dir, fname, ext);
		if (!find_fractal_info(tmpmask, &read_info, &resume_info_blk, &formula_info,
				&ranges_info, &mp_info, &evolver_info, &orbits_info)
			&& (typeOK(&read_info, &formula_info) || !g_browse_state.check_type())
			&& (paramsOK(&read_info) || !g_browse_state.check_parameters())
			&& stricmp(g_browse_state.name(), g_dta.filename)
			&& evolver_info.got_data != 1
			&& is_visible_window(&winlist, &read_info, &mp_info))
		{
			strcpy(winlist.name, g_dta.filename);
			drawindow(color_of_box, &winlist);
			g_box_count *= 2; /* double for byte count */
			winlist.box_count = g_box_count;
			browse_windows[wincount] = winlist;

			memcpy(&boxx_storage[wincount*vidlength], g_box_x, vidlength*sizeof(int));
			memcpy(&boxy_storage[wincount*vidlength], g_box_y, vidlength*sizeof(int));
			memcpy(&boxvalues_storage[wincount*vidlength/2], g_box_values, vidlength/2*sizeof(int));
			wincount++;
		}

		if (resume_info_blk.got_data == 1) /* Clean up any memory allocated */
		{
			free(resume_info_blk.resume_data);
		}
		if (ranges_info.got_data == 1) /* Clean up any memory allocated */
		{
			free(ranges_info.range_data);
		}
		if (mp_info.got_data == 1) /* Clean up any memory allocated */
		{
			free(mp_info.apm_data);
		}

		done = (fr_find_next() || wincount >= MAX_WINDOWS_OPEN);
	}

	if (no_memory)
	{
		text_temp_message("Sorry...not enough memory to browse."); /* doesn't work if NO memory available, go figure */
	}
	if (wincount >= MAX_WINDOWS_OPEN)
	{ /* hard code message at MAX_WINDOWS_OPEN = 450 */
		text_temp_message("Sorry...no more space, 450 displayed.");
	}
	if (vid_too_big == 2)
	{
		text_temp_message("Xdots + Ydots > 4096.");
	}
	int c = 0;
	if (wincount)
	{
		driver_buzzer(BUZZER_COMPLETE); /*let user know we've finished */
		index = 0; done = 0;
		winlist = browse_windows[index];
		memcpy(g_box_x, &boxx_storage[index*vidlength], vidlength*sizeof(int));
		memcpy(g_box_y, &boxy_storage[index*vidlength], vidlength*sizeof(int));
		memcpy(g_box_values, &boxvalues_storage[index*vidlength/2], vidlength/2*sizeof(int));
		show_temp_message(winlist.name);
		while (!done)  /* on exit done = 1 for quick exit,
						done = 2 for erase boxes and  exit
						done = 3 for rescan
						done = 4 for set boxes and exit to save image */
		{
#ifdef XFRACT
			blinks = 1;
#endif
			while (!driver_key_pressed())
			{
				time(&thistime);
				if (difftime(thistime, lastime) > .2)
				{
					lastime = thistime;
					toggle = 1- toggle;
				}
				drawindow(toggle ? g_color_bright : g_color_dark, &winlist);   /* flash current window */
#ifdef XFRACT
				blinks++;
#endif
			}
#ifdef XFRACT
			if ((blinks & 1) == 1)   /* Need an odd # of blinks, so next one leaves box turned off */
			{
				drawindow(g_color_bright, &winlist);
			}
#endif

			c = driver_get_key();
			switch (c)
			{
			case FIK_RIGHT_ARROW:
			case FIK_LEFT_ARROW:
			case FIK_DOWN_ARROW:
			case FIK_UP_ARROW:
				clear_temp_message();
				drawindow(color_of_box, &winlist); /* dim last window */
				if (c == FIK_RIGHT_ARROW || c == FIK_UP_ARROW)
				{
					index++;                     /* shift attention to next window */
					if (index >= wincount)
					{
						index = 0;
					}
				}
				else
				{
					index --;
					if (index < 0)
					{
						index = wincount -1;
					}
				}
				winlist = browse_windows[index];
				memcpy(g_box_x, &boxx_storage[index*vidlength], vidlength*sizeof(int));
				memcpy(g_box_y, &boxy_storage[index*vidlength], vidlength*sizeof(int));
				memcpy(g_box_values, &boxvalues_storage[index*vidlength/2], vidlength/2*sizeof(int));
				show_temp_message(winlist.name);
				break;
#ifndef XFRACT
			case FIK_CTL_INSERT:
				color_of_box += key_count(FIK_CTL_INSERT);
				for (int i = 0; i < wincount; i++)
				{
					winlist = browse_windows[i];
					drawindow(color_of_box, &winlist);
				}
				winlist = browse_windows[index];
				drawindow(color_of_box, &winlist);
				break;

			case FIK_CTL_DEL:
				color_of_box -= key_count(FIK_CTL_DEL);
				for (int i = 0; i < wincount; i++)
				{
					winlist = browse_windows[i];
					drawindow(color_of_box, &winlist);
				}
				winlist = browse_windows[index];
				drawindow(color_of_box, &winlist);
				break;
#endif
			case FIK_ENTER:
			case FIK_ENTER_2:   /* this file please */
				g_browse_state.set_name(winlist.name);
				done = 1;
				break;

			case FIK_ESC:
			case 'l':
			case 'L':
#ifdef XFRACT
				/* Need all boxes turned on, turn last one back on. */
				drawindow(g_color_bright, &winlist);
#endif
				g_browse_state.set_auto_browse(false);
				done = 2;
				break;

			case 'D': /* delete file */
				clear_temp_message();
				_snprintf(mesg, NUM_OF(mesg), "Delete %s? (Y/N)", winlist.name);
				show_temp_message(mesg);
				driver_wait_key_pressed(0);
				clear_temp_message();
				c = driver_get_key();
				if (c == 'Y' && g_ui_state.double_caution)
				{
					text_temp_message("ARE YOU SURE???? (Y/N)");
					if (driver_get_key() != 'Y')
					{
						c = 'N';
					}
				}
				if (c == 'Y')
				{
					split_path(g_read_name, drive, dir, NULL, NULL);
					split_path(winlist.name, NULL, NULL, fname, ext);
					make_path(tmpmask, drive, dir, fname, ext);
					if (!unlink(tmpmask))
					{
						/* do a rescan */
						done = 3;
						strcpy(oldname, winlist.name);
						tmpmask[0] = '\0';
						check_history(oldname, tmpmask);
						break;
					}
					else if (errno == EACCES)
					{
						text_temp_message("Sorry...it's a read only file, can't del");
						show_temp_message(winlist.name);
						break;
					}
				}
				text_temp_message("file not deleted (phew!)");
				show_temp_message(winlist.name);
				break;

			case 'R':
				clear_temp_message();
				driver_stack_screen();
				newname[0] = 0;
				strcpy(mesg, "Enter the new filename for ");
				split_path(g_read_name, drive, dir, NULL, NULL);
				split_path(winlist.name, NULL, NULL, fname, ext);
				make_path(tmpmask, drive, dir, fname, ext);
				strcpy(newname, tmpmask);
				strcat(mesg, tmpmask);
				{
					int i = field_prompt(mesg, NULL, newname, 60, NULL);
					driver_unstack_screen();
					if (i != -1)
					{
						if (!rename(tmpmask, newname))
						{
							if (errno == EACCES)
							{
								text_temp_message("Sorry....can't rename");
							}
							else
							{
								split_path(newname, NULL, NULL, fname, ext);
								make_path(tmpmask, NULL, NULL, fname, ext);
								strcpy(oldname, winlist.name);
								check_history(oldname, tmpmask);
								strcpy(winlist.name, tmpmask);
							}
						}
					}
				}
				browse_windows[index] = winlist;
				show_temp_message(winlist.name);
				break;

			case FIK_CTL_B:
				clear_temp_message();
				driver_stack_screen();
				done = abs(get_browse_parameters());
				driver_unstack_screen();
				show_temp_message(winlist.name);
				break;

			case 's': /* save image with boxes */
				g_browse_state.set_auto_browse(false);
				drawindow(color_of_box, &winlist); /* current window white */
				done = 4;
				break;

			case '\\': /*back out to last image */
				done = 2;
				break;

			default:
				break;
			} /*switch */
		} /*while*/

		/* now clean up memory (and the screen if necessary) */
		clear_temp_message();
		if (done >= 1 && done < 4)
		{
			for (index = wincount-1; index >= 0; index--) /* don't need index, reuse it */
			{
				winlist = browse_windows[index];
				g_box_count = winlist.box_count;
				memcpy(g_box_x, &boxx_storage[index*vidlength], vidlength*sizeof(int));
				memcpy(g_box_y, &boxy_storage[index*vidlength], vidlength*sizeof(int));
				memcpy(g_box_values, &boxvalues_storage[index*vidlength/2], vidlength/2*sizeof(int));
				g_box_count >>= 1;
				if (g_box_count > 0)
				{
#ifdef XFRACT
					/* Turn all boxes off */
					drawindow(g_color_bright, &winlist);
#else
					clear_box();
#endif
				}
			}
		}
		if (done == 3)
		{
			goto rescan; /* hey everybody I just used the g word! */
		}
	}/*if*/
	else
	{
		driver_buzzer(BUZZER_INTERRUPT); /*no suitable files in directory! */
		text_temp_message("Sorry.. I can't find anything");
		g_no_sub_images = true;
	}

	free(boxx_storage);
	free(boxy_storage);
	free(boxvalues_storage);
	restore_stack(saved);
	if (!oldbf_math)
	{
		free_bf_vars();
	}
	g_bf_math = oldbf_math;
	g_float_flag = (g_user_float_flag != 0);

	return c;
}


