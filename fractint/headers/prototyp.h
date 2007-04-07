#ifndef PROTOTYP_H
#define PROTOTYP_H

/* includes needed to define the prototypes */

#include "mpmath.h"
#include "big.h"
#include "fractint.h"
#include "helpcom.h"
#include "externs.h"

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

extern long cdecl calcmandasm(void);

/*  calmanfp -- assembler file prototypes */

extern void cdecl calcmandfpasmstart(void);
/* extern long  cdecl g_calculate_mandelbrot_asm_fp(void); */
extern long  cdecl calcmandfpasm_287(void);
extern long  cdecl calcmandfpasm_87(void);

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

extern int asmlMODbailout(void);
extern int asmlREALbailout(void);
extern int asmlIMAGbailout(void);
extern int asmlORbailout(void);
extern int asmlANDbailout(void);
extern int asmlMANHbailout(void);
extern int asmlMANRbailout(void);
extern int asm386lMODbailout(void);
extern int asm386lREALbailout(void);
extern int asm386lIMAGbailout(void);
extern int asm386lORbailout(void);
extern int asm386lANDbailout(void);
extern int asm386lMANHbailout(void);
extern int asm386lMANRbailout(void);
extern int FManOWarfpFractal(void);
extern int FJuliafpFractal(void);
extern int FBarnsley1FPFractal(void);
extern int FBarnsley2FPFractal(void);
extern int FLambdaFPFractal(void);
extern int asmfpMODbailout(void);
extern int asmfpREALbailout(void);
extern int asmfpIMAGbailout(void);
extern int asmfpORbailout(void);
extern int asmfpANDbailout(void);
extern int asmfpMANHbailout(void);
extern int asmfpMANRbailout(void);

/* history -- C file prototypes */

void _fastcall restore_history_info(void);
void _fastcall save_history_info(void);
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

/* CAE removed static functions from header 28 Jan 95  */

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
extern int lya_setup(void);
extern int cellular(void);
extern int cellular_setup(void);
extern int froth_calc(void);
extern int froth_per_pixel(void);
extern int froth_per_orbit(void);
extern int froth_setup(void);
extern int logtable_in_extra_ok(void);
extern int find_alternate_math(int, int);

/*  cmdfiles -- C file prototypes */

extern int cmdfiles(int, char **);
extern int load_commands(FILE *);
extern void set_3d_defaults(void);
extern int get_curarg_len(char *curarg);
extern int get_max_curarg_len(char *floatvalstr[], int totparm);
extern int init_msg(const char *, char *, int);
extern int process_command(char *curarg, int mode);
extern int getpower10(LDBL x);
extern void dopause(int);

/*  decoder -- C file prototypes */

extern short decoder(short);
extern void set_byte_buff(BYTE *ptr);

/*  diskvid -- C file prototypes */

extern int pot_startdisk(void);
extern int targa_startdisk(FILE *, int);
extern void enddisk(void);
extern int readdisk(int, int);
extern void writedisk(int, int, int);
extern void targa_readdisk(unsigned int, unsigned int, BYTE *, BYTE *, BYTE *);
extern void targa_writedisk(unsigned int, unsigned int, BYTE, BYTE, BYTE);
extern void dvid_status(int, char *);
extern int  _fastcall common_startdisk(long, long, int);
extern int FromMemDisk(long, int, void *);
extern int ToMemDisk(long, int, void *);

/*  editpal -- C file prototypes */

extern void EditPalette(void);
void putrow(int x, int y, int width, char *buff);
void getrow(int x, int y, int width, char *buff);
/* void hline(int x, int y, int width, int color); */
int Cursor_WaitKey(void);
void Cursor_CheckBlink(void);
#ifdef XFRACT
void Cursor_StartMouseTracking(void);
void Cursor_EndMouseTracking(void);
#endif
void clip_putcolor(int x, int y, int color);
int clip_getcolor(int x, int y);
BOOLEAN Cursor_Construct (void);
void Cursor_Destroy (void);
void Cursor_SetPos (int x, int y);
void Cursor_Move (int xoff, int yoff);
int Cursor_GetX (void);
int Cursor_GetY (void);
void Cursor_Hide (void);
void Cursor_Show (void);
extern void displayc(int, int, int, int, int);

