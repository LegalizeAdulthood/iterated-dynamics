#include "stdafx.h"

#include <boost/filesystem.hpp>

#include "port.h"
#include "id.h"
#include "big.h"
#include "cmplx.h"
#include "mpmath.h"

#include "calcfrac.h"
#include "EscapeTime.h"

bool g_tab_display_enabled = 0;
int g_text_col = 0;
VIDEOINFO g_video_table[1] = { 0 };
int g_calculation_status = 0;
int g_fractal_type = 0;
FractalTypeSpecificData *g_current_fractal_specific = 0;
double g_parameters[1] = { 0.0 };
bool g_using_jiim = false;
int g_screen_width = 0;
int g_screen_height = 0;
int g_screen_x_offset = 0;
int g_screen_y_offset = 0;
int g_text_row = 0;
int g_text_cbase = 0;
bool g_command_initialize = false;
int g_initialize_batch = 0;
int g_debug_mode = 0;
boost::filesystem::path g_work_dir("");
void (*g_plot_color)(int, int, int) = 0;
int g_and_color = 0;
int g_colors = 0;
ComplexD g_parameter;
ComplexD g_initial_z;
long g_max_iteration = 0;
int g_inside = 0;
double (*g_dy_pixel)() = 0;
double (*g_dx_pixel)() = 0;
int g_x_stop = 0;
int g_col = 0;
int g_y_stop = 0;
int g_row = 0;
int g_passes = 0;
CalculationMode g_standard_calculation_mode;
bool g_resuming = false;
int g_y_dots = 0;
int g_x_dots = 0;
int g_max_colors = 0;
void (*g_plot_color_put_color)(int, int, int) = 0;
bool g_potential_16bit = false;
bool g_potential_flag = false;
int g_outside = 0;
int g_random_seed = 0;
bool g_use_fixed_random_seed = false;
bool g_show_orbit = false;
long g_fudge = 0;
int g_bit_shift = 0;
ComplexL g_initial_z_l;
int g_integer_fractal = 0;
int g_periodicity_check = 0;
bool g_overflow = false;
void (*g_trig0_d)() = 0;
Arg *g_argument1 = 0;
ComplexD g_temp_z;
void (*g_trig0_l)() = 0;
ComplexL g_tmp_z_l;
void lStkPwr() {}
Arg *g_argument2 = 0;
ComplexL g_parameter2_l;
int standard_fractal() { return 0; }
bool g_reset_periodicity = false;
double g_temp_sqr_x = 0.0;
long g_temp_sqr_x_l = 0;
int g_input_counter = 0;
int g_max_input_counter = 0;
int g_color = 0;
int g_invert = 0;
CalculationMode g_user_standard_calculation_mode;
long g_log_palette_mode = 0;
bool g_next_screen_flag = false;
int g_orbit_color = 0;
double g_rq_limit = 0.0;
long g_old_color_iter = 0;
long g_real_color_iter = 0;
void orbit_scrub() {}
ComplexL g_new_z_l;
long g_limit_l = 0;
long g_magnitude_l = 0;
long g_temp_sqr_y_l = 0;
long (*g_ly_pixel)() = 0;
long (*g_lx_pixel)() = 0;
ComplexL g_old_z_l;
ComplexD g_new_z;
double g_temp_sqr_y = 0.0;
ComplexD g_old_z;
int g_show_dot = 0;
long g_color_iter = 0;
int g_orbit_index = 0;
AbstractDriver *gdi_driver = 0;
AbstractDriver *disk_driver = 0;

int alloc_resume(int, int)						{ return 0; }
int check_f6_key(int curkey, int)				{ return 0; }
int check_key()									{ return 0; }
int disk_read(int, int)							{ return 0; }
int disk_start_potential()						{ return 0; }
void disk_video_status(int,char const *)		{ }
void disk_write(int, int, int)					{ }
long divide(long, long, int)					{ return 0; }
void end_resume()								{ }
bool ends_with_slash(char const *)				{ return false; }
int filename_speedstr(int row, int col, int vid, char *speedstring, int speed_match) { return 0; }
void find_special_colors()						{ }
void FPUsincos(double *, double *, double *)	{ }
bool fractal_type_julia_or_inverse(int)			{ return false; }
bool fractal_type_none(int)						{ return false; }
int get_key_no_help()							{ return 0; }
int get_color(int, int)							{ return 0; }
int get_help_mode()								{ return 0; }
void get_line(int,int,int,unsigned char *)		{ }
int get_resume(int, void *)						{ return 0; }
int get_text_color(int index)					{ return 0; }
void goodbye()									{ }
void help(enum HelpAction)						{ }
void init_failure(char const *)					{ }
void invert_z(ComplexD *)						{ }
long multiply(long, long, int)					{ return 0; }
void not_disk_message()							{ }
void plot_color_none(int, int, int)				{ }
void plot_orbit(double, double, int)			{ }
void plot_orbit_i(long, long, int)				{ }
void put_line(int,int,int,unsigned char *)		{ }
int put_resume(int, void const *)				{ return 0; }
void round_float_d(double *)					{ }
void set_help_mode(int)							{ }
void set_pixel_calc_functions()					{ }
void set_text_color(int index, int value)		{ }
void spin_dac(int, int)							{ }
int start_resume()								{ return 0; }
int tab_display()								{ return 0; }
void test_end()									{ }
int test_per_pixel(double, double, double, double, long, int) { return 0; }
int test_start()								{ return 0; }
int timer_engine(int (*)())						{ return 0; }
bool validate_luts(char const *)				{ return 0; }
