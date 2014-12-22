#ifndef PROTOTYP_H
#define PROTOTYP_H
// includes needed to define the prototypes
#include "mpmath.h"
#include "big.h"
#include "fractint.h"
#include "externs.h"
// maintain the common prototypes in this file

#ifdef XFRACT
#include "unixprot.h"
#else
#ifdef _WIN32
#include "winprot.h"
#endif
#endif
extern long multiply(long x, long y, int n);
extern long divide(long x, long y, int n);
extern void spindac(int dir, int inc);
extern void put_line(int row, int startcol, int stopcol, BYTE *pixels);
extern void get_line(int row, int startcol, int stopcol, BYTE *pixels);
extern void find_special_colors();
extern int getakeynohelp();
extern long readticker();
extern int get_sound_params();
extern void setnullvideo();
// calcmand -- assembler file prototypes
extern long calcmandasm();
// calmanfp -- assembler file prototypes
extern void calcmandfpasmstart();
extern long calcmandfpasm();
// fpu087 -- assembler file prototypes
extern void FPUcplxmul(DComplex *, DComplex *, DComplex *);
extern void FPUcplxdiv(DComplex *, DComplex *, DComplex *);
extern void FPUsincos(double *, double *, double *);
extern void FPUsinhcosh(double *, double *, double *);
extern void FPUcplxlog(DComplex *, DComplex *);
extern void SinCos086(long, long *, long *);
extern void SinhCosh086(long, long *, long *);
extern long r16Mul(long, long);
extern long RegFloat2Fg(long, int);
extern long Exp086(long);
extern unsigned long ExpFudged(long, int);
extern long RegDivFloat(long, long);
extern long LogFudged(unsigned long, int);
extern long LogFloat14(unsigned long);
extern long RegFg2Float(long, int);
extern long RegSftFloat(long, int);
// fracsuba -- assembler file prototypes
extern int asmlMODbailout();
extern int asmlREALbailout();
extern int asmlIMAGbailout();
extern int asmlORbailout();
extern int asmlANDbailout();
extern int asmlMANHbailout();
extern int asmlMANRbailout();
extern int asm386lMODbailout();
extern int asm386lREALbailout();
extern int asm386lIMAGbailout();
extern int asm386lORbailout();
extern int asm386lANDbailout();
extern int asm386lMANHbailout();
extern int asm386lMANRbailout();
extern int asmfpMODbailout();
extern int asmfpREALbailout();
extern int asmfpIMAGbailout();
extern int asmfpORbailout();
extern int asmfpANDbailout();
extern int asmfpMANHbailout();
extern int asmfpMANRbailout();
// mpmath_a -- assembler file prototypes
extern MP * MPmul086(MP, MP);
extern MP * MPdiv086(MP, MP);
extern MP * MPadd086(MP, MP);
extern int         MPcmp086(MP, MP);
extern MP * d2MP086(double);
extern double    * MP2d086(MP);
extern MP * fg2MP086(long, int);
extern MP * MPmul386(MP, MP);
extern MP * MPdiv386(MP, MP);
extern MP * MPadd386(MP, MP);
extern int         MPcmp386(MP, MP);
extern MP * d2MP386(double);
extern double    * MP2d386(MP);
extern MP * fg2MP386(long, int);
extern double *    MP2d(MP);
extern int         MPcmp(MP, MP);
extern MP * MPmul(MP, MP);
extern MP * MPadd(MP, MP);
extern MP * MPdiv(MP, MP);
extern MP * d2MP(double);   // Convert double to type MP
extern MP * fg2MP(long, int);  // Convert fudged to type MP
// newton -- assembler file prototypes
extern int NewtonFractal2();
extern void invertz2(DComplex *);
// tplus_a -- assembler file prototypes
extern void WriteTPlusBankedPixel(int, int, unsigned long);
extern unsigned long ReadTPlusBankedPixel(int, int);
// 3d -- C file prototypes
extern void identity(MATRIX);
extern void mat_mul(MATRIX, MATRIX, MATRIX);
extern void scale(double, double, double, MATRIX);
extern void xrot(double, MATRIX);
extern void yrot(double, MATRIX);
extern void zrot(double, MATRIX);
extern void trans(double, double, double, MATRIX);
extern int cross_product(VECTOR, VECTOR, VECTOR);
extern bool normalize_vector(VECTOR);
extern int vmult(VECTOR, MATRIX, VECTOR);
extern void mult_vec(VECTOR);
extern int perspective(VECTOR);
extern int longvmultpersp(LVECTOR, LMATRIX, LVECTOR, LVECTOR, LVECTOR, int);
extern int longpersp(LVECTOR, LVECTOR, int);
extern int longvmult(LVECTOR, LMATRIX, LVECTOR, int);
// biginit -- C file prototypes
void free_bf_vars();
bn_t alloc_stack(size_t size);
int save_stack();
void restore_stack(int old_offset);
void init_bf_dec(int dec);
void init_bf_length(int bnl);
void init_big_pi();
// calcfrac -- C file prototypes
extern int calcfract();
extern int calcmand();
extern int calcmandfp();
extern int StandardFractal();
extern int test();
extern int plasma();
extern int diffusion();
extern int Bifurcation();
extern int BifurcLambda();
extern int BifurcSetTrigPi();
extern int LongBifurcSetTrigPi();
extern int BifurcAddTrigPi();
extern int LongBifurcAddTrigPi();
extern int BifurcMay();
extern bool BifurcMaySetup();
extern int LongBifurcMay();
extern int BifurcLambdaTrig();
extern int LongBifurcLambdaTrig();
extern int BifurcVerhulstTrig();
extern int LongBifurcVerhulstTrig();
extern int BifurcStewartTrig();
extern int LongBifurcStewartTrig();
extern int popcorn();
extern int lyapunov();
extern bool lya_setup();
extern int cellular();
extern bool CellularSetup();
extern int calcfroth();
extern int froth_per_pixel();
extern int froth_per_orbit();
extern bool froth_setup();
extern int logtable_in_extra_ok();
extern int find_alternate_math(fractal_type type, bf_math_type math);
// cmdfiles -- C file prototypes
extern int cmdfiles(int, char **);
extern int load_commands(FILE *);
extern void set_3d_defaults();
extern int get_curarg_len(const char *curarg);
extern int get_max_curarg_len(const char *floatvalstr[], int totparm);
extern int init_msg(const char *cmdstr, char *badfilename, cmd_file mode);
extern int cmdarg(char *curarg, cmd_file mode);
extern int getpower10(LDBL x);
extern void dopause(int);
// decoder -- C file prototypes
extern short decoder(short);
extern void set_byte_buff(BYTE *ptr);
// diskvid -- C file prototypes
extern int pot_startdisk();
extern int targa_startdisk(FILE *, int);
extern void enddisk();
extern int readdisk(int, int);
extern void writedisk(int, int, int);
extern void targa_readdisk(unsigned int, unsigned int, BYTE *, BYTE *, BYTE *);
extern void targa_writedisk(unsigned int, unsigned int, BYTE, BYTE, BYTE);
extern void dvid_status(int line, const char *msg);
extern int  common_startdisk(long, long, int);
extern int FromMemDisk(long, int, void *);
extern bool ToMemDisk(long, int, void *);
// editpal -- C file prototypes
extern void EditPalette();
extern void *mem_alloc(unsigned size);
void putrow(int x, int y, int width, char *buff);
void getrow(int x, int y, int width, char *buff);
void mem_init(void *block, unsigned size);
int Cursor_WaitKey();
void Cursor_CheckBlink();
void clip_putcolor(int x, int y, int color);
int clip_getcolor(int x, int y);
void Cursor_Construct();
void Cursor_Destroy();
void Cursor_SetPos(int x, int y);
void Cursor_Move(int xoff, int yoff);
int Cursor_GetX();
int Cursor_GetY();
void Cursor_Hide();
void Cursor_Show();
extern void displayc(int, int, int, int, int);
// encoder -- C file prototypes
extern int savetodisk(char *);
extern bool encoder();
extern int new_to_old(int new_fractype);
// evolve -- C file prototypes
extern  void initgene();
extern  void param_history(int);
extern  int get_variations();
extern  int get_evolve_Parms();
extern  void set_current_params();
extern  void fiddleparms(GENEBASE gene[], int ecount);
extern  void set_evolve_ranges();
extern  void set_mutation_level(int);
extern  void drawparmbox(int);
extern  void spiralmap(int);
extern  int unspiralmap();
extern  void SetupParamBox();
extern  void ReleaseParamBox();
// f16 -- C file prototypes
extern FILE *t16_open(char *, int *, int *, int *, U8 *);
extern int t16_getline(FILE *, int, U16 *);
// fracsubr -- C file prototypes
extern void free_grid_pointers();
extern void calcfracinit();
extern void adjust_corner();
extern int put_resume(int, ...);
extern int get_resume(int, ...);
extern int alloc_resume(int, int);
extern int start_resume();
extern void end_resume();
extern void sleepms(long);
extern void reset_clock();
extern void iplot_orbit(long, long, int);
extern void plot_orbit(double, double, int);
extern void scrub_orbit();
extern int add_worklist(int, int, int, int, int, int, int, int);
extern void tidy_worklist();
extern void get_julia_attractor(double, double);
extern int ssg_blocksize();
extern void symPIplot(int, int, int);
extern void symPIplot2J(int, int, int);
extern void symPIplot4J(int, int, int);
extern void symplot2(int, int, int);
extern void symplot2Y(int, int, int);
extern void symplot2J(int, int, int);
extern void symplot4(int, int, int);
extern void symplot2basin(int, int, int);
extern void symplot4basin(int, int, int);
extern void noplot(int, int, int);
extern void fractal_floattobf();
extern void adjust_cornerbf();
extern void set_grid_pointers();
extern void fill_dx_array();
extern void fill_lx_array();
extern bool snd_open();
extern void w_snd(int);
extern void snd_time_write();
extern void close_snd();
// fractalp -- C file prototypes
extern bool typehasparm(fractal_type type, int parm, char *buf);
extern bool paramnotused(int);
// fractals -- C file prototypes
extern void FloatPreCalcMagnet2();
extern void cpower(DComplex *, int, DComplex *);
extern int lcpower(LComplex *, int, LComplex *, int);
extern int lcomplex_mult(LComplex, LComplex, LComplex *, int);
extern int MPCNewtonFractal();
extern int Barnsley1Fractal();
extern int Barnsley1FPFractal();
extern int Barnsley2Fractal();
extern int Barnsley2FPFractal();
extern int JuliaFractal();
extern int JuliafpFractal();
extern int LambdaFPFractal();
extern int LambdaFractal();
extern int SierpinskiFractal();
extern int SierpinskiFPFractal();
extern int LambdaexponentFractal();
extern int LongLambdaexponentFractal();
extern int FloatTrigPlusExponentFractal();
extern int LongTrigPlusExponentFractal();
extern int MarksLambdaFractal();
extern int MarksLambdafpFractal();
extern int UnityFractal();
extern int UnityfpFractal();
extern int Mandel4Fractal();
extern int Mandel4fpFractal();
extern int floatZtozPluszpwrFractal();
extern int longZpowerFractal();
extern int longCmplxZpowerFractal();
extern int floatZpowerFractal();
extern int floatCmplxZpowerFractal();
extern int Barnsley3Fractal();
extern int Barnsley3FPFractal();
extern int TrigPlusZsquaredFractal();
extern int TrigPlusZsquaredfpFractal();
extern int Richard8fpFractal();
extern int Richard8Fractal();
extern int PopcornFractal();
extern int LPopcornFractal();
extern int PopcornFractal_Old();
extern int LPopcornFractal_Old();
extern int PopcornFractalFn();
extern int LPopcornFractalFn();
extern int MarksCplxMand();
extern int SpiderfpFractal();
extern int SpiderFractal();
extern int TetratefpFractal();
extern int ZXTrigPlusZFractal();
extern int ScottZXTrigPlusZFractal();
extern int SkinnerZXTrigSubZFractal();
extern int ZXTrigPlusZfpFractal();
extern int ScottZXTrigPlusZfpFractal();
extern int SkinnerZXTrigSubZfpFractal();
extern int Sqr1overTrigFractal();
extern int Sqr1overTrigfpFractal();
extern int TrigPlusTrigFractal();
extern int TrigPlusTrigfpFractal();
extern int ScottTrigPlusTrigFractal();
extern int ScottTrigPlusTrigfpFractal();
extern int SkinnerTrigSubTrigFractal();
extern int SkinnerTrigSubTrigfpFractal();
extern int TrigXTrigfpFractal();
extern int TrigXTrigFractal();
extern int TrigPlusSqrFractal();
extern int TrigPlusSqrfpFractal();
extern int ScottTrigPlusSqrFractal();
extern int ScottTrigPlusSqrfpFractal();
extern int SkinnerTrigSubSqrFractal();
extern int SkinnerTrigSubSqrfpFractal();
extern int TrigZsqrdfpFractal();
extern int TrigZsqrdFractal();
extern int SqrTrigFractal();
extern int SqrTrigfpFractal();
extern int Magnet1Fractal();
extern int Magnet2Fractal();
extern int LambdaTrigFractal();
extern int LambdaTrigfpFractal();
extern int LambdaTrigFractal1();
extern int LambdaTrigfpFractal1();
extern int LambdaTrigFractal2();
extern int LambdaTrigfpFractal2();
extern int ManOWarFractal();
extern int ManOWarfpFractal();
extern int MarksMandelPwrfpFractal();
extern int MarksMandelPwrFractal();
extern int TimsErrorfpFractal();
extern int TimsErrorFractal();
extern int CirclefpFractal();
extern int VLfpFractal();
extern int EscherfpFractal();
extern int long_julia_per_pixel();
extern int long_richard8_per_pixel();
extern int long_mandel_per_pixel();
extern int julia_per_pixel();
extern int marks_mandelpwr_per_pixel();
extern int mandel_per_pixel();
extern int marksmandel_per_pixel();
extern int marksmandelfp_per_pixel();
extern int marks_mandelpwrfp_per_pixel();
extern int mandelfp_per_pixel();
extern int juliafp_per_pixel();
extern int MPCjulia_per_pixel();
extern int otherrichard8fp_per_pixel();
extern int othermandelfp_per_pixel();
extern int otherjuliafp_per_pixel();
extern int MarksCplxMandperp();
extern int LambdaTrigOrTrigFractal();
extern int LambdaTrigOrTrigfpFractal();
extern int JuliaTrigOrTrigFractal();
extern int JuliaTrigOrTrigfpFractal();
extern int HalleyFractal();
extern int Halley_per_pixel();
extern int MPCHalleyFractal();
extern int MPCHalley_per_pixel();
extern int dynamfloat(double *, double *, double*);
extern int mandelcloudfloat(double *, double *, double*);
extern int dynam2dfloat();
extern int QuaternionFPFractal();
extern int quaternionfp_per_pixel();
extern int quaternionjulfp_per_pixel();
extern int LongPhoenixFractal();
extern int PhoenixFractal();
extern int long_phoenix_per_pixel();
extern int phoenix_per_pixel();
extern int long_mandphoenix_per_pixel();
extern int mandphoenix_per_pixel();
extern int HyperComplexFPFractal();
extern int LongPhoenixFractalcplx();
extern int PhoenixFractalcplx();
extern int (*floatbailout)();
extern int (*longbailout)();
extern int (*bignumbailout)();
extern int (*bigfltbailout)();
extern int fpMODbailout();
extern int fpREALbailout();
extern int fpIMAGbailout();
extern int fpORbailout();
extern int fpANDbailout();
extern int fpMANHbailout();
extern int fpMANRbailout();
extern int bnMODbailout();
extern int bnREALbailout();
extern int bnIMAGbailout();
extern int bnORbailout();
extern int bnANDbailout();
extern int bnMANHbailout();
extern int bnMANRbailout();
extern int bfMODbailout();
extern int bfREALbailout();
extern int bfIMAGbailout();
extern int bfORbailout();
extern int bfANDbailout();
extern int bfMANHbailout();
extern int bfMANRbailout();
extern int ant();
extern void free_ant_storage();
extern int LongPhoenixFractal();
extern int PhoenixFractal();
extern int LongPhoenixFractalcplx();
extern int PhoenixFractalcplx();
extern int LongPhoenixPlusFractal();
extern int PhoenixPlusFractal();
extern int LongPhoenixMinusFractal();
extern int PhoenixMinusFractal();
extern int LongPhoenixCplxPlusFractal();
extern int PhoenixCplxPlusFractal();
extern int LongPhoenixCplxMinusFractal();
extern int PhoenixCplxMinusFractal();
extern int long_phoenix_per_pixel();
extern int phoenix_per_pixel();
extern int long_mandphoenix_per_pixel();
extern int mandphoenix_per_pixel();
extern void set_pixel_calc_functions();
extern int MandelbrotMix4fp_per_pixel();
extern int MandelbrotMix4fpFractal();
extern bool MandelbrotMix4Setup();
// fractint -- C file prototypes
extern int main(int argc, char **argv);
extern int elapsed_time(int);
// framain2 -- C file prototypes
extern big_while_loop_result big_while_loop(bool *kbdmore, bool *stacked, bool resume_flag);
extern bool check_key();
extern int cmp_line(BYTE *, int);
extern int key_count(int);
extern big_while_loop_result main_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked, int axmode);
extern int pot_line(BYTE *, int);
extern int sound_line(BYTE *, int);
extern int timer(int, int (*subrtn)(), ...);
extern void clear_zoombox();
extern void flip_image(int kbdchar);
extern void reset_zoom_corners();
// frasetup -- C file prototypes
extern bool VLSetup();
extern bool MandelSetup();
extern bool MandelfpSetup();
extern bool JuliaSetup();
extern bool NewtonSetup();
extern bool StandaloneSetup();
extern bool UnitySetup();
extern bool JuliafpSetup();
extern bool MandellongSetup();
extern bool JulialongSetup();
extern bool TrigPlusSqrlongSetup();
extern bool TrigPlusSqrfpSetup();
extern bool TrigPlusTriglongSetup();
extern bool TrigPlusTrigfpSetup();
extern bool FnPlusFnSym();
extern bool ZXTrigPlusZSetup();
extern bool LambdaTrigSetup();
extern bool JuliafnPlusZsqrdSetup();
extern bool SqrTrigSetup();
extern bool FnXFnSetup();
extern bool MandelTrigSetup();
extern bool MarksJuliaSetup();
extern bool MarksJuliafpSetup();
extern bool SierpinskiSetup();
extern bool SierpinskiFPSetup();
extern bool StandardSetup();
extern bool LambdaTrigOrTrigSetup();
extern bool JuliaTrigOrTrigSetup();
extern bool ManlamTrigOrTrigSetup();
extern bool MandelTrigOrTrigSetup();
extern bool HalleySetup();
extern bool dynam2dfloatsetup();
extern bool PhoenixSetup();
extern bool MandPhoenixSetup();
extern bool PhoenixCplxSetup();
extern bool MandPhoenixCplxSetup();
// gifview -- C file prototypes
extern int get_byte();
extern int get_bytes(BYTE *, int);
extern int gifview();
// hcmplx -- C file prototypes
extern void HComplexTrig0(DHyperComplex *, DHyperComplex *);
// intro -- C file prototypes
extern void intro();
// jb -- C file prototypes
extern bool JulibrotSetup();
extern bool JulibrotfpSetup();
extern int jb_per_pixel();
extern int jbfp_per_pixel();
extern int zline(long, long);
extern int zlinefp(double, double);
extern int Std4dFractal();
extern int Std4dfpFractal();
// jiim -- C file prototypes
extern void Jiim(int);
extern LComplex PopLong();
extern DComplex PopFloat();
extern LComplex DeQueueLong();
extern DComplex DeQueueFloat();
extern LComplex ComplexSqrtLong(long, long);
extern DComplex ComplexSqrtFloat(double, double);
extern bool Init_Queue(unsigned long);
extern void   Free_Queue();
extern void   ClearQueue();
extern int    QueueEmpty();
extern int    QueueFull();
extern int    QueueFullAlmost();
extern int    PushLong(long, long);
extern int    PushFloat(float, float);
extern int    EnQueueLong(long, long);
extern int    EnQueueFloat(float, float);
// line3d -- C file prototypes
extern int line3d(BYTE *, unsigned int);
extern int targa_color(int, int, int);
extern bool targa_validate(char *File_Name);
bool startdisk1(char *File_Name2, FILE *Source, bool overlay);
// loadfdos -- C file prototypes
extern int get_video_mode(FRACTAL_INFO *info, ext_blk_3 *blk_3_info);
// loadfile -- C file prototypes
extern int read_overlay();
extern void set_if_old_bif();
extern void set_function_parm_defaults();
extern int fgetwindow();
extern void backwards_v18();
extern void backwards_v19();
extern void backwards_v20();
extern bool check_back();
// loadmap -- C file prototypes
//extern void SetTgaColors();
extern bool ValidateLuts(const char *mapname);
extern int SetColorPaletteName(char *);
// lorenz -- C file prototypes
extern bool orbit3dlongsetup();
extern bool orbit3dfloatsetup();
extern int lorenz3dlongorbit(long *, long *, long *);
extern int lorenz3d1floatorbit(double *, double *, double *);
extern int lorenz3dfloatorbit(double *, double *, double *);
extern int lorenz3d3floatorbit(double *, double *, double *);
extern int lorenz3d4floatorbit(double *, double *, double *);
extern int henonfloatorbit(double *, double *, double *);
extern int henonlongorbit(long *, long *, long *);
extern int inverse_julia_orbit(double *, double *, double *);
extern int Minverse_julia_orbit();
extern int Linverse_julia_orbit();
extern int inverse_julia_per_image();
extern int rosslerfloatorbit(double *, double *, double *);
extern int pickoverfloatorbit(double *, double *, double *);
extern int gingerbreadfloatorbit(double *, double *, double *);
extern int rosslerlongorbit(long *, long *, long *);
extern int kamtorusfloatorbit(double *, double *, double *);
extern int kamtoruslongorbit(long *, long *, long *);
extern int hopalong2dfloatorbit(double *, double *, double *);
extern int chip2dfloatorbit(double *, double *, double *);
extern int quadruptwo2dfloatorbit(double *, double *, double *);
extern int threeply2dfloatorbit(double *, double *, double *);
extern int martin2dfloatorbit(double *, double *, double *);
extern int orbit2dfloat();
extern int orbit2dlong();
extern int funny_glasses_call(int (*)());
extern int ifs();
extern int orbit3dfloat();
extern int orbit3dlong();
extern int iconfloatorbit(double *, double *, double *);
extern int latoofloatorbit(double *, double *, double *);
extern bool setup_convert_to_screen(affine *);
extern int plotorbits2dsetup();
extern int plotorbits2dfloat();
// lsys -- C file prototypes
extern LDBL  getnumber(char **);
extern bool ispow2(int);
extern int Lsystem();
extern bool LLoad();
// miscfrac -- C file prototypes
extern void froth_cleanup();
// miscovl -- C file prototypes
extern void make_batch_file();
extern void edit_text_colors();
extern int select_video_mode(int);
extern void format_vid_table(int choice, char *buf);
extern void make_mig(unsigned int, unsigned int);
extern int getprecdbl(int);
extern int getprecbf(int);
extern int getprecbf_mag();
extern void parse_comments(char *value);
extern void init_comments();
extern void write_batch_parms(char *colorinf, bool colorsonly, int maxcolor, int i, int j);
extern void expand_comments(char *, char *);
// miscres -- C file prototypes
extern void restore_active_ovly();
extern void findpath(const char *filename, char *fullpathname);
extern void notdiskmsg();
extern void cvtcentermag(double *, double *, LDBL *, double *, double *, double *);
extern void cvtcorners(double, double, LDBL, double, double, double);
extern void cvtcentermagbf(bf_t, bf_t, LDBL *, double *, double *, double *);
extern void cvtcornersbf(bf_t, bf_t, LDBL, double, double, double);
extern void updatesavename(char *);
extern int check_writefile(char *name, const char *ext);
extern void showtrig(char *);
extern int set_trig_array(int k, const char *name);
extern void set_trig_pointers(int);
extern int tab_display();
extern int endswithslash(const char *fl);
extern int ifsload();
extern bool find_file_item(char *, char *, FILE **, int);
extern int file_gets(char *, int, FILE *);
extern void roundfloatd(double *);
extern void fix_inversion(double *);
extern int ungetakey(int);
extern void get_calculation_time(char *, long);
// mpmath_c -- C file prototypes
extern MP *MPsub(MP, MP);
extern MP *MPsub086(MP, MP);
extern MP *MPsub386(MP, MP);
extern MP *MPabs(MP);
extern MPC MPCsqr(MPC);
extern MP MPCmod(MPC);
extern MPC MPCmul(MPC, MPC);
extern MPC MPCdiv(MPC, MPC);
extern MPC MPCadd(MPC, MPC);
extern MPC MPCsub(MPC, MPC);
extern MPC MPCpow(MPC, int);
extern int MPCcmp(MPC, MPC);
extern DComplex MPC2cmplx(MPC);
extern MPC cmplx2MPC(DComplex);
extern void setMPfunctions();
extern DComplex ComplexPower(DComplex, DComplex);
extern void SetupLogTable();
extern long logtablecalc(long);
extern long ExpFloat14(long);
extern bool ComplexNewtonSetup();
extern int ComplexNewton();
extern int ComplexBasin();
extern int GausianNumber(int, int);
extern void Arcsinz(DComplex z, DComplex *rz);
extern void Arccosz(DComplex z, DComplex *rz);
extern void Arcsinhz(DComplex z, DComplex *rz);
extern void Arccoshz(DComplex z, DComplex *rz);
extern void Arctanhz(DComplex z, DComplex *rz);
extern void Arctanz(DComplex z, DComplex *rz);
// msccos -- C file prototypes
extern double _cos(double);
// parser -- C file prototypes
struct fls
{ // function, load, store pointers
    void (*function)();
    Arg *operand;
};
extern unsigned int SkipWhiteSpace(char *);
extern unsigned long NewRandNum();
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
extern void FPUcplxexp(DComplex *, DComplex *);
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
extern void (*mtrig0)();
extern void (*mtrig1)();
extern void (*mtrig2)();
extern void (*mtrig3)();
extern void EndInit();
extern struct ConstArg *isconst(char *, int);
extern void NotAFnct();
extern void FnctNotFound();
extern int whichfn(char *, int);
extern int CvtStk();
extern int fFormula();
extern void (*isfunct(char *, int))();
extern void RecSortPrec();
extern int Formula();
extern int BadFormula();
extern int form_per_pixel();
extern int frm_get_param_stuff(char *);
extern bool RunForm(char *Name, bool from_prompts1c);
extern bool fpFormulaSetup();
extern bool intFormulaSetup();
extern void init_misc();
extern void free_workarea();
extern int fill_if_group(int endif_index, JUMP_PTRS_ST *jump_data);
// plot3d -- C file prototypes
extern void draw_line(int, int, int, int, int);
extern void plot3dsuperimpose16(int, int, int);
extern void plot3dsuperimpose256(int, int, int);
extern void plotIFS3dsuperimpose256(int, int, int);
extern void plot3dalternate(int, int, int);
extern void plot_setup();
// printer -- C file prototypes
extern void Print_Screen();
// prompts1 -- C file prototypes
struct fullscreenvalues;
extern int fullscreen_prompt(
    const char *hdg,
    int numprompts,
    const char **prompts,
    fullscreenvalues *values,
    int fkeymask,
    char *extrainfo);
