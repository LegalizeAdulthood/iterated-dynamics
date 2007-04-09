/*  HISTORY.C
	History routines taken out of framain2.c to make them accessable
	to WinFract */

/* see Fractint.c for a description of the "include"  hierarchy */
#include <string.h>
#include "port.h"
#include "prototyp.h"
#include "fractype.h"

static HISTORY *history = NULL;		/* history storage */

static int historyptr = -1;			/* user pointer into history tbl  */
static int saveptr = 0;				/* save ptr into history tbl      */
static int historyflag;				/* are we backing off in history? */

void _fastcall history_save_info(void)
{
	HISTORY current = { 0 };
	HISTORY last;

	if (maxhistory <= 0 || bf_math || !history)
	{
		return;
	}
#if defined(_WIN32)
	_ASSERTE(saveptr >= 0 && saveptr < maxhistory);
#endif
	last = history[saveptr];

	memset((void *) &current, 0, sizeof(HISTORY));
	current.fractal_type		= (short) fractype;
	current.xmin				= xxmin;
	current.xmax				= xxmax;
	current.ymin				= yymin;
	current.ymax				= yymax;
	current.g_c_real				= param[0];
	current.g_c_imag				= param[1];
	current.dparm3				= param[2];
	current.dparm4				= param[3];
	current.dparm5				= param[4];
	current.dparm6				= param[5];
	current.dparm7				= param[6];
	current.dparm8				= param[7];
	current.dparm9				= param[8];
	current.dparm10				= param[9];
	current.fill_color			= (short) g_fill_color;
	current.potential[0]		= potparam[0];
	current.potential[1]		= potparam[1];
	current.potential[2]		= potparam[2];
	current.random_flag				= (short) g_random_flag;
	current.random_seed				= (short) g_random_seed;
	current.inside				= (short) g_inside;
	current.logmap				= g_log_palette_flag;
	current.invert[0]			= g_inversion[0];
	current.invert[1]			= g_inversion[1];
	current.invert[2]			= g_inversion[2];
	current.decomposition				= (short) g_decomposition[0]; ;
	current.biomorph			= (short) g_biomorph;
	current.symmetry			= (short) g_force_symmetry;
	current.init_3d[0]			= (short) g_init_3d[0];
	current.init_3d[1]			= (short) g_init_3d[1];
	current.init_3d[2]			= (short) g_init_3d[2];
	current.init_3d[3]			= (short) g_init_3d[3];
	current.init_3d[4]			= (short) g_init_3d[4];
	current.init_3d[5]			= (short) g_init_3d[5];
	current.init_3d[6]			= (short) g_init_3d[6];
	current.init_3d[7]			= (short) g_init_3d[7];
	current.init_3d[8]			= (short) g_init_3d[8];
	current.init_3d[9]			= (short) g_init_3d[9];
	current.init_3d[10]			= (short) g_init_3d[10];
	current.init_3d[11]			= (short) g_init_3d[12];
	current.init_3d[12]			= (short) g_init_3d[13];
	current.init_3d[13]			= (short) g_init_3d[14];
	current.init_3d[14]			= (short) g_init_3d[15];
	current.init_3d[15]			= (short) g_init_3d[16];
	current.previewfactor		= (short) g_preview_factor;
	current.xtrans				= (short) g_x_trans;
	current.ytrans				= (short) g_y_trans;
	current.red_crop_left		= (short) g_red_crop_left;
	current.red_crop_right		= (short) g_red_crop_right;
	current.blue_crop_left		= (short) g_blue_crop_left;
	current.blue_crop_right		= (short) g_blue_crop_right;
	current.red_bright			= (short) g_red_bright;
	current.blue_bright			= (short) g_blue_bright;
	current.xadjust				= (short) g_x_adjust;
	current.yadjust				= (short) g_y_adjust;
	current.eyeseparation		= (short) g_eye_separation;
	current.glassestype			= (short) g_glasses_type;
	current.outside				= (short) g_outside;
	current.x3rd				= xx3rd;
	current.y3rd				= yy3rd;
	current.stdcalcmode			= usr_stdcalcmode;
	current.three_pass			= (char) g_three_pass;
	current.stop_pass			= (short) g_stop_pass;
	current.distance_test				= g_distance_test;
	current.trigndx[0]			= trigndx[0];
	current.trigndx[1]			= trigndx[1];
	current.trigndx[2]			= trigndx[2];
	current.trigndx[3]			= trigndx[3];
	current.finattract			= (short) g_finite_attractor;
	current.initial_orbit_z[0]		= g_initial_orbit_z.x;
	current.initial_orbit_z[1]		= g_initial_orbit_z.y;
	current.use_initial_orbit_z		= g_use_initial_orbit_z;
	current.periodicity			= (short) g_periodicity_check;
	current.potential_16bit		= (short) g_disk_16bit;
	current.release				= (short) g_release;
	current.g_save_release		= (short) g_save_release;
	current.flag3d				= (short) g_display_3d;
	current.ambient				= (short) g_ambient;
	current.randomize			= (short) g_randomize;
	current.haze				= (short) g_haze;
	current.transparent[0]		= (short) g_transparent[0];
	current.transparent[1]		= (short) g_transparent[1];
	current.rotate_lo			= (short) g_rotate_lo;
	current.rotate_hi			= (short) g_rotate_hi;
	current.distance_test_width		= (short) g_distance_test_width;
	current.mxmaxfp				= g_m_x_max_fp;
	current.mxminfp				= g_m_x_min_fp;
	current.mymaxfp				= g_m_y_max_fp;
	current.myminfp				= g_m_y_min_fp;
	current.zdots				= (short) g_z_dots;
	current.originfp			= g_origin_fp;
	current.depthfp				= g_depth_fp;
	current.heightfp			= g_height_fp;
	current.widthfp				= g_width_fp;
	current.screen_distance_fp				= g_screen_distance_fp;
	current.eyesfp				= g_eyes_fp;
	current.orbittype			= (short) g_new_orbit_type;
	current.juli3Dmode			= (short) g_juli_3D_mode;
	current.maxfn				= maxfn;
	current.major_method		= (short) g_major_method;
	current.minor_method		= (short) g_minor_method;
	current.bail_out				= g_bail_out;
	current.bailoutest			= (short) g_bail_out_test;
	current.iterations			= maxit;
	current.g_old_demm_colors		= (short) g_old_demm_colors;
	current.logcalc				= (short) g_log_dynamic_calculate;
	current.ismand				= (short) g_is_mand;
	current.proximity			= g_proximity;
	current.no_bof				= (short) g_no_bof;
	current.orbit_delay			= (short) g_orbit_delay;
	current.orbit_interval		= g_orbit_interval;
	current.oxmin				= g_orbit_x_min;
	current.oxmax				= g_orbit_x_max;
	current.oymin				= g_orbit_y_min;
	current.oymax				= g_orbit_y_max;
	current.ox3rd				= g_orbit_x_3rd;
	current.oy3rd				= g_orbit_y_3rd;
	current.keep_scrn_coords	= (short) g_keep_screen_coords;
	current.drawmode			= (char) g_orbit_draw_mode;
	memcpy(current.dac, g_dac_box, 256*3);
	switch (fractype)
	{
	case FORMULA:
	case FFORMULA:
		strncpy(current.filename, g_formula_filename, FILE_MAX_PATH);
		strncpy(current.itemname, g_formula_name, ITEMNAMELEN + 1);
		break;
	case IFS:
	case IFS3D:
		strncpy(current.filename, g_ifs_filename, FILE_MAX_PATH);
		strncpy(current.itemname, g_ifs_name, ITEMNAMELEN + 1);
		break;
	case LSYSTEM:
		strncpy(current.filename, g_l_system_filename, FILE_MAX_PATH);
		strncpy(current.itemname, g_l_system_name, ITEMNAMELEN + 1);
		break;
	default:
		*(current.filename) = 0;
		*(current.itemname) = 0;
		break;
	}
	if (historyptr == -1)        /* initialize the history file */
	{
		int i;
		for (i = 0; i < maxhistory; i++)
		{
			history[i] = current;
		}
		historyflag = saveptr = historyptr = 0;   /* initialize history ptr */
	}
	else if (historyflag == 1)
	{
		historyflag = 0;   /* coming from user history command, don't save */
	}
	else if (memcmp(&current, &last, sizeof(HISTORY)))
	{
		if (++saveptr >= maxhistory)  /* back to beginning of circular buffer */
		{
			saveptr = 0;
		}
		if (++historyptr >= maxhistory)  /* move user pointer in parallel */
		{
			historyptr = 0;
		}
#if defined(_WIN32)
		_ASSERTE(saveptr >= 0 && saveptr < maxhistory);
#endif
		history[saveptr] = current;
	}
}

