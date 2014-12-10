#ifndef PROTOTYP_H
#define PROTOTYP_H

/* includes needed to define the prototypes */

#include "mpmath.h"
#include "big.h"
#include "fractint.h"
#include "externs.h"

/* maintain the common prototypes in this file
 */

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
extern void find_special_colors(void);
extern int getakeynohelp(void);
extern long readticker(void);
extern void adapter_detect(void);
extern int get_sound_params(void);
extern void setnullvideo(void);

/*  calcmand -- assembler file prototypes */

extern long calcmandasm(void);

/*  calmanfp -- assembler file prototypes */

extern void calcmandfpasmstart(void);
extern long calcmandfpasm(void);

/*  fpu087 -- assembler file prototypes */

extern void FPUcplxmul(DComplex *, DComplex *, DComplex *);
extern void FPUcplxdiv(DComplex *, DComplex *, DComplex *);
extern void FPUsincos(double *, double *, double *);
extern void FPUsinhcosh(double *, double *, double *);
extern void FPUcplxlog(DComplex *, DComplex *);
extern void SinCos086(long , long *, long *);
extern void SinhCosh086(long , long *, long *);
extern long r16Mul(long , long);
extern long RegFloat2Fg(long , int);
extern long Exp086(long);
extern unsigned long ExpFudged(long , int);
extern long RegDivFloat(long , long);
extern long LogFudged(unsigned long , int);
extern long LogFloat14(unsigned long);
extern long RegFg2Float(long , int);
extern long RegSftFloat(long , int);

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
extern int asmfpMODbailout(void);
extern int asmfpREALbailout(void);
extern int asmfpIMAGbailout(void);
extern int asmfpORbailout(void);
extern int asmfpANDbailout(void);
extern int asmfpMANHbailout(void);
extern int asmfpMANRbailout(void);

/*  mpmath_a -- assembler file prototypes */

extern struct MP * MPmul086(struct MP , struct MP);
extern struct MP * MPdiv086(struct MP , struct MP);
extern struct MP * MPadd086(struct MP , struct MP);
extern int         MPcmp086(struct MP , struct MP);
extern struct MP * d2MP086(double);
extern double    * MP2d086(struct MP);
extern struct MP * fg2MP086(long , int);
extern struct MP * MPmul386(struct MP , struct MP);
extern struct MP * MPdiv386(struct MP , struct MP);
extern struct MP * MPadd386(struct MP , struct MP);
extern int         MPcmp386(struct MP , struct MP);
extern struct MP * d2MP386(double);
extern double    * MP2d386(struct MP);
extern struct MP * fg2MP386(long , int);
extern double *    MP2d(struct MP);
extern int         MPcmp(struct MP , struct MP);
extern struct MP * MPmul(struct MP , struct MP);
extern struct MP * MPadd(struct MP , struct MP);
extern struct MP * MPdiv(struct MP , struct MP);
extern struct MP * d2MP(double);   /* Convert double to type MP */
extern struct MP * fg2MP(long , int);  /* Convert fudged to type MP */

/*  newton -- assembler file prototypes */

extern int NewtonFractal2(void);
extern void invertz2(DComplex *);

/*  tplus_a -- assembler file prototypes */

extern void WriteTPlusBankedPixel(int, int, unsigned long);
extern unsigned long ReadTPlusBankedPixel(int, int);

/*  3d -- C file prototypes */

extern void identity(MATRIX);
extern void mat_mul(MATRIX,MATRIX,MATRIX);
extern void scale(double ,double ,double ,MATRIX);
extern void xrot(double ,MATRIX);
extern void yrot(double ,MATRIX);
extern void zrot(double ,MATRIX);
extern void trans(double ,double ,double ,MATRIX);
extern int cross_product(VECTOR,VECTOR,VECTOR);
extern int normalize_vector(VECTOR);
extern int vmult(VECTOR,MATRIX,VECTOR);
extern void mult_vec(VECTOR);
extern int perspective(VECTOR);
extern int longvmultpersp(LVECTOR,LMATRIX,LVECTOR,LVECTOR,LVECTOR,int);
extern int longpersp(LVECTOR,LVECTOR,int);
extern int longvmult(LVECTOR,LMATRIX,LVECTOR,int);

/*  biginit -- C file prototypes */

