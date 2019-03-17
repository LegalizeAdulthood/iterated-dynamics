/* generalasm.c
 * This file contains routines to replace general.asm.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */
#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "id_data.h"
#include "loadfile.h"
#include "miscres.h"
#include "slideshw.h"

#include <fcntl.h>
#ifndef NOBSTRING
#ifndef sun
// If this gives you an error, read the README and modify the Makefile.
#include <bstring.h>
#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <cmath>
#include <cstring>
#include <vector>

// ********************** Mouse Support Variables **************************


bool g_inside_help = false;


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
static int keybuffer = 0;

int getkeynowait();
int getkeyint(int);

int keypressed()
{
    int ch;
    ch = getkeynowait();
    if (!ch)
        return 0;
    keybuffer = ch;
    if (ch == FIK_F1 && g_help_mode != help_labels::IDHELP_INDEX)
    {
        keybuffer = 0;
        g_inside_help = true;
        help(0);
        g_inside_help = false;
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
int waitkeypressed(int timeout)
{
    while (!keybuffer)
    {
        keybuffer = getkeyint(1);
        if (timeout)
            break;
    }
    return keypressed();
}

/*
 * This routine returns a key, ignoring F1
 */
int
getakeynohelp()
{
    int ch;
    while (1)
    {
        ch = getakey();
        if (ch != FIK_F1)
            break;
    }
    return ch;
}
/*
 * This routine returns a keypress
 */
int
getakey()
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
int
getkeynowait()
{
    return getkeyint(0);
}

/*
 * This is the low level key handling routine.
 * If block is set, we want to block before returning, since we are waiting
 * for a key press.
 * We also have to handle the slide file, etc.
 */

int
getkeyint(int block)
{
    if (keybuffer)
    {
        int ch = keybuffer;
        keybuffer = 0;
        return ch;
    }
    int curkey = xgetkey(0);
    if (g_slides == slides_mode::PLAY && curkey == FIK_ESC)
    {
        stopslideshow();
        return 0;
    }

    if (curkey == 0 && g_slides == slides_mode::PLAY)
    {
        curkey = slideshw();
    }

    if (curkey == 0 && block)
    {
        curkey = xgetkey(1);
        if (g_slides == slides_mode::PLAY && curkey == FIK_ESC)
        {
            stopslideshow();
            return 0;
        }
    }

    if (curkey && g_slides == slides_mode::RECORD)
    {
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
buzzer(buzzer_codes buzzertype)
{
    if ((g_sound_flag & 7) != 0)
    {
        printf("\007");
        fflush(stdout);
    }
    if (buzzertype == buzzer_codes::COMPLETE)
    {
        redrawscreen();
    }
}

/*
; ***************** Function delay(int delaytime) ************************
;
;       performs a delay loop for 'delaytime' milliseconds
*/
void
delay(int delaytime)
{
    static struct timeval delay;
    delay.tv_sec = delaytime/1000;
    delay.tv_usec = (delaytime%1000)*1000;
    select(0, nullptr, nullptr, nullptr, &delay);
}

/*
; ************** Function tone(int frequency,int delaytime) **************
;
;       buzzes the speaker with this frequency for this amount of time
*/
void
tone(int frequency, int delaytime)
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
snd(int hertz)
{
}

void
nosnd()
{}

/*
; long readticker() returns current bios ticker value
*/
long
readticker()
{
    return clock_ticks();
}

/* --------------------------------------------------------------------
 * The following routines are used for encoding/decoding gif images.
 * If we aren't on a PC, things are rough for decoding the fractal info
 * structure in the GIF file.  These routines look after converting the
 * MS_DOS format data into a form we can use.
 * If dir==0, we convert to MSDOS form.  Otherwise we convert from MSDOS.
 */

static void getChar(unsigned char *dst, unsigned char **src, int dir);
static void getInt(short *dst, unsigned char **src, int dir);
static void getLong(long *dst, unsigned char **src, int dir);
static void getFloat(float *dst, unsigned char **src, int dir);
static void getDouble(double *dst, unsigned char **src, int dir);

void decode_fractal_info(FRACTAL_INFO *info, int dir)
{
    std::vector<unsigned char> info_buff;
    unsigned char *buf;
    unsigned char *bufPtr;

    if (dir == 1)
    {
        info_buff.resize(FRACTAL_INFO_SIZE);
        buf = &info_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)info, FRACTAL_INFO_SIZE);
    }
    else
    {
        info_buff.resize(sizeof(FRACTAL_INFO));
        buf = &info_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)info, sizeof(FRACTAL_INFO));
    }

    if (dir == 1)
    {
        std::strncpy(info->info_id, (char *)bufPtr, 8);
    }
    else
    {
        std::strncpy((char *)bufPtr, info->info_id, 8);
    }
    bufPtr += 8;
    getInt(&info->iterationsold, &bufPtr, dir);
    getInt(&info->fractal_type, &bufPtr, dir);
    getDouble(&info->xmin, &bufPtr, dir);
    getDouble(&info->xmax, &bufPtr, dir);
    getDouble(&info->ymin, &bufPtr, dir);
    getDouble(&info->ymax, &bufPtr, dir);
    getDouble(&info->creal, &bufPtr, dir);
    getDouble(&info->cimag, &bufPtr, dir);
    getInt(&info->videomodeax, &bufPtr, dir);
    getInt(&info->videomodebx, &bufPtr, dir);
    getInt(&info->videomodecx, &bufPtr, dir);
    getInt(&info->videomodedx, &bufPtr, dir);
    getInt(&info->dotmode, &bufPtr, dir);
    getInt(&info->xdots, &bufPtr, dir);
    getInt(&info->ydots, &bufPtr, dir);
    getInt(&info->colors, &bufPtr, dir);
    getInt(&info->version, &bufPtr, dir);
    getFloat(&info->parm3, &bufPtr, dir);
    getFloat(&info->parm4, &bufPtr, dir);
    getFloat(&info->potential[0], &bufPtr, dir);
    getFloat(&info->potential[1], &bufPtr, dir);
    getFloat(&info->potential[2], &bufPtr, dir);
    getInt(&info->rseed, &bufPtr, dir);
    getInt(&info->rflag, &bufPtr, dir);
    getInt(&info->biomorph, &bufPtr, dir);
    getInt(&info->inside, &bufPtr, dir);
    {
        short tmp = 0;
        getInt(&tmp, &bufPtr, dir);
        info->logmap = tmp;
    }
    getFloat(&info->invert[0], &bufPtr, dir);
    getFloat(&info->invert[1], &bufPtr, dir);
    getFloat(&info->invert[2], &bufPtr, dir);
    getInt(&info->decomp[0], &bufPtr, dir);
    getInt(&info->decomp[1], &bufPtr, dir);
    getInt(&info->symmetry, &bufPtr, dir);
    for (int i = 0; i < 16; i++)
    {
        getInt(&info->init3d[i], &bufPtr, dir);
    }
    getInt(&info->previewfactor, &bufPtr, dir);
    getInt(&info->xtrans, &bufPtr, dir);
    getInt(&info->ytrans, &bufPtr, dir);
    getInt(&info->red_crop_left, &bufPtr, dir);
    getInt(&info->red_crop_right, &bufPtr, dir);
    getInt(&info->blue_crop_left, &bufPtr, dir);
    getInt(&info->blue_crop_right, &bufPtr, dir);
    getInt(&info->red_bright, &bufPtr, dir);
    getInt(&info->blue_bright, &bufPtr, dir);
    getInt(&info->xadjust, &bufPtr, dir);
    getInt(&info->eyeseparation, &bufPtr, dir);
    getInt(&info->glassestype, &bufPtr, dir);
    getInt(&info->outside, &bufPtr, dir);
    getDouble(&info->x3rd, &bufPtr, dir);
    getDouble(&info->y3rd, &bufPtr, dir);
    getChar(reinterpret_cast<unsigned char *>(&info->stdcalcmode), &bufPtr, dir);
    getChar(reinterpret_cast<unsigned char *>(&info->useinitorbit), &bufPtr, dir);
    getInt(&info->calc_status, &bufPtr, dir);
    getLong(&info->tot_extend_len, &bufPtr, dir);
    {
        short tmp = 0;
        getInt(&tmp, &bufPtr, dir);
        info->distest = tmp;
    }
    getInt(&info->floatflag, &bufPtr, dir);
    getInt(&info->bailoutold, &bufPtr, dir);
    getLong(&info->calctime, &bufPtr, dir);
    for (int i = 0; i < 4; i++)
    {
        getChar(&info->trigndx[i], &bufPtr, dir);
    }
    getInt(&info->finattract, &bufPtr, dir);
    getDouble(&info->initorbit[0], &bufPtr, dir);
    getDouble(&info->initorbit[1], &bufPtr, dir);
    getInt(&info->periodicity, &bufPtr, dir);
    getInt(&info->pot16bit, &bufPtr, dir);
    getFloat(&info->faspectratio, &bufPtr, dir);
    getInt(&info->system, &bufPtr, dir);
    getInt(&info->release, &bufPtr, dir);
    getInt(&info->display_3d, &bufPtr, dir);
    getInt(&info->transparent[0], &bufPtr, dir);
    getInt(&info->transparent[1], &bufPtr, dir);
    getInt(&info->ambient, &bufPtr, dir);
    getInt(&info->haze, &bufPtr, dir);
    getInt(&info->randomize, &bufPtr, dir);
    getInt(&info->rotate_lo, &bufPtr, dir);
    getInt(&info->rotate_hi, &bufPtr, dir);
    getInt(&info->distestwidth, &bufPtr, dir);
    getDouble(&info->dparm3, &bufPtr, dir);
    getDouble(&info->dparm4, &bufPtr, dir);
    getInt(&info->fillcolor, &bufPtr, dir);
    getDouble(&info->mxmaxfp, &bufPtr, dir);
    getDouble(&info->mxminfp, &bufPtr, dir);
    getDouble(&info->mymaxfp, &bufPtr, dir);
    getDouble(&info->myminfp, &bufPtr, dir);
    getInt(&info->zdots, &bufPtr, dir);
    getFloat(&info->originfp, &bufPtr, dir);
    getFloat(&info->depthfp, &bufPtr, dir);
    getFloat(&info->heightfp, &bufPtr, dir);
    getFloat(&info->widthfp, &bufPtr, dir);
    getFloat(&info->distfp, &bufPtr, dir);
    getFloat(&info->eyesfp, &bufPtr, dir);
    getInt(&info->orbittype, &bufPtr, dir);
    getInt(&info->juli3Dmode, &bufPtr, dir);
    getInt(&info->maxfn, &bufPtr, dir);
    getInt(&info->inversejulia, &bufPtr, dir);
    getDouble(&info->dparm5, &bufPtr, dir);
    getDouble(&info->dparm6, &bufPtr, dir);
    getDouble(&info->dparm7, &bufPtr, dir);
    getDouble(&info->dparm8, &bufPtr, dir);
    getDouble(&info->dparm9, &bufPtr, dir);
    getDouble(&info->dparm10, &bufPtr, dir);
    getLong(&info->bailout, &bufPtr, dir);
    getInt(&info->bailoutest, &bufPtr, dir);
    getLong(&info->iterations, &bufPtr, dir);
    getInt(&info->bf_math, &bufPtr, dir);
    getInt(&info->bflength, &bufPtr, dir);
    getInt(&info->yadjust, &bufPtr, dir);
    getInt(&info->old_demm_colors, &bufPtr, dir);
    getLong(&info->logmap, &bufPtr, dir);
    getLong(&info->distest, &bufPtr, dir);
    getDouble(&info->dinvert[0], &bufPtr, dir);
    getDouble(&info->dinvert[1], &bufPtr, dir);
    getDouble(&info->dinvert[2], &bufPtr, dir);
    getInt(&info->logcalc, &bufPtr, dir);
    getInt(&info->stoppass, &bufPtr, dir);
    getInt(&info->quick_calc, &bufPtr, dir);
    getDouble(&info->closeprox, &bufPtr, dir);
    getInt(&info->nobof, &bufPtr, dir);
    getLong(&info->orbit_interval, &bufPtr, dir);
    getInt(&info->orbit_delay, &bufPtr, dir);
    getDouble(&info->math_tol[0], &bufPtr, dir);
    getDouble(&info->math_tol[1], &bufPtr, dir);

    for (int i = 0; i < (sizeof(info->future)/sizeof(short)); i++)
    {
        getInt(&info->future[i], &bufPtr, dir);
    }
    if (bufPtr-buf != FRACTAL_INFO_SIZE)
    {
        printf("Warning: loadfile miscount on fractal_info structure.\n");
        printf("Components add up to %d bytes, but FRACTAL_INFO_SIZE = %d\n",
               (int)(bufPtr-buf), (int) FRACTAL_INFO_SIZE);
    }
    if (dir == 0)
    {
        std::memcpy((char *)info, (char *)buf, FRACTAL_INFO_SIZE);
    }
}

