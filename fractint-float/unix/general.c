/* generalasm.c
 * This file contains routines to replace general.asm.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */

#include <string.h>
#ifndef NOBSTRING
#ifndef sun
/* If this gives you an error, read the README and modify the Makefile. */
#include <bstring.h>
#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>

#include "port.h"
#include "prototyp.h"
int overflow = 0;

int boxx[2304], boxy[1024];
int boxvalues[512];
char tstack[4096];
BYTE dacbox[256][3];
BYTE olddacbox[256][3];

extern int tabmode;

int DivideOverflow = 0;
int cpu=0;		/* cpu type: 86, 186, 286, or 386 */
int fpu=0;		/* fpu type: 0, 87, 287, 387 */

SEGTYPE extraseg=0;		/* extra 64K segment (allocated by init) */
/* ********************** Mouse Support Variables ************************** */

int lookatmouse=0;	/* see notes at mouseread routine */
long savebase=0;		/* base clock ticks */ 
long saveticks=0;	/* save after this many ticks */ 
int finishrow=0;	/* save when this row is finished */

int inside_help = 0;

extern int slides;	/* 1 for playback */

unsigned int toextra(tooffset, fromaddr, fromcount)
unsigned int tooffset;
char *fromaddr;
int fromcount;
{
    bcopy(fromaddr,(char *)(extraseg+tooffset),fromcount);
    return tooffset;
}

unsigned int fromextra(fromoffset, toaddr, tocount)
unsigned int fromoffset;
char *toaddr;
int tocount;
{
    bcopy((char *)(extraseg+fromoffset),toaddr,tocount);
    return fromoffset;
}

unsigned int
cmpextra(cmpoffset,cmpaddr,cmpcount)
unsigned int cmpoffset;
char *cmpaddr;
int cmpcount;
{
    return bcmp((char *)(extraseg+cmpoffset),cmpaddr,cmpcount);
}

/*
; ****************** Function initasmvars() *****************************
*/
void
initasmvars(void)
{
    if (cpu!=0) return;
    overflow = 0;
    extraseg = malloc(0x18000);

    /* set cpu type */
    cpu = 1;

    /* set fpu type */
   /* not needed, set fpu in sstools.ini */
}

void fpe_handler(int signum)
{
    overflow = 1;
}

/*
;
;       32-bit integer multiply routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with overflow = 1;
;
;       long x, y, z, multiply();
;       int n;
;
;       z = multiply(x,y,n)
;
*/

/*
 * 32 bit integer multiply with n bit shift.
 * Note that we fake integer multiplication with floating point
 * multiplication.
 */
long
multiply(x, y, n)
long x,y;
int n;
{
    register long l;
    l = ((float)x)* ((float)y)/(float)(1<<n);
    if (l==0x7fffffff) {
	overflow = 1;
    }
    return l;
}

/*
;
;       32-bit integer divide routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with overflow = 1;
;
;       z = divide(x,y,n);       z = x / y;
*/
long
divide(x,y,n)
long x,y;
int n;
{
    return (long) ( ((float)x)/ ((float)y)*(float)(1<<n));
}

/*
; ****************** Function getakey() *****************************
; **************** Function keypressed() ****************************

;       'getakey()' gets a key from either a "normal" or an enhanced
;       keyboard.   Returns either the vanilla ASCII code for regular
;       keys, or 1000+(the scan code) for special keys (like F1, etc)
;       Use of this routine permits the Control-Up/Down arrow keys on
;       enhanced keyboards.
;
;       The concept for this routine was "borrowed" from the MSKermit
;       SCANCHEK utility
;
;       'keypressed()' returns a zero if no keypress is outstanding,
;       and the value that 'getakey()' will return if one is.  Note
;       that you must still call 'getakey()' to flush the character.
;       As a sidebar function, calls 'help()' if appropriate, or
;       'tab_display()' if appropriate.
;       Think of 'keypressed()' as a super-'kbhit()'.
*/
int keybuffer = 0;

int getkeynowait(void);
int getkeyint(int);