void free_bf_vars(void);
bn_t alloc_stack(size_t size);
int save_stack(void);
void restore_stack(int old_offset);
void init_bf_dec(int dec);
void init_bf_length(int bnl);
void init_big_pi(void);


/*  calcfrac -- C file prototypes */

extern int calcfract(void);
extern int calcmand(void);
extern int calcmandfp(void);
extern int StandardFractal(void);
extern int test(void);
extern int plasma(void);
extern int diffusion(void);
extern int Bifurcation(void);
extern int BifurcLambda(void);
extern int BifurcSetTrigPi(void);
extern int LongBifurcSetTrigPi(void);
extern int BifurcAddTrigPi(void);
extern int LongBifurcAddTrigPi(void);
extern int BifurcMay(void);
extern bool BifurcMaySetup(void);
extern int LongBifurcMay(void);
extern int BifurcLambdaTrig(void);
extern int LongBifurcLambdaTrig(void);
extern int BifurcVerhulstTrig(void);
extern int LongBifurcVerhulstTrig(void);
extern int BifurcStewartTrig(void);
extern int LongBifurcStewartTrig(void);
extern int popcorn(void);
extern int lyapunov(void);
extern bool lya_setup(void);
extern int cellular(void);
extern bool CellularSetup(void);
extern int calcfroth(void);
extern int froth_per_pixel(void);
extern int froth_per_orbit(void);
extern bool froth_setup(void);
extern int logtable_in_extra_ok(void);
extern int find_alternate_math(int, int);

/*  cmdfiles -- C file prototypes */

extern int cmdfiles(int ,char **);
extern int load_commands(FILE *);
extern void set_3d_defaults(void);
extern int get_curarg_len(const char *curarg);
extern int get_max_curarg_len(const char *floatvalstr[], int totparm);
extern int init_msg(const char *cmdstr, char *badfilename, int mode);
extern int cmdarg(char *curarg,int mode);
extern int getpower10(LDBL x);
extern void dopause(int);

/*  decoder -- C file prototypes */

extern short decoder(short);
extern void set_byte_buff(BYTE *ptr);

/*  diskvid -- C file prototypes */

extern int pot_startdisk(void);
extern int targa_startdisk(FILE *,int);
extern void enddisk(void);
extern int readdisk(int, int);
extern void writedisk(int, int, int);
extern void targa_readdisk(unsigned int ,unsigned int ,BYTE *,BYTE *,BYTE *);
extern void targa_writedisk(unsigned int ,unsigned int ,BYTE ,BYTE ,BYTE);
extern void dvid_status(int line, const char *msg);
extern int  common_startdisk(long, long, int);
extern int FromMemDisk(long,int,void *);
extern bool ToMemDisk(long,int,void *);

/*  editpal -- C file prototypes */

extern void EditPalette(void);
extern void *mem_alloc(unsigned size);
void putrow(int x, int y, int width, char *buff);
void getrow(int x, int y, int width, char *buff);
void mem_init(void *block, unsigned size);
int Cursor_WaitKey(void);
void Cursor_CheckBlink(void);
void clip_putcolor(int x, int y, int color);
int clip_getcolor(int x, int y);
BOOLEAN Cursor_Construct(void);
void Cursor_Destroy(void);
void Cursor_SetPos(int x, int y);
void Cursor_Move(int xoff, int yoff);
int Cursor_GetX(void);
int Cursor_GetY(void);
void Cursor_Hide(void);
void Cursor_Show(void);
extern void displayc(int, int, int, int, int);

/*  encoder -- C file prototypes */

extern int savetodisk(char *);
extern bool encoder(void);
extern int new_to_old(int new_fractype);

/*  evolve -- C file prototypes */

extern  void initgene(void);
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

extern FILE *t16_open(char *,int *,int *,int *,U8 *);
extern int t16_getline(FILE *,int ,U16 *);

/*  fracsubr -- C file prototypes */

