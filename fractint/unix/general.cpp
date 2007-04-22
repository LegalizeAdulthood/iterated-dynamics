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
#include "fihelp.h"

int g_overflow = 0;

int boxx[2304], boxy[1024];
int boxvalues[512];
BYTE g_dac_box[256][3];

int DivideOverflow = 0;
int cpu = 0;		/* cpu type: 86, 186, 286, or 386 */
int fpu = 0;		/* fpu type: 0, 87, 287, 387 */

/* ********************** Mouse Support Variables ************************** */

int g_look_at_mouse = LOOK_MOUSE_NONE;	/* see notes at mouseread routine */
int g_finish_row = 0;	/* save when this row is finished */

int inside_help = 0;

extern int g_slides;	/* 1 for playback */

int getakey(void);

/*
; ****************** Function initasmvars() *****************************
*/
void
initasmvars(void)
{
	if (cpu != 0)
	{
		return;
	}
	g_overflow = 0;

	/* set cpu type */
	cpu = 1;

	/* set fpu type */
	/* not needed, set fpu in sstools.ini */
}

void fpe_handler(int signum)
{
	g_overflow = 1;
}

/*
;
;       32-bit integer multiply routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with overflow = 1;
;
;       long x, y, z, multiply();
;       int n;
;
;       z = multiply(x, y, n)
;
*/

/*
 * 32 bit integer multiply with n bit shift.
 * Note that we fake integer multiplication with floating point
 * multiplication.
 */
long multiply(long x, long y, int n)
{
	register long l = (long) (((float) x)*((float) y)/(float) (1 << n));
	if (l == 0x7fffffff)
	{
		g_overflow = 1;
	}
	return l;
}

