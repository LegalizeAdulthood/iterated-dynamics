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
#include <assert.h>
#include <limits.h>

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "drivers.h"

#define FILEERROR_NONE				0
#define FILEERROR_OPEN				1
#define FILEERROR_NO_SPACE			2
#define FILEERROR_BAD_IMAGE_SIZE	3
#define FILEERROR_BAD_FILE_TYPE		4

#define TARGA_24 24
#define TARGA_32 32

#define PERSPECTIVE_DISTANCE 250		/* Perspective dist used when viewing light vector */
#define BAD_CHECK -3000					/* check values against this to determine if good */

struct point
{
	int x;
	int y;
	int color;
};

struct f_point
{
	float x;
	float y;
	float color;
};

struct minmax
{
	int minx;
	int maxx;
};

/* routines in this module */
int line3d(BYTE *, unsigned);
int _fastcall targa_color(int, int, int);
int startdisk1(char *, FILE *, int);

/* global variables defined here */
void (_fastcall *g_standard_plot)(int x, int y, int color) = NULL;
int g_ambient = 0;
int g_randomize = 0;
int g_haze = 0;
char g_light_name[FILE_MAX_PATH] = "fract001";
int g_targa_overlay = 0;
BYTE g_back_color[3] = { 0, 0, 0 };
char g_ray_name[FILE_MAX_PATH] = "fract001";
int g_preview = 0;
int g_show_box = 0;
int g_preview_factor = 20;
int g_x_adjust = 0;
int g_y_adjust = 0;
int g_xx_adjust = 0;
int g_yy_adjust = 0;
int g_x_shift = 0;
int g_y_shift;
int g_bad_value = -10000; /* set bad values to this */
int g_raytrace_output = RAYTRACE_NONE;        /* Flag to generate Ray trace compatible files in 3d */
int g_raytrace_brief = 0;      /* 1 = short ray trace files */
VECTOR g_view;                /* position of observer for perspective */
VECTOR g_cross;

static int targa_validate(char *);
static int first_time(int, VECTOR);
static int H_R(BYTE *, BYTE *, BYTE *, unsigned long, unsigned long, unsigned long);
static int line_3d_mem(void);
static int R_H(BYTE, BYTE, BYTE, unsigned long *, unsigned long *, unsigned long *);
static int set_pixel_buff(BYTE *, BYTE *, unsigned);
static void set_upr_lwr(void);
static int _fastcall end_object(int);
static int _fastcall off_screen(struct point);
static int _fastcall out_triangle(struct f_point, struct f_point, struct f_point, int, int, int);
static int _fastcall raytrace_header(void);
static int _fastcall start_object(void);
static void corners(MATRIX, int, double *, double *, double *, double *, double *, double *);
static void draw_light_box(double *, double *, MATRIX);
static void draw_rect(VECTOR, VECTOR, VECTOR, VECTOR, int, int);
static void line3d_cleanup(void);
static void _fastcall clip_color(int, int, int);
static void _fastcall interp_color(int, int, int);
static void _fastcall put_a_triangle(struct point, struct point, struct point, int);
static void _fastcall put_minmax(int, int, int);
static void _fastcall triangle_bounds(float pt_t[3][3]);
static void _fastcall transparent_clip_color(int, int, int);
static void _fastcall vdraw_line(double *, double *, int color);
static void (_fastcall *fill_plot)(int, int, int);
static void (_fastcall *normal_plot)(int, int, int);
static void file_error(char *filename, int code);

/* static variables */
static double s_r_scale = 0.0;			/* surface roughness factor */
static long s_x_center = 0, s_y_center = 0; /* circle center */
static double s_scale_x = 0.0, s_scale_y = 0.0, s_scale_z = 0.0; /* scale factors */
static double s_radius = 0.0;			/* radius values */
static double s_radius_factor = 0.0;	/* for intermediate calculation */
static LMATRIX s_lm;					/* "" */
static LVECTOR s_lview;					/* for perspective views */
static double s_z_cutoff = 0.0;			/* perspective backside cutoff value */
static float s_two_cos_delta_phi = 0.0;
static float s_cos_phi, s_sin_phi;		/* precalculated sin/cos of longitude */
static float s_old_cos_phi1, s_old_sin_phi1;
static float s_old_cos_phi2, s_old_sin_phi2;
static BYTE *s_fraction = NULL;			/* float version of pixels array */
static float s_min_xyz[3], s_max_xyz[3];	/* For Raytrace output */
static int s_line_length;
static int s_targa_header_len = 18;			/* Size of current Targa-24 header */
static FILE *s_raytrace_file = NULL;
static unsigned int s_ambient;
static int s_rand_factor;
static int s_haze_mult;
static BYTE s_targa_size[4];
static int s_targa_safe;						/* Original Targa Image successfully copied to s_targa_temp */
static VECTOR s_light_direction;
static BYTE s_real_color;					/* Actual color of cur pixel */
static int RO, CO, CO_MAX;				/* For use in Acrospin support */
static int s_local_preview_factor;
static int s_z_coord = 256;
static double s_aspect;					/* aspect ratio */
static int s_even_odd_row;
static float *s_sin_theta_array;			/* all sine thetas go here  */
static float *s_cos_theta_array;			/* all cosine thetas go here */
static double s_r_scale_r;					/* precalculation factor */
static int s_persp;						/* flag for indicating perspective transformations */
static struct point s_p1, s_p2, s_p3;
static struct f_point s_f_bad;			/* out of range value */
static struct point s_bad;				/* out of range value */
static long s_num_tris;					/* number of triangles output to ray trace file */
static struct f_point *s_f_last_row = NULL;
static MATRIX s_m;						/* transformation matrix */
static int s_real_v = 0;					/* mrr Actual value of V for fillytpe > 4 monochrome images */
static int s_file_error = FILEERROR_NONE;
static char s_targa_temp[14] = "fractemp.tga";
static struct point *s_last_row = NULL;	/* this array remembers the previous line */
static struct minmax *s_minmax_x;			/* array of min and max x values used in triangle fill */

