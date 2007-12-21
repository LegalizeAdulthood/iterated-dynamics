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
bool g_got_real_dac = false;
bool g_using_jiim = false;
int g_screen_width = 0;
int g_screen_height = 0;
int g_sx_offset = 0;
int g_sy_offset = 0;
int g_color_medium = 0;
int g_color_dark = 0;
unsigned char *g_text_colors = 0;
int g_text_row = 0;
int g_text_cbase = 0;
bool g_command_initialize = false;
int g_initialize_batch = 0;
int g_debug_mode = 0;
boost::filesystem::path g_work_dir("");

int driver_key_cursor(int, int)
{
	return 0;
}
void round_float_d(double *)
{
}
bool fractal_type_julia_or_inverse(int)
{
	return false;
}
bool fractal_type_none(int)
{
	return false;
}
void help(enum HelpAction)
{
}
int tab_display()
{
	return 0;
}
int get_help_mode()
{
	return 0;
}
void set_help_mode(int)
{
}
bool ends_with_slash(char const *)
{
	return false;
}
void put_line(int,int,int,unsigned char *)
{
}
void driver_set_clear()
{
}
int driver_diskp()
{
	return 0;
}
void disk_video_status(int,char const *)
{
}
void driver_unstack_screen()
{
}
int getakeynohelp()
{
	return 0;
}
int driver_get_key()
{
	return 0;
}
void driver_buzzer(int)
{
}
void driver_hide_text_cursor()
{
}
void driver_set_attr(int,int,int,int)
{
}
void driver_put_string(int,int,int,char const *)
{
}
void driver_move_cursor(int,int)
{
}
void driver_stack_screen()
{
}
void goodbye()
{
}
void init_failure(char const *)
{
}
void driver_set_mouse_mode(int)
{
}
int driver_get_mouse_mode()
{
	return 0;
}
int driver_wait_key_pressed(int)
{
	return 0;
}
void driver_display_string(int,int,int,int,char const *)
{
}
void find_special_colors()
{
}
void get_line(int,int,int,unsigned char *)
{
}
int getcolor(int, int)
{
	return 0;
}
void (*g_plot_color)(int, int, int) = 0;
int g_and_color = 0;
int g_colors = 0;
int test_per_pixel(double, double, double, double, long, int)
{
	return 0;
}
ComplexD g_parameter;
ComplexD g_initial_z;
long g_max_iteration = 0;
int g_inside = 0;
int put_resume(int, ...)
{
	return 0;
}
int alloc_resume(int, int)
{
	return 0;
}
void test_end()
{
}
double (*g_dy_pixel)() = 0;
double (*g_dx_pixel)() = 0;
int g_x_stop = 0;
int g_col = 0;
int g_y_stop = 0;
int g_row = 0;
int g_passes = 0;
CalculationMode g_standard_calculation_mode;
int test_start()
{
	return 0;
}
void end_resume()
{
}
int get_resume(int, ...)
{
	return 0;
}
int start_resume()
{
	return 0;
}
bool g_resuming = false;
int g_y_dots = 0;
int g_x_dots = 0;
int g_max_colors = 0;
void (*g_plot_color_put_color)(int, int, int) = 0;
bool g_potential_16bit = false;
bool g_potential_flag = false;
int g_outside = 0;
int disk_start_potential()
{
	return 0;
}
int g_random_seed = 0;
bool g_use_fixed_random_seed = false;
void disk_write(int, int, int)
{
}
int disk_read(int, int)
{
	return 0;
}
void spindac(int, int)
{
}
unsigned char (*g_dac_box)[3] = 0;
bool g_color_preloaded = false;
unsigned char *g_map_dac_box = 0;
int check_key()
{
	return 0;
}
bool g_show_orbit = false;
void FPUsincos(double *, double *, double *)
{
}
long g_fudge = 0;
int g_bit_shift = 0;
void not_disk_message()
{
}
ComplexL g_initial_z_l;
int g_integer_fractal = 0;
int g_periodicity_check = 0;
bool g_overflow = false;
void (*g_trig0_d)() = 0;
Arg *g_argument1 = 0;
ComplexD g_temp_z;
long multiply(long, long, int) { return 0; }
void (*g_trig0_l)() = 0;
ComplexL g_tmp_z_l;
long divide(long, long, int) { return 0; }
void lStkPwr() {}
Arg *g_argument2 = 0;
ComplexL g_parameter2_l;
int timer_engine(int (*)()) { return 0; }
int standard_fractal() { return 0; }
bool g_reset_periodicity = false;
double g_temp_sqr_x = 0.0;
long g_temp_sqr_x_l = 0;
void plot_color_none(int, int, int) {}
int g_input_counter = 0;
int g_max_input_counter = 0;
int g_color = 0;
void invert_z(ComplexD *) {}
int g_invert = 0;
CalculationMode g_user_standard_calculation_mode;
long g_log_palette_mode = 0;
bool g_next_screen_flag = false;
int g_color_state = 0;
int g_orbit_color = 0;
double g_rq_limit = 0.0;
bool validate_luts(char const *) { return 0; }
long g_old_color_iter = 0;
long g_real_color_iter = 0;
void orbit_scrub() {}
void plot_orbit_i(long, long, int) {}
ComplexL g_new_z_l;
long g_limit_l = 0;
long g_magnitude_l = 0;
long g_temp_sqr_y_l = 0;
long (*g_ly_pixel)() = 0;
long (*g_lx_pixel)() = 0;
ComplexL g_old_z_l;
void plot_orbit(double, double, int) {}
ComplexD g_new_z;
double g_temp_sqr_y = 0.0;
ComplexD g_old_z;
int g_show_dot = 0;
long g_color_iter = 0;
int g_orbit_index = 0;
void set_pixel_calc_functions() {}
int filename_speedstr(int row, int col, int vid, char *speedstring, int speed_match)
{ return 0; }
int check_f6_key(int curkey, int)
{ return 0; }
int full_screen_choice(int options,
	const char *heading, const char *heading2, const char *instructions,
	int num_choices, char **choices, const int *attributes,
	int box_width, int box_depth, int column_width, int current,
	void (*format_item)(int, char *),
	char *speed_string, int (*speed_prompt)(int, int, int, char *, int),
	int (*check_key)(int, int))
{ return -1; }
