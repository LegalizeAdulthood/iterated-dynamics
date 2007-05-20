#ifndef PROTOTYP_H
#define PROTOTYP_H

/* includes needed to define the prototypes */

#include "mpmath.h"
#include "big.h"
#include "fractint.h"
#include "helpcom.h"
#include "externs.h"

extern int get_corners();
extern int getakeynohelp();
extern void set_null_video();
extern void spindac(int, int);
extern void initasmvars();

/* maintain the common prototypes in this file
 * split the dos/win/unix prototypes into separate files.
 */

#ifdef XFRACT
#include "unixprot.h"
#endif

#ifdef _WIN32
#include "winprot.h"
#endif

extern long cdecl divide(long, long, int);
extern long cdecl multiply(long, long, int);
extern void put_line(int, int, int, BYTE *);
extern void get_line(int, int, int, BYTE *);
extern void find_special_colors();
extern long read_ticker();

/*  fractint -- C file prototypes */

extern int application_main(int argc, char **argv);
extern int elapsed_time(int);

/*  hcmplx -- C file prototypes */

extern void HComplexTrig0(HyperComplexD *, HyperComplexD *);

/*  help -- C file prototypes */

extern int _find_token_length(char *, unsigned int, int *, int *);
extern int find_token_length(int, char *, unsigned int, int *, int *);
extern int find_line_width(int, char *, unsigned int);
extern int process_document(PD_FUNC, PD_FUNC, VOIDPTR);
extern int help(int);
extern int read_help_topic(int, int, int, VOIDPTR);
extern int makedoc_msg_func(int, int);
extern void print_document(const char *, int (*)(int, int), int);
extern int init_help();
extern void end_help();

/*  intro -- C file prototypes */

extern void intro();

/*  jb -- C file prototypes */

extern int julibrot_setup();
extern int julibrot_setup_fp();
extern int julibrot_per_pixel();
extern int julibrot_per_pixel_fp();
extern int standard_4d_fractal();
extern int standard_4d_fractal_fp();

/*  jiim -- C file prototypes */

extern void Jiim(int);
extern ComplexL PopLong();
extern ComplexD PopFloat();
extern ComplexL DeQueueLong();
extern ComplexD DeQueueFloat();
extern ComplexL ComplexSqrtLong(long,  long);
extern ComplexD ComplexSqrtFloat(double, double);
extern int    Init_Queue(unsigned long);
extern void   Free_Queue();
extern void   ClearQueue();
extern int    QueueEmpty();
extern int    QueueFull();
extern int    QueueFullAlmost();
extern int    PushLong(long,  long);
extern int    PushFloat(float,  float);
extern int    EnQueueLong(long,  long);
extern int    EnQueueFloat(float,  float);

/*  line3d -- C file prototypes */

extern int line3d(BYTE *, int);
extern int targa_color(int, int, int);
extern int start_disk1(char *, FILE *, int);
extern void line_3d_free();

/*  loadfdos -- C file prototypes */
extern int get_video_mode(const fractal_info *, struct ext_blk_formula_info *);

/*  loadfile -- C file prototypes */

extern int read_overlay();
extern void set_if_old_bif();
extern void set_function_parm_defaults();
extern int look_get_window();
extern void backwards_v18();
extern void backwards_v19();
extern void backwards_v20();
extern int check_back();

/*  loadmap -- C file prototypes */

extern int validate_luts(const char *);
extern int set_color_palette_name(char *);

/*  lorenz -- C file prototypes */