/*
 * This routine gets a char out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getChar(unsigned char *dst, unsigned char **src, int dir)
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

/*
 * This routine gets an int out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getInt(short *dst, unsigned char **src, int dir)
{
    if (dir == 1)
    {
        *dst = (*src)[0] + ((((char *)(*src))[1]) << 8);
    }
    else
    {
        (*src)[0] = (*dst)&0xff;
        (*src)[1] = ((*dst)&0xff00) >> 8;
    }
    (*src) += 2; // sizeof(int) in MS_DOS
}

/*
 * This routine gets a long out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getLong(long *dst, unsigned char **src, int dir)
{
    if (dir == 1)
    {
        *dst = ((unsigned long)((*src)[0])) +
               (((unsigned long)((*src)[1])) << 8) +
               (((unsigned long)((*src)[2])) << 16) +
               (((long)(((char *)(*src))[3])) << 24);
    }
    else
    {
        (*src)[0] = (*dst)&0xff;
        (*src)[1] = ((*dst)&0xff00) >> 8;
        (*src)[2] = ((*dst)&0xff0000) >> 16;
        (*src)[3] = ((*dst)&0xff000000) >> 24;
    }
    (*src) += 4; // sizeof(long) in MS_DOS
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
    if (dir == 1)
    {
        int i;
        for (i = 0; i < 8; i++)
        {
            if ((*src)[i] != 0)
                break;
        }
        if (i == 8)
        {
            *dst = 0;
        }
        else
        {
            e = (((*src)[7]&0x7f) << 4) + (((*src)[6]&0xf0) >> 4) - 1023;
            f = 1 + ((*src)[6]&0x0f)/P4 + (*src)[5]/P12 + (*src)[4]/P20 +
                (*src)[3]/P28 + (*src)[2]/P36 + (*src)[1]/P44 + (*src)[0]/P52;
            f *= std::pow(2., (double)e);
            if ((*src)[7]&0x80)
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
            std::memset((char *)(*src), 0, 8);
        }
        else
        {
            int s = 0;
            f = *dst;
            if (f < 0)
            {
                s = 0x80;
                f = -f;
            }
            e = std::log(f)/std::log(2.);
            f = f/std::pow(2., (double)e) - 1;
            if (f < 0)
            {
                e--;
                f = (f+1)*2-1;
            }
            else if (f >= 1)
            {
                e++;
                f = (f+1)/2-1;
            }
            e += 1023;
            (*src)[7] = s | ((e&0x7f0) >> 4);
            f *= P4;
            (*src)[6] = ((e&0x0f) << 4) | (((int)f)&0x0f);
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
    *src += 8; // sizeof(double) in MSDOS
}

/*
 * This routine gets a float out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void getFloat(float *dst, unsigned char **src, int dir)
{
    int e;
    double f;
    if (dir == 1)
    {
        int i;
        for (i = 0; i < 4; i++)
        {
            if ((*src)[i] != 0)
                break;
        }
        if (i == 4)
        {
            *dst = 0;
        }
        else
        {
            e = ((((*src)[3]&0x7f) << 1) | (((*src)[2]&0x80) >> 7)) - 127;
            f = 1 + ((*src)[2]&0x7f)/P7 + (*src)[1]/P15 + (*src)[0]/P23;
            f *= std::pow(2., (double)e);
            if ((*src)[3]&0x80)
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
            std::memset((char *)(*src), 0, 4);
        }
        else
        {
            int s = 0;
            f = *dst;
            if (f < 0)
            {
                s = 0x80;
                f = -f;
            }
            e = std::log(f)/std::log(2.);
            f = f/std::pow(2., (double)e) - 1;
            if (f < 0)
            {
                e--;
                f = (f+1)*2-1;
            }
            else if (f >= 1)
            {
                e++;
                f = (f+1)/2-1;
            }
            e += 127;
            (*src)[3] = s | ((e&0xf7) >> 1);
            f *= P7;
            (*src)[2] = ((e&0x01) << 7) | (((int)f)&0x7f);
            f = (f-(int)f)*P8;
            (*src)[1] = (((int)f)&0xff);
            f = (f-(int)f)*P8;
            (*src)[0] = (((int)f)&0xff);
        }
    }
    *src += 4; // sizeof(float) in MSDOS
}

/*
 * Fix up the ranges data for byte ordering.
 */
