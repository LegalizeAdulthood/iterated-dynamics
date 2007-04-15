#ifndef PROTOTYP_H
#define PROTOTYP_H

/* includes needed to define the prototypes */

#include "mpmath.h"
#include "big.h"
#include "fractint.h"
#include "helpcom.h"
#include "externs.h"

extern int get_corners(void);

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

/*  calcmand -- assembler file prototypes */

extern long cdecl calculate_mandelbrot_asm(void);

/*  calmanfp -- assembler file prototypes */

extern void cdecl calculate_mandelbrot_start_fp_asm(void);
/* extern long  cdecl g_calculate_mandelbrot_asm_fp(void); */
extern long  cdecl calculate_mandelbrot_fp_287_asm(void);
extern long  cdecl calculate_mandelbrot_fp_87_asm(void);

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

extern int bail_out_mod_l_asm(void);
extern int bail_out_real_l_asm(void);
extern int bail_out_imag_l_asm(void);
extern int bail_out_or_l_asm(void);
extern int bail_out_and_l_asm(void);
extern int bail_out_manhattan_l_asm(void);
extern int bail_out_manhattan_r_l_asm(void);
extern int asm386lMODbailout(void);
extern int asm386lREALbailout(void);
extern int asm386lIMAGbailout(void);
extern int asm386lORbailout(void);
extern int asm386lANDbailout(void);
extern int asm386lMANHbailout(void);
extern int asm386lMANRbailout(void);
extern int bail_out_mod_fp_asm(void);
extern int bail_out_real_fp_asm(void);
extern int bail_out_imag_fp_asm(void);
extern int bail_out_or_fp_asm(void);
extern int bail_out_and_fp_asm(void);
extern int bail_out_manhattan_fp_asm(void);
extern int bail_out_manhattan_r_fp_asm(void);

/* history -- C file prototypes */

void _fastcall history_restore_info(void);
void _fastcall history_save_info(void);
void history_allocate(void);
void history_free(void);
void history_back(void);
void history_forward(void);

/*  mpmath_a -- assembler file prototypes */

extern struct MP * MPmul086(struct MP, struct MP);
extern struct MP * MPdiv086(struct MP, struct MP);
extern struct MP * MPadd086(struct MP, struct MP);
extern int         MPcmp086(struct MP, struct MP);
extern struct MP * d2MP086(double);
extern double    * MP2d086(struct MP);
extern struct MP * fg2MP086(long, int);
extern struct MP * MPmul386(struct MP, struct MP);
extern struct MP * MPdiv386(struct MP, struct MP);
extern struct MP * MPadd386(struct MP, struct MP);
extern int         MPcmp386(struct MP, struct MP);
extern struct MP * d2MP386(double);
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

extern int cdecl    newton2_orbit(void);
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

void free_bf_vars(void);
bn_t alloc_stack(size_t size);
int save_stack(void);
void restore_stack(int old_offset);
void init_bf_dec(int dec);
void init_bf_length(int bnl);
void init_big_pi(void);

/*  calcfrac -- C file prototypes */

extern int calculate_fractal(void);
extern int calculate_mandelbrot(void);
extern int calculate_mandelbrot_fp(void);
extern int standard_fractal(void);
extern int test(void);
extern int plasma(void);
extern int diffusion(void);
extern int bifurcation(void);
extern int bifurcation_lambda(void);
extern int bifurcation_set_trig_pi_fp(void);
extern int bifurcation_set_trig_pi(void);
extern int bifurcation_add_trig_pi_fp(void);
extern int bifurcation_add_trig_pi(void);
extern int bifurcation_may_fp(void);
extern int bifurcation_may_setup(void);
extern int bifurcation_may(void);
extern int bifurcation_lambda_trig_fp(void);
extern int bifurcation_lambda_trig(void);
extern int bifurcation_verhulst_trig_fp(void);
extern int bifurcation_verhulst_trig(void);
extern int bifurcation_stewart_trig_fp(void);
extern int bifurcation_stewart_trig(void);
extern int popcorn(void);
extern int lyapunov(void);
extern int lyapunov_setup(void);
extern int cellular(void);
extern int cellular_setup(void);
extern int froth_calc(void);
extern int froth_per_pixel(void);
extern int froth_per_orbit(void);
extern int froth_setup(void);
extern int logtable_in_extra_ok(void);
extern int find_alternate_math(int, int);

/*  cmdfiles -- C file prototypes */

extern int command_files(int, char **);
extern int load_commands(FILE *);
extern void set_3d_defaults(void);
extern int get_curarg_len(char *curarg);
extern int get_max_curarg_len(char *floatvalstr[], int totparm);
extern int init_msg(const char *, char *, int);
extern int process_command(char *curarg, int mode);
extern int get_power_10(LDBL x);
extern void pause_error(int);

/*  decoder -- C file prototypes */

extern short decoder(short);
extern void set_byte_buff(BYTE *ptr);

/*  diskvid -- C file prototypes */

