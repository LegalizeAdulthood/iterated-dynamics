//  history.cpp
//	History routines taken out of framain2.c to make them accessable
//	to WinFract
#include <string.h>
#include <string>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "calcfrac.h"
#include "history.h"
#include "EscapeTime.h"
#include "Externals.h"
#include "FiniteAttractor.h"
#include "Formula.h"
#include "ThreeDimensionalState.h"

struct HISTORY_ITEM
{
	int fractal_type;
	double x_min;
	double x_max;
	double y_min;
	double y_max;
	double c_real;
	double c_imag;
	double potential[3];
	int random_seed;
	bool use_fixed_random_seed;
	int biomorph;
	int inside;
	long logmap;
	double invert[3];
	int decomposition;
	int symmetry;
	short init_3d[16];
	int preview_factor;
	int x_translate;
	int y_translate;
	int red_crop_left;
	int red_crop_right;
	int blue_crop_left;
	int blue_crop_right;
	int red_bright;
	int blue_bright;
	int x_adjust;
	int eye_separation;
	int glasses_type;
	int outside;
	double x_3rd;
	double y_3rd;
	long distance_test;
	int function_index[4];
	int finite_attractor;
	double initial_orbit_z[2];
	int periodicity;
	bool potential_16bit;
	int release;
	int save_release;
	int display_3d;
	int transparent[2];
	int ambient;
	int haze;
	int randomize;
	int rotate_lo;
	int rotate_hi;
	int distance_test_width;
	double dparm3;
	double dparm4;
	int fill_color;
	double m_x_max_fp;
	double m_x_min_fp;
	double m_y_max_fp;
	double m_y_min_fp;
	int z_dots;
	float origin_fp;
	float depth_fp;
	float height_fp;
	float width_fp;
	float screen_distance_fp;
	float eyes_fp;
	int orbit_type;
	int juli_3d_mode;
	MajorMethodType major_method;
	MinorMethodType minor_method;
	double dparm5;
	double dparm6;
	double dparm7;
	double dparm8;
	double dparm9;
	double dparm10;
	long bail_out;
	BailOutType bail_out_test;
	long iterations;
	int bf_math;
	int bflength;
	int yadjust;
	bool old_demm_colors;
	std::string file_name;
	std::string item_name;
	ColormapTable dac;
	int max_fn;
	CalculationMode standard_calculation_mode;
	bool three_pass;
	InitialZType use_initial_orbit_z;
	int log_dynamic_calculate;
	int stop_pass;
	bool is_mandelbrot;
	double proximity;
	bool beauty_of_fractals;
	double math_tolerance[2];
	int orbit_delay;
	long orbit_interval;
	double oxmin;
	double oxmax;
	double oymin;
	double oymax;
	double ox3rd;
	double oy3rd;
	bool keep_screen_coordinates;
	int draw_mode;
};

template <typename T, int N>
bool equals(const T lhs[N], const T rhs[N])
{
	for (int i = 0; i < N; i++)
	{
		if (lhs[i] != rhs[i])
		{
			return false;
		}
	}
	return true;
}

template <typename T, int N, int M>
bool equals(const T lhs[N][M], const T rhs[N][M])
{
	for (int i = 0; i < N; i++)
	{
		if (!equals<T, N>(lhs[i], rhs[i]))
		{
			return false;
		}
	}
	return true;
}