int line3d(BYTE *pixels, unsigned linelen)
{
	int tout;                    /* triangle has been sent to ray trace file */
	float f_water = 0.0f;        /* transformed WATERLINE for ray trace files */
	double r0;
	int xcenter0 = 0;
	int ycenter0 = 0;      /* Unfudged versions */
	double r;                    /* sphere radius */
	float cos_theta, sin_theta;    /* precalculated sin/cos of latitude */
	int next;                    /* used by preview and grid */
	int col;                     /* current column (original GIF) */
	struct point cur;            /* current pixels */
	struct point old;            /* old pixels */
	struct f_point f_cur;
	struct f_point f_old;
	VECTOR v;                    /* double vector */
	VECTOR v1, v2;
	VECTOR cross_avg;
	int cross_not_init = 0;           /* flag for cross_avg init indication */
	LVECTOR lv;                  /* long equivalent of v */
	LVECTOR lv0;                 /* long equivalent of v */
	int last_dot;
	long fudge;
	static struct point s_old_last = { 0, 0, 0 }; /* old pixels */

	fudge = 1L << 16;
	g_plot_color = (transparent[0] || transparent[1]) ? transparent_clip_color : clip_color;
	normal_plot = g_plot_color;

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
		int err = first_time(linelen, v);
		if (err != 0)
		{
			return err;
		}
		if (xdots > OLDMAXPIXELS)
		{
			return -1;
		}
		tout = 0;
		cross_avg[0] = 0;
		cross_avg[1] = 0;
		cross_avg[2] = 0;
		s_x_center = xdots/2 + g_x_shift;
		s_y_center = ydots/2 - g_y_shift;
		xcenter0 = (int) s_x_center;
		ycenter0 = (int) s_y_center;
	}
	/* make sure these pixel coordinates are out of range */
	old = s_bad;
	f_old = s_f_bad;

	/* copies pixels buffer to float type fraction buffer for fill purposes */
	if (g_potential_16bit)
	{
		if (set_pixel_buff(pixels, s_fraction, linelen))
		{
			return 0;
		}
	}
	else if (g_grayscale_depth)           /* convert color numbers to grayscale values */
	{
		for (col = 0; col < (int) linelen; col++)
		{
			int color_num = pixels[col];
			/* effectively (30*R + 59*G + 11*B)/100 scaled 0 to 255 */
			int pal = ((int) g_dac_box[color_num][0]*77 +
					(int) g_dac_box[color_num][1]*151 +
					(int) g_dac_box[color_num][2]*28);

			pal >>= 6;
			pixels[col] = (BYTE) pal;
		}
	}
	cross_not_init = 1;
	col = 0;

	CO = 0;

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
	last_dot = min(xdots - 1, (int) linelen - 1);
	if (FILLTYPE >= FILLTYPE_LIGHT_BEFORE)
	{
		if (g_haze && Targa_Out)
		{
			s_haze_mult = (int) (g_haze*((float) ((long) (ydots - 1 - g_current_row)*(long) (ydots - 1 - g_current_row))
									/ (float) ((long) (ydots - 1)*(long) (ydots - 1))));
			s_haze_mult = 100 - s_haze_mult;
		}
	}

	if (g_preview_factor >= ydots || g_preview_factor > last_dot)
	{
		g_preview_factor = min(ydots - 1, last_dot);
	}

	s_local_preview_factor = ydots/g_preview_factor;

	tout = 0;
	/* Insure last line is drawn in preview and filltypes <0  */
	if ((g_raytrace_output || g_preview || FILLTYPE < FILLTYPE_POINTS)
		&& (g_current_row != ydots - 1)
		&& (g_current_row % s_local_preview_factor) /* Draw mod preview lines */
		&& !(!g_raytrace_output && (FILLTYPE > FILLTYPE_FILL_BARS) && (g_current_row == 1)))
			/* Get init geometry in lightsource modes */
	{
		goto reallythebottom;     /* skip over most of the line3d calcs */
	}
	if (driver_diskp())
	{
		char s[40];
		sprintf(s, "mapping to 3d, reading line %d", g_current_row);
		dvid_status(1, s);
	}

	if (!col && g_raytrace_output && g_current_row != 0)
	{
		start_object();
	}
	/* PROCESS ROW LOOP BEGINS HERE */
	while (col < (int) linelen)
	{
		if ((g_raytrace_output || g_preview || FILLTYPE < FILLTYPE_POINTS)
			&& (col != last_dot) /* if this is not the last col */
			&&  (col % (int) (s_aspect*s_local_preview_factor)) /* if not the 1st or mod factor col */
			&& (!(!g_raytrace_output && FILLTYPE > FILLTYPE_FILL_BARS && col == 1)))
		{
			goto loopbottom;
		}

		cur.color = s_real_color = pixels[col];
		f_cur.color = (float) cur.color;

		if (g_raytrace_output || g_preview || FILLTYPE < FILLTYPE_POINTS)
		{
			next = (int) (col + s_aspect*s_local_preview_factor);
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

		if (cur.color > 0 && cur.color < WATERLINE)
		{
			cur.color = s_real_color = (BYTE)WATERLINE;
			f_cur.color = (float) cur.color; /* "lake" */
		}
		else if (g_potential_16bit)
		{
			f_cur.color += ((float) s_fraction[col])/(float) (1 << 8);
		}

		if (SPHERE)            /* sphere case */
		{
			sin_theta = s_sin_theta_array[col];
			cos_theta = s_cos_theta_array[col];

			if (s_sin_phi < 0 && !(g_raytrace_output || FILLTYPE < FILLTYPE_POINTS))
			{
				cur = s_bad;
				f_cur = s_f_bad;
				goto loopbottom; /* another goto ! */
			}
			/************************************************************/
			/* KEEP THIS FOR DOCS - original formula --                 */
			/* if (s_r_scale < 0.0)                                         */
			/* r = 1.0 + ((double)cur.color/(double)s_z_coord)*s_r_scale;       */
			/* else                                                     */
			/* r = 1.0-s_r_scale + ((double)cur.color/(double)s_z_coord)*s_r_scale;*/
			/* s_radius = (double)ydots/2;                                     */
			/* r = r*s_radius;                                                 */
			/* cur.x = xdots/2 + s_scale_x*r*sin_theta*s_aspect + xup ;         */
			/* cur.y = ydots/2 + s_scale_y*r*cos_theta*s_cos_phi - yup ;         */
			/************************************************************/

			if (s_r_scale < 0.0)
			{
				r = s_radius + s_radius_factor*(double) f_cur.color*cos_theta;
			}
			else if (s_r_scale > 0.0)
			{
				r = s_radius - s_r_scale_r + s_radius_factor*(double) f_cur.color*cos_theta;
			}
			else
			{
				r = s_radius;
			}
			/* Allow Ray trace to go through so display ok */
			if (s_persp || g_raytrace_output)
			{  /* mrr how do lv[] and cur and f_cur all relate */
				/* NOTE: fudge was pre-calculated above in r and s_radius */
				/* (almost) guarantee negative */
				lv[2] = (long) (-s_radius - r*cos_theta*s_sin_phi);     /* z */
				if ((lv[2] > s_z_cutoff) && !FILLTYPE < FILLTYPE_POINTS)
				{
					cur = s_bad;
					f_cur = s_f_bad;
					goto loopbottom;      /* another goto ! */
				}
				lv[0] = (long) (s_x_center + sin_theta*s_scale_x*r);  /* x */
				lv[1] = (long) (s_y_center + cos_theta*s_cos_phi*s_scale_y*r); /* y */

				if ((FILLTYPE >= FILLTYPE_LIGHT_BEFORE) || g_raytrace_output)
				{     /* calculate illumination normal before s_persp */
					r0 = r/65536L;
					f_cur.x = (float) (xcenter0 + sin_theta*s_scale_x*r0);
					f_cur.y = (float) (ycenter0 + cos_theta*s_cos_phi*s_scale_y*r0);
					f_cur.color = (float) (-r0*cos_theta*s_sin_phi);
				}
				if (!(usr_floatflag || g_raytrace_output))
				{
					if (longpersp(lv, s_lview, 16) == -1)
					{
						cur = s_bad;
						f_cur = s_f_bad;
						goto loopbottom;   /* another goto ! */
					}
					cur.x = (int) (((lv[0] + 32768L) >> 16) + g_xx_adjust);
					cur.y = (int) (((lv[1] + 32768L) >> 16) + g_yy_adjust);
				}
				if (usr_floatflag || overflow || g_raytrace_output)
				{
					v[0] = lv[0];
					v[1] = lv[1];
					v[2] = lv[2];
					v[0] /= fudge;
					v[1] /= fudge;
					v[2] /= fudge;
					perspective(v);
					cur.x = (int) (v[0] + .5 + g_xx_adjust);
					cur.y = (int) (v[1] + .5 + g_yy_adjust);
				}
			}
			/* mrr Not sure how this an 3rd if above relate */
			else if (!(s_persp && g_raytrace_output))
			{
				/* mrr Why the xx- and g_yy_adjust here and not above? */
				f_cur.x = (float) (s_x_center + sin_theta*s_scale_x*r + g_xx_adjust);
				f_cur.y = (float) (s_y_center + cos_theta*s_cos_phi*s_scale_y*r + g_yy_adjust);
				cur.x = (int) f_cur.x;
				cur.y = (int) f_cur.y;
				if (FILLTYPE >= FILLTYPE_LIGHT_BEFORE || g_raytrace_output)        /* mrr why do we do this for filltype > 5? */
				{
					f_cur.color = (float) (-r*cos_theta*s_sin_phi*s_scale_z);
				}
				v[0] = v[1] = v[2] = 0;  /* MRR Why do we do this? */
			}
		}
		else
			/* non-sphere 3D */
		{
			if (!usr_floatflag && !g_raytrace_output)
			{
				/* flag to save vector before perspective */
				/* in longvmultpersp calculation */
				lv0[0] = (FILLTYPE >= FILLTYPE_LIGHT_BEFORE) ? 1 : 0;   

				/* use 32-bit multiply math to snap this out */
				lv[0] = col;
				lv[0] = lv[0] << 16;
				lv[1] = g_current_row;
				lv[1] = lv[1] << 16;
				if (filetype || g_potential_16bit) /* don't truncate fractional part */
				{
					lv[2] = (long) (f_cur.color*65536.0);
				}
				else
					/* there IS no fractional part here! */
				{
					lv[2] = (long) f_cur.color;
					lv[2] = lv[2] << 16;
				}

				if (longvmultpersp(lv, s_lm, lv0, lv, s_lview, 16) == -1)
				{
					cur = s_bad;
					f_cur = s_f_bad;
					goto loopbottom;
				}

				cur.x = (int) (((lv[0] + 32768L) >> 16) + g_xx_adjust);
				cur.y = (int) (((lv[1] + 32768L) >> 16) + g_yy_adjust);
				if (FILLTYPE >= FILLTYPE_LIGHT_BEFORE && !overflow)
				{
					f_cur.x = (float) lv0[0];
					f_cur.x /= 65536.0f;
					f_cur.y = (float) lv0[1];
					f_cur.y /= 65536.0f;
					f_cur.color = (float) lv0[2];
					f_cur.color /= 65536.0f;
				}
			}

			if (usr_floatflag || overflow || g_raytrace_output)
				/* do in float if integer math overflowed or doing Ray trace */
			{
				/* slow float version for comparison */
				v[0] = col;
				v[1] = g_current_row;
				v[2] = f_cur.color;      /* Actually the z value */

				mult_vec(v, s_m);     /* matrix*vector routine */

				if (FILLTYPE > FILLTYPE_FILL_BARS || g_raytrace_output)
				{
					f_cur.x = (float) v[0];
					f_cur.y = (float) v[1];
					f_cur.color = (float) v[2];

					if (g_raytrace_output == RAYTRACE_ACROSPIN)
					{
						f_cur.x = f_cur.x*(2.0f/xdots) - 1.0f;
						f_cur.y = f_cur.y*(2.0f/ydots) - 1.0f;
						f_cur.color = -f_cur.color*(2.0f/numcolors) - 1.0f;
					}
				}

				if (s_persp && !g_raytrace_output)
				{
					perspective(v);
				}
				cur.x = (int) (v[0] + g_xx_adjust + .5);
				cur.y = (int) (v[1] + g_yy_adjust + .5);

				v[0] = 0;
				v[1] = 0;
				v[2] = WATERLINE;
				mult_vec(v, s_m);
				f_water = (float) v[2];
			}
		}

		if (g_randomize)
		{
			if (cur.color > WATERLINE)
			{
				int rnd = rand15() >> 8;     /* 7-bit number */
				rnd = rnd*rnd >> s_rand_factor;  /* n-bit number */

				if (rand() & 1)
				{
					rnd = -rnd;   /* Make +/- n-bit number */
				}

				if ((int) (cur.color) + rnd >= colors)
				{
					cur.color = colors - 2;
				}
				else if ((int) (cur.color) + rnd <= WATERLINE)
				{
					cur.color = WATERLINE + 1;
				}
				else
				{
					cur.color = cur.color + rnd;
				}
				s_real_color = (BYTE)cur.color;
			}
		}

		if (g_raytrace_output)
		{
			if (col && g_current_row &&
				old.x > BAD_CHECK &&
				old.x < (xdots - BAD_CHECK) &&
				s_last_row[col].x > BAD_CHECK &&
				s_last_row[col].y > BAD_CHECK &&
				s_last_row[col].x < (xdots - BAD_CHECK) &&
				s_last_row[col].y < (ydots - BAD_CHECK))
			{
				/* Get rid of all the triangles in the plane at the base of
				* the object */

				if (f_cur.color == f_water &&
					s_f_last_row[col].color == f_water &&
					s_f_last_row[next].color == f_water)
				{
					goto loopbottom;
				}

				if (g_raytrace_output != RAYTRACE_ACROSPIN)    /* Output the vertex info */
				{
					out_triangle(f_cur, f_old, s_f_last_row[col],
						cur.color, old.color, s_last_row[col].color);
				}

				tout = 1;

				driver_draw_line(old.x, old.y, cur.x, cur.y, old.color);
				driver_draw_line(old.x, old.y, s_last_row[col].x,
					s_last_row[col].y, old.color);
				driver_draw_line(s_last_row[col].x, s_last_row[col].y,
					cur.x, cur.y, cur.color);
				s_num_tris++;
			}

			if (col < last_dot && g_current_row &&
				s_last_row[col].x > BAD_CHECK &&
				s_last_row[col].y > BAD_CHECK &&
				s_last_row[col].x < (xdots - BAD_CHECK) &&
				s_last_row[col].y < (ydots - BAD_CHECK) &&
				s_last_row[next].x > BAD_CHECK &&
				s_last_row[next].y > BAD_CHECK &&
				s_last_row[next].x < (xdots - BAD_CHECK) &&
				s_last_row[next].y < (ydots - BAD_CHECK))
			{
				/* Get rid of all the triangles in the plane at the base of
				* the object */

				if (f_cur.color == f_water &&
					s_f_last_row[col].color == f_water &&
					s_f_last_row[next].color == f_water)
				{
					goto loopbottom;
				}

				if (g_raytrace_output != RAYTRACE_ACROSPIN)    /* Output the vertex info */
				{
					out_triangle(f_cur, s_f_last_row[col], s_f_last_row[next],
							cur.color, s_last_row[col].color, s_last_row[next].color);
				}
				tout = 1;

				driver_draw_line(s_last_row[col].x, s_last_row[col].y, cur.x, cur.y,
					cur.color);
				driver_draw_line(s_last_row[next].x, s_last_row[next].y, cur.x, cur.y,
					cur.color);
				driver_draw_line(s_last_row[next].x, s_last_row[next].y, s_last_row[col].x,
					s_last_row[col].y, s_last_row[col].color);
				s_num_tris++;
			}

			if (g_raytrace_output == RAYTRACE_ACROSPIN)       /* Output vertex info for Acrospin */
			{
				fprintf(s_raytrace_file, "% #4.4f % #4.4f % #4.4f R%dC%d\n",
					f_cur.x, f_cur.y, f_cur.color, RO, CO);
				if (CO > CO_MAX)
				{
					CO_MAX = CO;
				}
				CO++;
			}
			goto loopbottom;
		}

		switch (FILLTYPE)
		{
		case FILLTYPE_SURFACE_GRID:
			if (col &&
				old.x > BAD_CHECK &&
				old.x < (xdots - BAD_CHECK))
			{
				driver_draw_line(old.x, old.y, cur.x, cur.y, cur.color);
			}
			if (g_current_row &&
				s_last_row[col].x > BAD_CHECK &&
				s_last_row[col].y > BAD_CHECK &&
				s_last_row[col].x < (xdots - BAD_CHECK) &&
				s_last_row[col].y < (ydots - BAD_CHECK))
			{
				driver_draw_line(s_last_row[col].x, s_last_row[col].y, cur.x,
					cur.y, cur.color);
			}
			break;

		case FILLTYPE_POINTS:
			(*g_plot_color)(cur.x, cur.y, cur.color);
			break;

		case FILLTYPE_WIRE_FRAME:                /* connect-a-dot */
			if ((old.x < xdots) && (col) &&
				old.x > BAD_CHECK &&
				old.y > BAD_CHECK)      /* Don't draw from old to cur on col 0 */
			{
				driver_draw_line(old.x, old.y, cur.x, cur.y, cur.color);
			}
			break;

		case FILLTYPE_FILL_GOURAUD:                /* with interpolation */
		case FILLTYPE_FILL_FLAT:                /* no interpolation */
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
				put_a_triangle(s_last_row[next], s_last_row[col], cur, cur.color);
			}
			if (g_current_row && col)  /* skip first row and first column */
			{
				if (col == 1)
				{
					put_a_triangle(s_last_row[col], s_old_last, old, old.color);
				}
				if (col < last_dot)
				{
					put_a_triangle(s_last_row[next], s_last_row[col], cur, cur.color);
				}
				put_a_triangle(old, s_last_row[col], cur, cur.color);
			}
			break;

		case FILLTYPE_FILL_BARS:                /* "solid fill" */
			if (SPHERE)
			{
				if (s_persp)
				{
					old.x = (int) (s_x_center >> 16);
					old.y = (int) (s_y_center >> 16);
				}
				else
				{
					old.x = (int) s_x_center;
					old.y = (int) s_y_center;
				}
			}
			else
			{
				lv[0] = col;
				lv[1] = g_current_row;
				lv[2] = 0;
				/* apply fudge bit shift for integer math */
				lv[0] = lv[0] << 16;
				lv[1] = lv[1] << 16;
				/* Since 0, unnecessary lv[2] = lv[2] << 16; */

				if (longvmultpersp(lv, s_lm, lv0, lv, s_lview, 16))
				{
					cur = s_bad;
					f_cur = s_f_bad;
					goto loopbottom;      /* another goto ! */
				}

				/* Round and fudge back to original  */
				old.x = (int) ((lv[0] + 32768L) >> 16);
				old.y = (int) ((lv[1] + 32768L) >> 16);
			}
			if (old.x < 0)
			{
				old.x = 0;
			}
			else if (old.x >= xdots)
			{
				old.x = xdots - 1;
			}
			if (old.y < 0)
			{
				old.y = 0;
			}
			else if (old.y >= ydots)
			{
				old.y = ydots - 1;
			}
			driver_draw_line(old.x, old.y, cur.x, cur.y, cur.color);
			break;

		case FILLTYPE_LIGHT_BEFORE:
		case FILLTYPE_LIGHT_AFTER:
			/* light-source modulated fill */
			if (g_current_row && col)  /* skip first row and first column */
			{
				if (f_cur.color < BAD_CHECK || f_old.color < BAD_CHECK ||
					s_f_last_row[col].color < BAD_CHECK)
				{
					break;
				}

				v1[0] = f_cur.x - f_old.x;
				v1[1] = f_cur.y - f_old.y;
				v1[2] = f_cur.color - f_old.color;

				v2[0] = s_f_last_row[col].x - f_cur.x;
				v2[1] = s_f_last_row[col].y - f_cur.y;
				v2[2] = s_f_last_row[col].color - f_cur.color;

				cross_product(v1, v2, g_cross);

				/* normalize cross - and check if non-zero */
				if (normalize_vector(g_cross))
				{
					if (g_debug_flag)
					{
						stopmsg(0, "debug, cur.color=bad");
					}
					f_cur.color = (float) s_bad.color;
					cur.color = s_bad.color;
				}
				else
				{
					static VECTOR tmpcross;

					/* line-wise averaging scheme */
					if (LIGHTAVG > 0)
					{
						if (cross_not_init)
						{
							/* initialize array of old normal vectors */
							cross_avg[0] = g_cross[0];
							cross_avg[1] = g_cross[1];
							cross_avg[2] = g_cross[2];
							cross_not_init = 0;
						}
						tmpcross[0] = (cross_avg[0]*LIGHTAVG + g_cross[0]) /
							(LIGHTAVG + 1);
						tmpcross[1] = (cross_avg[1]*LIGHTAVG + g_cross[1]) /
							(LIGHTAVG + 1);
						tmpcross[2] = (cross_avg[2]*LIGHTAVG + g_cross[2]) /
							(LIGHTAVG + 1);
						g_cross[0] = tmpcross[0];
						g_cross[1] = tmpcross[1];
						g_cross[2] = tmpcross[2];
						if (normalize_vector(g_cross))
						{
							/* this shouldn't happen */
							if (g_debug_flag)
							{
								stopmsg(0, "debug, normal vector err2");
								/* use next instead if you ever need details:
								* static char tmp[] = {"debug, vector err"};
								* char msg[200]; #ifndef XFRACT
								* sprintf(msg, "%s\n%f %f %f\n%f %f %f\n%f %f
								* %f", #else sprintf(msg, "%s\n%f %f %f\n%f %f
								* %f\n%f %f %f", #endif tmp, f_cur.x, f_cur.y,
								* f_cur.color, s_f_last_row[col].x,
								* s_f_last_row[col].y, s_f_last_row[col].color,
								* s_f_last_row[col-1].x,
								* s_f_last_row[col-1].y, s_f_last_row[col-1].color);
								* stopmsg(0, msg); */
							}
							f_cur.color = (float) colors;
							cur.color = colors;
						}
					}
					cross_avg[0] = tmpcross[0];
					cross_avg[1] = tmpcross[1];
					cross_avg[2] = tmpcross[2];

					/* dot product of unit vectors is cos of angle between */
					/* we will use this value to shade surface */

					cur.color = (int) (1 + (colors - 2) *
						(1.0 - dot_product(g_cross, s_light_direction)));
				}
				/* if colors out of range, set them to min or max color index
				* but avoid background index. This makes colors "opaque" so
				* SOMETHING plots. These conditions shouldn't happen but just
				* in case                                        */
				if (cur.color < 1)       /* prevent transparent colors */
				{
					cur.color = 1; /* avoid background */
				}
				if (cur.color > colors - 1)
				{
					cur.color = colors - 1;
				}

				/* why "col < 2"? So we have sufficient geometry for the fill */
				/* algorithm, which needs previous point in same row to have  */
				/* already been calculated (variable old)                 */
				/* fix ragged left margin in preview */
				if (col == 1 && g_current_row > 1)
				{
					put_a_triangle(s_last_row[next], s_last_row[col], cur, cur.color);
				}

				if (col < 2 || g_current_row < 2)       /* don't have valid colors yet */
				{
					break;
				}

				if (col < last_dot)
				{
					put_a_triangle(s_last_row[next], s_last_row[col], cur, cur.color);
				}
				put_a_triangle(old, s_last_row[col], cur, cur.color);
				assert(g_standard_plot);
				g_plot_color = g_standard_plot;
			}
			break;
		}                      /* End of CASE statement for fill type  */

loopbottom:
		if (g_raytrace_output || (FILLTYPE != FILLTYPE_POINTS && FILLTYPE != FILLTYPE_FILL_BARS))
		{
			/* for triangle and grid fill purposes */
			s_old_last = s_last_row[col];
			old = s_last_row[col] = cur;

			/* for illumination model purposes */
			f_old = s_f_last_row[col] = f_cur;
			if (g_current_row && g_raytrace_output && col >= last_dot)
				/* if we're at the end of a row, close the object */
			{
				end_object(tout);
				tout = 0;
				if (ferror(s_raytrace_file))
				{
					fclose(s_raytrace_file);
					remove(g_light_name);
					file_error(g_ray_name, FILEERROR_NO_SPACE);
					return -1;
				}
			}
		}
		col++;
	}                         /* End of while statement for plotting line  */
	RO++;