int
keypressed(void) {
    int ch;
    ch = getkeynowait();
    if (!ch) return 0;
    keybuffer = ch;
    if (ch==F1 && helpmode) {
	keybuffer = 0;
	inside_help = 1;
	help(0);
	inside_help = 0;
	return 0;
    } else if (ch==TAB && tabmode) {
	keybuffer = 0;
	tab_display();
	return 0;
    }
    return ch;
}

/* Wait for a key.
 * This should be used instead of:
 * while (!keypressed()) {}
 * If timeout=1, waitkeypressed will time out after .5 sec.
 */
int
waitkeypressed(timeout)
int timeout;
{
    while (!keybuffer) {
	keybuffer = getkeyint(1);
	if (timeout) break;
    }
    return keypressed();
}

/*
 * This routine returns a key, ignoring F1
 */
int
getakeynohelp(void) {
    int ch;
    while (1) {
	ch = getakey();
	if (ch != F1) break;
    }
    return ch;
}
/*
 * This routine returns a keypress
 */
int
getakey(void)
{
    int ch;

    do {
	ch = getkeyint(1);
    } while (ch==0);
    return ch;
}

/*
 * This routine returns the current key, or 0.
 */
int
getkeynowait(void) {
    return getkeyint(0);
}

/*
 * This is the low level key handling routine.
 * If block is set, we want to block before returning, since we are waiting
 * for a key press.
 * We also have to handle the slide file, etc.
 */

int
getkeyint(block)
int block;
{
    int ch;
    int curkey;
    if (keybuffer) {
	ch = keybuffer;
	keybuffer = 0;
	return ch;
    }
    curkey = xgetkey(0);
    if (slides==1 && curkey == ESC) {
	stopslideshow();
	return 0;
    }

    if (curkey==0 && slides==1) {
	curkey = slideshw();
    }

    if (curkey==0 && block) {
	curkey = xgetkey(1);
	if (slides==1 && curkey == ESC) {
	    stopslideshow();
	    return 0;
	}
    }

    if (curkey && slides==2) {
	recordshw(curkey);
    }

    return curkey;
}

/*
; ****************** Function buzzer(int buzzertype) *******************
;
;       Sound a tone based on the value of the parameter
;
;       0 = normal completion of task
;       1 = interrupted task
;       2 = error contition

;       "buzzer()" codes:  strings of two-word pairs
;               (frequency in cycles/sec, delay in milliseconds)
;               frequency == 0 means no sound
;               delay     == 0 means end-of-tune
*/
void
buzzer(buzzertype)
int buzzertype;
{
    if ((soundflag & 7) != 0) {
        printf("\007");
        fflush(stdout);
    }
    if (buzzertype==0) {
        redrawscreen();
    }
}

/*
; ***************** Function delay(int delaytime) ************************
;
;       performs a delay loop for 'delaytime' milliseconds
*/
void
delay(delaytime)
int delaytime;
{
    static struct timeval delay;
    delay.tv_sec = delaytime/1000;
    delay.tv_usec = (delaytime%1000)*1000;
#if defined( __SVR4) || defined(LINUX)
    (void) select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &delay);
#else
    (void) select(0, (int *) 0, (int *) 0, (int *) 0, &delay);
#endif
}

/*
; ************** Function tone(int frequency,int delaytime) **************
;
;       buzzes the speaker with this frequency for this amount of time
*/
void
tone(frequency, delaytime)
int frequency, delaytime;
{
}

/*
; ************** Function snd(int hertz) and nosnd() **************
;
;       turn the speaker on with this frequency (snd) or off (nosnd)
;
; *****************************************************************
*/
void
snd(hertz)
int hertz;
{
}

void
nosnd(void)
{}

/*
; long readticker() returns current bios ticker value
*/
long
readticker(void)
{
    return clock_ticks();
}

/*
; ************************* Far Segment RAM Support **************************
;
;
;       farptr = (char far *)farmemalloc(long bytestoalloc);
;       (void)farmemfree(farptr);
*/

VOIDPTR 
farmemalloc(len)
long len;
{
    return (VOIDPTR )malloc((unsigned)len);
}

void
farmemfree(addr)
VOIDPTR addr;
{
    free((char *)addr);
}