extern long get_file_entry(int type, const char *title, char *fmask,
                    char *filename, char *entryname);
extern int get_fracttype();
extern int get_fract_params(int);
extern int get_fract3d_params();
extern int get_3d_params();
extern int prompt_valuestring(char *buf, fullscreenvalues *val);
extern void setbailoutformula(bailouts);
extern int find_extra_param(fractal_type type);
extern void load_params(fractal_type fractype);
extern bool check_orbit_name(char *);
struct entryinfo;
extern int scan_entries(FILE *infile, struct entryinfo *ch, char *itemname);
// prompts2 -- C file prototypes
extern int get_toggles();
extern int get_toggles2();
extern int passes_options();
extern int get_view_params();
extern int get_starfield_params();
extern int get_commands();
extern void goodbye();
extern bool isadirectory(char *s);
extern bool getafilename(const char *hdg, const char *file_template, char *flname);
extern int splitpath(const char *file_template, char *drive, char *dir, char *fname, char *ext);
extern int makepath(char *template_str, const char *drive, const char *dir, const char *fname, const char *ext);
extern int fr_findfirst(char *path);
extern int fr_findnext();
extern void shell_sort(void *, int n, unsigned, int (*fct)(VOIDPTR, VOIDPTR));
extern void fix_dirname(char *dirname);
extern int merge_pathnames(char *oldfullpath, char *newfilename, cmd_file mode);
extern int get_browse_params();
extern int get_cmd_string();
extern int get_rds_params();
extern int starfield();
extern int get_a_number(double *, double *);
extern int lccompare(VOIDPTR, VOIDPTR);
extern int dir_remove(char *, char *);
extern FILE *dir_fopen(const char *dir, const char *filename, const char *mode);
extern void extract_filename(char *, char *);
extern char *has_ext(char *source);
// realdos -- C file prototypes
extern int showvidlength();
extern int stopmsg(int flags, const char *msg);
extern void blankrows(int, int, int);
extern int texttempmsg(const char *);
extern int fullscreen_choice(
    int options,
    const char *hdg,
    const char *hdg2,
    const char *instr,
    int numchoices,
    const char **choices,
    int *attributes,
    int boxwidth,
    int boxdepth,
    int colwidth,
    int current,
    void (*formatitem)(int, char*),
    char *speedstring,
    int (*speedprompt)(int, int, int, char *, int),
    int (*checkkey)(int, int)
);
extern bool showtempmsg(const char *);
extern void cleartempmsg();
extern void helptitle();
extern int putstringcenter(int row, int col, int width, int attr, const char *msg);
extern int main_menu(int);
extern int input_field(int, int, char *, int, int, int, int (*)(int));
extern int field_prompt(const char *hdg, const char *instr, char *fld, int len, int (*checkkey)(int));
extern bool thinking(int options, const char *msg);
extern void discardgraphics();
extern void load_fractint_config();
extern int check_vidmode_key(int, int);
extern int check_vidmode_keyname(char *);
extern void vidmode_keyname(int, char *);
extern void freetempmsg();
extern void load_videotable(int);
extern void bad_fractint_cfg_msg();
// rotate -- C file prototypes
extern void rotate(int);
extern void save_palette();
extern bool load_palette();
// slideshw -- C file prototypes
extern int slideshw();
extern slides_mode startslideshow();
extern void stopslideshow();
extern void recordshw(int);
// stereo -- C file prototypes
extern bool do_AutoStereo();
extern int outline_stereo(BYTE *, int);
// testpt -- C file prototypes
extern int teststart();
extern void testend();
extern int testpt(double, double, double, double, long, int);
// zoom -- C file prototypes
extern void drawbox(int);
extern void moveboxf(double, double);
extern void resizebox(int);
extern void chgboxi(int, int);
extern void zoomout();
extern void aspectratio_crop(float, float);
extern int init_pan_or_recalc(int);
extern void drawlines(struct coords, struct coords, int, int);
extern void addbox(struct coords);
extern void clearbox();
extern void dispbox();
// fractalb.c -- C file prototypes
extern DComplex cmplxbntofloat(BNComplex *);
extern DComplex cmplxbftofloat(BFComplex *);
extern void comparevalues(char *, LDBL, bn_t);
extern void comparevaluesbf(char *, LDBL, bf_t);
extern void show_var_bf(char *s, bf_t n);
extern void show_two_bf(char *, bf_t, char *, bf_t, int);
extern void bfcornerstofloat();
extern void showcornersdbl(char *);
extern bool MandelbnSetup();
extern int mandelbn_per_pixel();
extern int juliabn_per_pixel();
extern int JuliabnFractal();
extern int JuliaZpowerbnFractal();
extern BNComplex *cmplxlog_bn(BNComplex *t, BNComplex *s);
extern BNComplex *cplxmul_bn(BNComplex *t, BNComplex *x, BNComplex *y);
extern BNComplex *ComplexPower_bn(BNComplex *t, BNComplex *xx, BNComplex *yy);
extern bool MandelbfSetup();
extern int mandelbf_per_pixel();
extern int juliabf_per_pixel();
extern int JuliabfFractal();
extern int JuliaZpowerbfFractal();
extern BFComplex *cmplxlog_bf(BFComplex *t, BFComplex *s);
extern BFComplex *cplxmul_bf(BFComplex *t, BFComplex *x, BFComplex *y);
extern BFComplex *ComplexPower_bf(BFComplex *t, BFComplex *xx, BFComplex *yy);
// memory -- C file prototypes
// TODO: Get rid of this and use regular memory routines;
// see about creating standard disk memory routines for disk video
extern void DisplayHandle(U16 handle);
extern int MemoryType(U16 handle);
extern void InitMemory();
extern void ExitCheck();
extern U16 MemoryAlloc(U16 size, long count, int stored_at);
extern void MemoryRelease(U16 handle);
extern bool CopyFromMemoryToHandle(BYTE const *buffer, U16 size, long count, long offset, U16 handle);
extern bool CopyFromHandleToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern bool SetMemory(int value, U16 size, long count, long offset, U16 handle);
// soi -- C file prototypes
extern void soi();
extern void soi_ldbl();
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
extern int abortmsg(char *file, unsigned int line, int flags, char *msg);
#define ABORT(flags_, msg_) abortmsg(__FILE__, __LINE__, flags_, msg_)
extern long stackavail();
extern int getcolor(int, int);
extern int out_line(BYTE *, int);
extern void setvideomode(int, int, int, int);
extern int pot_startdisk();
extern void putcolor_a(int, int, int);
extern int startdisk();
#endif