extern void free_grid_pointers(void);
extern void calcfracinit(void);
extern void adjust_corner(void);
extern int put_resume(int ,...);
extern int get_resume(int ,...);
extern int alloc_resume(int ,int);
extern int start_resume(void);
extern void end_resume(void);
extern void sleepms(long);
extern void reset_clock(void);
extern void iplot_orbit(long ,long ,int);
extern void plot_orbit(double ,double ,int);
extern void scrub_orbit(void);
extern int add_worklist(int ,int, int ,int ,int ,int ,int ,int);
extern void tidy_worklist(void);
extern void get_julia_attractor(double ,double);
extern int ssg_blocksize(void);
extern void symPIplot(int ,int ,int);
extern void symPIplot2J(int ,int ,int);
extern void symPIplot4J(int ,int ,int);
extern void symplot2(int ,int ,int);
extern void symplot2Y(int ,int ,int);
extern void symplot2J(int ,int ,int);
extern void symplot4(int ,int ,int);
extern void symplot2basin(int ,int ,int);
extern void symplot4basin(int ,int ,int);
extern void noplot(int ,int ,int);
extern void fractal_floattobf(void);
extern void adjust_cornerbf(void);
extern void set_grid_pointers(void);
extern void fill_dx_array(void);
extern void fill_lx_array(void);
extern int snd_open(void);
extern void w_snd(int);
extern void snd_time_write(void);
extern void close_snd(void);

/*  fractalp -- C file prototypes */

extern bool typehasparm(int type, int parm, char *buf);
extern bool paramnotused(int);

/*  fractals -- C file prototypes */