void _fastcall history_restore_info(void)
{
	HISTORY last;
	if (maxhistory <= 0 || bf_math || history == 0)
	{
		return;
	}
#if defined(_WIN32)
	_ASSERTE(historyptr >= 0 && historyptr < maxhistory);
#endif
	last = history[historyptr];

	g_invert				= 0;
	g_calculation_status				= CALCSTAT_PARAMS_CHANGED;
	g_resuming				= 0;
	fractype				= last.fractal_type;
	xxmin               	= last.xmin;
	xxmax               	= last.xmax;
	yymin               	= last.ymin;
	yymax               	= last.ymax;
	param[0]            	= last.g_c_real;
	param[1]            	= last.g_c_imag;
	param[2]            	= last.dparm3;
	param[3]            	= last.dparm4;
	param[4]            	= last.dparm5;
	param[5]            	= last.dparm6;
	param[6]            	= last.dparm7;
	param[7]            	= last.dparm8;
	param[8]            	= last.dparm9;
	param[9]            	= last.dparm10;
	g_fill_color           	= last.fill_color;
	potparam[0]         	= last.potential[0];
	potparam[1]         	= last.potential[1];
	potparam[2]         	= last.potential[2];
	g_random_flag			= last.random_flag;
	g_random_seed			= last.random_seed;
	g_inside              	= last.inside;
	g_log_palette_flag             	= last.logmap;
	g_inversion[0]        	= last.invert[0];
	g_inversion[1]        	= last.invert[1];
	g_inversion[2]        	= last.invert[2];
	g_decomposition[0]		= last.decomposition;
	g_user_biomorph        	= last.biomorph;
	g_biomorph            	= last.biomorph;
	g_force_symmetry       	= last.symmetry;
	g_init_3d[0]           	= last.init_3d[0];
	g_init_3d[1]           	= last.init_3d[1];
	g_init_3d[2]           	= last.init_3d[2];
	g_init_3d[3]           	= last.init_3d[3];
	g_init_3d[4]           	= last.init_3d[4];
	g_init_3d[5]           	= last.init_3d[5];
	g_init_3d[6]           	= last.init_3d[6];
	g_init_3d[7]           	= last.init_3d[7];
	g_init_3d[8]           	= last.init_3d[8];
	g_init_3d[9]           	= last.init_3d[9];
	g_init_3d[10]          	= last.init_3d[10];
	g_init_3d[12]          	= last.init_3d[11];
	g_init_3d[13]          	= last.init_3d[12];
	g_init_3d[14]          	= last.init_3d[13];
	g_init_3d[15]          	= last.init_3d[14];
	g_init_3d[16]          	= last.init_3d[15];
	g_preview_factor       	= last.previewfactor;
	g_x_trans              	= last.xtrans;
	g_y_trans              	= last.ytrans;
	g_red_crop_left       	= last.red_crop_left;
	g_red_crop_right      	= last.red_crop_right;
	g_blue_crop_left      	= last.blue_crop_left;
	g_blue_crop_right     	= last.blue_crop_right;
	g_red_bright          	= last.red_bright;
	g_blue_bright         	= last.blue_bright;
	g_x_adjust             	= last.xadjust;
	g_y_adjust             	= last.yadjust;
	g_eye_separation    	= last.eyeseparation;
	g_glasses_type      	= last.glassestype;
	g_outside             	= last.outside;
	xx3rd               	= last.x3rd;
	yy3rd               	= last.y3rd;
	usr_stdcalcmode     	= last.stdcalcmode;
	stdcalcmode         	= last.stdcalcmode;
	g_three_pass          	= (int) last.three_pass;
	g_stop_pass            	= last.stop_pass;
	g_distance_test			= last.distance_test;
	usr_distest         	= last.distance_test;
	trigndx[0]          	= last.trigndx[0];
	trigndx[1]          	= last.trigndx[1];
	trigndx[2]          	= last.trigndx[2];
	trigndx[3]          	= last.trigndx[3];
	g_finite_attractor		= last.finattract;
	g_initial_orbit_z.x		= last.initial_orbit_z[0];
	g_initial_orbit_z.y		= last.initial_orbit_z[1];
	g_use_initial_orbit_z	= last.use_initial_orbit_z;
	g_periodicity_check    	= last.periodicity;
	usr_periodicitycheck	= last.periodicity;
	g_disk_16bit           	= last.potential_16bit;
	g_release           	= last.release;
	g_save_release        	= last.g_save_release;
	g_display_3d           	= last.flag3d;
	g_ambient             	= last.ambient;
	g_randomize           	= last.randomize;
	g_haze                	= last.haze;
	g_transparent[0]      	= last.transparent[0];
	g_transparent[1]      	= last.transparent[1];
	g_rotate_lo           	= last.rotate_lo;
	g_rotate_hi           	= last.rotate_hi;
	g_distance_test_width	= last.distance_test_width;
	g_m_x_max_fp			= last.mxmaxfp;
	g_m_x_min_fp			= last.mxminfp;
	g_m_y_max_fp			= last.mymaxfp;
	g_m_y_min_fp			= last.myminfp;
	g_z_dots               	= last.zdots;
	g_origin_fp            	= last.originfp;
	g_depth_fp             	= last.depthfp;
	g_height_fp            	= last.heightfp;
	g_width_fp             	= last.widthfp;
	g_screen_distance_fp              	= last.screen_distance_fp;
	g_eyes_fp              	= last.eyesfp;
	g_new_orbit_type        = last.orbittype;
	g_juli_3D_mode			= last.juli3Dmode;
	maxfn               	= last.maxfn;
	g_major_method        	= (enum Major) last.major_method;
	g_minor_method        	= (enum Minor) last.minor_method;
	g_bail_out             	= last.bail_out;
	g_bail_out_test			= (enum bailouts) last.bailoutest;
	maxit               	= last.iterations;
	g_old_demm_colors     	= last.g_old_demm_colors;
	g_current_fractal_specific  	= &g_fractal_specific[fractype];
	g_potential_flag		= (potparam[0] != 0.0);
	if (g_inversion[0] != 0.0)
	{
		g_invert = 3;
	}
	g_log_dynamic_calculate			= last.logcalc;
	g_is_mand				= last.ismand;
	g_proximity				= last.proximity;
	g_no_bof					= last.no_bof;
	g_orbit_delay				= last.orbit_delay;
	g_orbit_interval		= last.orbit_interval;
	g_orbit_x_min			= last.oxmin;
	g_orbit_x_max			= last.oxmax;
	g_orbit_y_min			= last.oymin;
	g_orbit_y_max			= last.oymax;
	g_orbit_x_3rd			= last.ox3rd;
	g_orbit_y_3rd			= last.oy3rd;
	g_keep_screen_coords	= last.keep_scrn_coords;
	if (g_keep_screen_coords)
	{
		g_set_orbit_corners = 1;
	}
	g_orbit_draw_mode		= (int) last.drawmode;
	usr_floatflag			= (char) (g_current_fractal_specific->isinteger ? 0 : 1);
	memcpy(g_dac_box, last.dac, 256*3);
	memcpy(olddacbox, last.dac, 256*3);
	if (g_map_dac_box)
	{
		memcpy(g_map_dac_box, last.dac, 256*3);
	}
	spindac(0, 1);
	savedac = (fractype == JULIBROT || fractype == JULIBROTFP) ? SAVEDAC_NO : SAVEDAC_YES;
	switch (fractype)
	{
	case FORMULA:
	case FFORMULA:
		strncpy(g_formula_filename, last.filename, FILE_MAX_PATH);
		strncpy(g_formula_name, last.itemname, ITEMNAMELEN + 1);
		break;
	case IFS:
	case IFS3D:
		strncpy(g_ifs_filename, last.filename, FILE_MAX_PATH);
		strncpy(g_ifs_name, last.itemname, ITEMNAMELEN + 1);
		break;
	case LSYSTEM:
		strncpy(g_l_system_filename, last.filename, FILE_MAX_PATH);
		strncpy(g_l_system_name, last.itemname, ITEMNAMELEN + 1);
		break;
	default:
		break;
	}
}

void history_allocate(void)
{
	while (maxhistory > 0) /* decrease history if necessary */
	{
		history = (HISTORY *) malloc(sizeof(HISTORY)*maxhistory);
		if (history)
		{
			break;
		}
		maxhistory--;
	}
}

void history_free(void)
{
	if (history != NULL)
	{
		free(history);
	}
}

void history_back(void)
{
	--historyptr;
	if (historyptr <= 0)
	{
		historyptr = maxhistory - 1;
	}
	historyflag = 1;
}

void history_forward(void)
{
	++historyptr;
	if (historyptr >= maxhistory)
	{
		historyptr = 0;
	}
	historyflag = 1;
}