/*  encoder -- C file prototypes */

extern int savetodisk(char *);
extern int encoder(void);
extern int _fastcall new_to_old(int new_fractype);

/*  evolve -- C file prototypes */

extern  void param_history(int);
extern  int get_variations(void);
extern  int get_evolve_Parms(void);
extern  void set_current_params(void);
extern  void fiddleparms(GENEBASE gene[], int ecount);
extern  void set_evolve_ranges(void);
extern  void set_mutation_level(int);
extern  void drawparmbox(int);
extern  void spiralmap(int);
extern  int unspiralmap(void);
extern  int explore_check(void);
extern  void SetupParamBox(void);
extern  void ReleaseParamBox(void);

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
extern int VLfpFractal(void);
extern int EscherfpFractal(void);
extern int julia_per_pixel(void);
extern int long_richard8_per_pixel(void);
extern int long_mandel_per_pixel(void);
extern int julia_per_pixel_fp(void);
extern int marks_mandelpwr_per_pixel(void);
extern int mandel_per_pixel(void);
extern int marksmandel_per_pixel(void);
extern int marksmandelfp_per_pixel(void);
extern int marks_mandelpwrfp_per_pixel(void);
extern int mandelfp_per_pixel(void);
extern int juliafp_per_pixel(void);
extern int MPCjulia_per_pixel(void);
extern int otherrichard8fp_per_pixel(void);
extern int othermandelfp_per_pixel(void);
extern int otherjuliafp_per_pixel(void);
extern int MarksCplxMandperp(void);
extern int lambda_trig_or_trig_orbit(void);
extern int lambda_trig_or_trig_orbit_fp(void);
extern int julia_trig_or_trig_orbit(void);
extern int julia_trig_or_trig_orbit_fp(void);
extern int halley_orbit_fp(void);
extern int Halley_per_pixel(void);
extern int halley_orbit_mpc(void);
extern int MPCHalley_per_pixel(void);
extern int dynamic_orbit_fp(double *, double *, double*);
extern int mandel_cloud_orbit_fp(double *, double *, double*);
extern int dynamic_2d_fp(void);
extern int QuaternionFPFractal(void);
extern int quaternionfp_per_pixel(void);
extern int quaternionjulfp_per_pixel(void);
extern int phoenix_orbit(void);
extern int phoenix_orbit_fp(void);
extern int long_phoenix_per_pixel(void);
extern int phoenix_per_pixel(void);
extern int long_mandphoenix_per_pixel(void);
extern int mandphoenix_per_pixel(void);
extern int HyperComplexFPFractal(void);
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
extern int long_phoenix_per_pixel(void);
extern int phoenix_per_pixel(void);
extern int long_mandphoenix_per_pixel(void);
extern int mandphoenix_per_pixel(void);
extern void set_pixel_calc_functions(void);
extern int MandelbrotMix4fp_per_pixel(void);
extern int MandelbrotMix4fpFractal(void);
extern int MandelbrotMix4Setup(void);

/*  fractint -- C file prototypes */

extern int main(int argc, char **argv);
extern int elapsed_time(int);

/*  framain2 -- C file prototypes */

extern int big_while_loop(int *, char *, int);
extern int check_key(void);
extern int cmp_line(BYTE *, int);
extern int key_count(int);
extern int main_menu_switch(int *, int *, int *, char *, int);
extern int pot_line(BYTE *, int);
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

extern void clear_zoombox(void);
extern void flip_image(int kbdchar);
#ifndef WINFRACT
extern void reset_zoom_corners(void);
#endif
extern void setup287code(void);

/*  frasetup -- C file prototypes */