extern int disk_start_potential(void);
extern int disk_start_targa(FILE *, int);
extern void disk_end(void);
extern int disk_read(int, int);
extern void disk_write(int, int, int);
extern void disk_read_targa(unsigned int, unsigned int, BYTE *, BYTE *, BYTE *);
extern void disk_write_targa(unsigned int, unsigned int, BYTE, BYTE, BYTE);
extern void disk_video_status(int, char *);
extern int  _fastcall disk_start_common(long, long, int);
extern int disk_from_memory(long, int, void *);
extern int disk_to_memory(long, int, void *);

/*  editpal -- C file prototypes */

extern void palette_edit(void);
void put_row(int x, int y, int width, char *buff);
void get_row(int x, int y, int width, char *buff);
/* void hline(int x, int y, int width, int color); */
int cursor_wait_key(void);
void cursor_check_blink(void);
#ifdef XFRACT
void cursor_start_mouse_tracking(void);
void cursor_end_mouse_tracking(void);
#endif
BOOLEAN cursor_new(void);
void cursor_destroy(void);
void cursor_set_position(int x, int y);
void cursor_move(int xoff, int yoff);
int cursor_get_x(void);
int cursor_get_y(void);
void cursor_hide(void);
void cursor_show(void);
extern void displayc(int, int, int, int, int);

/*  encoder -- C file prototypes */

extern int save_to_disk(char *);
extern int encoder(void);
extern int _fastcall new_to_old(int new_fractype);

/*  evolve -- C file prototypes */

extern void save_parameter_history(void);
extern void restore_parameter_history(void);
extern  int get_variations(void);
extern  int get_evolve_parameters(void);
extern  void set_current_parameters(void);
extern  void fiddle_parameters(GENEBASE gene[], int ecount);
extern  void set_evolve_ranges(void);
extern  void set_mutation_level(int);
extern  void draw_parameter_box(int);
extern  void spiral_map(int);
extern  int unspiral_map(void);
extern  int explore_check(void);
extern  void setup_parameter_box(void);
extern  void release_parameter_box(void);

/*  f16 -- C file prototypes */

extern FILE *t16_open(char *, int *, int *, int *, U8 *);
extern int t16_getline(FILE *, int, U16 *);

/*  fracsubr -- C file prototypes */

extern void free_grid_pointers(void);
extern void calculate_fractal_initialize(void);
extern void adjust_corner(void);
#ifndef USE_VARARGS
extern int put_resume(int, ...);
extern int get_resume(int, ...);
#else
extern int put_resume();
extern int get_resume();
#endif
extern int alloc_resume(int, int);
extern int start_resume(void);
extern void end_resume(void);
extern void sleep_ms(long);
extern void reset_clock(void);
extern void plot_orbit_i(long, long, int);
extern void plot_orbit(double, double, int);
extern void orbit_scrub(void);
extern int work_list_add(int, int, int, int, int, int, int, int);
extern void work_list_tidy(void);
extern void get_julia_attractor(double, double);
extern int solid_guess_block_size(void);
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
extern void fractal_float_to_bf(void);
extern void adjust_corner_bf(void);
extern void set_grid_pointers(void);
extern void fill_dx_array(void);
extern void fill_lx_array(void);
extern void sound_tone(int);
extern void sound_write_time(void);
extern void sound_close(void);

/*  fractalp -- C file prototypes */

extern int type_has_parameter(int, int, char *);
extern int parameter_not_used(int);

/*  fractals -- C file prototypes */