extern void FloatPreCalcMagnet2(void);
extern void cpower(DComplex *,int ,DComplex *);
extern int lcpower(LComplex *,int ,LComplex *,int);
extern int lcomplex_mult(LComplex ,LComplex ,LComplex *,int);
extern int MPCNewtonFractal(void);
extern int Barnsley1Fractal(void);
extern int Barnsley1FPFractal(void);
extern int Barnsley2Fractal(void);
extern int Barnsley2FPFractal(void);
extern int JuliaFractal(void);
extern int JuliafpFractal(void);
extern int LambdaFPFractal(void);
extern int LambdaFractal(void);
extern int SierpinskiFractal(void);
extern int SierpinskiFPFractal(void);
extern int LambdaexponentFractal(void);
extern int LongLambdaexponentFractal(void);
extern int FloatTrigPlusExponentFractal(void);
extern int LongTrigPlusExponentFractal(void);
extern int MarksLambdaFractal(void);
extern int MarksLambdafpFractal(void);
extern int UnityFractal(void);
extern int UnityfpFractal(void);
extern int Mandel4Fractal(void);
extern int Mandel4fpFractal(void);
extern int floatZtozPluszpwrFractal(void);
extern int longZpowerFractal(void);
extern int longCmplxZpowerFractal(void);
extern int floatZpowerFractal(void);
extern int floatCmplxZpowerFractal(void);
extern int Barnsley3Fractal(void);
extern int Barnsley3FPFractal(void);
extern int TrigPlusZsquaredFractal(void);
extern int TrigPlusZsquaredfpFractal(void);
extern int Richard8fpFractal(void);
extern int Richard8Fractal(void);
extern int PopcornFractal(void);
extern int LPopcornFractal(void);
extern int PopcornFractal_Old(void);
extern int LPopcornFractal_Old(void);
extern int PopcornFractalFn(void);
extern int LPopcornFractalFn(void);
extern int MarksCplxMand(void);
extern int SpiderfpFractal(void);
extern int SpiderFractal(void);
extern int TetratefpFractal(void);
extern int ZXTrigPlusZFractal(void);
extern int ScottZXTrigPlusZFractal(void);
extern int SkinnerZXTrigSubZFractal(void);
extern int ZXTrigPlusZfpFractal(void);
extern int ScottZXTrigPlusZfpFractal(void);
extern int SkinnerZXTrigSubZfpFractal(void);
extern int Sqr1overTrigFractal(void);
extern int Sqr1overTrigfpFractal(void);
extern int TrigPlusTrigFractal(void);
extern int TrigPlusTrigfpFractal(void);
extern int ScottTrigPlusTrigFractal(void);
extern int ScottTrigPlusTrigfpFractal(void);
extern int SkinnerTrigSubTrigFractal(void);
extern int SkinnerTrigSubTrigfpFractal(void);
extern int TrigXTrigfpFractal(void);
extern int TrigXTrigFractal(void);
extern int TrigPlusSqrFractal(void);
extern int TrigPlusSqrfpFractal(void);
extern int ScottTrigPlusSqrFractal(void);
extern int ScottTrigPlusSqrfpFractal(void);
extern int SkinnerTrigSubSqrFractal(void);
extern int SkinnerTrigSubSqrfpFractal(void);
extern int TrigZsqrdfpFractal(void);
extern int TrigZsqrdFractal(void);
extern int SqrTrigFractal(void);
extern int SqrTrigfpFractal(void);
extern int Magnet1Fractal(void);
extern int Magnet2Fractal(void);
extern int LambdaTrigFractal(void);
extern int LambdaTrigfpFractal(void);
extern int LambdaTrigFractal1(void);
extern int LambdaTrigfpFractal1(void);
extern int LambdaTrigFractal2(void);
extern int LambdaTrigfpFractal2(void);
extern int ManOWarFractal(void);
extern int ManOWarfpFractal(void);
extern int MarksMandelPwrfpFractal(void);
extern int MarksMandelPwrFractal(void);
extern int TimsErrorfpFractal(void);
extern int TimsErrorFractal(void);
extern int CirclefpFractal(void);
extern int VLfpFractal(void);
extern int EscherfpFractal(void);
extern int long_julia_per_pixel(void);
extern int long_richard8_per_pixel(void);
extern int long_mandel_per_pixel(void);
extern int julia_per_pixel(void);
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
extern int LambdaTrigOrTrigFractal(void);
extern int LambdaTrigOrTrigfpFractal(void);
extern int JuliaTrigOrTrigFractal(void);
extern int JuliaTrigOrTrigfpFractal(void);
extern int HalleyFractal(void);
extern int Halley_per_pixel(void);
extern int MPCHalleyFractal(void);
extern int MPCHalley_per_pixel(void);
extern int dynamfloat(double *,double *,double*);
extern int mandelcloudfloat(double *,double *,double*);
extern int dynam2dfloat(void);
extern int QuaternionFPFractal(void);
extern int quaternionfp_per_pixel(void);
extern int quaternionjulfp_per_pixel(void);
extern int LongPhoenixFractal(void);
extern int PhoenixFractal(void);
extern int long_phoenix_per_pixel(void);
extern int phoenix_per_pixel(void);
extern int long_mandphoenix_per_pixel(void);
extern int mandphoenix_per_pixel(void);
extern int HyperComplexFPFractal(void);
extern int LongPhoenixFractalcplx(void);
extern int PhoenixFractalcplx(void);
extern int (*floatbailout)(void);
extern int (*longbailout)(void);
extern int (*bignumbailout)(void);
extern int (*bigfltbailout)(void);
extern int fpMODbailout(void);
extern int fpREALbailout(void);
extern int fpIMAGbailout(void);
extern int fpORbailout(void);
extern int fpANDbailout(void);
extern int fpMANHbailout(void);
extern int fpMANRbailout(void);
extern int bnMODbailout(void);
extern int bnREALbailout(void);
extern int bnIMAGbailout(void);
extern int bnORbailout(void);
extern int bnANDbailout(void);
extern int bnMANHbailout(void);
extern int bnMANRbailout(void);
extern int bfMODbailout(void);
extern int bfREALbailout(void);
extern int bfIMAGbailout(void);
extern int bfORbailout(void);
extern int bfANDbailout(void);
extern int bfMANHbailout(void);
extern int bfMANRbailout(void);
extern int ant(void);
extern void free_ant_storage(void);
extern int LongPhoenixFractal(void);
extern int PhoenixFractal(void);
extern int LongPhoenixFractalcplx(void);
extern int PhoenixFractalcplx(void);
extern int LongPhoenixPlusFractal(void);
extern int PhoenixPlusFractal(void);
extern int LongPhoenixMinusFractal(void);
extern int PhoenixMinusFractal(void);
extern int LongPhoenixCplxPlusFractal(void);
extern int PhoenixCplxPlusFractal(void);
extern int LongPhoenixCplxMinusFractal(void);
extern int PhoenixCplxMinusFractal(void);
extern int long_phoenix_per_pixel(void);
extern int phoenix_per_pixel(void);
extern int long_mandphoenix_per_pixel(void);
extern int mandphoenix_per_pixel(void);
extern void set_pixel_calc_functions(void);
extern int MandelbrotMix4fp_per_pixel(void);
extern int MandelbrotMix4fpFractal(void);
extern bool MandelbrotMix4Setup(void);