extern int VLSetup(void);
extern int MandelSetup(void);
extern int MandelfpSetup(void);
extern int JuliaSetup(void);
extern int NewtonSetup(void);
extern int StandaloneSetup(void);
extern int UnitySetup(void);
extern int JuliafpSetup(void);
extern int MandellongSetup(void);
extern int JulialongSetup(void);
extern int TrigPlusSqrlongSetup(void);
extern int TrigPlusSqrfpSetup(void);
extern int TrigPlusTriglongSetup(void);
extern int TrigPlusTrigfpSetup(void);
extern int FnPlusFnSym(void);
extern int ZXTrigPlusZSetup(void);
extern int LambdaTrigSetup(void);
extern int JuliafnPlusZsqrdSetup(void);
extern int SqrTrigSetup(void);
extern int FnXFnSetup(void);
extern int MandelTrigSetup(void);
extern int MarksJuliaSetup(void);
extern int MarksJuliafpSetup(void);
extern int SierpinskiSetup(void);
extern int SierpinskiFPSetup(void);
extern int StandardSetup(void);
extern int LambdaTrigOrTrigSetup(void);
extern int JuliaTrigOrTrigSetup(void);
extern int ManlamTrigOrTrigSetup(void);
extern int MandelTrigOrTrigSetup(void);
extern int HalleySetup(void);
extern int dynamic_2d_setup_fp(void);
extern int PhoenixSetup(void);
extern int MandPhoenixSetup(void);
extern int PhoenixCplxSetup(void);
extern int MandPhoenixCplxSetup(void);

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
extern int JulibrotfpSetup(void);
extern int julibrot_per_pixel(void);
extern int julibrot_per_pixel_fp(void);
extern int zline(long, long);
extern int zlinefp(double, double);
extern int std_4d_fractal(void);
extern int std_4d_fractal_fp(void);

/*  jiim -- C file prototypes */

extern void Jiim(int);
extern LCMPLX PopLong         (void);
extern _CMPLX PopFloat        (void);
extern LCMPLX DeQueueLong     (void);
extern _CMPLX DeQueueFloat    (void);
extern LCMPLX ComplexSqrtLong (long,  long);
extern _CMPLX ComplexSqrtFloat(double, double);
extern int    Init_Queue      (unsigned long);
extern void   Free_Queue      (void);
extern void   ClearQueue      (void);
extern int    QueueEmpty      (void);
extern int    QueueFull       (void);
extern int    QueueFullAlmost (void);
extern int    PushLong        (long,  long);
extern int    PushFloat       (float,  float);
extern int    EnQueueLong     (long,  long);
extern int    EnQueueFloat    (float,  float);

/*  line3d -- C file prototypes */

extern int line3d(BYTE *, unsigned int);
extern int _fastcall targa_color(int, int, int);
extern int startdisk1(char *, FILE *, int);

/*  loadfdos -- C file prototypes */
#ifndef WINFRACT
extern int get_video_mode(struct fractal_info *, struct ext_blk_formula_info *);
#endif
/*  loadfile -- C file prototypes */

extern int read_overlay(void);
extern void set_if_old_bif(void);
extern void set_function_parm_defaults(void);
extern int fgetwindow(void);
extern void backwards_v18(void);
extern void backwards_v19(void);
extern void backwards_v20(void);
extern int check_back(void);

/*  loadmap -- C file prototypes */

//extern void SetTgaColors(void);
extern int ValidateLuts(char *);
extern int SetColorPaletteName(char *);

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
extern int  setup_convert_to_screen(struct affine *);
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
extern int getprecdbl(int);
extern int getprecbf(int);
extern int getprecbf_mag(void);
extern void parse_comments(char *value);
extern void init_comments(void);
extern void write_batch_parms(char *, int, int, int, int);
extern void expand_comments(char *, char *);

/*  miscres -- C file prototypes */

extern void restore_active_ovly(void);
extern void findpath(char *, char *);
extern void notdiskmsg(void);
extern void cvtcentermag(double *, double *, LDBL *, double *, double *, double *);
extern void cvtcorners(double, double, LDBL, double, double, double);
extern void cvtcentermagbf(bf_t, bf_t, LDBL *, double *, double *, double *);
extern void cvtcornersbf(bf_t, bf_t, LDBL, double, double, double);
extern void updatesavename(char *);
extern int check_writefile(char *, char *);
extern int check_key(void);
extern void showtrig(char *);
extern int set_trig_array(int, const char *);
extern void set_trig_pointers(int);
extern int tab_display(void);
extern int endswithslash(char *);
extern int ifsload(void);
extern int find_file_item(char *, char *, FILE **, int);
extern int file_gets(char *, int, FILE *);
extern void roundfloatd(double *);
extern void fix_inversion(double *);
extern int ungetakey(int);
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
extern int ComplexNewtonSetup(void);
extern int ComplexNewton(void);
extern int ComplexBasin(void);
extern int GausianNumber(int, int);
extern void Arcsinz(_CMPLX z, _CMPLX *rz);
extern void Arccosz(_CMPLX z, _CMPLX *rz);
extern void Arcsinhz(_CMPLX z, _CMPLX *rz);
extern void Arccoshz(_CMPLX z, _CMPLX *rz);
extern void Arctanhz(_CMPLX z, _CMPLX *rz);
extern void Arctanz(_CMPLX z, _CMPLX *rz);

