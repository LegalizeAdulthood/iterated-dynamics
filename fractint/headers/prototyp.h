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

#if (!defined(XFRACT) && !defined(WINFRACT) && !defined(_WIN32))
#include "dosprot.h"
#endif

extern long cdecl divide(long, long, int);
extern long cdecl multiply(long, long, int);
extern void put_line(int, int, int, BYTE *);
extern void get_line(int, int, int, BYTE *);
extern void find_special_colors();
extern long read_ticker();

/*  calcmand -- assembler file prototypes */

extern long cdecl calculate_mandelbrot_asm();

/*  calmanfp -- assembler file prototypes */

extern void cdecl calculate_mandelbrot_start_fp_asm();
/* extern long  cdecl g_calculate_mandelbrot_asm_fp(); */
extern long  cdecl calculate_mandelbrot_fp_287_asm();
extern long  cdecl calculate_mandelbrot_fp_87_asm();

/*  fpu087 -- assembler file prototypes */

extern void cdecl FPUcplxmul(_CMPLX *, _CMPLX *, _CMPLX *);
extern void cdecl FPUcplxdiv(_CMPLX *, _CMPLX *, _CMPLX *);
extern void cdecl FPUsincos(double *, double *, double *);
extern void cdecl FPUsinhcosh(double *, double *, double *);
extern void cdecl FPUcplxlog(_CMPLX *, _CMPLX *);
extern void cdecl SinCos086(long, long *, long *);
extern void cdecl SinhCosh086(long, long *, long *);
extern long cdecl r16Mul(long, long);
extern long cdecl RegFloat2Fg(long, int);
extern long cdecl Exp086(long);
extern unsigned long cdecl ExpFudged(long, int);
extern long cdecl RegDivFloat(long, long);
extern long cdecl LogFudged(unsigned long, int);
extern long cdecl LogFloat14(unsigned long);
#if !defined(XFRACT) && !defined(_WIN32)
extern long cdecl RegFg2Float(long, char);
extern long cdecl RegSftFloat(long, char);
#else
extern long cdecl RegFg2Float(long, int);
extern long cdecl RegSftFloat(long, int);
#endif

/*  fpu387 -- assembler file prototypes */

extern void cdecl FPUaptan387(double *, double *, double *);
extern void cdecl FPUcplxexp387(_CMPLX *, _CMPLX *);

/*  fracsuba -- assembler file prototypes */

extern int bail_out_mod_l();
extern int bail_out_real_l();
extern int bail_out_imag_l();
extern int bail_out_or_l();
extern int bail_out_and_l();
extern int bail_out_manhattan_l();
extern int bail_out_manhattan_r_l();

/* history -- C file prototypes */

void _fastcall history_restore_info();
void _fastcall history_save_info();
void history_allocate();
void history_free();
void history_back();
void history_forward();

/*  mpmath_a -- assembler file prototypes */

extern struct MP * MPmul386(struct MP, struct MP);
extern struct MP * MPdiv386(struct MP, struct MP);
extern struct MP * MPadd386(struct MP, struct MP);
extern int         MPcmp386(struct MP, struct MP);
extern double    * MP2d386(struct MP);
extern struct MP * fg2MP386(long, int);
extern double *    MP2d(struct MP);
extern int         MPcmp(struct MP, struct MP);
extern struct MP * MPmul(struct MP, struct MP);
extern struct MP * MPadd(struct MP, struct MP);
extern struct MP * MPdiv(struct MP, struct MP);
extern struct MP * d2MP(double);  /* Convert double to type MP */
extern struct MP * fg2MP(long, int); /* Convert fudged to type MP */

/*  newton -- assembler file prototypes */

extern void cdecl   invert_z(_CMPLX *);

/*  tplus_a -- assembler file prototypes */

extern void WriteTPlusBankedPixel(int, int, unsigned long);
extern unsigned long ReadTPlusBankedPixel(int, int);

/*  3d -- C file prototypes */