extern void magnet2_precalculate_fp(void);
extern void complex_power(_CMPLX *, int, _CMPLX *);
extern int complex_power_l(_LCMPLX *, int, _LCMPLX *, int);
extern int lcomplex_mult(_LCMPLX, _LCMPLX, _LCMPLX *, int);
extern int newton_orbit_mpc(void);
extern int barnsley1_orbit(void);
extern int barnsley1_orbit_fp(void);
extern int barnsley2_orbit(void);
extern int barnsley2_orbit_fp(void);
extern int julia_orbit(void);
extern int julia_orbit_fp(void);
extern int lambda_orbit_fp(void);
extern int lambda_orbit(void);
extern int sierpinski_orbit(void);
extern int sierpinski_orbit_fp(void);
extern int lambda_exponent_orbit_fp(void);
extern int lambda_exponent_orbit(void);
extern int trig_plus_exponent_orbit_fp(void);
extern int trig_plus_exponent_orbit(void);
extern int marks_lambda_orbit(void);
extern int marks_lambda_orbit_fp(void);
extern int unity_orbit(void);
extern int unity_orbit_fp(void);
extern int mandel4_orbit(void);
extern int mandel4_orbit_fp(void);
extern int z_to_z_plus_z_orbit_fp(void);
extern int z_power_orbit(void);
extern int complex_z_power_orbit(void);
extern int z_power_orbit_fp(void);
extern int complex_z_power_orbit_fp(void);
extern int barnsley3_orbit(void);
extern int barnsley3_orbit_fp(void);
extern int trig_plus_z_squared_orbit(void);
extern int trig_plus_z_squared_orbit_fp(void);
extern int richard8_orbit_fp(void);
extern int richard8_orbit(void);
extern int popcorn_orbit_fp(void);
extern int popcorn_orbit(void);
extern int popcorn_old_orbit_fp(void);
extern int popcorn_old_orbit(void);
extern int popcorn_fn_orbit_fp(void);
extern int popcorn_fn_orbit(void);
extern int marks_complex_mandelbrot_orbit(void);
extern int spider_orbit_fp(void);
extern int spider_orbit(void);
extern int tetrate_orbit_fp(void);
extern int z_trig_z_plus_z_orbit(void);
extern int scott_z_trig_z_plus_z_orbit(void);
extern int skinner_z_trig_z_minus_z_orbit(void);
extern int z_trig_z_plus_z_orbit_fp(void);
extern int scott_z_trig_z_plus_z_orbit_fp(void);
extern int skinner_z_trig_z_minus_z_orbit_fp(void);
extern int sqr_1_over_trig_z_orbit(void);
extern int sqr_1_over_trig_z_orbit_fp(void);
extern int trig_plus_trig_orbit(void);
extern int trig_plus_trig_orbit_fp(void);
extern int scott_trig_plus_trig_orbit(void);
extern int scott_trig_plus_trig_orbit_fp(void);
extern int skinner_trig_sub_trig_orbit(void);
extern int skinner_trig_sub_trig_orbit_fp(void);
extern int trig_trig_orbit_fp(void);
extern int trig_trig_orbit(void);
extern int trig_plus_sqr_orbit(void);
extern int trig_plus_sqr_orbit_fp(void);
extern int scott_trig_plus_sqr_orbit(void);
extern int scott_trig_plus_sqr_orbit_fp(void);
extern int skinner_trig_sub_sqr_orbit(void);
extern int skinner_trig_sub_sqr_orbit_fp(void);
extern int trig_z_squared_orbit_fp(void);
extern int trig_z_squared_orbit(void);
extern int sqr_trig_orbit(void);
extern int sqr_trig_orbit_fp(void);
extern int magnet1_orbit_fp(void);
extern int magnet2_orbit_fp(void);
extern int lambda_trig_orbit(void);
extern int lambda_trig_orbit_fp(void);
extern int lambda_trig1_orbit(void);
extern int lambda_trig1_orbit_fp(void);
extern int lambda_trig2_orbit(void);
extern int lambda_trig2_orbit_fp(void);
extern int man_o_war_orbit(void);
extern int man_o_war_orbit_fp(void);
extern int marks_mandel_power_orbit_fp(void);
extern int marks_mandel_power_orbit(void);
extern int tims_error_orbit_fp(void);
extern int tims_error_orbit(void);
extern int circle_orbit_fp(void);
extern int volterra_lotka_orbit_fp(void);
extern int escher_orbit_fp(void);
extern int julia_per_pixel_l(void);
extern int richard8_per_pixel(void);
extern int mandelbrot_per_pixel_l(void);
extern int julia_per_pixel(void);
extern int marks_mandelbrot_power_per_pixel(void);
extern int mandelbrot_per_pixel(void);
extern int marks_mandelbrot_per_pixel(void);
extern int marks_mandelbrot_per_pixel_fp(void);
extern int marks_mandelbrot_power_per_pixel_fp(void);
extern int mandelbrot_per_pixel_fp(void);
extern int julia_per_pixel_fp(void);
extern int julia_per_pixel_mpc(void);
extern int other_richard8_per_pixel_fp(void);
extern int other_mandelbrot_per_pixel_fp(void);
extern int other_julia_per_pixel_fp(void);
extern int marks_complex_mandelbrot_per_pixel(void);
extern int lambda_trig_or_trig_orbit(void);
extern int lambda_trig_or_trig_orbit_fp(void);
extern int julia_trig_or_trig_orbit(void);
extern int julia_trig_or_trig_orbit_fp(void);
extern int halley_orbit_fp(void);
extern int halley_per_pixel(void);
extern int halley_orbit_mpc(void);
extern int halley_per_pixel_mpc(void);
extern int dynamic_orbit_fp(double *, double *, double*);
extern int mandel_cloud_orbit_fp(double *, double *, double*);
extern int dynamic_2d_fp(void);
extern int quaternion_orbit_fp(void);
extern int quaternion_per_pixel_fp(void);
extern int quaternion_julia_per_pixel_fp(void);
extern int phoenix_orbit(void);
extern int phoenix_orbit_fp(void);
extern int phoenix_per_pixel(void);
extern int phoenix_per_pixel_fp(void);
extern int mandelbrot_phoenix_per_pixel(void);
extern int mandelbrot_phoenix_per_pixel_fp(void);
extern int hyper_complex_orbit_fp(void);
extern int phoenix_complex_orbit(void);
extern int phoenix_complex_orbit_fp(void);
extern int bail_out_mod_fp(void);
extern int bail_out_real_fp(void);
extern int bail_out_imag_fp(void);
extern int bail_out_or_fp(void);
extern int bail_out_and_fp(void);
extern int bail_out_manhattan_fp(void);
extern int bail_out_manhattan_r_fp(void);
extern int bail_out_mod_bn(void);
extern int bail_out_real_bn(void);
extern int bail_out_imag_bn(void);
extern int bail_out_or_bn(void);
extern int bail_out_and_bn(void);
extern int bail_out_manhattan_bn(void);
extern int bail_out_manhattan_r_bn(void);
extern int bail_out_mod_bf(void);
extern int bail_out_real_bf(void);
extern int bail_out_imag_bf(void);
extern int bail_out_or_bf(void);
extern int bail_out_and_bf(void);
extern int bail_out_manhattan_bf(void);
extern int bail_out_manhattan_r_bf(void);
extern int ant(void);
extern void free_ant_storage(void);
extern int phoenix_orbit(void);
extern int phoenix_orbit_fp(void);
extern int phoenix_complex_orbit(void);
extern int phoenix_complex_orbit_fp(void);
extern int phoenix_plus_orbit(void);
extern int phoenix_plus_orbit_fp(void);
extern int phoenix_minus_orbit(void);
extern int phoenix_minus_orbit_fp(void);
extern int phoenix_complex_plus_orbit(void);
extern int phoenix_complex_plus_orbit_fp(void);
extern int phoenix_complex_minus_orbit(void);
extern int phoenix_complex_minus_orbit_fp(void);
extern int phoenix_per_pixel(void);
extern int phoenix_per_pixel_fp(void);
extern int mandelbrot_phoenix_per_pixel(void);
extern int mandelbrot_phoenix_per_pixel_fp(void);
extern void set_pixel_calc_functions(void);
extern int mandelbrot_mix4_per_pixel_fp(void);
extern int mandelbrot_mix4_orbit_fp(void);
extern int mandelbrot_mix4_setup(void);

