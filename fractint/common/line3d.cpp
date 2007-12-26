/************************************************************************/
/* This file contains a 3D replacement for the out_line function called */
/* by the decoder. The purpose is to apply various 3D transformations   */
/* before displaying points. Called once per line of the input file.    */
/*                                                                      */
/* Original Author Tim Wegner, with extensive help from Marc Reinig.    */
/*    July 1994 - TW broke out several pieces of code and added pragma  */
/*                to eliminate compiler warnings. Did a long-overdue    */
/*                formatting cleanup.                                   */
/************************************************************************/
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

#include <limits.h>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"

#include "3d.h"
#include "diskvid.h"
#include "drivers.h"
#include "filesystem.h"
#include "line3d.h"
#include "miscres.h"
#include "prompts2.h"
#include "realdos.h"

#include "ThreeDimensionalState.h"
#include "MathUtil.h"

#define FILEERROR_NONE				0
#define FILEERROR_OPEN				1
#define FILEERROR_NO_SPACE			2
#define FILEERROR_BAD_IMAGE_SIZE	3
#define FILEERROR_BAD_FILE_TYPE		4

#define TARGA_24 24
#define TARGA_32 32

#define PERSPECTIVE_DISTANCE 250		/* Perspective dist used when viewing light vector */
#define BAD_CHECK -3000					/* check values against this to determine if good */

template <typename T>
struct point_t
{
	T x;
	T y;
	T color;
};

typedef point_t<int> point;
typedef point_t<float> point_fp;

struct minmax
{
	int minx;
	int maxx;
};

class acrospin_data
{
public:
	acrospin_data()
		: _rows(0),
		_columns(0),
		_maxColumns(-1)
	{
	}

	void line3d_cleanup();
	void first_time();
	void line3d_planar(point_fp *current);
	void line3d_raytrace(point_fp const *current);
	void next_row();
	void reset_columns() { _columns = 0; }

private:
	int _rows;
	int _columns;
	int _maxColumns;
};

/* routines in this module */
int out_line_3d(BYTE *pixels, int line_length);
int targa_color(int, int, int);
int start_disk_targa(const std::string &file_name2, FILE *Source, bool overlay_file);
int start_disk_targa(const std::string &filename, std::ifstream &source, bool overlay_file);

/* global variables defined here */
void (*g_plot_color_standard)(int x, int y, int color) = 0;

std::string g_light_name = "fract001";
bool g_targa_overlay = false;
int g_xx_adjust = 0;
int g_yy_adjust = 0;
int g_x_shift = 0;
int g_y_shift = 0;
VECTOR g_view;                /* position of observer for perspective */
VECTOR g_cross;
const int g_bad_value = -10000; /* set bad values to this */

static int targa_validate(const std::string &filename);
static int first_time(int, VECTOR);
static int HSVtoRGB(BYTE *, BYTE *, BYTE *, unsigned long, unsigned long, unsigned long);
static bool line_3d_mem();
static int RGBtoHSV(BYTE, BYTE, BYTE, unsigned long *, unsigned long *, unsigned long *);
static int set_pixel_buff(BYTE *, BYTE *, unsigned);
static void set_upr_lwr();
static void end_object(bool triangle_was_output);
static int off_screen(point);
static int out_triangle(const point_fp &, const point_fp &, const point_fp &, int, int, int);
static int raytrace_header();
static int start_object();
static void corners(MATRIX m, bool show, double *pxmin, double *pymin, double *pzmin, double *pxmax, double *pymax, double *pzmax);
static void draw_light_box(double *, double *, MATRIX);
static void draw_rectangle(VECTOR V0, VECTOR V1, VECTOR V2, VECTOR V3, int color);
static void draw_rectangle_lines(VECTOR V0, VECTOR V1, VECTOR V2, VECTOR V3, int color);
static void line3d_cleanup();
static void plot_color_clip(int, int, int);
static void interp_color(int, int, int);
static void put_a_triangle(point, point, point, int);
static void plot_color_put_min_max(int, int, int);
static void triangle_bounds(float pt_t[3][3]);
static void plot_color_transparent_clip(int, int, int);
static void vdraw_line(double *, double *, int color);
static void (*s_plot_color_fill)(int, int, int);
static void (*s_plot_color_normal)(int, int, int);
static void file_error(const std::string &filename, int code);

/* static variables */
static double s_r_scale = 0.0;			/* surface roughness factor */
static long s_x_center = 0;
static long s_y_center = 0; /* circle center */
static double s_scale_x = 0.0;
static double s_scale_y = 0.0;
static double s_scale_z = 0.0; /* scale factors */
static double s_radius = 0.0;			/* radius values */
static double s_radius_factor = 0.0;	/* for intermediate calculation */
static MATRIX_L s_lm;					/* "" */
static VECTOR_L s_lview;					/* for perspective views */
static double s_z_cutoff = 0.0;			/* perspective backside cutoff value */
static float s_two_cos_delta_phi = 0.0;
static float s_cos_phi;
static float s_sin_phi;		/* precalculated sin/cos of longitude */
static float s_old_cos_phi1;
static float s_old_sin_phi1;
static float s_old_cos_phi2;
static float s_old_sin_phi2;
static BYTE *s_fraction = 0;			/* float version of pixels array */
static float s_min_xyz[3];
static float s_max_xyz[3];	/* For Raytrace output */
static int s_line_length;
static int s_targa_header_len = 18;			/* Size of current Targa-24 header */
static std::ofstream s_raytrace_file;
static unsigned int s_ambient;
static int s_rand_factor;
static int s_haze_mult;
static BYTE s_targa_size[4];
static bool s_targa_safe;						/* Original Targa Image successfully copied to s_targa_temp */
static VECTOR s_light_direction;
static BYTE s_real_color;					/* Actual color of cur pixel */
static acrospin_data s_acrospin;
static int CO;
static int CO_MAX;				/* For use in Acrospin support */
static int s_local_preview_factor;
static int s_z_coord = 256;
static double s_aspect;					/* aspect ratio */
static int s_even_odd_row;
static float *s_sin_theta_array;			/* all sine thetas go here  */
static float *s_cos_theta_array;			/* all cosine thetas go here */
static double s_r_scale_r;					/* precalculation factor */
static bool s_persp;						/* flag for indicating perspective transformations */
static point s_p1;
static point s_p2;
static point s_p3;
static point_fp s_f_bad;			/* out of range value */
static point s_bad;				/* out of range value */
static long s_num_tris;					/* number of triangles output to ray trace file */
static point_fp *s_f_last_row = 0;
static MATRIX s_m;						/* transformation matrix */
static int s_file_error = FILEERROR_NONE;
static char *s_targa_temp = "idtemp.tga";
static point *s_last_row = 0;	/* this array remembers the previous line */
static struct minmax *s_minmax_x;			/* array of min and max x values used in triangle fill */

void acrospin_data::line3d_cleanup()
{
	s_raytrace_file << "\n"; 
	s_raytrace_file << "LineList From To\n";
	for (int i = 0; i < _rows; i++)
	{
		for (int j = 0; j <= _maxColumns; j++)
		{
			if (j < _maxColumns)
			{
				s_raytrace_file << boost::format("R%dC%d R%dC%d\n") % i % j % i % (j + 1);
			}
			if (i < _rows - 1)
			{
				s_raytrace_file << boost::format("R%dC%d R%dC%d\n") % i % j % (i + 1) % j;
			}
			if (i && i < _rows && j < _maxColumns)
			{
				s_raytrace_file << boost::format("R%dC%d R%dC%d\n") % i % j % (i - 1) % (j + 1);
			}
		}
	}
	s_raytrace_file << "\n\n--";
	s_raytrace_file << boost::format("{ No. Of Triangles = %ld }*/\n\n") % s_num_tris;
}

void acrospin_data::line3d_raytrace(point_fp const *current)
{
	s_raytrace_file << boost::format("% #4.4f % #4.4f % #4.4f R%dC%d\n")
		% current->x % current->y % current->color % _rows % _columns;
	_maxColumns = std::max(_columns, _maxColumns);
	_columns++;
}

void acrospin_data::first_time()
{
	_maxColumns = 0;
	_columns = 0;
	_rows = 0;
}

void acrospin_data::line3d_planar(point_fp *current)
{
	current->x = current->x*(2.0f/g_x_dots) - 1.0f;
	current->y = current->y*(2.0f/g_y_dots) - 1.0f;
	current->color = -current->color*(2.0f/g_num_colors) - 1.0f;
}

void acrospin_data::next_row()
{
	_rows++;
}

static int line3d_init(unsigned linelen, bool &triangle_was_output,
					   int *xcenter0, int *ycenter0, VECTOR cross_avg, VECTOR v)
{
	int err = first_time(linelen, v);
	if (err != 0)
	{
		return err;
	}
	if (g_x_dots > OLD_MAX_PIXELS)
	{
		return -1;
	}
	triangle_was_output = false;
	cross_avg[0] = 0;
	cross_avg[1] = 0;
	cross_avg[2] = 0;
	s_x_center = g_x_dots/2 + g_x_shift;
	s_y_center = g_y_dots/2 - g_y_shift;
	*xcenter0 = int(s_x_center);
	*ycenter0 = int(s_y_center);
	return 0;
}

static int line3d_sphere(int col, int xcenter0, int ycenter0,
						 point *cur, point_fp *f_cur, double *r, VECTOR_L lv, VECTOR v)
{
	float cos_theta = s_sin_theta_array[col];
	float sin_theta = s_cos_theta_array[col];    /* precalculated sin/cos of latitude */

	if (s_sin_phi < 0 && !(g_3d_state.raytrace_output() || g_3d_state.fill_type() < FillType::Points))
	{
		*cur = s_bad;
		*f_cur = s_f_bad;
		return 1;
	}
	/************************************************************/
	/* KEEP THIS FOR DOCS - original formula --                 */
	/* if (s_r_scale < 0.0)                                         */
	/* r = 1.0 + (double(cur.color)/double(s_z_coord))*s_r_scale;       */
	/* else                                                     */
	/* r = 1.0-s_r_scale + (double(cur.color)/double(s_z_coord))*s_r_scale;*/
	/* s_radius = double(g_y_dots)/2;                                     */
	/* r = r*s_radius;                                                 */
	/* cur.x = g_x_dots/2 + s_scale_x*r*sin_theta*s_aspect + xup;         */
	/* cur.y = g_y_dots/2 + s_scale_y*r*cos_theta*s_cos_phi - yup;         */
	/************************************************************/

	if (s_r_scale < 0.0)
	{
		*r = s_radius + s_radius_factor*f_cur->color*cos_theta;
	}
	else if (s_r_scale > 0.0)
	{
		*r = s_radius - s_r_scale_r + s_radius_factor*double(f_cur->color)*cos_theta;
	}
	else
	{
		*r = s_radius;
	}
	/* Allow Ray trace to go through so display ok */
	if (s_persp || g_3d_state.raytrace_output())
	{
		/* mrr how do lv[] and cur and f_cur all relate */
		/* NOTE: g_fudge was pre-calculated above in r and s_radius */
		/* (almost) guarantee negative */
		lv[2] = long(-s_radius - (*r)*cos_theta*s_sin_phi);     /* z */
		// TODO: what should this really be?  Was: !FILLTYPE < FillTypePoints
		if ((lv[2] > s_z_cutoff) && g_3d_state.fill_type() < FillType::Points)
		{
			*cur = s_bad;
			*f_cur = s_f_bad;
			return 1;
		}
		lv[0] = long(s_x_center + sin_theta*s_scale_x*(*r));  /* x */
		lv[1] = long(s_y_center + cos_theta*s_cos_phi*s_scale_y*(*r)); /* y */

		if ((g_3d_state.fill_type() >= FillType::LightBefore) || g_3d_state.raytrace_output())
		{
			/* calculate illumination normal before s_persp */
			double r0 = (*r)/65536L;
			f_cur->x = float(xcenter0 + sin_theta*s_scale_x*r0);
			f_cur->y = float(ycenter0 + cos_theta*s_cos_phi*s_scale_y*r0);
			f_cur->color = float(-r0*cos_theta*s_sin_phi);
		}
		if (!(g_user_float_flag || g_3d_state.raytrace_output()))
		{
			if (longpersp(lv, s_lview, 16) == -1)
			{
				*cur = s_bad;
				*f_cur = s_f_bad;
				return 1;
			}
			cur->x = int(((lv[0] + 32768L) >> 16) + g_xx_adjust);
			cur->y = int(((lv[1] + 32768L) >> 16) + g_yy_adjust);
		}
		if (g_user_float_flag || g_overflow || g_3d_state.raytrace_output())
		{
			v[0] = lv[0];
			v[1] = lv[1];
			v[2] = lv[2];
			v[0] /= g_fudge;
			v[1] /= g_fudge;
			v[2] /= g_fudge;
			perspective(v);
			cur->x = int(v[0] + .5 + g_xx_adjust);
			cur->y = int(v[1] + .5 + g_yy_adjust);
		}
	}
	/* mrr Not sure how this an 3rd if above relate */
	else if (!(s_persp && g_3d_state.raytrace_output()))
	{
		/* mrr Why the xx- and g_yy_adjust here and not above? */
		f_cur->x = float(s_x_center + sin_theta*s_scale_x*(*r) + g_xx_adjust);
		f_cur->y = float(s_y_center + cos_theta*s_cos_phi*s_scale_y*(*r) + g_yy_adjust);
		cur->x = int(f_cur->x);
		cur->y = int(f_cur->y);
		/* mrr why do we do this for filltype > 5? */
		if (g_3d_state.fill_type() >= FillType::LightBefore || g_3d_state.raytrace_output())
		{
			f_cur->color = float(-(*r)*cos_theta*s_sin_phi*s_scale_z);
		}
		v[0] = 0;
		v[1] = 0;
		v[2] = 0;  /* MRR Why do we do this? */
	}
	return 0;
}

