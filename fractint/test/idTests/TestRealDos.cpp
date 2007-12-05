#include "stdafx.h"

#include "realdos.h"
#include "id.h"

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
std::string g_work_dir = "";

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

TEST(video_mode_key_name_f1, realdos)
{
	std::string name = video_mode_key_name(FIK_F1);
	CHECK_EQUAL("F1", name);
}
TEST(video_mode_key_name_f10, realdos)
{
	std::string name = video_mode_key_name(FIK_F10);
	CHECK_EQUAL("F10", name);
}

TEST(video_mode_key_name_ctl_f1, realdos)
{
	std::string name = video_mode_key_name(FIK_CTL_F1);
	CHECK_EQUAL("CF1", name);
}
TEST(video_mode_key_name_ctl_f10, realdos)
{
	std::string name = video_mode_key_name(FIK_CTL_F10);
	CHECK_EQUAL("CF10", name);
}

TEST(video_mode_key_name_alt_f1, realdos)
{
	std::string name = video_mode_key_name(FIK_ALT_F1);
	CHECK_EQUAL("AF1", name);
}
TEST(video_mode_key_name_alt_f10, realdos)
{
	std::string name = video_mode_key_name(FIK_ALT_F10);
	CHECK_EQUAL("AF10", name);
}

TEST(video_mode_key_name_shift_f1, realdos)
{
	std::string name = video_mode_key_name(FIK_SF1);
	CHECK_EQUAL("SF1", name);
}
TEST(video_mode_key_name_shift_f10, realdos)
{
	std::string name = video_mode_key_name(FIK_SF10);
	CHECK_EQUAL("SF10", name);
}

TEST(video_mode_key_name_no_video, realdos)
{
	std::string name = video_mode_key_name(FIK_CTL_B);
	CHECK_EQUAL("", name);
}