bool operator==(const HISTORY_ITEM &lhs, const HISTORY_ITEM &rhs)
{
	return (lhs.fractal_type == rhs.fractal_type)
		&& (lhs.x_min == rhs.x_min)
		&& (lhs.x_max == rhs.x_max)
		&& (lhs.y_min == rhs.y_min)
		&& (lhs.y_max == rhs.y_max)
		&& (lhs.c_real == rhs.c_real)
		&& (lhs.c_imag == rhs.c_imag)
		&& (lhs.potential == rhs.potential)
		&& (lhs.random_seed == rhs.random_seed)
		&& (lhs.use_fixed_random_seed == rhs.use_fixed_random_seed)
		&& (lhs.biomorph == rhs.biomorph)
		&& (lhs.inside == rhs.inside)
		&& (lhs.logmap == rhs.logmap)
		&& equals<double, 3>(lhs.invert, rhs.invert)
		&& (lhs.decomposition == rhs.decomposition)
		&& (lhs.symmetry == rhs.symmetry)
		&& equals<short, 16>(lhs.init_3d, rhs.init_3d)
		&& (lhs.preview_factor == rhs.preview_factor)
		&& (lhs.x_translate == rhs.x_translate)
		&& (lhs.y_translate == rhs.y_translate)
		&& (lhs.red_crop_left == rhs.red_crop_left)
		&& (lhs.red_crop_right == rhs.red_crop_right)
		&& (lhs.blue_crop_left == rhs.blue_crop_left)
		&& (lhs.blue_crop_right == rhs.blue_crop_right)
		&& (lhs.red_bright == rhs.red_bright)
		&& (lhs.blue_bright == rhs.blue_bright)
		&& (lhs.x_adjust == rhs.x_adjust)
		&& (lhs.eye_separation == rhs.eye_separation)
		&& (lhs.glasses_type == rhs.glasses_type)
		&& (lhs.outside == rhs.outside)
		&& (lhs.x_3rd == rhs.x_3rd)
		&& (lhs.y_3rd == rhs.y_3rd)
		&& (lhs.distance_test == rhs.distance_test)
		&& equals<int, 4>(lhs.function_index, rhs.function_index)
		&& (lhs.finite_attractor == rhs.finite_attractor)
		&& equals<double, 2>(lhs.initial_orbit_z, rhs.initial_orbit_z)
		&& (lhs.periodicity == rhs.periodicity)
		&& (lhs.potential_16bit == rhs.potential_16bit)
		&& (lhs.release == rhs.release)
		&& (lhs.save_release == rhs.save_release)
		&& (lhs.display_3d == rhs.display_3d)
		&& equals<int, 2>(lhs.transparent, rhs.transparent)
		&& (lhs.ambient == rhs.ambient)
		&& (lhs.haze == rhs.haze)
		&& (lhs.randomize == rhs.randomize)
		&& (lhs.rotate_lo == rhs.rotate_lo)
		&& (lhs.rotate_hi == rhs.rotate_hi)
		&& (lhs.distance_test_width == rhs.distance_test_width)
		&& (lhs.dparm3 == rhs.dparm3)
		&& (lhs.dparm4 == rhs.dparm4)
		&& (lhs.fill_color == rhs.fill_color)
		&& (lhs.m_x_max_fp == rhs.m_x_max_fp)
		&& (lhs.m_x_min_fp == rhs.m_x_min_fp)
		&& (lhs.m_y_max_fp == rhs.m_y_max_fp)
		&& (lhs.m_y_min_fp == rhs.m_y_min_fp)
		&& (lhs.z_dots == rhs.z_dots)
		&& (lhs.origin_fp == rhs.origin_fp)
		&& (lhs.depth_fp == rhs.depth_fp)
		&& (lhs.height_fp == rhs.height_fp)
		&& (lhs.width_fp == rhs.width_fp)
		&& (lhs.screen_distance_fp == rhs.screen_distance_fp)
		&& (lhs.eyes_fp == rhs.eyes_fp)
		&& (lhs.orbit_type == rhs.orbit_type)
		&& (lhs.juli_3d_mode == rhs.juli_3d_mode)
		&& (lhs.major_method == rhs.major_method)
		&& (lhs.minor_method == rhs.minor_method)
		&& (lhs.dparm5 == rhs.dparm5)
		&& (lhs.dparm6 == rhs.dparm6)
		&& (lhs.dparm7 == rhs.dparm7)
		&& (lhs.dparm8 == rhs.dparm8)
		&& (lhs.dparm9 == rhs.dparm9)
		&& (lhs.dparm10 == rhs.dparm10)
		&& (lhs.bail_out == rhs.bail_out)
		&& (lhs.bail_out_test == rhs.bail_out_test)
		&& (lhs.iterations == rhs.iterations)
		&& (lhs.bf_math == rhs.bf_math)
		&& (lhs.bflength == rhs.bflength)
		&& (lhs.yadjust == rhs.yadjust)
		&& (lhs.old_demm_colors == rhs.old_demm_colors)
		&& (lhs.file_name == rhs.file_name)
		&& (lhs.item_name == rhs.item_name)
		&& (lhs.dac == rhs.dac)
		&& (lhs.max_fn == rhs.max_fn)
		&& (lhs.standard_calculation_mode == rhs.standard_calculation_mode)
		&& (lhs.three_pass == rhs.three_pass)
		&& (lhs.use_initial_orbit_z == rhs.use_initial_orbit_z)
		&& (lhs.log_dynamic_calculate == rhs.log_dynamic_calculate)
		&& (lhs.stop_pass == rhs.stop_pass)
		&& (lhs.is_mandelbrot == rhs.is_mandelbrot)
		&& (lhs.proximity == rhs.proximity)
		&& (lhs.beauty_of_fractals == rhs.beauty_of_fractals)
		&& equals<double, 2>(lhs.math_tolerance, rhs.math_tolerance)
		&& (lhs.orbit_delay == rhs.orbit_delay)
		&& (lhs.orbit_interval == rhs.orbit_interval)
		&& (lhs.oxmin == rhs.oxmin)
		&& (lhs.oxmax == rhs.oxmax)
		&& (lhs.oymin == rhs.oymin)
		&& (lhs.oymax == rhs.oymax)
		&& (lhs.ox3rd == rhs.ox3rd)
		&& (lhs.oy3rd == rhs.oy3rd)
		&& (lhs.keep_screen_coordinates == rhs.keep_screen_coordinates)
		&& (lhs.draw_mode == rhs.draw_mode);
}