void
fix_ranges(int *ranges, int num, int dir)
{
    std::vector<unsigned char> ranges_buff;
    unsigned char *buf;
    unsigned char *bufPtr;

    if (dir == 1)
    {
        ranges_buff.resize(num*2);
        buf = &ranges_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)ranges, num*2);
    }
    else
    {
        ranges_buff.resize(num*sizeof(int));
        buf = &ranges_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)ranges, num*sizeof(int));
    }
    for (int i = 0; i < num; i++)
    {
        short dest = 0;
        getInt(&dest, &bufPtr, dir);
        ranges[i] = dest;
    }
}

void
decode_evolver_info(EVOLUTION_INFO *info, int dir)
{
    std::vector<unsigned char> evolution_info_buff;
    unsigned char *buf;
    unsigned char *bufPtr;

    if (dir == 1)
    {
        evolution_info_buff.resize(EVOLVER_INFO_SIZE);
        buf = &evolution_info_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)info, EVOLVER_INFO_SIZE);
    }
    else
    {
        evolution_info_buff.resize(sizeof(EVOLUTION_INFO));
        buf = &evolution_info_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)info, sizeof(EVOLUTION_INFO));
    }

    getInt((short *) &info->evolving, &bufPtr, dir);
    getInt(&info->image_grid_size, &bufPtr, dir);
    getInt((short *) &info->this_generation_random_seed, &bufPtr, dir);
    getDouble(&info->max_random_mutation, &bufPtr, dir);
    getDouble(&info->x_parameter_range, &bufPtr, dir);
    getDouble(&info->y_parameter_range, &bufPtr, dir);
    getDouble(&info->x_parameter_offset, &bufPtr, dir);
    getDouble(&info->y_parameter_offset, &bufPtr, dir);
    getInt(&info->discrete_x_parameter_offset, &bufPtr, dir);
    getInt(&info->discrete_y_paramter_offset, &bufPtr, dir);
    getInt(&info->px, &bufPtr, dir);
    getInt(&info->py, &bufPtr, dir);
    getInt(&info->sxoffs, &bufPtr, dir);
    getInt(&info->syoffs, &bufPtr, dir);
    getInt(&info->xdots, &bufPtr, dir);
    getInt(&info->ydots, &bufPtr, dir);
    for (int i = 0; i < NUM_GENES; i++)
    {
        getInt(&info->mutate[i], &bufPtr, dir);
    }
    getInt(&info->ecount, &bufPtr, dir);

    for (int i = 0; i < (sizeof(info->future)/sizeof(short)); i++)
    {
        getInt(&info->future[i], &bufPtr, dir);
    }
    if (bufPtr-buf != EVOLVER_INFO_SIZE)
    {
        printf("Warning: loadfile miscount on evolution_info structure.\n");
        printf("Components add up to %d bytes, but EVOLVER_INFO_SIZE = %d\n",
               (int)(bufPtr-buf), (int) EVOLVER_INFO_SIZE);
    }
    if (dir == 0)
    {
        std::memcpy((char *)info, (char *)buf, EVOLVER_INFO_SIZE);
    }
}

