/*
		loadfile.c - load an existing fractal image, control level
*/

#include <string.h>
#include <time.h>
#include <errno.h>

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "targa_lc.h"
#include "drivers.h"
#include "fihelp.h"

#define BLOCKTYPE_MAIN_INFO		1
#define BLOCKTYPE_RESUME_INFO	2
#define BLOCKTYPE_FORMULA_INFO	3
#define BLOCKTYPE_RANGES_INFO	4
#define BLOCKTYPE_MP_INFO		5
#define BLOCKTYPE_EVOLVER_INFO	6
#define BLOCKTYPE_ORBITS_INFO	7

/* routines in this module      */

static int find_fractal_info(char *, struct fractal_info *,
							struct ext_blk_resume_info *,
							struct ext_blk_formula_info *,
							struct ext_blk_ranges_info *,
							struct ext_blk_mp_info *,
							struct ext_blk_evolver_info *,
							struct ext_blk_orbits_info *);
static void load_ext_blk(char *loadptr, int loadlen);
static void skip_ext_blk(int *, int *);
static void backwardscompat(struct fractal_info *info);
static int fix_bof();
static int fix_period_bof();

int g_file_type;
int g_loaded_3d;
static FILE *fp;
int g_file_y_dots, g_file_x_dots, g_file_colors;
float g_file_aspect_ratio;
short g_skip_x_dots, g_skip_y_dots;      /* for decoder, when reducing image */
int g_bad_outside = 0;
int g_use_old_complex_power = FALSE;