void erasesegment(segaddress,segvalue)
int segaddress;
int segvalue;
{
}


int
farread(handle, buf, len)
int handle;
VOIDPTR buf;
unsigned len;
{
    return read(handle, buf, len);
}

int
farwrite(handle, buf, len)
int handle;
VOIDPTR buf;
unsigned len;
{
    return write(handle,buf,len);
}


long
normalize(ptr)
char *ptr;
{
    return (long) ptr;
}

/*
; *************** Far string/memory functions *********
*/
int
far_strlen (a)
char *a;
{
    return strlen(a);
}


void
far_strcpy (a,b)
char *a,*b;
{
    strcpy(a,b);
}

int
far_strcmp (a,b)
char *a, *b;
{
    return strcmp(a,b);
}

int
far_stricmp(a,b)
char *a,*b;
{
   return stricmp(a,b);
}

int
far_strnicmp(a,b,n)
char *a,*b;
int n;
{
    return strnicmp(a,b,n);
}

void
far_strcat (a,b)
char *a,*b;
{
    strcat(a,b);
}

void
far_memset ( a,c,len)
VOIDFARPTR a;
int c;
unsigned int len;
{
    memset(a,c,len);
}

void
far_memcpy ( a,b,len)
VOIDFARPTR a,b;
int len;
{
    memcpy(a,b,len);
}

int
far_memcmp (a,b,len)
VOIDFARPTR a,b;
int len;
{
    return memcmp(a,b,len);
}

void
far_memicmp(a,b,len)
VOIDFARPTR a,b;
int len;
{
    memicmp(a,b,len);
}

/* --------------------------------------------------------------------
 * The following routines are used for encoding/decoding gif images.
 * If we aren't on a PC, things are rough for decoding the fractal info
 * structure in the GIF file.  These routines look after converting the
 * MS_DOS format data into a form we can use.
 * If dir==0, we convert to MSDOS form.  Otherwise we convert from MSDOS.
 */

static void getChar(), getInt(), getLong(), getFloat(), getDouble();