reallythebottom:
	/* stuff that HAS to be done, even in preview mode, goes here */
	if (SPHERE)
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
static void _fastcall vdraw_line(double *v1, double *v2, int color)
{
	int x1, y1, x2, y2;
	x1 = (int) v1[0];
	y1 = (int) v1[1];
	x2 = (int) v2[0];
	y2 = (int) v2[1];
	driver_draw_line(x1, y1, x2, y2, color);
}

static void corners(MATRIX m, int show, double *pxmin, double *pymin, double *pzmin, double *pxmax, double *pymax, double *pzmax)
{
	int i, j;
	VECTOR S[2][4];              /* Holds the top an bottom points,
								* S[0][]=bottom */

	/* define corners of box fractal is in in x, y, z plane "b" stands for
	* "bottom" - these points are the corners of the screen in the x-y plane.
	* The "t"'s stand for Top - they are the top of the cube where 255 color
	* points hit. */

	*pxmin = *pymin = *pzmin = (int) INT_MAX;
	*pxmax = *pymax = *pzmax = (int) INT_MIN;

	for (j = 0; j < 4; ++j)
	{
		for (i = 0; i < 3; i++)
		{
			S[0][j][i] = S[1][j][i] = 0;
		}
	}

	S[0][1][0] = S[0][2][0] = S[1][1][0] = S[1][2][0] = xdots - 1;
	S[0][2][1] = S[0][3][1] = S[1][2][1] = S[1][3][1] = ydots - 1;
	S[1][0][2] = S[1][1][2] = S[1][2][2] = S[1][3][2] = s_z_coord - 1;

	for (i = 0; i < 4; ++i)
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
			for (i = 0; i < 4; i++)
			{
				perspective(S[0][i]);
				perspective(S[1][i]);
			}
		}

		/* Keep the box surrounding the fractal */
		for (j = 0; j < 2; j++)
		{
			for (i = 0; i < 4; ++i)
			{
				S[j][i][0] += g_xx_adjust;
				S[j][i][1] += g_yy_adjust;
			}
		}

		draw_rect(S[0][0], S[0][1], S[0][2], S[0][3], 2, 1);      /* Bottom */

		draw_rect(S[0][0], S[1][0], S[0][1], S[1][1], 5, 0);      /* Sides */
		draw_rect(S[0][2], S[1][2], S[0][3], S[1][3], 6, 0);

		draw_rect(S[1][0], S[1][1], S[1][2], S[1][3], 8, 1);      /* Top */
	}
}