/*  fractint -- C file prototypes */

extern int main(int argc, char **argv);
extern int elapsed_time(int);

/*  framain2 -- C file prototypes */

extern int big_while_loop(int *kbd_more, int *stacked, int resume_flag);
extern int check_key(void);
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

extern void clear_zoom_box(void);
extern void flip_image(int kbdchar);
#ifndef WINFRACT
extern void reset_zoom_corners(void);
#endif

/*  frasetup -- C file prototypes */

extern int volterra_lotka_setup(void);
extern int mandelbrot_setup(void);
extern int mandelbrot_setup_fp(void);
extern int julia_setup(void);
extern int newton_setup(void);
extern int stand_alone_setup(void);
extern int unity_setup(void);
extern int julia_setup_fp(void);
extern int mandelbrot_setup_l(void);
extern int julia_setup_l(void);
extern int trig_plus_sqr_setup_l(void);
extern int trig_plus_sqr_setup_fp(void);
extern int trig_plus_trig_setup_l(void);
extern int trig_plus_trig_setup_fp(void);
extern int z_trig_plus_z_setup(void);
extern int lambda_trig_setup(void);
extern int julia_fn_plus_z_squared_setup(void);
extern int sqr_trig_setup(void);
extern int fn_fn_setup(void);
extern int mandelbrot_trig_setup(void);
extern int marks_julia_setup(void);
extern int marks_julia_setup_fp(void);
extern int sierpinski_setup(void);
extern int sierpinski_setup_fp(void);
extern int standard_setup(void);
extern int lambda_trig_or_trig_setup(void);
extern int julia_trig_or_trig_setup(void);
extern int mandelbrot_lambda_trig_or_trig_setup(void);
extern int mandelbrot_trig_or_trig_setup(void);
extern int halley_setup(void);
extern int dynamic_2d_setup_fp(void);
extern int phoenix_setup(void);
extern int mandelbrot_phoenix_setup(void);
extern int phoenix_complex_setup(void);
extern int mandelbrot_phoenix_complex_setup(void);

/*  gifview -- C file prototypes */

extern int get_byte(void);
extern int get_bytes(BYTE *, int);
extern int gifview(void);

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
extern void print_document(char *, int (*)(int, int), int);
extern int init_help(void);
extern void end_help(void);

/*  intro -- C file prototypes */

extern void intro(void);

/*  jb -- C file prototypes */

extern int julibrot_setup(void);
extern int julibrot_setup_fp(void);
extern int julibrot_per_pixel(void);
extern int julibrot_per_pixel_fp(void);
extern int standard_4d_fractal(void);
extern int standard_4d_fractal_fp(void);

/*  jiim -- C file prototypes */