/*  msccos -- C file prototypes */

extern double _cos(double);

/*  parser -- C file prototypes */

struct fls { /* function, load, store pointers  CAE fp */
   void (*function)(void);
   union Arg *operand;
};

extern unsigned int SkipWhiteSpace(char *);
extern unsigned long NewRandNum(void);
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
extern void (*mtrig0)(void);
extern void (*mtrig1)(void);
extern void (*mtrig2)(void);
extern void (*mtrig3)(void);
extern void EndInit(void);
extern struct ConstArg *isconst(char *, int);
extern void NotAFnct(void);
extern void FnctNotFound(void);
extern int whichfn(char *, int);
extern int CvtStk(void);
extern int fFormula(void);
#ifndef XFRACT
extern void (*isfunct(char *, int))(void);
#else
extern void (*isfunct(char *, int))();
#endif
extern void RecSortPrec(void);
extern int Formula(void);
extern int BadFormula(void);
extern int form_per_pixel(void);
extern int frm_get_param_stuff (char *);
extern int RunForm(char *, int);
extern int fpFormulaSetup(void);
extern int intFormulaSetup(void);
extern void init_misc(void);
extern void free_workarea(void);
extern int fill_if_group(int endif_index, JUMP_PTRS_ST *jump_data);

/*  plot3d -- C file prototypes */

extern void cdecl draw_line(int, int, int, int, int);
extern void _fastcall plot3dsuperimpose16(int, int, int);
extern void _fastcall plot3dsuperimpose256(int, int, int);
extern void _fastcall plotIFS3dsuperimpose256(int, int, int);
extern void _fastcall plot3dalternate(int, int, int);
extern void plot_setup(void);

/*  printer -- C file prototypes */

extern void Print_Screen(void);

/*  prompts1 -- C file prototypes */

extern int fullscreen_prompt(char far*, int, char **, struct fullscreenvalues *, int, char *);
extern long get_file_entry(int, char *, char *, char *, char *);
extern int get_fracttype(void);
extern int get_fract_params(int);
extern int get_fract3d_params(void);
extern int get_3d_params(void);
extern int prompt_valuestring(char *buf, struct fullscreenvalues *val);
extern void setbailoutformula(enum bailouts);
extern int find_extra_param(int);
extern void load_params(int fractype);
extern int check_orbit_name(char *);
struct entryinfo;
extern int scan_entries(FILE *infile, struct entryinfo *choices, char *itemname);

/*  prompts2 -- C file prototypes */

extern int get_toggles(void);
extern int get_toggles2(void);
extern int passes_options(void);
extern int get_view_params(void);
extern int get_starfield_params(void);
extern int get_commands(void);
extern void goodbye(void);
extern int isadirectory(char *s);
extern int getafilename(char *, char *, char *);
extern int splitpath(char *file_template, char *drive, char *dir, char *fname, char *ext);
extern int makepath(char *file_template, char *drive, char *dir, char *fname, char *ext);
extern int fr_findfirst(char *path);
extern int fr_findnext(void);
extern void shell_sort(void *, int n, unsigned, int (__cdecl *fct)(VOIDPTR, VOIDPTR));
extern void fix_dirname(char *dirname);
extern int merge_pathnames(char *, char *, int);
extern int get_browse_params(void);
extern int get_cmd_string(void);
extern int get_rds_params(void);
extern int starfield(void);
extern int get_a_number(double *, double *);
extern int lccompare(VOIDPTR, VOIDPTR);
extern int dir_remove(char *, char *);
extern FILE *dir_fopen(char *, char *, char *);
extern void extract_filename(char *, char *);
extern char *has_ext(char *source);
extern int integer_unsupported(void);

