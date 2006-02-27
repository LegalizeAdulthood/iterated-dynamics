/*
	"Stubbed-Off" Routines and variables
	which exist only in Fractint for DOS
*/

#include "port.h"
#include "prototyp.h"

/* not-yet-implemented variables */

double sxmin, sxmax, sx3rd, symin, symax, sy3rd;
int color_bright = 15;
int color_medium = 7;
int color_dark = 0;
int hasinverse = 0;
char diskfilename[] = {"FRACTINT.$$$"};
BYTE *line_buff;
char *fract_dir1=".", *fract_dir2=".";
int Printer_Compress;
int keybuffer;
int LPTnumber;
int ColorPS;
int Print_To_File;
int Printer_Type;
int Printer_CRLF;
int Printer_Resolution;
int Printer_Titleblock;
int Printer_ColorXlat;
int Printer_BAngle;
int Printer_GAngle;
int Printer_SAngle;
int Printer_RAngle;
int Printer_RFrequency;
int Printer_GFrequency;
int Printer_BFrequency;
int Printer_SFrequency;
int Printer_SetScreen;
int Printer_RStyle;
int Printer_GStyle;
int Printer_BStyle;
int Printer_SStyle;
int EPSFileType;
int integerfractal;
int video_type;
int adapter;
int usr_periodicitycheck;
int active_system = WINFRAC;	/* running under windows */
char busy;
int mode7text;
int textsafe;
long calctime;
char stdcalcmode;
int compiled_by_turboc = 0;
int tabmode;
double plotmx1, plotmx2, plotmy1, plotmy2;
int vesa_detect;
long creal, cimag;
int TranspSymmetry;
long fudge;
long l_at_rad;		/* finite attractor radius  */
double f_at_rad;		/* finite attractor radius  */
int timedsave = 0;
int made_dsktemp = 0;
int reallyega = 0;
int started_resaves = 0;
float viewreduction=1;
int viewxdots=0,viewydots=0;
char usr_floatflag;
int disk16bit = 0;
double potparam[3];
int gotrealdac = 1;
int svga_type = 0;
int viewcrop = 1;
int viewwindow = 0;

int video_cutboth = 0;          /* nonzero to keep virtual aspect */
int zscroll = 0;                /* screen/zoombox 0 fixed, 1 relaxed */
int video_startx = 0;
int video_starty = 0;
int vesa_xres = 0;
int vesa_yres = 0;
int video_vram = 0;
int virtual = 0;
int istruecolor = 0;
int video_scroll = 0;

int fm_attack;
int fm_decay;
int fm_sustain;
int fm_release;
int fm_vol;
int fm_wavetype;
int polyphony;
int hi_atten;

U16 evolve_handle = 0;
int disktarga = 0;

int boxcolor;
int chkd_vvs = 0;

int max_colors;                         /* maximum palette size */
int        zoomoff;                     /* = 0 when zoom is disabled    */
int        savedac;                     /* save-the-Video DAC flag      */
int browsing;                 /* browse mode flag */
char file_name_stack[16][13]; /* array of file names used while browsing */
int name_stack_ptr ;
double toosmall;
int  minbox;
int no_sub_images;
int autobrowse,doublecaution;
char brwscheckparms,brwschecktype;
char browsemask[13];
int scale_map[12] = {1,2,3,4,5,6,7,8,9,10,11,12}; /*RB, array for mapping notes to a (user defined) scale */

/* fake/not-yet-implemented subroutines */

void rotate(int x) {}
void find_special_colors(void) {}
int spawnl(int dummy1, char *dummy2, char *dummy3) {return 0;}
int showtempmsg(char far *foo) {return 1;}
void cleartempmsg(void) {}
void freetempmsg(void) {}
int FromMemDisk(long offset, int size, void far *src) {return 0;}
int ToMemDisk(long offset, int size, void far *src) {return 0;}
int  common_startdisk(long newrowsize, long newcolsize, int colors) {return 0;}
long cdecl normalize(char far *foo) {return 0;}
void drawbox(int foo) {}

void farmessage(unsigned char far *foo) {}
void setvideomode(int foo1, int foo2, int foo3, int foo4) {}
int fromvideotable(void) {return 0;}
void home(void) {}

int intro_overlay(void) {return 0;}
int rotate_overlay(void) {return 0;}
int printer_overlay(void) {return 0;}
int pot_startdisk(void) {return 0;}
void SetTgaColors(void) {}
int startdisk(void) {return 0;}
void enddisk(void) {}
int readdisk(unsigned int foo1,unsigned int foo2) {return 0;}
void writedisk(unsigned int foo1,unsigned int foo2,unsigned int foo3) {}
int targa_startdisk(FILE *foo1,int foo2){return 0;}
void targa_writedisk(unsigned int foo1,unsigned int foo2,BYTE foo3,BYTE foo4,BYTE foo5){}
void targa_readdisk(unsigned int foo1,unsigned int foo2,BYTE *foo3,BYTE *foo4,BYTE *foo5){}
int SetColorPaletteName(char *foo1) {return 0;}
BYTE far *findfont(int foo1) {return(0);}
long cdecl readticker(void){return(0);}
void EndTGA(void){}

int key_count(int keynum) {return 0;}

void dispbox(void) {}
void clearbox(void) {}
void _fastcall addbox(struct coords point) {}
void _fastcall drawlines(struct coords fr, struct coords to, int dx, int dy) {}
int showvidlength(void) {return 0;}

/* sound.c file prototypes */
int get_sound_params(void) {return(0);}

int soundon(int i) {return(0);}

void soundoff(void) {}

int initfm(void) {return(0);}

void mute(void) {}

void dvid_status(int foo1, char far *foo2){}
int tovideotable(void){return 0;}

void TranspPerPixel(void){}

void stopslideshow(void) {}
void aspectratio_crop(float foo1, float foo2) {}
void setvideotext(void) {}
int load_fractint_cfg(int foo1) {return 0;}