/*  fractint -- C file prototypes */

extern int main(int argc,char **argv);
extern int elapsed_time(int);

/*  framain2 -- C file prototypes */

extern int big_while_loop(bool *kbdmore, bool *stacked, bool resume_flag);
extern int check_key(void);
extern int cmp_line(BYTE *,int);
extern int key_count(int);
extern int main_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked, int axmode);
extern int pot_line(BYTE *,int);
extern int sound_line(BYTE *,int);
extern int timer(int,int (*subrtn)(),...);

extern void clear_zoombox(void);
extern void flip_image(int kbdchar);
extern void reset_zoom_corners(void);

/*  frasetup -- C file prototypes */

extern bool VLSetup(void);
extern bool MandelSetup(void);
extern bool MandelfpSetup(void);
extern bool JuliaSetup(void);
extern bool NewtonSetup(void);
extern bool StandaloneSetup(void);
extern bool UnitySetup(void);
extern bool JuliafpSetup(void);
extern bool MandellongSetup(void);
extern bool JulialongSetup(void);
extern bool TrigPlusSqrlongSetup(void);
extern bool TrigPlusSqrfpSetup(void);
extern bool TrigPlusTriglongSetup(void);
extern bool TrigPlusTrigfpSetup(void);
extern bool FnPlusFnSym(void);
extern bool ZXTrigPlusZSetup(void);
extern bool LambdaTrigSetup(void);
extern bool JuliafnPlusZsqrdSetup(void);
extern bool SqrTrigSetup(void);
extern bool FnXFnSetup(void);
extern bool MandelTrigSetup(void);
extern bool MarksJuliaSetup(void);
extern bool MarksJuliafpSetup(void);
extern bool SierpinskiSetup(void);
extern bool SierpinskiFPSetup(void);
extern bool StandardSetup(void);
extern bool LambdaTrigOrTrigSetup(void);
extern bool JuliaTrigOrTrigSetup(void);
extern bool ManlamTrigOrTrigSetup(void);
extern bool MandelTrigOrTrigSetup(void);
extern bool HalleySetup(void);
extern bool dynam2dfloatsetup(void);
extern bool PhoenixSetup(void);
extern bool MandPhoenixSetup(void);
extern bool PhoenixCplxSetup(void);
extern bool MandPhoenixCplxSetup(void);

/*  gifview -- C file prototypes */

extern int get_byte(void);
extern int get_bytes(BYTE *,int);
extern int gifview(void);

/*  hcmplx -- C file prototypes */

extern void HComplexTrig0(DHyperComplex *,DHyperComplex *);

/*  intro -- C file prototypes */

extern void intro(void);

/*  jb -- C file prototypes */

extern bool JulibrotSetup(void);
extern bool JulibrotfpSetup(void);
extern int jb_per_pixel(void);
extern int jbfp_per_pixel(void);
extern int zline(long ,long);
extern int zlinefp(double ,double);
extern int Std4dFractal(void);
extern int Std4dfpFractal(void);

/*  jiim -- C file prototypes */

extern void Jiim(int);
extern LCMPLX PopLong(void);
extern DComplex PopFloat(void);
extern LCMPLX DeQueueLong(void);
extern DComplex DeQueueFloat(void);
extern LCMPLX ComplexSqrtLong(long ,  long);
extern DComplex ComplexSqrtFloat(double, double);
extern bool Init_Queue(unsigned long);
extern void   Free_Queue(void);
extern void   ClearQueue(void);
extern int    QueueEmpty(void);
extern int    QueueFull(void);
extern int    QueueFullAlmost(void);
extern int    PushLong(long ,  long);
extern int    PushFloat(float,  float);
extern int    EnQueueLong(long ,  long);
extern int    EnQueueFloat(float,  float);

/*  line3d -- C file prototypes */

extern int line3d(BYTE *,unsigned int);
extern int targa_color(int ,int ,int);
extern bool targa_validate(char *File_Name);
bool startdisk1(char *File_Name2, FILE *Source, bool overlay);

/*  loadfdos -- C file prototypes */
extern int get_video_mode(struct fractal_info *,struct ext_blk_3 *);

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
extern bool ValidateLuts(const char *mapname);
extern int SetColorPaletteName(char *);

/*  lorenz -- C file prototypes */

