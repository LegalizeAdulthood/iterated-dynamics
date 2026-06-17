// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/decode_info.h"

#include "io/gif_extensions.h"
#include "ui/evolve.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace id::ui;

namespace id::io
{

constexpr int DOS_FLOAT_SIZE = 4;
constexpr int DOS_DOUBLE_SIZE = 8;

// Assumptions about the floating-point types used in the blob structures.
static_assert(sizeof(float) == DOS_FLOAT_SIZE, "sizeof(float) != DOS_FLOAT_SIZE");
static_assert(sizeof(double) == DOS_DOUBLE_SIZE, "sizeof(double) != DOS_DOUBLE_SIZE");

// The size of these structures must remain fixed in order to maintain
// compatibility with the binary blobs written into GIF files.
//
// TODO: verify that size of binary EVOLUTION_INFO blob is 200; DOS code reported size of 208.
//
static_assert(sizeof(EvolutionInfo) == 200, "EvolutionInfo size is incorrect");
static_assert(sizeof(FractalInfo) == 504, "FractalInfo size is incorrect");
static_assert(sizeof(FractalInfo) == 504, "FractalInfo size is incorrect");
static_assert(sizeof(OrbitsInfo) == 200, "OrbitsInfo size is incorrect");

/* --------------------------------------------------------------------
 * The following routines are used for encoding/decoding gif images.
 * If we aren't on a PC, things are rough for decoding the fractal info
 * structure in the GIF file.  These routines look after converting the
 * MS_DOS format data into a form we can use.
 * If dir==TO_FILE, we convert to file format (little-endian) form.
 * Otherwise, we convert from file format (little-endian) form.
 */

#if ID_BIG_ENDIAN
/*
 * This routine gets a char out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static std::uint8_t decode_uint8(unsigned char **const src)
{
    std::uint8_t result = **src;
    (*src)++;
    return result;
}

static void encode_uint8(const uint8_t val, unsigned char **dst)
{
    **dst = val;
    (*dst)++;
}

/*
 * This routine gets an int16_t out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static std::int16_t decode_int16(unsigned char **const src)
{
    std::int16_t result = static_cast<int16_t>((*src)[0] + (reinterpret_cast<const char *>(*src)[1] << 8));
    *src += sizeof(std::uint16_t);
    return result;
}

static void encode_int16(const int16_t val, unsigned char **dst)
{
    (*dst)[0] = val & 0xff;
    (*dst)[1] = (val & 0xff00) >> 8;
    *dst += sizeof(std::uint16_t);
}

/*
 * This routine gets a uint16_t out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static std::uint16_t decode_uint16(unsigned char **const src)
{
    std::uint16_t dst = static_cast<std::uint16_t>((*src)[0]) + (static_cast<std::uint16_t>((*src)[1] << 8));
    *src += sizeof(std::uint16_t);
    return dst;
}

static void encode_uint16(const uint16_t val, unsigned char **dest)
{
    (*dest)[0] = val & 0xff;
    (*dest)[1] = (val & 0xff00) >> 8;
    *dest += sizeof(std::uint16_t);
}

/*
 * This routine gets a long out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static std::int32_t decode_int32(unsigned char **src)
{
    std::int32_t result = static_cast<unsigned long>((*src)[0]) //
        + (static_cast<unsigned long>((*src)[1]) << 8)          //
        + (static_cast<unsigned long>((*src)[2]) << 16)         //
        + (static_cast<long>(reinterpret_cast<char *>(*src)[3]) << 24);
    *src += sizeof(int32_t);
    return result;
}

static void encode_int32(const int32_t val, unsigned char **dst)
{
    (*dst)[0] = val & 0xff;
    (*dst)[1] = (val & 0xff00) >> 8;
    (*dst)[2] = (val & 0xff0000) >> 16;
    (*dst)[3] = (val & 0xff000000) >> 24;
    *dst += sizeof(std::int32_t);
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
static double decode_double(unsigned char **src)
{
    double val{};
    int e;
    double f;
    int i;
    for (i = 0; i < DOS_DOUBLE_SIZE; i++)
    {
        if ((*src)[i] != 0)
        {
            break;
        }
    }
    if (i == DOS_DOUBLE_SIZE)
    {
        val = 0;
    }
    else
    {
        e = (((*src)[7] & 0x7f) << 4) + (((*src)[6] & 0xf0) >> 4) - 1023;
        f = 1 + ((*src)[6] & 0x0f) / P4 + (*src)[5] / P12 + (*src)[4] / P20 + (*src)[3] / P28 + (*src)[2] / P36 +
            (*src)[1] / P44 + (*src)[0] / P52;
        f *= pow(2., static_cast<double>(e));
        if ((*src)[7] & 0x80)
        {
            f = -f;
        }
        val = f;
    }
    *src += DOS_DOUBLE_SIZE;
    return val;
}

static void encode_double(const double val, unsigned char **dst)
{
    int e;
    double f;
    if (val == 0.0)
    {
        std::memset(*dst, 0, DOS_DOUBLE_SIZE);
    }
    else
    {
        int s = 0;
        f = val;
        if (f < 0)
        {
            s = 0x80;
            f = -f;
        }
        e = std::log(f) / std::log(2.);
        f = f / std::pow(2., static_cast<double>(e)) - 1;
        if (f < 0)
        {
            e--;
            f = (f + 1) * 2 - 1;
        }
        else if (f >= 1)
        {
            e++;
            f = (f + 1) / 2 - 1;
        }
        e += 1023;
        (*dst)[7] = s | (e & 0x7f0) >> 4;
        f *= P4;
        (*dst)[6] = (e & 0x0f) << 4 | static_cast<int>(f) & 0x0f;
        f = (f - static_cast<int>(f)) * P8;
        (*dst)[5] = static_cast<int>(f) & 0xff;
        f = (f - static_cast<int>(f)) * P8;
        (*dst)[4] = static_cast<int>(f) & 0xff;
        f = (f - static_cast<int>(f)) * P8;
        (*dst)[3] = static_cast<int>(f) & 0xff;
        f = (f - static_cast<int>(f)) * P8;
        (*dst)[2] = static_cast<int>(f) & 0xff;
        f = (f - static_cast<int>(f)) * P8;
        (*dst)[1] = static_cast<int>(f) & 0xff;
        f = (f - static_cast<int>(f)) * P8;
        (*dst)[0] = static_cast<int>(f) & 0xff;
    }
    *dst += DOS_DOUBLE_SIZE;
}

/*
 * This routine gets a float out of the buffer.
 * It updates the buffer pointer accordingly.
 */
static float decode_float(unsigned char **src)
{
    float val{};
    int e;
    double f;
    int i;
    for (i = 0; i < DOS_FLOAT_SIZE; i++)
    {
        if ((*src)[i] != 0)
        {
            break;
        }
    }
    if (i == DOS_FLOAT_SIZE)
    {
        val = 0;
    }
    else
    {
        e = (((*src)[3] & 0x7f) << 1 | ((*src)[2] & 0x80) >> 7) - 127;
        f = 1 + ((*src)[2] & 0x7f) / P7 + (*src)[1] / P15 + (*src)[0] / P23;
        f *= pow(2., static_cast<double>(e));
        if ((*src)[3] & 0x80)
        {
            f = -f;
        }
        val = f;
    }
    *src += DOS_FLOAT_SIZE;
    return val;
}

static void encode_float(const float val, unsigned char **dst)
{
    int e;
    double f;
    if (val == 0)
    {
        std::memset(*dst, 0, DOS_FLOAT_SIZE);
    }
    else
    {
        int s = 0;
        f = val;
        if (f < 0)
        {
            s = 0x80;
            f = -f;
        }
        e = std::log(f) / std::log(2.);
        f = f / std::pow(2., static_cast<double>(e)) - 1;
        if (f < 0)
        {
            e--;
            f = (f + 1) * 2 - 1;
        }
        else if (f >= 1)
        {
            e++;
            f = (f + 1) / 2 - 1;
        }
        e += 127;
        (*dst)[3] = s | (e & 0xf7) >> 1;
        f *= P7;
        (*dst)[2] = (e & 0x01) << 7 | static_cast<int>(f) & 0x7f;
        f = (f - static_cast<int>(f)) * P8;
        (*dst)[1] = static_cast<int>(f) & 0xff;
        f = (f - static_cast<int>(f)) * P8;
        (*dst)[0] = static_cast<int>(f) & 0xff;
    }
    *dst += DOS_FLOAT_SIZE;
}

namespace
{

template <typename T>
unsigned char *copy_to_buffer(std::vector<unsigned char> &info_buff, const T &src)
{
    info_buff.resize(sizeof(T));
    unsigned char *buf = info_buff.data();
    std::memcpy(buf, &src, sizeof(T));
    return buf;
}

template <typename T>
void copy_from_buffer(T &info, const std::vector<unsigned char> &buffer)
{
    void *dest = static_cast<void*>(&info);
    std::memcpy(dest, buffer.data(), sizeof(T));
}

} // namespace

void decode_fractal_info_big_endian(FractalInfo &info)
{
    std::vector<unsigned char> buffer;
    unsigned char *buf_ptr = copy_to_buffer(buffer, info);

    std::copy_n(reinterpret_cast<const char *>(buf_ptr), sizeof(info.info_id), info.info_id);
    buf_ptr += 8;
    info.iterations_old = decode_int16(&buf_ptr);
    info.fractal_type = decode_int16(&buf_ptr);
    info.x_min = decode_double(&buf_ptr);
    info.x_max = decode_double(&buf_ptr);
    info.y_min = decode_double(&buf_ptr);
    info.y_max = decode_double(&buf_ptr);
    info.c_real = decode_double(&buf_ptr);
    info.c_imag = decode_double(&buf_ptr);
    info.ax = decode_int16(&buf_ptr);
    info.bx = decode_int16(&buf_ptr);
    info.cx = decode_int16(&buf_ptr);
    info.dx = decode_int16(&buf_ptr);
    info.dot_mode = decode_int16(&buf_ptr);
    info.x_dots = decode_uint16(&buf_ptr);
    info.y_dots = decode_uint16(&buf_ptr);
    info.colors = decode_int16(&buf_ptr);
    info.info_version = decode_int16(&buf_ptr);
    info.param3 = decode_float(&buf_ptr);
    info.param4 = decode_float(&buf_ptr);
    info.potential[0] = decode_float(&buf_ptr);
    info.potential[1] = decode_float(&buf_ptr);
    info.potential[2] = decode_float(&buf_ptr);
    info.random_seed = decode_int16(&buf_ptr);
    info.random_seed_flag = decode_int16(&buf_ptr);
    info.biomorph = decode_int16(&buf_ptr);
    info.inside = decode_int16(&buf_ptr);
    {
        short tmp = 0;
        tmp = decode_int16(&buf_ptr);
        info.log_map = tmp;
    }
    info.invert[0] = decode_float(&buf_ptr);
    info.invert[1] = decode_float(&buf_ptr);
    info.invert[2] = decode_float(&buf_ptr);
    info.decomp[0] = decode_int16(&buf_ptr);
    info.decomp[1] = decode_int16(&buf_ptr);
    info.symmetry = decode_int16(&buf_ptr);
    for (int i = 0; i < 16; i++) // NOLINT(modernize-loop-convert)
    {
        info.init3d[i] = decode_int16(&buf_ptr);
    }
    info.preview_factor = decode_int16(&buf_ptr);
    info.x_trans = decode_int16(&buf_ptr);
    info.y_trans = decode_int16(&buf_ptr);
    info.red_crop_left = decode_int16(&buf_ptr);
    info.red_crop_right = decode_int16(&buf_ptr);
    info.blue_crop_left = decode_int16(&buf_ptr);
    info.blue_crop_right = decode_int16(&buf_ptr);
    info.red_bright = decode_int16(&buf_ptr);
    info.blue_bright = decode_int16(&buf_ptr);
    info.x_adjust = decode_int16(&buf_ptr);
    info.eye_separation = decode_int16(&buf_ptr);
    info.glasses_type = decode_int16(&buf_ptr);
    info.outside = decode_int16(&buf_ptr);
    info.x3rd = decode_double(&buf_ptr);
    info.y3rd = decode_double(&buf_ptr);
    info.std_calc_mode = static_cast<char>(decode_uint8(&buf_ptr));
    info.use_init_orbit = static_cast<char>(decode_uint8(&buf_ptr));
    info.calc_status = decode_int16(&buf_ptr);
    info.tot_extend_len = decode_int32(&buf_ptr);
    {
        short tmp = 0;
        tmp = decode_int16(&buf_ptr);
        info.dist_est = tmp;
    }
    info.float_flag = decode_int16(&buf_ptr);
    info.bailout_old = decode_int16(&buf_ptr);
    info.calc_time = decode_int32(&buf_ptr);
    for (int i = 0; i < 4; i++) // NOLINT(modernize-loop-convert)
    {
        info.trig_index[i] = decode_uint8(&buf_ptr);
    }
    info.finite_attractor = decode_int16(&buf_ptr);
    info.init_orbit[0] = decode_double(&buf_ptr);
    info.init_orbit[1] = decode_double(&buf_ptr);
    info.periodicity = decode_int16(&buf_ptr);
    info.pot16bit = decode_int16(&buf_ptr);
    info.final_aspect_ratio = decode_float(&buf_ptr);
    info.system = decode_int16(&buf_ptr);
    info.release = decode_int16(&buf_ptr);
    info.display_3d = decode_int16(&buf_ptr);
    info.transparent[0] = decode_int16(&buf_ptr);
    info.transparent[1] = decode_int16(&buf_ptr);
    info.ambient = decode_int16(&buf_ptr);
    info.haze = decode_int16(&buf_ptr);
    info.randomize = decode_int16(&buf_ptr);
    info.rotate_lo = decode_int16(&buf_ptr);
    info.rotate_hi = decode_int16(&buf_ptr);
    info.dist_est_width = decode_int16(&buf_ptr);
    info.d_param3 = decode_double(&buf_ptr);
    info.d_param4 = decode_double(&buf_ptr);
    info.fill_color = decode_int16(&buf_ptr);
    info.julibrot_x_max = decode_double(&buf_ptr);
    info.julibrot_x_min = decode_double(&buf_ptr);
    info.julibrot_y_max = decode_double(&buf_ptr);
    info.julibrot_y_min = decode_double(&buf_ptr);
    info.julibrot_z_dots = decode_int16(&buf_ptr);
    info.julibrot_origin_fp = decode_float(&buf_ptr);
    info.julibrot_depth_fp = decode_float(&buf_ptr);
    info.julibrot_height_fp = decode_float(&buf_ptr);
    info.julibrot_width_fp = decode_float(&buf_ptr);
    info.julibrot_dist_fp = decode_float(&buf_ptr);
    info.eyes_fp = decode_float(&buf_ptr);
    info.orbit_type = decode_int16(&buf_ptr);
    info.juli3d_mode = decode_int16(&buf_ptr);
    info.max_fn = decode_int16(&buf_ptr);
    info.inverse_julia = decode_int16(&buf_ptr);
    info.d_param5 = decode_double(&buf_ptr);
    info.d_param6 = decode_double(&buf_ptr);
    info.d_param7 = decode_double(&buf_ptr);
    info.d_param8 = decode_double(&buf_ptr);
    info.d_param9 = decode_double(&buf_ptr);
    info.d_param10 = decode_double(&buf_ptr);
    info.bailout = decode_int32(&buf_ptr);
    info.bailout_test = decode_int16(&buf_ptr);
    info.iterations = decode_int32(&buf_ptr);
    info.bf_math = decode_int16(&buf_ptr);
    info.bf_length = decode_int16(&buf_ptr);
    info.y_adjust = decode_int16(&buf_ptr);
    info.old_demm_colors = decode_int16(&buf_ptr);
    info.log_map = decode_int32(&buf_ptr);
    info.dist_est = decode_int32(&buf_ptr);
    info.d_invert[0] = decode_double(&buf_ptr);
    info.d_invert[1] = decode_double(&buf_ptr);
    info.d_invert[2] = decode_double(&buf_ptr);
    info.log_calc = decode_int16(&buf_ptr);
    info.stop_pass = decode_int16(&buf_ptr);
    info.quick_calc = decode_int16(&buf_ptr);
    info.close_prox = decode_double(&buf_ptr);
    info.no_bof = decode_int16(&buf_ptr);
    info.orbit_interval = decode_int32(&buf_ptr);
    info.orbit_delay = decode_int16(&buf_ptr);
    info.math_tol[0] = decode_double(&buf_ptr);
    info.math_tol[1] = decode_double(&buf_ptr);
    info.version_major = decode_uint8(&buf_ptr);
    info.version_minor = decode_uint8(&buf_ptr);
    info.version_patch = decode_uint8(&buf_ptr);
    info.version_tweak = decode_uint8(&buf_ptr);

    for (int i = 0; i < sizeof(info.future) / sizeof(short); i++) // NOLINT(modernize-loop-convert)
    {
        info.future[i] = decode_int16(&buf_ptr);
    }
    if (const auto len = buf_ptr - buffer.data(); len != sizeof(FractalInfo))
    {
        fmt::print("Warning: loadfile miscount on fractal_info structure.\n"
                   "Components add up to {:d} bytes, but sizeof(FractalInfo) = {:d}\n",
            static_cast<int>(len), sizeof(FractalInfo));
    }
}

void encode_fractal_info_big_endian(FractalInfo &info)
{
    std::vector<unsigned char> buffer;
    unsigned char *buf_ptr = copy_to_buffer(buffer, info);

    std::copy_n(info.info_id, sizeof(info.info_id), reinterpret_cast<char *>(buf_ptr));
    buf_ptr += 8;
    encode_int16(info.iterations_old, &buf_ptr);
    encode_int16(info.fractal_type, &buf_ptr);
    encode_double(info.x_min, &buf_ptr);
    encode_double(info.x_max, &buf_ptr);
    encode_double(info.y_min, &buf_ptr);
    encode_double(info.y_max, &buf_ptr);
    encode_double(info.c_real, &buf_ptr);
    encode_double(info.c_imag, &buf_ptr);
    encode_int16(info.ax, &buf_ptr);
    encode_int16(info.bx, &buf_ptr);
    encode_int16(info.cx, &buf_ptr);
    encode_int16(info.dx, &buf_ptr);
    encode_int16(info.dot_mode, &buf_ptr);
    encode_uint16(info.x_dots, &buf_ptr);
    encode_uint16(info.y_dots, &buf_ptr);
    encode_int16(info.colors, &buf_ptr);
    encode_int16(info.info_version, &buf_ptr);
    encode_float(info.param3, &buf_ptr);
    encode_float(info.param4, &buf_ptr);
    encode_float(info.potential[0], &buf_ptr);
    encode_float(info.potential[1], &buf_ptr);
    encode_float(info.potential[2], &buf_ptr);
    encode_int16(info.random_seed, &buf_ptr);
    encode_int16(info.random_seed_flag, &buf_ptr);
    encode_int16(info.biomorph, &buf_ptr);
    encode_int16(info.inside, &buf_ptr);
    encode_int16(static_cast<std::int16_t>(info.log_map), &buf_ptr);
    encode_float(info.invert[0], &buf_ptr);
    encode_float(info.invert[1], &buf_ptr);
    encode_float(info.invert[2], &buf_ptr);
    encode_int16(info.decomp[0], &buf_ptr);
    encode_int16(info.decomp[1], &buf_ptr);
    encode_int16(info.symmetry, &buf_ptr);
    for (int i = 0; i < 16; i++) // NOLINT(modernize-loop-convert)
    {
        encode_int16(info.init3d[i], &buf_ptr);
    }
    encode_int16(info.preview_factor, &buf_ptr);
    encode_int16(info.x_trans, &buf_ptr);
    encode_int16(info.y_trans, &buf_ptr);
    encode_int16(info.red_crop_left, &buf_ptr);
    encode_int16(info.red_crop_right, &buf_ptr);
    encode_int16(info.blue_crop_left, &buf_ptr);
    encode_int16(info.blue_crop_right, &buf_ptr);
    encode_int16(info.red_bright, &buf_ptr);
    encode_int16(info.blue_bright, &buf_ptr);
    encode_int16(info.x_adjust, &buf_ptr);
    encode_int16(info.eye_separation, &buf_ptr);
    encode_int16(info.glasses_type, &buf_ptr);
    encode_int16(info.outside, &buf_ptr);
    encode_double(info.x3rd, &buf_ptr);
    encode_double(info.y3rd, &buf_ptr);
    encode_uint8(static_cast<std::uint8_t>(info.std_calc_mode), &buf_ptr);
    encode_uint8(static_cast<std::uint8_t>(info.use_init_orbit), &buf_ptr);
    encode_int16(info.calc_status, &buf_ptr);
    encode_int32(info.tot_extend_len, &buf_ptr);
    encode_int16(static_cast<std::int16_t>(info.dist_est), &buf_ptr);
    encode_int16(info.float_flag, &buf_ptr);
    encode_int16(info.bailout_old, &buf_ptr);
    encode_int32(info.calc_time, &buf_ptr);
    for (int i = 0; i < 4; i++) // NOLINT(modernize-loop-convert)
    {
        encode_uint8(info.trig_index[i], &buf_ptr);
    }
    encode_int16(info.finite_attractor, &buf_ptr);
    encode_double(info.init_orbit[0], &buf_ptr);
    encode_double(info.init_orbit[1], &buf_ptr);
    encode_int16(info.periodicity, &buf_ptr);
    encode_int16(info.pot16bit, &buf_ptr);
    encode_float(info.final_aspect_ratio, &buf_ptr);
    encode_int16(info.system, &buf_ptr);
    encode_int16(info.release, &buf_ptr);
    encode_int16(info.display_3d, &buf_ptr);
    encode_int16(info.transparent[0], &buf_ptr);
    encode_int16(info.transparent[1], &buf_ptr);
    encode_int16(info.ambient, &buf_ptr);
    encode_int16(info.haze, &buf_ptr);
    encode_int16(info.randomize, &buf_ptr);
    encode_int16(info.rotate_lo, &buf_ptr);
    encode_int16(info.rotate_hi, &buf_ptr);
    encode_int16(info.dist_est_width, &buf_ptr);
    encode_double(info.d_param3, &buf_ptr);
    encode_double(info.d_param4, &buf_ptr);
    encode_int16(info.fill_color, &buf_ptr);
    encode_double(info.julibrot_x_max, &buf_ptr);
    encode_double(info.julibrot_x_min, &buf_ptr);
    encode_double(info.julibrot_y_max, &buf_ptr);
    encode_double(info.julibrot_y_min, &buf_ptr);
    encode_int16(info.julibrot_z_dots, &buf_ptr);
    encode_float(info.julibrot_origin_fp, &buf_ptr);
    encode_float(info.julibrot_depth_fp, &buf_ptr);
    encode_float(info.julibrot_height_fp, &buf_ptr);
    encode_float(info.julibrot_width_fp, &buf_ptr);
    encode_float(info.julibrot_dist_fp, &buf_ptr);
    encode_float(info.eyes_fp, &buf_ptr);
    encode_int16(info.orbit_type, &buf_ptr);
    encode_int16(info.juli3d_mode, &buf_ptr);
    encode_int16(info.max_fn, &buf_ptr);
    encode_int16(info.inverse_julia, &buf_ptr);
    encode_double(info.d_param5, &buf_ptr);
    encode_double(info.d_param6, &buf_ptr);
    encode_double(info.d_param7, &buf_ptr);
    encode_double(info.d_param8, &buf_ptr);
    encode_double(info.d_param9, &buf_ptr);
    encode_double(info.d_param10, &buf_ptr);
    encode_int32(info.bailout, &buf_ptr);
    encode_int16(info.bailout_test, &buf_ptr);
    encode_int32(info.iterations, &buf_ptr);
    encode_int16(info.bf_math, &buf_ptr);
    encode_int16(info.bf_length, &buf_ptr);
    encode_int16(info.y_adjust, &buf_ptr);
    encode_int16(info.old_demm_colors, &buf_ptr);
    encode_int32(info.log_map, &buf_ptr);
    encode_int32(info.dist_est, &buf_ptr);
    encode_double(info.d_invert[0], &buf_ptr);
    encode_double(info.d_invert[1], &buf_ptr);
    encode_double(info.d_invert[2], &buf_ptr);
    encode_int16(info.log_calc, &buf_ptr);
    encode_int16(info.stop_pass, &buf_ptr);
    encode_int16(info.quick_calc, &buf_ptr);
    encode_double(info.close_prox, &buf_ptr);
    encode_int16(info.no_bof, &buf_ptr);
    encode_int32(info.orbit_interval, &buf_ptr);
    encode_int16(info.orbit_delay, &buf_ptr);
    encode_double(info.math_tol[0], &buf_ptr);
    encode_double(info.math_tol[1], &buf_ptr);
    encode_uint8(info.version_major, &buf_ptr);
    encode_uint8(info.version_minor, &buf_ptr);
    encode_uint8(info.version_patch, &buf_ptr);
    encode_uint8(info.version_tweak, &buf_ptr);

    for (int i = 0; i < sizeof(info.future) / sizeof(short); i++) // NOLINT(modernize-loop-convert)
    {
        encode_int16(info.future[i], &buf_ptr);
    }
    if (const auto len = buf_ptr - buffer.data() != sizeof(FractalInfo))
    {
        fmt::print("Warning: loadfile miscount on fractal_info structure.\n"
                   "Components add up to {:d} bytes, but sizeof(FractalInfo) = {:d}\n",
            static_cast<int>(len), sizeof(FractalInfo));
    }
    copy_from_buffer(info, buffer);
}

void decode_evolver_info_big_endian(EvolutionInfo &info)
{
    std::vector<unsigned char> buffer;
    unsigned char *buf_ptr = copy_to_buffer(buffer, info);

    info.evolving = decode_int16(&buf_ptr);
    info.image_grid_size = decode_int16(&buf_ptr);
    info.this_generation_random_seed = static_cast<std::uint16_t>(decode_int16(&buf_ptr));
    info.max_random_mutation = decode_double(&buf_ptr);
    info.x_parameter_range = decode_double(&buf_ptr);
    info.y_parameter_range = decode_double(&buf_ptr);
    info.x_parameter_offset = decode_double(&buf_ptr);
    info.y_parameter_offset = decode_double(&buf_ptr);
    info.discrete_x_parameter_offset = decode_int16(&buf_ptr);
    info.discrete_y_parameter_offset = decode_int16(&buf_ptr);
    info.px = decode_int16(&buf_ptr);
    info.py = decode_int16(&buf_ptr);
    info.screen_x_offset = decode_int16(&buf_ptr);
    info.screen_y_offset = decode_int16(&buf_ptr);
    info.x_dots = decode_uint16(&buf_ptr);
    info.y_dots = decode_uint16(&buf_ptr);
    for (int i = 0; i < NUM_GENES; i++)  // NOLINT(modernize-loop-convert)
    {
        info.mutate[i] = decode_int16(&buf_ptr);
    }
    info.count = decode_int16(&buf_ptr);

    for (int i = 0; i < sizeof(info.future) / sizeof(short); i++)  // NOLINT(modernize-loop-convert)
    {
        info.future[i] = decode_int16(&buf_ptr);
    }
    if (const auto len = buf_ptr - buffer.data(); len != sizeof(EvolutionInfo))
    {
        fmt::print("Warning: loadfile miscount on evolution_info structure.\n"
                   "Components add up to {:d} bytes, but sizeof(EVOLUTION_INFO) = {:d}\n",
            static_cast<int>(len), sizeof(EvolutionInfo));
    }
}

void encode_evolver_info_big_endian(EvolutionInfo &info)
{
    std::vector<unsigned char> buffer;
    unsigned char *buf_ptr = copy_to_buffer(buffer, info);

    encode_int16(info.evolving, &buf_ptr);
    encode_int16(info.image_grid_size, &buf_ptr);
    encode_int16(static_cast<std::int16_t>(info.this_generation_random_seed), &buf_ptr);
    encode_double(info.max_random_mutation, &buf_ptr);
    encode_double(info.x_parameter_range, &buf_ptr);
    encode_double(info.y_parameter_range, &buf_ptr);
    encode_double(info.x_parameter_offset, &buf_ptr);
    encode_double(info.y_parameter_offset, &buf_ptr);
    encode_int16(info.discrete_x_parameter_offset, &buf_ptr);
    encode_int16(info.discrete_y_parameter_offset, &buf_ptr);
    encode_int16(info.px, &buf_ptr);
    encode_int16(info.py, &buf_ptr);
    encode_int16(info.screen_x_offset, &buf_ptr);
    encode_int16(info.screen_y_offset, &buf_ptr);
    encode_uint16(info.x_dots, &buf_ptr);
    encode_uint16(info.y_dots, &buf_ptr);
    for (int i = 0; i < NUM_GENES; i++)  // NOLINT(modernize-loop-convert)
    {
        encode_int16(info.mutate[i], &buf_ptr);
    }
    encode_int16(info.count, &buf_ptr);

    for (int i = 0; i < sizeof(info.future) / sizeof(short); i++)  // NOLINT(modernize-loop-convert)
    {
        encode_int16(info.future[i], &buf_ptr);
    }
    if (const auto len = buf_ptr - buffer.data() != sizeof(EvolutionInfo))
    {
        fmt::print("Warning: loadfile miscount on evolution_info structure.\n"
                   "Components add up to {:d} bytes, but sizeof(EVOLUTION_INFO) = {:d}\n",
            static_cast<int>(len), sizeof(EvolutionInfo));
    }
    copy_from_buffer(info, buffer);
}

void decode_orbits_info_big_endian(OrbitsInfo &info)
{
    std::vector<unsigned char> buffer;
    unsigned char *buf_ptr = copy_to_buffer(buffer, info);

    info.orbit_corner_min_x = decode_double(&buf_ptr);
    info.orbit_corner_max_x = decode_double(&buf_ptr);
    info.orbit_corner_min_y = decode_double(&buf_ptr);
    info.orbit_corner_max_y = decode_double(&buf_ptr);
    info.orbit_corner_3rd_x = decode_double(&buf_ptr);
    info.orbit_corner_3rd_y = decode_double(&buf_ptr);
    info.keep_screen_coords = decode_int16(&buf_ptr);
    info.draw_mode = static_cast<char>(decode_uint8(&buf_ptr));
    info.dummy = static_cast<char>(decode_uint8(&buf_ptr));

    for (int i = 0; i < sizeof(info.future) / sizeof(short); i++)  // NOLINT(modernize-loop-convert)
    {
        info.future[i] = decode_int16(&buf_ptr);
    }
    if (const auto len = buf_ptr - buffer.data(); len != sizeof(OrbitsInfo))
    {
        fmt::print("Warning: loadfile miscount on orbits_info structure.\n"
                   "Components add up to {:d} bytes, but sizeof(OrbitsInfo) = {:d}\n",
            static_cast<int>(len), sizeof(OrbitsInfo));
    }
}

void encode_orbits_info_big_endian(OrbitsInfo &info)
{
    std::vector<unsigned char> buffer;
    unsigned char *buf_ptr = copy_to_buffer(buffer, info);

    encode_double(info.orbit_corner_min_x, &buf_ptr);
    encode_double(info.orbit_corner_max_x, &buf_ptr);
    encode_double(info.orbit_corner_min_y, &buf_ptr);
    encode_double(info.orbit_corner_max_y, &buf_ptr);
    encode_double(info.orbit_corner_3rd_x, &buf_ptr);
    encode_double(info.orbit_corner_3rd_y, &buf_ptr);
    encode_int16(info.keep_screen_coords, &buf_ptr);
    encode_uint8(static_cast<std::uint8_t>(info.draw_mode), &buf_ptr);
    encode_uint8(static_cast<std::uint8_t>(info.dummy), &buf_ptr);

    for (int i = 0; i < sizeof(info.future) / sizeof(short); i++)  // NOLINT(modernize-loop-convert)
    {
        encode_int16(info.future[i], &buf_ptr);
    }
    if (const auto len = buf_ptr - buffer.data(); len != sizeof(OrbitsInfo))
    {
        fmt::print("Warning: loadfile miscount on orbits_info structure.\n"
                   "Components add up to {:d} bytes, but sizeof(OrbitsInfo) = {:d}\n",
            static_cast<int>(len), sizeof(OrbitsInfo));
    }
    copy_from_buffer(info, buffer);
}
#endif

} // namespace id::io