extern void Jiim(int);
extern LCMPLX PopLong(void);
extern _CMPLX PopFloat(void);
extern LCMPLX DeQueueLong(void);
extern _CMPLX DeQueueFloat(void);
extern LCMPLX ComplexSqrtLong(long,  long);
extern _CMPLX ComplexSqrtFloat(double, double);
extern int    Init_Queue(unsigned long);
extern void   Free_Queue(void);
extern void   ClearQueue(void);
extern int    QueueEmpty(void);
extern int    QueueFull(void);
extern int    QueueFullAlmost(void);
extern int    PushLong(long,  long);
extern int    PushFloat(float,  float);
extern int    EnQueueLong(long,  long);
extern int    EnQueueFloat(float,  float);

/*  line3d -- C file prototypes */

extern int line3d(BYTE *, unsigned int);
extern int _fastcall targa_color(int, int, int);
extern int start_disk1(char *, FILE *, int);
extern void line_3d_free(void);

/*  loadfdos -- C file prototypes */
#ifndef WINFRACT
extern int get_video_mode(struct fractal_info *, struct ext_blk_formula_info *);
#endif
/*  loadfile -- C file prototypes */

extern int read_overlay(void);
extern void set_if_old_bif(void);
extern void set_function_parm_defaults(void);
extern int look_get_window(void);
extern void backwards_v18(void);
extern void backwards_v19(void);
extern void backwards_v20(void);
extern int check_back(void);

/*  loadmap -- C file prototypes */

//extern void SetTgaColors(void);
extern int validate_luts(char *);
extern int set_color_palette_name(char *);

/*  lorenz -- C file prototypes */

extern int orbit_3d_setup(void);
extern int orbit_3d_setup_fp(void);
extern int lorenz_3d_orbit(long *, long *, long *);
extern int lorenz_3d_orbit_fp(double *, double *, double *);
extern int lorenz_3d1_orbit_fp(double *, double *, double *);
extern int lorenz_3d3_orbit_fp(double *, double *, double *);
extern int lorenz_3d4_orbit_fp(double *, double *, double *);
extern int henon_orbit_fp(double *, double *, double *);
extern int henon_orbit(long *, long *, long *);
extern int inverse_julia_orbit(double *, double *, double *);
extern int Minverse_julia_orbit(void);
extern int Linverse_julia_orbit(void);
extern int inverse_julia_per_image(void);
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
extern int orbit_2d_fp(void);
extern int orbit_2d(void);
extern int funny_glasses_call(int (*)(void));
extern int ifs(void);
extern int orbit_3d_fp(void);
extern int orbit_3d(void);
extern int icon_orbit_fp(double *, double *, double *);  /* dmf */
extern int latoo_orbit_fp(double *, double *, double *);  /* hb */
extern int setup_convert_to_screen(struct affine *);
extern int plotorbits2dsetup(void);
extern int plotorbits2dfloat(void);

/*  lsys -- C file prototypes */

extern LDBL  _fastcall get_number(char **);
extern int _fastcall is_pow2(int);
extern int l_system(void);
extern int l_load(void);

/*  miscfrac -- C file prototypes */

extern void froth_cleanup(void);

/*  miscovl -- C file prototypes */

extern void make_batch_file(void);
extern void edit_text_colors(void);
extern int select_video_mode(int);
extern void format_vid_table(int choice, char *buf);
extern void make_mig(unsigned int, unsigned int);
extern int get_precision_dbl(int);
extern int get_precision_bf(int);
extern int get_precision_mag_bf(void);
extern void parse_comments(char *value);
extern void init_comments(void);
extern void write_batch_parms(char *, int, int, int, int);
extern void expand_comments(char *, char *);

/*  miscres -- C file prototypes */

extern void restore_active_ovly(void);
extern void findpath(char *, char *);
extern void not_disk_message(void);
extern void convert_center_mag(double *, double *, LDBL *, double *, double *, double *);
extern void convert_corners(double, double, LDBL, double, double, double);
extern void convert_center_mag_bf(bf_t, bf_t, LDBL *, double *, double *, double *);
extern void convert_corners_bf(bf_t, bf_t, LDBL, double, double, double);
extern void update_save_name(char *);
extern int check_write_file(char *, char *);
extern int check_key(void);
extern void show_trig(char *);
extern int set_trig_array(int, const char *);
extern void set_trig_pointers(int);
extern int tab_display(void);
extern int ends_with_slash(char *);
extern int ifs_load(void);
extern int find_file_item(char *, char *, FILE **, int);
extern int file_gets(char *, int, FILE *);
extern void round_float_d(double *);
extern void fix_inversion(double *);
extern int unget_a_key(int);
extern void get_calculation_time(char *, long);

/*  mpmath_c -- C file prototypes */

extern struct MP *MPsub(struct MP, struct MP);
extern struct MP *MPsub086(struct MP, struct MP);
extern struct MP *MPsub386(struct MP, struct MP);
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
extern void setMPfunctions(void);
extern _CMPLX ComplexPower(_CMPLX, _CMPLX);
extern void SetupLogTable(void);
extern long logtablecalc(long);
extern long ExpFloat14(long);
extern int complex_newton_setup(void);
extern int complex_newton(void);
extern int complex_basin(void);
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