static int line3d_planar(int col, point_fp *f_cur, point *cur,
						 VECTOR_L lv0, VECTOR_L lv, VECTOR v, float *f_water)
{
	if (!g_user_float_flag && !g_3d_state.raytrace_output())
	{
		/* flag to save vector before perspective */
		/* in longvmultpersp calculation */
		lv0[0] = (g_3d_state.fill_type() >= FillType::LightBefore) ? 1 : 0;   

		/* use 32-bit multiply math to snap this out */
		lv[0] = col;
		lv[0] = lv[0] << 16;
		lv[1] = g_current_row;
		lv[1] = lv[1] << 16;
		lv[2] = long(f_cur->color);
		lv[2] = lv[2] << 16;

		if (vmult_perspective_l(lv, s_lm, lv0, lv, s_lview, 16) == -1)
		{
			*cur = s_bad;
			*f_cur = s_f_bad;
			return 1;
		}

		cur->x = int(((lv[0] + 32768L) >> 16) + g_xx_adjust);
		cur->y = int(((lv[1] + 32768L) >> 16) + g_yy_adjust);
		if (g_3d_state.fill_type() >= FillType::LightBefore && !g_overflow)
		{
			f_cur->x = float(lv0[0]);
			f_cur->x /= 65536.0f;
			f_cur->y = float(lv0[1]);
			f_cur->y /= 65536.0f;
			f_cur->color = float(lv0[2]);
			f_cur->color /= 65536.0f;
		}
	}

	if (g_user_float_flag || g_overflow || g_3d_state.raytrace_output())
		/* do in float if integer math overflowed or doing Ray trace */
	{
		/* slow float version for comparison */
		v[0] = col;
		v[1] = g_current_row;
		v[2] = f_cur->color;      /* Actually the z value */

		mult_vec(v, s_m);     /* matrix*vector routine */

		if (g_3d_state.fill_type() > FillType::Bars || g_3d_state.raytrace_output())
		{
			f_cur->x = float(v[0]);
			f_cur->y = float(v[1]);
			f_cur->color = float(v[2]);

			if (g_3d_state.raytrace_output() == RAYTRACE_ACROSPIN)
			{
				s_acrospin.line3d_planar(f_cur);
			}
		}

		if (s_persp && !g_3d_state.raytrace_output())
		{
			perspective(v);
		}
		cur->x = int(v[0] + g_xx_adjust + .5);
		cur->y = int(v[1] + g_yy_adjust + .5);

		v[0] = 0;
		v[1] = 0;
		v[2] = g_3d_state.water_line();
		mult_vec(v, s_m);
		*f_water = float(v[2]);
	}

	return 0;
}

void draw_line(const point &first, const point &second, int color)
{
	driver_draw_line(first.x, first.y, second.x, second.y, color);
}

bool bad_point(const point &pt)
{
	return pt.x > BAD_CHECK && pt.y > BAD_CHECK;
}

bool bad_point_size(const point &pt, int width, int height)
{
	return pt.x < (width - BAD_CHECK) && pt.y < (height - BAD_CHECK);
}

bool bad_point_size(const point &pt)
{
	return bad_point_size(pt, g_x_dots, g_y_dots);
}

static void line3d_raytrace(int col, int next,
							const point *old, const point *cur,
							const point_fp *f_old, const point_fp *f_cur,
							float f_water, int last_dot,
							bool &triangle_was_output)
{
	if (col && g_current_row
		&& old->x > BAD_CHECK && old->x < (g_x_dots - BAD_CHECK)
		&& bad_point(s_last_row[col]) && bad_point_size(s_last_row[col]))
	{
		/* Get rid of all the triangles in the plane at the base of
		* the object */

		if (f_cur->color == f_water
			&& s_f_last_row[col].color == f_water
			&& s_f_last_row[next].color == f_water)
		{
			return;
		}

		if (! (g_3d_state.raytrace_output() == RAYTRACE_ACROSPIN))
		{
			/* Output the vertex info */
			out_triangle(*f_cur, *f_old, s_f_last_row[col],
				cur->color, old->color, s_last_row[col].color);
		}

		triangle_was_output = true;

		draw_line(*old, *cur, old->color);
		draw_line(*old, s_last_row[col], old->color);
		draw_line(s_last_row[col], *cur, cur->color);
		s_num_tris++;
	}

	if (col < last_dot && g_current_row
		&& bad_point(s_last_row[col]) && bad_point_size(s_last_row[col])
		&& bad_point(s_last_row[next]) && bad_point_size(s_last_row[next]))
	{
		/* Get rid of all the triangles in the plane at the base of
		* the object */

		if (f_cur->color == f_water &&
			s_f_last_row[col].color == f_water &&
			s_f_last_row[next].color == f_water)
		{
			return;
		}

		if (g_3d_state.raytrace_output() != RAYTRACE_ACROSPIN)    /* Output the vertex info */
		{
			out_triangle(*f_cur, s_f_last_row[col], s_f_last_row[next],
					cur->color, s_last_row[col].color, s_last_row[next].color);
		}
		triangle_was_output = true;

		driver_draw_line(s_last_row[col].x, s_last_row[col].y, cur->x, cur->y,
			cur->color);
		driver_draw_line(s_last_row[next].x, s_last_row[next].y, cur->x, cur->y,
			cur->color);
		driver_draw_line(s_last_row[next].x, s_last_row[next].y, s_last_row[col].x,
			s_last_row[col].y, s_last_row[col].color);
		s_num_tris++;
	}

	if (g_3d_state.raytrace_output() == RAYTRACE_ACROSPIN)       /* Output vertex info for Acrospin */
	{
		s_acrospin.line3d_raytrace(f_cur);
	}
}

static void line3d_fill_surface_grid(int col, const point *old, const point *cur)
{
	if (col
		&& old->x > BAD_CHECK
		&& old->x < (g_x_dots - BAD_CHECK))
	{
		driver_draw_line(old->x, old->y, cur->x, cur->y, cur->color);
	}
	if (g_current_row
		&& bad_point(s_last_row[col])
		&& bad_point_size(s_last_row[col]))
	{
		driver_draw_line(s_last_row[col].x, s_last_row[col].y, cur->x,
			cur->y, cur->color);
	}
}

static void line3d_fill_points(const point *cur)
{
	(*g_plot_color)(cur->x, cur->y, cur->color);
}

/* connect-a-dot */
static void line3d_fill_wire_frame(int col, const point *old, const point *cur)
{
	if ((old->x < g_x_dots) && col && bad_point(*old))      /* Don't draw from old to cur on col 0 */
	{
		driver_draw_line(old->x, old->y, cur->x, cur->y, cur->color);
	}
}

static void line3d_fill_gouraud_flat(int col, int next, int last_dot, const point *old, const point *cur, const point *old_last)
{
	/*************************************************************/
	/* "triangle fill" - consider four points: current point,    */
	/* previous point same row, point opposite current point in  */
	/* previous row, point after current point in previous row.  */
	/* The object is to fill all points inside the two triangles.*/
	/*                                                           */
	/* s_last_row[col].x/y___ s_last_row[next]                         */
	/* /        1                 /                              */
	/* /                1         /                              */
	/* /                       1  /                              */
	/* oldrow/col ________ trow/col                              */
	/*************************************************************/
	if (g_current_row && !col)
	{
		put_a_triangle(s_last_row[next], s_last_row[col], *cur, cur->color);
	}
	if (g_current_row && col)  /* skip first row and first column */
	{
		if (col == 1)
		{
			put_a_triangle(s_last_row[col], *old_last, *old, old->color);
		}
		if (col < last_dot)
		{
			put_a_triangle(s_last_row[next], s_last_row[col], *cur, cur->color);
		}
		put_a_triangle(*old, s_last_row[col], *cur, cur->color);
	}
}

static void line3d_fill_bars(int col,
	point *old, point *cur, point_fp *f_cur,
	VECTOR_L lv, VECTOR_L lv0)
{
	if (g_3d_state.sphere())
	{
		if (s_persp)
		{
			old->x = int(s_x_center >> 16);
			old->y = int(s_y_center >> 16);
		}
		else
		{
			old->x = int(s_x_center);
			old->y = int(s_y_center);
		}
	}
	else
	{
		lv[0] = col;
		lv[1] = g_current_row;
		lv[2] = 0;
		/* apply g_fudge bit shift for integer math */
		lv[0] = lv[0] << 16;
		lv[1] = lv[1] << 16;
		/* Since 0, unnecessary lv[2] = lv[2] << 16; */

		if (vmult_perspective_l(lv, s_lm, lv0, lv, s_lview, 16))
		{
			*cur = s_bad;
			*f_cur = s_f_bad;
			return;
		}

		/* Round and g_fudge back to original  */
		old->x = int((lv[0] + 32768L) >> 16);
		old->y = int((lv[1] + 32768L) >> 16);
	}
	old->x = MathUtil::Clamp(old->x, 0, g_x_dots - 1);
	old->y = MathUtil::Clamp(old->y, 0, g_y_dots - 1);
	driver_draw_line(old->x, old->y, cur->x, cur->y, cur->color);
}

static void line3d_fill_light(int col, int next, int last_dot, bool cross_not_init,
							  VECTOR v1, VECTOR v2,
							  const point *old, const point_fp *f_old,
							  point *cur, point_fp *f_cur,
							  VECTOR cross_avg)
{
	/* light-source modulated fill */
	if (g_current_row && col)  /* skip first row and first column */
	{
		if (f_cur->color < BAD_CHECK || f_old->color < BAD_CHECK ||
			s_f_last_row[col].color < BAD_CHECK)
		{
			return;
		}

		v1[0] = f_cur->x - f_old->x;
		v1[1] = f_cur->y - f_old->y;
		v1[2] = f_cur->color - f_old->color;

		v2[0] = s_f_last_row[col].x - f_cur->x;
		v2[1] = s_f_last_row[col].y - f_cur->y;
		v2[2] = s_f_last_row[col].color - f_cur->color;

		cross_product(v1, v2, g_cross);

		/* normalize cross - and check if non-zero */
		if (normalize_vector(g_cross))
		{
			if (g_debug_mode)
			{
				stop_message(STOPMSG_NORMAL, "debug, cur->color=bad");
			}
			f_cur->color = float(s_bad.color);
			cur->color = s_bad.color;
		}
		else
		{
			/* line-wise averaging scheme */
			static VECTOR tmpcross;
			if (g_3d_state.light_avg() > 0)
			{
				if (cross_not_init)
				{
					/* initialize array of old normal vectors */
					cross_avg[0] = g_cross[0];
					cross_avg[1] = g_cross[1];
					cross_avg[2] = g_cross[2];
					cross_not_init = false;
				}
				tmpcross[0] = (cross_avg[0]*g_3d_state.light_avg() + g_cross[0]) /
					(g_3d_state.light_avg() + 1);
				tmpcross[1] = (cross_avg[1]*g_3d_state.light_avg() + g_cross[1]) /
					(g_3d_state.light_avg() + 1);
				tmpcross[2] = (cross_avg[2]*g_3d_state.light_avg() + g_cross[2]) /
					(g_3d_state.light_avg() + 1);
				g_cross[0] = tmpcross[0];
				g_cross[1] = tmpcross[1];
				g_cross[2] = tmpcross[2];
				if (normalize_vector(g_cross))
				{
					/* this shouldn't happen */
					if (g_debug_mode)
					{
						stop_message(STOPMSG_NORMAL, "debug, normal vector err2");
					}
					f_cur->color = float(g_colors);
					cur->color = g_colors;
				}
			}
			cross_avg[0] = tmpcross[0];
			cross_avg[1] = tmpcross[1];
			cross_avg[2] = tmpcross[2];

			/* dot product of unit vectors is cos of angle between */
			/* we will use this value to shade surface */

			cur->color = int(1 + (g_colors - 2)*
				(1.0 - DOT_PRODUCT(g_cross, s_light_direction)));
		}
		/* if colors out of range, set them to min or max color index
		* but avoid background index. This makes colors "opaque" so
		* SOMETHING plots. These conditions shouldn't happen but just
		* in case                                        */
		cur->color = MathUtil::Clamp(cur->color, 1, g_colors - 1);

		/* why "col < 2"? So we have sufficient geometry for the fill */
		/* algorithm, which needs previous point in same row to have  */
		/* already been calculated (variable old)                 */
		/* fix ragged left margin in preview */
		if (col == 1 && g_current_row > 1)
		{
			put_a_triangle(s_last_row[next], s_last_row[col], *cur, cur->color);
		}

		if (col < 2 || g_current_row < 2)       /* don't have valid color yet */
		{
			return;
		}

		if (col < last_dot)
		{
			put_a_triangle(s_last_row[next], s_last_row[col], *cur, cur->color);
		}
		put_a_triangle(*old, s_last_row[col], *cur, cur->color);
		assert(g_plot_color_standard);
		g_plot_color = g_plot_color_standard;
	}
}

