#include "decode_info.h"

#include "evolve.h"
#include "loadfile.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

// Assumptions about the floating-point types used in the blob structures.
static_assert(sizeof(float) == 4, "sizeof(float) != 4");
static_assert(sizeof(double) == 8, "sizeof(double) != 8");

// The size of these structures must remain fixed in order to maintain
// compatibility with the binary blobs written into GIF files.
//
// TODO: verify that size of binary EVOLUTION_INFO blob is 200; DOS code reported size of 208.
//
static_assert(sizeof(EVOLUTION_INFO) == 200, "EVOLUTION_INFO size is incorrect");
static_assert(sizeof(FRACTAL_INFO) == 504, "FRACTAL_INFO size is incorrect");
static_assert(sizeof(ORBITS_INFO) == 200, "ORBITS_INFO size is incorrect");

/* --------------------------------------------------------------------
 * The following routines are used for encoding/decoding gif images.
 * If we aren't on a PC, things are rough for decoding the fractal info
 * structure in the GIF file.  These routines look after converting the
 * MS_DOS format data into a form we can use.
 * If dir==0, we convert to MSDOS form.  Otherwise we convert from MSDOS.
 */

static void get_uint8(std::uint8_t *dst, unsigned char **src, int dir);
static void get_int16(std::int16_t *dst, unsigned char **src, int dir);
static void get_int32(std::int32_t *dst, unsigned char **src, int dir);
static void get_float(float *dst, unsigned char **src, int dir);
static void get_double(double *dst, unsigned char **src, int dir);