extern unsigned long new_random_number(void);
extern void lRandom(void);
extern void dRandom(void);
extern void mRandom(void);
extern void SetRandFnct(void);
extern void RandomSeed(void);
extern void lStkSRand(void);
extern void mStkSRand(void);
extern void dStkSRand(void);
extern void dStkAbs(void);
extern void mStkAbs(void);
extern void lStkAbs(void);
extern void dStkSqr(void);
extern void mStkSqr(void);
extern void lStkSqr(void);
extern void dStkAdd(void);
extern void mStkAdd(void);
extern void lStkAdd(void);
extern void dStkSub(void);
extern void mStkSub(void);
extern void lStkSub(void);
extern void dStkConj(void);
extern void mStkConj(void);
extern void lStkConj(void);
extern void dStkZero(void);
extern void mStkZero(void);
extern void lStkZero(void);
extern void dStkOne(void);
extern void mStkOne(void);
extern void lStkOne(void);
extern void dStkReal(void);
extern void mStkReal(void);
extern void lStkReal(void);
extern void dStkImag(void);
extern void mStkImag(void);
extern void lStkImag(void);
extern void dStkNeg(void);
extern void mStkNeg(void);
extern void lStkNeg(void);
extern void dStkMul(void);
extern void mStkMul(void);
extern void lStkMul(void);
extern void dStkDiv(void);
extern void mStkDiv(void);
extern void lStkDiv(void);
extern void StkSto(void);
extern void StkLod(void);
extern void dStkMod(void);
extern void mStkMod(void);
extern void lStkMod(void);
extern void StkClr(void);
extern void dStkFlip(void);
extern void mStkFlip(void);
extern void lStkFlip(void);
extern void dStkSin(void);
extern void mStkSin(void);
extern void lStkSin(void);
extern void dStkTan(void);
extern void mStkTan(void);
extern void lStkTan(void);
extern void dStkTanh(void);
extern void mStkTanh(void);
extern void lStkTanh(void);
extern void dStkCoTan(void);
extern void mStkCoTan(void);
extern void lStkCoTan(void);
extern void dStkCoTanh(void);
extern void mStkCoTanh(void);
extern void lStkCoTanh(void);
extern void dStkRecip(void);
extern void mStkRecip(void);
extern void lStkRecip(void);
extern void StkIdent(void);
extern void dStkSinh(void);
extern void mStkSinh(void);
extern void lStkSinh(void);
extern void dStkCos(void);
extern void mStkCos(void);
extern void lStkCos(void);
extern void dStkCosXX(void);
extern void mStkCosXX(void);
extern void lStkCosXX(void);
extern void dStkCosh(void);
extern void mStkCosh(void);
extern void lStkCosh(void);
extern void dStkLT(void);
extern void mStkLT(void);
extern void lStkLT(void);
extern void dStkGT(void);
extern void mStkGT(void);
extern void lStkGT(void);
extern void dStkLTE(void);
extern void mStkLTE(void);
extern void lStkLTE(void);
extern void dStkGTE(void);
extern void mStkGTE(void);
extern void lStkGTE(void);
extern void dStkEQ(void);
extern void mStkEQ(void);
extern void lStkEQ(void);
extern void dStkNE(void);
extern void mStkNE(void);
extern void lStkNE(void);
extern void dStkOR(void);
extern void mStkOR(void);
extern void lStkOR(void);
extern void dStkAND(void);
extern void mStkAND(void);
extern void lStkAND(void);
extern void dStkLog(void);
extern void mStkLog(void);
extern void lStkLog(void);
extern void FPUcplxexp(_CMPLX *, _CMPLX *);
extern void dStkExp(void);
extern void mStkExp(void);
extern void lStkExp(void);
extern void dStkPwr(void);
extern void mStkPwr(void);
extern void lStkPwr(void);
extern void dStkASin(void);
extern void mStkASin(void);
extern void lStkASin(void);
extern void dStkASinh(void);
extern void mStkASinh(void);
extern void lStkASinh(void);
extern void dStkACos(void);
extern void mStkACos(void);
extern void lStkACos(void);
extern void dStkACosh(void);
extern void mStkACosh(void);
extern void lStkACosh(void);
extern void dStkATan(void);
extern void mStkATan(void);
extern void lStkATan(void);
extern void dStkATanh(void);
extern void mStkATanh(void);
extern void lStkATanh(void);
extern void dStkCAbs(void);
extern void mStkCAbs(void);
extern void lStkCAbs(void);
extern void dStkSqrt(void);
extern void mStkSqrt(void);
extern void lStkSqrt(void);
extern void dStkFloor(void);
extern void mStkFloor(void);
extern void lStkFloor(void);
extern void dStkCeil(void);
extern void mStkCeil(void);
extern void lStkCeil(void);
extern void dStkTrunc(void);
extern void mStkTrunc(void);
extern void lStkTrunc(void);
extern void dStkRound(void);
extern void mStkRound(void);
extern void lStkRound(void);
extern void EndInit(void);
extern struct ConstArg *is_constant(char *, int);
extern void not_a_function(void);
extern void function_not_found(void);
extern int whichfn(char *, int);
extern int CvtStk(void);
extern int fFormula(void);
#if !defined(XFRACT)
typedef void t_function(void);
extern t_function *is_function(char *, int);
#endif
extern void RecSortPrec(void);
extern int Formula(void);
extern int BadFormula(void);
extern int form_per_pixel(void);
extern int frm_get_param_stuff (char *);
extern int RunForm(char *, int);
extern int formula_setup_fp(void);
extern int formula_setup_int(void);
extern void init_misc(void);
extern void free_work_area(void);
extern int fill_if_group(int endif_index, JUMP_PTRS_ST *jump_data);