static void line3d_fill(int col, int next, int last_dot, bool cross_not_init,
						const point *old_last,
						point *old, point *cur,
						point_fp *f_old, point_fp *f_cur,
						VECTOR_L lv, VECTOR_L lv0, VECTOR v1, VECTOR v2, VECTOR cross_avg)
{
	switch (g_3d_state.fill_type())
	{
	case FillType::SurfaceGrid:
		line3d_fill_surface_grid(col, old, cur);
		break;

	case FillType::Points:
		line3d_fill_points(cur);
		break;
	case FillType::WireFrame:
		line3d_fill_wire_frame(col, old, cur);
		break;
	case FillType::Gouraud:
	case FillType::Flat:
		line3d_fill_gouraud_flat(col, next, last_dot, old, cur, old_last);
		break;
	case FillType::Bars:
		line3d_fill_bars(col, old, cur, f_cur, lv, lv0);
		break;
	case FillType::LightBefore:
	case FillType::LightAfter:
		line3d_fill_light(col, next, last_dot, cross_not_init, 
			v1, v2, old, f_old, cur, f_cur, cross_avg);
		break;
	}
}

int out_line_3d(BYTE *pixels, int line_length)
{
	bool triangle_was_output;		/* triangle has been sent to ray trace file */
	float f_water = 0.0f;			/* transformed WATERLINE for ray trace files */
	int xcenter0 = 0;
	int ycenter0 = 0;				/* Unfudged versions */
	double r;						/* sphere radius */
	int next;						/* used by preview and grid */
	int col;						/* current column (original GIF) */
	point cur;				/* current pixels */
	point old;				/* old pixels */
	point_fp f_cur;
	point_fp f_old;
	VECTOR v;						/* double vector */
	VECTOR v1, v2;
	VECTOR cross_avg;
	bool cross_not_init = false;	/* flag for cross_avg init indication */
	VECTOR_L lv;						/* long equivalent of v */
	VECTOR_L lv0;					/* long equivalent of v */
	int last_dot;
	long g_fudge;
	static point old_last = { 0, 0, 0 }; /* old pixels */

	g_fudge = 1L << 16;
	g_plot_color = (g_3d_state.transparent0() || g_3d_state.transparent1()) ? plot_color_transparent_clip : plot_color_clip;
	s_plot_color_normal = g_plot_color;

	g_current_row = g_row_count;
	/* use separate variable to allow for g_potential_16bit files */
	if (g_potential_16bit)
	{
		g_current_row >>= 1;
	}

	/************************************************************************/
	/* This IF clause is executed ONCE per image. All precalculations are   */
	/* done here, with out any special concern about speed. DANGER -        */
	/* communication with the rest of the program is generally via static   */
	/* or global variables.                                                 */
	/************************************************************************/
	if (g_row_count++ == 0)
	{
		int error = line3d_init(line_length, triangle_was_output, &xcenter0, &ycenter0, cross_avg, v);
		if (error)
		{
			return error;
		}
	}
	/* make sure these pixel coordinates are out of range */
	old = s_bad;
	f_old = s_f_bad;

	/* copies pixels buffer to float type fraction buffer for fill purposes */
	if (g_potential_16bit)
	{
		if (set_pixel_buff(pixels, s_fraction, line_length))
		{
			return 0;
		}
	}
	else if (g_grayscale_depth)           /* convert color numbers to grayscale values */
	{
		for (col = 0; col < int(line_length); col++)
		{
			int color_num = pixels[col];
			/* TODO: the following does not work when COLOR_CHANNEL_MAX != 63 */
			/* effectively (30*R + 59*G + 11*B)/100 scaled 0 to 255 */
			int pal = (int(g_dac_box[color_num][0])*77 +
					int(g_dac_box[color_num][1])*151 +
					int(g_dac_box[color_num][2])*28);

			pal >>= 6;
			pixels[col] = (BYTE) pal;
		}
	}
	cross_not_init = true;
	col = 0;

	s_acrospin.reset_columns();

	/*************************************************************************/
	/* This section of code allows the operation of a preview mode when the  */
	/* preview flag is set. Enabled, it allows the drawing of only the first */
	/* line of the source image, then every 10th line, until and including   */
	/* the last line. For the undrawn lines, only necessary calculations are */
	/* made. As a bonus, in non-sphere mode a box is drawn to help visualize */
	/* the effects of 3D transformations. Thanks to Marc Reinig for this idea*/
	/* and code -- BTW, Marc did NOT put the goto in, but WE did, to avoid   */
	/* copying code here, and to avoid a HUGE "if-then" construct. Besides,  */
	/* we have ALREADY sinned, so why not sin some more?                     */
	/*************************************************************************/
	last_dot = std::min(g_x_dots - 1, int(line_length) - 1);
	if (g_3d_state.fill_type() >= FillType::LightBefore)
	{
		if (g_3d_state.haze() && g_targa_output)
		{
			s_haze_mult = int(g_3d_state.haze()*(float(long(g_y_dots - 1 - g_current_row)*long(g_y_dots - 1 - g_current_row))
									/float(long(g_y_dots - 1)*long(g_y_dots - 1))));
			s_haze_mult = 100 - s_haze_mult;
		}
	}

	if (g_3d_state.preview_factor() >= g_y_dots || g_3d_state.preview_factor() > last_dot)
	{
		g_3d_state.set_preview_factor(std::min(g_y_dots - 1, last_dot));
	}

	s_local_preview_factor = g_y_dots/g_3d_state.preview_factor();

	triangle_was_output = false;
	/* Insure last line is drawn in preview and filltypes <0  */
	if ((g_3d_state.raytrace_output() || g_3d_state.preview() || g_3d_state.fill_type() < FillType::Points)
		&& (g_current_row != g_y_dots - 1)
		&& (g_current_row % s_local_preview_factor) /* Draw mod preview lines */
		&& !(!g_3d_state.raytrace_output() && (g_3d_state.fill_type() > FillType::Bars) && (g_current_row == 1)))
			/* Get init geometry in lightsource modes */
	{
		goto reallythebottom;     /* skip over most of the line3d calcs */
	}
	if (driver_diskp())
	{
		disk_video_status(1, str(boost::format("mapping to 3d, reading line %d") % g_current_row));
	}

	if (!col && g_3d_state.raytrace_output() && g_current_row != 0)
	{
		start_object();
	}
	/* PROCESS ROW LOOP BEGINS HERE */
	while (col < int(line_length))
	{
		if ((g_3d_state.raytrace_output() || g_3d_state.preview() || g_3d_state.fill_type() < FillType::Points)
			&& (col != last_dot) /* if this is not the last col */
			&&  (col % int(s_aspect*s_local_preview_factor)) /* if not the 1st or mod factor col */
			&& (!(!g_3d_state.raytrace_output() && g_3d_state.fill_type() > FillType::Bars && col == 1)))
		{
			goto loopbottom;
		}

		s_real_color = pixels[col];
		cur.color = s_real_color;
		f_cur.color = float(cur.color);

		if (g_3d_state.raytrace_output() || g_3d_state.preview() || g_3d_state.fill_type() < FillType::Points)
		{
			next = int(col + s_aspect*s_local_preview_factor);
			if (next == col)
			{
				next = col + 1;
			}
		}
		else
		{
			next = col + 1;
		}
		if (next >= last_dot)
		{
			next = last_dot;
		}

		if (cur.color > 0 && cur.color < g_3d_state.water_line())
		{
			s_real_color = (BYTE) g_3d_state.water_line();
			cur.color = s_real_color;
			f_cur.color = float(cur.color); /* "lake" */
		}
		else if (g_potential_16bit)
		{
			f_cur.color += float(s_fraction[col])/float(1 << 8);
		}

		if (g_3d_state.sphere())            /* sphere case */
		{
			if (line3d_sphere(col, xcenter0, ycenter0, &cur, &f_cur, &r, lv, v))
			{
				goto loopbottom;
			}
		}
		else
			/* non-sphere 3D */
		{
			if (line3d_planar(col, &f_cur, &cur, lv0, lv, v, &f_water))
			{
				goto loopbottom;
			}
		}

		if (g_3d_state.randomize_colors())
		{
			if (cur.color > g_3d_state.water_line())
			{
				int rnd = rand15() >> 8;     /* 7-bit number */
				rnd = rnd*rnd >> s_rand_factor;  /* n-bit number */

				if (rand() & 1)
				{
					rnd = -rnd;   /* Make +/- n-bit number */
				}

				if (int(cur.color) + rnd >= g_colors)
				{
					cur.color = g_colors - 2;
				}
				else if (int(cur.color) + rnd <= g_3d_state.water_line())
				{
					cur.color = g_3d_state.water_line() + 1;
				}
				else
				{
					cur.color = cur.color + rnd;
				}
				s_real_color = (BYTE)cur.color;
			}
		}

		if (g_3d_state.raytrace_output())
		{
			line3d_raytrace(col, next, &old, &cur, &f_old, &f_cur, f_water, last_dot, triangle_was_output);
			goto loopbottom;
		}

		line3d_fill(col, next, last_dot, cross_not_init,
			&old_last, &old, &cur, &f_old, &f_cur,
			lv, lv0, v1, v2, cross_avg);

loopbottom:
		if (g_3d_state.raytrace_output() || (g_3d_state.fill_type() != FillType::Points && g_3d_state.fill_type() != FillType::Bars))
		{
			/* for triangle and grid fill purposes */
			old_last = s_last_row[col];
			s_last_row[col] = cur;
			old = cur;

			/* for illumination model purposes */
			f_old = f_cur;
			s_f_last_row[col] = f_cur;
			if (g_current_row && g_3d_state.raytrace_output() && col >= last_dot)
				/* if we're at the end of a row, close the object */
			{
				end_object(triangle_was_output);
				triangle_was_output = false;
				if (s_raytrace_file.bad())
				{
					s_raytrace_file.close();
					remove(g_light_name.c_str());
					file_error(g_3d_state.ray_name(), FILEERROR_NO_SPACE);
					return -1;
				}
			}
		}
		col++;
	}                         /* End of while statement for plotting line  */
	s_acrospin.next_row();

reallythebottom:
	/* stuff that HAS to be done, even in preview mode, goes here */
	if (g_3d_state.sphere())
	{
		/* incremental sin/cos phi calc */
		if (g_current_row == 0)
		{
			s_sin_phi = s_old_sin_phi2;
			s_cos_phi = s_old_cos_phi2;
		}
		else
		{
			s_sin_phi = s_two_cos_delta_phi*s_old_sin_phi2 - s_old_sin_phi1;
			s_cos_phi = s_two_cos_delta_phi*s_old_cos_phi2 - s_old_cos_phi1;
			s_old_sin_phi1 = s_old_sin_phi2;
			s_old_sin_phi2 = s_sin_phi;
			s_old_cos_phi1 = s_old_cos_phi2;
			s_old_cos_phi2 = s_cos_phi;
		}
	}
	return 0;                  /* decoder needs to know all is well !!! */
}

/* vector version of line draw */
static void vdraw_line(double *v1, double *v2, int color)
{
	driver_draw_line(int(v1[0]), int(v1[1]), int(v2[0]), int(v2[1]), color);
}

static void corners(MATRIX m, bool show, double *pxmin, double *pymin, double *pzmin, double *pxmax, double *pymax, double *pzmax)
{
	VECTOR S[2][4];              /* Holds the top an bottom points,
								* S[0][]=bottom */

	/* define corners of box fractal is in in x, y, z plane "b" stands for
	* "bottom" - these points are the corners of the screen in the x-y plane.
	* The "t"'s stand for Top - they are the top of the cube where 255 color
	* points hit. */
	*pxmin = int(INT_MAX);
	*pymin = int(INT_MAX);
	*pzmin = int(INT_MAX);
	*pxmax = int(INT_MIN);
	*pymax = int(INT_MIN);
	*pzmax = int(INT_MIN);

	for (int j = 0; j < 4; ++j)
	{
		for (int i = 0; i < 3; i++)
		{
			S[0][j][i] = 0;
			S[1][j][i] = 0;
		}
	}

	S[0][1][0] = g_x_dots - 1;
	S[0][2][0] = g_x_dots - 1;
	S[1][1][0] = g_x_dots - 1;
	S[1][2][0] = g_x_dots - 1;

	S[0][2][1] = g_y_dots - 1;
	S[0][3][1] = g_y_dots - 1;
	S[1][2][1] = g_y_dots - 1;
	S[1][3][1] = g_y_dots - 1;

	S[1][0][2] = s_z_coord - 1;
	S[1][1][2] = s_z_coord - 1;
	S[1][2][2] = s_z_coord - 1;
	S[1][3][2] = s_z_coord - 1;

	for (int i = 0; i < 4; ++i)
	{
		/* transform points */
		vmult(S[0][i], m, S[0][i]);
		vmult(S[1][i], m, S[1][i]);

		/* update minimums and maximums */
		if (S[0][i][0] <= *pxmin)
		{
			*pxmin = S[0][i][0];
		}
		if (S[0][i][0] >= *pxmax)
		{
			*pxmax = S[0][i][0];
		}
		if (S[1][i][0] <= *pxmin)
		{
			*pxmin = S[1][i][0];
		}
		if (S[1][i][0] >= *pxmax)
		{
			*pxmax = S[1][i][0];
		}
		if (S[0][i][1] <= *pymin)
		{
			*pymin = S[0][i][1];
		}
		if (S[0][i][1] >= *pymax)
		{
			*pymax = S[0][i][1];
		}
		if (S[1][i][1] <= *pymin)
		{
			*pymin = S[1][i][1];
		}
		if (S[1][i][1] >= *pymax)
		{
			*pymax = S[1][i][1];
		}
		if (S[0][i][2] <= *pzmin)
		{
			*pzmin = S[0][i][2];
		}
		if (S[0][i][2] >= *pzmax)
		{
			*pzmax = S[0][i][2];
		}
		if (S[1][i][2] <= *pzmin)
		{
			*pzmin = S[1][i][2];
		}
		if (S[1][i][2] >= *pzmax)
		{
			*pzmax = S[1][i][2];
		}
	}

	if (show)
	{
		if (s_persp)
		{
			for (int i = 0; i < 4; i++)
			{
				perspective(S[0][i]);
				perspective(S[1][i]);
			}
		}

		/* Keep the box surrounding the fractal */
		for (int j = 0; j < 2; j++)
		{
			for (int i = 0; i < 4; ++i)
			{
				S[j][i][0] += g_xx_adjust;
				S[j][i][1] += g_yy_adjust;
			}
		}

		draw_rectangle(S[0][0], S[0][1], S[0][2], S[0][3], 2);      /* Bottom */

		draw_rectangle_lines(S[0][0], S[1][0], S[0][1], S[1][1], 5);      /* Sides */
		draw_rectangle_lines(S[0][2], S[1][2], S[0][3], S[1][3], 6);

		draw_rectangle(S[1][0], S[1][1], S[1][2], S[1][3], 8);      /* Top */
	}
}