void
decode_fractal_info(info,dir)
    struct fractal_info *info;
    int dir;
{
    unsigned char *buf;
    unsigned char *bufPtr;
    int i;

    if (dir==1) {
	buf = (unsigned char *)malloc(FRACTAL_INFO_SIZE);
	bufPtr = buf;
	bcopy((char *)info,(char *)buf,FRACTAL_INFO_SIZE);
    }  else {
	buf = (unsigned char *)malloc(sizeof(struct fractal_info));
	bufPtr = buf;
	bcopy((char *)info,(char *)buf,sizeof(struct fractal_info));
    }

    if (dir==1) {
	strncpy(info->info_id,(char *)bufPtr,8);
    } else {
	strncpy((char *)bufPtr,info->info_id,8);
    }
    bufPtr += 8;
    getInt(&info->iterationsold,&bufPtr,dir);
    getInt(&info->fractal_type,&bufPtr,dir);
    getDouble(&info->xmin,&bufPtr,dir);
    getDouble(&info->xmax,&bufPtr,dir);
    getDouble(&info->ymin,&bufPtr,dir);
    getDouble(&info->ymax,&bufPtr,dir);
    getDouble(&info->creal,&bufPtr,dir);
    getDouble(&info->cimag,&bufPtr,dir);
    getInt(&info->videomodeax,&bufPtr,dir);
    getInt(&info->videomodebx,&bufPtr,dir);
    getInt(&info->videomodecx,&bufPtr,dir);
    getInt(&info->videomodedx,&bufPtr,dir);
    getInt(&info->dotmode,&bufPtr,dir);
    getInt(&info->xdots,&bufPtr,dir);
    getInt(&info->ydots,&bufPtr,dir);
    getInt(&info->colors,&bufPtr,dir);
    getInt(&info->version,&bufPtr,dir);
    getFloat(&info->parm3,&bufPtr,dir);
    getFloat(&info->parm4,&bufPtr,dir);
    getFloat(&info->potential[0],&bufPtr,dir);
    getFloat(&info->potential[1],&bufPtr,dir);
    getFloat(&info->potential[2],&bufPtr,dir);
    getInt(&info->rseed,&bufPtr,dir);
    getInt(&info->rflag,&bufPtr,dir);
    getInt(&info->biomorph,&bufPtr,dir);
    getInt(&info->inside,&bufPtr,dir);
    getInt(&info->logmap,&bufPtr,dir);
    getFloat(&info->invert[0],&bufPtr,dir);
    getFloat(&info->invert[1],&bufPtr,dir);
    getFloat(&info->invert[2],&bufPtr,dir);
    getInt(&info->decomp[0],&bufPtr,dir);
    getInt(&info->decomp[1],&bufPtr,dir);
    getInt(&info->symmetry,&bufPtr,dir);
    for (i=0;i<16;i++) {
	getInt(&info->init3d[i],&bufPtr,dir);
    }
    getInt(&info->previewfactor,&bufPtr,dir);
    getInt(&info->xtrans,&bufPtr,dir);
    getInt(&info->ytrans,&bufPtr,dir);
    getInt(&info->red_crop_left,&bufPtr,dir);
    getInt(&info->red_crop_right,&bufPtr,dir);
    getInt(&info->blue_crop_left,&bufPtr,dir);
    getInt(&info->blue_crop_right,&bufPtr,dir);
    getInt(&info->red_bright,&bufPtr,dir);
    getInt(&info->blue_bright,&bufPtr,dir);
    getInt(&info->xadjust,&bufPtr,dir);
    getInt(&info->eyeseparation,&bufPtr,dir);
    getInt(&info->glassestype,&bufPtr,dir);
    getInt(&info->outside,&bufPtr,dir);
    getDouble(&info->x3rd,&bufPtr,dir);
    getDouble(&info->y3rd,&bufPtr,dir);
    getChar(&info->stdcalcmode,&bufPtr,dir);
    getChar(&info->useinitorbit,&bufPtr,dir);
    getInt(&info->calc_status,&bufPtr,dir);
    getLong(&info->tot_extend_len,&bufPtr,dir);
    getInt(&info->distest,&bufPtr,dir);
    getInt(&info->floatflag,&bufPtr,dir);
    getInt(&info->bailoutold,&bufPtr,dir);
    getLong(&info->calctime,&bufPtr,dir);
    for (i=0;i<4;i++) {
	getChar(&info->trigndx[i],&bufPtr,dir);
    }
    getInt(&info->finattract,&bufPtr,dir);
    getDouble(&info->initorbit[0],&bufPtr,dir);
    getDouble(&info->initorbit[1],&bufPtr,dir);
    getInt(&info->periodicity,&bufPtr,dir);
    getInt(&info->pot16bit,&bufPtr,dir);
    getFloat(&info->faspectratio,&bufPtr,dir);
    getInt(&info->system,&bufPtr,dir);
    getInt(&info->release,&bufPtr,dir);
    getInt(&info->flag3d,&bufPtr,dir);
    getInt(&info->transparent[0],&bufPtr,dir);
    getInt(&info->transparent[1],&bufPtr,dir);
    getInt(&info->ambient,&bufPtr,dir);
    getInt(&info->haze,&bufPtr,dir);
    getInt(&info->randomize,&bufPtr,dir);
    getInt(&info->rotate_lo,&bufPtr,dir);
    getInt(&info->rotate_hi,&bufPtr,dir);
    getInt(&info->distestwidth,&bufPtr,dir);
    getDouble(&info->dparm3,&bufPtr,dir);
    getDouble(&info->dparm4,&bufPtr,dir);
    getInt(&info->fillcolor,&bufPtr,dir);
    getDouble(&info->mxmaxfp,&bufPtr,dir);
    getDouble(&info->mxminfp,&bufPtr,dir);
    getDouble(&info->mymaxfp,&bufPtr,dir);
    getDouble(&info->myminfp,&bufPtr,dir);
    getInt(&info->zdots,&bufPtr,dir);
    getFloat(&info->originfp,&bufPtr,dir);
    getFloat(&info->depthfp,&bufPtr,dir);
    getFloat(&info->heightfp,&bufPtr,dir);
    getFloat(&info->widthfp,&bufPtr,dir);
    getFloat(&info->distfp,&bufPtr,dir);
    getFloat(&info->eyesfp,&bufPtr,dir);
    getInt(&info->orbittype,&bufPtr,dir);
    getInt(&info->juli3Dmode,&bufPtr,dir);
    getInt(&info->maxfn,&bufPtr,dir);
    getInt(&info->inversejulia,&bufPtr,dir);
    getDouble(&info->dparm5,&bufPtr,dir);
    getDouble(&info->dparm6,&bufPtr,dir);
    getDouble(&info->dparm7,&bufPtr,dir);
    getDouble(&info->dparm8,&bufPtr,dir);
    getDouble(&info->dparm9,&bufPtr,dir);
    getDouble(&info->dparm10,&bufPtr,dir);
    getLong(&info->bailout,&bufPtr,dir);  
    getInt(&info->bailoutest,&bufPtr,dir);
    getLong(&info->iterations,&bufPtr,dir);
    getInt(&info->bf_math,&bufPtr,dir);
    getInt(&info->bflength,&bufPtr,dir);
    getInt(&info->yadjust,&bufPtr,dir); 
    getInt(&info->old_demm_colors,&bufPtr,dir);
    getLong(&info->logmap,&bufPtr,dir);
    getLong(&info->distest,&bufPtr,dir);
    getDouble(&info->dinvert[0],&bufPtr,dir);
    getDouble(&info->dinvert[1],&bufPtr,dir);
    getDouble(&info->dinvert[2],&bufPtr,dir);
    getInt(&info->logcalc,&bufPtr,dir);
    getInt(&info->stoppass,&bufPtr,dir);
    getInt(&info->quick_calc,&bufPtr,dir);
    getDouble(&info->closeprox,&bufPtr,dir);
    getInt(&info->nobof,&bufPtr,dir);
    getLong(&info->orbit_interval,&bufPtr,dir);

    for (i=0;i<(sizeof(info->future)/sizeof(short));i++) {
        getInt(&info->future[i],&bufPtr,dir);
    }   
    if (bufPtr-buf != FRACTAL_INFO_SIZE) {
	printf("Warning: loadfile miscount on fractal_info structure.\n");
	printf("Components add up to %d bytes, but FRACTAL_INFO_SIZE = %d\n",
		bufPtr-buf, FRACTAL_INFO_SIZE);
    } 
    if (dir==0) {
	bcopy((char *)buf,(char *)info,FRACTAL_INFO_SIZE);
    }

    free(buf);
}