/*
;
;       32-bit integer divide routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with overflow = 1;
;
;       z = divide(x, y, n);       z = x / y;
*/
long divide(long x, long y, int n)
{
	return (long) (((float) x)/((float) y)*(float) (1 << n));
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

int keypressed(void)
{
	int ch;
	ch = getkeynowait();
	if (!ch)
	{
		return 0;
	}
	keybuffer = ch;
	if (ch == FIK_F1 && get_help_mode())
	{
		keybuffer = 0;
		inside_help = 1;
		help(0);
		inside_help = 0;
		return 0;
	}
	else if (ch == FIK_TAB && g_tab_mode)
	{
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
waitkeypressed(int timeout)
{
	while (!keybuffer)
	{
		keybuffer = getkeyint(1);
		if (timeout)
		{
			break;
		}
	}
	return keypressed();
}

/*
 * This routine returns a key, ignoring F1
 */
int getakeynohelp(void)
{
	int ch;
	while (1)
	{
		ch = getakey();
		if (ch != FIK_F1)
		{
			break;
		}
	}
	return ch;
}

/*
* This routine returns a keypress
*/
int getakey(void)
{
	int ch;

	do
	{
		ch = getkeyint(1);
	}
	while (ch == 0);
	return ch;
}

/*
 * This routine returns the current key, or 0.
 */
int getkeynowait(void)
{
	return getkeyint(0);
}

/*
 * This is the low level key handling routine.
 * If block is set, we want to block before returning, since we are waiting
 * for a key press.
 * We also have to handle the slide file, etc.
 */
int getkeyint(int block)
{
	int ch;
	int curkey;
	if (keybuffer)
	{
		ch = keybuffer;
		keybuffer = 0;
		return ch;
	}
	curkey = xgetkey(0);
	if (g_slides == SLIDES_PLAY && curkey == FIK_ESC)
	{
		stop_slide_show();
		return 0;
	}

	if (curkey == 0 && g_slides == SLIDES_PLAY)
	{
		curkey = slide_show();
	}

	if (curkey == 0 && block)
	{
		curkey = xgetkey(1);
		if (g_slides == SLIDES_PLAY && curkey == FIK_ESC)
		{
			stop_slide_show();
			return 0;
		}
	}

	if (curkey && g_slides == SLIDES_RECORD)
	{
		record_show(curkey);
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
void buzzer(int buzzertype)
{
	if ((g_sound_flags & 7) != 0)
	{
		printf("\007");
		fflush(stdout);
	}
	if (buzzertype == 0)
	{
		redrawscreen();
	}
}

/*
; ***************** Function delay(int delaytime) ************************
;
;       performs a delay loop for 'delaytime' milliseconds
*/
void delay(int delaytime)
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
; long readticker() returns current bios ticker value
*/
long readticker(void)
{
	return clock_ticks();
}

/* --------------------------------------------------------------------
 * The following routines are used for encoding/decoding gif images.
 * If we aren't on a PC, things are rough for decoding the fractal info
 * structure in the GIF file.  These routines look after converting the
 * MS_DOS format data into a form we can use.
 * If dir == 0, we convert to MSDOS form.  Otherwise we convert from MSDOS.
 */
static void getUChar(unsigned char *dst, unsigned char **src, int dir);
static void getChar(char *dst, unsigned char **src, int dir);
static void getShort(short *dst, unsigned char **src, int dir);
static void getUShort(unsigned short *dst, unsigned char **src, int dir);
static void getInt(int *dst, unsigned char **src, int dir);
static void getLong(long *dst, unsigned char **src, int dir);
static void getFloat(float *dst, unsigned char **src, int dir);
static void getDouble(double *dst, unsigned char **src, int dir);

void decode_fractal_info(struct fractal_info *info, int dir)
{
	unsigned char *buf;
	unsigned char *bufPtr;
	int i;

	if (dir == 1)
	{
		buf = (unsigned char *)malloc(FRACTAL_INFO_SIZE);
		bufPtr = buf;
		bcopy((char *)info, (char *)buf, FRACTAL_INFO_SIZE);
	}
	else
	{
		buf = (unsigned char *)malloc(sizeof(struct fractal_info));
		bufPtr = buf;
		bcopy((char *)info, (char *)buf, sizeof(struct fractal_info));
	}

	if (dir == 1)
	{
		strncpy(info->info_id, (char *)bufPtr, 8);
	}
	else
	{
		strncpy((char *)bufPtr, info->info_id, 8);
	}
	bufPtr += 8;
	getShort(&info->iterationsold, &bufPtr, dir);
	getShort(&info->fractal_type, &bufPtr, dir);
	getDouble(&info->x_min, &bufPtr, dir);
	getDouble(&info->x_max, &bufPtr, dir);
	getDouble(&info->y_min, &bufPtr, dir);
	getDouble(&info->y_max, &bufPtr, dir);
	getDouble(&info->c_real, &bufPtr, dir);
	getDouble(&info->c_imag, &bufPtr, dir);
	getShort(&info->videomodeax, &bufPtr, dir);
	getShort(&info->videomodebx, &bufPtr, dir);
	getShort(&info->videomodecx, &bufPtr, dir);
	getShort(&info->videomodedx, &bufPtr, dir);
	getShort(&info->dotmode, &bufPtr, dir);
	getShort(&info->x_dots, &bufPtr, dir);
	getShort(&info->y_dots, &bufPtr, dir);
	getShort(&info->colors, &bufPtr, dir);
	getShort(&info->version, &bufPtr, dir);
	getFloat(&info->parm3, &bufPtr, dir);
	getFloat(&info->parm4, &bufPtr, dir);
	getFloat(&info->potential[0], &bufPtr, dir);
	getFloat(&info->potential[1], &bufPtr, dir);
	getFloat(&info->potential[2], &bufPtr, dir);
	getShort(&info->random_seed, &bufPtr, dir);
	getShort(&info->random_flag, &bufPtr, dir);
	getShort(&info->biomorph, &bufPtr, dir);
	getShort(&info->inside, &bufPtr, dir);
	getLong(&info->logmap, &bufPtr, dir);
	getFloat(&info->invert[0], &bufPtr, dir);
	getFloat(&info->invert[1], &bufPtr, dir);
	getFloat(&info->invert[2], &bufPtr, dir);
	getShort(&info->decomposition[0], &bufPtr, dir);
	getShort(&info->decomposition[1], &bufPtr, dir);
	getShort(&info->symmetry, &bufPtr, dir);
	for (i = 0; i < 16; i++)
	{
		getShort(&info->init_3d[i], &bufPtr, dir);
	}
	getShort(&info->previewfactor, &bufPtr, dir);
	getShort(&info->xtrans, &bufPtr, dir);
	getShort(&info->ytrans, &bufPtr, dir);
	getShort(&info->red_crop_left, &bufPtr, dir);
	getShort(&info->red_crop_right, &bufPtr, dir);
	getShort(&info->blue_crop_left, &bufPtr, dir);
	getShort(&info->blue_crop_right, &bufPtr, dir);
	getShort(&info->red_bright, &bufPtr, dir);
	getShort(&info->blue_bright, &bufPtr, dir);
	getShort(&info->xadjust, &bufPtr, dir);
	getShort(&info->eyeseparation, &bufPtr, dir);
	getShort(&info->glassestype, &bufPtr, dir);
	getShort(&info->outside, &bufPtr, dir);
	getDouble(&info->x_3rd, &bufPtr, dir);
	getDouble(&info->y_3rd, &bufPtr, dir);
	getChar(&info->stdcalcmode, &bufPtr, dir);
	getChar(&info->use_initial_orbit_z, &bufPtr, dir);
	getShort(&info->calculation_status, &bufPtr, dir);
	getLong(&info->tot_extend_len, &bufPtr, dir);
	getLong(&info->distance_test, &bufPtr, dir);
	getShort(&info->float_flag, &bufPtr, dir);
	getShort(&info->bailoutold, &bufPtr, dir);
	getLong(&info->calculation_time, &bufPtr, dir);
	for (i = 0; i < 4; i++)
	{
		getUChar(&info->trig_index[i], &bufPtr, dir);
	}
	getShort(&info->finattract, &bufPtr, dir);
	getDouble(&info->initial_orbit_z[0], &bufPtr, dir);
	getDouble(&info->initial_orbit_z[1], &bufPtr, dir);
	getShort(&info->periodicity, &bufPtr, dir);
	getShort(&info->potential_16bit, &bufPtr, dir);
	getFloat(&info->faspectratio, &bufPtr, dir);
	getShort(&info->system, &bufPtr, dir);
	getShort(&info->release, &bufPtr, dir);
	getShort(&info->flag3d, &bufPtr, dir);
	getShort(&info->transparent[0], &bufPtr, dir);
	getShort(&info->transparent[1], &bufPtr, dir);
	getShort(&info->ambient, &bufPtr, dir);
	getShort(&info->haze, &bufPtr, dir);
	getShort(&info->randomize, &bufPtr, dir);
	getShort(&info->rotate_lo, &bufPtr, dir);
	getShort(&info->rotate_hi, &bufPtr, dir);
	getShort(&info->distance_test_width, &bufPtr, dir);
	getDouble(&info->dparm3, &bufPtr, dir);
	getDouble(&info->dparm4, &bufPtr, dir);
	getShort(&info->fill_color, &bufPtr, dir);
	getDouble(&info->mxmaxfp, &bufPtr, dir);
	getDouble(&info->mxminfp, &bufPtr, dir);
	getDouble(&info->mymaxfp, &bufPtr, dir);
	getDouble(&info->myminfp, &bufPtr, dir);
	getShort(&info->zdots, &bufPtr, dir);
	getFloat(&info->originfp, &bufPtr, dir);
	getFloat(&info->depthfp, &bufPtr, dir);
	getFloat(&info->heightfp, &bufPtr, dir);
	getFloat(&info->widthfp, &bufPtr, dir);
	getFloat(&info->screen_distance_fp, &bufPtr, dir);
	getFloat(&info->eyesfp, &bufPtr, dir);
	getShort(&info->orbittype, &bufPtr, dir);
	getShort(&info->juli3Dmode, &bufPtr, dir);
	getShort(&info->max_fn, &bufPtr, dir);
	getShort(&info->inversejulia, &bufPtr, dir);
	getDouble(&info->dparm5, &bufPtr, dir);
	getDouble(&info->dparm6, &bufPtr, dir);
	getDouble(&info->dparm7, &bufPtr, dir);
	getDouble(&info->dparm8, &bufPtr, dir);
	getDouble(&info->dparm9, &bufPtr, dir);
	getDouble(&info->dparm10, &bufPtr, dir);
	getLong(&info->bail_out, &bufPtr, dir);
	getShort(&info->bailoutest, &bufPtr, dir);
	getLong(&info->iterations, &bufPtr, dir);
	getShort(&info->bf_math, &bufPtr, dir);
	getShort(&info->bflength, &bufPtr, dir);
	getShort(&info->yadjust, &bufPtr, dir);
	getShort(&info->old_demm_colors, &bufPtr, dir);
	getLong(&info->logmap, &bufPtr, dir);
	getLong(&info->distance_test, &bufPtr, dir);
	getDouble(&info->dinvert[0], &bufPtr, dir);
	getDouble(&info->dinvert[1], &bufPtr, dir);
	getDouble(&info->dinvert[2], &bufPtr, dir);
	getShort(&info->logcalc, &bufPtr, dir);
	getShort(&info->stop_pass, &bufPtr, dir);
	getShort(&info->quick_calculate, &bufPtr, dir);
	getDouble(&info->proximity, &bufPtr, dir);
	getShort(&info->no_bof, &bufPtr, dir);
	getLong(&info->orbit_interval, &bufPtr, dir);
	getShort(&info->orbit_delay, &bufPtr, dir);
	getDouble(&info->math_tolerance[0], &bufPtr, dir);
	getDouble(&info->math_tolerance[1], &bufPtr, dir);

	for (i = 0; i < (sizeof(info->future)/sizeof(short)); i++)
	{
		getShort(&info->future[i], &bufPtr, dir);
	}
	if (bufPtr-buf != FRACTAL_INFO_SIZE)
	{
		printf("Warning: loadfile miscount on fractal_info structure.\n");
		printf("Components add up to %d bytes, but FRACTAL_INFO_SIZE = %d\n",
		bufPtr-buf, FRACTAL_INFO_SIZE);
	}
	if (dir == 0)
	{
		bcopy((char *)buf, (char *)info, FRACTAL_INFO_SIZE);
	}

	free(buf);
}

/*
 * This routine gets a char out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getUChar(unsigned char *dst, unsigned char **src, int dir)
{
	if (dir == 1)
	{
		*dst = **src;
	}
	else
	{
		**src = *dst;
	}
	(*src)++;
}

static void getChar(char *dst, unsigned char **src, int dir)
{
	if (dir == 1)
	{
		*dst = static_cast<char>(**src);
	}
	else
	{
		**src = static_cast<unsigned char>(*dst);
	}
	(*src)++;
}

/*
 * This routine gets an int out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getShort(short *dst, unsigned char **src, int dir)
{
	if (dir == 1)
	{
		*dst = (*src)[0] + ((((char *)(*src))[1])<<8);
	}
	else
	{
		(*src)[0] = (*dst) & 0xff;
		(*src)[1] = ((*dst) & 0xff00) >> 8;
	}
	(*src) += 2; /* sizeof(int) in MS_DOS */
}

static void getUShort(unsigned short *dst, unsigned char **src, int dir)
{
	if (dir == 1)
	{
		*dst = (*src)[0] + ((((char *)(*src))[1])<<8);
	}
	else
	{
		(*src)[0] = (*dst) & 0xff;
		(*src)[1] = ((*dst) & 0xff00) >> 8;
	}
	(*src) += 2; /* sizeof(int) in MS_DOS */
}

static void getInt(int *dst, unsigned char **src, int dir)
{
	if (dir == 1)
	{
		*dst = (*src)[0] + ((((char *)(*src))[1])<<8);
	}
	else
	{
		(*src)[0] = (*dst) & 0xff;
		(*src)[1] = ((*dst) & 0xff00) >> 8;
	}
	(*src) += 2; /* sizeof(int) in MS_DOS */
}

/*
 * This routine gets a long out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getLong(long *dst, unsigned char **src, int dir)
{
	if (dir == 1)
	{
		*dst = ((unsigned long) ((*src)[0])) +
			(((unsigned long) ((*src)[1])) << 8) +
			(((unsigned long) ((*src)[2])) << 16) +
			(((long) (((char *) (*src))[3])) << 24);
	}
	else
	{
		(*src)[0] = (*dst) & 0xff;
		(*src)[1] = ((*dst) & 0xff00) >> 8;
		(*src)[2] = ((*dst) & 0xff0000) >> 16;
#ifdef __SVR4
		(*src)[3] = (unsigned)((*dst) & 0xff000000) >> 24;
#else
		(*src)[3] = ((*dst) & 0xff000000) >> 24;
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
static void getDouble(double *dst, unsigned char **src, int dir)
{
	int e;
	double f;
	int i;
	if (dir == 1)
	{
		for (i = 0; i < 8; i++)
		{
			if ((*src)[i] != 0)
			{
				break;
			}
		}
		if (i == 8)
		{
			*dst = 0;
		}
		else
		{
#ifdef __SVR4
			e = (((*src)[7] & 0x7f) << 4) + ((int)((*src)[6] & 0xf0) >> 4) - 1023;
			f = 1 + (int)((*src)[6] & 0x0f)/P4 + (int)((*src)[5])/P12 +
				(int)((*src)[4])/P20 + (int)((*src)[3])/P28 + (int)((*src)[2])/P36 +
				(int)((*src)[1])/P44 + (int)((*src)[0])/P52;
#else
			e = (((*src)[7] & 0x7f) << 4) + (((*src)[6] & 0xf0) >> 4) - 1023;
			f = 1 + ((*src)[6] & 0x0f)/P4 + (*src)[5]/P12 + (*src)[4]/P20 +
				(*src)[3]/P28 + (*src)[2]/P36 + (*src)[1]/P44 + (*src)[0]/P52;
#endif
			f *= pow(2., (double)e);
			if ((*src)[7] & 0x80)
			{
				f = -f;
			}
			*dst = f;
		}
	}
	else
	{
		if (*dst == 0)
		{
			bzero((char *)(*src), 8);
		}
		else
		{
			int s=0;
			f = *dst;
			if (f<0)
			{
				s = 0x80;
				f = -f;
			}
			e = static_cast<int>(log(f)/log(2.));
			f = f/pow(2., (double)e) - 1;
			if (f<0)
			{
				e--;
				f = (f+1)*2-1;
			}
			else if (f>=1)
			{
				e++;
				f = (f+1)/2-1;
			}
			e += 1023;
			(*src)[7] = s | ((e & 0x7f0) >> 4);
			f *= P4;
			(*src)[6] = ((e & 0x0f) << 4) | (((int)f) & 0x0f);
			f = (f-(int)f)*P8;
			(*src)[5] = (((int)f) & 0xff);
			f = (f-(int)f)*P8;
			(*src)[4] = (((int)f) & 0xff);
			f = (f-(int)f)*P8;
			(*src)[3] = (((int)f) & 0xff);
			f = (f-(int)f)*P8;
			(*src)[2] = (((int)f) & 0xff);
			f = (f-(int)f)*P8;
			(*src)[1] = (((int)f) & 0xff);
			f = (f-(int)f)*P8;
			(*src)[0] = (((int)f) & 0xff);
		}
	}
	*src += 8; /* sizeof(double) in MSDOS */
}

/*
 * This routine gets a float out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getFloat(float *dst, unsigned char **src, int dir)
{
	int e;
	double f;
	int i;
	if (dir == 1)
	{
		for (i = 0; i < 4; i++)
		{
			if ((*src)[i] != 0) break;
		}
		if (i == 4)
		{
			*dst = 0;
		}
		else
		{
#ifdef __SVR4
			e = ((((*src)[3] & 0x7f) << 1) | ((int)((*src)[2] & 0x80) >> 7)) - 127;
			f = 1 + (int)((*src)[2] & 0x7f)/P7 + (int)((*src)[1])/P15 + (int)((*src)[0])/P23;
#else
			e = ((((*src)[3] & 0x7f) << 1) | (((*src)[2] & 0x80) >> 7)) - 127;
			f = 1 + ((*src)[2] & 0x7f)/P7 + (*src)[1]/P15 + (*src)[0]/P23;
#endif
			f *= pow(2., (double)e);
			if ((*src)[3] & 0x80)
			{
				f = -f;
			}
			*dst = f;
		}
	}
	else
	{
		if (*dst == 0)
		{
			bzero((char *)(*src), 4);
		}
		else
		{
			int s=0;
			f = *dst;
			if (f<0)
			{
				s = 0x80;
				f = -f;
			}
			e = static_cast<int>(log(f)/log(2.));
			f = f/pow(2., (double)e) - 1;
			if (f<0)
			{
				e--;
				f = (f+1)*2-1;
			}
			else if (f>=1)
			{
				e++;
				f = (f+1)/2-1;
			}
			e += 127;
			(*src)[3] = s | ((e & 0xf7) >> 1);
			f *= P7;
			(*src)[2] = ((e & 0x01) << 7) | (((int)f) & 0x7f);
			f = (f-(int)f)*P8;
			(*src)[1] = (((int)f) & 0xff);
			f = (f-(int)f)*P8;
			(*src)[0] = (((int)f) & 0xff);
		}
	}
	*src += 4; /* sizeof(float) in MSDOS */
}

/*
 * Fix up the ranges data.
 */
void fix_ranges(int *ranges, int num, int dir)
{
	short *tmp;
	unsigned char *buf;
	unsigned char *bufPtr;
	int i;

	if (dir == 1)
	{
		buf = (unsigned char *) malloc(num*2);
		bufPtr = buf;
		bcopy((char *)ranges, (char *)buf, num*2);
	}
	else
	{
		buf = (unsigned char *) malloc(num*sizeof(int));
		bufPtr = buf;
		bcopy((char *)ranges, (char *)buf, num*sizeof(int));
	}
	for (i = 0; i < num; i++)
	{
		getInt(&ranges[i], &bufPtr, dir);
	}
	free(buf);
}

void decode_evolver_info(struct evolution_info *info, int dir)
{
	unsigned char *buf;
	unsigned char *bufPtr;
	int i;

	if (dir == 1)
	{
		buf = (unsigned char *)malloc(EVOLVER_INFO_SIZE);
		bufPtr = buf;
		bcopy((char *)info, (char *)buf, EVOLVER_INFO_SIZE);
	}
	else
	{
		buf = (unsigned char *)malloc(sizeof(struct evolution_info));
		bufPtr = buf;
		bcopy((char *)info, (char *)buf, sizeof(struct evolution_info));
	}

	getShort(&info->evolving, &bufPtr, dir);
	getShort(&info->gridsz, &bufPtr, dir);
	getUShort(&info->this_generation_random_seed, &bufPtr, dir);
	getDouble(&info->fiddle_factor, &bufPtr, dir);
	getDouble(&info->parameter_range_x, &bufPtr, dir);
	getDouble(&info->parameter_range_y, &bufPtr, dir);
	getDouble(&info->opx, &bufPtr, dir);
	getDouble(&info->opy, &bufPtr, dir);
	getShort(&info->odpx, &bufPtr, dir);
	getShort(&info->odpy, &bufPtr, dir);
	getShort(&info->px, &bufPtr, dir);
	getShort(&info->py, &bufPtr, dir);
	getShort(&info->sxoffs, &bufPtr, dir);
	getShort(&info->syoffs, &bufPtr, dir);
	getShort(&info->x_dots, &bufPtr, dir);
	getShort(&info->y_dots, &bufPtr, dir);
	for (i = 0; i < NUMGENES; i++)
	{
		getShort(&info->mutate[i], &bufPtr, dir);
	}
	getShort(&info->ecount, &bufPtr, dir);

	for (i = 0; i < (sizeof(info->future)/sizeof(short)); i++)
	{
		getShort(&info->future[i], &bufPtr, dir);
	}
	if (bufPtr-buf != EVOLVER_INFO_SIZE)
	{
		printf("Warning: loadfile miscount on evolution_info structure.\n");
		printf("Components add up to %d bytes, but EVOLVER_INFO_SIZE = %d\n",
			bufPtr-buf, EVOLVER_INFO_SIZE);
	}
	if (dir == 0)
	{
		bcopy((char *)buf, (char *)info, EVOLVER_INFO_SIZE);
	}

	free(buf);
}

void decode_orbits_info(struct orbits_info *info, int dir)
{
	unsigned char *buf;
	unsigned char *bufPtr;
	int i;

	if (dir == 1)
	{
		buf = (unsigned char *)malloc(ORBITS_INFO_SIZE);
		bufPtr = buf;
		bcopy((char *)info, (char *)buf, ORBITS_INFO_SIZE);
	}
	else
	{
		buf = (unsigned char *)malloc(sizeof(struct orbits_info));
		bufPtr = buf;
		bcopy((char *)info, (char *)buf, sizeof(struct orbits_info));
	}

	getDouble(&info->oxmin, &bufPtr, dir);
	getDouble(&info->oxmax, &bufPtr, dir);
	getDouble(&info->oymin, &bufPtr, dir);
	getDouble(&info->oymax, &bufPtr, dir);
	getDouble(&info->ox3rd, &bufPtr, dir);
	getDouble(&info->oy3rd, &bufPtr, dir);
	getShort(&info->keep_scrn_coords, &bufPtr, dir);
	getChar(&info->drawmode, &bufPtr, dir);
	getChar(&info->dummy, &bufPtr, dir);

	for (i = 0; i < (sizeof(info->future)/sizeof(short)); i++)
	{
		getShort(&info->future[i], &bufPtr, dir);
	}
	if (bufPtr-buf != ORBITS_INFO_SIZE)
	{
		printf("Warning: loadfile miscount on orbits_info structure.\n");
		printf("Components add up to %d bytes, but ORBITS_INFO_SIZE = %d\n",
			bufPtr-buf, ORBITS_INFO_SIZE);
	}
	if (dir == 0)
	{
		bcopy((char *)buf, (char *)info, ORBITS_INFO_SIZE);
	}

	free(buf);
}