extern bool orbit3dlongsetup(void);
extern bool orbit3dfloatsetup(void);
extern int lorenz3dlongorbit(long *,long *,long *);
extern int lorenz3d1floatorbit(double *,double *,double *);
extern int lorenz3dfloatorbit(double *,double *,double *);
extern int lorenz3d3floatorbit(double *,double *,double *);
extern int lorenz3d4floatorbit(double *,double *,double *);
extern int henonfloatorbit(double *,double *,double *);
extern int henonlongorbit(long *,long *,long *);
extern int inverse_julia_orbit(double *,double *,double *);
extern int Minverse_julia_orbit(void);
extern int Linverse_julia_orbit(void);
extern int inverse_julia_per_image(void);
extern int rosslerfloatorbit(double *,double *,double *);
extern int pickoverfloatorbit(double *,double *,double *);
extern int gingerbreadfloatorbit(double *,double *,double *);
extern int rosslerlongorbit(long *,long *,long *);
extern int kamtorusfloatorbit(double *,double *,double *);
extern int kamtoruslongorbit(long *,long *,long *);
extern int hopalong2dfloatorbit(double *,double *,double *);
extern int chip2dfloatorbit(double *,double *,double *);
extern int quadruptwo2dfloatorbit(double *,double *,double *);
extern int threeply2dfloatorbit(double *,double *,double *);
extern int martin2dfloatorbit(double *,double *,double *);
extern int orbit2dfloat(void);
extern int orbit2dlong(void);
extern int funny_glasses_call(int (*)(void));
extern int ifs(void);
extern int orbit3dfloat(void);
extern int orbit3dlong(void);
extern int iconfloatorbit(double *, double *, double *);
extern int latoofloatorbit(double *, double *, double *);
extern int  setup_convert_to_screen(struct affine *);
extern int plotorbits2dsetup(void);
extern int plotorbits2dfloat(void);

/*  lsys -- C file prototypes */

extern LDBL  getnumber(char **);
extern bool ispow2(int);
extern int Lsystem(void);
extern int LLoad(void);

/*  miscfrac -- C file prototypes */

extern void froth_cleanup(void);

/*  miscovl -- C file prototypes */

extern void make_batch_file(void);
extern void edit_text_colors(void);
extern int select_video_mode(int);
extern void format_vid_table(int choice,char *buf);
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
extern void findpath(const char *filename, char *fullpathname);
extern void notdiskmsg(void);
extern void cvtcentermag(double *,double *,LDBL *, double *,double *,double *);
extern void cvtcorners(double,double,LDBL,double,double,double);
extern void cvtcentermagbf(bf_t, bf_t, LDBL *, double *, double *, double *);
extern void cvtcornersbf(bf_t, bf_t, LDBL,double,double,double);
extern void updatesavename(char *);
extern int check_writefile(char *name, const char *ext);
extern int check_key(void);
extern void showtrig(char *);
extern int set_trig_array(int k, const char *name);
extern void set_trig_pointers(int);
extern int tab_display(void);
extern int endswithslash(const char *fl);
extern int ifsload(void);
extern int find_file_item(char *,char *,FILE **, int);
extern int file_gets(char *,int ,FILE *);
extern void roundfloatd(double *);
extern void fix_inversion(double *);
extern int ungetakey(int);
extern void get_calculation_time(char *, long);

/*  mpmath_c -- C file prototypes */

extern struct MP *MPsub(struct MP ,struct MP);
extern struct MP *MPsub086(struct MP ,struct MP);
extern struct MP *MPsub386(struct MP ,struct MP);
extern struct MP *MPabs(struct MP);
extern struct MPC MPCsqr(struct MPC);
extern struct MP MPCmod(struct MPC);
extern struct MPC MPCmul(struct MPC ,struct MPC);
extern struct MPC MPCdiv(struct MPC ,struct MPC);
extern struct MPC MPCadd(struct MPC ,struct MPC);
extern struct MPC MPCsub(struct MPC ,struct MPC);
extern struct MPC MPCpow(struct MPC ,int);
extern int MPCcmp(struct MPC ,struct MPC);
extern DComplex MPC2cmplx(struct MPC);
extern struct MPC cmplx2MPC(DComplex);
extern void setMPfunctions(void);
extern DComplex ComplexPower(DComplex ,DComplex);
extern void SetupLogTable(void);
extern long logtablecalc(long);
extern long ExpFloat14(long);
extern bool ComplexNewtonSetup(void);
extern int ComplexNewton(void);
extern int ComplexBasin(void);
extern int GausianNumber(int ,int);
extern void Arcsinz(DComplex z, DComplex *rz);
extern void Arccosz(DComplex z, DComplex *rz);
extern void Arcsinhz(DComplex z, DComplex *rz);
extern void Arccoshz(DComplex z, DComplex *rz);
extern void Arctanhz(DComplex z, DComplex *rz);
extern void Arctanz(DComplex z, DComplex *rz);