void decode_fractal_info_big_endian(FRACTAL_INFO *info, int dir)
{
    std::vector<unsigned char> info_buff;
    info_buff.resize(sizeof(FRACTAL_INFO));
    unsigned char *buf = &info_buff[0];
    unsigned char *bufPtr = buf;
    std::memcpy(buf, info, sizeof(FRACTAL_INFO));

    if (dir == 1)
    {
        std::strncpy(info->info_id, (char *)bufPtr, 8);
    }
    else
    {
        std::strncpy((char *)bufPtr, info->info_id, 8);
    }
    bufPtr += 8;
    get_int16(&info->iterationsold, &bufPtr, dir);
    get_int16(&info->fractal_type, &bufPtr, dir);
    get_double(&info->xmin, &bufPtr, dir);
    get_double(&info->xmax, &bufPtr, dir);
    get_double(&info->ymin, &bufPtr, dir);
    get_double(&info->ymax, &bufPtr, dir);
    get_double(&info->creal, &bufPtr, dir);
    get_double(&info->cimag, &bufPtr, dir);
    get_int16(&info->videomodeax, &bufPtr, dir);
    get_int16(&info->videomodebx, &bufPtr, dir);
    get_int16(&info->videomodecx, &bufPtr, dir);
    get_int16(&info->videomodedx, &bufPtr, dir);
    get_int16(&info->dotmode, &bufPtr, dir);
    get_int16(&info->xdots, &bufPtr, dir);
    get_int16(&info->ydots, &bufPtr, dir);
    get_int16(&info->colors, &bufPtr, dir);
    get_int16(&info->version, &bufPtr, dir);
    get_float(&info->parm3, &bufPtr, dir);
    get_float(&info->parm4, &bufPtr, dir);
    get_float(&info->potential[0], &bufPtr, dir);
    get_float(&info->potential[1], &bufPtr, dir);
    get_float(&info->potential[2], &bufPtr, dir);
    get_int16(&info->rseed, &bufPtr, dir);
    get_int16(&info->rflag, &bufPtr, dir);
    get_int16(&info->biomorph, &bufPtr, dir);
    get_int16(&info->inside, &bufPtr, dir);
    {
        short tmp = 0;
        get_int16(&tmp, &bufPtr, dir);
        info->logmap = tmp;
    }
    get_float(&info->invert[0], &bufPtr, dir);
    get_float(&info->invert[1], &bufPtr, dir);
    get_float(&info->invert[2], &bufPtr, dir);
    get_int16(&info->decomp[0], &bufPtr, dir);
    get_int16(&info->decomp[1], &bufPtr, dir);
    get_int16(&info->symmetry, &bufPtr, dir);
    for (int i = 0; i < 16; i++)
    {
        get_int16(&info->init3d[i], &bufPtr, dir);
    }
    get_int16(&info->previewfactor, &bufPtr, dir);
    get_int16(&info->xtrans, &bufPtr, dir);
    get_int16(&info->ytrans, &bufPtr, dir);
    get_int16(&info->red_crop_left, &bufPtr, dir);
    get_int16(&info->red_crop_right, &bufPtr, dir);
    get_int16(&info->blue_crop_left, &bufPtr, dir);
    get_int16(&info->blue_crop_right, &bufPtr, dir);
    get_int16(&info->red_bright, &bufPtr, dir);
    get_int16(&info->blue_bright, &bufPtr, dir);
    get_int16(&info->xadjust, &bufPtr, dir);
    get_int16(&info->eyeseparation, &bufPtr, dir);
    get_int16(&info->glassestype, &bufPtr, dir);
    get_int16(&info->outside, &bufPtr, dir);
    get_double(&info->x3rd, &bufPtr, dir);
    get_double(&info->y3rd, &bufPtr, dir);
    get_uint8(reinterpret_cast<unsigned char *>(&info->stdcalcmode), &bufPtr, dir);
    get_uint8(reinterpret_cast<unsigned char *>(&info->useinitorbit), &bufPtr, dir);
    get_int16(&info->calc_status, &bufPtr, dir);
    get_int32(&info->tot_extend_len, &bufPtr, dir);
    {
        short tmp = 0;
        get_int16(&tmp, &bufPtr, dir);
        info->distest = tmp;
    }
    get_int16(&info->floatflag, &bufPtr, dir);
    get_int16(&info->bailoutold, &bufPtr, dir);
    get_int32(&info->calctime, &bufPtr, dir);
    for (int i = 0; i < 4; i++)
    {
        get_uint8(&info->trigndx[i], &bufPtr, dir);
    }
    get_int16(&info->finattract, &bufPtr, dir);
    get_double(&info->initorbit[0], &bufPtr, dir);
    get_double(&info->initorbit[1], &bufPtr, dir);
    get_int16(&info->periodicity, &bufPtr, dir);
    get_int16(&info->pot16bit, &bufPtr, dir);
    get_float(&info->faspectratio, &bufPtr, dir);
    get_int16(&info->system, &bufPtr, dir);
    get_int16(&info->release, &bufPtr, dir);
    get_int16(&info->display_3d, &bufPtr, dir);
    get_int16(&info->transparent[0], &bufPtr, dir);
    get_int16(&info->transparent[1], &bufPtr, dir);
    get_int16(&info->ambient, &bufPtr, dir);
    get_int16(&info->haze, &bufPtr, dir);
    get_int16(&info->randomize, &bufPtr, dir);
    get_int16(&info->rotate_lo, &bufPtr, dir);
    get_int16(&info->rotate_hi, &bufPtr, dir);
    get_int16(&info->distestwidth, &bufPtr, dir);
    get_double(&info->dparm3, &bufPtr, dir);
    get_double(&info->dparm4, &bufPtr, dir);
    get_int16(&info->fillcolor, &bufPtr, dir);
    get_double(&info->mxmaxfp, &bufPtr, dir);
    get_double(&info->mxminfp, &bufPtr, dir);
    get_double(&info->mymaxfp, &bufPtr, dir);
    get_double(&info->myminfp, &bufPtr, dir);
    get_int16(&info->zdots, &bufPtr, dir);
    get_float(&info->originfp, &bufPtr, dir);
    get_float(&info->depthfp, &bufPtr, dir);
    get_float(&info->heightfp, &bufPtr, dir);
    get_float(&info->widthfp, &bufPtr, dir);
    get_float(&info->distfp, &bufPtr, dir);
    get_float(&info->eyesfp, &bufPtr, dir);
    get_int16(&info->orbittype, &bufPtr, dir);
    get_int16(&info->juli3Dmode, &bufPtr, dir);
    get_int16(&info->maxfn, &bufPtr, dir);
    get_int16(&info->inversejulia, &bufPtr, dir);
    get_double(&info->dparm5, &bufPtr, dir);
    get_double(&info->dparm6, &bufPtr, dir);
    get_double(&info->dparm7, &bufPtr, dir);
    get_double(&info->dparm8, &bufPtr, dir);
    get_double(&info->dparm9, &bufPtr, dir);
    get_double(&info->dparm10, &bufPtr, dir);
    get_int32(&info->bailout, &bufPtr, dir);
    get_int16(&info->bailoutest, &bufPtr, dir);
    get_int32(&info->iterations, &bufPtr, dir);
    get_int16(&info->bf_math, &bufPtr, dir);
    get_int16(&info->g_bf_length, &bufPtr, dir);
    get_int16(&info->yadjust, &bufPtr, dir);
    get_int16(&info->old_demm_colors, &bufPtr, dir);
    get_int32(&info->logmap, &bufPtr, dir);
    get_int32(&info->distest, &bufPtr, dir);
    get_double(&info->dinvert[0], &bufPtr, dir);
    get_double(&info->dinvert[1], &bufPtr, dir);
    get_double(&info->dinvert[2], &bufPtr, dir);
    get_int16(&info->logcalc, &bufPtr, dir);
    get_int16(&info->stoppass, &bufPtr, dir);
    get_int16(&info->quick_calc, &bufPtr, dir);
    get_double(&info->closeprox, &bufPtr, dir);
    get_int16(&info->nobof, &bufPtr, dir);
    get_int32(&info->orbit_interval, &bufPtr, dir);
    get_int16(&info->orbit_delay, &bufPtr, dir);
    get_double(&info->math_tol[0], &bufPtr, dir);
    get_double(&info->math_tol[1], &bufPtr, dir);

    for (int i = 0; i < (sizeof(info->future)/sizeof(short)); i++)
    {
        get_int16(&info->future[i], &bufPtr, dir);
    }
    if (bufPtr-buf != sizeof(FRACTAL_INFO))
    {
        std::printf("Warning: loadfile miscount on fractal_info structure.\n");
        std::printf("Components add up to %d bytes, but sizeof(FRACTAL_INFO) = %d\n",
               (int)(bufPtr-buf), (int) sizeof(FRACTAL_INFO));
    }
    if (dir == 0)
    {
        std::memcpy((char *)info, (char *)buf, sizeof(FRACTAL_INFO));
    }
}