/* This function draws a vector from origin[] to direct[] and a box
		around it. The vector and box are transformed or not depending on
		g_3d_state.fill_type().
*/

static void draw_light_box(double *origin, double *direct, MATRIX light_m)
{
	VECTOR S[2][4];
	double temp;

	S[1][0][0] = origin[0];
	S[0][0][0] = origin[0];
	S[1][0][1] = origin[1];
	S[0][0][1] = origin[1];

	S[1][0][2] = direct[2];

	for (int i = 0; i < 2; i++)
	{
		S[i][1][0] = S[i][0][0];
		S[i][1][1] = direct[1];
		S[i][1][2] = S[i][0][2];
		S[i][2][0] = direct[0];
		S[i][2][1] = S[i][1][1];
		S[i][2][2] = S[i][0][2];
		S[i][3][0] = S[i][2][0];
		S[i][3][1] = S[i][0][1];
		S[i][3][2] = S[i][0][2];
	}

	/* transform the corners if necessary */
	if (g_3d_state.fill_type() == FillType::LightAfter)
	{
		for (int i = 0; i < 4; i++)
		{
			vmult(S[0][i], light_m, S[0][i]);
			vmult(S[1][i], light_m, S[1][i]);
		}
	}

	/* always use perspective to aid viewing */
	temp = g_view[2];              /* save perspective distance for a later
									* restore */
	g_view[2] = -PERSPECTIVE_DISTANCE*300.0/100.0;

	for (int i = 0; i < 4; i++)
	{
		perspective(S[0][i]);
		perspective(S[1][i]);
	}
	g_view[2] = temp;              /* Restore perspective distance */

	/* Adjust for aspect */
	for (int i = 0; i < 4; i++)
	{
		S[0][i][0] = S[0][i][0]*s_aspect;
		S[1][i][0] = S[1][i][0]*s_aspect;
	}

	/* draw box connecting transformed points. NOTE order and COLORS */
	draw_rectangle(S[0][0], S[0][1], S[0][2], S[0][3], 2);

	vdraw_line(S[0][0], S[1][2], 8);

	/* sides */
	draw_rectangle_lines(S[0][0], S[1][0], S[0][1], S[1][1], 4);
	draw_rectangle_lines(S[0][2], S[1][2], S[0][3], S[1][3], 5);

	draw_rectangle(S[1][0], S[1][1], S[1][2], S[1][3], 3);

	/* Draw the "arrow head" */
	for (int i = -3; i < 4; i++)
	{
		for (int j = -3; j < 4; j++)
		{
			if (abs(i) + abs(j) < 6)
			{
				g_plot_color(int(S[1][2][0] + i), int(S[1][2][1] + j), 10);
			}
		}
	}
}

static void pack_rectangle_vector(VECTOR V[4], VECTOR V0, VECTOR V1, VECTOR V2, VECTOR V3)
{
	for (int i = 0; i < 2; i++)
	{
		V[0][i] = V0[i];
		V[1][i] = V1[i];
		V[2][i] = V2[i];
		V[3][i] = V3[i];
	}
}

static void draw_rectangle(VECTOR V0, VECTOR V1, VECTOR V2, VECTOR V3, int color)
{
	VECTOR V[4];
	pack_rectangle_vector(V, V0, V1, V2, V3);

	for (int i = 0; i < 4; i++)
	{
		if (fabs(V[i][0] - V[(i + 1) % 4][0]) < -2*BAD_CHECK &&
			fabs(V[i][1] - V[(i + 1) % 4][1]) < -2*BAD_CHECK)
		{
			vdraw_line(V[i], V[(i + 1) % 4], color);
		}
	}
}

static void draw_rectangle_lines(VECTOR V0, VECTOR V1, VECTOR V2, VECTOR V3, int color)
{
	VECTOR V[4];
	pack_rectangle_vector(V, V0, V1, V2, V3);
	for (int i = 0; i < 3; i += 2)
	{
		if (fabs(V[i][0] - V[i + 1][0]) < -2*BAD_CHECK &&
			fabs(V[i][1] - V[i + 1][1]) < -2*BAD_CHECK)
		{
			vdraw_line(V[i], V[i + 1], color);
		}
	}
}

/* replacement for plot - builds a table of min and max x's instead of plot */
/* called by draw_line as part of triangle fill routine */
static void plot_color_put_min_max(int x, int y, int color)
{
	color = 0; /* to supress warning only */
	if (y >= 0 && y < g_y_dots)
	{
		if (x < s_minmax_x[y].minx)
		{
			s_minmax_x[y].minx = x;
		}
		if (x > s_minmax_x[y].maxx)
		{
			s_minmax_x[y].maxx = x;
		}
	}
}

/*
		This routine fills in a triangle. Extreme left and right values for
		each row are calculated by calling the line function for the sides.
		Then rows are filled in with horizontal lines
*/
#define MAXOFFSCREEN  2    /* allow two of three points to be off screen */

static void put_a_triangle(point pt1, point pt2, point pt3, int color)
{
	/* Too many points off the screen? */
	if ((off_screen(pt1) + off_screen(pt2) + off_screen(pt3)) > MAXOFFSCREEN)
	{
		return;
	}

	s_p1 = pt1;                    /* needed by interp_color */
	s_p2 = pt2;
	s_p3 = pt3;

	/* fast way if single point or single line */
	if (s_p1.y == s_p2.y && s_p1.x == s_p2.x)
	{
		g_plot_color = s_plot_color_fill;
		if (s_p1.y == s_p3.y && s_p1.x == s_p3.x)
		{
			(*g_plot_color)(s_p1.x, s_p1.y, color);
		}
		else
		{
			driver_draw_line(s_p1.x, s_p1.y, s_p3.x, s_p3.y, color);
		}
		g_plot_color = s_plot_color_normal;
		return;
	}
	else if ((s_p3.y == s_p1.y && s_p3.x == s_p1.x) || (s_p3.y == s_p2.y && s_p3.x == s_p2.x))
	{
		g_plot_color = s_plot_color_fill;
		driver_draw_line(s_p1.x, s_p1.y, s_p2.x, s_p2.y, color);
		g_plot_color = s_plot_color_normal;
		return;
	}

	/* find min max y */
	int miny = s_p1.y;
	int maxy = s_p1.y;
	if (s_p2.y < miny)
	{
		miny = s_p2.y;
	}
	else
	{
		maxy = s_p2.y;
	}
	if (s_p3.y < miny)
	{
		miny = s_p3.y;
	}
	else if (s_p3.y > maxy)
	{
		maxy = s_p3.y;
	}

	/* only worried about values on screen */
	if (miny < 0)
	{
		miny = 0;
	}
	if (maxy >= g_y_dots)
	{
		maxy = g_y_dots - 1;
	}

	for (int y = miny; y <= maxy; y++)
	{
		s_minmax_x[y].minx = int(INT_MAX);
		s_minmax_x[y].maxx = int(INT_MIN);
	}

	/* set plot to "fake" plot function */
	g_plot_color = plot_color_put_min_max;

	/* build table of extreme x's of triangle */
	driver_draw_line(s_p1.x, s_p1.y, s_p2.x, s_p2.y, 0);
	driver_draw_line(s_p2.x, s_p2.y, s_p3.x, s_p3.y, 0);
	driver_draw_line(s_p3.x, s_p3.y, s_p1.x, s_p1.y, 0);

	for (int y = miny; y <= maxy; y++)
	{
		int xlim = s_minmax_x[y].maxx;
		for (int x = s_minmax_x[y].minx; x <= xlim; x++)
		{
			(*s_plot_color_fill)(x, y, color);
		}
	}
	g_plot_color = s_plot_color_normal;
}

static int off_screen(point pt)
{
	if ((pt.x >= 0) && (pt.x < g_x_dots) && (pt.y >= 0) && (pt.y < g_y_dots))
	{
		return 0;      /* point is ok */
	}

	if (abs(pt.x) > -BAD_CHECK || abs(pt.y) > -BAD_CHECK)
	{
		return 99;              /* point is bad */
	}
	return 1;                  /* point is off the screen */
}

static void plot_color_clip(int x, int y, int color)
{
	if (0 <= x && x < g_x_dots &&
		0 <= y && y < g_y_dots &&
		0 <= color && color < g_file_colors)
	{
		assert(g_plot_color_standard);
		(*g_plot_color_standard)(x, y, color);

		if (g_targa_output)
		{
			/* g_plot_color_standard modifies color in these types */
			if (!(g_3d_state.glasses_type() == STEREO_ALTERNATE || g_3d_state.glasses_type() == STEREO_SUPERIMPOSE))
			{
				targa_color(x, y, color);
			}
		}
	}
}

/*********************************************************************/
/* This function is the same as plot_color_clip but checks for color being */
/* in transparent range. Intended to be called only if transparency  */
/* has been enabled.                                                 */
/*********************************************************************/

static void plot_color_transparent_clip(int x, int y, int color)
{
	if (0 <= x && x < g_x_dots &&   /* is the point on screen?  */
		0 <= y && y < g_y_dots &&   /* Yes?  */
		0 <= color && color < g_colors &&  /* Colors in valid range?  */
		/* Lets make sure its not a transparent color  */
		(g_3d_state.transparent0() > color || color > g_3d_state.transparent1()))
	{
		assert(g_plot_color_standard);
		(*g_plot_color_standard)(x, y, color); /* I guess we can plot then  */
		if (g_targa_output)
		{
			/* g_plot_color_standard modifies color in these types */
			if (!(g_3d_state.glasses_type() == STEREO_ALTERNATE || g_3d_state.glasses_type() == STEREO_SUPERIMPOSE))
			{
				targa_color(x, y, color);
			}
		}
	}
}

/************************************************************************/
/* A substitute for plotcolor that interpolates the colors according    */
/* to the x and y values of three points (s_p1, s_p2, s_p3) which are static in */
/* this routine                                                         */
/*                                                                      */
/*      In Light source modes, color is light value, not actual color   */
/*      s_real_color always contains the actual color                     */
/************************************************************************/

static void interp_color(int x, int y, int color)
{
	/* this distance formula is not the usual one - but it has the virtue that
	* it uses ONLY additions (almost) and it DOES go to zero as the points
	* get close. */

	int d1 = abs(s_p1.x - x) + abs(s_p1.y - y);
	int d2 = abs(s_p2.x - x) + abs(s_p2.y - y);
	int d3 = abs(s_p3.x - x) + abs(s_p3.y - y);

	int D = (d1 + d2 + d3) << 1;
	if (D)
	{  /* calculate a weighted average of colors long casts prevent integer
			overflow. This can evaluate to zero */
		color = int((long(d2 + d3)*long(s_p1.color) +
				long(d1 + d3)*long(s_p2.color) +
				long(d1 + d2)*long(s_p3.color))/D);
	}

	if (0 <= x && x < g_x_dots &&
		0 <= y && y < g_y_dots &&
		0 <= color && color < g_colors &&
		(g_3d_state.transparent1() == 0 || int(s_real_color) > g_3d_state.transparent1() ||
			g_3d_state.transparent0() > int(s_real_color)))
	{
		if (g_targa_output)
		{
			/* g_plot_color_standard modifies color in these types */
			if (!(g_3d_state.glasses_type() == STEREO_ALTERNATE || g_3d_state.glasses_type() == STEREO_SUPERIMPOSE))
			{
				D = targa_color(x, y, color);
			}
		}

		if (g_3d_state.fill_type() >= FillType::LightBefore)
		{
			color = (1 + (unsigned) color*s_ambient)/256;
			if (color == 0)
			{
				color = 1;
			}
		}
		assert(g_plot_color_standard);
		(*g_plot_color_standard)(x, y, color);
	}
}