extern void identity(MATRIX);
extern void mat_mul(MATRIX, MATRIX, MATRIX);
extern void scale(double, double, double, MATRIX);
extern void xrot(double, MATRIX);
extern void yrot(double, MATRIX);
extern void zrot(double, MATRIX);
extern void trans(double, double, double, MATRIX);
extern int cross_product(VECTOR, VECTOR, VECTOR);
extern int normalize_vector(VECTOR);
extern int vmult(VECTOR, MATRIX, VECTOR);
extern void mult_vec(VECTOR, MATRIX);
extern int perspective(VECTOR);
extern int longvmultpersp(LVECTOR, LMATRIX, LVECTOR, LVECTOR, LVECTOR, int);
extern int longpersp(LVECTOR, LVECTOR, int);
extern int longvmult(LVECTOR, LMATRIX, LVECTOR, int);

/*  biginit -- C file prototypes */

void free_bf_vars();
bn_t alloc_stack(size_t size);
int save_stack();
void restore_stack(int old_offset);
void init_bf_dec(int dec);
void init_bf_length(int bnl);
void init_big_pi();

/*  calcfrac -- C file prototypes */

extern int calculate_fractal();
extern int calculate_mandelbrot();
extern int calculate_mandelbrot_fp();
extern int standard_fractal();
extern int test();
extern int plasma();
extern int diffusion();
extern int bifurcation();
extern int bifurcation_lambda();
extern int bifurcation_set_trig_pi_fp();
extern int bifurcation_set_trig_pi();
extern int bifurcation_add_trig_pi_fp();
extern int bifurcation_add_trig_pi();
extern int bifurcation_may_fp();
extern int bifurcation_may_setup();
extern int bifurcation_may();
extern int bifurcation_lambda_trig_fp();
extern int bifurcation_lambda_trig();
extern int bifurcation_verhulst_trig_fp();
extern int bifurcation_verhulst_trig();
extern int bifurcation_stewart_trig_fp();
extern int bifurcation_stewart_trig();
extern int popcorn();
extern int lyapunov();
extern int lyapunov_setup();
extern int cellular();
extern int cellular_setup();
extern int froth_calc();
extern int froth_per_pixel();
extern int froth_per_orbit();
extern int froth_setup();
extern int logtable_in_extra_ok();
extern int find_alternate_math(int, int);

/*  cmdfiles -- C file prototypes */

extern int command_files(int, char **);
extern int load_commands(FILE *);
extern int get_curarg_len(char *curarg);
extern int get_max_curarg_len(char *floatvalstr[], int totparm);
extern int init_msg(const char *, char *, int);
extern int process_command(char *curarg, int mode);
extern int get_power_10(LDBL x);
extern void pause_error(int);
extern int bad_arg(const char *curarg);

/*  decoder -- C file prototypes */

extern short decoder(short);
extern void set_byte_buff(BYTE *ptr);

/*  diskvid -- C file prototypes */

extern int disk_start_potential();
extern int disk_start_targa(FILE *, int);
extern void disk_end();
extern int disk_read(int, int);
extern void disk_write(int, int, int);
extern void disk_read_targa(unsigned int, unsigned int, BYTE *, BYTE *, BYTE *);
extern void disk_write_targa(unsigned int, unsigned int, BYTE, BYTE, BYTE);
extern void disk_video_status(int, char *);
extern int  _fastcall disk_start_common(long, long, int);
extern int disk_from_memory(long, int, void *);
extern int disk_to_memory(long, int, void *);

/*  editpal -- C file prototypes */

extern void palette_edit();
void put_row(int x, int y, int width, char *buff);
void get_row(int x, int y, int width, char *buff);
/* void hline(int x, int y, int width, int color); */
int cursor_wait_key();
void cursor_check_blink();
#ifdef XFRACT
void cursor_start_mouse_tracking();
void cursor_end_mouse_tracking();
#endif
BOOLEAN cursor_new();
void cursor_destroy();
void cursor_set_position(int x, int y);
void cursor_move(int xoff, int yoff);
int cursor_get_x();
int cursor_get_y();
void cursor_hide();
void cursor_show();
extern void displayc(int, int, int, int, int);

/*  encoder -- C file prototypes */

extern int save_to_disk(char *);
extern int encoder();
extern int _fastcall new_to_old(int new_fractype);

/*  evolve -- C file prototypes */

extern void save_parameter_history();
extern void restore_parameter_history();
extern  int get_evolve_parameters();
extern  void set_current_parameters();
extern  void fiddle_parameters(GENEBASE gene[], int ecount);
extern  void set_evolve_ranges();
extern  void set_mutation_level(int);
extern  void draw_parameter_box(int);
extern  void spiral_map(int);
extern  int unspiral_map();
extern  void setup_parameter_box();
extern  void release_parameter_box();