int read_overlay()      /* read overlay/3D files, if reqr'd */
{
	struct fractal_info read_info;
	char oldfloatflag;
	char msg[110];
	struct ext_blk_resume_info resume_info_blk;
	struct ext_blk_formula_info formula_info;
	struct ext_blk_ranges_info ranges_info;
	struct ext_blk_mp_info mp_info;
	struct ext_blk_evolver_info evolver_info;
	struct ext_blk_orbits_info orbits_info;

	g_show_file = 1;                /* for any abort exit, pretend done */
	g_init_mode = -1;               /* no viewing mode set yet */
	oldfloatflag = g_user_float_flag;
	g_loaded_3d = 0;
	if (g_fast_restore)
	{
		g_view_window = 0;
	}
	if (has_extension(g_read_name) == NULL)
	{
		strcat(g_read_name, ".gif");
	}

	if (find_fractal_info(g_read_name, &read_info, &resume_info_blk, &formula_info,
		&ranges_info, &mp_info, &evolver_info, &orbits_info))
	{
		/* didn't find a useable file */
		sprintf(msg, "Sorry, %s isn't a file I can decode.", g_read_name);
		stop_message(0, msg);
		return -1;
	}

	g_max_iteration        = read_info.iterationsold;
	g_fractal_type     = read_info.fractal_type;
	if (g_fractal_type < 0 || g_fractal_type >= g_num_fractal_types)
	{
		sprintf(msg, "Warning: %s has a bad fractal type; using 0", g_read_name);
		g_fractal_type = 0;
	}
	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
	g_xx_min        = read_info.x_min;
	g_xx_max        = read_info.x_max;
	g_yy_min        = read_info.y_min;
	g_yy_max        = read_info.y_max;
	g_parameters[0]     = read_info.c_real;
	g_parameters[1]     = read_info.c_imag;
	g_save_release = 1100; /* unless we find out better later on */

	g_invert = 0;
	if (read_info.version > 0)
	{
		g_parameters[2]      = read_info.parm3;
		round_float_d(&g_parameters[2]);
		g_parameters[3]      = read_info.parm4;
		round_float_d(&g_parameters[3]);
		g_potential_parameter[0]   = read_info.potential[0];
		g_potential_parameter[1]   = read_info.potential[1];
		g_potential_parameter[2]   = read_info.potential[2];
		if (*g_make_par == '\0')
		{
			g_colors = read_info.colors;
		}
		g_potential_flag = (g_potential_parameter[0] != 0.0);
		g_random_flag         = read_info.random_flag;
		g_random_seed         = read_info.random_seed;
		g_inside        = read_info.inside;
		g_log_palette_flag       = read_info.logmapold;
		g_inversion[0]  = read_info.invert[0];
		g_inversion[1]  = read_info.invert[1];
		g_inversion[2]  = read_info.invert[2];
		if (g_inversion[0] != 0.0)
		{
			g_invert = 3;
		}
		g_decomposition[0]     = read_info.decomposition[0];
		g_decomposition[1]     = read_info.decomposition[1];
		g_user_biomorph  = read_info.biomorph;
		g_force_symmetry = read_info.symmetry;
	}

	if (read_info.version > 1)
	{
		g_save_release  = 1200;
		if (!g_display_3d
			&& (read_info.version <= 4 || read_info.flag3d > 0
				|| (g_current_fractal_specific->flags & PARMS3D)))
		{
			int i;
			for (i = 0; i < 16; i++)
			{
				g_init_3d[i] = read_info.init_3d[i];
			}
			g_preview_factor   = read_info.previewfactor;
			g_x_trans          = read_info.xtrans;
			g_y_trans          = read_info.ytrans;
			g_red_crop_left   = read_info.red_crop_left;
			g_red_crop_right  = read_info.red_crop_right;
			g_blue_crop_left  = read_info.blue_crop_left;
			g_blue_crop_right = read_info.blue_crop_right;
			g_red_bright      = read_info.red_bright;
			g_blue_bright     = read_info.blue_bright;
			g_x_adjust         = read_info.xadjust;
			g_eye_separation   = read_info.eyeseparation;
			g_glasses_type     = read_info.glassestype;
		}
	}

	if (read_info.version > 2)
	{
		g_save_release = 1300;
		g_outside      = read_info.outside;
	}

	g_calculation_status = CALCSTAT_PARAMS_CHANGED;       /* defaults if version < 4 */
	g_xx_3rd = g_xx_min;
	g_yy_3rd = g_yy_min;
	g_user_distance_test = 0;
	g_calculation_time = 0;
	if (read_info.version > 3)
	{
		g_save_release = 1400;
		g_xx_3rd       = read_info.x_3rd;
		g_yy_3rd       = read_info.y_3rd;
		g_calculation_status = read_info.calculation_status;
		g_user_standard_calculation_mode = read_info.stdcalcmode;
		g_three_pass = 0;
		if (g_user_standard_calculation_mode == 127)
		{
			g_three_pass = 1;
			g_user_standard_calculation_mode = '3';
		}
		g_user_distance_test     = read_info.distestold;
		g_user_float_flag   = (char)read_info.float_flag;
		g_bail_out     = read_info.bailoutold;
		g_calculation_time    = read_info.calculation_time;
		g_trig_index[0]  = read_info.trig_index[0];
		g_trig_index[1]  = read_info.trig_index[1];
		g_trig_index[2]  = read_info.trig_index[2];
		g_trig_index[3]  = read_info.trig_index[3];
		g_finite_attractor  = read_info.finattract;
		g_initial_orbit_z.x = read_info.initial_orbit_z[0];
		g_initial_orbit_z.y = read_info.initial_orbit_z[1];
		g_use_initial_orbit_z = read_info.use_initial_orbit_z;
		g_user_periodicity_check = read_info.periodicity;
	}

	g_potential_16bit = FALSE;
	g_save_system = 0;
	if (read_info.version > 4)
	{
		g_potential_16bit     = read_info.potential_16bit;
		if (g_potential_16bit)
		{
			g_file_x_dots >>= 1;
		}
		g_file_aspect_ratio = read_info.faspectratio;
		if (g_file_aspect_ratio < 0.01)       /* fix files produced in early v14.1 */
		{
			g_file_aspect_ratio = g_screen_aspect_ratio;
		}
		g_save_system  = read_info.system;
		g_save_release = read_info.release; /* from fmt 5 on we know real number */
		if (read_info.version == 5        /* except a few early fmt 5 cases: */
			&& (g_save_release <= 0 || g_save_release >= 4000))
		{
			g_save_release = 1410;
			g_save_system = 0;
		}
		if (!g_display_3d && read_info.flag3d > 0)
		{
			g_loaded_3d       = 1;
			g_ambient        = read_info.ambient;
			g_randomize      = read_info.randomize;
			g_haze           = read_info.haze;
			g_transparent[0] = read_info.transparent[0];
			g_transparent[1] = read_info.transparent[1];
		}
	}

	g_rotate_lo = 1;
	g_rotate_hi = 255;
	g_distance_test_width = 71;
	if (read_info.version > 5)
	{
		g_rotate_lo         = read_info.rotate_lo;
		g_rotate_hi         = read_info.rotate_hi;
		g_distance_test_width      = read_info.distance_test_width;
	}

	if (read_info.version > 6)
	{
		g_parameters[2]          = read_info.dparm3;
		g_parameters[3]          = read_info.dparm4;
	}

	if (read_info.version > 7)
	{
		g_fill_color         = read_info.fill_color;
	}

	if (read_info.version > 8)
	{
		g_m_x_max_fp   =  read_info.mxmaxfp        ;
		g_m_x_min_fp   =  read_info.mxminfp        ;
		g_m_y_max_fp   =  read_info.mymaxfp        ;
		g_m_y_min_fp   =  read_info.myminfp        ;
		g_z_dots     =  read_info.zdots          ;
		g_origin_fp  =  read_info.originfp       ;
		g_depth_fp   =  read_info.depthfp        ;
		g_height_fp  =  read_info.heightfp       ;
		g_width_fp   =  read_info.widthfp        ;
		g_screen_distance_fp    =  read_info.screen_distance_fp         ;
		g_eyes_fp    =  read_info.eyesfp         ;
		g_new_orbit_type = read_info.orbittype    ;
		g_juli_3d_mode   = read_info.juli3Dmode   ;
		g_max_fn    =   (char)read_info.max_fn          ;
		g_major_method = (enum Major) (read_info.inversejulia >> 8);
		g_minor_method = (enum Minor) (read_info.inversejulia & 255);
		g_parameters[4] = read_info.dparm5;
		g_parameters[5] = read_info.dparm6;
		g_parameters[6] = read_info.dparm7;
		g_parameters[7] = read_info.dparm8;
		g_parameters[8] = read_info.dparm9;
		g_parameters[9] = read_info.dparm10;
	}

	if (read_info.version < 4 && read_info.version != 0) /* pre-version 14.0? */
	{
		backwardscompat(&read_info); /* translate obsolete types */
		if (g_log_palette_flag)
		{
			g_log_palette_flag = 2;
		}
		g_user_float_flag = (char) (g_current_fractal_specific->isinteger ? 0 : 1);
	}

	if (read_info.version < 5 && read_info.version != 0) /* pre-version 15.0? */
	{
		if (g_log_palette_flag == 2) /* logmap = old changed again in format 5! */
		{
			g_log_palette_flag = LOGPALETTE_OLD;
		}
		if (g_decomposition[0] > 0 && g_decomposition[1] > 0)
		{
			g_bail_out = g_decomposition[1];
		}
	}
	if (g_potential_flag) /* in version 15.x and 16.x logmap didn't work with pot */
	{
		if (read_info.version == 6 || read_info.version == 7)
		{
			g_log_palette_flag = LOGPALETTE_NONE;
		}
	}
	set_trig_pointers(-1);

	if (read_info.version < 9 && read_info.version != 0) /* pre-version 18.0? */
	{
		/* g_force_symmetry==FORCESYMMETRY_SEARCH means we want to force symmetry but don't
			know which symmetry yet, will find out in setsymmetry() */
		if (g_outside == REAL || g_outside == IMAG || g_outside == MULT || g_outside == SUM
			|| g_outside == ATAN)
		{
			if (g_force_symmetry == FORCESYMMETRY_NONE)
			{
				g_force_symmetry = FORCESYMMETRY_SEARCH;
			}
		}
	}
	if (g_save_release < 1725 && read_info.version != 0) /* pre-version 17.25 */
	{
		set_if_old_bif(); /* translate bifurcation types */
		g_function_preloaded = TRUE;
	}

	if (read_info.version > 9)
	{ /* post-version 18.22 */
		g_bail_out     = read_info.bail_out; /* use long bailout */
		g_bail_out_test = (enum bailouts) read_info.bailoutest;
	}
	else
	{
		g_bail_out_test = Mod;
	}
	set_bail_out_formula(g_bail_out_test);

	if (read_info.version > 9)
	{
		/* post-version 18.23 */
		g_max_iteration = read_info.iterations; /* use long maxit */
		/* post-version 18.27 */
		g_old_demm_colors = read_info.old_demm_colors;
	}

	if (read_info.version > 10) /* post-version 19.20 */
	{
		g_log_palette_flag = read_info.logmap;
		g_user_distance_test = read_info.distance_test;
	}

	if (read_info.version > 11) /* post-version 19.20, inversion fix */
	{
		g_inversion[0] = read_info.dinvert[0];
		g_inversion[1] = read_info.dinvert[1];
		g_inversion[2] = read_info.dinvert[2];
		g_log_dynamic_calculate = read_info.logcalc;
		g_stop_pass     = read_info.stop_pass;
	}

	if (read_info.version > 12) /* post-version 19.60 */
	{
		g_quick_calculate   = read_info.quick_calculate;
		g_proximity    = read_info.proximity;
		if (g_fractal_type == FPPOPCORN || g_fractal_type == LPOPCORN ||
			g_fractal_type == FPPOPCORNJUL || g_fractal_type == LPOPCORNJUL ||
			g_fractal_type == LATOO)
		{
			g_function_preloaded = TRUE;
		}
	}

	g_no_bof = FALSE;
	if (read_info.version > 13) /* post-version 20.1.2 */
	{
		g_no_bof = read_info.no_bof;
	}

	/* if (read_info.version > 14)  post-version 20.1.12 */
	/* modified saved evolver structure JCO 12JUL01 */
	g_log_automatic_flag = FALSE;  /* make sure it's turned off */

	g_orbit_interval = 1;
	if (read_info.version > 15) /* post-version 20.3.2 */
	{
		g_orbit_interval = read_info.orbit_interval;
	}

	g_orbit_delay = 0;
	g_math_tolerance[0] = 0.05;
	g_math_tolerance[1] = 0.05;
	if (read_info.version > 16) /* post-version 20.4.0 */
	{
		g_orbit_delay = read_info.orbit_delay;
		g_math_tolerance[0] = read_info.math_tolerance[0];
		g_math_tolerance[1] = read_info.math_tolerance[1];
	}

	backwards_v18();
	backwards_v19();
	backwards_v20();

	if (g_display_3d)                   /* PB - a klooge till the meaning of */
	{
		g_user_float_flag = oldfloatflag; /*  g_float_flag in line3d is clarified */
	}

	if (g_overlay_3d)
	{
		g_init_mode = g_adapter;          /* use previous adapter mode for overlays */
		if (g_file_x_dots > g_x_dots || g_file_y_dots > g_y_dots)
		{
			stop_message(0, "Can't overlay with a larger image");
			g_init_mode = -1;
			return -1;
		}
	}
	else
	{
		int olddisplay3d, i;
		char oldfloatflag;
		olddisplay3d = g_display_3d;
		oldfloatflag = g_float_flag;
		g_display_3d = g_loaded_3d;      /* for <tab> display during next */
		g_float_flag = g_user_float_flag; /* ditto */
		i = get_video_mode(&read_info, &formula_info);
#if defined(_WIN32)
		_ASSERTE(_CrtCheckMemory());
#endif
		g_display_3d = olddisplay3d;
		g_float_flag = oldfloatflag;
		if (i)
		{
			if (resume_info_blk.got_data == 1)
			{
				free(resume_info_blk.resume_data);
				resume_info_blk.length = 0;
			}
			g_init_mode = -1;
			return -1;
		}
	}

	if (g_display_3d)
	{
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		g_fractal_type = PLASMA;
		g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
		g_parameters[0] = 0;
		if (!g_initialize_batch)
		{
			if (get_3d_parameters() < 0)
			{
				g_init_mode = -1;
				return -1;
			}
		}
	}

	end_resume();

	if (resume_info_blk.got_data == 1)
	{
		g_resume_info = resume_info_blk.resume_data;
		g_resume_length = resume_info_blk.length;
	}

	if (formula_info.got_data == 1)
	{
		char *nameptr;
		switch (read_info.fractal_type)
		{
		case LSYSTEM:
			nameptr = g_l_system_name;
			break;

		case IFS:
		case IFS3D:
			nameptr = g_ifs_name;
			break;

		default:
			nameptr = g_formula_name;
			g_uses_p1 = formula_info.uses_p1;
			g_uses_p2 = formula_info.uses_p2;
			g_uses_p3 = formula_info.uses_p3;
			g_uses_is_mand = formula_info.uses_is_mand;
			g_is_mand = formula_info.ismand;
			g_uses_p4 = formula_info.uses_p4;
			g_uses_p5 = formula_info.uses_p5;
			break;
		}
		formula_info.form_name[ITEMNAMELEN] = 0;
		strcpy(nameptr, formula_info.form_name);
		/* perhaps in future add more here, check block_len for backward compatibility */
	}

	if (g_ranges_length) /* free prior ranges */
	{
		free(g_ranges);
		g_ranges = NULL;
		g_ranges_length = 0;
	}

	if (ranges_info.got_data == 1)
	{
		g_ranges = (int *) ranges_info.range_data;
		g_ranges_length = ranges_info.length;
#ifdef XFRACT
		fix_ranges(g_ranges, g_ranges_length, 1);
#endif
	}

	if (mp_info.got_data == 1)
	{
		g_bf_math = 1;
		init_bf_length(read_info.bflength);
		memcpy((char *) bfxmin, mp_info.apm_data, mp_info.length);
		free(mp_info.apm_data);
	}
	else
	{
		g_bf_math = 0;
	}

	if (evolver_info.got_data == 1)
	{
		struct evolution_info resume_e_info;
		int i;

		if (read_info.version < 15)  /* This is VERY Ugly!  JCO  14JUL01 */
		{
			/* Increasing NUMGENES moves ecount in the data structure */
			/* We added 4 to NUMGENES, so ecount is at NUMGENES-4 */
			evolver_info.ecount = evolver_info.mutate[NUMGENES - 4];
		}
		if (evolver_info.ecount != evolver_info.gridsz*evolver_info.gridsz
				&& g_calculation_status != CALCSTAT_COMPLETED)
		{
			g_calculation_status = CALCSTAT_RESUMABLE;
			if (g_evolve_handle == NULL)
			{
				g_evolve_handle = malloc(sizeof(resume_e_info));
			}
			resume_e_info.parameter_range_x  = evolver_info.parameter_range_x;
			resume_e_info.parameter_range_y  = evolver_info.parameter_range_y;
			resume_e_info.opx          = evolver_info.opx;
			resume_e_info.opy          = evolver_info.opy;
			resume_e_info.odpx         = evolver_info.odpx;
			resume_e_info.odpy         = evolver_info.odpy;
			resume_e_info.px           = evolver_info.px;
			resume_e_info.py           = evolver_info.py;
			resume_e_info.sxoffs       = evolver_info.sxoffs;
			resume_e_info.syoffs       = evolver_info.syoffs;
			resume_e_info.x_dots        = evolver_info.x_dots;
			resume_e_info.y_dots        = evolver_info.y_dots;
			resume_e_info.gridsz       = evolver_info.gridsz;
			resume_e_info.evolving     = evolver_info.evolving;
			resume_e_info.this_generation_random_seed = evolver_info.this_generation_random_seed;
			resume_e_info.fiddle_factor = evolver_info.fiddle_factor;
			resume_e_info.ecount       = evolver_info.ecount;
			memcpy(g_evolve_handle, &resume_e_info, sizeof(resume_e_info));
		}
		else
		{
			if (g_evolve_handle != NULL)  /* Image completed, release it. */
			{
				free(g_evolve_handle);
				g_evolve_handle = NULL;
			}
			g_calculation_status = CALCSTAT_COMPLETED;
		}
		g_parameter_range_x  = evolver_info.parameter_range_x;
		g_parameter_range_y  = evolver_info.parameter_range_y;
		g_parameter_offset_x = g_new_parameter_offset_x = evolver_info.opx;
		g_parameter_offset_y = g_new_parameter_offset_y = evolver_info.opy;
		g_discrete_parameter_offset_x = g_new_discrete_parameter_offset_x = (char) evolver_info.odpx;
		g_discrete_parameter_offset_y = g_new_discrete_parameter_offset_y = (char) evolver_info.odpy;
		g_px           = evolver_info.px;
		g_py           = evolver_info.py;
		g_sx_offset       = evolver_info.sxoffs;
		g_sy_offset       = evolver_info.syoffs;
		g_x_dots        = evolver_info.x_dots;
		g_y_dots        = evolver_info.y_dots;
		g_grid_size       = evolver_info.gridsz;
		g_this_generation_random_seed = evolver_info.this_generation_random_seed;
		g_fiddle_factor   = evolver_info.fiddle_factor;
		g_evolving = g_view_window = (int) evolver_info.evolving;
		g_delta_parameter_image_x = g_parameter_range_x/(g_grid_size - 1);
		g_delta_parameter_image_y = g_parameter_range_y/(g_grid_size - 1);
		if (read_info.version > 14)
		{
			for (i = 0; i < NUMGENES; i++)
			{
				g_genes[i].mutate = (int) evolver_info.mutate[i];
			}
		}
		else
		{
			for (i = 0; i < 6; i++)
			{
				g_genes[i].mutate = (int) evolver_info.mutate[i];
			}
			for (i = 6; i < 10; i++)
			{
				g_genes[i].mutate = 0;
			}
			for (i = 10; i < NUMGENES; i++)
			{
				g_genes[i].mutate = (int) evolver_info.mutate[i-4];
			}
		}
		save_parameter_history();
	}
	else
	{
		g_evolving = EVOLVE_NONE;
	}

	if (orbits_info.got_data == 1)
	{
		g_orbit_x_min       = orbits_info.oxmin;
		g_orbit_x_max       = orbits_info.oxmax;
		g_orbit_y_min       = orbits_info.oymin;
		g_orbit_y_max       = orbits_info.oymax;
		g_orbit_x_3rd       = orbits_info.ox3rd;
		g_orbit_y_3rd       = orbits_info.oy3rd;
		g_keep_screen_coords = orbits_info.keep_scrn_coords;
		g_orbit_draw_mode    = (int) orbits_info.drawmode;
		if (g_keep_screen_coords)
		{
			g_set_orbit_corners = 1;
		}
	}

	g_show_file = 0;                   /* trigger the file load */
	return 0;
}