/*
		In non light source modes, both color and s_real_color contain the
		actual pixel color. In light source modes, color contains the
		light value, and s_real_color contains the origninal color

		This routine takes a pixel modifies it for lightshading if appropriate
		and plots it in a Targa file. Used in plot3d.c
*/

int targa_color(int x, int y, int color)
{
	if (g_3d_state.fill_type() == FillType::Gouraud
		|| g_3d_state.glasses_type() == STEREO_ALTERNATE
		|| g_3d_state.glasses_type() == STEREO_SUPERIMPOSE
		|| g_true_color)
	{
		s_real_color = (BYTE) color;       /* So Targa gets interpolated color */
	}

	BYTE rgb[3];
	if (g_true_mode_iterates)
	{
		rgb[0] = BYTE((g_real_color_iter >> 16) & 0xff);  /* red   */
		rgb[1] = BYTE((g_real_color_iter >> 8) & 0xff);  /* green */
		rgb[2] = BYTE((g_real_color_iter) & 0xff);  /* blue  */
	}
	else
	{
		/* TODO: does not work when COLOR_CHANNEL_MAX != 63 */
		rgb[0] = BYTE(g_dac_box[s_real_color][0] << 2); /* Move color space to */
		rgb[1] = BYTE(g_dac_box[s_real_color][1] << 2); /* 256 color primaries */
		rgb[2] = BYTE(g_dac_box[s_real_color][2] << 2); /* from 64 colors */
	}

	/* Now lets convert it to HSV */
	unsigned long hue;
	unsigned long saturation;
	unsigned long value;
	RGBtoHSV(rgb[0], rgb[1], rgb[2], &hue, &saturation, &value);

	/* Modify original saturation and V components */
	if (g_3d_state.fill_type() > FillType::Bars
		&& !(g_3d_state.glasses_type() == STEREO_ALTERNATE
			 || g_3d_state.glasses_type() == STEREO_SUPERIMPOSE))
	{
		/* Adjust for ambient */
		value = (value*(65535L - (unsigned) (color*s_ambient)))/65535L;
	}

	if (g_3d_state.haze())
	{
		/* Haze lowers saturatoin of colors */
		saturation = (unsigned long) (saturation*s_haze_mult)/100;
		if (value >= 32640)           /* Haze reduces contrast */
		{
			value -= 32640;
			value = (unsigned long) ((value*s_haze_mult)/100);
			value += 32640;
		}
		else
		{
			value = 32640 - value;
			value = (unsigned long) ((value*s_haze_mult)/100);
			value = 32640 - value;
		}
	}
	/* Now lets convert it back to RGB. Original Hue, modified Sat and Val */
	HSVtoRGB(&rgb[0], &rgb[1], &rgb[2], hue, saturation, value);

	/* Now write the color triple to its transformed location */
	/* on the disk. */
	disk_write_targa(x + g_sx_offset, y + g_sy_offset, rgb[0], rgb[1], rgb[2]);

	return int(255 - value);
}

static int set_pixel_buff(BYTE *pixels, BYTE *fraction, unsigned linelen)
{
	if ((s_even_odd_row++ & 1) == 0) /* even rows are color value */
	{
		for (int i = 0; i < int(linelen); i++)       /* add the fractional part in odd row */
		{
			fraction[i] = pixels[i];
		}
		return 1;
	}
	else
		/* swap */
	{
		BYTE tmp;
		for (int i = 0; i < int(linelen); i++)       /* swap so pixel has color */
		{
			tmp = pixels[i];
			pixels[i] = fraction[i];
			fraction[i] = tmp;
		}
	}
	return 0;
}

/**************************************************************************

Common routine for printing error messages to the screen for Targa
and other files

**************************************************************************/
static std::ostream &file_error_filename(std::ostream &msgbuf, const std::string &filename)
{
	return msgbuf << " < " << filename << " >";
}
static void file_error(const std::string &filename, int code)
{
	std::ostringstream msgbuf;
	s_file_error = code;
	switch (code)
	{
	case FILEERROR_OPEN:                      /* Can't Open */
		msgbuf << "OOPS, couldn't open " << file_error_filename(msgbuf, filename);
		break;
	case FILEERROR_NO_SPACE:                      /* Not enough room */
		msgbuf << "OOPS, ran out of disk space." << file_error_filename(msgbuf, filename);
		break;
	case FILEERROR_BAD_IMAGE_SIZE:                      /* Image wrong size */
		msgbuf << "OOPS, image wrong size\n";
		break;
	case FILEERROR_BAD_FILE_TYPE:                      /* Wrong file type */
		msgbuf << "OOPS, can't handle this type of file.\n";
		break;
	}
	msgbuf << std::ends;
	stop_message(STOPMSG_NORMAL, msgbuf.str());
}


/* This function opens a TARGA_24 file for reading and writing. If
 * its a new file, (overlay == 0) it writes a header. If it is to
 * overlay an existing file (overlay == 1) it copies the original
 * header whose lenght and validity was determined in
 * Targa_validate.
 *
 * It Verifies there is enough disk space, and leaves the file
 * at the start of the display data area.
 */
int start_disk_targa(const std::string &file_name2, FILE *Source, bool overlay_file)
{
	/* Open File for both reading and writing */
	FILE *fps = dir_fopen(g_work_dir, file_name2, "wb");
	if (fps == 0)
	{
		file_error(file_name2, FILEERROR_OPEN);
		return -1;              /* Oops, somethings wrong! */
	}

	int inc = 1;                     /* Assume we are overlaying a file */

	/* Write the header */
	if (overlay_file)
	{
		for (int i = 0; i < s_targa_header_len; i++) /* Copy the header from the Source */
		{
			fputc(fgetc(Source), fps);
		}
	}
	else
	{                            /* Write header for a new file */
		/* ID field size = 0, No color map, Targa type 2 file */
		for (int i = 0; i < 12; i++)
		{
			if (i == 0 && g_true_color)
			{
				set_upr_lwr();
				fputc(4, fps); /* make room to write an extra number */
				s_targa_header_len = 18 + 4;
			}
			else if (i == 2)
			{
				fputc(i, fps);
			}
			else
			{
				fputc(0, fps);
			}
		}
		/* Write image size  */
		for (int i = 0; i < 4; i++)
		{
			fputc(s_targa_size[i], fps);
		}
		fputc(TARGA_24, fps);          /* Targa 24 file */
		fputc(TARGA_32, fps);          /* Image at upper left */
		inc = 3;
	}

	if (g_true_color) /* write maxit */
	{
		fputc(BYTE(g_max_iteration       & 0xff), fps);
		fputc(BYTE((g_max_iteration >> 8) & 0xff), fps);
		fputc(BYTE((g_max_iteration >> 16) & 0xff), fps);
		fputc(BYTE((g_max_iteration >> 24) & 0xff), fps);
	}

	/* Finished with the header, now lets work on the display area  */
	for (int i = 0; i < g_y_dots; i++)  /* "clear the screen" (write to the disk) */
	{
		for (int j = 0; j < s_line_length; j += inc)
		{
			if (overlay_file)
			{
				fputc(fgetc(Source), fps);
			}
			else
			{
				/* Targa order (B, G, R) */
				fputc(g_3d_state.background_blue(), fps);
				fputc(g_3d_state.background_green(), fps);
				fputc(g_3d_state.background_red(), fps);
			}
		}
		if (ferror(fps))
		{
			/* Almost certainly not enough disk space  */
			fclose(fps);
			if (overlay_file)
			{
				fclose(Source);
			}
			dir_remove(g_work_dir, file_name2);
			file_error(file_name2, FILEERROR_NO_SPACE);
			return -2;
		}
		if (driver_key_pressed())
		{
			return -3;
		}
	}

	if (disk_start_targa(fps, s_targa_header_len) != 0)
	{
		disk_end();
		dir_remove(g_work_dir, file_name2);
		return -4;
	}
	return 0;
}

int start_disk_targa(const std::string &file_name2, std::ifstream &source, bool overlay_file)
{
	/* Open File for both reading and writing */
	FILE *fps = dir_fopen(g_work_dir, file_name2, "wb");
	if (fps == 0)
	{
		file_error(file_name2, FILEERROR_OPEN);
		return -1;              /* Oops, somethings wrong! */
	}

	int inc = 1;                     /* Assume we are overlaying a file */

	/* Write the header */
	if (overlay_file)
	{
		for (int i = 0; i < s_targa_header_len; i++) /* Copy the header from the Source */
		{
			fputc(source.get(), fps);
		}
	}
	else
	{                            /* Write header for a new file */
		/* ID field size = 0, No color map, Targa type 2 file */
		for (int i = 0; i < 12; i++)
		{
			if (i == 0 && g_true_color)
			{
				set_upr_lwr();
				fputc(4, fps); /* make room to write an extra number */
				s_targa_header_len = 18 + 4;
			}
			else if (i == 2)
			{
				fputc(i, fps);
			}
			else
			{
				fputc(0, fps);
			}
		}
		/* Write image size  */
		for (int i = 0; i < 4; i++)
		{
			fputc(s_targa_size[i], fps);
		}
		fputc(TARGA_24, fps);          /* Targa 24 file */
		fputc(TARGA_32, fps);          /* Image at upper left */
		inc = 3;
	}

	if (g_true_color) /* write maxit */
	{
		fputc(BYTE(g_max_iteration       & 0xff), fps);
		fputc(BYTE((g_max_iteration >> 8) & 0xff), fps);
		fputc(BYTE((g_max_iteration >> 16) & 0xff), fps);
		fputc(BYTE((g_max_iteration >> 24) & 0xff), fps);
	}

	/* Finished with the header, now lets work on the display area  */
	for (int i = 0; i < g_y_dots; i++)  /* "clear the screen" (write to the disk) */
	{
		for (int j = 0; j < s_line_length; j += inc)
		{
			if (overlay_file)
			{
				fputc(source.get(), fps);
			}
			else
			{
				/* Targa order (B, G, R) */
				fputc(g_3d_state.background_blue(), fps);
				fputc(g_3d_state.background_green(), fps);
				fputc(g_3d_state.background_red(), fps);
			}
		}
		if (ferror(fps))
		{
			/* Almost certainly not enough disk space  */
			fclose(fps);
			if (overlay_file)
			{
				source.close();
			}
			dir_remove(g_work_dir, file_name2);
			file_error(file_name2, FILEERROR_NO_SPACE);
			return -2;
		}
		if (driver_key_pressed())
		{
			return -3;
		}
	}

	if (disk_start_targa(fps, s_targa_header_len) != 0)
	{
		disk_end();
		dir_remove(g_work_dir, file_name2);
		return -4;
	}
	return 0;
}

static int targa_validate(const std::string &file_name)
{
	/* Attempt to open source file for reading */
	FILE *fp = dir_fopen(g_work_dir, file_name, "rb");
	if (fp == 0)
	{
		file_error(file_name, FILEERROR_OPEN);
		return -1;              /* Oops, file does not exist */
	}

	s_targa_header_len += fgetc(fp);    /* Check ID field and adjust header size */

	if (fgetc(fp))               /* Make sure this is an unmapped file */
	{
		file_error(file_name, FILEERROR_BAD_FILE_TYPE);
		return -1;
	}

	if (fgetc(fp) != 2)          /* Make sure it is a type 2 file */
	{
		file_error(file_name, FILEERROR_BAD_FILE_TYPE);
		return -1;
	}

	/* Skip color map specification */
	for (int i = 0; i < 5; i++)
	{
		fgetc(fp);
	}

	for (int i = 0; i < 4; i++)
	{
		/* Check image origin */
		fgetc(fp);
	}
	/* Check Image specs */
	for (int i = 0; i < 4; i++)
	{
		if (fgetc(fp) != int(s_targa_size[i]))
		{
			file_error(file_name, FILEERROR_BAD_IMAGE_SIZE);
			return -1;
		}
	}

	if ((fgetc(fp) != int(TARGA_24))			/* Is it a targa 24 file? */
		|| (fgetc(fp) != int(TARGA_32)))		/* Is the origin at the upper left? */
	{
		file_error(file_name, FILEERROR_BAD_FILE_TYPE);
		return -1;
	}
	rewind(fp);

	/* Now that we know its a good file, create a working copy */
	if (start_disk_targa(s_targa_temp, fp, true))
	{
		return -1;
	}

	fclose(fp);                  /* Close the source */

	/* Original file successfully copied to s_targa_temp */
	s_targa_safe = true;
	return 0;
}

static unsigned long color_distance(BYTE color, unsigned long value, unsigned long denominator)
{
	return (((value - color)*60) << 6)/denominator;
}