extern int orbit_3d_setup();
extern int orbit_3d_setup_fp();
extern int lorenz_3d_orbit(long *, long *, long *);
extern int lorenz_3d_orbit_fp(double *, double *, double *);
extern int lorenz_3d1_orbit_fp(double *, double *, double *);
extern int lorenz_3d3_orbit_fp(double *, double *, double *);
extern int lorenz_3d4_orbit_fp(double *, double *, double *);
extern int henon_orbit_fp(double *, double *, double *);
extern int henon_orbit(long *, long *, long *);
extern int inverse_julia_orbit(double *, double *, double *);
extern int Minverse_julia_orbit();
extern int Linverse_julia_orbit();
extern int inverse_julia_per_image();
extern int rossler_orbit_fp(double *, double *, double *);
extern int pickover_orbit_fp(double *, double *, double *);
extern int gingerbread_orbit_fp(double *, double *, double *);
extern int rossler_orbit(long *, long *, long *);
extern int kam_torus_orbit_fp(double *, double *, double *);
extern int kam_torus_orbit(long *, long *, long *);
extern int hopalong_2d_orbit_fp(double *, double *, double *);
extern int chip_2d_orbit_fp(double *, double *, double *);
extern int quadrup_two_2d_orbit_fp(double *, double *, double *);
extern int three_ply_2d_orbit_fp(double *, double *, double *);
extern int martin_2d_orbit_fp(double *, double *, double *);
extern int orbit_2d_fp();
extern int orbit_2d();
extern int funny_glasses_call(int (*)());
extern int ifs();
extern int orbit_3d_fp();
extern int orbit_3d();
extern int icon_orbit_fp(double *, double *, double *);  /* dmf */
extern int latoo_orbit_fp(double *, double *, double *);  /* hb */
extern int setup_convert_to_screen(struct affine *);
extern int plotorbits2dsetup();
extern int plotorbits2dfloat();

/*  lsys -- C file prototypes */

extern LDBL  _fastcall get_number(char **);
extern bool _fastcall is_pow2(int);
extern int l_system();
extern int l_load();

/*  miscfrac -- C file prototypes */

/*  miscovl -- C file prototypes */

extern void make_batch_file();
extern void edit_text_colors();
extern int select_video_mode(int);
extern void format_vid_table(int choice, char *buf);
extern void make_mig(unsigned int, unsigned int);
extern int get_precision_dbl(int);
extern int get_precision_bf(int);
extern int get_precision_mag_bf();
extern void parse_comments(char *value);
extern void init_comments();
extern void write_batch_parms(const char *, int, int, int, int);
extern void expand_comments(char *, char *);

/*  miscres -- C file prototypes */

extern void restore_active_ovly();
extern void findpath(const char *, char *);
extern void not_disk_message();
extern void convert_center_mag(double *, double *, LDBL *, double *, double *, double *);
extern void convert_corners(double, double, LDBL, double, double, double);
extern void convert_center_mag_bf(bf_t, bf_t, LDBL *, double *, double *, double *);
extern void convert_corners_bf(bf_t, bf_t, LDBL, double, double, double);
extern void update_save_name(char *);
extern int check_write_file(char *filename, const char *ext);
extern int check_key();
extern void show_trig(char *);
extern int set_trig_array(int, const char *);
extern void set_trig_pointers(int);
extern int tab_display();
extern int ends_with_slash(char *);
extern int ifs_load();
extern int find_file_item(char *, const char *item_name, FILE **, int);
extern int file_gets(char *, int, FILE *);
extern void round_float_d(double *);
extern void fix_inversion(double *);
extern int unget_a_key(int);
extern void get_calculation_time(char *, long);

/*  mpmath_c -- C file prototypes */

extern struct MP *MPsub(struct MP, struct MP);
extern struct MP *MPabs(struct MP);
extern struct MPC MPCsqr(struct MPC);
extern struct MP MPCmod(struct MPC);
extern struct MPC MPCmul(struct MPC, struct MPC);
extern struct MPC MPCdiv(struct MPC, struct MPC);
extern struct MPC MPCadd(struct MPC, struct MPC);
extern struct MPC MPCsub(struct MPC, struct MPC);
extern struct MPC MPCpow(struct MPC, int);
extern int MPCcmp(struct MPC, struct MPC);
extern ComplexD MPC2cmplx(struct MPC);
extern struct MPC cmplx2MPC(ComplexD);
extern ComplexD ComplexPower(ComplexD, ComplexD);
extern void SetupLogTable();
extern long logtablecalc(long);
extern long ExpFloat14(long);
extern int complex_basin();
extern int gaussian_number(int, int);
extern void Arcsinz(ComplexD z, ComplexD *rz);
extern void Arccosz(ComplexD z, ComplexD *rz);
extern void Arcsinhz(ComplexD z, ComplexD *rz);
extern void Arccoshz(ComplexD z, ComplexD *rz);
extern void Arctanhz(ComplexD z, ComplexD *rz);
extern void Arctanz(ComplexD z, ComplexD *rz);