/*
 * This routine gets a char out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getChar(dst,src,dir)
    unsigned char *dst;
    unsigned char **src;
    int dir;
{
    if (dir==1) {
	*dst = **src;
    } else {
	**src = *dst;
    }
    (*src)++;
}

/*
 * This routine gets an int out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getInt(dst,src,dir)
    short *dst;
    unsigned char **src;
    int dir;
{
    if (dir==1) {
	*dst = (*src)[0] + ((((char *)(*src))[1])<<8);
    } else {
	(*src)[0] = (*dst)&0xff;
	(*src)[1] = ((*dst)&0xff00)>>8;
    }
    (*src) += 2; /* sizeof(int) in MS_DOS */
}

/*
 * This routine gets a long out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getLong(dst,src,dir)
    long *dst;
    unsigned char **src;
    int dir;
{
    if (dir==1) {
	*dst = ((unsigned long)((*src)[0])) +
	    (((unsigned long)((*src)[1]))<<8) +
	    (((unsigned long)((*src)[2]))<<16) +
	    (((long)(((char *)(*src))[3]))<<24);
    } else {
	(*src)[0] = (*dst)&0xff;
	(*src)[1] = ((*dst)&0xff00)>>8;
	(*src)[2] = ((*dst)&0xff0000)>>16;
#ifdef __SVR4
	(*src)[3] = (unsigned)((*dst)&0xff000000)>>24;
#else
	(*src)[3] = ((*dst)&0xff000000)>>24;
#endif
    }
    (*src) += 4; /* sizeof(long) in MS_DOS */
}