/*  msccos -- C file prototypes */

extern double _cos(double);

/*  parser -- C file prototypes */

struct fls { /* function, load, store pointers */
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
extern void FPUcplxexp(DComplex *,DComplex *);
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
extern struct ConstArg *isconst(char *,int);
extern void NotAFnct(void);
extern void FnctNotFound(void);
extern int whichfn(char *,int);
extern int CvtStk(void);
extern int fFormula(void);
extern void (*isfunct(char *,int))(void);
extern void RecSortPrec(void);
extern int Formula(void);
extern int BadFormula(void);
extern int form_per_pixel(void);
extern int frm_get_param_stuff(char *);
extern bool RunForm(char *Name, bool from_prompts1c);
extern bool fpFormulaSetup(void);
extern bool intFormulaSetup(void);
extern void init_misc(void);
extern void free_workarea(void);
extern int fill_if_group(int endif_index, JUMP_PTRS_ST *jump_data);

/*  plot3d -- C file prototypes */

extern void draw_line(int ,int ,int ,int ,int);
extern void plot3dsuperimpose16(int ,int ,int);
extern void plot3dsuperimpose256(int ,int ,int);
extern void plotIFS3dsuperimpose256(int ,int ,int);
extern void plot3dalternate(int ,int ,int);
extern void plot_setup(void);

/*  printer -- C file prototypes */

extern void Print_Screen(void);

/*  prompts1 -- C file prototypes */

extern int fullscreen_prompt(
    const char *hdg,
    int numprompts,
    const char **prompts,
    struct fullscreenvalues *values,
    int fkeymask,
    char *extrainfo);
extern long get_file_entry(int type, const char *title,char *fmask,
                    char *filename,char *entryname);
extern int get_fracttype(void);
extern int get_fract_params(int);
extern int get_fract3d_params(void);
extern int get_3d_params(void);
extern int prompt_valuestring(char *buf,struct fullscreenvalues *val);
extern void setbailoutformula(enum bailouts);
extern int find_extra_param(int);
extern void load_params(int fractype);
extern int check_orbit_name(char *);
struct entryinfo;
extern int scan_entries(FILE *infile, struct entryinfo *ch, char *itemname);

/*  prompts2 -- C file prototypes */

extern int get_toggles(void);
extern int get_toggles2(void);
extern int passes_options(void);
extern int get_view_params(void);
extern int get_starfield_params(void);
extern int get_commands(void);
extern void goodbye(void);
extern int isadirectory(char *s);
extern int getafilename(const char *hdg, const char *file_template, char *flname);
extern int splitpath(const char *file_template, char *drive, char *dir, char *fname, char *ext);
extern int makepath(char *template_str, const char *drive, const char *dir, const char *fname, const char *ext);
extern int fr_findfirst(char *path);
extern int fr_findnext(void);
extern void shell_sort(void *,int n,unsigned,int (*fct)(VOIDPTR,VOIDPTR));
extern void fix_dirname(char *dirname);
extern int merge_pathnames(char *, char *, int);
extern int get_browse_params(void);
extern int get_cmd_string(void);
extern int get_rds_params(void);
extern int starfield(void);
extern int get_a_number(double *, double *);
extern int lccompare(VOIDPTR, VOIDPTR);
extern int dir_remove(char *,char *);
extern FILE *dir_fopen(const char *dir, const char *filename, const char *mode);
extern void extract_filename(char *, char *);
extern char *has_ext(char *source);

/*  realdos -- C file prototypes */

extern int showvidlength(void);
extern int stopmsg(int flags, const char *msg);
extern void blankrows(int ,int ,int);
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
    void (*formatitem)(int,char*),
    char *speedstring,
    int (*speedprompt)(int,int,int,char *,int),
    int (*checkkey)(int,int)
);