/*  plot3d -- C file prototypes */

extern void cdecl draw_line(int, int, int, int, int);
extern void _fastcall plot_3d_superimpose_16(int, int, int);
extern void _fastcall plot_3d_superimpose_256(int, int, int);
extern void _fastcall plot_ifs_3d_superimpose_256(int, int, int);
extern void _fastcall plot_3d_alternate(int, int, int);
extern void plot_setup(void);

/*  prompts1 -- C file prototypes */

extern int full_screen_prompt(char *hdg, int numprompts, char **prompts,
	struct full_screen_values *values, int fkeymask, char *extrainfo);
extern long get_file_entry(int, char *, char *, char *, char *);
extern int get_fractal_type(void);
extern int get_fractal_parameters(int);
extern int get_fractal_3d_parameters(void);
extern int get_3d_parameters(void);
extern int prompt_value_string(char *buf, struct full_screen_values *val);
extern void set_bail_out_formula(enum bailouts);
extern int find_extra_parameter(int);
extern void load_parameters(int g_fractal_type);
extern int check_orbit_name(char *);
extern int scan_entries(FILE *infile, struct entryinfo *choices, char *itemname);

/*  prompts2 -- C file prototypes */

extern int get_toggles(void);
extern int get_toggles2(void);
extern int passes_options(void);
extern int get_view_params(void);
extern int get_starfield_params(void);
extern int get_commands(void);
extern void goodbye(void);
extern int is_a_directory(char *s);
extern int get_a_filename(char *, char *, char *);
extern int split_path(char *file_template, char *drive, char *dir, char *fname, char *ext);
extern int make_path(char *file_template, char *drive, char *dir, char *fname, char *ext);
extern int fr_find_first(char *path);
extern int fr_find_next(void);
extern void shell_sort(void *, int n, unsigned, int (__cdecl *fct)(VOIDPTR, VOIDPTR));
extern void fix_dir_name(char *dirname);
extern int merge_path_names(char *, char *, int);
extern int get_browse_parameters(void);
extern int get_command_string(void);
extern int get_random_dot_stereogram_parameters(void);
extern int starfield(void);
extern int get_a_number(double *, double *);
extern int lccompare(VOIDPTR, VOIDPTR);
extern int dir_remove(char *, char *);
extern FILE *dir_fopen(const char *, const char *, const char *);
extern void extract_filename(char *, char *);
extern char *has_extension(char *source);
extern int integer_unsupported(void);

/*  realdos -- C file prototypes */

extern int show_vid_length(void);
extern int stop_message(int, char *);
extern void blank_rows(int, int, int);
extern int text_temp_message(char *);
extern int full_screen_choice(int options, char *hdg, char *hdg2,
							 char *instr, int numchoices, char **choices, int *attributes,
							 int boxwidth, int boxdepth, int colwidth, int current,
							 void (*formatitem)(int, char *), char *speedstring,
							 int (*speedprompt)(int, int, int, char *, int),
							 int (*checkkey)(int, int));
extern int full_screen_choice_help(int help_mode, int options, char *hdg, char *hdg2,
							 char *instr, int numchoices, char **choices, int *attributes,
							 int boxwidth, int boxdepth, int colwidth, int current,
							 void (*formatitem)(int, char *), char *speedstring,
							 int (*speedprompt)(int, int, int, char *, int),
							 int (*checkkey)(int, int));
#if !defined(WINFRACT)
extern int show_temp_message(char *);
extern void clear_temp_message(void);
extern void help_title(void);
extern int put_string_center(int, int, int, int, char *);
#ifndef XFRACT /* Unix should have this in string.h */
extern int strncasecmp(char *, char *, int);
#endif
extern int main_menu(int);
extern int input_field(int, int, char *, int, int, int, int (*)(int));
extern int field_prompt(char *, char *, char *, int, int (*)(int));
extern int thinking(int, char *);
extern void load_fractint_config(void);
extern int check_video_mode_key(int, int);
extern int check_vidmode_keyname(char *);
extern void video_mode_key_name(int, char *);
extern void free_temp_message(void);
#endif
extern void load_video_table(int);
extern void bad_fractint_cfg_msg(void);