static int find_fractal_info(char *gif_file, struct fractal_info *info,
	struct ext_blk_resume_info *resume_info_blk,
	struct ext_blk_formula_info *formula_info,
	struct ext_blk_ranges_info *ranges_info,
	struct ext_blk_mp_info *mp_info,
	struct ext_blk_evolver_info *evolver_info,
	struct ext_blk_orbits_info *orbits_info)
{
	BYTE gifstart[18];
	char temp1[81];
	int scan_extend, block_type, block_len, data_len;
	int fractinf_len;
	int hdr_offset;
	struct formula_info fload_info;
	struct evolution_info eload_info;
	struct orbits_info oload_info;
	int i, j, k = 0;

	resume_info_blk->got_data = 0; /* initialize to no data */
	formula_info->got_data = 0; /* initialize to no data */
	ranges_info->got_data = 0; /* initialize to no data */
	mp_info->got_data = 0; /* initialize to no data */
	evolver_info->got_data = 0; /* initialize to no data */
	orbits_info->got_data = 0; /* initialize to no data */

	fp = fopen(gif_file, "rb");
	if (fp == NULL)
	{
		return -1;
	}
	fread(gifstart, 13, 1, fp);
	if (strncmp((char *)gifstart, "GIF", 3) != 0)  /* not GIF, maybe old .tga? */
	{
		fclose(fp);
		return -1;
	}

	g_file_type = 0; /* GIF */
	GET16(gifstart[6], g_file_x_dots);
	GET16(gifstart[8], g_file_y_dots);
	g_file_colors = 2 << (gifstart[10] & 7);
	g_file_aspect_ratio = 0; /* unknown */
	if (gifstart[12])  /* calc reasonably close value from gif header */
	{
		g_file_aspect_ratio = (float)((64.0 / ((double)(gifstart[12]) + 15.0))
						*(double)g_file_y_dots / (double)g_file_x_dots);
		if (g_file_aspect_ratio > g_screen_aspect_ratio-0.03
			&& g_file_aspect_ratio < g_screen_aspect_ratio + 0.03)
		{
			g_file_aspect_ratio = g_screen_aspect_ratio;
		}
	}
	else if (g_file_y_dots*4 == g_file_x_dots*3) /* assume the common square pixels */
	{
		g_file_aspect_ratio = g_screen_aspect_ratio;
	}

