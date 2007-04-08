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
	current.creal				= param[0];
	current.cimag				= param[1];
	current.dparm3				= param[2];
	current.dparm4				= param[3];
	current.dparm5				= param[4];
	current.dparm6				= param[5];
	current.dparm7				= param[6];
	current.dparm8				= param[7];
	current.dparm9				= param[8];
	current.dparm10				= param[9];
	current.fillcolor			= (short) fillcolor;
	current.potential[0]		= potparam[0];
	current.potential[1]		= potparam[1];
	current.potential[2]		= potparam[2];
	current.rflag				= (short) rflag;
	current.rseed				= (short) rseed;
	current.inside				= (short) inside;
	current.logmap				= LogFlag;
	current.invert[0]			= inversion[0];
	current.invert[1]			= inversion[1];
	current.invert[2]			= inversion[2];
	current.decomp				= (short) decomp[0]; ;
	current.biomorph			= (short) g_biomorph;
	current.symmetry			= (short) g_force_symmetry;
	current.init3d[0]			= (short) init3d[0];
	current.init3d[1]			= (short) init3d[1];
	current.init3d[2]			= (short) init3d[2];
	current.init3d[3]			= (short) init3d[3];
	current.init3d[4]			= (short) init3d[4];
	current.init3d[5]			= (short) init3d[5];
	current.init3d[6]			= (short) init3d[6];
	current.init3d[7]			= (short) init3d[7];
	current.init3d[8]			= (short) init3d[8];
	current.init3d[9]			= (short) init3d[9];
	current.init3d[10]			= (short) init3d[10];
	current.init3d[11]			= (short) init3d[12];
	current.init3d[12]			= (short) init3d[13];
	current.init3d[13]			= (short) init3d[14];
	current.init3d[14]			= (short) init3d[15];
	current.init3d[15]			= (short) init3d[16];
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
	current.outside				= (short) outside;
	current.x3rd				= xx3rd;
	current.y3rd				= yy3rd;
	current.stdcalcmode			= usr_stdcalcmode;
	current.three_pass			= (char) g_three_pass;
	current.stop_pass			= (short) g_stop_pass;
	current.distest				= distest;
	current.trigndx[0]			= trigndx[0];
	current.trigndx[1]			= trigndx[1];
	current.trigndx[2]			= trigndx[2];
	current.trigndx[3]			= trigndx[3];
	current.finattract			= (short) finattract;
	current.initorbit[0]		= initorbit.x;
	current.initorbit[1]		= initorbit.y;
	current.useinitorbit		= useinitorbit;
	current.periodicity			= (short) g_periodicity_check;
	current.potential_16bit		= (short) disk16bit;
	current.release				= (short) g_release;
	current.save_release		= (short) save_release;
	current.flag3d				= (short) display3d;
	current.ambient				= (short) g_ambient;
	current.randomize			= (short) g_randomize;
	current.haze				= (short) g_haze;
	current.transparent[0]		= (short) transparent[0];
	current.transparent[1]		= (short) transparent[1];
	current.rotate_lo			= (short) rotate_lo;
	current.rotate_hi			= (short) rotate_hi;
	current.distestwidth		= (short) distestwidth;
	current.mxmaxfp				= g_m_x_max_fp;
	current.mxminfp				= g_m_x_min_fp;
	current.mymaxfp				= g_m_y_max_fp;
	current.myminfp				= g_m_y_min_fp;
	current.zdots				= (short) g_z_dots;
	current.originfp			= g_origin_fp;
	current.depthfp				= g_depth_fp;
	current.heightfp			= g_height_fp;
	current.widthfp				= g_width_fp;
	current.distfp				= g_dist_fp;
	current.eyesfp				= g_eyes_fp;
	current.orbittype			= (short) g_new_orbit_type;
	current.juli3Dmode			= (short) g_juli_3D_mode;
	current.maxfn				= maxfn;
	current.major_method		= (short) g_major_method;
	current.minor_method		= (short) g_minor_method;
	current.bailout				= bailout;
	current.bailoutest			= (short) g_bail_out_test;
	current.iterations			= maxit;
	current.g_old_demm_colors		= (short) g_old_demm_colors;
	current.logcalc				= (short) Log_Fly_Calc;
	current.ismand				= (short) g_is_mand;
	current.proximity			= g_proximity;
	current.nobof				= (short) nobof;
	current.orbit_delay			= (short) orbit_delay;
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
		strncpy(current.filename, FormFileName, FILE_MAX_PATH);
		strncpy(current.itemname, FormName, ITEMNAMELEN + 1);
		break;
	case IFS:
	case IFS3D:
		strncpy(current.filename, IFSFileName, FILE_MAX_PATH);
		strncpy(current.itemname, IFSName, ITEMNAMELEN + 1);
		break;
	case LSYSTEM:
		strncpy(current.filename, LFileName, FILE_MAX_PATH);
		strncpy(current.itemname, LName, ITEMNAMELEN + 1);
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

	g_invert					= 0;
	calc_status				= CALCSTAT_PARAMS_CHANGED;
	g_resuming				= 0;
	fractype				= last.fractal_type;
	xxmin               	= last.xmin;
	xxmax               	= last.xmax;
	yymin               	= last.ymin;
	yymax               	= last.ymax;
	param[0]            	= last.creal;
	param[1]            	= last.cimag;
	param[2]            	= last.dparm3;
	param[3]            	= last.dparm4;
	param[4]            	= last.dparm5;
	param[5]            	= last.dparm6;
	param[6]            	= last.dparm7;
	param[7]            	= last.dparm8;
	param[8]            	= last.dparm9;
	param[9]            	= last.dparm10;
	fillcolor           	= last.fillcolor;
	potparam[0]         	= last.potential[0];
	potparam[1]         	= last.potential[1];
	potparam[2]         	= last.potential[2];
	rflag               	= last.rflag;
	rseed               	= last.rseed;
	inside              	= last.inside;
	LogFlag             	= last.logmap;
	inversion[0]        	= last.invert[0];
	inversion[1]        	= last.invert[1];
	inversion[2]        	= last.invert[2];
	decomp[0]           	= last.decomp;
	g_user_biomorph        	= last.biomorph;
	g_biomorph            	= last.biomorph;
	g_force_symmetry       	= last.symmetry;
	init3d[0]           	= last.init3d[0];
	init3d[1]           	= last.init3d[1];
	init3d[2]           	= last.init3d[2];
	init3d[3]           	= last.init3d[3];
	init3d[4]           	= last.init3d[4];
	init3d[5]           	= last.init3d[5];
	init3d[6]           	= last.init3d[6];
	init3d[7]           	= last.init3d[7];
	init3d[8]           	= last.init3d[8];
	init3d[9]           	= last.init3d[9];
	init3d[10]          	= last.init3d[10];
	init3d[12]          	= last.init3d[11];
	init3d[13]          	= last.init3d[12];
	init3d[14]          	= last.init3d[13];
	init3d[15]          	= last.init3d[14];
	init3d[16]          	= last.init3d[15];
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
	outside             	= last.outside;
	xx3rd               	= last.x3rd;
	yy3rd               	= last.y3rd;
	usr_stdcalcmode     	= last.stdcalcmode;
	stdcalcmode         	= last.stdcalcmode;
	g_three_pass          	= (int) last.three_pass;
	g_stop_pass            	= last.stop_pass;
	distest             	= last.distest;
	usr_distest         	= last.distest;
	trigndx[0]          	= last.trigndx[0];
	trigndx[1]          	= last.trigndx[1];
	trigndx[2]          	= last.trigndx[2];
	trigndx[3]          	= last.trigndx[3];
	finattract          	= last.finattract;
	initorbit.x         	= last.initorbit[0];
	initorbit.y         	= last.initorbit[1];
	useinitorbit        	= last.useinitorbit;
	g_periodicity_check    	= last.periodicity;
	usr_periodicitycheck	= last.periodicity;
	disk16bit           	= last.potential_16bit;
	g_release           	= last.release;
	save_release        	= last.save_release;
	display3d           	= last.flag3d;
	g_ambient             	= last.ambient;
	g_randomize           	= last.randomize;
	g_haze                	= last.haze;
	transparent[0]      	= last.transparent[0];
	transparent[1]      	= last.transparent[1];
	rotate_lo           	= last.rotate_lo;
	rotate_hi           	= last.rotate_hi;
	distestwidth        	= last.distestwidth;
	g_m_x_max_fp             	= last.mxmaxfp;
	g_m_x_min_fp             	= last.mxminfp;
	g_m_y_max_fp             	= last.mymaxfp;
	g_m_y_min_fp             	= last.myminfp;
	g_z_dots               	= last.zdots;
	g_origin_fp            	= last.originfp;
	g_depth_fp             	= last.depthfp;
	g_height_fp            	= last.heightfp;
	g_width_fp             	= last.widthfp;
	g_dist_fp              	= last.distfp;
	g_eyes_fp              	= last.eyesfp;
	g_new_orbit_type        	= last.orbittype;
	g_juli_3D_mode          	= last.juli3Dmode;
	maxfn               	= last.maxfn;
	g_major_method        	= (enum Major) last.major_method;
	g_minor_method        	= (enum Minor) last.minor_method;
	bailout             	= last.bailout;
	g_bail_out_test          	= (enum bailouts) last.bailoutest;
	maxit               	= last.iterations;
	g_old_demm_colors     	= last.g_old_demm_colors;
	curfractalspecific  	= &fractalspecific[fractype];
	g_potential_flag		= (potparam[0] != 0.0);
	if (inversion[0] != 0.0)
	{
		g_invert = 3;
	}
	Log_Fly_Calc			= last.logcalc;
	g_is_mand					= last.ismand;
	g_proximity				= last.proximity;
	nobof					= last.nobof;
	orbit_delay				= last.orbit_delay;
	g_orbit_interval			= last.orbit_interval;
	g_orbit_x_min					= last.oxmin;
	g_orbit_x_max					= last.oxmax;
	g_orbit_y_min					= last.oymin;
	g_orbit_y_max					= last.oymax;
	g_orbit_x_3rd					= last.ox3rd;
	g_orbit_y_3rd					= last.oy3rd;
	g_keep_screen_coords		= last.keep_scrn_coords;
	if (g_keep_screen_coords)
	{
		g_set_orbit_corners = 1;
	}
	g_orbit_draw_mode		= (int) last.drawmode;
	usr_floatflag			= (char) (curfractalspecific->isinteger ? 0 : 1);
	memcpy(g_dac_box, last.dac, 256*3);
	memcpy(olddacbox, last.dac, 256*3);
	if (mapdacbox)
	{
		memcpy(mapdacbox, last.dac, 256*3);
	}
	spindac(0, 1);
	savedac = (fractype == JULIBROT || fractype == JULIBROTFP) ? SAVEDAC_NO : SAVEDAC_YES;
	switch (fractype)
	{
	case FORMULA:
	case FFORMULA:
		strncpy(FormFileName, last.filename, FILE_MAX_PATH);
		strncpy(FormName, last.itemname, ITEMNAMELEN + 1);
		break;
	case IFS:
	case IFS3D:
		strncpy(IFSFileName, last.filename, FILE_MAX_PATH);
		strncpy(IFSName, last.itemname, ITEMNAMELEN + 1);
		break;
	case LSYSTEM:
		strncpy(LFileName, last.filename, FILE_MAX_PATH);
		strncpy(LName, last.itemname, ITEMNAMELEN + 1);
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
