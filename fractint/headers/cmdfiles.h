#pragma once

#include <boost/filesystem.hpp>

enum AutoShowDotKind
{
	AUTOSHOWDOT_DEFAULT = 0,
	AUTOSHOWDOT_DARK = 'd',
	AUTOSHOWDOT_MEDIUM = 'm',
	AUTOSHOWDOT_BRIGHT = 'b',
	AUTOSHOWDOT_AUTO = 'a'
};

enum RecordColorsKind
{
	RECORDCOLORS_AUTO = 'a',
	RECORDCOLORS_COMMENT = 'c',
	RECORDCOLORS_YES = 'y'
};

/* process_command(), AbstractCommandParser::parse() return values */
enum CommandResultType
{
	COMMANDRESULT_ERROR = -1,
	COMMANDRESULT_OK = 0,
	COMMANDRESULT_FRACTAL_PARAMETER = 1,
	COMMANDRESULT_3D_PARAMETER = 2,
	COMMANDRESULT_3D_YES = 4,
	COMMANDRESULT_RESET = 8
};

struct search_path
{
	std::string par;
	std::string frm;
	std::string ifs;
	std::string lsys;
};

extern std::string g_color_file;
extern std::string g_command_comment[4];
extern std::string g_command_file;
extern std::string g_command_name;
extern std::string g_gif_mask;
extern std::string g_ifs_filename;
extern std::string g_ifs_name;
extern std::string g_l_system_filename;
extern std::string g_l_system_name;
extern boost::filesystem::path g_organize_formula_dir;
extern std::string g_read_name;
extern std::string g_save_name;
extern boost::filesystem::path g_temp_dir;
extern boost::filesystem::path g_work_dir;

extern AutoShowDotKind g_auto_show_dot;
extern RecordColorsKind g_record_colors;

extern search_path g_search_for;

extern bool g_make_par_flag;
extern bool g_make_par_colors_only;

extern bool g_beauty_of_fractals;

extern void command_files(int, char **);
extern int load_commands(std::ifstream &stream);
extern int get_curarg_len(char *curarg);
extern int get_max_curarg_len(char *floatvalstr[], int totparm);
extern int init_msg(const char *, const char *bad_filename, int mode);
extern int process_command(char *curarg, int mode);
extern int get_power_10(LDBL x);
extern void pause_error(int);
extern CommandResultType bad_arg(const char *curarg);