/*  f16 -- C file prototypes */

extern FILE *t16_open(char *, int *, int *, int *, U8 *);
extern int t16_getline(FILE *, int, U16 *);

/*  fracsubr -- C file prototypes */

extern void calculate_fractal_initialize();
extern void adjust_corner();
#ifndef USE_VARARGS
extern int put_resume(int, ...);
extern int get_resume(int, ...);
#else
extern int put_resume();
extern int get_resume();
#endif
extern int alloc_resume(int, int);
extern int start_resume();
extern void end_resume();
extern void sleep_ms(long);
extern void reset_clock();
extern void plot_orbit_i(long, long, int);
extern void plot_orbit(double, double, int);
extern void orbit_scrub();
extern int work_list_add(int, int, int, int, int, int, int, int);
extern void work_list_tidy();
extern void get_julia_attractor(double, double);
extern int solid_guess_block_size();
extern void _fastcall symPIplot(int, int, int);
extern void _fastcall symPIplot2J(int, int, int);
extern void _fastcall symPIplot4J(int, int, int);
extern void _fastcall symplot2(int, int, int);
extern void _fastcall symplot2Y(int, int, int);
extern void _fastcall symplot2J(int, int, int);
extern void _fastcall symplot4(int, int, int);
extern void _fastcall symplot2basin(int, int, int);
extern void _fastcall symplot4basin(int, int, int);
extern void _fastcall noplot(int, int, int);
extern void fractal_float_to_bf();
extern void adjust_corner_bf();

/*  fractalp -- C file prototypes */

extern int type_has_parameter(int, int, char *);
extern int parameter_not_used(int);

/*  fractals -- C file prototypes */