#define P4 16.
#define P7 128.
#define P8 256.
#define P12 4096.
#define P15 32768.
#define P20 1048576.
#define P23 8388608.
#define P28 268435456.
#define P36 68719476736.
#define P44 17592186044416.
#define P52 4503599627370496.


/*
 * This routine gets a double out of the buffer, or puts a double into the
 * buffer;
 * It updates the buffer pointer accordingly.
 */
static void getDouble(dst,src,dir)
    double *dst;
    unsigned char **src;
    int dir;
{
    int e;
    double f;
    int i;
    if (dir==1) {
	for (i=0;i<8;i++) {
	    if ((*src)[i] != 0) break;
	}
	if (i==8) {
	    *dst = 0;
	} else {
#ifdef __SVR4
	    e = (((*src)[7]&0x7f)<<4) + ((int)((*src)[6]&0xf0)>>4) - 1023;
	    f = 1 + (int)((*src)[6]&0x0f)/P4 + (int)((*src)[5])/P12 +
        (int)((*src)[4])/P20 + (int)((*src)[3])/P28 + (int)((*src)[2])/P36 +
        (int)((*src)[1])/P44 + (int)((*src)[0])/P52;
#else
      e = (((*src)[7]&0x7f)<<4) + (((*src)[6]&0xf0)>>4) - 1023;
      f = 1 + ((*src)[6]&0x0f)/P4 + (*src)[5]/P12 + (*src)[4]/P20 +
    (*src)[3]/P28 + (*src)[2]/P36 + (*src)[1]/P44 + (*src)[0]/P52;
#endif
	    f *= pow(2.,(double)e);
	    if ((*src)[7]&0x80) {
		f = -f;
	    }
	    *dst = f;
	}
    } else {
	if (*dst==0) {
	    bzero((char *)(*src),8);
	} else {
	    int s=0;
	    f = *dst;
	    if (f<0) {
		s = 0x80;
		f = -f;
	    }
	    e = log(f)/log(2.);
	    f = f/pow(2.,(double)e) - 1;
	    if (f<0) {
		e--;
		f = (f+1)*2-1;
	    } else if (f>=1) {
		e++;
		f = (f+1)/2-1;
	    }
	    e += 1023;
	    (*src)[7] = s | ((e&0x7f0)>>4);
	    f *= P4;
	    (*src)[6] = ((e&0x0f)<<4) | (((int)f)&0x0f);
	    f = (f-(int)f)*P8;
	    (*src)[5] = (((int)f)&0xff);
	    f = (f-(int)f)*P8;
	    (*src)[4] = (((int)f)&0xff);
	    f = (f-(int)f)*P8;
	    (*src)[3] = (((int)f)&0xff);
	    f = (f-(int)f)*P8;
	    (*src)[2] = (((int)f)&0xff);
	    f = (f-(int)f)*P8;
	    (*src)[1] = (((int)f)&0xff);
	    f = (f-(int)f)*P8;
	    (*src)[0] = (((int)f)&0xff);
	}
    }
    *src += 8; /* sizeof(double) in MSDOS */
}