bool operator!=(const HISTORY_ITEM &lhs, const HISTORY_ITEM &rhs)
{
	return !(lhs == rhs);
}

static HISTORY_ITEM *s_history = 0;		// history storage
static int s_history_index = -1;			// user pointer into history tbl
static int s_save_index = 0;				// save ptr into history tbl
static bool s_history_flag;				// are we backing off in history?

void history_save_info()
{
	if (g_max_history <= 0 || g_bf_math || !s_history)
	{
		return;
	}
	assert(s_save_index >= 0 && s_save_index < g_max_history);
	HISTORY_ITEM last = s_history[s_save_index];

	HISTORY_ITEM current = { 0 };
	current.fractal_type = g_fractal_type;
	current.x_min = g_escape_time_state.m_grid_fp.x_min();
	current.x_max = g_escape_time_state.m_grid_fp.x_max();
	current.y_min = g_escape_time_state.m_grid_fp.y_min();
	current.y_max = g_escape_time_state.m_grid_fp.y_max();
	current.c_real = g_parameters[P1_REAL];
	current.c_imag = g_parameters[P1_IMAG];
	current.dparm3 = g_parameters[P2_REAL];
	current.dparm4 = g_parameters[P2_IMAG];
	current.dparm5 = g_parameters[P3_REAL];
	current.dparm6 = g_parameters[P3_IMAG];
	current.dparm7 = g_parameters[P4_REAL];
	current.dparm8 = g_parameters[P4_IMAG];
	current.dparm9 = g_parameters[P5_REAL];
	current.dparm10 = g_parameters[P5_IMAG];
	current.fill_color = g_fill_color;
	current.potential[0] = g_potential_parameter[0];
	current.potential[1] = g_potential_parameter[1];
	current.potential[2] = g_potential_parameter[2];
	current.use_fixed_random_seed = g_use_fixed_random_seed;
	current.random_seed = g_random_seed;
	current.inside = g_externs.Inside();
	current.logmap = g_log_palette_mode;
	current.invert[0] = g_inversion[0];
	current.invert[1] = g_inversion[1];
	current.invert[2] = g_inversion[2];
	current.decomposition = g_decomposition[0];
	current.biomorph = g_externs.Biomorph();
	current.symmetry = g_force_symmetry;
	g_3d_state.get_raytrace_parameters(&current.init_3d[0]);
	current.preview_factor = g_3d_state.preview_factor();
	current.x_translate = g_3d_state.x_trans();
	current.y_translate = g_3d_state.y_trans();
	current.red_crop_left = g_3d_state.red().crop_left();
	current.red_crop_right = g_3d_state.red().crop_right();
	current.blue_crop_left = g_3d_state.blue().crop_left();
	current.blue_crop_right = g_3d_state.blue().crop_right();
	current.red_bright = g_3d_state.red().bright();
	current.blue_bright = g_3d_state.blue().bright();
	current.x_adjust = g_3d_state.x_adjust();
	current.yadjust = g_3d_state.y_adjust();
	current.eye_separation = g_3d_state.eye_separation();
	current.glasses_type = g_3d_state.glasses_type();
	current.outside = g_externs.Outside();
	current.x_3rd = g_escape_time_state.m_grid_fp.x_3rd();
	current.y_3rd = g_escape_time_state.m_grid_fp.y_3rd();
	current.standard_calculation_mode = g_externs.UserStandardCalculationMode();
	current.three_pass = g_three_pass;
	current.stop_pass = g_externs.StopPass();
	current.distance_test = g_distance_test;
	current.function_index[0] = g_function_index[0];
	current.function_index[1] = g_function_index[1];
	current.function_index[2] = g_function_index[2];
	current.function_index[3] = g_function_index[3];
	current.finite_attractor = g_finite_attractor;
	current.initial_orbit_z[0] = g_initial_orbit_z.real();
	current.initial_orbit_z[1] = g_initial_orbit_z.imag();
	current.use_initial_orbit_z = g_externs.UseInitialOrbitZ();
	current.periodicity = g_periodicity_check;
	current.potential_16bit = g_disk_16bit;
	current.release = g_release;
	current.save_release = g_save_release;
	current.display_3d = g_display_3d;
	current.ambient = g_3d_state.ambient();
	current.randomize = g_3d_state.randomize_colors();
	current.haze = g_3d_state.haze();
	current.transparent[0] = g_3d_state.transparent0();
	current.transparent[1] = g_3d_state.transparent1();
	current.rotate_lo = g_rotate_lo;
	current.rotate_hi = g_rotate_hi;
	current.distance_test_width = g_distance_test_width;
	current.m_x_max_fp = g_m_x_max_fp;
	current.m_x_min_fp = g_m_x_min_fp;
	current.m_y_max_fp = g_m_y_max_fp;
	current.m_y_min_fp = g_m_y_min_fp;
	current.z_dots = g_z_dots;
	current.origin_fp = g_origin_fp;
	current.depth_fp = g_depth_fp;
	current.height_fp = g_height_fp;
	current.width_fp = g_width_fp;
	current.screen_distance_fp = g_screen_distance_fp;
	current.eyes_fp = g_eyes_fp;
	current.orbit_type = g_new_orbit_type;
	current.juli_3d_mode = g_juli_3d_mode;
	current.max_fn = g_formula_state.max_fn();
	current.major_method = g_major_method;
	current.minor_method = g_minor_method;
	current.bail_out = g_externs.BailOut();
	current.bail_out_test = g_externs.BailOutTest();
	current.iterations = g_max_iteration;
	current.old_demm_colors = g_old_demm_colors;
	current.log_dynamic_calculate = g_log_dynamic_calculate;
	current.is_mandelbrot = g_is_mandelbrot;
	current.proximity = g_proximity;
	current.beauty_of_fractals = g_beauty_of_fractals;
	current.orbit_delay = g_orbit_delay;
	current.orbit_interval = g_orbit_interval;
	current.oxmin = g_orbit_x_min;
	current.oxmax = g_orbit_x_max;
	current.oymin = g_orbit_y_min;
	current.oymax = g_orbit_y_max;
	current.ox3rd = g_orbit_x_3rd;
	current.oy3rd = g_orbit_y_3rd;
	current.keep_screen_coordinates = g_keep_screen_coords;
	current.draw_mode = g_orbit_draw_mode;
	current.dac = g_.DAC();
	switch (g_fractal_type)
	{
	case FRACTYPE_FORMULA:
	case FRACTYPE_FORMULA_FP:
		current.file_name = g_formula_state.get_filename();
		current.item_name = g_formula_state.get_formula();
		break;
	case FRACTYPE_IFS:
	case FRACTYPE_IFS_3D:
		current.file_name = g_ifs_filename;
		current.item_name = g_ifs_name;
		break;
	case FRACTYPE_L_SYSTEM:
		current.file_name = g_l_system_filename;
		current.item_name = g_l_system_name;
		break;
	default:
		current.file_name = "";
		current.item_name = "";
		break;
	}
	if (s_history_index == -1)        // initialize the history file
	{
		for (int i = 0; i < g_max_history; i++)
		{
			s_history[i] = current;
		}
		s_history_flag = false;
		s_save_index = 0;
		s_history_index = 0;   // initialize history ptr
	}
	else if (s_history_flag)
	{
		s_history_flag = false;   // coming from user history command, don't save
	}
	else if (current != last)
	{
		if (++s_save_index >= g_max_history)  // back to beginning of circular buffer
		{
			s_save_index = 0;
		}
		if (++s_history_index >= g_max_history)  // move user pointer in parallel
		{
			s_history_index = 0;
		}
		assert(s_save_index >= 0 && s_save_index < g_max_history);
		s_history[s_save_index] = current;
	}
}