/*  plot3d -- C file prototypes */

extern void cdecl draw_line(int, int, int, int, int);
extern void _fastcall plot_3d_superimpose_16(int, int, int);
extern void _fastcall plot_3d_superimpose_256(int, int, int);
extern void _fastcall plot_ifs_3d_superimpose_256(int, int, int);
extern void _fastcall plot_3d_alternate(int, int, int);
extern void plot_setup();

/*  prompts1 -- C file prototypes */

extern int full_screen_prompt(const char *heading, int num_prompts, const char **prompts,
	struct full_screen_values *values, int function_key_mask, char *footer);
extern long get_file_entry(int type, const char *title, char *fmask, char *filename, char *entryname);
extern int get_fractal_type();
extern int get_fractal_parameters(int);
extern int get_fractal_3d_parameters();
extern int get_3d_parameters();
extern int prompt_value_string(char *buf, struct full_screen_values *val);
extern void set_bail_out_formula(enum bailouts);
extern int find_extra_parameter(int);
extern void load_parameters(int g_fractal_type);
extern int check_orbit_name(char *);
extern int scan_entries(FILE *infile, struct entryinfo *choices, const char *itemname);

/*  prompts2 -- C file prototypes */

extern int get_toggles();
extern int get_toggles2();
extern int passes_options();
extern int get_view_params();
extern int get_starfield_params();
extern int get_commands();
extern void goodbye();
extern bool is_a_directory(char *s);
extern int get_a_filename(char *, char *, char *);
extern int split_path(const char *file_template, char *drive, char *dir, char *fname, char *ext);
extern int make_path(char *file_template, char *drive, char *dir, char *fname, char *ext);
extern int fr_find_first(char *path);
extern int fr_find_next();
extern void shell_sort(void *, int n, unsigned, int (__cdecl *fct)(VOIDPTR, VOIDPTR));
extern void fix_dir_name(char *dirname);
extern int merge_path_names(char *, char *, int);
extern int get_browse_parameters();
extern int get_command_string();
extern int get_random_dot_stereogram_parameters();
extern int starfield();
extern int get_a_number(double *, double *);
extern int lccompare(VOIDPTR, VOIDPTR);
extern int dir_remove(char *, char *);
extern FILE *dir_fopen(const char *, const char *, const char *);
extern void extract_filename(char *, char *);
extern char *has_extension(char *source);
extern int integer_unsupported();

/*  realdos -- C file prototypes */

extern int show_vid_length();
extern int stop_message(int, const char *);
extern void blank_rows(int, int, int);
extern int text_temp_message(char *);
extern int full_screen_choice(int options, const char *hdg, char *hdg2,
	char *instr, int numchoices, char **choices, int *attributes,
	int boxwidth, int boxdepth, int colwidth, int current,
	void (*formatitem)(int, char *), char *speedstring,
	int (*speedprompt)(int, int, int, char *, int),
	int (*checkkey)(int, int));
extern int full_screen_choice_help(int help_mode, int options,
	const char *hdg, char *hdg2, char *instr, int numchoices,
	char **choices, int *attributes, int boxwidth, int boxdepth,
	int colwidth, int current, void (*formatitem)(int, char *),
	char *speedstring, int (*speedprompt)(int, int, int, char *, int),
	int (*checkkey)(int, int));

extern int show_temp_message(char *);
extern void clear_temp_message();
extern void help_title();
extern int put_string_center(int, int, int, int, const char *);
#ifndef XFRACT /* Unix should have this in string.h */
extern int strncasecmp(char *, char *, int);
#endif
extern int main_menu(bool full_menu);
extern int input_field(int, int, char *, int, int, int, int (*)(int));
extern int field_prompt(char *, char *, char *, int, int (*)(int));
extern int thinking(int, char *);
extern void load_fractint_config();
extern int check_video_mode_key(int, int);
extern int check_vidmode_keyname(char *);
extern void video_mode_key_name(int, char *);
extern void free_temp_message();

extern void load_video_table(int);
extern void bad_fractint_cfg_msg();