static int RGBtoHSV(BYTE red, BYTE green, BYTE blue,
					unsigned long *hue, unsigned long *saturation, unsigned long *value)
{
	*value = red;
	BYTE smallest = green;
	if (red < green)
	{
		*value = green;
		smallest = red;
		if (green < blue)
		{
			*value = blue;
		}
		if (blue < red)
		{
			smallest = blue;
		}
	}
	else
	{
		if (blue < green)
		{
			smallest = blue;
		}
		if (red < blue)
		{
			*value = blue;
		}
	}
	unsigned long denominator = *value - smallest;
	if (*value != 0 && denominator != 0)
	{
		*saturation = ((denominator << 16)/(*value)) - 1;
	}
	else
	{
		*saturation = 0;      /* Color is black! and Sat has no meaning */
	}
	if (*saturation == 0)    /* red = G=blue => shade of grey and Hue has no meaning */
	{
		*hue = 0;
		*value = *value << 8;
		return 1;               /* v or s or both are 0 */
	}
	if (*value == smallest)
	{
		*hue = 0;
		*value = *value << 8;
		return 0;
	}
	unsigned long red_distance = color_distance(red, *value, denominator);
	unsigned long green_distance = color_distance(green, *value, denominator);
	unsigned long blue_distance = color_distance(blue, *value, denominator);
	if (*value == red)
	{
		*hue = (smallest == green) ? (300 << 6) + blue_distance : (60 << 6) - green_distance;
	}
	if (*value == green)
	{
		*hue = (smallest == blue) ? (60 << 6) + red_distance : (180 << 6) - blue_distance;
	}
	if (*value == blue)
	{
		*hue = (smallest == red) ? (180 << 6) + green_distance : (300 << 6) - red_distance;
	}
	*value <<= 8;
	return 0;
}

static int HSVtoRGB(BYTE *red, BYTE *green, BYTE *blue, unsigned long hue, unsigned long saturation, unsigned long value)
{
	if (hue >= 23040)
	{
		hue %= 23040;            /* Makes h circular  */
	}

	int I = int(hue/3840);
	int RMD = int(hue % 3840);      /* RMD = fractional part of hue    */

	unsigned long P1 = ((value*(65535L - saturation))/65280L) >> 8;
	unsigned long P2 = (((value*(65535L - (saturation*RMD)/3840))/65280L) - 1) >> 8;
	unsigned long P3 = (((value*(65535L - (saturation*(3840 - RMD))/3840))/65280L)) >> 8;
	value >>= 8;
	switch (I)
	{
	case 0:
		*red = (BYTE) value;
		*green = (BYTE) P3;
		*blue = (BYTE) P1;
		break;
	case 1:
		*red = (BYTE) P2;
		*green = (BYTE) value;
		*blue = (BYTE) P1;
		break;
	case 2:
		*red = (BYTE) P1;
		*green = (BYTE) value;
		*blue = (BYTE) P3;
		break;
	case 3:
		*red = (BYTE) P1;
		*green = (BYTE) P2;
		*blue = (BYTE) value;
		break;
	case 4:
		*red = (BYTE) P3;
		*green = (BYTE) P1;
		*blue = (BYTE) value;
		break;
	case 5:
		*red = (BYTE) value;
		*green = (BYTE) P1;
		*blue = (BYTE) P2;
		break;
	}
	return 0;
}

static void dxf_header()
{
	s_raytrace_file <<
		"  0\n"
		"SECTION\n"
		"  2\n"
		"TABLES\n"
		"  0\n"
		"TABLE\n"
		"  2\n"
		"LAYER\n"
		" 70\n"
		"     2\n"
		"  0\n"
		"LAYER\n"
		"  2\n"
		"0\n"
		" 70\n"
		"     0\n"
		" 62\n"
		"     7\n"
		"  6\n"
		"CONTINUOUS\n"
		"  0\n"
		"LAYER\n"
		"  2\n"
		"FRACTAL\n"
		" 70\n"
		"    64\n"
		" 62\n"
		"     1\n"
		"  6\n"
		"CONTINUOUS\n"
		"  0\n"
		"ENDTAB\n"
		"  0\n"
		"ENDSEC\n"
		"  0\n"
		"SECTION\n"
		"  2\n"
		"ENTITIES\n";
}
/***************************************************************************/
/*                                                                         */
/* EB & DG fiddled with outputs for Rayshade so they work. with v4.x.      */
/* EB == eli brandt.     ebrandt@jarthur.claremont.edu                     */
/* DG == dan goldwater.  daniel_goldwater@brown.edu & dgold@math.umass.edu */
/*  (NOTE: all the stuff we fiddled with is commented with "EB & DG")     */
/* general raytracing code info/notes:                                     */
/*                                                                         */
/*  ray == 0 means no raytracer output  ray == 7 is for dxf                */
/*  ray == 1 is for dkb/pov             ray == 4 is for mtv                */
/*  ray == 2 is for vivid               ray == 5 is for rayshade           */
/*  ray == 3 is for raw                 ray == 6 is for acrospin           */
/*                                                                         */
/*  rayshade needs counterclockwise triangles.  raytracers that support    */
/*  the 'heightfield' primitive include rayshade and pov.  anyone want to  */
/*  write code to make heightfields?  they are *MUCH* faster to trace than */
/*  triangles when doing landscapes...                                     */
/*                                                                         */
/*  stuff EB & DG changed:                                                 */
/*  made the rayshade output create a "grid" aggregate object (one of      */
/*  rayshade's primitives), instead  of a global grid.  as a result, the   */
/*  grid can be optimized based on the number of triangles.                */
/*  the z component of the grid can always be 1 since the surface formed   */
/*  by the triangles is flat                                               */
/*  (ie, it doesnt curve over itself).  this is a major optimization.      */
/*  the x and y grid size is also optimized for a 4:3 aspect ratio image,  */
/*  to get the fewest possible traingles in each grid square.              */
/*  also, we fixed the rayshade code so it actually produces output that   */
/*  works with rayshade.                                                   */
/*  (maybe the old code was for a really old version of rayshade?).        */
/*                                                                         */
/***************************************************************************/

/*
 * This routine writes a header to a ray tracer data file. It
 * identifies the version of Iterated Dynamics which created it and the
 * key 3D parameters in effect at the time.
 */
static int raytrace_header()
{
	/* Open the ray tracing output file */
	g_3d_state.next_ray_name();
	s_raytrace_file.open(g_3d_state.ray_name());
	if (!s_raytrace_file.is_open())
	{
		return -1;              /* Oops, somethings wrong! */
	}

	switch (g_3d_state.raytrace_output())
	{
	case RAYTRACE_VIVID:
		s_raytrace_file << "//"
			<< boost::format("{ Created by Iterated Dynamics Ver. %#4.2f }\n\n") % (g_release/100.0);
		if (g_3d_state.raytrace_brief())
		{
			s_raytrace_file << "surf={diff=0.8 0.4 0.1;}\n";
		}
		s_raytrace_file << "\n";
		break;

	case RAYTRACE_MTV:
		s_raytrace_file << "#"
			<< boost::format("{ Created by Iterated Dynamics Ver. %#4.2f }\n\n") % (g_release/100.0);
		if (g_3d_state.raytrace_brief())
		{
			s_raytrace_file << "f 0.8 0.4 0.1 0.95 0.05 5 0 0\n";
		}
		s_raytrace_file << "\n";
		break;

	case RAYTRACE_RAYSHADE:
		s_raytrace_file << "/*\n"
			<< boost::format("{ Created by Iterated Dynamics Ver. %#4.2f }\n\n") % (g_release/100.0)
			<< "*/\n";
		if (g_3d_state.raytrace_brief())
		{
			s_raytrace_file << "applysurf diffuse 0.8 0.4 0.1\n";
		}
		/* open "grid" opject, a speedy way to do aggregates in rayshade */
		s_raytrace_file <<
			"\n"
			"/* make a gridded aggregate. this size grid is fast for landscapes. */\n"
			"/* make z grid = 1 always for landscapes. */\n\n"
			"grid 33 25 1\n";
		break;

	case RAYTRACE_ACROSPIN:
		s_raytrace_file
			<< "--"
			<< boost::format("{ Created by Iterated Dynamics Ver. %#4.2f }\n\n") % (g_release/100.0)
			<< "\n"
			"Set Layer 1\nSet Color 2\nEndpointList X Y Z Name\n";
		break;

	case RAYTRACE_DXF:
		dxf_header();
		break;

	case RAYTRACE_POVRAY:
		/* Set the default color */
		s_raytrace_file << "DECLARE       F_Dflt = COLOR  RED 0.8 GREEN 0.4 BLUE 0.1\n"
			"\n";
		break;
	}

	return 0;
}


static void out_triangle_povray(float c[3], float pt_t[3][3])
{
	s_raytrace_file << " OBJECT\n"
		"  TRIANGLE ";
	for (int i = 0; i <= 2; i++)     /* Describe each  Vertex  */
	{
		s_raytrace_file << "\n"
			"      <";

		for (int j = 0; j <= 2; j++)
		{
			s_raytrace_file << boost::format("% #4.4f ") % pt_t[2 - i][j];     /* Left handed */
		}

		s_raytrace_file << ">";
	}
	s_raytrace_file << " END_TRIANGLE \n";
	if (!g_3d_state.raytrace_brief())
	{
		s_raytrace_file << boost::format("  TEXTURE\n"
			"   COLOR  RED% #4.4f GREEN% #4.4f BLUE% #4.4f\n"
			"      AMBIENT 0.25 DIFFUSE 0.75 END_TEXTURE\n")
			% c[0] % c[1] % c[2];
	}
	s_raytrace_file << "  COLOR  F_Dflt  END_OBJECT";
	triangle_bounds(pt_t);    /* update bounding info */
	s_raytrace_file << "\n";
}
static void out_triangle_vivid(float c[3], float pt_t[3][3])
{
	if (!g_3d_state.raytrace_brief())
	{
		s_raytrace_file << "surf={diff=";
		for (int i = 0; i <= 2; i++)
		{
			s_raytrace_file << boost::format("% #4.4f ") % c[i];
		}
		s_raytrace_file << ";}\n";
	}
	s_raytrace_file << "polygon={points=3;";
	for (int i = 0; i <= 2; i++)     /* Describe each  Vertex  */
	{
		s_raytrace_file << "\n"
			" vertex =  ";

		for (int j = 0; j <= 2; j++)
		{
			s_raytrace_file << boost::format("% #4.4f ") % pt_t[i][j]; /* Right handed */
		}
		s_raytrace_file << ";";
	}
	s_raytrace_file << "}";
	s_raytrace_file << "\n";
}
static void out_triangle_mtv(float c[3], float pt_t[3][3])
{
	if (!g_3d_state.raytrace_brief())
	{
		s_raytrace_file << "f";
		for (int i = 0; i <= 2; i++)
		{
			s_raytrace_file << boost::format("% #4.4f ") % c[i];
		}
		s_raytrace_file << "0.95 0.05 5 0 0\n";
	}
	s_raytrace_file << "p 3";
	for (int i = 0; i <= 2; i++)     /* Describe each  Vertex  */
	{
		s_raytrace_file << "\n";
		s_raytrace_file << " ";

		for (int j = 0; j <= 2; j++)
		{
			s_raytrace_file << boost::format("% #4.4f ") % pt_t[2 - i][j];     /* Left handed */
		}
	}
	s_raytrace_file << "\n";
}
static void out_triangle_rayshade(float c[3], float pt_t[3][3])
{
	if (!g_3d_state.raytrace_brief())
	{
		s_raytrace_file << "applysurf diffuse ";
		for (int i = 0; i <= 2; i++)
		{
			s_raytrace_file << boost::format("% #4.4f ") % c[i];
		}
		s_raytrace_file << "\n";
	}
	s_raytrace_file << "triangle";
	for (int i = 0; i <= 2; i++)     /* Describe each  Vertex  */
	{
		s_raytrace_file << "\n"
			" ";

		for (int j = 0; j <= 2; j++)
		{
			s_raytrace_file << boost::format("% #4.4f ") % pt_t[2 - i][j];     /* Left handed */
		}
	}
	s_raytrace_file << "\n";
}
static void out_triangle_raw(float c[3], float pt_t[3][3])
{
	if (!g_3d_state.raytrace_brief())
	{
		for (int i = 0; i <= 2; i++)
		{
			s_raytrace_file << boost::format("% #4.4f ") % c[i];
		}
	}
	for (int i = 0; i <= 2; i++)     /* Describe each  Vertex  */
	{
		s_raytrace_file << "\n";

		for (int j = 0; j <= 2; j++)
		{
			s_raytrace_file << boost::format("% #4.4f ") % pt_t[i][j]; /* Right handed */
		}
	}
	if (!g_3d_state.raytrace_brief())
	{
		s_raytrace_file << "\n";
	}
	s_raytrace_file << "\n";
}
static void out_triangle_acrospin(float c[3], float pt_t[3][3])
{
	if (!g_3d_state.raytrace_brief())
	{
		for (int i = 0; i <= 2; i++)
		{
			s_raytrace_file << boost::format("% #4.4f ") % c[i];
		}
	}
	for (int i = 0; i <= 2; i++)     /* Describe each  Vertex  */
	{
		s_raytrace_file << "\n"
			" ";

		for (int j = 0; j <= 2; j++)
		{
			s_raytrace_file << boost::format("% #4.4f ") % pt_t[i][j]; /* Right handed */
		}
	}
	s_raytrace_file << "\n";
}
static void out_triangle_dxf(int c1, float pt_t[3][3])
{
	s_raytrace_file << boost::format("  0\n"
		"3DFACE\n"
		"  8\n"
		"FRACTAL\n"
		" 62\n"
		"%3d\n") % std::min(255, std::max(1, c1));
	for (int i = 0; i <= 2; i++)     /* Describe each  Vertex  */
	{
		for (int j = 0; j <= 2; j++)
		{
			/* write 3dface entity to dxf file */
			s_raytrace_file << boost::format("%3d\n%g\n") % (10*(j + 1) + i) % pt_t[i][j];
			if (i == 2)         /* 3dface needs 4 vertices */
			{
				s_raytrace_file << boost::format("%3d\n%g\n") % (10*(j + 1) + i + 1) % pt_t[i][j];
			}
		}
	}
}
/********************************************************************/
/*                                                                  */
/*  This routine describes the triangle to the ray tracer, it       */
/*  sets the color of the triangle to the average of the color      */
/*  of its verticies and sets the light parameters to arbitrary     */
/*  values.                                                         */
/*                                                                  */
/*  Note: g_num_colors (number of colors in the source              */
/*  file) is used instead of g_colors (number of colors avail. with */
/*  display) so you can generate ray trace files with your LCD      */
/*  or monochrome display                                           */
/*                                                                  */
/********************************************************************/