void history_restore_info()
{
	HISTORY_ITEM last;
	if (g_max_history <= 0 || g_bf_math || !s_history)
	{
		return;
	}
	assert(s_history_index >= 0 && s_history_index < g_max_history);
	last = s_history[s_history_index];

	g_invert = 0;
	g_externs.SetCalculationStatus(CALCSTAT_PARAMS_CHANGED);
	g_resuming = false;
	g_fractal_type = last.fractal_type;
	g_escape_time_state.m_grid_fp.x_min() = last.x_min;
	g_escape_time_state.m_grid_fp.x_max() = last.x_max;
	g_escape_time_state.m_grid_fp.y_min() = last.y_min;
	g_escape_time_state.m_grid_fp.y_max() = last.y_max;
	g_parameters[P1_REAL] = last.c_real;
	g_parameters[P1_IMAG] = last.c_imag;
	g_parameters[P2_REAL] = last.dparm3;
	g_parameters[P2_IMAG] = last.dparm4;
	g_parameters[P3_REAL] = last.dparm5;
	g_parameters[P3_IMAG] = last.dparm6;
	g_parameters[P4_REAL] = last.dparm7;
	g_parameters[P4_IMAG] = last.dparm8;
	g_parameters[P5_REAL] = last.dparm9;
	g_parameters[P5_IMAG] = last.dparm10;
	g_fill_color = last.fill_color;
	g_potential_parameter[0] = last.potential[0];
	g_potential_parameter[1] = last.potential[1];
	g_potential_parameter[2] = last.potential[2];
	g_use_fixed_random_seed = last.use_fixed_random_seed;
	g_random_seed = last.random_seed;
	g_externs.SetInside(last.inside);
	g_log_palette_mode = last.logmap;
	g_inversion[0] = last.invert[0];
	g_inversion[1] = last.invert[1];
	g_inversion[2] = last.invert[2];
	g_decomposition[0] = last.decomposition;
	g_externs.SetUserBiomorph(last.biomorph);
	g_externs.SetBiomorph(last.biomorph);
	g_force_symmetry = last.symmetry;
	g_3d_state.set_raytrace_parameters(&last.init_3d[0]);
	g_3d_state.set_preview_factor(last.preview_factor);
	g_3d_state.set_x_trans(last.x_translate);
	g_3d_state.set_y_trans(last.y_translate);
	g_3d_state.set_red().set_crop_left(last.red_crop_left);
	g_3d_state.set_red().set_crop_right(last.red_crop_right);
	g_3d_state.set_blue().set_crop_left(last.blue_crop_left);
	g_3d_state.set_blue().set_crop_right(last.blue_crop_right);
	g_3d_state.set_red().set_bright(last.red_bright);
	g_3d_state.set_blue().set_bright(last.blue_bright);
	g_3d_state.set_x_adjust(last.x_adjust);
	g_3d_state.set_y_adjust(last.yadjust);
	g_3d_state.set_eye_separation(last.eye_separation);
	g_3d_state.set_glasses_type(GlassesType(last.glasses_type));
	g_externs.SetOutside(last.outside);
	g_escape_time_state.m_grid_fp.x_3rd() = last.x_3rd;
	g_escape_time_state.m_grid_fp.y_3rd() = last.y_3rd;
	g_externs.SetUserStandardCalculationMode(CalculationMode(last.standard_calculation_mode));
	g_externs.SetStandardCalculationMode(CalculationMode(last.standard_calculation_mode));
	g_three_pass = (last.three_pass != 0);
	g_externs.SetStopPass(last.stop_pass);
	g_distance_test = last.distance_test;
	g_user_distance_test = last.distance_test;
	g_function_index[0] = last.function_index[0];
	g_function_index[1] = last.function_index[1];
	g_function_index[2] = last.function_index[2];
	g_function_index[3] = last.function_index[3];
	g_finite_attractor = last.finite_attractor;
	g_initial_orbit_z.real(last.initial_orbit_z[0]);
	g_initial_orbit_z.imag(last.initial_orbit_z[1]);
	g_externs.SetUseInitialOrbitZ(InitialZType(last.use_initial_orbit_z));
	g_periodicity_check = last.periodicity;
	g_user_periodicity_check = last.periodicity;
	g_disk_16bit = last.potential_16bit;
	g_release = last.release;
	g_save_release = last.save_release;
	g_display_3d = Display3DType(last.display_3d);
	g_3d_state.set_ambient(last.ambient);
	g_3d_state.set_randomize_colors(last.randomize);
	g_3d_state.set_haze(last.haze);
	g_3d_state.set_transparent0(last.transparent[0]);
	g_3d_state.set_transparent1(last.transparent[1]);
	g_rotate_lo = last.rotate_lo;
	g_rotate_hi = last.rotate_hi;
	g_distance_test_width = last.distance_test_width;
	g_m_x_max_fp = last.m_x_max_fp;
	g_m_x_min_fp = last.m_x_min_fp;
	g_m_y_max_fp = last.m_y_max_fp;
	g_m_y_min_fp = last.m_y_min_fp;
	g_z_dots = last.z_dots;
	g_origin_fp = last.origin_fp;
	g_depth_fp = last.depth_fp;
	g_height_fp = last.height_fp;
	g_width_fp = last.width_fp;
	g_screen_distance_fp = last.screen_distance_fp;
	g_eyes_fp = last.eyes_fp;
	g_new_orbit_type = last.orbit_type;
	g_juli_3d_mode = last.juli_3d_mode;
	g_formula_state.set_max_fn(int(last.max_fn));
	g_major_method = MajorMethodType(last.major_method);
	g_minor_method = MinorMethodType(last.minor_method);
	g_externs.SetBailOut(last.bail_out);
	g_externs.SetBailOutTest(BailOutType(last.bail_out_test));
	g_max_iteration = last.iterations;
	g_old_demm_colors = last.old_demm_colors;
	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
	g_potential_flag = (g_potential_parameter[0] != 0.0);
	if (g_inversion[0] != 0.0)
	{
		g_invert = 3;
	}
	g_log_dynamic_calculate = last.log_dynamic_calculate;
	g_is_mandelbrot = last.is_mandelbrot;
	g_proximity = last.proximity;
	g_beauty_of_fractals = last.beauty_of_fractals;
	g_orbit_delay = last.orbit_delay;
	g_orbit_interval = last.orbit_interval;
	g_orbit_x_min = last.oxmin;
	g_orbit_x_max = last.oxmax;
	g_orbit_y_min = last.oymin;
	g_orbit_y_max = last.oymax;
	g_orbit_x_3rd = last.ox3rd;
	g_orbit_y_3rd = last.oy3rd;
	g_keep_screen_coords = last.keep_screen_coordinates;
	if (g_keep_screen_coords)
	{
		g_set_orbit_corners = true;
	}
	g_orbit_draw_mode = int(last.draw_mode);
	g_user_float_flag = (g_current_fractal_specific->isinteger != 0);
	g_.DAC() = last.dac;
	g_.OldDAC() = last.dac;
	if (g_.MapDAC())
	{
		g_.SetMapDAC(&last.dac);
	}
	load_dac();
	g_.SetSaveDAC(fractal_type_julibrot(g_fractal_type) ? SAVEDAC_NO : SAVEDAC_YES);
	switch (g_fractal_type)
	{
	case FRACTYPE_FORMULA:
	case FRACTYPE_FORMULA_FP:
		g_formula_state.set_filename(last.file_name);
		g_formula_state.set_formula(last.item_name);
		break;
	case FRACTYPE_IFS:
	case FRACTYPE_IFS_3D:
		g_ifs_filename = last.file_name;
		g_ifs_name = last.item_name;
		break;
	case FRACTYPE_L_SYSTEM:
		g_l_system_filename = last.file_name;
		g_l_system_name = last.item_name;
		break;
	default:
		break;
	}
}

void history_allocate()
{
	while (g_max_history > 0) // decrease history if necessary
	{
		s_history = new HISTORY_ITEM[g_max_history];
		if (s_history)
		{
			break;
		}
		g_max_history--;
	}
}

void history_free()
{
	delete[] s_history;
	s_history = 0;
}

void history_back()
{
	--s_history_index;
	if (s_history_index <= 0)
	{
		s_history_index = g_max_history - 1;
	}
	s_history_flag = true;
}

void history_forward()
{
	++s_history_index;
	if (s_history_index >= g_max_history)
	{
		s_history_index = 0;
	}
	s_history_flag = true;
}
