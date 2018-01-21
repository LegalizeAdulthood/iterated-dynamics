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
extern void put_line(int row, int startcol, int stopcol, BYTE const *pixels);
extern void get_line(int row, int startcol, int stopcol, BYTE *pixels);
extern void find_special_colors();
extern int getakeynohelp();
extern long readticker();
extern int get_sound_params();
extern void setnullvideo();
// msccos -- C file prototypes
extern double _cos(double);
// parser -- C file prototypes
struct fn_operand
{ // function, load, store pointers
    void (*function)();
    Arg *operand;
};
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
extern void NotAFnct();
extern void FnctNotFound();
extern int CvtStk();
extern int fFormula();
extern void RecSortPrec();
extern int Formula();
extern int BadFormula();
extern int form_per_pixel();
extern int frm_get_param_stuff(char const *Name);
extern bool RunForm(char const *Name, bool from_prompts1c);
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
    char const *hdg,
    int numprompts,
    char const **prompts,
    fullscreenvalues *values,
    int fkeymask,
    char *extrainfo);
extern long get_file_entry(int type, char const *title, char const *fmask,
                    char *filename, char *entryname);
extern long get_file_entry(int type, char const *title, char const *fmask,
                    std::string &filename, char *entryname);
extern long get_file_entry(int type, char const *title, char const *fmask,
                    std::string &filename, std::string &entryname);
extern int get_fracttype();
extern int get_fract_params(int);
extern int get_fract3d_params();
extern int get_3d_params();
extern int prompt_valuestring(char *buf, fullscreenvalues const *val);
extern void setbailoutformula(bailouts);
extern int find_extra_param(fractal_type type);
extern void load_params(fractal_type fractype);
extern bool check_orbit_name(char const *orbitname);
struct entryinfo;
extern int scan_entries(FILE *infile, struct entryinfo *ch, char const *itemname);
// prompts2 -- C file prototypes
extern int get_toggles();
extern int get_toggles2();
extern int passes_options();
extern int get_view_params();
extern int get_starfield_params();
extern int get_commands();
extern void goodbye();
extern bool isadirectory(char const *s);
extern bool getafilename(char const *hdg, char const *file_template, char *flname);
extern bool getafilename(char const *hdg, char const *file_template, std::string &flname);
extern int splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext);
extern int makepath(char *template_str, char const *drive, char const *dir, char const *fname, char const *ext);
extern int fr_findfirst(char const *path);
extern int fr_findnext();
extern void shell_sort(void *, int n, unsigned, int (*fct)(VOIDPTR, VOIDPTR));
extern void fix_dirname(char *dirname);
extern void fix_dirname(std::string &dirname);
extern int merge_pathnames(char *oldfullpath, char const *newfilename, cmd_file mode);
extern int merge_pathnames(std::string &oldfullpath, char const *newfilename, cmd_file mode);
extern int get_browse_params();
extern int get_cmd_string();
extern int get_rds_params();
extern int starfield();
extern int get_a_number(double *, double *);
extern int lccompare(VOIDPTR, VOIDPTR);
extern int dir_remove(char const *dir, char const *filename);
extern FILE *dir_fopen(char const *dir, char const *filename, char const *mode);
extern void extract_filename(char *target, char const *source);
extern std::string extract_filename(char const *source);
extern const char *has_ext(char const *source);
// realdos -- C file prototypes
extern int showvidlength();
extern bool stopmsg(int flags, char const* msg);
extern void blankrows(int, int, int);
extern int texttempmsg(char const *);
extern int fullscreen_choice(
    int options,
    char const *hdg,
    char const *hdg2,
    char const *instr,
    int numchoices,
    char const **choices,
    int *attributes,
    int boxwidth,
    int boxdepth,
    int colwidth,
    int current,
    void (*formatitem)(int, char*),
    char *speedstring,
    int (*speedprompt)(int row, int col, int vid, char const *speedstring, int speed_match),
    int (*checkkey)(int, int)
);
extern bool showtempmsg(char const *);
extern void cleartempmsg();
extern void helptitle();
extern int putstringcenter(int row, int col, int width, int attr, char const *msg);
extern int main_menu(int);
extern int input_field(int options, int attr, char *fld, int len, int row, int col,
    int (*checkkey)(int curkey));
extern int field_prompt(char const *hdg, char const *instr, char *fld, int len,
    int (*checkkey)(int curkey));
extern bool thinking(int options, char const *msg);
extern void discardgraphics();
extern void load_fractint_config();
extern int check_vidmode_key(int, int);
extern int check_vidmode_keyname(char const *kname);
extern void vidmode_keyname(int k, char *buf);
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
extern int handle_special_keys(int ch);
// stereo -- C file prototypes
extern bool do_AutoStereo();
extern int outline_stereo(BYTE *, int);
// testpt -- C file prototypes
extern int teststart();
extern void testend();
extern int testpt(double, double, double, double, long, int);
// zoom -- C file prototypes
extern void drawbox(bool draw_it);
extern void moveboxf(double, double);
extern void resizebox(int);
extern void chgboxi(int, int);
extern void zoomout();
extern void aspectratio_crop(float, float);
extern int init_pan_or_recalc(bool);
extern void drawlines(struct coords, struct coords, int, int);
extern void addbox(struct coords);
extern void clearbox();
extern void dispbox();
// fractalb.c -- C file prototypes
extern DComplex cmplxbntofloat(BNComplex *);
extern DComplex cmplxbftofloat(BFComplex *);
extern void comparevalues(char const *s, LDBL x, bn_t bnx);
extern void comparevaluesbf(char const *s, LDBL x, bf_t bfx);
extern void show_var_bf(char const *s, bf_t n);
extern void show_two_bf(char const *s1, bf_t t1, char const *s2, bf_t t2, int digits);
extern void bfcornerstofloat();
extern void showcornersdbl(char const *s);
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
/*
 *  uclock -- C file prototypes
 *  The  uclock_t typedef placed here because uclock.h
 *  prototype is for DOS version only.
 */
typedef unsigned long uclock_t;
extern uclock_t usec_clock();
extern void restart_uclock();
extern void wait_until(int index, uclock_t wait_time);
extern void init_failure(char const *message);
extern int expand_dirname(char *dirname, char *drive);
extern bool abortmsg(char const *file, unsigned int line, int flags, char const *msg);
#define ABORT(flags_, msg_) abortmsg(__FILE__, __LINE__, flags_, msg_)
extern long stackavail();
extern int getcolor(int, int);
extern int out_line(BYTE *, int);
extern void setvideomode(int, int, int, int);
extern int pot_startdisk();
extern void putcolor_a(int, int, int);
extern int startdisk();
#endif