static int out_triangle(const point_fp &pt1, const point_fp &pt2, const point_fp &pt3,
						int c1, int c2, int c3)
{
	float c[3];
	float pt_t[3][3];

	/* Normalize each vertex to screen size and adjust coordinate system */
	pt_t[0][0] = 2*pt1.x/g_x_dots - 1;
	pt_t[0][1] = (2*pt1.y/g_y_dots - 1);
	pt_t[0][2] = -2*pt1.color/g_num_colors - 1;
	pt_t[1][0] = 2*pt2.x/g_x_dots - 1;
	pt_t[1][1] = (2*pt2.y/g_y_dots - 1);
	pt_t[1][2] = -2*pt2.color/g_num_colors - 1;
	pt_t[2][0] = 2*pt3.x/g_x_dots - 1;
	pt_t[2][1] = (2*pt3.y/g_y_dots - 1);
	pt_t[2][2] = -2*pt3.color/g_num_colors - 1;

	/* Color of triangle is average of colors of its verticies */
	if (!g_3d_state.raytrace_brief())
	{
		for (int i = 0; i <= 2; i++)
		{
			c[i] = float(g_dac_box[c1][i] + g_dac_box[c2][i] + g_dac_box[c3][i])
				/(3*COLOR_CHANNEL_MAX);
		}
	}

	/* get rid of degenerate triangles: any two points equal */
	if ((pt_t[0][0] == pt_t[1][0] &&
			pt_t[0][1] == pt_t[1][1] &&
			pt_t[0][2] == pt_t[1][2]) ||
		(pt_t[0][0] == pt_t[2][0] &&
			pt_t[0][1] == pt_t[2][1] &&
			pt_t[0][2] == pt_t[2][2]) ||
		(pt_t[2][0] == pt_t[1][0] &&
			pt_t[2][1] == pt_t[1][1] &&
			pt_t[2][2] == pt_t[1][2]))
	{
		return 0;
	}

	/* Describe the triangle */
	switch (g_3d_state.raytrace_output())
	{
	case RAYTRACE_POVRAY:
		out_triangle_povray(c, pt_t);
		break;

	case RAYTRACE_VIVID:
		out_triangle_vivid(c, pt_t);
		break;

	case RAYTRACE_MTV:
		out_triangle_mtv(c, pt_t);
		break;

	case RAYTRACE_RAYSHADE:
		out_triangle_rayshade(c, pt_t);
		break;

	case RAYTRACE_RAW:
		out_triangle_raw(c, pt_t);
		break;

	case RAYTRACE_ACROSPIN:
		out_triangle_acrospin(c, pt_t);
		break;

	case RAYTRACE_DXF:
		out_triangle_dxf(c1, pt_t);
		break;
	}

	return 0;
}

/********************************************************************/
/*                                                                  */
/*  This routine calculates the min and max values of a triangle    */
/*  for use in creating ray tracer data files. The values of min    */
/*  and max x, y, and z are assumed to be global.                   */
/*                                                                  */
/********************************************************************/

static void triangle_bounds(float pt_t[3][3])
{
	for (int i = 0; i <= 2; i++)
	{
		for (int j = 0; j <= 2; j++)
		{
			if (pt_t[i][j] < s_min_xyz[j])
			{
				s_min_xyz[j] = pt_t[i][j];
			}
			if (pt_t[i][j] > s_max_xyz[j])
			{
				s_max_xyz[j] = pt_t[i][j];
			}
		}
	}
	return;
}

/********************************************************************/
/*                                                                  */
/*  This routine starts a composite object for ray trace data files */
/*                                                                  */
/********************************************************************/

static int start_object()
{
	if (g_3d_state.raytrace_output() != RAYTRACE_POVRAY)
	{
		return 0;
	}

	/* Reset the min/max values, for bounding box  */
	s_min_xyz[0] = 999999.0f;
	s_min_xyz[1] = 999999.0f;
	s_min_xyz[2] = 999999.0f;

	s_max_xyz[0] = -999999.0f;
	s_max_xyz[1] = -999999.0f;
	s_max_xyz[2] = -999999.0f;

	s_raytrace_file << "COMPOSITE\n";
	return 0;
}

static void end_object_povray(bool triangle_was_output)
{
	if (triangle_was_output)
	{
		/* Make sure the bounding box is slightly larger than the object */
		for (int i = 0; i <= 2; i++)
		{
			if (s_min_xyz[i] == s_max_xyz[i])
			{
				s_min_xyz[i] -= 0.01f;
				s_max_xyz[i] += 0.01f;
			}
			else
			{
				s_min_xyz[i] -= (s_max_xyz[i] - s_min_xyz[i])*0.01f;
				s_max_xyz[i] += (s_max_xyz[i] - s_min_xyz[i])*0.01f;
			}
		}

		/* Add the bounding box info */
		s_raytrace_file << " BOUNDED_BY\n  INTERSECTION\n"
			<< boost::format("   PLANE <-1.0  0.0  0.0 > % #4.3f END_PLANE\n") % (-s_min_xyz[0])
			<< boost::format("   PLANE < 1.0  0.0  0.0 > % #4.3f END_PLANE\n") % s_max_xyz[0]
		<< boost::format("   PLANE < 0.0 -1.0  0.0 > % #4.3f END_PLANE\n") % (-s_min_xyz[1])
			<< boost::format("   PLANE < 0.0  1.0  0.0 > % #4.3f END_PLANE\n") %  s_max_xyz[1]
		<< boost::format("   PLANE < 0.0  0.0 -1.0 > % #4.3f END_PLANE\n") % (-s_min_xyz[2])
			<< boost::format("   PLANE < 0.0  0.0  1.0 > % #4.3f END_PLANE\n") %  s_max_xyz[2]
		<< "  END_INTERSECTION\n END_BOUND\n";
	}

	/* Complete the composite object statement */
	s_raytrace_file << "END_COMPOSITE\n";
	s_raytrace_file << "\n";
}
/********************************************************************/
/*                                                                  */
/*  This routine adds a bounding box for the triangles drawn        */
/*  in the last g_block and completes the composite object created.   */
/*  It uses the globals min and max x, y and z calculated in         */
/*  z calculated in Triangle_Bounds().                              */
/*                                                                  */
/********************************************************************/

static void end_object(bool triangle_was_output)
{
	switch (g_3d_state.raytrace_output())
	{
	case RAYTRACE_POVRAY:
		end_object_povray(triangle_was_output);
		break;

	case RAYTRACE_NONE:
	case RAYTRACE_VIVID:
	case RAYTRACE_RAW:
	case RAYTRACE_MTV:
		s_raytrace_file << "\n";
		break;
	}
}

static void line3d_cleanup()
{
	if (g_3d_state.raytrace_output() == RAYTRACE_NONE || !s_raytrace_file)
	{
		return;
	}

	switch (g_3d_state.raytrace_output())
	{
	case RAYTRACE_POVRAY:
		s_raytrace_file << "\n"
			<< boost::format("{ No. Of Triangles = %ld }*/\n\n") % s_num_tris;
		break;

	case RAYTRACE_VIVID:
		s_raytrace_file << "\n"
			<< "\n\n//"
			<< boost::format("{ No. Of Triangles = %ld }*/\n\n") % s_num_tris;
		break;

	case RAYTRACE_RAW:
		s_raytrace_file << "\n"
			<< boost::format("{ No. Of Triangles = %ld }*/\n\n") % s_num_tris;
		break;

	case RAYTRACE_MTV:
		s_raytrace_file << "\n"
				"\n\n#"
			<< boost::format("{ No. Of Triangles = %ld }*/\n\n") % s_num_tris;
		break;
	case RAYTRACE_RAYSHADE:
		/* end grid aggregate */
		s_raytrace_file <<
				"end\n"
				"\n"
				"/*good landscape:*/\n"
				"screen 640 480\n"
				"eyep 0 2.1 0.8\n"
				"lookp 0 0 -0.95\n"
				"light 1 point -2 1 1.5\n"
				"background .3 0 0\n"
				"report verbose\n"
				"\n"
				"/*"
			<< boost::format("{ No. Of Triangles = %ld }*/\n\n") % s_num_tris;
		break;

	case RAYTRACE_ACROSPIN:
		s_acrospin.line3d_cleanup();
		break;

	case RAYTRACE_DXF:
		s_raytrace_file << "  0\n"
			"ENDSEC\n"
			"  0\n"
			"EOF\n";
		break;
	}

	if (g_targa_output)
	{                            /* Finish up targa files */
		s_targa_header_len = 18;         /* Reset Targa header size */
		disk_end();
		if (!g_debug_mode && (!s_targa_safe || s_file_error) && g_targa_overlay)
		{
			dir_remove(g_work_dir, g_light_name);
			rename(s_targa_temp, g_light_name.c_str());
		}
		if (!g_debug_mode && g_targa_overlay)
		{
			dir_remove(g_work_dir, s_targa_temp);
		}
	}
	s_file_error = FILEERROR_NONE;
	s_targa_safe = false;
}

static void set_upr_lwr()
{
	s_targa_size[0] = BYTE(g_x_dots & 0xff);
	s_targa_size[1] = BYTE(g_x_dots >> 8);
	s_targa_size[2] = BYTE(g_y_dots & 0xff);
	s_targa_size[3] = BYTE(g_y_dots >> 8);
	s_line_length = 3*g_x_dots;    /* line length @ 3 bytes per pixel  */
}