	if (*g_make_par == 0 && (gifstart[10] & 0x80) != 0)
	{
		for (i = 0; i < g_file_colors; i++)
		{
			for (j = 0; j < 3; j++)
			{
				k = getc(fp);
				if (k < 0)
				{
					break;
				}
				/* TODO: does not work when COLOR_CHANNEL_MAX != 63 */
				g_dac_box[i][j] = (BYTE)(k >> 2);
			}
			if (k < 0)
			{
				break;
			}
		}
	}

	/* Format of .gif extension blocks is:
			1 byte    '!', extension g_block identifier
			1 byte    extension g_block number, 255
			1 byte    length of id, 11
			11 bytes   alpha id, "fractintnnn" with fractint, nnn is secondary id
		n * {
			1 byte    length of g_block info in bytes
			x bytes   g_block info
			}
			1 byte    0, extension terminator
		To scan extension blocks, we first look in file at length of fractal_info
		(the main extension g_block) from end of file, looking for a literal known
		to be at start of our g_block info.  Then we scan forward a bit, in case
		the file is from an earlier fractint vsn with shorter fractal_info.
		If fractal_info is found and is from vsn >= 14, it includes the total length
		of all extension blocks; we then scan them all first to last to load
		any optional ones which are present.
		Defined extension blocks:
		fractint001     header, always present
		fractint002     resume info for interrupted resumable image
		fractint003     additional formula type info
		fractint004     ranges info
		fractint005     extended precision parameters
		fractint006     evolver params
	*/

	memset(info, 0, FRACTAL_INFO_SIZE);
	fractinf_len = FRACTAL_INFO_SIZE + (FRACTAL_INFO_SIZE + 254)/255;
	fseek(fp, (long)(-1-fractinf_len), SEEK_END);
	/* TODO: revise this to read members one at a time so we get natural alignment
		of fields within the FRACTAL_INFO structure for the platform */
	fread(info, 1, FRACTAL_INFO_SIZE, fp);
	if (strcmp(INFO_ID, info->info_id) == 0)
	{
#ifdef XFRACT
		decode_fractal_info(info, 1);
#endif
		hdr_offset = -1-fractinf_len;
	}
	else
	{
		/* didn't work 1st try, maybe an older vsn, maybe junk at eof, scan: */
		int offset, i;
		char tmpbuf[110];
		hdr_offset = 0;
		offset = 80; /* don't even check last 80 bytes of file for id */
		while (offset < fractinf_len + 513)  /* allow 512 garbage at eof */
		{
			offset += 100; /* go back 100 bytes at a time */
			fseek(fp, (long) -offset, SEEK_END);
			fread(tmpbuf, 1, 110, fp); /* read 10 extra for string compare */
			for (i = 0; i < 100; ++i)
			{
				if (!strcmp(INFO_ID, &tmpbuf[i]))  /* found header? */
				{
					strcpy(info->info_id, INFO_ID);
					fseek(fp, (long)(hdr_offset = i-offset), SEEK_END);
					/* TODO: revise this to read members one at a time so we get natural alignment
						of fields within the FRACTAL_INFO structure for the platform */
					fread(info, 1, FRACTAL_INFO_SIZE, fp);
#ifdef XFRACT
					decode_fractal_info(info, 1);
#endif
					offset = 10000; /* force exit from outer loop */
					break;
				}
			}
		}
	}

	if (hdr_offset)  /* we found INFO_ID */
	{

		if (info->version >= 4)
		{
			/* first reload main extension g_block, reasons:
			might be over 255 chars, and thus earlier load might be bad
			find exact endpoint, so scan back to start of ext blks works
			*/
			fseek(fp, (long)(hdr_offset-15), SEEK_END);
			scan_extend = 1;
			while (scan_extend)
			{
				if (fgetc(fp) != '!' /* if not what we expect just give up */
					|| fread(temp1, 1, 13, fp) != 13
					|| strncmp(&temp1[2], "fractint", 8))
				{
					break;
				}
				temp1[13] = 0;
				block_type = atoi(&temp1[10]); /* e.g. "fractint002" */
				switch (block_type)
				{
				case BLOCKTYPE_MAIN_INFO: /* "fractint001", the main extension g_block */
					if (scan_extend == 2)  /* we've been here before, done now */
					{
						scan_extend = 0;
						break;
					}
					load_ext_blk((char *)info, FRACTAL_INFO_SIZE);
#ifdef XFRACT
					decode_fractal_info(info, 1);
#endif
					scan_extend = 2;
					/* now we know total extension len, back up to first g_block */
					fseek(fp, 0L-info->tot_extend_len, SEEK_CUR);
					break;
				case BLOCKTYPE_RESUME_INFO: /* resume info */
					skip_ext_blk(&block_len, &data_len); /* once to get lengths */
					resume_info_blk->resume_data = (char *) malloc(data_len);
					if (resume_info_blk->resume_data == 0)
					{
						info->calculation_status = CALCSTAT_NON_RESUMABLE; /* not resumable after all */
					}
					else
					{
						fseek(fp, (long) -block_len, SEEK_CUR);
						load_ext_blk(resume_info_blk->resume_data, data_len);
						resume_info_blk->length = data_len;
						resume_info_blk->got_data = 1; /* got data */
					}
					break;
				case BLOCKTYPE_FORMULA_INFO: /* formula info */
					skip_ext_blk(&block_len, &data_len); /* once to get lengths */
					/* check data_len for backward compatibility */
					fseek(fp, (long) -block_len, SEEK_CUR);
					load_ext_blk((char *)&fload_info, data_len);
					strcpy(formula_info->form_name, fload_info.form_name);
					formula_info->length = data_len;
					formula_info->got_data = 1; /* got data */
					if (data_len < sizeof(fload_info))  /* must be old GIF */
					{
						formula_info->uses_p1 = 1;
						formula_info->uses_p2 = 1;
						formula_info->uses_p3 = 1;
						formula_info->uses_is_mand = 0;
						formula_info->ismand = 1;
						formula_info->uses_p4 = 0;
						formula_info->uses_p5 = 0;
					}
					else
					{
						formula_info->uses_p1 = fload_info.uses_p1;
						formula_info->uses_p2 = fload_info.uses_p2;
						formula_info->uses_p3 = fload_info.uses_p3;
						formula_info->uses_is_mand = fload_info.uses_is_mand;
						formula_info->ismand = fload_info.ismand;
						formula_info->uses_p4 = fload_info.uses_p4;
						formula_info->uses_p5 = fload_info.uses_p5;
					}
					break;
				case BLOCKTYPE_RANGES_INFO: /* ranges info */
					skip_ext_blk(&block_len, &data_len); /* once to get lengths */
					ranges_info->range_data = (int *)malloc((long)data_len);
					if (ranges_info->range_data != NULL)
					{
						fseek(fp, (long) -block_len, SEEK_CUR);
						load_ext_blk((char *)ranges_info->range_data, data_len);
						ranges_info->length = data_len/2;
						ranges_info->got_data = 1; /* got data */
					}
					break;
				case BLOCKTYPE_MP_INFO: /* extended precision parameters  */
					skip_ext_blk(&block_len, &data_len); /* once to get lengths */
					mp_info->apm_data = (char *)malloc((long)data_len);
					if (mp_info->apm_data != NULL)
					{
						fseek(fp, (long) -block_len, SEEK_CUR);
						load_ext_blk(mp_info->apm_data, data_len);
						mp_info->length = data_len;
						mp_info->got_data = 1; /* got data */
						}
					break;
				case BLOCKTYPE_EVOLVER_INFO: /* evolver params */
					skip_ext_blk(&block_len, &data_len); /* once to get lengths */
					fseek(fp, (long) -block_len, SEEK_CUR);
					load_ext_blk((char *)&eload_info, data_len);
					/* XFRACT processing of doubles here */
#ifdef XFRACT
					decode_evolver_info(&eload_info, 1);
#endif
					evolver_info->length = data_len;
					evolver_info->got_data = 1; /* got data */

					evolver_info->parameter_range_x     = eload_info.parameter_range_x;
					evolver_info->parameter_range_y     = eload_info.parameter_range_y;
					evolver_info->opx             = eload_info.opx;
					evolver_info->opy             = eload_info.opy;
					evolver_info->odpx            = (char)eload_info.odpx;
					evolver_info->odpy            = (char)eload_info.odpy;
					evolver_info->px              = eload_info.px;
					evolver_info->py              = eload_info.py;
					evolver_info->sxoffs          = eload_info.sxoffs;
					evolver_info->syoffs          = eload_info.syoffs;
					evolver_info->x_dots           = eload_info.x_dots;
					evolver_info->y_dots           = eload_info.y_dots;
					evolver_info->gridsz          = eload_info.gridsz;
					evolver_info->evolving        = eload_info.evolving;
					evolver_info->this_generation_random_seed  = eload_info.this_generation_random_seed;
					evolver_info->fiddle_factor    = eload_info.fiddle_factor;
					evolver_info->ecount          = eload_info.ecount;
					for (i = 0; i < NUMGENES; i++)
					{
						evolver_info->mutate[i]    = eload_info.mutate[i];
					}
					break;
				case BLOCKTYPE_ORBITS_INFO: /* orbits parameters  */
					skip_ext_blk(&block_len, &data_len); /* once to get lengths */
					fseek(fp, (long) -block_len, SEEK_CUR);
					load_ext_blk((char *)&oload_info, data_len);
					/* XFRACT processing of doubles here */
#ifdef XFRACT
					decode_orbits_info(&oload_info, 1);
#endif
					orbits_info->length = data_len;
					orbits_info->got_data = 1; /* got data */
					orbits_info->oxmin           = oload_info.oxmin;
					orbits_info->oxmax           = oload_info.oxmax;
					orbits_info->oymin           = oload_info.oymin;
					orbits_info->oymax           = oload_info.oymax;
					orbits_info->ox3rd           = oload_info.ox3rd;
					orbits_info->oy3rd           = oload_info.oy3rd;
					orbits_info->keep_scrn_coords = oload_info.keep_scrn_coords;
					orbits_info->drawmode        = oload_info.drawmode;
					break;
				default:
					skip_ext_blk(&block_len, &data_len);
				}
			}
		}

		fclose(fp);
		g_file_aspect_ratio = g_screen_aspect_ratio; /* if not >= v15, this is correct */
		return 0;
	}