extern void magnet2_precalculate_fp();
extern void complex_power(_CMPLX *, int, _CMPLX *);
extern int complex_power_l(_LCMPLX *, int, _LCMPLX *, int);
extern int lcomplex_mult(_LCMPLX, _LCMPLX, _LCMPLX *, int);
extern int barnsley1_orbit();
extern int barnsley1_orbit_fp();
extern int barnsley2_orbit();
extern int barnsley2_orbit_fp();
extern int julia_orbit();
extern int julia_orbit_fp();
extern int lambda_orbit_fp();
extern int lambda_orbit();
extern int sierpinski_orbit();
extern int sierpinski_orbit_fp();
extern int lambda_exponent_orbit_fp();
extern int lambda_exponent_orbit();
extern int trig_plus_exponent_orbit_fp();
extern int trig_plus_exponent_orbit();
extern int marks_lambda_orbit();
extern int marks_lambda_orbit_fp();
extern int unity_orbit();
extern int unity_orbit_fp();
extern int mandel4_orbit();
extern int mandel4_orbit_fp();
extern int z_to_z_plus_z_orbit_fp();
extern int z_power_orbit();
extern int complex_z_power_orbit();
extern int z_power_orbit_fp();
extern int complex_z_power_orbit_fp();
extern int barnsley3_orbit();
extern int barnsley3_orbit_fp();
extern int trig_plus_z_squared_orbit();
extern int trig_plus_z_squared_orbit_fp();
extern int richard8_orbit_fp();
extern int richard8_orbit();
extern int popcorn_orbit_fp();
extern int popcorn_orbit();
extern int popcorn_old_orbit_fp();
extern int popcorn_old_orbit();
extern int popcorn_fn_orbit_fp();
extern int popcorn_fn_orbit();
extern int marks_complex_mandelbrot_orbit();
extern int spider_orbit_fp();
extern int spider_orbit();
extern int tetrate_orbit_fp();
extern int z_trig_z_plus_z_orbit();
extern int scott_z_trig_z_plus_z_orbit();
extern int skinner_z_trig_z_minus_z_orbit();
extern int z_trig_z_plus_z_orbit_fp();
extern int scott_z_trig_z_plus_z_orbit_fp();
extern int skinner_z_trig_z_minus_z_orbit_fp();
extern int sqr_1_over_trig_z_orbit();
extern int sqr_1_over_trig_z_orbit_fp();
extern int trig_plus_trig_orbit();
extern int trig_plus_trig_orbit_fp();
extern int scott_trig_plus_trig_orbit();
extern int scott_trig_plus_trig_orbit_fp();
extern int skinner_trig_sub_trig_orbit();
extern int skinner_trig_sub_trig_orbit_fp();
extern int trig_trig_orbit_fp();
extern int trig_trig_orbit();
extern int trig_plus_sqr_orbit();
extern int trig_plus_sqr_orbit_fp();
extern int scott_trig_plus_sqr_orbit();
extern int scott_trig_plus_sqr_orbit_fp();
extern int skinner_trig_sub_sqr_orbit();
extern int skinner_trig_sub_sqr_orbit_fp();
extern int trig_z_squared_orbit_fp();
extern int trig_z_squared_orbit();
extern int sqr_trig_orbit();
extern int sqr_trig_orbit_fp();
extern int magnet1_orbit_fp();
extern int magnet2_orbit_fp();
extern int lambda_trig_orbit();
extern int lambda_trig_orbit_fp();
extern int lambda_trig1_orbit();
extern int lambda_trig1_orbit_fp();
extern int lambda_trig2_orbit();
extern int lambda_trig2_orbit_fp();
extern int man_o_war_orbit();
extern int man_o_war_orbit_fp();
extern int marks_mandel_power_orbit_fp();
extern int marks_mandel_power_orbit();
extern int tims_error_orbit_fp();
extern int tims_error_orbit();
extern int circle_orbit_fp();
extern int volterra_lotka_orbit_fp();
extern int escher_orbit_fp();
extern int julia_per_pixel_l();
extern int richard8_per_pixel();
extern int mandelbrot_per_pixel_l();
extern int julia_per_pixel();
extern int marks_mandelbrot_power_per_pixel();
extern int mandelbrot_per_pixel();
extern int marks_mandelbrot_per_pixel();
extern int marks_mandelbrot_per_pixel_fp();
extern int marks_mandelbrot_power_per_pixel_fp();
extern int mandelbrot_per_pixel_fp();
extern int julia_per_pixel_fp();
extern int julia_per_pixel_mpc();
extern int other_richard8_per_pixel_fp();
extern int other_mandelbrot_per_pixel_fp();
extern int other_julia_per_pixel_fp();
extern int marks_complex_mandelbrot_per_pixel();
extern int lambda_trig_or_trig_orbit();
extern int lambda_trig_or_trig_orbit_fp();
extern int julia_trig_or_trig_orbit();
extern int julia_trig_or_trig_orbit_fp();
extern int dynamic_orbit_fp(double *, double *, double*);
extern int mandel_cloud_orbit_fp(double *, double *, double*);
extern int dynamic_2d_fp();
extern int quaternion_orbit_fp();
extern int quaternion_per_pixel_fp();
extern int quaternion_julia_per_pixel_fp();
extern int phoenix_orbit();
extern int phoenix_orbit_fp();
extern int phoenix_per_pixel();
extern int phoenix_per_pixel_fp();
extern int mandelbrot_phoenix_per_pixel();
extern int mandelbrot_phoenix_per_pixel_fp();
extern int hyper_complex_orbit_fp();
extern int phoenix_complex_orbit();
extern int phoenix_complex_orbit_fp();
extern int bail_out_mod_fp();
extern int bail_out_real_fp();
extern int bail_out_imag_fp();
extern int bail_out_or_fp();
extern int bail_out_and_fp();
extern int bail_out_manhattan_fp();
extern int bail_out_manhattan_r_fp();
extern int bail_out_mod_bn();
extern int bail_out_real_bn();
extern int bail_out_imag_bn();
extern int bail_out_or_bn();
extern int bail_out_and_bn();
extern int bail_out_manhattan_bn();
extern int bail_out_manhattan_r_bn();
extern int bail_out_mod_bf();
extern int bail_out_real_bf();
extern int bail_out_imag_bf();
extern int bail_out_or_bf();
extern int bail_out_and_bf();
extern int bail_out_manhattan_bf();
extern int bail_out_manhattan_r_bf();
extern int ant();
extern int phoenix_orbit();
extern int phoenix_orbit_fp();
extern int phoenix_complex_orbit();
extern int phoenix_complex_orbit_fp();
extern int phoenix_plus_orbit();
extern int phoenix_plus_orbit_fp();
extern int phoenix_minus_orbit();
extern int phoenix_minus_orbit_fp();
extern int phoenix_complex_plus_orbit();
extern int phoenix_complex_plus_orbit_fp();
extern int phoenix_complex_minus_orbit();
extern int phoenix_complex_minus_orbit_fp();
extern int phoenix_per_pixel();
extern int phoenix_per_pixel_fp();
extern int mandelbrot_phoenix_per_pixel();
extern int mandelbrot_phoenix_per_pixel_fp();
extern void set_pixel_calc_functions();
extern int mandelbrot_mix4_per_pixel_fp();
extern int mandelbrot_mix4_orbit_fp();
extern int mandelbrot_mix4_setup();