/* This function draws a vector from origin[] to direct[] and a box
		around it. The vector and box are transformed or not depending on
		FILLTYPE.
*/

static void draw_light_box(double *origin, double *direct, MATRIX light_m)
{
	VECTOR S[2][4];
	int i, j;
	double temp;

	S[1][0][0] = S[0][0][0] = origin[0];
	S[1][0][1] = S[0][0][1] = origin[1];

	S[1][0][2] = direct[2];

	for (i = 0; i < 2; i++)
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
	if (FILLTYPE == FILLTYPE_LIGHT_AFTER)
	{
		for (i = 0; i < 4; i++)
		{
			vmult(S[0][i], light_m, S[0][i]);
			vmult(S[1][i], light_m, S[1][i]);
		}
	}

	/* always use perspective to aid viewing */
	temp = g_view[2];              /* save perspective distance for a later
									* restore */
	g_view[2] = -PERSPECTIVE_DISTANCE*300.0/100.0;

	for (i = 0; i < 4; i++)
	{
		perspective(S[0][i]);
		perspective(S[1][i]);
	}
	g_view[2] = temp;              /* Restore perspective distance */

	/* Adjust for aspect */
	for (i = 0; i < 4; i++)
	{
		S[0][i][0] = S[0][i][0]*s_aspect;
		S[1][i][0] = S[1][i][0]*s_aspect;
	}

	/* draw box connecting transformed points. NOTE order and COLORS */
	draw_rect(S[0][0], S[0][1], S[0][2], S[0][3], 2, 1);

	vdraw_line(S[0][0], S[1][2], 8);

	/* sides */
	draw_rect(S[0][0], S[1][0], S[0][1], S[1][1], 4, 0);
	draw_rect(S[0][2], S[1][2], S[0][3], S[1][3], 5, 0);

	draw_rect(S[1][0], S[1][1], S[1][2], S[1][3], 3, 1);

	/* Draw the "arrow head" */
	for (i = -3; i < 4; i++)
	{
		for (j = -3; j < 4; j++)
		{
			if (abs(i) + abs(j) < 6)
			{
				g_plot_color((int) (S[1][2][0] + i), (int) (S[1][2][1] + j), 10);
			}
		}
	}
}

static void draw_rect(VECTOR V0, VECTOR V1, VECTOR V2, VECTOR V3, int color, int rect)
{
	VECTOR V[4];
	int i;

	/* Since V[2] is not used by vdraw_line don't bother setting it */
	for (i = 0; i < 2; i++)
	{
		V[0][i] = V0[i];
		V[1][i] = V1[i];
		V[2][i] = V2[i];
		V[3][i] = V3[i];
	}
	if (rect)                    /* Draw a rectangle */
	{
		for (i = 0; i < 4; i++)
		{
			if (fabs(V[i][0] - V[(i + 1) % 4][0]) < -2*BAD_CHECK &&
				fabs(V[i][1] - V[(i + 1) % 4][1]) < -2*BAD_CHECK)
			{
				vdraw_line(V[i], V[(i + 1) % 4], color);
			}
		}
	}
	else
		/* Draw 2 lines instead */
	{
		for (i = 0; i < 3; i += 2)
		{
			if (fabs(V[i][0] - V[i + 1][0]) < -2*BAD_CHECK &&
				fabs(V[i][1] - V[i + 1][1]) < -2*BAD_CHECK)
			{
				vdraw_line(V[i], V[i + 1], color);
			}
		}
	}
	return;
}

/* replacement for plot - builds a table of min and max x's instead of plot */
/* called by draw_line as part of triangle fill routine */
static void _fastcall put_minmax(int x, int y, int color)
{
	color = 0; /* to supress warning only */
	if (y >= 0 && y < ydots)
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

static void _fastcall put_a_triangle(struct point pt1, struct point pt2, struct point pt3, int color)
{
	int miny, maxy;
	int x, y, xlim;

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
		g_plot_color = fill_plot;
		if (s_p1.y == s_p3.y && s_p1.x == s_p3.x)
		{
			(*g_plot_color)(s_p1.x, s_p1.y, color);
		}
		else
		{
			driver_draw_line(s_p1.x, s_p1.y, s_p3.x, s_p3.y, color);
		}
		g_plot_color = normal_plot;
		return;
	}
	else if ((s_p3.y == s_p1.y && s_p3.x == s_p1.x) || (s_p3.y == s_p2.y && s_p3.x == s_p2.x))
	{
		g_plot_color = fill_plot;
		driver_draw_line(s_p1.x, s_p1.y, s_p2.x, s_p2.y, color);
		g_plot_color = normal_plot;
		return;
	}

	/* find min max y */
	miny = maxy = s_p1.y;
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
	if (maxy >= ydots)
	{
		maxy = ydots - 1;
	}

	for (y = miny; y <= maxy; y++)
	{
		s_minmax_x[y].minx = (int) INT_MAX;
		s_minmax_x[y].maxx = (int) INT_MIN;
	}

	/* set plot to "fake" plot function */
	g_plot_color = put_minmax;

	/* build table of extreme x's of triangle */
	driver_draw_line(s_p1.x, s_p1.y, s_p2.x, s_p2.y, 0);
	driver_draw_line(s_p2.x, s_p2.y, s_p3.x, s_p3.y, 0);
	driver_draw_line(s_p3.x, s_p3.y, s_p1.x, s_p1.y, 0);

	for (y = miny; y <= maxy; y++)
	{
		xlim = s_minmax_x[y].maxx;
		for (x = s_minmax_x[y].minx; x <= xlim; x++)
		{
			(*fill_plot)(x, y, color);
		}
	}
	g_plot_color = normal_plot;
}

static int _fastcall off_screen(struct point pt)
{
	if ((pt.x >= 0) && (pt.x < xdots) && (pt.y >= 0) && (pt.y < ydots))
	{
		return 0;      /* point is ok */
	}

	if (abs(pt.x) > -BAD_CHECK || abs(pt.y) > -BAD_CHECK)
	{
		return 99;              /* point is bad */
	}
	return 1;                  /* point is off the screen */
}

static void _fastcall clip_color(int x, int y, int color)
{
	if (0 <= x && x < xdots &&
		0 <= y && y < ydots &&
		0 <= color && color < filecolors)
	{
		assert(g_standard_plot);
		(*g_standard_plot)(x, y, color);

		if (Targa_Out)
		{
			/* g_standard_plot modifies color in these types */
			if (!(g_glasses_type == STEREO_ALTERNATE || g_glasses_type == STEREO_SUPERIMPOSE))
			{
				targa_color(x, y, color);
			}
		}
	}
}

/*********************************************************************/
/* This function is the same as clip_color but checks for color being */
/* in transparent range. Intended to be called only if transparency  */
/* has been enabled.                                                 */
/*********************************************************************/