	strcpy(info->info_id, "GIFFILE");
	info->iterations = 150;
	info->iterationsold = 150;
	info->fractal_type = PLASMA;
	info->x_min = -1;
	info->x_max = 1;
	info->y_min = -1;
	info->y_max = 1;
	info->x_3rd = -1;
	info->y_3rd = -1;
	info->c_real = 0;
	info->c_imag = 0;
	info->videomodeax = 255;
	info->videomodebx = 255;
	info->videomodecx = 255;
	info->videomodedx = 255;
	info->dotmode = 0;
	info->x_dots = (short)g_file_x_dots;
	info->y_dots = (short)g_file_y_dots;
	info->colors = (short)g_file_colors;
	info->version = 0; /* this forces lots more init at calling end too */

	/* zero means we won */
	fclose(fp);
	return 0;
}

static void load_ext_blk(char *loadptr, int loadlen)
{
	int len;
	while ((len = fgetc(fp)) > 0)
	{
		while (--len >= 0)
		{
			if (--loadlen >= 0)
			{
				*(loadptr++) = (char)fgetc(fp);
			}
			else
			{
				fgetc(fp); /* discard excess characters */
			}
		}
	}
}

static void skip_ext_blk(int *block_len, int *data_len)
{
	int len;
	*data_len = 0;
	*block_len = 1;
	while ((len = fgetc(fp)) > 0)
	{
		fseek(fp, (long)len, SEEK_CUR);
		*data_len += len;
		*block_len += len + 1;
	}
}


/* switch obsolete fractal types to new generalizations */
static void backwardscompat(struct fractal_info *info)
{
	switch (g_fractal_type)
	{
	case LAMBDASINE:
		g_fractal_type = LAMBDATRIGFP;
		g_trig_index[0] = SIN;
		break;
	case LAMBDACOS    :
		g_fractal_type = LAMBDATRIGFP;
		g_trig_index[0] = COS;
		break;
	case LAMBDAEXP    :
		g_fractal_type = LAMBDATRIGFP;
		g_trig_index[0] = EXP;
		break;
	case MANDELSINE   :
		g_fractal_type = MANDELTRIGFP;
		g_trig_index[0] = SIN;
		break;
	case MANDELCOS    :
		g_fractal_type = MANDELTRIGFP;
		g_trig_index[0] = COS;
		break;
	case MANDELEXP    :
		g_fractal_type = MANDELTRIGFP;
		g_trig_index[0] = EXP;
		break;
	case MANDELSINH   :
		g_fractal_type = MANDELTRIGFP;
		g_trig_index[0] = SINH;
		break;
	case LAMBDASINH   :
		g_fractal_type = LAMBDATRIGFP;
		g_trig_index[0] = SINH;
		break;
	case MANDELCOSH   :
		g_fractal_type = MANDELTRIGFP;
		g_trig_index[0] = COSH;
		break;
	case LAMBDACOSH   :
		g_fractal_type = LAMBDATRIGFP;
		g_trig_index[0] = COSH;
		break;
	case LMANDELSINE  :
		g_fractal_type = MANDELTRIG;
		g_trig_index[0] = SIN;
		break;
	case LLAMBDASINE  :
		g_fractal_type = LAMBDATRIG;
		g_trig_index[0] = SIN;
		break;
	case LMANDELCOS   :
		g_fractal_type = MANDELTRIG;
		g_trig_index[0] = COS;
		break;
	case LLAMBDACOS   :
		g_fractal_type = LAMBDATRIG;
		g_trig_index[0] = COS;
		break;
	case LMANDELSINH  :
		g_fractal_type = MANDELTRIG;
		g_trig_index[0] = SINH;
		break;
	case LLAMBDASINH  :
		g_fractal_type = LAMBDATRIG;
		g_trig_index[0] = SINH;
		break;
	case LMANDELCOSH  :
		g_fractal_type = MANDELTRIG;
		g_trig_index[0] = COSH;
		break;
	case LLAMBDACOSH  :
		g_fractal_type = LAMBDATRIG;
		g_trig_index[0] = COSH;
		break;
	case LMANDELEXP   :
		g_fractal_type = MANDELTRIG;
		g_trig_index[0] = EXP;
		break;
	case LLAMBDAEXP   :
		g_fractal_type = LAMBDATRIG;
		g_trig_index[0] = EXP;
		break;
	case DEMM         :
		g_fractal_type = MANDELFP;
		g_user_distance_test = (info->y_dots - 1)*2;
		break;
	case DEMJ         :
		g_fractal_type = JULIAFP;
		g_user_distance_test = (info->y_dots - 1)*2;
		break;
	case MANDELLAMBDA :
		g_use_initial_orbit_z = 2;
		break;
	}
	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
}

/* switch old bifurcation fractal types to new generalizations */
void set_if_old_bif()
{
	/* set functions if not set already, may need to check 'g_function_preloaded'
		before calling this routine.  JCO 7/5/92 */

	switch (g_fractal_type)
	{
	case BIFURCATION:
	case LBIFURCATION:
	case BIFSTEWART:
	case LBIFSTEWART:
	case BIFLAMBDA:
	case LBIFLAMBDA:
		set_trig_array(0, "ident");
		break;

	case BIFEQSINPI:
	case LBIFEQSINPI:
	case BIFADSINPI:
	case LBIFADSINPI:
		set_trig_array(0, "sin");
		break;
	}
}

/* miscellaneous function variable defaults */
void set_function_parm_defaults()
{
	switch (g_fractal_type)
	{
	case FPPOPCORN:
	case LPOPCORN:
	case FPPOPCORNJUL:
	case LPOPCORNJUL:
		set_trig_array(0, "sin");
		set_trig_array(1, "tan");
		set_trig_array(2, "sin");
		set_trig_array(3, "tan");
		break;
	case LATOO:
		set_trig_array(0, "sin");
		set_trig_array(1, "sin");
		set_trig_array(2, "sin");
		set_trig_array(3, "sin");
		break;
	}
}