/*  fractint -- C file prototypes */

extern int main(int argc, char **argv);
extern int elapsed_time(int);

/*  framain2 -- C file prototypes */

extern int big_while_loop(int *kbd_more, int *stacked, int resume_flag);
extern int check_key();
extern int cmp_line(BYTE *, int);
extern int key_count(int);
extern int main_menu_switch(int *, int *, int *, int *, int);
extern int potential_line(BYTE *, int);
extern int sound_line(BYTE *, int);
#if !defined(XFRACT)
#if !defined(_WIN32)
extern int _cdecl _matherr(struct exception *);
#endif
#else
extern int XZoomWaiting;
#endif
#ifndef USE_VARARGS
extern int timer(int, int (*subrtn)(), ...);
#else
extern int timer();
#endif

extern void clear_zoom_box();
extern void flip_image(int kbdchar);
#ifndef WINFRACT
extern void reset_zoom_corners();
#endif

/*  frasetup -- C file prototypes */

extern int volterra_lotka_setup();
extern int mandelbrot_setup();
extern int mandelbrot_setup_fp();
extern int julia_setup();
extern int stand_alone_setup();
extern int unity_setup();
extern int julia_setup_fp();
extern int mandelbrot_setup_l();
extern int julia_setup_l();
extern int trig_plus_sqr_setup_l();
extern int trig_plus_sqr_setup_fp();
extern int trig_plus_trig_setup_l();
extern int trig_plus_trig_setup_fp();
extern int z_trig_plus_z_setup();
extern int lambda_trig_setup();
extern int julia_fn_plus_z_squared_setup();
extern int sqr_trig_setup();
extern int fn_fn_setup();
extern int mandelbrot_trig_setup();
extern int marks_julia_setup();
extern int marks_julia_setup_fp();
extern int sierpinski_setup();
extern int sierpinski_setup_fp();
extern int standard_setup();
extern int lambda_trig_or_trig_setup();
extern int julia_trig_or_trig_setup();
extern int mandelbrot_lambda_trig_or_trig_setup();
extern int mandelbrot_trig_or_trig_setup();
extern int halley_setup();
extern int dynamic_2d_setup_fp();
extern int phoenix_setup();
extern int mandelbrot_phoenix_setup();
extern int phoenix_complex_setup();
extern int mandelbrot_phoenix_complex_setup();

/*  gifview -- C file prototypes */

extern int get_byte();
extern int get_bytes(BYTE *, int);
extern int gifview();

/*  hcmplx -- C file prototypes */

extern void HComplexTrig0(_HCMPLX *, _HCMPLX *);

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
extern LCMPLX PopLong();
extern _CMPLX PopFloat();
extern LCMPLX DeQueueLong();
extern _CMPLX DeQueueFloat();
extern LCMPLX ComplexSqrtLong(long,  long);
extern _CMPLX ComplexSqrtFloat(double, double);
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
extern int _fastcall is_pow2(int);
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
extern int find_file_item(char *, char *, FILE **, int);
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
extern _CMPLX MPC2cmplx(struct MPC);
extern struct MPC cmplx2MPC(_CMPLX);
extern _CMPLX ComplexPower(_CMPLX, _CMPLX);
extern void SetupLogTable();
extern long logtablecalc(long);
extern long ExpFloat14(long);
extern int complex_basin();
extern int gaussian_number(int, int);
extern void Arcsinz(_CMPLX z, _CMPLX *rz);
extern void Arccosz(_CMPLX z, _CMPLX *rz);
extern void Arcsinhz(_CMPLX z, _CMPLX *rz);
extern void Arccoshz(_CMPLX z, _CMPLX *rz);
extern void Arctanhz(_CMPLX z, _CMPLX *rz);
extern void Arctanz(_CMPLX z, _CMPLX *rz);