extern int showtempmsg(const char *);
extern void cleartempmsg(void);
extern void helptitle(void);
extern int putstringcenter(int row, int col, int width, int attr, const char *msg);
extern int main_menu(int);
extern int input_field(int ,int ,char *,int ,int ,int ,int (*)(int));
extern int field_prompt(const char *hdg, const char *instr, char *fld, int len, int (*checkkey)(int));
extern int thinking(int options, const char *msg);
extern void discardgraphics(void);
extern void load_fractint_config(void);
extern int check_vidmode_key(int ,int);
extern int check_vidmode_keyname(char *);
extern void vidmode_keyname(int ,char *);
extern void freetempmsg(void);
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

/*  testpt -- C file prototypes */

extern int teststart(void);
extern void testend(void);
extern int testpt(double ,double ,double ,double ,long ,int);

/*  tgaview -- C file prototypes */

extern int tgaview(void);
extern int outlin16(BYTE*,int);

/*  zoom -- C file prototypes */

extern void drawbox(int);
extern void moveboxf(double ,double);
extern void resizebox(int);
extern void chgboxi(int ,int);
extern void zoomout(void);
extern void aspectratio_crop(float ,float);
extern int init_pan_or_recalc(int);
extern void drawlines(struct coords, struct coords, int, int);
extern void addbox(struct coords);
extern void clearbox(void);
extern void dispbox(void);

/*  fractalb.c -- C file prototypes */

extern DComplex cmplxbntofloat(BNComplex *);
extern DComplex cmplxbftofloat(BFComplex *);
extern void comparevalues(char *,LDBL,bn_t);
extern void comparevaluesbf(char *,LDBL,bf_t);
extern void show_var_bf(char *s, bf_t n);
extern void show_two_bf(char *,bf_t,char *, bf_t, int);
extern void bfcornerstofloat(void);
extern void showcornersdbl(char *);
extern bool MandelbnSetup(void);
extern int mandelbn_per_pixel(void);
extern int juliabn_per_pixel(void);
extern int JuliabnFractal(void);
extern int JuliaZpowerbnFractal(void);
extern BNComplex *cmplxlog_bn(BNComplex *t, BNComplex *s);
extern BNComplex *cplxmul_bn(BNComplex *t, BNComplex *x, BNComplex *y);
extern BNComplex *ComplexPower_bn(BNComplex *t, BNComplex *xx, BNComplex *yy);
extern bool MandelbfSetup(void);
extern int mandelbf_per_pixel(void);
extern int juliabf_per_pixel(void);
extern int JuliabfFractal(void);
extern int JuliaZpowerbfFractal(void);
extern BFComplex *cmplxlog_bf(BFComplex *t, BFComplex *s);
extern BFComplex *cplxmul_bf(BFComplex *t, BFComplex *x, BFComplex *y);
extern BFComplex *ComplexPower_bf(BFComplex *t, BFComplex *xx, BFComplex *yy);

/*  memory -- C file prototypes */
/* TODO: Get rid of this and use regular memory routines;
** see about creating standard disk memory routines for disk video
*/
extern void DisplayHandle(U16 handle);
extern int MemoryType(U16 handle);
extern void InitMemory(void);
extern void ExitCheck(void);
extern U16 MemoryAlloc(U16 size, long count, int stored_at);
extern void MemoryRelease(U16 handle);
extern int MoveToMemory(BYTE *buffer,U16 size,long count,long offset,U16 handle);
extern int MoveFromMemory(BYTE *buffer,U16 size,long count,long offset,U16 handle);
extern int SetMemory(int value,U16 size,long count,long offset,U16 handle);

/*  soi -- C file prototypes */

extern void soi(void);
extern void soi_ldbl(void);

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
extern int expand_dirname(char *dirname,char *drive);
extern int abortmsg(char *file, unsigned int line, int flags, char *msg);
#define ABORT(flags_,msg_) abortmsg(__FILE__, __LINE__, flags_, msg_)

extern long stackavail();

extern int getcolor(int, int);
extern int out_line(BYTE *, int);
extern void setvideomode(int, int, int, int);
extern int pot_startdisk(void);
extern void putcolor_a(int, int, int);
extern int startdisk(void);

#endif