void backwards_v18()
{
	if (!g_function_preloaded)
	{
		set_if_old_bif(); /* old bifs need function set, JCO 7/5/92 */
	}
	if (g_fractal_type == MANDELTRIG && g_user_float_flag == 1
			&& g_save_release < 1800 && g_bail_out == 0)
	{
		g_bail_out = 2500;
	}
	if (g_fractal_type == LAMBDATRIG && g_user_float_flag == 1
			&& g_save_release < 1800 && g_bail_out == 0)
	{
		g_bail_out = 2500;
	}
}

void backwards_v19()
{
	if (g_fractal_type == MARKSJULIA && g_save_release < 1825)
	{
		if (g_parameters[2] == 0)
		{
			g_parameters[2] = 2;
		}
		else
		{
			g_parameters[2]++;
		}
	}
	if (g_fractal_type == MARKSJULIAFP && g_save_release < 1825)
	{
		if (g_parameters[2] == 0)
		{
			g_parameters[2] = 2;
		}
		else
		{
			g_parameters[2]++;
		}
	}
	if ((g_fractal_type == FORMULA || g_fractal_type == FFORMULA) && g_save_release < 1824)
	{
		g_inversion[0] = g_inversion[1] = g_inversion[2] = g_invert = 0;
	}
	g_no_magnitude_calculation = fix_bof() ? TRUE : FALSE; /* fractal has old bof60/61 problem with magnitude */
	g_use_old_periodicity = fix_period_bof() ? TRUE : FALSE; /* fractal uses old periodicity method */
	g_use_old_distance_test = (g_save_release < 1827 && g_distance_test) ? TRUE : FALSE; /* use old distest code */
}

void backwards_v20()
{
	/* Fractype == FP type is not seen from PAR file ????? */
	g_bad_outside = ((g_fractal_type == MANDELFP || g_fractal_type == JULIAFP
						|| g_fractal_type == MANDEL || g_fractal_type == JULIA)
					&& (g_outside <= REAL && g_outside >= SUM) && g_save_release <= 1960)
		? 1 : 0;
	g_use_old_complex_power = ((g_fractal_type == FORMULA || g_fractal_type == FFORMULA)
				&& (g_save_release < 1900 || DEBUGFLAG_OLD_POWER == g_debug_flag))
		? 1 : 0;
	if (g_inside == EPSCROSS && g_save_release < 1961)
	{
		g_proximity = 0.01;
	}
	if (!g_function_preloaded)
	{
		set_function_parm_defaults();
	}
}

int check_back()
{
	/*
		put the features that need to save the value in g_save_release for backwards
		compatibility in this routine
	*/
	int ret = 0;
	if (g_fractal_type == LYAPUNOV
		|| g_fractal_type == FROTH
		|| g_fractal_type == FROTHFP
		|| fix_bof()
		|| fix_period_bof()
		|| g_use_old_distance_test
		|| g_decomposition[0] == 2
		|| (g_fractal_type == FORMULA && g_save_release <= 1920)
		|| (g_fractal_type == FFORMULA && g_save_release <= 1920)
		|| (g_log_palette_flag != 0 && g_save_release <= 2001)
		|| (g_fractal_type == TRIGSQR && g_save_release < 1900)
		|| (g_inside == STARTRAIL && g_save_release < 1825)
		|| (g_max_iteration > 32767 && g_save_release <= 1950)
		|| (g_distance_test && g_save_release <= 1950)
		|| ((g_outside <= REAL && g_outside >= ATAN) && g_save_release <= 1960)
		|| (g_fractal_type == FPPOPCORN && g_save_release <= 1960)
		|| (g_fractal_type == LPOPCORN && g_save_release <= 1960)
		|| (g_fractal_type == FPPOPCORNJUL && g_save_release <= 1960)
		|| (g_fractal_type == LPOPCORNJUL && g_save_release <= 1960)
		|| (g_inside == FMODI && g_save_release <= 2000)
		|| ((g_inside == ATANI || g_outside == ATAN) && g_save_release <= 2002)
		|| (g_fractal_type == LAMBDATRIGFP && g_trig_index[0] == EXP && g_save_release <= 2002)
		|| ((g_fractal_type == JULIBROT || g_fractal_type == JULIBROTFP)
			&& (g_new_orbit_type == QUATFP || g_new_orbit_type == HYPERCMPLXFP)
			&& g_save_release <= 2002))
	{
		ret = 1;
	}
	return ret;
}

static int fix_bof()
{
	int ret = 0;
	if (g_inside <= BOF60 && g_inside >= BOF61 && g_save_release < 1826)
	{
		if ((g_current_fractal_specific->calculate_type == standard_fractal &&
			(g_current_fractal_specific->flags & BAILTEST) == 0) ||
			(g_fractal_type == FORMULA || g_fractal_type == FFORMULA))
		{
			ret = 1;
		}
	}
	return ret;
}

static int fix_period_bof()
{
	int ret = 0;
	if (g_inside <= BOF60 && g_inside >= BOF61 && g_save_release < 1826)
	{
		ret = 1;
	}
	return ret;
}

/* browse code RB*/

#define MAX_WINDOWS_OPEN 450

struct window  /* for look_get_window on screen browser */
{
	struct coords itl; /* screen coordinates */
	struct coords ibl;
	struct coords itr;
	struct coords ibr;
	double win_size;   /* box size for drawindow() */
	char name[FILE_MAX_FNAME];     /* for filename */
	int box_count;      /* bytes of saved screen info */
};

/* prototypes */
static void drawindow(int, struct window *);
static char is_visible_window
				(struct window *, struct fractal_info *, struct ext_blk_mp_info *);
static void transform(struct dblcoords *);
static char paramsOK(struct fractal_info *);
static char typeOK(struct fractal_info *, struct ext_blk_formula_info *);
static char functionOK(struct fractal_info *, int);
static void check_history(char *, char *);
static void bfsetup_convert_to_screen();
static void bftransform(bf_t, bf_t, struct dblcoords *);

char g_browse_name[FILE_MAX_FNAME]; /* name for browse file */
struct window browse_windows[MAX_WINDOWS_OPEN] = { 0 };
static int *boxx_storage = NULL;
static int *boxy_storage = NULL;
static int *boxvalues_storage = NULL;

/* here because must be visible inside several routines */
static struct affine *cvt;
static bf_t   bt_a, bt_b, bt_c, bt_d, bt_e, bt_f;
static bf_t   n_a, n_b, n_c, n_d, n_e, n_f;
int oldbf_math;

/* look_get_window reads all .GIF files and draws window outlines on the screen */
int look_get_window()
{
	struct affine stack_cvt;
	struct fractal_info read_info;
	struct ext_blk_resume_info resume_info_blk;
	struct ext_blk_formula_info formula_info;
	struct ext_blk_ranges_info ranges_info;
	struct ext_blk_mp_info mp_info;
	struct ext_blk_evolver_info evolver_info;
	struct ext_blk_orbits_info orbits_info;
	time_t thistime, lastime;
	char mesg[40], newname[60], oldname[60];
	int c, i, index, done, wincount, toggle, color_of_box;
	struct window winlist;
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

	push_help_mode(HELPBROWSE);
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
	g_no_sub_images = FALSE;
	split_path(g_read_name, drive, dir, NULL, NULL);
	split_path(g_browse_mask, NULL, NULL, fname, ext);
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
			&& (typeOK(&read_info, &formula_info) || !g_browse_check_type)
			&& (paramsOK(&read_info) || !g_browse_check_parameters)
			&& stricmp(g_browse_name, g_dta.filename)
			&& evolver_info.got_data != 1
			&& is_visible_window(&winlist, &read_info, &mp_info))
		{
			strcpy(winlist.name, g_dta.filename);
			drawindow(color_of_box, &winlist);
			g_box_count <<= 1; /*g_box_count*2;*/ /* double for byte count */
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
	c = 0;
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
				for (i = 0; i < wincount; i++)
				{
					winlist = browse_windows[i];
					drawindow(color_of_box, &winlist);
				}
				winlist = browse_windows[index];
				drawindow(color_of_box, &winlist);
				break;

			case FIK_CTL_DEL:
				color_of_box -= key_count(FIK_CTL_DEL);
				for (i = 0; i < wincount; i++)
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
				strcpy(g_browse_name, winlist.name);
				done = 1;
				break;

			case FIK_ESC:
			case 'l':
			case 'L':
#ifdef XFRACT
				/* Need all boxes turned on, turn last one back on. */
				drawindow(g_color_bright, &winlist);
#endif
				g_auto_browse = FALSE;
				done = 2;
				break;

			case 'D': /* delete file */
				clear_temp_message();
				_snprintf(mesg, NUM_OF(mesg), "Delete %s? (Y/N)", winlist.name);
				show_temp_message(mesg);
				driver_wait_key_pressed(0);
				clear_temp_message();
				c = driver_get_key();
				if (c == 'Y' && g_double_caution)
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
				i = field_prompt(mesg, NULL, newname, 60, NULL);
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
				g_auto_browse = FALSE;
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
		g_no_sub_images = TRUE;
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
	g_float_flag = g_user_float_flag;
	pop_help_mode();

	return c;
}