/*  rotate -- C file prototypes */

extern void rotate(int);
extern void save_palette(void);
extern int load_palette(void);

/*  slideshw -- C file prototypes */

extern int slide_show(void);
extern int start_slide_show(void);
extern void stop_slide_show(void);
extern void record_show(int);

/*  stereo -- C file prototypes */

extern int auto_stereo(void);
extern int out_line_stereo(BYTE *, int);

/*  targa -- C file prototypes */

extern void tga_write(int, int, int);
extern int tga_read(int, int);
extern void tga_end(void);
extern void tga_start(void);
extern void tga_reopen(void);

/*  testpt -- C file prototypes */

extern int test_start(void);
extern void test_end(void);
extern int test_per_pixel(double, double, double, double, long, int);

/*  tga_view -- C file prototypes */

extern int tga_view(void);
extern int out_line_16(BYTE*, int);

/*  yourvid -- C file prototypes */

//extern int startvideo(void);
//extern int endvideo(void);
//extern void writevideo(int, int, int);
//extern int readvideo(int, int);
//extern int readvideopalette(void);
//extern int writevideopalette(void);
#ifdef XFRACT
//extern void readvideoline(int, int, int, BYTE *);
//extern void writevideoline(int, int, int, BYTE *);
#endif

/*  zoom -- C file prototypes */

extern void zoom_box_draw(int);
extern void zoom_box_move(double, double);
extern void zoom_box_resize(int);
extern void zoom_box_change_i(int, int);
extern void zoom_box_out(void);
extern void aspect_ratio_crop(float, float);
extern int init_pan_or_recalc(int);
extern void _fastcall draw_lines(struct coords, struct coords, int, int);
extern void _fastcall add_box(struct coords);
extern void clear_box(void);
extern void display_box(void);

/*  fractalb.c -- C file prototypes */

extern _CMPLX complex_bn_to_float(_BNCMPLX *);
extern _CMPLX complex_bf_to_float(_BFCMPLX *);
extern void compare_values(char *, LDBL, bn_t);
extern void compare_values_bf(char *, LDBL, bf_t);
extern void show_var_bf(char *s, bf_t n);
extern void show_two_bf(char *, bf_t, char *, bf_t, int);
extern void corners_bf_to_float(void);
extern void show_corners_dbl(char *);
extern int mandelbrot_setup_bn(void);
extern int mandelbrot_per_pixel_bn(void);
extern int julia_per_pixel_bn(void);
extern int julia_orbit_bn(void);
extern int julia_z_power_orbit_bn(void);
extern _BNCMPLX *complex_log_bn(_BNCMPLX *t, _BNCMPLX *s);
extern _BNCMPLX *complex_multiply_bn(_BNCMPLX *t, _BNCMPLX *x, _BNCMPLX *y);
extern _BNCMPLX *complex_power_bn(_BNCMPLX *t, _BNCMPLX *xx, _BNCMPLX *yy);
extern int mandelbrot_setup_bf(void);
extern int mandelbrot_per_pixel_bf(void);
extern int julia_per_pixel_bf(void);
extern int julia_orbit_bf(void);
extern int julia_z_power_orbit_bf(void);
extern _BFCMPLX *complex_log_bf(_BFCMPLX *t, _BFCMPLX *s);
extern _BFCMPLX *cplxmul_bf(_BFCMPLX *t, _BFCMPLX *x, _BFCMPLX *y);
extern _BFCMPLX *ComplexPower_bf(_BFCMPLX *t, _BFCMPLX *xx, _BFCMPLX *yy);

/*  memory -- C file prototypes */
/* TODO: Get rid of this and use regular memory routines;
** see about creating standard disk memory routines for disk video
*/
extern int MemoryType(U16 handle);
extern void InitMemory(void);
extern void ExitCheck(void);
extern U16 MemoryAlloc(U16 size, long count, int stored_at);
extern void MemoryRelease(U16 handle);
extern int MoveToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern int MoveFromMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern int SetMemory(int value, U16 size, long count, long offset, U16 handle);

/*  soi -- C file prototypes */
extern void soi(void);
extern void soi_long_double(void);

/*
 *  uclock -- C file prototypes 
 *  The  uclock_t typedef placed here because uclock.h
 *  prototype is for DOS version only.
 */
typedef unsigned long uclock_t;

extern uclock_t usec_clock(void);
extern void restart_uclock(void);
extern void wait_until(int index, uclock_t wait_time);

extern void init_failure(const char *message);
extern int expand_dirname(char *dirname, char *drive);
extern int abort_message(char *file, unsigned int line, int flags, char *msg);
#define ABORT(flags_, msg_) abort_message(__FILE__, __LINE__, flags_, msg_)

#ifndef DEBUG
/*#define DEBUG */
#endif

#endif