/*  rotate -- C file prototypes */

extern void rotate(int);
extern void save_palette();
extern int load_palette();

/*  slideshw -- C file prototypes */

extern int slide_show();
extern int start_slide_show();
extern void stop_slide_show();
extern void record_show(int);

/*  stereo -- C file prototypes */

extern int auto_stereo();
extern int out_line_stereo(BYTE *, int);

/*  targa -- C file prototypes */

extern void tga_write(int, int, int);
extern int tga_read(int, int);
extern void tga_end();
extern void tga_start();
extern void tga_reopen();

/*  testpt -- C file prototypes */

extern int test_start();
extern void test_end();
extern int test_per_pixel(double, double, double, double, long, int);

/*  zoom -- C file prototypes */

extern void zoom_box_draw(int);
extern void zoom_box_move(double, double);
extern void zoom_box_resize(int);
extern void zoom_box_change_i(int, int);
extern void zoom_box_out();
extern void aspect_ratio_crop(float, float);
extern int init_pan_or_recalc(int);
extern void _fastcall draw_lines(Coordinate, Coordinate, int, int);
extern void _fastcall add_box(Coordinate);
extern void clear_box();
extern void display_box();

/*  fractalb.c -- C file prototypes */

extern ComplexD complex_bn_to_float(ComplexBigNum *);
extern ComplexD complex_bf_to_float(ComplexBigFloat *);
extern void compare_values(char *, LDBL, bn_t);
extern void compare_values_bf(char *, LDBL, bf_t);
extern void show_var_bf(char *s, bf_t n);
extern void show_two_bf(char *, bf_t, char *, bf_t, int);
extern void corners_bf_to_float();
extern void show_corners_dbl(char *);
extern int mandelbrot_setup_bn();
extern int mandelbrot_per_pixel_bn();
extern int julia_per_pixel_bn();
extern int julia_orbit_bn();
extern int julia_z_power_orbit_bn();
extern ComplexBigNum *complex_log_bn(ComplexBigNum *t, ComplexBigNum *s);
extern ComplexBigNum *complex_multiply_bn(ComplexBigNum *t, ComplexBigNum *x, ComplexBigNum *y);
extern ComplexBigNum *complex_power_bn(ComplexBigNum *t, ComplexBigNum *xx, ComplexBigNum *yy);
extern int mandelbrot_setup_bf();
extern int mandelbrot_per_pixel_bf();
extern int julia_per_pixel_bf();
extern int julia_orbit_bf();
extern int julia_z_power_orbit_bf();
extern ComplexBigFloat *complex_log_bf(ComplexBigFloat *t, ComplexBigFloat *s);
extern ComplexBigFloat *cplxmul_bf(ComplexBigFloat *t, ComplexBigFloat *x, ComplexBigFloat *y);
extern ComplexBigFloat *ComplexPower_bf(ComplexBigFloat *t, ComplexBigFloat *xx, ComplexBigFloat *yy);

/*  memory -- C file prototypes */
/* TODO: Get rid of this and use regular memory routines;
** see about creating standard disk memory routines for disk video
*/
extern int MemoryType(U16 handle);
extern void InitMemory();
extern void ExitCheck();
extern U16 MemoryAlloc(U16 size, long count, int stored_at);
extern void MemoryRelease(U16 handle);
extern bool MoveToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern bool MoveFromMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern bool SetMemory(int value, U16 size, long count, long offset, U16 handle);

/*  soi -- C file prototypes */
extern void soi();
extern void soi_long_double();

/*
 *  uclock -- C file prototypes 
 *  The  uclock_t typedef placed here because uclock.h
 *  prototype is for DOS version only.
 */
typedef unsigned long uclock_t;

extern uclock_t usec_clock();
extern void restart_uclock();
extern void wait_until(int index, uclock_t wait_time);

extern void init_failure(const char *message);
extern int expand_dirname(char *dirname, char *drive);
extern int abort_message(char *file, unsigned int line, int flags, char *msg);
#define ABORT(flags_, msg_) abort_message(__FILE__, __LINE__, flags_, msg_)

#ifndef DEBUG
/*#define DEBUG */
#endif

#endif