static void drawindow(int colour, struct window *info)
{
#ifndef XFRACT
	int cross_size;
	struct coords ibl, itr;
#endif

	g_box_color = colour;
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

/* maps points onto view screen*/
static void transform(struct dblcoords *point)
{
	double tmp_pt_x;
	tmp_pt_x = cvt->a*point->x + cvt->b*point->y + cvt->e;
	point->y = cvt->c*point->x + cvt->d*point->y + cvt->f;
	point->x = tmp_pt_x;
}

static char is_visible_window(struct window *list, struct fractal_info *info,
	struct ext_blk_mp_info *mp_info)
{
	struct dblcoords tl, tr, bl, br;
	bf_t bt_x, bt_y;
	bf_t bt_xmin, bt_xmax, bt_ymin, bt_ymax, bt_x3rd, bt_y3rd;
	int saved;
	int two_len;
	int cornercount, cant_see;
	int  orig_bflength,
		orig_bnlength,
		orig_padding,
		orig_rlength,
		orig_shiftfactor,
		orig_rbflength;
	double toobig, tmp_sqrt;
	toobig = sqrt(sqr((double)g_screen_width) + sqr((double)g_screen_height))*1.5;
	/* arbitrary value... stops browser zooming out too far */
	cornercount = 0;
	cant_see = 0;

	saved = save_stack();
	/* Save original values. */
	orig_bflength      = bflength;
	orig_bnlength      = bnlength;
	orig_padding       = padding;
	orig_rlength       = rlength;
	orig_shiftfactor   = shiftfactor;
	orig_rbflength     = rbflength;

	two_len = bflength + 2;
	bt_x = alloc_stack(two_len);
	bt_y = alloc_stack(two_len);
	bt_xmin = alloc_stack(two_len);
	bt_xmax = alloc_stack(two_len);
	bt_ymin = alloc_stack(two_len);
	bt_ymax = alloc_stack(two_len);
	bt_x3rd = alloc_stack(two_len);
	bt_y3rd = alloc_stack(two_len);

	if (info->bf_math)
	{
		bf_t   bt_t1, bt_t2, bt_t3, bt_t4, bt_t5, bt_t6;
		int di_bflength, two_di_len, two_rbf;

		di_bflength = info->bflength + bnstep;
		two_di_len = di_bflength + 2;
		two_rbf = rbflength + 2;

		n_a     = alloc_stack(two_rbf);
		n_b     = alloc_stack(two_rbf);
		n_c     = alloc_stack(two_rbf);
		n_d     = alloc_stack(two_rbf);
		n_e     = alloc_stack(two_rbf);
		n_f     = alloc_stack(two_rbf);

		convert_bf(n_a, bt_a, rbflength, orig_rbflength);
		convert_bf(n_b, bt_b, rbflength, orig_rbflength);
		convert_bf(n_c, bt_c, rbflength, orig_rbflength);
		convert_bf(n_d, bt_d, rbflength, orig_rbflength);
		convert_bf(n_e, bt_e, rbflength, orig_rbflength);
		convert_bf(n_f, bt_f, rbflength, orig_rbflength);

		bt_t1   = alloc_stack(two_di_len);
		bt_t2   = alloc_stack(two_di_len);
		bt_t3   = alloc_stack(two_di_len);
		bt_t4   = alloc_stack(two_di_len);
		bt_t5   = alloc_stack(two_di_len);
		bt_t6   = alloc_stack(two_di_len);

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
	if (oldbf_math || info->bf_math)
	{
		if (!info->bf_math)
		{
			floattobf(bt_x, info->x_min);
			floattobf(bt_y, info->y_max);
		}
		else
		{
			copy_bf(bt_x, bt_xmin);
			copy_bf(bt_y, bt_ymax);
		}
		bftransform(bt_x, bt_y, &tl);
	}
	else
	{
		tl.x = info->x_min;
		tl.y = info->y_max;
		transform(&tl);
	}
	list->itl.x = (int)(tl.x + 0.5);
	list->itl.y = (int)(tl.y + 0.5);
	if (oldbf_math || info->bf_math)
	{
		if (!info->bf_math)
		{
			floattobf(bt_x, (info->x_max)-(info->x_3rd-info->x_min));
			floattobf(bt_y, (info->y_max) + (info->y_min-info->y_3rd));
		}
		else
		{
			neg_a_bf(sub_bf(bt_x, bt_x3rd, bt_xmin));
			add_a_bf(bt_x, bt_xmax);
			sub_bf(bt_y, bt_ymin, bt_y3rd);
			add_a_bf(bt_y, bt_ymax);
		}
		bftransform(bt_x, bt_y, &tr);
	}
	else
	{
		tr.x = (info->x_max)-(info->x_3rd-info->x_min);
		tr.y = (info->y_max) + (info->y_min-info->y_3rd);
		transform(&tr);
	}
	list->itr.x = (int)(tr.x + 0.5);
	list->itr.y = (int)(tr.y + 0.5);
	if (oldbf_math || info->bf_math)
	{
		if (!info->bf_math)
		{
			floattobf(bt_x, info->x_3rd);
			floattobf(bt_y, info->y_3rd);
		}
		else
		{
			copy_bf(bt_x, bt_x3rd);
			copy_bf(bt_y, bt_y3rd);
		}
		bftransform(bt_x, bt_y, &bl);
	}
	else
	{
		bl.x = info->x_3rd;
		bl.y = info->y_3rd;
		transform(&bl);
	}
	list->ibl.x = (int)(bl.x + 0.5);
	list->ibl.y = (int)(bl.y + 0.5);
	if (oldbf_math || info->bf_math)
	{
		if (!info->bf_math)
		{
			floattobf(bt_x, info->x_max);
			floattobf(bt_y, info->y_min);
		}
		else
		{
			copy_bf(bt_x, bt_xmax);
			copy_bf(bt_y, bt_ymin);
		}
		bftransform(bt_x, bt_y, &br);
	}
	else
	{
		br.x = info->x_max;
		br.y = info->y_min;
		transform(&br);
	}
	list->ibr.x = (int)(br.x + 0.5);
	list->ibr.y = (int)(br.y + 0.5);

	tmp_sqrt = sqrt(sqr(tr.x-bl.x) + sqr(tr.y-bl.y));
	list->win_size = tmp_sqrt; /* used for box vs crosshair in drawindow() */
	if (tmp_sqrt < g_too_small)
	{
		cant_see = 1;
	}
	/* reject anything too small onscreen */
	if (tmp_sqrt > toobig)
	{
		cant_see = 1;
	}
	/* or too big... */

	/* restore original values */
	bflength      = orig_bflength;
	bnlength      = orig_bnlength;
	padding       = orig_padding;
	rlength       = orig_rlength;
	shiftfactor   = orig_shiftfactor;
	rbflength     = orig_rbflength;

	restore_stack(saved);
	if (cant_see) /* do it this way so bignum stack is released */
	{
		return FALSE;
	}

	/* now see how many corners are on the screen, accept if one or more */
	if (tl.x >= -g_sx_offset && tl.x <= (g_screen_width-g_sx_offset) && tl.y >= (0-g_sy_offset) && tl.y <= (g_screen_height-g_sy_offset))
	{
		cornercount ++;
	}
	if (bl.x >= -g_sx_offset && bl.x <= (g_screen_width-g_sx_offset) && bl.y >= (0-g_sy_offset) && bl.y <= (g_screen_height-g_sy_offset))
	{
		cornercount ++;
	}
	if (tr.x >= -g_sx_offset && tr.x <= (g_screen_width-g_sx_offset) && tr.y >= (0-g_sy_offset) && tr.y <= (g_screen_height-g_sy_offset))
	{
		cornercount ++;
	}
	if (br.x >= -g_sx_offset && br.x <= (g_screen_width-g_sx_offset) && br.y >= (0-g_sy_offset) && br.y <= (g_screen_height-g_sy_offset))
	{
		cornercount ++;
	}

	return (cornercount >= 1) ? TRUE : FALSE;
}

static char paramsOK(struct fractal_info *info)
{
	double tmpparm3, tmpparm4;
	double tmpparm5, tmpparm6;
	double tmpparm7, tmpparm8;
	double tmpparm9, tmpparm10;
#define MINDIF 0.001

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

static char functionOK(struct fractal_info *info, int numfn)
{
	int i, mzmatch;
	mzmatch = 0;
	for (i = 0; i < numfn; i++)
	{
		if (info->trig_index[i] != g_trig_index[i])
		{
			mzmatch++;
		}
	}
	return (mzmatch > 0) ? 0 : 1;
}

static char typeOK(struct fractal_info *info, struct ext_blk_formula_info *formula_info)
{
	int numfn;
	if ((g_fractal_type == FORMULA || g_fractal_type == FFORMULA) &&
		(info->fractal_type == FORMULA || info->fractal_type == FFORMULA))
	{
		if (!stricmp(formula_info->form_name, g_formula_name))
		{
			numfn = g_max_fn;
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

static void check_history (char *oldname, char *newname)
{
int i;

/* g_file_name_stack[] is maintained in framain2.c.  It is the history */
/*  file for the browser and holds a maximum of 16 images.  The history */
/*  file needs to be adjusted if the rename or delete functions of the */
/*  browser are used. */
/* g_name_stack_ptr is also maintained in framain2.c.  It is the index into */
/*  g_file_name_stack[]. */

	for (i = 0; i < g_name_stack_ptr; i++)
	{
		if (stricmp(g_file_name_stack[i], oldname) == 0) /* we have a match */
		{
			strcpy(g_file_name_stack[i], newname);    /* insert the new name */
		}
	}
}

static void bfsetup_convert_to_screen()
{
	/* setup_convert_to_screen() in LORENZ.C, converted to bf_math */
	/* Call only from within look_get_window() */
	bf_t   bt_det, bt_xd, bt_yd, bt_tmp1, bt_tmp2;
	bf_t   bt_inter1, bt_inter2;
	int saved;

	saved = save_stack();
	bt_inter1 = alloc_stack(rbflength + 2);
	bt_inter2 = alloc_stack(rbflength + 2);
	bt_det = alloc_stack(rbflength + 2);
	bt_xd  = alloc_stack(rbflength + 2);
	bt_yd  = alloc_stack(rbflength + 2);
	bt_tmp1 = alloc_stack(rbflength + 2);
	bt_tmp2 = alloc_stack(rbflength + 2);

	/* g_xx_3rd-g_xx_min */
	sub_bf(bt_inter1, bfx3rd, bfxmin);
	/* g_yy_min-g_yy_max */
	sub_bf(bt_inter2, bfymin, bfymax);
	/* (g_xx_3rd-g_xx_min)*(g_yy_min-g_yy_max) */
	mult_bf(bt_tmp1, bt_inter1, bt_inter2);

	/* g_yy_max-g_yy_3rd */
	sub_bf(bt_inter1, bfymax, bfy3rd);
	/* g_xx_max-g_xx_min */
	sub_bf(bt_inter2, bfxmax, bfxmin);
	/* (g_yy_max-g_yy_3rd)*(g_xx_max-g_xx_min) */
	mult_bf(bt_tmp2, bt_inter1, bt_inter2);

	/* det = (g_xx_3rd-g_xx_min)*(g_yy_min-g_yy_max) + (g_yy_max-g_yy_3rd)*(g_xx_max-g_xx_min) */
	add_bf(bt_det, bt_tmp1, bt_tmp2);

	/* xd = g_dx_size/det */
	floattobf(bt_tmp1, g_dx_size);
	div_bf(bt_xd, bt_tmp1, bt_det);

	/* a =  xd*(g_yy_max-g_yy_3rd) */
	sub_bf(bt_inter1, bfymax, bfy3rd);
	mult_bf(bt_a, bt_xd, bt_inter1);

	/* b =  xd*(g_xx_3rd-g_xx_min) */
	sub_bf(bt_inter1, bfx3rd, bfxmin);
	mult_bf(bt_b, bt_xd, bt_inter1);

	/* e = -(a*g_xx_min + b*g_yy_max) */
	mult_bf(bt_tmp1, bt_a, bfxmin);
	mult_bf(bt_tmp2, bt_b, bfymax);
	neg_a_bf(add_bf(bt_e, bt_tmp1, bt_tmp2));

	/* g_xx_3rd-g_xx_max */
	sub_bf(bt_inter1, bfx3rd, bfxmax);
	/* g_yy_min-g_yy_max */
	sub_bf(bt_inter2, bfymin, bfymax);
	/* (g_xx_3rd-g_xx_max)*(g_yy_min-g_yy_max) */
	mult_bf(bt_tmp1, bt_inter1, bt_inter2);

	/* g_yy_min-g_yy_3rd */
	sub_bf(bt_inter1, bfymin, bfy3rd);
	/* g_xx_max-g_xx_min */
	sub_bf(bt_inter2, bfxmax, bfxmin);
	/* (g_yy_min-g_yy_3rd)*(g_xx_max-g_xx_min) */
	mult_bf(bt_tmp2, bt_inter1, bt_inter2);

	/* det = (g_xx_3rd-g_xx_max)*(g_yy_min-g_yy_max) + (g_yy_min-g_yy_3rd)*(g_xx_max-g_xx_min) */
	add_bf(bt_det, bt_tmp1, bt_tmp2);

	/* yd = g_dy_size/det */
	floattobf(bt_tmp2, g_dy_size);
	div_bf(bt_yd, bt_tmp2, bt_det);

	/* c =  yd*(g_yy_min-g_yy_3rd) */
	sub_bf(bt_inter1, bfymin, bfy3rd);
	mult_bf(bt_c, bt_yd, bt_inter1);

	/* d =  yd*(g_xx_3rd-g_xx_max) */
	sub_bf(bt_inter1, bfx3rd, bfxmax);
	mult_bf(bt_d, bt_yd, bt_inter1);

	/* f = -(c*g_xx_min + d*g_yy_max) */
	mult_bf(bt_tmp1, bt_c, bfxmin);
	mult_bf(bt_tmp2, bt_d, bfymax);
	neg_a_bf(add_bf(bt_f, bt_tmp1, bt_tmp2));

	restore_stack(saved);
}

/* maps points onto view screen*/
static void bftransform(bf_t bt_x, bf_t bt_y, struct dblcoords *point)
{
	bf_t   bt_tmp1, bt_tmp2;
	int saved;

	saved = save_stack();
	bt_tmp1 = alloc_stack(rbflength + 2);
	bt_tmp2 = alloc_stack(rbflength + 2);

/*  point->x = cvt->a*point->x + cvt->b*point->y + cvt->e; */
	mult_bf(bt_tmp1, n_a, bt_x);
	mult_bf(bt_tmp2, n_b, bt_y);
	add_a_bf(bt_tmp1, bt_tmp2);
	add_a_bf(bt_tmp1, n_e);
	point->x = (double)bftofloat(bt_tmp1);

/*  point->y = cvt->c*point->x + cvt->d*point->y + cvt->f; */
	mult_bf(bt_tmp1, n_c, bt_x);
	mult_bf(bt_tmp2, n_d, bt_y);
	add_a_bf(bt_tmp1, bt_tmp2);
	add_a_bf(bt_tmp1, n_f);
	point->y = (double)bftofloat(bt_tmp1);

	restore_stack(saved);
}