void
decode_orbits_info(ORBITS_INFO *info, int dir)
{
    std::vector<unsigned char> orbits_info_buff;
    unsigned char *buf;
    unsigned char *bufPtr;

    if (dir == 1)
    {
        orbits_info_buff.resize(ORBITS_INFO_SIZE);
        buf = &orbits_info_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)info, ORBITS_INFO_SIZE);
    }
    else
    {
        orbits_info_buff.resize(sizeof(ORBITS_INFO));
        buf = &orbits_info_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)info, sizeof(ORBITS_INFO));
    }

    getDouble(&info->oxmin, &bufPtr, dir);
    getDouble(&info->oxmax, &bufPtr, dir);
    getDouble(&info->oymin, &bufPtr, dir);
    getDouble(&info->oymax, &bufPtr, dir);
    getDouble(&info->ox3rd, &bufPtr, dir);
    getDouble(&info->oy3rd, &bufPtr, dir);
    getInt(&info->keep_scrn_coords, &bufPtr, dir);
    getChar((unsigned char *) &info->drawmode, &bufPtr, dir);
    getChar((unsigned char *) &info->dummy, &bufPtr, dir);

    for (int i = 0; i < (sizeof(info->future)/sizeof(short)); i++)
    {
        getInt(&info->future[i], &bufPtr, dir);
    }
    if (bufPtr-buf != ORBITS_INFO_SIZE)
    {
        printf("Warning: loadfile miscount on orbits_info structure.\n");
        printf("Components add up to %d bytes, but ORBITS_INFO_SIZE = %d\n",
               (int)(bufPtr-buf), (int) ORBITS_INFO_SIZE);
    }
    if (dir == 0)
    {
        std::memcpy((char *)info, (char *)buf, ORBITS_INFO_SIZE);
    }
}