/*
 * This routine gets a char out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void get_uint8(std::uint8_t *dst, unsigned char **src, int dir)
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
static void get_int16(std::int16_t *dst, unsigned char **src, int dir)
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
static void get_int32(std::int32_t *dst, unsigned char **src, int dir)
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
static void get_double(double *dst, unsigned char **src, int dir)
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
static void get_float(float *dst, unsigned char **src, int dir)
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

void decode_evolver_info_big_endian(EVOLUTION_INFO *info, int dir)
{
    std::vector<unsigned char> evolution_info_buff;
    unsigned char *buf;
    unsigned char *bufPtr;

    if (dir == 1)
    {
        evolution_info_buff.resize(sizeof(EVOLUTION_INFO));
        buf = &evolution_info_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)info, sizeof(EVOLUTION_INFO));
    }
    else
    {
        evolution_info_buff.resize(sizeof(EVOLUTION_INFO));
        buf = &evolution_info_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)info, sizeof(EVOLUTION_INFO));
    }

    get_int16((short *) &info->evolving, &bufPtr, dir);
    get_int16(&info->image_grid_size, &bufPtr, dir);
    get_int16((short *) &info->this_generation_random_seed, &bufPtr, dir);
    get_double(&info->max_random_mutation, &bufPtr, dir);
    get_double(&info->x_parameter_range, &bufPtr, dir);
    get_double(&info->y_parameter_range, &bufPtr, dir);
    get_double(&info->x_parameter_offset, &bufPtr, dir);
    get_double(&info->y_parameter_offset, &bufPtr, dir);
    get_int16(&info->discrete_x_parameter_offset, &bufPtr, dir);
    get_int16(&info->discrete_y_paramter_offset, &bufPtr, dir);
    get_int16(&info->px, &bufPtr, dir);
    get_int16(&info->py, &bufPtr, dir);
    get_int16(&info->sxoffs, &bufPtr, dir);
    get_int16(&info->syoffs, &bufPtr, dir);
    get_int16(&info->xdots, &bufPtr, dir);
    get_int16(&info->ydots, &bufPtr, dir);
    for (int i = 0; i < NUM_GENES; i++)
    {
        get_int16(&info->mutate[i], &bufPtr, dir);
    }
    get_int16(&info->ecount, &bufPtr, dir);

    for (int i = 0; i < (sizeof(info->future)/sizeof(short)); i++)
    {
        get_int16(&info->future[i], &bufPtr, dir);
    }
    if (bufPtr-buf != sizeof(EVOLUTION_INFO))
    {
        std::printf("Warning: loadfile miscount on evolution_info structure.\n");
        std::printf("Components add up to %d bytes, but sizeof(EVOLUTION_INFO) = %d\n",
               (int)(bufPtr-buf), (int) sizeof(EVOLUTION_INFO));
    }
    if (dir == 0)
    {
        std::memcpy((char *)info, (char *)buf, sizeof(EVOLUTION_INFO));
    }
}

void decode_orbits_info_big_endian(ORBITS_INFO *info, int dir)
{
    std::vector<unsigned char> orbits_info_buff;
    unsigned char *buf;
    unsigned char *bufPtr;

    if (dir == 1)
    {
        orbits_info_buff.resize(sizeof(ORBITS_INFO));
        buf = &orbits_info_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)info, sizeof(ORBITS_INFO));
    }
    else
    {
        orbits_info_buff.resize(sizeof(ORBITS_INFO));
        buf = &orbits_info_buff[0];
        bufPtr = buf;
        std::memcpy((char *)buf, (char *)info, sizeof(ORBITS_INFO));
    }

    get_double(&info->oxmin, &bufPtr, dir);
    get_double(&info->oxmax, &bufPtr, dir);
    get_double(&info->oymin, &bufPtr, dir);
    get_double(&info->oymax, &bufPtr, dir);
    get_double(&info->ox3rd, &bufPtr, dir);
    get_double(&info->oy3rd, &bufPtr, dir);
    get_int16(&info->keep_scrn_coords, &bufPtr, dir);
    get_uint8((unsigned char *) &info->drawmode, &bufPtr, dir);
    get_uint8((unsigned char *) &info->dummy, &bufPtr, dir);

    for (int i = 0; i < (sizeof(info->future)/sizeof(short)); i++)
    {
        get_int16(&info->future[i], &bufPtr, dir);
    }
    if (bufPtr-buf != sizeof(ORBITS_INFO))
    {
        std::printf("Warning: loadfile miscount on orbits_info structure.\n");
        std::printf("Components add up to %d bytes, but sizeof(ORBITS_INFO) = %d\n",
               (int)(bufPtr-buf), (int) sizeof(ORBITS_INFO));
    }
    if (dir == 0)
    {
        std::memcpy((char *)info, (char *)buf, sizeof(ORBITS_INFO));
    }
}
