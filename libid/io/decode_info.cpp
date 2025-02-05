// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/decode_info.h"

#include "io/loadfile.h"
#include "ui/evolve.h"

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
static_assert(sizeof(EvolutionInfo) == 200, "EvolutionInfo size is incorrect");
static_assert(sizeof(FractalInfo) == 504, "FractalInfo size is incorrect");
static_assert(sizeof(OrbitsInfo) == 200, "OrbitsInfo size is incorrect");

/* --------------------------------------------------------------------
 * The following routines are used for encoding/decoding gif images.
 * If we aren't on a PC, things are rough for decoding the fractal info
 * structure in the GIF file.  These routines look after converting the
 * MS_DOS format data into a form we can use.
 * If dir==0, we convert to MSDOS form.  Otherwise, we convert from MSDOS.
 */

static void get_uint8(std::uint8_t *dst, unsigned char **src, int dir);
static void get_int16(std::int16_t *dst, unsigned char **src, int dir);
static void get_int32(std::int32_t *dst, unsigned char **src, int dir);
static void get_float(float *dst, unsigned char **src, int dir);
static void get_double(double *dst, unsigned char **src, int dir);

void decode_fractal_info_big_endian(FractalInfo *info, int dir)
{
    std::vector<unsigned char> info_buff;
    info_buff.resize(sizeof(FractalInfo));
    unsigned char *buf = info_buff.data();
    unsigned char *buf_ptr = buf;
    std::memcpy(buf, info, sizeof(FractalInfo));

    if (dir == 1)
    {
        std::strncpy(info->info_id, (char *)buf_ptr, 8);
    }
    else
    {
        std::strncpy((char *)buf_ptr, info->info_id, 8);
    }
    buf_ptr += 8;
    get_int16(&info->iterations_old, &buf_ptr, dir);
    get_int16(&info->fractal_type, &buf_ptr, dir);
    get_double(&info->x_min, &buf_ptr, dir);
    get_double(&info->x_max, &buf_ptr, dir);
    get_double(&info->y_min, &buf_ptr, dir);
    get_double(&info->y_max, &buf_ptr, dir);
    get_double(&info->c_real, &buf_ptr, dir);
    get_double(&info->c_imag, &buf_ptr, dir);
    get_int16(&info->ax, &buf_ptr, dir);
    get_int16(&info->bx, &buf_ptr, dir);
    get_int16(&info->cx, &buf_ptr, dir);
    get_int16(&info->dx, &buf_ptr, dir);
    get_int16(&info->dot_mode, &buf_ptr, dir);
    get_int16(&info->x_dots, &buf_ptr, dir);
    get_int16(&info->y_dots, &buf_ptr, dir);
    get_int16(&info->colors, &buf_ptr, dir);
    get_int16(&info->info_version, &buf_ptr, dir);
    get_float(&info->param3, &buf_ptr, dir);
    get_float(&info->param4, &buf_ptr, dir);
    get_float(&info->potential[0], &buf_ptr, dir);
    get_float(&info->potential[1], &buf_ptr, dir);
    get_float(&info->potential[2], &buf_ptr, dir);
    get_int16(&info->random_seed, &buf_ptr, dir);
    get_int16(&info->random_seed_flag, &buf_ptr, dir);
    get_int16(&info->biomorph, &buf_ptr, dir);
    get_int16(&info->inside, &buf_ptr, dir);
    {
        short tmp = 0;
        get_int16(&tmp, &buf_ptr, dir);
        info->log_map = tmp;
    }
    get_float(&info->invert[0], &buf_ptr, dir);
    get_float(&info->invert[1], &buf_ptr, dir);
    get_float(&info->invert[2], &buf_ptr, dir);
    get_int16(&info->decomp[0], &buf_ptr, dir);
    get_int16(&info->decomp[1], &buf_ptr, dir);
    get_int16(&info->symmetry, &buf_ptr, dir);
    for (int i = 0; i < 16; i++)  // NOLINT(modernize-loop-convert)
    {
        get_int16(&info->init3d[i], &buf_ptr, dir);
    }
    get_int16(&info->preview_factor, &buf_ptr, dir);
    get_int16(&info->x_trans, &buf_ptr, dir);
    get_int16(&info->y_trans, &buf_ptr, dir);
    get_int16(&info->red_crop_left, &buf_ptr, dir);
    get_int16(&info->red_crop_right, &buf_ptr, dir);
    get_int16(&info->blue_crop_left, &buf_ptr, dir);
    get_int16(&info->blue_crop_right, &buf_ptr, dir);
    get_int16(&info->red_bright, &buf_ptr, dir);
    get_int16(&info->blue_bright, &buf_ptr, dir);
    get_int16(&info->x_adjust, &buf_ptr, dir);
    get_int16(&info->eye_separation, &buf_ptr, dir);
    get_int16(&info->glasses_type, &buf_ptr, dir);
    get_int16(&info->outside, &buf_ptr, dir);
    get_double(&info->x3rd, &buf_ptr, dir);
    get_double(&info->y3rd, &buf_ptr, dir);
    get_uint8(reinterpret_cast<unsigned char *>(&info->std_calc_mode), &buf_ptr, dir);
    get_uint8(reinterpret_cast<unsigned char *>(&info->use_init_orbit), &buf_ptr, dir);
    get_int16(&info->calc_status, &buf_ptr, dir);
    get_int32(&info->tot_extend_len, &buf_ptr, dir);
    {
        short tmp = 0;
        get_int16(&tmp, &buf_ptr, dir);
        info->dist_est = tmp;
    }
    get_int16(&info->float_flag, &buf_ptr, dir);
    get_int16(&info->bailout_old, &buf_ptr, dir);
    get_int32(&info->calc_time, &buf_ptr, dir);
    for (int i = 0; i < 4; i++)  // NOLINT(modernize-loop-convert)
    {
        get_uint8(&info->trig_index[i], &buf_ptr, dir);
    }
    get_int16(&info->finite_attractor, &buf_ptr, dir);
    get_double(&info->init_orbit[0], &buf_ptr, dir);
    get_double(&info->init_orbit[1], &buf_ptr, dir);
    get_int16(&info->periodicity, &buf_ptr, dir);
    get_int16(&info->pot16bit, &buf_ptr, dir);
    get_float(&info->final_aspect_ratio, &buf_ptr, dir);
    get_int16(&info->system, &buf_ptr, dir);
    get_int16(&info->release, &buf_ptr, dir);
    get_int16(&info->display_3d, &buf_ptr, dir);
    get_int16(&info->transparent[0], &buf_ptr, dir);
    get_int16(&info->transparent[1], &buf_ptr, dir);
    get_int16(&info->ambient, &buf_ptr, dir);
    get_int16(&info->haze, &buf_ptr, dir);
    get_int16(&info->randomize, &buf_ptr, dir);
    get_int16(&info->rotate_lo, &buf_ptr, dir);
    get_int16(&info->rotate_hi, &buf_ptr, dir);
    get_int16(&info->dist_est_width, &buf_ptr, dir);
    get_double(&info->d_param3, &buf_ptr, dir);
    get_double(&info->d_param4, &buf_ptr, dir);
    get_int16(&info->fill_color, &buf_ptr, dir);
    get_double(&info->julibrot_x_max, &buf_ptr, dir);
    get_double(&info->julibrot_x_min, &buf_ptr, dir);
    get_double(&info->julibrot_y_max, &buf_ptr, dir);
    get_double(&info->julibrot_y_min, &buf_ptr, dir);
    get_int16(&info->julibrot_z_dots, &buf_ptr, dir);
    get_float(&info->julibrot_origin_fp, &buf_ptr, dir);
    get_float(&info->julibrot_depth_fp, &buf_ptr, dir);
    get_float(&info->julibrot_height_fp, &buf_ptr, dir);
    get_float(&info->julibrot_width_fp, &buf_ptr, dir);
    get_float(&info->julibrot_dist_fp, &buf_ptr, dir);
    get_float(&info->eyes_fp, &buf_ptr, dir);
    get_int16(&info->orbit_type, &buf_ptr, dir);
    get_int16(&info->juli3d_mode, &buf_ptr, dir);
    get_int16(&info->max_fn, &buf_ptr, dir);
    get_int16(&info->inverse_julia, &buf_ptr, dir);
    get_double(&info->d_param5, &buf_ptr, dir);
    get_double(&info->d_param6, &buf_ptr, dir);
    get_double(&info->d_param7, &buf_ptr, dir);
    get_double(&info->d_param8, &buf_ptr, dir);
    get_double(&info->d_param9, &buf_ptr, dir);
    get_double(&info->d_param10, &buf_ptr, dir);
    get_int32(&info->bailout, &buf_ptr, dir);
    get_int16(&info->bailout_test, &buf_ptr, dir);
    get_int32(&info->iterations, &buf_ptr, dir);
    get_int16(&info->bf_math, &buf_ptr, dir);
    get_int16(&info->bf_length, &buf_ptr, dir);
    get_int16(&info->y_adjust, &buf_ptr, dir);
    get_int16(&info->old_demm_colors, &buf_ptr, dir);
    get_int32(&info->log_map, &buf_ptr, dir);
    get_int32(&info->dist_est, &buf_ptr, dir);
    get_double(&info->d_invert[0], &buf_ptr, dir);
    get_double(&info->d_invert[1], &buf_ptr, dir);
    get_double(&info->d_invert[2], &buf_ptr, dir);
    get_int16(&info->log_calc, &buf_ptr, dir);
    get_int16(&info->stop_pass, &buf_ptr, dir);
    get_int16(&info->quick_calc, &buf_ptr, dir);
    get_double(&info->close_prox, &buf_ptr, dir);
    get_int16(&info->no_bof, &buf_ptr, dir);
    get_int32(&info->orbit_interval, &buf_ptr, dir);
    get_int16(&info->orbit_delay, &buf_ptr, dir);
    get_double(&info->math_tol[0], &buf_ptr, dir);
    get_double(&info->math_tol[1], &buf_ptr, dir);

    for (int i = 0; i < (sizeof(info->future)/sizeof(short)); i++)  // NOLINT(modernize-loop-convert)
    {
        get_int16(&info->future[i], &buf_ptr, dir);
    }
    if (buf_ptr-buf != sizeof(FractalInfo))
    {
        std::printf("Warning: loadfile miscount on fractal_info structure.\n");
        std::printf("Components add up to %d bytes, but sizeof(FractalInfo) = %d\n",
               (int)(buf_ptr-buf), (int) sizeof(FractalInfo));
    }
    if (dir == 0)
    {
        std::memcpy(info, buf, sizeof(FractalInfo));
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
 * This routine gets an int16_t out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static void get_int16(std::int16_t *dst, unsigned char **src, int dir)
{
    if (dir == 1)
    {
        *dst = static_cast<std::int16_t>((*src)[0] + (((char *) *src)[1] << 8));
    }
    else
    {
        (*src)[0] = *dst & 0xff;
        (*src)[1] = (*dst & 0xff00) >> 8;
    }
    *src += 2; // sizeof(std::uint16_t)
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
            {
                break;
            }
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

void decode_evolver_info_big_endian(EvolutionInfo *info, int dir)
{
    std::vector<unsigned char> evolution_info_buff;
    unsigned char *buf_ptr;

    evolution_info_buff.resize(sizeof(EvolutionInfo));
    unsigned char *buf = evolution_info_buff.data();
    buf_ptr = buf;
    std::memcpy((char *)buf, (char *)info, sizeof(EvolutionInfo));

    get_int16((short *) &info->evolving, &buf_ptr, dir);
    get_int16(&info->image_grid_size, &buf_ptr, dir);
    get_int16((short *) &info->this_generation_random_seed, &buf_ptr, dir);
    get_double(&info->max_random_mutation, &buf_ptr, dir);
    get_double(&info->x_parameter_range, &buf_ptr, dir);
    get_double(&info->y_parameter_range, &buf_ptr, dir);
    get_double(&info->x_parameter_offset, &buf_ptr, dir);
    get_double(&info->y_parameter_offset, &buf_ptr, dir);
    get_int16(&info->discrete_x_parameter_offset, &buf_ptr, dir);
    get_int16(&info->discrete_y_parameter_offset, &buf_ptr, dir);
    get_int16(&info->px, &buf_ptr, dir);
    get_int16(&info->py, &buf_ptr, dir);
    get_int16(&info->screen_x_offset, &buf_ptr, dir);
    get_int16(&info->screen_y_offset, &buf_ptr, dir);
    get_int16(&info->x_dots, &buf_ptr, dir);
    get_int16(&info->y_dots, &buf_ptr, dir);
    for (int i = 0; i < NUM_GENES; i++)  // NOLINT(modernize-loop-convert)
    {
        get_int16(&info->mutate[i], &buf_ptr, dir);
    }
    get_int16(&info->count, &buf_ptr, dir);

    for (int i = 0; i < (sizeof(info->future)/sizeof(short)); i++)  // NOLINT(modernize-loop-convert)
    {
        get_int16(&info->future[i], &buf_ptr, dir);
    }
    if (buf_ptr-buf != sizeof(EvolutionInfo))
    {
        std::printf("Warning: loadfile miscount on evolution_info structure.\n");
        std::printf("Components add up to %d bytes, but sizeof(EVOLUTION_INFO) = %d\n",
               (int)(buf_ptr-buf), (int) sizeof(EvolutionInfo));
    }
    if (dir == 0)
    {
        std::memcpy((char *)info, (char *)buf, sizeof(EvolutionInfo));
    }
}

void decode_orbits_info_big_endian(OrbitsInfo *info, int dir)
{
    std::vector<unsigned char> orbits_info_buff;
    unsigned char *buf_ptr;

    orbits_info_buff.resize(sizeof(OrbitsInfo));
    unsigned char *buf = orbits_info_buff.data();
    buf_ptr = buf;
    std::memcpy((char *)buf, (char *)info, sizeof(OrbitsInfo));

    get_double(&info->orbit_corner_min_x, &buf_ptr, dir);
    get_double(&info->orbit_corner_max_x, &buf_ptr, dir);
    get_double(&info->orbit_corner_min_y, &buf_ptr, dir);
    get_double(&info->orbit_corner_max_y, &buf_ptr, dir);
    get_double(&info->orbit_corner_3rd_x, &buf_ptr, dir);
    get_double(&info->orbit_corner_3rd_y, &buf_ptr, dir);
    get_int16(&info->keep_screen_coords, &buf_ptr, dir);
    get_uint8((unsigned char *) &info->draw_mode, &buf_ptr, dir);
    get_uint8((unsigned char *) &info->dummy, &buf_ptr, dir);

    for (int i = 0; i < (sizeof(info->future)/sizeof(short)); i++)  // NOLINT(modernize-loop-convert)
    {
        get_int16(&info->future[i], &buf_ptr, dir);
    }
    if (buf_ptr-buf != sizeof(OrbitsInfo))
    {
        std::printf("Warning: loadfile miscount on orbits_info structure.\n");
        std::printf("Components add up to %d bytes, but sizeof(OrbitsInfo) = %d\n",
               (int)(buf_ptr-buf), (int) sizeof(OrbitsInfo));
    }
    if (dir == 0)
    {
        std::memcpy((char *)info, (char *)buf, sizeof(OrbitsInfo));
    }
}