/*  realdos -- C file prototypes */

extern int showvidlength(void);
extern int stopmsg(int, char *);
extern void blankrows(int, int, int);
extern int texttempmsg(char *);
extern int fullscreen_choice(int options, char *hdg, char *hdg2,
							 char *instr, int numchoices, char **choices, int *attributes,
							 int boxwidth, int boxdepth, int colwidth, int current,
							 void (*formatitem)(int, char *), char *speedstring,
							 int (*speedprompt)(int, int, int, char *, int),
							 int (*checkkey)(int, int));
#if !defined(WINFRACT)
extern int showtempmsg(char *);
extern void cleartempmsg(void);
extern void helptitle(void);
extern int putstringcenter(int, int, int, int, char *);
#ifndef XFRACT /* Unix should have this in string.h */
extern int strncasecmp(char *, char *, int);
#endif
extern int main_menu(int);
extern int input_field(int, int, char *, int, int, int, int (*)(int));
extern int field_prompt(char *, char *, char *, int, int (*)(int));
extern int thinking(int, char *);
extern int savegraphics(void);
extern int restoregraphics(void);
extern void discardgraphics(void);
extern void load_fractint_config(void);
extern int check_vidmode_key(int, int);
extern int check_vidmode_keyname(char *);
extern void vidmode_keyname(int, char *);
extern void freetempmsg(void);
#endif
extern void load_videotable(int);
extern void bad_fractint_cfg_msg(void);

/*  rotate -- C file prototypes */

extern void rotate(int);
extern void save_palette(void);
extern int load_palette(void);

/*  slideshw -- C file prototypes */

extern int slideshw(void);
extern int startslideshow(void);
extern void stopslideshow(void);
extern void recordshw(int);

/*  stereo -- C file prototypes */

extern int do_AutoStereo(void);
extern int outline_stereo(BYTE *, int);

/*  targa -- C file prototypes */

extern void WriteTGA(int, int, int);
extern int ReadTGA(int, int);
extern void EndTGA(void);
extern void StartTGA(void);
extern void ReopenTGA(void);

/*  testpt -- C file prototypes */

extern int teststart(void);
extern void testend(void);
extern int testpt(double, double, double, double, long, int);

/*  tgaview -- C file prototypes */

extern int tgaview(void);
extern int outlin16(BYTE*, int);

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

extern void drawbox(int);
extern void moveboxf(double, double);
extern void resizebox(int);
extern void chgboxi(int, int);
extern void zoomout(void);
extern void aspectratio_crop(float, float);
extern int init_pan_or_recalc(int);
extern void _fastcall drawlines(struct coords, struct coords, int, int);
extern void _fastcall addbox(struct coords);
extern void clearbox(void);
extern void dispbox(void);

/*  fractalb.c -- C file prototypes */

extern _CMPLX complex_bn_to_float(_BNCMPLX *);
extern _CMPLX complex_bf_to_float(_BFCMPLX *);
extern void comparevalues(char *, LDBL, bn_t);
extern void comparevaluesbf(char *, LDBL, bf_t);
extern void show_var_bf(char *s, bf_t n);
extern void show_two_bf(char *, bf_t, char *, bf_t, int);
extern void corners_bf_to_float(void);
extern void showcornersdbl(char *);
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
extern void DisplayHandle (U16 handle);
extern int MemoryType (U16 handle);
extern void InitMemory (void);
extern void ExitCheck (void);
extern U16 MemoryAlloc(U16 size, long count, int stored_at);
extern void MemoryRelease(U16 handle);
extern int MoveToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern int MoveFromMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern int SetMemory(int value, U16 size, long count, long offset, U16 handle);

/*  soi -- C file prototypes */

extern void soi (void);
extern void soi_ldbl (void);

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
extern int abortmsg(char *file, unsigned int line, int flags, char *msg);
#define ABORT(flags_, msg_) abortmsg(__FILE__, __LINE__, flags_, msg_)

#ifndef DEBUG
/*#define DEBUG */
#endif

#endif
