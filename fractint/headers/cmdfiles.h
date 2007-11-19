#if !defined(CMD_FILES_H)
#define CMD_FILES_H

extern std::string g_autokey_name;
extern std::string g_color_file;
extern std::string g_command_comment[4];
extern std::string g_command_file;
extern std::string g_command_name;
extern std::string g_gif_mask;
extern std::string g_ifs_filename;
extern std::string g_ifs_name;
extern std::string g_l_system_filename;
extern std::string g_l_system_name;
extern std::string g_organize_formula_dir;
extern std::string g_read_name;
extern std::string g_save_name;
extern std::string g_temp_dir;

extern bool g_make_par_flag;
extern bool g_make_par_colors_only;

extern int command_files(int, char **);
extern int load_commands(FILE *);
extern int get_curarg_len(char *curarg);
extern int get_max_curarg_len(char *floatvalstr[], int totparm);
extern int init_msg(const char *, const char *bad_filename, int mode);
extern int process_command(char *curarg, int mode);
extern int get_power_10(LDBL x);
extern void pause_error(int);
extern int bad_arg(const char *curarg);

#endif
