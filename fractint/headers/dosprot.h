#ifndef DOSPROT_H
#define DOSPROT_H

/* This file contains prototypes for dos specific functions. */


/*  calmanp5 -- assembler file prototypes */

extern long  cdecl calcmandfpasm_p5(void);
extern void cdecl calcmandfpasmstart_p5(void);

/*  general -- assembler file prototypes */

extern  long   cdecl multiply(long, long, int);
extern  long   cdecl divide(long, long, int);
extern  int    cdecl getakey(void);
/*extern  void   cdecl buzzer(int); */
extern  void   cdecl buzzerpcspkr(int);
extern  void   cdecl farmemfree(VOIDFARPTR );
extern  int    cdecl far_strlen( char far *);
extern  int    cdecl far_strnicmp(char far *, char far *,int);
extern  void   cdecl far_strcpy( char far *, char far *);
extern  int    cdecl far_strcmp( char far *, char far *);
extern  void   cdecl far_strcat( char far *, char far *);
extern  void   cdecl far_memset( VOIDFARPTR , int      , unsigned);
extern  void   cdecl far_memcpy( VOIDFARPTR , VOIDFARPTR , int);
extern  int    cdecl far_memcmp( VOIDFARPTR , VOIDFARPTR , int);
extern  BYTE far *cdecl emmquery(void);
extern  unsigned int cdecl emmgetfree(void);
extern  unsigned int cdecl emmallocate(unsigned int);
extern  void   cdecl emmdeallocate(unsigned int);
extern  void   cdecl emmgetpage(unsigned int, unsigned int);
extern  void   cdecl emmclearpage(unsigned int, unsigned int);
extern  unsigned int *cdecl xmmquery(void);
extern  unsigned int cdecl xmmlongest(void);
extern  unsigned int cdecl xmmfree(void);
extern  unsigned int cdecl xmmallocate(unsigned int);
extern  void   cdecl xmmdeallocate(unsigned int);
extern  unsigned int cdecl xmmreallocate(unsigned int, unsigned int);
extern  unsigned int cdecl xmmmoveextended(struct XMM_Move *);
extern  int    cdecl keypressed(void);
extern  long   cdecl readticker( void );
extern  void   cdecl snd( int );
extern  void   cdecl nosnd( void );
extern  void   cdecl initasmvars( void );

#ifndef __BORLANDC__
extern  void   cdecl enable( void );
extern  void   cdecl disable( void );
extern  void   cdecl delay( int );
#endif

extern  int    cdecl farread(int, VOIDFARPTR, unsigned);
extern  int    cdecl farwrite(int, VOIDFARPTR, unsigned);
extern  long   cdecl normalize(char far *);
extern  void   cdecl erasesegment(int, int);
extern  int    cdecl getakeynohelp( void );
extern  unsigned int cdecl cmpextra( unsigned int, char *, int );
extern  unsigned int cdecl fromextra( unsigned int, char *, int );
extern  unsigned int cdecl toextra( unsigned int, char *, int );
extern  void   cdecl load_mat(double (*)[4]);

/* sound.c file prototypes */
extern int get_sound_params(void);
extern void buzzer(int);
extern int soundon(int);
extern void soundoff(void);
extern int initfm(void);
extern void mute(void);

/*  tplus -- C file prototypes */

extern void WriteTPWord(unsigned int ,unsigned int );
extern void WriteTPByte(unsigned int ,unsigned int );
extern unsigned int ReadTPWord(unsigned int );
extern BYTE ReadTPByte(unsigned int );
extern void DisableMemory(void );
extern void EnableMemory(void );
extern int TargapSys(int ,unsigned int );
extern int _SetBoard(int );
extern int TPlusLUT(BYTE far *,unsigned int ,unsigned int ,unsigned int );
extern int SetVGA_LUT(void );
extern int SetColorDepth(int );
extern int SetBoard(int );
extern int ResetBoard(int );
extern int CheckForTPlus(void );
extern int SetTPlusMode(int ,int ,int ,int );
extern int FillTPlusRegion(unsigned int ,unsigned int ,unsigned int ,unsigned int ,unsigned long );
extern void BlankScreen(unsigned long );
extern void UnBlankScreen(void );
extern void EnableOverlayCapture(void );
extern void DisableOverlayCapture(void );
extern void ClearTPlusScreen(void );
extern int MatchTPlusMode(unsigned int ,unsigned int ,unsigned int ,unsigned int ,unsigned int );
extern void TPlusZoom(int );

/*  video -- assembler file prototypes */

extern void   cdecl adapter_detect(void);
extern void   cdecl scroll_center(int, int);
extern void   cdecl scroll_relative(int, int);
extern void   cdecl scroll_state(int);
extern void   cdecl setvideotext(void);
extern void   cdecl setnullvideo(void);
extern void   cdecl setfortext(void);
extern void   cdecl setforgraphics(void);
extern void   cdecl swapnormwrite(void);
extern void   cdecl setclear(void);
extern int    cdecl keycursor(int,int);
extern void   cdecl swapnormread(void);
extern void   cdecl setvideomode(int, int, int, int);
extern void   cdecl movewords(int,BYTE far*,BYTE far*);
extern void   cdecl movecursor(int, int);
extern void   cdecl get_line(int, int, int, BYTE *);
extern void   cdecl put_line(int, int, int, BYTE *);
extern void   cdecl setattr(int, int, int, int);
extern void   cdecl putstring(int,int,int,char far *);
extern void   cdecl spindac(int, int);
extern void   cdecl find_special_colors(void);
extern char   cdecl get_a_char(void);
extern void   cdecl put_a_char(int);
extern void   cdecl scrollup(int, int);
extern void   cdecl home(void);
extern BYTE far *cdecl  findfont(int);
extern int _fastcall getcolor(int, int);
extern void _fastcall putcolor_a(int, int, int);
extern void gettruecolor(int, int, int*, int*, int*);
extern void puttruecolor(int, int, int, int, int);
extern int  out_line(BYTE *, int);
extern void   (*swapsetup)(void);

#endif