static void initialize_trig_tables(int linelen)
{
	/* Sphere is on side - north pole on right. Top is -90 degrees
	* latitude; bottom 90 degrees */

	/* Map X to this LATITUDE range */
	float theta1 = float(MathUtil::DegreesToRadians(g_3d_state.theta1()));
	float theta2 = float(MathUtil::DegreesToRadians(g_3d_state.theta2()));

	/* Map Y to this LONGITUDE range */
	float phi1 = float(MathUtil::DegreesToRadians(g_3d_state.phi1()));
	float phi2 = float(MathUtil::DegreesToRadians(g_3d_state.phi2()));

	float theta = theta1;

	/*********************************************************************/
	/* Thanks to Hugh Bray for the following idea: when calculating      */
	/* a table of evenly spaced sines or cosines, only a few initial     */
	/* values need be calculated, and the remaining values can be        */
	/* gotten from a derivative of the sine/cosine angle sum formula     */
	/* at the cost of one multiplication and one addition per value!     */
	/*                                                                   */
	/* This idea is applied once here to get a complete table for        */
	/* latitude, and near the bottom of this routine to incrementally    */
	/* calculate longitude.                                              */
	/*                                                                   */
	/* Precalculate 2*cos(deltaangle), sin(start) and sin(start + delta).  */
	/* Then apply recursively:                                           */
	/* sin(angle + 2*delta) = sin(angle + delta)*2cosdelta - sin(angle)    */
	/*                                                                   */
	/* Similarly for cosine. Neat!                                       */
	/*********************************************************************/

	float deltatheta = float(theta2 - theta1)/float(linelen);

	/* initial sin, cos theta */
	s_sin_theta_array[0] = float(sin(double(theta)));
	s_cos_theta_array[0] = float(cos(double(theta)));
	s_sin_theta_array[1] = float(sin(double(theta + deltatheta)));
	s_cos_theta_array[1] = float(cos(double(theta + deltatheta)));

	/* sin, cos delta theta */
	float two_cos_delta_theta = float(2.0*cos(double(deltatheta)));

	/* build table of other sin, cos with trig identity */
	for (int i = 2; i < int(linelen); i++)
	{
		s_sin_theta_array[i] = s_sin_theta_array[i - 1]*two_cos_delta_theta -
			s_sin_theta_array[i - 2];
		s_cos_theta_array[i] = s_cos_theta_array[i - 1]*two_cos_delta_theta -
			s_cos_theta_array[i - 2];
	}

	/* now phi - these calculated as we go - get started here */
	{
		/* increment of latitude, longitude */
		float delta_phi = float(phi2 - phi1)/float(g_height);

		/* initial sin, cos phi */
		s_old_sin_phi1 = float(sin(double(phi1)));
		s_sin_phi = s_old_sin_phi1;
		s_old_cos_phi1 = float(cos(double(phi1)));
		s_cos_phi = s_old_cos_phi1;
		s_old_sin_phi2 = float(sin(double(phi1 + delta_phi)));
		s_old_cos_phi2 = float(cos(double(phi1 + delta_phi)));

		/* sin, cos delta phi */
		s_two_cos_delta_phi = float(2.0*cos(double(delta_phi)));
	}
}
static int first_time(int linelen, VECTOR v)
{
	g_out_line_cleanup = line3d_cleanup;

	g_calculation_time = 0;
	s_even_odd_row = 0;
	/* mark as in-progress */
	g_calculation_status = CALCSTAT_IN_PROGRESS;

	s_ambient = (unsigned int) (255*float(100 - g_3d_state.ambient())/100.0);
	if (s_ambient < 1)
	{
		s_ambient = 1;
	}

	s_num_tris = 0;

	/* Open file for raytrace output and write header */
	if (g_3d_state.raytrace_output())
	{
		raytrace_header();
		g_xx_adjust = 0;
		g_yy_adjust = 0;  /* Disable shifting in ray tracing */
		g_x_shift = 0;
		g_y_shift = 0;
	}

	s_acrospin.first_time();

	set_upr_lwr();
	s_file_error = FILEERROR_NONE;

	if (g_which_image < WHICHIMAGE_BLUE)
	{
		s_targa_safe = false; /* Not safe yet to mess with the source image */
	}

	if (g_targa_output
		&& !((g_3d_state.glasses_type() == STEREO_ALTERNATE || g_3d_state.glasses_type() == STEREO_SUPERIMPOSE)
		&& g_which_image == WHICHIMAGE_BLUE))
	{
		if (g_targa_overlay)
		{
			/* Make sure target file is a supportable Targa File */
			if (targa_validate(g_light_name))
			{
				return -1;
			}
		}
		else
		{
			check_write_file(g_light_name, ".tga");
			if (start_disk_targa(g_light_name, 0, false))   /* Open new file */
			{
				return -1;
			}
		}
	}

	s_rand_factor = 14 - g_3d_state.randomize_colors();

	s_z_coord = g_file_colors;

	if (line_3d_mem())
	{
		return 1;
	}


	/* get scale factors */
	s_scale_x = g_3d_state.x_scale()/100.0;
	s_scale_y = g_3d_state.y_scale()/100.0;
	if (g_3d_state.roughness())
	{
		s_scale_z = -g_3d_state.roughness()/100.0;
	}
	else
	{
		/* if rough=0 make it very flat but plot something */
		s_r_scale = -0.0001;
		s_scale_z = -0.0001;
	}

	/* aspect ratio calculation - assume screen is 4 x 3 */
	s_aspect = double(g_x_dots) *.75/double(g_y_dots);

	MATRIX lightm;
	/* corners of transformed xdotx by ydotx colors box */
	double x_min;
	double y_min;
	double z_min;
	double x_max;
	double y_max;
	double z_max;
	if (g_3d_state.sphere() == false)         /* skip this slow stuff in sphere case */
	{
		/*********************************************************************/
		/* What is done here is to create a single matrix, m, which has      */
		/* scale, rotation, and shift all combined. This allows us to use    */
		/* a single matrix to transform any point. Additionally, we create   */
		/* two perspective vectors.                                          */
		/*                                                                   */
		/* Start with a unit matrix. Add scale and rotation. Then calculate  */
		/* the perspective vectors. Finally add enough translation to center */
		/* the final image plus whatever shift the user has set.             */
		/*********************************************************************/

		/* start with identity */
		identity(s_m);
		identity(lightm);

		/* translate so origin is in center of box, so that when we rotate */
		/* it, we do so through the center */
		trans(double(g_x_dots)/-2.0, double(g_y_dots)/-2.0, double(s_z_coord)/-2.0, s_m);
		trans(double(g_x_dots)/-2.0, double(g_y_dots)/-2.0, double(s_z_coord)/-2.0, lightm);

		/* apply scale factors */
		scale(s_scale_x, s_scale_y, s_scale_z, s_m);
		scale(s_scale_x, s_scale_y, s_scale_z, lightm);

		/* rotation values - converting from degrees to radians */
		double xval = MathUtil::DegreesToRadians(g_3d_state.x_rotation());
		double yval = MathUtil::DegreesToRadians(g_3d_state.y_rotation());
		double zval = MathUtil::DegreesToRadians(g_3d_state.z_rotation());

		if (g_3d_state.raytrace_output())
		{
			xval = 0;
			yval = 0;
			zval = 0;
		}

		xrot(xval, s_m);
		xrot(xval, lightm);
		yrot(yval, s_m);
		yrot(yval, lightm);
		zrot(zval, s_m);
		zrot(zval, lightm);

		/* Find values of translation that make all x, y, z negative */
		/* m current matrix */
		/* 0 means don't show box */
		/* returns minimum and maximum values of x, y, z in fractal */
		corners(s_m, false, &x_min, &y_min, &z_min, &x_max, &y_max, &z_max);
	}

	/* perspective 3D vector - s_lview[2] == 0 means no perspective */

	/* set perspective flag */
	s_persp = false;
	if (g_3d_state.z_viewer() != 0)
	{
		s_persp = true;
		if (g_3d_state.z_viewer() < 80)         /* force float */
		{
			g_user_float_flag = true;
		}
	}

	/* set up view vector, and put viewer in center of screen */
	s_lview[0] = g_x_dots >> 1;
	s_lview[1] = g_y_dots >> 1;

	/* z value of user's eye - should be more negative than extreme negative
	* part of image */
	if (g_3d_state.sphere())                  /* sphere case */
	{
		s_lview[2] = -long(double(g_y_dots)*double(g_3d_state.z_viewer())/100.0);
	}
	else                         /* non-sphere case */
	{
		s_lview[2] = long((z_min - z_max)*double(g_3d_state.z_viewer())/100.0);
	}

	g_view[0] = s_lview[0];
	g_view[1] = s_lview[1];
	g_view[2] = s_lview[2];
	s_lview[0] <<= 16;
	s_lview[1] <<= 16;
	s_lview[2] <<= 16;

	if (!g_3d_state.sphere())
	{
		/* translate back exactly amount we translated earlier plus enough to
		* center image so maximum values are non-positive */
		trans((double(g_x_dots) - x_max - x_min)/2, (double(g_y_dots) - y_max - y_min)/2, -z_max, s_m);

		/* Keep the box centered and on screen regardless of shifts */
		trans((double(g_x_dots) - x_max - x_min)/2, (double(g_y_dots) - y_max - y_min)/2, -z_max, lightm);

		trans(double(g_x_shift), double(-g_y_shift), 0.0, s_m);

		/* matrix s_m now contains ALL those transforms composed together !!
		* convert s_m to long integers shifted 16 bits */
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				s_lm[i][j] = long(s_m[i][j]*65536.0);
			}
		}
	}
	else
	{
		initialize_trig_tables(linelen);

		/* affects how rough planet terrain is */
		if (g_3d_state.roughness())
		{
			s_r_scale = 0.3*g_3d_state.roughness()/100.0;
		}

		/* radius of planet */
		s_radius = double(g_y_dots)/2;

		/* precalculate factor */
		s_r_scale_r = s_radius*s_r_scale;

		s_scale_x = g_3d_state.radius()/100.0;      /* Need x, y, z for g_3d_state.raytrace_output() */
		s_scale_y = s_scale_x;
		s_scale_z = s_scale_x;

		/* adjust x scale factor for aspect */
		s_scale_x *= s_aspect;

		/* precalculation factor used in sphere calc */
		s_radius_factor = s_r_scale*s_radius/double(s_z_coord);

		if (s_persp)                /* precalculate g_fudge factor */
		{
			double radius;
			double zview;
			double angle;

			s_x_center <<= 16;
			s_y_center <<= 16;

			s_radius_factor *= 65536.0;
			s_radius *= 65536.0;

			/* calculate z cutoff factor attempt to prevent out-of-view surfaces
			* from being written */
			zview = -long(double(g_y_dots)*double(g_3d_state.z_viewer())/100.0);
			radius = double(g_y_dots)/2;
			angle = atan(-radius/(zview + radius));
			s_z_cutoff = -radius - sin(angle)*radius;
			s_z_cutoff *= 1.1;        /* for safety */
			s_z_cutoff *= 65536L;
		}
	}

	/* set fill plot function */
	if (g_3d_state.fill_type() != FillType::Flat)
	{
		s_plot_color_fill = interp_color;
	}
	else
	{
		s_plot_color_fill = plot_color_clip;

		/* If transparent colors are set */
		if (g_3d_state.transparent0() || g_3d_state.transparent1())
		{
			s_plot_color_fill = plot_color_transparent_clip; /* Use the transparent plot function  */
		}
	}

	/* Both Sphere and Normal 3D */
	VECTOR direct;
	direct[0] = g_3d_state.x_light();
	direct[1] = -g_3d_state.y_light();
	direct[2] = g_3d_state.z_light();
	s_light_direction[0] = direct[0];
	s_light_direction[1] = direct[1];
	s_light_direction[2] = direct[2];

	/* Needed because s_scale_z = -ROUGH/100 and s_light_direction is transformed in
	* FILLTYPE 6 but not in 5. */
	if (g_3d_state.fill_type() == FillType::LightBefore)
	{
		direct[2] = -g_3d_state.z_light();
		s_light_direction[2] = direct[2];
	}

	if (g_3d_state.fill_type() == FillType::LightAfter)           /* transform light direction */
	{
		/* Think of light direction  as a vector with tail at (0, 0, 0) and head
		* at (s_light_direction). We apply the transformation to BOTH head and
		* tail and take the difference */

		v[0] = 0.0;
		v[1] = 0.0;
		v[2] = 0.0;
		vmult(v, s_m, v);
		vmult(s_light_direction, s_m, s_light_direction);

		for (int i = 0; i < 3; i++)
		{
			s_light_direction[i] -= v[i];
		}
	}
	normalize_vector(s_light_direction);

	if (g_3d_state.preview() && g_3d_state.show_box())
	{
		normalize_vector(direct);

		/* move light vector to be more clear with grey scale maps */
		VECTOR origin;
		origin[0] = (3*g_x_dots)/16;
		origin[1] = (3*g_y_dots)/4;
		if (g_3d_state.fill_type() == FillType::LightAfter)
		{
			origin[1] = (11*g_y_dots)/16;
		}

		origin[2] = 0.0;

		double v_length = std::min(g_x_dots, g_y_dots)/2;
		if (s_persp && g_3d_state.z_viewer() <= PERSPECTIVE_DISTANCE)
		{
			v_length *= long(PERSPECTIVE_DISTANCE + 600)/(long(g_3d_state.z_viewer() + 600)*2);
		}

		/* Set direct[] to point from origin[] in direction of untransformed
		* s_light_direction (direct[]). */
		for (int i = 0; i < 3; i++)
		{
			direct[i] = origin[i] + direct[i]*v_length;
		}

		/* center light box */
		for (int i = 0; i < 2; i++)
		{
			VECTOR tmp;
			tmp[i] = (direct[i] - origin[i])/2;
			origin[i] -= tmp[i];
			direct[i] -= tmp[i];
		}

		/* Draw light source vector and box containing it, draw_light_box will
		* transform them if necessary. */
		draw_light_box(origin, direct, lightm);
		/* draw box around original field of view to help visualize effect of
		* rotations 1 means show box - g_x_min etc. do nothing here */
		if (!g_3d_state.sphere())
		{
			corners(s_m, true, &x_min, &y_min, &z_min, &x_max, &y_max, &z_max);
		}
	}

	/* bad has values caught by clipping */
	s_bad.x = g_bad_value;
	s_bad.y = g_bad_value;
	s_bad.color = g_bad_value;
	s_f_bad.x = float(s_bad.x);
	s_f_bad.y = float(s_bad.y);
	s_f_bad.color = float(s_bad.color);
	for (int i = 0; i < int(linelen); i++)
	{
		s_last_row[i] = s_bad;
		s_f_last_row[i] = s_f_bad;
	}
	g_got_status = GOT_STATUS_3D;
	return 0;
} /* end of once-per-image intializations */

/*
	line_3d_mem

	Allocate buffers needed, depending on the pixel dimensions of the
	vide mode.
*/
static bool line_3d_mem()
{
	/* s_last_row stores the previous row of the original GIF image for
		the purpose of filling in gaps with triangle procedure */
	s_last_row = new point[g_x_dots];
	s_f_last_row = new point_fp[g_y_dots];
	if (!s_last_row || !s_f_last_row)
	{
		return true;
	}

	if (g_3d_state.sphere())
	{
		s_sin_theta_array = new float[g_x_dots];
		s_cos_theta_array = new float[g_x_dots];
		if (!s_sin_theta_array || !s_cos_theta_array)
		{
			return true;
		}
	}

	if (g_potential_16bit)
	{
		s_fraction = new BYTE[g_x_dots];
		if (!s_fraction)
		{
			return true;
		}
	}
	s_minmax_x = 0;

	/* these fill types call put_a_triangle which uses s_minmax_x */
	if (g_3d_state.fill_type() == FillType::Gouraud
		|| g_3d_state.fill_type() == FillType::Flat
		|| g_3d_state.fill_type() == FillType::LightBefore
		|| g_3d_state.fill_type() == FillType::LightAfter)
	{
		/* end of arrays if we use extra segement */
		s_minmax_x = new minmax[g_y_dots];
		if (!s_minmax_x)
		{
			return true;
		}
	}

	/* no errors, got all memroy */
	return false;
}

void line_3d_free()
{
	delete[] s_last_row;
	s_last_row = 0;
	delete[] s_f_last_row;
	s_f_last_row = 0;
	delete[] s_sin_theta_array;
	s_sin_theta_array = 0;
	delete[] s_cos_theta_array;
	s_cos_theta_array = 0;
	delete[] s_fraction;
	s_fraction = 0;
	delete[] s_minmax_x;
	s_minmax_x = 0;
}