static bool path_exists(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd != -1)
    {
        close(fd);
    }
    return fd != -1;
}

/*
 *----------------------------------------------------------------------
 *
 * findpath --
 *
 *      Find where a file is.
 *  We return filename if it is an absolute path.
 *  Otherwise we first try FRACTDIR/filename, SRCDIR/filename,
 *      and then ./filename.
 *
 * Results:
 *      Returns full pathname in fullpathname.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
void findpath(char const *filename, char *fullpathname)
{
    // check current directory if curdir= parameter set
    if (g_check_cur_dir)
    {
        // check for current dir
        std::strcpy(fullpathname, "./");
        std::strcat(fullpathname, filename);
        if (path_exists(fullpathname))
        {
            return;
        }
    }

    // check for absolute path
    if (filename[0] == '/')
    {
        std::strcpy(fullpathname, filename);
        if (path_exists(fullpathname))
        {
            return;
        }
    }

    // check for FRACTDIR
    std::strcpy(fullpathname, g_fractal_search_dir1);
    std::strcat(fullpathname, "/");
    std::strcat(fullpathname, filename);
    if (path_exists(fullpathname))
    {
        return;
    }

    // check for SRCDIR
    std::strcpy(fullpathname, g_fractal_search_dir2);
    std::strcat(fullpathname, "/");
    std::strcat(fullpathname, filename);
    if (path_exists(fullpathname))
    {
        return;
    }

    fullpathname[0] = 0;
}