/*  msccos -- C file prototypes */

extern double _cos(double);

/*  parser -- C file prototypes */

extern unsigned long new_random_number();
extern void lRandom();
extern void dRandom();
extern void mRandom();
extern void SetRandFnct();
extern void RandomSeed();
extern void lStkSRand();
extern void mStkSRand();
extern void dStkSRand();
extern void dStkAbs();
extern void mStkAbs();
extern void lStkAbs();
extern void dStkSqr();
extern void mStkSqr();
extern void lStkSqr();
extern void dStkAdd();
extern void mStkAdd();
extern void lStkAdd();
extern void dStkSub();
extern void mStkSub();
extern void lStkSub();
extern void dStkConj();
extern void mStkConj();
extern void lStkConj();
extern void dStkZero();
extern void mStkZero();
extern void lStkZero();
extern void dStkOne();
extern void mStkOne();
extern void lStkOne();
extern void dStkReal();
extern void mStkReal();
extern void lStkReal();
extern void dStkImag();
extern void mStkImag();
extern void lStkImag();
extern void dStkNeg();
extern void mStkNeg();
extern void lStkNeg();
extern void dStkMul();
extern void mStkMul();
extern void lStkMul();
extern void dStkDiv();
extern void mStkDiv();
extern void lStkDiv();
extern void StkSto();
extern void StkLod();
extern void dStkMod();
extern void mStkMod();
extern void lStkMod();
extern void StkClr();
extern void dStkFlip();
extern void mStkFlip();
extern void lStkFlip();
extern void dStkSin();
extern void mStkSin();
extern void lStkSin();
extern void dStkTan();
extern void mStkTan();
extern void lStkTan();
extern void dStkTanh();
extern void mStkTanh();
extern void lStkTanh();
extern void dStkCoTan();
extern void mStkCoTan();
extern void lStkCoTan();
extern void dStkCoTanh();
extern void mStkCoTanh();
extern void lStkCoTanh();
extern void dStkRecip();
extern void mStkRecip();
extern void lStkRecip();
extern void StkIdent();
extern void dStkSinh();
extern void mStkSinh();
extern void lStkSinh();
extern void dStkCos();
extern void mStkCos();
extern void lStkCos();
extern void dStkCosXX();
extern void mStkCosXX();
extern void lStkCosXX();
extern void dStkCosh();
extern void mStkCosh();
extern void lStkCosh();
extern void dStkLT();
extern void mStkLT();
extern void lStkLT();
extern void dStkGT();
extern void mStkGT();
extern void lStkGT();
extern void dStkLTE();
extern void mStkLTE();
extern void lStkLTE();
extern void dStkGTE();
extern void mStkGTE();
extern void lStkGTE();
extern void dStkEQ();
extern void mStkEQ();
extern void lStkEQ();
extern void dStkNE();
extern void mStkNE();
extern void lStkNE();
extern void dStkOR();
extern void mStkOR();
extern void lStkOR();
extern void dStkAND();
extern void mStkAND();
extern void lStkAND();
extern void dStkLog();
extern void mStkLog();
extern void lStkLog();
extern void FPUcplxexp(_CMPLX *, _CMPLX *);
extern void dStkExp();
extern void mStkExp();
extern void lStkExp();
extern void dStkPwr();
extern void mStkPwr();
extern void lStkPwr();
extern void dStkASin();
extern void mStkASin();
extern void lStkASin();
extern void dStkASinh();
extern void mStkASinh();
extern void lStkASinh();
extern void dStkACos();
extern void mStkACos();
extern void lStkACos();
extern void dStkACosh();
extern void mStkACosh();
extern void lStkACosh();
extern void dStkATan();
extern void mStkATan();
extern void lStkATan();
extern void dStkATanh();
extern void mStkATanh();
extern void lStkATanh();
extern void dStkCAbs();
extern void mStkCAbs();
extern void lStkCAbs();
extern void dStkSqrt();
extern void mStkSqrt();
extern void lStkSqrt();
extern void dStkFloor();
extern void mStkFloor();
extern void lStkFloor();
extern void dStkCeil();
extern void mStkCeil();
extern void lStkCeil();
extern void dStkTrunc();
extern void mStkTrunc();
extern void lStkTrunc();
extern void dStkRound();
extern void mStkRound();
extern void lStkRound();
extern void EndInit();
extern struct ConstArg *is_constant(char *, int);
extern void not_a_function();
extern void function_not_found();
extern int whichfn(char *, int);
extern int CvtStk();
extern int fFormula();
#if !defined(XFRACT)
typedef void t_function();
extern t_function *is_function(char *, int);
#endif
extern void RecSortPrec();
extern int Formula();
extern int BadFormula();
extern int form_per_pixel();
extern int frm_get_param_stuff (char *);
extern int RunForm(char *, int);
extern int formula_setup_fp();
extern int formula_setup_int();
extern void init_misc();
extern void free_work_area();
extern int fill_if_group(int endif_index, JUMP_PTRS_ST *jump_data);