/*
 * This routine gets a float out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getFloat(dst,src,dir)
    float *dst;
    unsigned char **src;
    int dir;
{
    int e;
    double f;
    int i;
    if (dir==1) {
	for (i=0;i<4;i++) {
	    if ((*src)[i] != 0) break;
	}
	if (i==4) {
	    *dst = 0;
	} else {
#ifdef __SVR4
	    e = ((((*src)[3]&0x7f)<<1) | ((int)((*src)[2]&0x80)>>7)) - 127;
	    f = 1 + (int)((*src)[2]&0x7f)/P7 + (int)((*src)[1])/P15 + (int)((*src)[0])/P23;
#else
      e = ((((*src)[3]&0x7f)<<1) | (((*src)[2]&0x80)>>7)) - 127;
      f = 1 + ((*src)[2]&0x7f)/P7 + (*src)[1]/P15 + (*src)[0]/P23;
#endif
	    f *= pow(2.,(double)e);
	    if ((*src)[3]&0x80) {
		f = -f;
	    }
	    *dst = f;
	}
    } else {
	if (*dst==0) {
	    bzero((char *)(*src),4);
	} else {
	    int s=0;
	    f = *dst;
	    if (f<0) {
		s = 0x80;
		f = -f;
	    }
	    e = log(f)/log(2.);
	    f = f/pow(2.,(double)e) - 1;
	    if (f<0) {
		e--;
		f = (f+1)*2-1;
	    } else if (f>=1) {
		e++;
		f = (f+1)/2-1;
	    }
	    e += 127;
	    (*src)[3] = s | ((e&0xf7)>>1);
	    f *= P7;
	    (*src)[2] = ((e&0x01)<<7) | (((int)f)&0x7f);
	    f = (f-(int)f)*P8;
	    (*src)[1] = (((int)f)&0xff);
	    f = (f-(int)f)*P8;
	    (*src)[0] = (((int)f)&0xff);
	}
    }
    *src += 4; /* sizeof(float) in MSDOS */
}

/*
 * Fix up the ranges data.
 */
void
fix_ranges(ranges,num,dir)
    int *ranges, num;
    int dir;
{
    unsigned char *buf;
    unsigned char *bufPtr;
    int i;

    if (dir==1) {
	buf = (unsigned char *)malloc(num*2);
	bufPtr = buf;
	bcopy((char *)ranges, (char *)buf, num*2);
    } else {
	buf = (unsigned char *)malloc(num*sizeof(int));
	bufPtr = buf;
	bcopy((char *)ranges, (char *)buf, num*sizeof(int));
    }
    for (i=0;i<num;i++) {
	getInt(&ranges[i],&bufPtr,dir);
    }
    free((char *)buf);
}

void
decode_evolver_info(info,dir)
    struct evolution_info *info;
    int dir;
{
    unsigned char *buf;
    unsigned char *bufPtr;
    int i;

    if (dir==1) {
	buf = (unsigned char *)malloc(EVOLVER_INFO_SIZE);
	bufPtr = buf;
	bcopy((char *)info,(char *)buf,EVOLVER_INFO_SIZE);
    }  else {
	buf = (unsigned char *)malloc(sizeof(struct evolution_info));
	bufPtr = buf;
	bcopy((char *)info,(char *)buf,sizeof(struct evolution_info));
    }

    getInt(&info->evolving,&bufPtr,dir);
    getInt(&info->gridsz,&bufPtr,dir);
    getInt(&info->this_gen_rseed,&bufPtr,dir);
    getDouble(&info->fiddlefactor,&bufPtr,dir);
    getDouble(&info->paramrangex,&bufPtr,dir);
    getDouble(&info->paramrangey,&bufPtr,dir);
    getDouble(&info->opx,&bufPtr,dir);
    getDouble(&info->opy,&bufPtr,dir);
    getInt(&info->odpx,&bufPtr,dir);
    getInt(&info->odpy,&bufPtr,dir);
    getInt(&info->px,&bufPtr,dir);
    getInt(&info->py,&bufPtr,dir);
    getInt(&info->sxoffs,&bufPtr,dir);
    getInt(&info->syoffs,&bufPtr,dir);
    getInt(&info->xdots,&bufPtr,dir);
    getInt(&info->ydots,&bufPtr,dir);
    for (i=0;i<NUMGENES;i++) {
        getInt(&info->mutate[i],&bufPtr,dir);
    }
    getInt(&info->ecount,&bufPtr,dir);

    for (i=0;i<(sizeof(info->future)/sizeof(short));i++) {
        getInt(&info->future[i],&bufPtr,dir);
    }   
    if (bufPtr-buf != EVOLVER_INFO_SIZE) {
	printf("Warning: loadfile miscount on evolution_info structure.\n");
	printf("Components add up to %d bytes, but EVOLVER_INFO_SIZE = %d\n",
		bufPtr-buf, EVOLVER_INFO_SIZE);
    } 
    if (dir==0) {
	bcopy((char *)buf,(char *)info,EVOLVER_INFO_SIZE);
    }

    free(buf);
}