static void _fastcall transparent_clip_color(int x, int y, int color)
{
	if (0 <= x && x < xdots &&   /* is the point on screen?  */
		0 <= y && y < ydots &&   /* Yes?  */
		0 <= color && color < colors &&  /* Colors in valid range?  */
		/* Lets make sure its not a transparent color  */
		(transparent[0] > color || color > transparent[1]))
	{
		assert(g_standard_plot);
		(*g_standard_plot)(x, y, color); /* I guess we can plot then  */
		if (Targa_Out)
		{
			/* g_standard_plot modifies color in these types */
			if (!(g_glasses_type == STEREO_ALTERNATE || g_glasses_type == STEREO_SUPERIMPOSE))
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

static void _fastcall interp_color(int x, int y, int color)
{
	int D, d1, d2, d3;

	/* this distance formula is not the usual one - but it has the virtue that
	* it uses ONLY additions (almost) and it DOES go to zero as the points
	* get close. */

	d1 = abs(s_p1.x - x) + abs(s_p1.y - y);
	d2 = abs(s_p2.x - x) + abs(s_p2.y - y);
	d3 = abs(s_p3.x - x) + abs(s_p3.y - y);

	D = (d1 + d2 + d3) << 1;
	if (D)
	{  /* calculate a weighted average of colors long casts prevent integer
			overflow. This can evaluate to zero */
		color = (int) (((long) (d2 + d3)*(long) s_p1.color +
				(long) (d1 + d3)*(long) s_p2.color +
				(long) (d1 + d2)*(long) s_p3.color)/D);
	}

	if (0 <= x && x < xdots &&
		0 <= y && y < ydots &&
		0 <= color && color < colors &&
		(transparent[1] == 0 || (int) s_real_color > transparent[1] ||
			transparent[0] > (int) s_real_color))
	{
		if (Targa_Out)
		{
			/* g_standard_plot modifies color in these types */
			if (!(g_glasses_type == STEREO_ALTERNATE || g_glasses_type == STEREO_SUPERIMPOSE))
			{
				D = targa_color(x, y, color);
			}
		}

		if (FILLTYPE >= FILLTYPE_LIGHT_BEFORE)
		{
			if (s_real_v && Targa_Out)
			{
				color = D;
			}
			else
			{
				color = (1 + (unsigned) color*s_ambient)/256;
				if (color == 0)
				{
					color = 1;
				}
			}
		}
		assert(g_standard_plot);
		(*g_standard_plot)(x, y, color);
	}
}

/*
		In non light source modes, both color and s_real_color contain the
		actual pixel color. In light source modes, color contains the
		light value, and s_real_color contains the origninal color

		This routine takes a pixel modifies it for lightshading if appropriate
		and plots it in a Targa file. Used in plot3d.c
*/

int _fastcall targa_color(int x, int y, int color)
{
	unsigned long H, S, V;
	BYTE RGB[3];

	if (FILLTYPE == FILLTYPE_FILL_GOURAUD
		|| g_glasses_type == STEREO_ALTERNATE
		|| g_glasses_type == STEREO_SUPERIMPOSE
		|| truecolor)
	{
		s_real_color = (BYTE)color;       /* So Targa gets interpolated color */
	}

	switch (truemode)
	{
	case TRUEMODE_DEFAULT:
		RGB[0] = (BYTE)(g_dac_box[s_real_color][0] << 2); /* Move color space to */
		RGB[1] = (BYTE)(g_dac_box[s_real_color][1] << 2); /* 256 color primaries */
		RGB[2] = (BYTE)(g_dac_box[s_real_color][2] << 2); /* from 64 colors */
		break;
	case TRUEMODE_ITERATES:
		RGB[0] = (BYTE)((g_real_color_iter >> 16) & 0xff);  /* red   */
		RGB[1] = (BYTE)((g_real_color_iter >> 8) & 0xff);  /* green */
		RGB[2] = (BYTE)((g_real_color_iter) & 0xff);  /* blue  */
		break;
	}

	/* Now lets convert it to HSV */
	R_H(RGB[0], RGB[1], RGB[2], &H, &S, &V);

	/* Modify original S and V components */
	if (FILLTYPE > FILLTYPE_FILL_BARS
		&& !(g_glasses_type == STEREO_ALTERNATE
			 || g_glasses_type == STEREO_SUPERIMPOSE))
	{
		/* Adjust for g_ambient */
		V = (V*(65535L - (unsigned) (color*s_ambient)))/65535L;
	}

	if (g_haze)
	{
		/* Haze lowers sat of colors */
		S = (unsigned long) (S*s_haze_mult)/100;
		if (V >= 32640)           /* Haze reduces contrast */
		{
			V = V - 32640;
			V = (unsigned long) ((V*s_haze_mult)/100);
			V = V + 32640;
		}
		else
		{
			V = 32640 - V;
			V = (unsigned long) ((V*s_haze_mult)/100);
			V = 32640 - V;
		}
	}
	/* Now lets convert it back to RGB. Original Hue, modified Sat and Val */
	H_R(&RGB[0], &RGB[1], &RGB[2], H, S, V);

	if (s_real_v)
	{
		V = (35*(int) RGB[0] + 45*(int) RGB[1] + 20*(int) RGB[2])/100;
	}

	/* Now write the color triple to its transformed location */
	/* on the disk. */
	targa_writedisk(x + sxoffs, y + syoffs, RGB[0], RGB[1], RGB[2]);

	return (int) (255 - V);
}

static int set_pixel_buff(BYTE *pixels, BYTE *fraction, unsigned linelen)
{
	int i;
	if ((s_even_odd_row++ & 1) == 0) /* even rows are color value */
	{
		for (i = 0; i < (int) linelen; i++)       /* add the fractional part in odd row */
		{
			fraction[i] = pixels[i];
		}
		return 1;
	}
	else
		/* swap */
	{
		BYTE tmp;
		for (i = 0; i < (int) linelen; i++)       /* swap so pixel has color */
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

static void file_error(char *filename, int code)
{
	char msgbuf[200];

	s_file_error = code;
	switch (code)
	{
	case FILEERROR_OPEN:                      /* Can't Open */
		sprintf(msgbuf, "OOPS, couldn't open  < %s >", filename);
		break;
	case FILEERROR_NO_SPACE:                      /* Not enough room */
		sprintf(msgbuf, "OOPS, ran out of disk space. < %s >", filename);
		break;
	case FILEERROR_BAD_IMAGE_SIZE:                      /* Image wrong size */
		sprintf(msgbuf, "OOPS, image wrong size\n");
		break;
	case FILEERROR_BAD_FILE_TYPE:                      /* Wrong file type */
		sprintf(msgbuf, "OOPS, can't handle this type of file.\n");
		break;
	}
	stopmsg(0, msgbuf);
	return;
}


/************************************************************************/
/*                                                                      */
/*   This function opens a TARGA_24 file for reading and writing. If    */
/*   its a new file, (overlay == 0) it writes a header. If it is to     */
/*   overlay an existing file (overlay == 1) it copies the original     */
/*   header whose lenght and validity was determined in                 */
/*   Targa_validate.                                                    */
/*                                                                      */
/*   It Verifies there is enough disk space, and leaves the file        */
/*   at the start of the display data area.                             */
/*                                                                      */
/*   If this is an overlay, closes source and copies to "s_targa_temp"    */
/*   If there is an error close the file.                               */
/*                                                                      */
/* **********************************************************************/

int startdisk1(char *file_name2, FILE *Source, int overlay)
{
	int i, j, k, inc;
	FILE *fps;

	/* Open File for both reading and writing */
	fps = dir_fopen(g_work_dir, file_name2, "w+b");
	if (fps == NULL)
	{
		file_error(file_name2, FILEERROR_OPEN);
		return -1;              /* Oops, somethings wrong! */
	}

	inc = 1;                     /* Assume we are overlaying a file */

	/* Write the header */
	if (overlay)                 /* We are overlaying a file */
	{
		for (i = 0; i < s_targa_header_len; i++) /* Copy the header from the Source */
		{
			fputc(fgetc(Source), fps);
		}
	}
	else
	{                            /* Write header for a new file */
		/* ID field size = 0, No color map, Targa type 2 file */
		for (i = 0; i < 12; i++)
		{
			if (i == 0 && truecolor != 0)
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
		for (i = 0; i < 4; i++)
		{
			fputc(s_targa_size[i], fps);
		}
		fputc(TARGA_24, fps);          /* Targa 24 file */
		fputc(TARGA_32, fps);          /* Image at upper left */
		inc = 3;
	}

	if (truecolor) /* write maxit */
	{
		fputc((BYTE)(maxit       & 0xff), fps);
		fputc((BYTE)((maxit >> 8) & 0xff), fps);
		fputc((BYTE)((maxit >> 16) & 0xff), fps);
		fputc((BYTE)((maxit >> 24) & 0xff), fps);
	}

	/* Finished with the header, now lets work on the display area  */
	for (i = 0; i < ydots; i++)  /* "clear the screen" (write to the disk) */
	{
		for (j = 0; j < s_line_length; j = j + inc)
		{
			if (overlay)
			{
				fputc(fgetc(Source), fps);
			}
			else
			{
				for (k = 2; k > -1; k--)
				{
					fputc(g_back_color[k], fps);       /* Targa order (B, G, R) */
				}
			}
		}
		if (ferror(fps))
		{
			/* Almost certainly not enough disk space  */
			fclose(fps);
			if (overlay)
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

	if (targa_startdisk(fps, s_targa_header_len) != 0)
	{
		enddisk();
		dir_remove(g_work_dir, file_name2);
		return -4;
	}
	return 0;
}

static int targa_validate(char *file_name)
{
	FILE *fp;
	int i;

	/* Attempt to open source file for reading */
	fp = dir_fopen(g_work_dir, file_name, "rb");
	if (fp == NULL)
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
	for (i = 0; i < 5; i++)
	{
		fgetc(fp);
	}

	for (i = 0; i < 4; i++)
	{
		/* Check image origin */
		fgetc(fp);
	}
	/* Check Image specs */
	for (i = 0; i < 4; i++)
	{
		if (fgetc(fp) != (int) s_targa_size[i])
		{
			file_error(file_name, FILEERROR_BAD_IMAGE_SIZE);
			return -1;
		}
	}

	if ((fgetc(fp) != (int) TARGA_24)			/* Is it a targa 24 file? */
		|| (fgetc(fp) != (int) TARGA_32))		/* Is the origin at the upper left? */
	{
		file_error(file_name, FILEERROR_BAD_FILE_TYPE);
		return -1;
	}
	rewind(fp);

	/* Now that we know its a good file, create a working copy */
	if (startdisk1(s_targa_temp, fp, 1))
	{
		return -1;
	}

	fclose(fp);                  /* Close the source */

	s_targa_safe = 1;                  /* Original file successfully copied to
									* s_targa_temp */
	return 0;
}

static int R_H(BYTE R, BYTE G, BYTE B, unsigned long *H, unsigned long *S, unsigned long *V)
{
	unsigned long R1, G1, B1, DENOM;
	BYTE MIN;

	*V = R;
	MIN = G;
	if (R < G)
	{
		*V = G;
		MIN = R;
		if (G < B)
		{
			*V = B;
		}
		if (B < R)
		{
			MIN = B;
		}
	}
	else
	{
		if (B < G)
		{
			MIN = B;
		}
		if (R < B)
		{
			*V = B;
		}
	}
	DENOM = *V - MIN;
	if (*V != 0 && DENOM != 0)
	{
		*S = ((DENOM << 16)/(*V)) - 1;
	}
	else
	{
		*S = 0;      /* Color is black! and Sat has no meaning */
	}
	if (*S == 0)    /* R = G=B => shade of grey and Hue has no meaning */
	{
		*H = 0;
		*V = *V << 8;
		return 1;               /* v or s or both are 0 */
	}
	if (*V == MIN)
	{
		*H = 0;
		*V = *V << 8;
		return 0;
	}
	R1 = (((*V - R)*60) << 6)/DENOM; /* distance of color from red   */
	G1 = (((*V - G)*60) << 6)/DENOM; /* distance of color from green */
	B1 = (((*V - B)*60) << 6)/DENOM; /* distance of color from blue  */
	if (*V == R)
	{
		*H = (MIN == G) ? (300 << 6) + B1 : (60 << 6) - G1;
	}
	if (*V == G)
	{
		*H = (MIN == B) ? (60 << 6) + R1 : (180 << 6) - B1;
	}
	if (*V == B)
	{
		*H = (MIN == R) ? (180 << 6) + G1 : (300 << 6) - R1;
	}
	*V = *V << 8;
	return 0;
}

static int H_R(BYTE *R, BYTE *G, BYTE *B, unsigned long H, unsigned long S, unsigned long V)
{
	unsigned long P1, P2, P3;
	int RMD, I;

	if (H >= 23040)
	{
		H = H % 23040;            /* Makes h circular  */
	}
	I = (int) (H/3840);
	RMD = (int) (H % 3840);      /* RMD = fractional part of H    */

	P1 = ((V*(65535L - S))/65280L) >> 8;
	P2 = (((V*(65535L - (S*RMD)/3840))/65280L) - 1) >> 8;
	P3 = (((V*(65535L - (S*(3840 - RMD))/3840))/65280L)) >> 8;
	V = V >> 8;
	switch (I)
	{
	case 0:
		*R = (BYTE) V;
		*G = (BYTE) P3;
		*B = (BYTE) P1;
		break;
	case 1:
		*R = (BYTE) P2;
		*G = (BYTE) V;
		*B = (BYTE) P1;
		break;
	case 2:
		*R = (BYTE) P1;
		*G = (BYTE) V;
		*B = (BYTE) P3;
		break;
	case 3:
		*R = (BYTE) P1;
		*G = (BYTE) P2;
		*B = (BYTE) V;
		break;
	case 4:
		*R = (BYTE) P3;
		*G = (BYTE) P1;
		*B = (BYTE) V;
		break;
	case 5:
		*R = (BYTE) V;
		*G = (BYTE) P1;
		*B = (BYTE) P2;
		break;
	}
	return 0;
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

/********************************************************************/
/*                                                                  */
/*  This routine writes a header to a ray tracer data file. It      */
/*  Identifies the version of FRACTINT which created it an the      */
/*  key 3D parameters in effect at the time.                        */
/*                                                                  */
/********************************************************************/

static int _fastcall raytrace_header(void)
{
	/* Open the ray tracing output file */
	check_writefile(g_ray_name, ".ray");
	s_raytrace_file = fopen(g_ray_name, "w");
	if (s_raytrace_file == NULL)
	{
		return -1;              /* Oops, somethings wrong! */
	}

	if (g_raytrace_output == RAYTRACE_VIVID)
	{
		fprintf(s_raytrace_file, "//");
	}
	if (g_raytrace_output == RAYTRACE_MTV)
	{
		fprintf(s_raytrace_file, "#");
	}
	if (g_raytrace_output == RAYTRACE_RAYSHADE)
	{
		fprintf(s_raytrace_file, "/*\n");
	}
	if (g_raytrace_output == RAYTRACE_ACROSPIN)
	{
		fprintf(s_raytrace_file, "--");
	}
	if (g_raytrace_output == RAYTRACE_DXF)
	{
		fprintf(s_raytrace_file,
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
			"ENTITIES\n");
	}

	if (g_raytrace_output != RAYTRACE_DXF)
	{
		fprintf(s_raytrace_file, "{ Created by FRACTINT Ver. %#4.2f }\n\n", g_release/100.);
	}

	if (g_raytrace_output == RAYTRACE_RAYSHADE)
	{
		fprintf(s_raytrace_file, "*/\n");
	}


	/* Set the default color */
	if (g_raytrace_output == RAYTRACE_POVRAY)
	{
		fprintf(s_raytrace_file, "DECLARE       F_Dflt = COLOR  RED 0.8 GREEN 0.4 BLUE 0.1\n");
	}
	if (g_raytrace_brief)
	{
		if (g_raytrace_output == RAYTRACE_VIVID)
		{
			fprintf(s_raytrace_file, "surf={diff=0.8 0.4 0.1;}\n");
		}
		if (g_raytrace_output == RAYTRACE_MTV)
		{
			fprintf(s_raytrace_file, "f 0.8 0.4 0.1 0.95 0.05 5 0 0\n");
		}
		if (g_raytrace_output == RAYTRACE_RAYSHADE)
		{
			fprintf(s_raytrace_file, "applysurf diffuse 0.8 0.4 0.1");
		}
	}
	if (g_raytrace_output != RAYTRACE_DXF)
	{
		fprintf(s_raytrace_file, "\n");
	}

	/* EB & DG: open "grid" opject, a speedy way to do aggregates in rayshade */
	if (g_raytrace_output == RAYTRACE_RAYSHADE)
	{
		fprintf(s_raytrace_file,
			"/* make a gridded aggregate. this size grid is fast for landscapes. */\n"
			"/* make z grid = 1 always for landscapes. */\n\n"
			"grid 33 25 1\n");
	}

	if (g_raytrace_output == RAYTRACE_ACROSPIN)
	{
		fprintf(s_raytrace_file, "Set Layer 1\nSet Color 2\nEndpointList X Y Z Name\n");
	}

	return 0;
}


/********************************************************************/
/*                                                                  */
/*  This routine describes the triangle to the ray tracer, it       */
/*  sets the color of the triangle to the average of the color      */
/*  of its verticies and sets the light parameters to arbitrary     */
/*  values.                                                         */
/*                                                                  */
/*  Note: numcolors (number of colors in the source                 */
/*  file) is used instead of colors (number of colors avail. with   */
/*  display) so you can generate ray trace files with your LCD      */
/*  or monochrome display                                           */
/*                                                                  */
/********************************************************************/

static int _fastcall out_triangle(struct f_point pt1, struct f_point pt2, struct f_point pt3, int c1, int c2, int c3)
{
	int i, j;
	float c[3];
	float pt_t[3][3];

	/* Normalize each vertex to screen size and adjust coordinate system */
	pt_t[0][0] = 2*pt1.x/xdots - 1;
	pt_t[0][1] = (2*pt1.y/ydots - 1);
	pt_t[0][2] = -2*pt1.color/numcolors - 1;
	pt_t[1][0] = 2*pt2.x/xdots - 1;
	pt_t[1][1] = (2*pt2.y/ydots - 1);
	pt_t[1][2] = -2*pt2.color/numcolors - 1;
	pt_t[2][0] = 2*pt3.x/xdots - 1;
	pt_t[2][1] = (2*pt3.y/ydots - 1);
	pt_t[2][2] = -2*pt3.color/numcolors - 1;

	/* Color of triangle is average of colors of its verticies */
	if (!g_raytrace_brief)
	{
		for (i = 0; i <= 2; i++)
		{
			c[i] = (float) (g_dac_box[c1][i] + g_dac_box[c2][i] + g_dac_box[c3][i])
				/ (3*63);
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
	if (g_raytrace_output == RAYTRACE_POVRAY)
	{
		fprintf(s_raytrace_file, " OBJECT\n  TRIANGLE ");
	}
	if (g_raytrace_output == RAYTRACE_VIVID && !g_raytrace_brief)
	{
		fprintf(s_raytrace_file, "surf={diff=");
	}
	if (g_raytrace_output == RAYTRACE_MTV && !g_raytrace_brief)
	{
		fprintf(s_raytrace_file, "f");
	}
	if (g_raytrace_output == RAYTRACE_RAYSHADE && !g_raytrace_brief)
	{
		fprintf(s_raytrace_file, "applysurf diffuse ");
	}

	if (!g_raytrace_brief && g_raytrace_output != RAYTRACE_POVRAY && g_raytrace_output != RAYTRACE_DXF)
	{
		for (i = 0; i <= 2; i++)
		{
			fprintf(s_raytrace_file, "% #4.4f ", c[i]);
		}
	}

	if (g_raytrace_output == RAYTRACE_VIVID)
	{
		if (!g_raytrace_brief)
		{
			fprintf(s_raytrace_file, ";}\n");
		}
		fprintf(s_raytrace_file, "polygon={points=3;");
	}
	if (g_raytrace_output == RAYTRACE_MTV)
	{
		if (!g_raytrace_brief)
		{
			fprintf(s_raytrace_file, "0.95 0.05 5 0 0\n");
		}
		fprintf(s_raytrace_file, "p 3");
	}
	if (g_raytrace_output == RAYTRACE_RAYSHADE)
	{
		if (!g_raytrace_brief)
		{
			fprintf(s_raytrace_file, "\n");
		}
		/* EB & DG: removed "T" after "triangle" */
		fprintf(s_raytrace_file, "triangle");
	}

	if (g_raytrace_output == RAYTRACE_DXF)
	{
		fprintf(s_raytrace_file, "  0\n3DFACE\n  8\nFRACTAL\n 62\n%3d\n", min(255, max(1, c1)));
	}

	for (i = 0; i <= 2; i++)     /* Describe each  Vertex  */
	{
		if (g_raytrace_output != RAYTRACE_DXF)
		{
			fprintf(s_raytrace_file, "\n");
		}

		if (g_raytrace_output == RAYTRACE_POVRAY)
		{
			fprintf(s_raytrace_file, "      <");
		}
		if (g_raytrace_output == RAYTRACE_VIVID)
		{
			fprintf(s_raytrace_file, " vertex =  ");
		}
		if (g_raytrace_output > RAYTRACE_RAW && g_raytrace_output != RAYTRACE_DXF)
		{
			fprintf(s_raytrace_file, " ");
		}

		for (j = 0; j <= 2; j++)
		{
			if (g_raytrace_output == RAYTRACE_DXF)
			{
				/* write 3dface entity to dxf file */
				fprintf(s_raytrace_file, "%3d\n%g\n", 10*(j + 1) + i, pt_t[i][j]);
				if (i == 2)         /* 3dface needs 4 vertecies */
					fprintf(s_raytrace_file, "%3d\n%g\n", 10*(j + 1) + i + 1,
						pt_t[i][j]);
			}
			else if (!(g_raytrace_output == RAYTRACE_MTV || g_raytrace_output == RAYTRACE_RAYSHADE))
			{
				fprintf(s_raytrace_file, "% #4.4f ", pt_t[i][j]); /* Right handed */
			}
			else
			{
				fprintf(s_raytrace_file, "% #4.4f ", pt_t[2 - i][j]);     /* Left handed */
			}
		}

		if (g_raytrace_output == RAYTRACE_POVRAY)
		{
			fprintf(s_raytrace_file, ">");
		}
		if (g_raytrace_output == RAYTRACE_VIVID)
		{
			fprintf(s_raytrace_file, ";");
		}
	}

	if (g_raytrace_output == RAYTRACE_POVRAY)
	{
		fprintf(s_raytrace_file, " END_TRIANGLE \n");
		if (!g_raytrace_brief)
		{
			fprintf(s_raytrace_file,
				"  TEXTURE\n"
				"   COLOR  RED% #4.4f GREEN% #4.4f BLUE% #4.4f\n"
				"      AMBIENT 0.25 DIFFUSE 0.75 END_TEXTURE\n",
				c[0], c[1], c[2]);
		}
		fprintf(s_raytrace_file, "  COLOR  F_Dflt  END_OBJECT");
		triangle_bounds(pt_t);    /* update bounding info */
	}
	if (g_raytrace_output == RAYTRACE_VIVID)
	{
		fprintf(s_raytrace_file, "}");
	}
	if (g_raytrace_output == RAYTRACE_RAW && !g_raytrace_brief)
	{
		fprintf(s_raytrace_file, "\n");
	}

	if (g_raytrace_output != RAYTRACE_DXF)
	{
		fprintf(s_raytrace_file, "\n");
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

static void _fastcall triangle_bounds(float pt_t[3][3])
{
	int i, j;

	for (i = 0; i <= 2; i++)
	{
		for (j = 0; j <= 2; j++)
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

static int _fastcall start_object(void)
{
	if (g_raytrace_output != RAYTRACE_POVRAY)
	{
		return 0;
	}

	/* Reset the min/max values, for bounding box  */
	s_min_xyz[0] = s_min_xyz[1] = s_min_xyz[2] = 999999.0f;
	s_max_xyz[0] = s_max_xyz[1] = s_max_xyz[2] = -999999.0f;

	fprintf(s_raytrace_file, "COMPOSITE\n");
	return 0;
}

/********************************************************************/
/*                                                                  */
/*  This routine adds a bounding box for the triangles drawn        */
/*  in the last block and completes the composite object created.   */
/*  It uses the globals min and max x, y and z calculated in         */
/*  z calculated in Triangle_Bounds().                              */
/*                                                                  */
/********************************************************************/

static int _fastcall end_object(int triout)
{
	if (g_raytrace_output == RAYTRACE_DXF)
	{
		return 0;
	}
	if (g_raytrace_output == RAYTRACE_POVRAY)
	{
		if (triout)
		{
			/* Make sure the bounding box is slightly larger than the object */
			int i;
			for (i = 0; i <= 2; i++)
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
			fprintf(s_raytrace_file, " BOUNDED_BY\n  INTERSECTION\n");
			fprintf(s_raytrace_file, "   PLANE <-1.0  0.0  0.0 > % #4.3f END_PLANE\n", -s_min_xyz[0]);
			fprintf(s_raytrace_file, "   PLANE < 1.0  0.0  0.0 > % #4.3f END_PLANE\n",  s_max_xyz[0]);
			fprintf(s_raytrace_file, "   PLANE < 0.0 -1.0  0.0 > % #4.3f END_PLANE\n", -s_min_xyz[1]);
			fprintf(s_raytrace_file, "   PLANE < 0.0  1.0  0.0 > % #4.3f END_PLANE\n",  s_max_xyz[1]);
			fprintf(s_raytrace_file, "   PLANE < 0.0  0.0 -1.0 > % #4.3f END_PLANE\n", -s_min_xyz[2]);
			fprintf(s_raytrace_file, "   PLANE < 0.0  0.0  1.0 > % #4.3f END_PLANE\n",  s_max_xyz[2]);
			fprintf(s_raytrace_file, "  END_INTERSECTION\n END_BOUND\n");
		}

		/* Complete the composite object statement */
		fprintf(s_raytrace_file, "END_%s\n", "COMPOSITE");
	}

	if (g_raytrace_output != RAYTRACE_ACROSPIN && g_raytrace_output != RAYTRACE_RAYSHADE)
	{
		fprintf(s_raytrace_file, "\n");    /* EB & DG: too many newlines */
	}

	return 0;
}

static void line3d_cleanup(void)
{
	int i, j;
	if (g_raytrace_output && s_raytrace_file)
	{                            /* Finish up the ray tracing files */
		if (g_raytrace_output != RAYTRACE_RAYSHADE && g_raytrace_output != RAYTRACE_DXF)
		{
			fprintf(s_raytrace_file, "\n"); /* EB & DG: too many newlines */
		}
		if (g_raytrace_output == RAYTRACE_VIVID)
		{
			fprintf(s_raytrace_file, "\n\n//");
		}
		if (g_raytrace_output == RAYTRACE_MTV)
		{
			fprintf(s_raytrace_file, "\n\n#");
		}

		if (g_raytrace_output == RAYTRACE_RAYSHADE)
		{
			/* EB & DG: end grid aggregate */
			fprintf(s_raytrace_file, "end\n\n/*good landscape:*/\n%s%s\n/*",
				"screen 640 480\neyep 0 2.1 0.8\nlookp 0 0 -0.95\nlight 1 point -2 1 1.5\n", "background .3 0 0\nreport verbose\n");
		}
		if (g_raytrace_output == RAYTRACE_ACROSPIN)
		{
			fprintf(s_raytrace_file, "LineList From To\n");
			for (i = 0; i < RO; i++)
			{
				for (j = 0; j <= CO_MAX; j++)
				{
					if (j < CO_MAX)
					{
						fprintf(s_raytrace_file, "R%dC%d R%dC%d\n", i, j, i, j + 1);
					}
					if (i < RO - 1)
					{
						fprintf(s_raytrace_file, "R%dC%d R%dC%d\n", i, j, i + 1, j);
					}
					if (i && i < RO && j < CO_MAX)
					{
						fprintf(s_raytrace_file, "R%dC%d R%dC%d\n", i, j, i - 1, j + 1);
					}
				}
			}
			fprintf(s_raytrace_file, "\n\n--");
		}
		if (g_raytrace_output != RAYTRACE_DXF)
		{
			fprintf(s_raytrace_file, "{ No. Of Triangles = %ld }*/\n\n", s_num_tris);
		}
		if (g_raytrace_output == RAYTRACE_DXF)
		{
			fprintf(s_raytrace_file, "  0\nENDSEC\n  0\nEOF\n");
		}
		fclose(s_raytrace_file);
		s_raytrace_file = NULL;
	}
	if (Targa_Out)
	{                            /* Finish up targa files */
		s_targa_header_len = 18;         /* Reset Targa header size */
		enddisk();
		if (!g_debug_flag && (!s_targa_safe || s_file_error) && g_targa_overlay)
		{
			dir_remove(g_work_dir, g_light_name);
			rename(s_targa_temp, g_light_name);
		}
		if (!g_debug_flag && g_targa_overlay)
		{
			dir_remove(g_work_dir, s_targa_temp);
		}
	}
	usr_floatflag &= 1;          /* strip second bit */
	s_file_error = FILEERROR_NONE;
	s_targa_safe = 0;
}

static void set_upr_lwr(void)
{
	s_targa_size[0] = (BYTE)(xdots & 0xff);
	s_targa_size[1] = (BYTE)(xdots >> 8);
	s_targa_size[2] = (BYTE)(ydots & 0xff);
	s_targa_size[3] = (BYTE)(ydots >> 8);
	s_line_length = 3*xdots;    /* line length @ 3 bytes per pixel  */
}

static int first_time(int linelen, VECTOR v)
{
	int err;
	MATRIX lightm;               /* m w/no trans, keeps obj. on screen */
	float twocosdeltatheta;
	double xval, yval, zval;     /* rotation values */
	/* corners of transformed xdotx by ydots x colors box */
	double xmin, ymin, zmin, xmax, ymax, zmax;
	int i, j;
	double v_length;
	VECTOR origin, direct, tmp;
	float theta, theta1, theta2; /* current, start, stop latitude */
	float phi1, phi2;            /* current start, stop longitude */
	float deltatheta;            /* increment of latitude */
	outln_cleanup = line3d_cleanup;

	calctime = s_even_odd_row = 0;
	/* mark as in-progress, and enable <tab> timer display */
	calc_status = CALCSTAT_IN_PROGRESS;

	s_ambient = (unsigned int) (255*(float) (100 - g_ambient)/100.0);
	if (s_ambient < 1)
	{
		s_ambient = 1;
	}

	s_num_tris = 0;

	/* Open file for g_raytrace_output and write header */
	if (g_raytrace_output)
	{
		raytrace_header();
		g_xx_adjust = g_yy_adjust = 0;  /* Disable shifting in ray tracing */
		g_x_shift = g_y_shift = 0;
	}

	CO_MAX = CO = RO = 0;

	set_upr_lwr();
	s_file_error = FILEERROR_NONE;

	if (g_which_image < WHICHIMAGE_BLUE)
	{
		s_targa_safe = 0; /* Not safe yet to mess with the source image */
	}

	if (Targa_Out && !((g_glasses_type == STEREO_ALTERNATE || g_glasses_type == STEREO_SUPERIMPOSE)
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
			check_writefile(g_light_name, ".tga");
			if (startdisk1(g_light_name, NULL, 0))   /* Open new file */
			{
				return -1;
			}
		}
	}

	s_rand_factor = 14 - g_randomize;

	s_z_coord = filecolors;

	err = line_3d_mem();
	if (err != 0)
	{
		return err;
	}


	/* get scale factors */
	s_scale_x = XSCALE/100.0;
	s_scale_y = YSCALE/100.0;
	if (ROUGH)
	{
		s_scale_z = -ROUGH/100.0;
	}
	else
	{
		s_r_scale = s_scale_z = -0.0001;  /* if rough=0 make it very flat but plot
									* something */
	}

	/* aspect ratio calculation - assume screen is 4 x 3 */
	s_aspect = (double) xdots *.75/(double) ydots;

	if (SPHERE == FALSE)         /* skip this slow stuff in sphere case */
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
		trans((double) xdots/-2.0, (double) ydots/-2.0, (double) s_z_coord/-2.0, s_m);
		trans((double) xdots/-2.0, (double) ydots/-2.0, (double) s_z_coord/-2.0, lightm);

		/* apply scale factors */
		scale(s_scale_x, s_scale_y, s_scale_z, s_m);
		scale(s_scale_x, s_scale_y, s_scale_z, lightm);

		/* rotation values - converting from degrees to radians */
		xval = XROT/57.29577;
		yval = YROT/57.29577;
		zval = ZROT/57.29577;

		if (g_raytrace_output)
		{
			xval = yval = zval = 0;
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
		corners(s_m, 0, &xmin, &ymin, &zmin, &xmax, &ymax, &zmax);
	}

	/* perspective 3D vector - s_lview[2] == 0 means no perspective */

	/* set perspective flag */
	s_persp = 0;
	if (ZVIEWER != 0)
	{
		s_persp = 1;
		if (ZVIEWER < 80)         /* force float */
		{
			usr_floatflag |= 2;    /* turn on second bit */
		}
	}

	/* set up view vector, and put viewer in center of screen */
	s_lview[0] = xdots >> 1;
	s_lview[1] = ydots >> 1;

	/* z value of user's eye - should be more negative than extreme negative
	* part of image */
	if (SPHERE)                  /* sphere case */
	{
		s_lview[2] = -(long) ((double) ydots*(double) ZVIEWER/100.0);
	}
	else                         /* non-sphere case */
	{
		s_lview[2] = (long) ((zmin - zmax)*(double) ZVIEWER/100.0);
	}

	g_view[0] = s_lview[0];
	g_view[1] = s_lview[1];
	g_view[2] = s_lview[2];
	s_lview[0] <<= 16;
	s_lview[1] <<= 16;
	s_lview[2] <<= 16;

	if (SPHERE == FALSE)         /* sphere skips this */
	{
		/* translate back exactly amount we translated earlier plus enough to
		* center image so maximum values are non-positive */
		trans(((double) xdots - xmax - xmin)/2, ((double) ydots - ymax - ymin)/2, -zmax, s_m);

		/* Keep the box centered and on screen regardless of shifts */
		trans(((double) xdots - xmax - xmin)/2, ((double) ydots - ymax - ymin)/2, -zmax, lightm);

		trans((double) (g_x_shift), (double) (-g_y_shift), 0.0, s_m);

		/* matrix s_m now contains ALL those transforms composed together !!
		* convert s_m to long integers shifted 16 bits */
		for (i = 0; i < 4; i++)
		{
			for (j = 0; j < 4; j++)
			{
				s_lm[i][j] = (long) (s_m[i][j]*65536.0);
			}
		}
	}
	else
		/* sphere stuff goes here */
	{
		/* Sphere is on side - north pole on right. Top is -90 degrees
		* latitude; bottom 90 degrees */

		/* Map X to this LATITUDE range */
		theta1 = (float) (THETA1*PI/180.0);
		theta2 = (float) (THETA2*PI/180.0);

		/* Map Y to this LONGITUDE range */
		phi1 = (float) (PHI1*PI/180.0);
		phi2 = (float) (PHI2*PI/180.0);

		theta = theta1;

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

		deltatheta = (float) (theta2 - theta1)/(float) linelen;

		/* initial sin, cos theta */
		s_sin_theta_array[0] = (float) sin((double) theta);
		s_cos_theta_array[0] = (float) cos((double) theta);
		s_sin_theta_array[1] = (float) sin((double) (theta + deltatheta));
		s_cos_theta_array[1] = (float) cos((double) (theta + deltatheta));

		/* sin, cos delta theta */
		twocosdeltatheta = (float) (2.0*cos((double) deltatheta));

		/* build table of other sin, cos with trig identity */
		for (i = 2; i < (int) linelen; i++)
		{
			s_sin_theta_array[i] = s_sin_theta_array[i - 1]*twocosdeltatheta -
				s_sin_theta_array[i - 2];
			s_cos_theta_array[i] = s_cos_theta_array[i - 1]*twocosdeltatheta -
				s_cos_theta_array[i - 2];
		}

		/* now phi - these calculated as we go - get started here */
		{
			/* increment of latitude, longitude */
			float delta_phi = (float) (phi2 - phi1)/(float) height;

			/* initial sin, cos phi */
			s_sin_phi = s_old_sin_phi1 = (float) sin((double) phi1);
			s_cos_phi = s_old_cos_phi1 = (float) cos((double) phi1);
			s_old_sin_phi2 = (float) sin((double) (phi1 + delta_phi));
			s_old_cos_phi2 = (float) cos((double) (phi1 + delta_phi));

			/* sin, cos delta phi */
			s_two_cos_delta_phi = (float) (2.0*cos((double) delta_phi));
		}

		/* affects how rough planet terrain is */
		if (ROUGH)
		{
			s_r_scale = .3*ROUGH/100.0;
		}

		/* radius of planet */
		s_radius = (double) (ydots)/2;

		/* precalculate factor */
		s_r_scale_r = s_radius*s_r_scale;

		s_scale_z = s_scale_x = s_scale_y = RADIUS/100.0;      /* Need x, y, z for g_raytrace_output */

		/* adjust x scale factor for aspect */
		s_scale_x *= s_aspect;

		/* precalculation factor used in sphere calc */
		s_radius_factor = s_r_scale*s_radius/(double) s_z_coord;

		if (s_persp)                /* precalculate fudge factor */
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
			zview = -(long) ((double) ydots*(double) ZVIEWER/100.0);
			radius = (double) (ydots)/2;
			angle = atan(-radius/(zview + radius));
			s_z_cutoff = -radius - sin(angle)*radius;
			s_z_cutoff *= 1.1;        /* for safety */
			s_z_cutoff *= 65536L;
		}
	}

	/* set fill plot function */
	if (FILLTYPE != FILLTYPE_FILL_FLAT)
	{
		fill_plot = interp_color;
	}
	else
	{
		fill_plot = clip_color;

		/* If transparent colors are set */
		if (transparent[0] || transparent[1])
		{
			fill_plot = transparent_clip_color; /* Use the transparent plot function  */
		}
	}

	/* Both Sphere and Normal 3D */
	direct[0] = s_light_direction[0] = XLIGHT;
	direct[1] = s_light_direction[1] = -YLIGHT;
	direct[2] = s_light_direction[2] = ZLIGHT;

	/* Needed because s_scale_z = -ROUGH/100 and s_light_direction is transformed in
	* FILLTYPE 6 but not in 5. */
	if (FILLTYPE == FILLTYPE_LIGHT_BEFORE)
	{
		direct[2] = s_light_direction[2] = -ZLIGHT;
	}

	if (FILLTYPE == FILLTYPE_LIGHT_AFTER)           /* transform light direction */
	{
		/* Think of light direction  as a vector with tail at (0, 0, 0) and head
		* at (s_light_direction). We apply the transformation to BOTH head and
		* tail and take the difference */

		v[0] = 0.0;
		v[1] = 0.0;
		v[2] = 0.0;
		vmult(v, s_m, v);
		vmult(s_light_direction, s_m, s_light_direction);

		for (i = 0; i < 3; i++)
		{
			s_light_direction[i] -= v[i];
		}
	}
	normalize_vector(s_light_direction);

	if (g_preview && g_show_box)
	{
		normalize_vector(direct);

		/* move light vector to be more clear with grey scale maps */
		origin[0] = (3*xdots)/16;
		origin[1] = (3*ydots)/4;
		if (FILLTYPE == FILLTYPE_LIGHT_AFTER)
		{
			origin[1] = (11*ydots)/16;
		}

		origin[2] = 0.0;

		v_length = min(xdots, ydots)/2;
		if (s_persp && ZVIEWER <= PERSPECTIVE_DISTANCE)
		{
			v_length *= (long) (PERSPECTIVE_DISTANCE + 600)/((long) (ZVIEWER + 600)*2);
		}

		/* Set direct[] to point from origin[] in direction of untransformed
		* s_light_direction (direct[]). */
		for (i = 0; i < 3; i++)
		{
			direct[i] = origin[i] + direct[i]*v_length;
		}

		/* center light box */
		for (i = 0; i < 2; i++)
		{
			tmp[i] = (direct[i] - origin[i])/2;
			origin[i] -= tmp[i];
			direct[i] -= tmp[i];
		}

		/* Draw light source vector and box containing it, draw_light_box will
		* transform them if necessary. */
		draw_light_box(origin, direct, lightm);
		/* draw box around original field of view to help visualize effect of
		* rotations 1 means show box - xmin etc. do nothing here */
		if (!SPHERE)
		{
			corners(s_m, 1, &xmin, &ymin, &zmin, &xmax, &ymax, &zmax);
		}
	}

	/* bad has values caught by clipping */
	s_bad.x = g_bad_value;
	s_bad.y = g_bad_value;
	s_bad.color = g_bad_value;
	s_f_bad.x = (float) s_bad.x;
	s_f_bad.y = (float) s_bad.y;
	s_f_bad.color = (float) s_bad.color;
	for (i = 0; i < (int) linelen; i++)
	{
		s_last_row[i] = s_bad;
		s_f_last_row[i] = s_f_bad;
	}
	g_got_status = GOT_STATUS_3D;
	return 0;
} /* end of once-per-image intializations */

static int line_3d_mem(void)
{
	/*********************************************************************/
	/*  Memory allocation - assumptions - a 64K segment starting at      */
	/*  extraseg has been grabbed. It may have other purposes elsewhere, */
	/*  but it is assumed that it is totally free for use here. Our      */
	/*  strategy is to assign all the pointers needed here to various*/
	/*  spots in the extra segment, depending on the pixel dimensions of */
	/*  the video mode, and check whether we have run out. There is      */
	/*  basically one case where the extra segment is not big enough     */
	/*  -- SPHERE mode with a fill type that uses put_a_triangle() (array  */
	/*  s_minmax_x) at the largest legal resolution of MAXPIXELSxMAXPIXELS */
	/*  or thereabouts. In that case we use farmemalloc to grab memory   */
	/*  for s_minmax_x. This memory is never freed.                        */
	/*********************************************************************/
	long check_extra;  /* keep track ofd extraseg array use */

	/* s_last_row stores the previous row of the original GIF image for
		the purpose of filling in gaps with triangle procedure */
	/* first 8k of extraseg now used in decoder TW 3/95 */
	/* TODO: allocate real memory, not reuse shared segment */
	s_last_row = (struct point *) malloc(sizeof(struct point)*xdots);

	check_extra = sizeof(struct point)*xdots;
	if (SPHERE)
	{
		s_sin_theta_array = (float *) (s_last_row + xdots);
		check_extra += sizeof(*s_sin_theta_array)*xdots;
		s_cos_theta_array = (float *) (s_sin_theta_array + xdots);
		check_extra += sizeof(*s_cos_theta_array)*xdots;
		s_f_last_row = (struct f_point *) (s_cos_theta_array + xdots);
	}
	else
	{
		s_f_last_row = (struct f_point *) (s_last_row + xdots);
	}
	check_extra += sizeof(*s_f_last_row)*(xdots);
	if (g_potential_16bit)
	{
		s_fraction = (BYTE *) (s_f_last_row + xdots);
		check_extra += sizeof(*s_fraction)*xdots;
	}
	s_minmax_x = (struct minmax *) NULL;

	/* TODO: clean up leftover extra segment business */
	/* these fill types call put_a_triangle which uses s_minmax_x */
	if (FILLTYPE == FILLTYPE_FILL_GOURAUD
		|| FILLTYPE == FILLTYPE_FILL_FLAT
		|| FILLTYPE == FILLTYPE_LIGHT_BEFORE
		|| FILLTYPE == FILLTYPE_LIGHT_AFTER)
	{
		/* end of arrays if we use extra segement */
		check_extra += sizeof(struct minmax)*ydots;
		if (check_extra > (1L << 16))     /* run out of extra segment? */
		{
			static struct minmax *got_mem = NULL;
			if (2222 == g_debug_flag)
			{
				stopmsg(0, "malloc minmax");
			}
			/* not using extra segment so decrement check_extra */
			check_extra -= sizeof(struct minmax)*ydots;
			if (got_mem == NULL)
			{
				got_mem = (struct minmax *) (malloc(OLDMAXPIXELS*sizeof(struct minmax)));
			}
			if (got_mem)
			{
				s_minmax_x = got_mem;
			}
			else
			{
				return -1;
			}
		}
		else /* ok to use extra segment */
		{
			s_minmax_x = g_potential_16bit ?
				(struct minmax *) (s_fraction + xdots)
				: (struct minmax *) (s_f_last_row + xdots);
		}
	}
	/* TODO: get rid of extra segment business */
	if (2222 == g_debug_flag || check_extra > (1L << 16))
	{
		char tmpmsg[70];
		sprintf(tmpmsg, "used %ld of extra segment", check_extra);
		stopmsg(STOPMSG_NO_BUZZER, tmpmsg);
	}
	return 0;
}