/*  plot3d -- C file prototypes */

extern void cdecl draw_line(int, int, int, int, int);
extern void _fastcall plot_3d_superimpose_16(int, int, int);
extern void _fastcall plot_3d_superimpose_256(int, int, int);
extern void _fastcall plot_ifs_3d_superimpose_256(int, int, int);
extern void _fastcall plot_3d_alternate(int, int, int);
extern void plot_setup();

/*  prompts1 -- C file prototypes */

extern int full_screen_prompt(const char *hdg, int numprompts, char **prompts,
	struct full_screen_values *values, int fkeymask, char *extrainfo);
extern long get_file_entry(int, char *, char *, char *, char *);
extern int get_fractal_type();
extern int get_fractal_parameters(int);
extern int get_fractal_3d_parameters();
extern int get_3d_parameters();
extern int prompt_value_string(char *buf, struct full_screen_values *val);
extern void set_bail_out_formula(enum bailouts);
extern int find_extra_parameter(int);
extern void load_parameters(int g_fractal_type);
extern int check_orbit_name(char *);
extern int scan_entries(FILE *infile, struct entryinfo *choices, char *itemname);

/*  prompts2 -- C file prototypes */

extern int get_toggles();
extern int get_toggles2();
extern int passes_options();
extern int get_view_params();
extern int get_starfield_params();
extern int get_commands();
extern void goodbye();
extern int is_a_directory(char *s);
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
extern int stop_message(int, char *);
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
extern int main_menu(int);
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
extern void _fastcall draw_lines(struct coords, struct coords, int, int);
extern void _fastcall add_box(struct coords);
extern void clear_box();
extern void display_box();

/*  fractalb.c -- C file prototypes */

extern _CMPLX complex_bn_to_float(_BNCMPLX *);
extern _CMPLX complex_bf_to_float(_BFCMPLX *);
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
extern _BNCMPLX *complex_log_bn(_BNCMPLX *t, _BNCMPLX *s);
extern _BNCMPLX *complex_multiply_bn(_BNCMPLX *t, _BNCMPLX *x, _BNCMPLX *y);
extern _BNCMPLX *complex_power_bn(_BNCMPLX *t, _BNCMPLX *xx, _BNCMPLX *yy);
extern int mandelbrot_setup_bf();
extern int mandelbrot_per_pixel_bf();
extern int julia_per_pixel_bf();
extern int julia_orbit_bf();
extern int julia_z_power_orbit_bf();
extern _BFCMPLX *complex_log_bf(_BFCMPLX *t, _BFCMPLX *s);
extern _BFCMPLX *cplxmul_bf(_BFCMPLX *t, _BFCMPLX *x, _BFCMPLX *y);
extern _BFCMPLX *ComplexPower_bf(_BFCMPLX *t, _BFCMPLX *xx, _BFCMPLX *yy);

/*  memory -- C file prototypes */
/* TODO: Get rid of this and use regular memory routines;
** see about creating standard disk memory routines for disk video
*/
extern int MemoryType(U16 handle);
extern void InitMemory();
extern void ExitCheck();
extern U16 MemoryAlloc(U16 size, long count, int stored_at);
extern void MemoryRelease(U16 handle);
extern int MoveToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern int MoveFromMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern int SetMemory(int value, U16 size, long count, long offset, U16 handle);

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
