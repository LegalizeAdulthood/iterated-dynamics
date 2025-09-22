// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/gif_file.h"

#include <config/port.h>

#include <boost/endian/buffers.hpp>
#include <boost/endian/conversion.hpp>
#include <gif_lib.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace id::ui;

namespace id::io
{

static std::int16_t extract_int16(const unsigned char *src)
{
    return boost::endian::load_little_s16(src);
}

static void insert_int16(unsigned char *dest, const std::int16_t value)
{
    boost::endian::store_little_s16(dest, value);
}

static bool is_fractint_extension(const ExtensionBlock &block, const char *name)
{
    return block.Function == APPLICATION_EXT_FUNC_CODE                           //
        && block.ByteCount == 11                                                 //
        && std::string(reinterpret_cast<const char *>(block.Bytes), 11) == name; //
}

static void add_gif_extension(GifFileType *gif, const char *name, unsigned char *bytes, const int length)
{
    GifAddExtensionBlock(&gif->ExtensionBlockCount, &gif->ExtensionBlocks, APPLICATION_EXT_FUNC_CODE, //
        11, reinterpret_cast<unsigned char *>(const_cast<char *>(name)));
    unsigned int count{static_cast<unsigned int>(length)};
    unsigned int offset{};
    while (count > 0)
    {
        const unsigned int chunk{count > 255 ? 255 : count};
        GifAddExtensionBlock(&gif->ExtensionBlockCount, &gif->ExtensionBlocks, CONTINUE_EXT_FUNC_CODE, //
            chunk, bytes + offset);
        count -= chunk;
        offset += chunk;
    }
}

template <int Size>
class ExtensionDeserializer
{
public:
    ExtensionDeserializer(const GifFileType *gif, const char *name);

    template <int N>
    void extract_chars(char (&dest)[N])
    {
        extract_chars(dest, N);
    }
    void extract_chars(char *dest, const int count)
    {
        check_overflow(count);
        std::copy_n(current(), count, reinterpret_cast<unsigned char *>(dest));
        advance(count);
    }

    std::uint8_t extract_byte()
    {
        check_overflow(1);
        const std::uint8_t result = m_bytes[m_offset];
        advance(1);
        return result;
    }
    template <int N>
    void extract_byte(std::uint8_t (&values)[N])
    {
        for (std::uint8_t &value : values)
        {
            value = extract_byte();
        }
    }

    std::int16_t extract_int16()
    {
        check_overflow(2);
        const std::int16_t result = io::extract_int16(current());
        advance(2);
        return result;
    }
    template <int N>
    void extract_int16(std::int16_t (&values)[N])
    {
        for (std::int16_t &value : values)
        {
            value = extract_int16();
        }
    }

    std::uint16_t extract_uint16()
    {
        check_overflow(2);
        const std::uint16_t result = boost::endian::load_little_u16(current());
        advance(2);
        return result;
    }

    std::int32_t extract_int32()
    {
        check_overflow(4);
        const std::int32_t result = boost::endian::load_little_s32(current());
        advance(4);
        return result;
    }

    float extract_float()
    {
        check_overflow(4);
        boost::endian::little_float32_buf_t value;
        std::copy_n(current(), 4, value.data());
        advance(4);
        return value.value();
    }
    template <int N>
    void extract_float(float (&values)[N])
    {
        for (float &value : values)
        {
            value = extract_float();
        }
    }

    double extract_double()
    {
        check_overflow(8);
        boost::endian::little_float64_buf_t value;
        std::copy_n(current(), 8, value.data());
        advance(8);
        return value.value();
    }
    template <int N>
    void extract_double(double (&values)[N])
    {
        for (double &value : values)
        {
            value = extract_double();
        }
    }

    void validate()
    {
        if (m_count != 0)
        {
            throw std::runtime_error("Expected zero count, but " + std::to_string(m_count) + " bytes remaining.");
        }
    }

private:
    const std::uint8_t *current() const
    {
        return m_bytes.data() + m_offset;
    }
    void check_overflow(const int count)
    {
        if (m_count < count)
        {
            throw std::runtime_error("Buffer overflow");
        }
    }
    void advance(const int size)
    {
        m_offset += size;
        m_count -= size;
    }

    int m_offset{};
    int m_count{Size};
    std::array<std::uint8_t, Size> m_bytes{};
};

template <int Size>
ExtensionDeserializer<Size>::ExtensionDeserializer(const GifFileType *gif, const char *name)
{
    int count{Size};
    for (int i = 0; i < gif->ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &block{gif->ExtensionBlocks[i]};
        if (is_fractint_extension(block, name))
        {
            int ext{i + 1};
            int offset{};
            while (count > 0                                                     //
                && ext < gif->ExtensionBlockCount                                //
                && gif->ExtensionBlocks[ext].Function == CONTINUE_EXT_FUNC_CODE) //
            {
                const ExtensionBlock &src = gif->ExtensionBlocks[ext];
                std::copy_n(src.Bytes, src.ByteCount, m_bytes.begin() + offset);
                offset += src.ByteCount;
                count -= src.ByteCount;
                ++ext;
            }
            if (count != 0)
            {
                throw std::runtime_error("Expected " + std::to_string(Size) + " bytes, got overflow of " +
                    std::to_string(-count) + " bytes");
            }
            break;
        }
    }
    if (count != 0)
    {
        throw std::runtime_error(
            "Expected " + std::to_string(Size) + " bytes, got only " + std::to_string(Size - count) + " bytes");
    }
}

FractalInfo get_fractal_info(GifFileType *gif)
{
    FractalInfo result{};
    ExtensionDeserializer<GIF_EXTENSION1_FRACTAL_INFO_LENGTH> deser(gif, "fractint001");
    deser.extract_chars(result.info_id);
    result.iterations_old = deser.extract_int16();
    result.fractal_type = deser.extract_int16();
    result.x_min = deser.extract_double();
    result.x_max = deser.extract_double();
    result.y_min = deser.extract_double();
    result.y_max = deser.extract_double();
    result.c_real = deser.extract_double();
    result.c_imag = deser.extract_double();
    result.ax = deser.extract_int16();
    result.bx = deser.extract_int16();
    result.cx = deser.extract_int16();
    result.dx = deser.extract_int16();
    result.dot_mode = deser.extract_int16();
    result.x_dots = deser.extract_int16();
    result.y_dots = deser.extract_int16();
    result.colors = deser.extract_int16();
    result.info_version = deser.extract_int16();
    result.param3 = deser.extract_float();
    result.param4 = deser.extract_float();
    {
        // TODO: when FractalInfo struct is no longer packed, we can avoid the intermediate copy
        // TODO: error: cannot bind packed field
        float potential[3];
        deser.extract_float(potential);
        for (size_t i = 0; i < std::size(potential); ++i)
        {
            result.potential[i] = potential[i];
        }
    }
    result.random_seed = deser.extract_int16();
    result.random_seed_flag = deser.extract_int16();
    result.biomorph = deser.extract_int16();
    result.inside = deser.extract_int16();
    result.log_map_old = deser.extract_int16();
    {
        // TODO: error: cannot bind packed field
        float invert[3];
        deser.extract_float(invert);
        for (size_t i = 0; i < std::size(invert); ++i)
        {
            result.invert[i] = invert[i];
        }
    }
    {
        // TODO: error: cannot bind packed field
        std::int16_t decomp[2];
        deser.extract_int16(decomp);
        for (size_t i = 0; i < std::size(decomp); ++i)
        {
            result.decomp[i] = decomp[i];
        }
    }
    result.symmetry = deser.extract_int16();
    {
        // TODO: error: cannot bind packed field
        std::int16_t init3d[16];
        deser.extract_int16(init3d);
        for (size_t i = 0; i < std::size(init3d); ++i)
        {
            result.init3d[i] = init3d[i];
        }
    }
    result.preview_factor = deser.extract_int16();
    result.x_trans = deser.extract_int16();
    result.y_trans = deser.extract_int16();
    result.red_crop_left = deser.extract_int16();
    result.red_crop_right = deser.extract_int16();
    result.blue_crop_left = deser.extract_int16();
    result.blue_crop_right = deser.extract_int16();
    result.red_bright = deser.extract_int16();
    result.blue_bright = deser.extract_int16();
    result.x_adjust = deser.extract_int16();
    result.eye_separation = deser.extract_int16();
    result.glasses_type = deser.extract_int16();
    result.outside = deser.extract_int16();
    result.x3rd = deser.extract_double();
    result.y3rd = deser.extract_double();
    deser.extract_chars(&result.std_calc_mode, 1);
    deser.extract_chars(&result.use_init_orbit, 1);
    result.calc_status = deser.extract_int16();
    result.tot_extend_len = deser.extract_int32();
    result.dist_est_old = deser.extract_int16();
    result.float_flag = deser.extract_int16();
    result.bailout_old = deser.extract_int16();
    result.calc_time = deser.extract_int32();
    deser.extract_byte(result.trig_index);
    result.finite_attractor = deser.extract_int16();
    {
        // TODO: error: cannot bind packed field
        double init_orbit[2];
        deser.extract_double(init_orbit);
        for (size_t i = 0; i < std::size(init_orbit); ++i)
        {
            result.init_orbit[i] = init_orbit[i];
        }
    }
    result.periodicity = deser.extract_int16();
    result.pot16bit = deser.extract_int16();
    result.final_aspect_ratio = deser.extract_float();
    result.system = deser.extract_int16();
    result.release = deser.extract_int16();
    result.display_3d = deser.extract_int16();
    {
        // TODO: error: cannot bind packed field
        std::int16_t transparent[2];
        deser.extract_int16(transparent);
        for (size_t i = 0; i < std::size(transparent); ++i)
        {
            result.transparent[i] = transparent[i];
        }
    }
    result.ambient = deser.extract_int16();
    result.haze = deser.extract_int16();
    result.randomize = deser.extract_int16();
    result.rotate_lo = deser.extract_int16();
    result.rotate_hi = deser.extract_int16();
    result.dist_est_width = deser.extract_int16();
    result.d_param3 = deser.extract_double();
    result.d_param4 = deser.extract_double();
    result.fill_color = deser.extract_int16();
    result.julibrot_x_max = deser.extract_double();
    result.julibrot_x_min = deser.extract_double();
    result.julibrot_y_max = deser.extract_double();
    result.julibrot_y_min = deser.extract_double();
    result.julibrot_z_dots = deser.extract_int16();
    result.julibrot_origin_fp = deser.extract_float();
    result.julibrot_depth_fp = deser.extract_float();
    result.julibrot_height_fp = deser.extract_float();
    result.julibrot_width_fp = deser.extract_float();
    result.julibrot_dist_fp = deser.extract_float();
    result.eyes_fp = deser.extract_float();
    result.orbit_type = deser.extract_int16();
    result.juli3d_mode = deser.extract_int16();
    result.max_fn = deser.extract_int16();
    result.inverse_julia = deser.extract_int16();
    result.d_param5 = deser.extract_double();
    result.d_param6 = deser.extract_double();
    result.d_param7 = deser.extract_double();
    result.d_param8 = deser.extract_double();
    result.d_param9 = deser.extract_double();
    result.d_param10 = deser.extract_double();
    result.bailout = deser.extract_int32();
    result.bailout_test = deser.extract_int16();
    result.iterations = deser.extract_int32();
    result.bf_math = deser.extract_int16();
    result.bf_length = deser.extract_int16();
    result.y_adjust = deser.extract_int16();
    result.old_demm_colors = deser.extract_int16();
    result.log_map = deser.extract_int32();
    result.dist_est = deser.extract_int32();
    {
        // TODO: error: cannot bind packed field
        double d_invert[3];
        deser.extract_double(d_invert);
        for (size_t i = 0; i < std::size(d_invert); ++i)
        {
            result.d_invert[i] = d_invert[i];
        }
    }
    result.log_calc = deser.extract_int16();
    result.stop_pass = deser.extract_int16();
    result.quick_calc = deser.extract_int16();
    result.close_prox = deser.extract_double();
    result.no_bof = deser.extract_int16();
    result.orbit_interval = deser.extract_int32();
    result.orbit_delay = deser.extract_int16();
    {
        // TODO: error: cannot bind packed field
        double math_tol[2];
        deser.extract_double(math_tol);
        for (size_t i = 0; i < std::size(math_tol); ++i)
        {
            result.math_tol[i] = math_tol[i];
        }
    }
    result.version_major = deser.extract_byte();
    result.version_minor = deser.extract_byte();
    result.version_patch = deser.extract_byte();
    result.version_tweak = deser.extract_byte();
    {
        // TODO: error: cannot bind packed field
        std::int16_t future[NUM_FRACTAL_INFO_FUTURE];
        deser.extract_int16(future);
        for (size_t i = 0; i < std::size(future); ++i)
        {
            result.future[i] = future[i];
        }
    }
    deser.validate();
    return result;
}

template <int Size>
class ExtensionSerializer
{
public:
    ExtensionSerializer() = default;

    template <int N>
    void insert_chars(const char (&dest)[N])
    {
        insert_chars(dest, N);
    }
    void insert_chars(const char *src, const int count)
    {
        check_overflow(count);
        std::copy_n(reinterpret_cast<const unsigned char *>(src), count, current());
        advance(count);
    }

    void insert_byte(std::uint8_t value)
    {
        check_overflow(1);
        m_bytes[m_offset] = value;
        advance(1);
    }
    template <int N>
    void insert_byte(const std::uint8_t (&values)[N])
    {
        for (int i = 0; i < N; ++i)
        {
            insert_byte(values[i]);
        }
    }

    void insert_int16(const std::int16_t value)
    {
        check_overflow(2);
        io::insert_int16(current(), value);
        advance(2);
    }
    template <int N>
    void insert_int16(const std::int16_t (&values)[N])
    {
        for (int i = 0; i < N; ++i)
        {
            insert_int16(values[i]);
        }
    }

    void insert_uint16(const std::uint16_t value)
    {
        check_overflow(2);
        boost::endian::store_little_u16(current(), value);
        advance(2);
    }

    void insert_int32(const std::int32_t value)
    {
        check_overflow(4);
        boost::endian::store_little_s32(current(), value);
        advance(4);
    }

    void insert_float(const float value)
    {
        check_overflow(4);
        boost::endian::little_float32_buf_t buffer(value);
        std::copy_n(buffer.data(), 4, current());
        advance(4);
    }
    template <int N>
    void insert_float(const float (&values)[N])
    {
        for (int i = 0; i < N; ++i)
        {
            insert_float(values[i]);
        }
    }

    void insert_double(const double value)
    {
        check_overflow(8);
        boost::endian::little_float64_buf_t buffer(value);
        std::copy_n(buffer.data(), 8, current());
        advance(8);
    }
    template <int N>
    void insert_double(const double (&values)[N])
    {
        for (int i = 0; i < N; ++i)
        {
            insert_double(values[i]);
        }
    }

    void add_extension(GifFileType *gif, const char *name)
    {
        add_gif_extension(gif, name, m_bytes.data(), static_cast<int>(m_bytes.size()));
    }

private:
    std::uint8_t *current()
    {
        return m_bytes.data() + m_offset;
    }
    void check_overflow(const int count)
    {
        if (m_count < count)
        {
            throw std::runtime_error("Buffer overflow");
        }
    }
    void advance(const int size)
    {
        m_offset += size;
        m_count -= size;
    }

    int m_offset{};
    int m_count{Size};
    std::array<std::uint8_t, Size> m_bytes{};
};

void put_fractal_info(GifFileType *gif, const FractalInfo &info)
{
    ExtensionSerializer<GIF_EXTENSION1_FRACTAL_INFO_LENGTH> ser;
    ser.insert_chars(info.info_id);
    ser.insert_int16(info.iterations_old);
    ser.insert_int16(info.fractal_type);
    ser.insert_double(info.x_min);
    ser.insert_double(info.x_max);
    ser.insert_double(info.y_min);
    ser.insert_double(info.y_max);
    ser.insert_double(info.c_real);
    ser.insert_double(info.c_imag);
    ser.insert_int16(info.ax);
    ser.insert_int16(info.bx);
    ser.insert_int16(info.cx);
    ser.insert_int16(info.dx);
    ser.insert_int16(info.dot_mode);
    ser.insert_int16(info.x_dots);
    ser.insert_int16(info.y_dots);
    ser.insert_int16(info.colors);
    ser.insert_int16(info.info_version);
    ser.insert_float(info.param3);
    ser.insert_float(info.param4);
    ser.insert_float(info.potential);
    ser.insert_int16(info.random_seed);
    ser.insert_int16(info.random_seed_flag);
    ser.insert_int16(info.biomorph);
    ser.insert_int16(info.inside);
    ser.insert_int16(info.log_map_old);
    ser.insert_float(info.invert);
    ser.insert_int16(info.decomp[0]);
    ser.insert_int16(info.decomp[1]);
    ser.insert_int16(info.symmetry);
    ser.insert_int16(info.init3d);
    ser.insert_int16(info.preview_factor);
    ser.insert_int16(info.x_trans);
    ser.insert_int16(info.y_trans);
    ser.insert_int16(info.red_crop_left);
    ser.insert_int16(info.red_crop_right);
    ser.insert_int16(info.blue_crop_left);
    ser.insert_int16(info.blue_crop_right);
    ser.insert_int16(info.red_bright);
    ser.insert_int16(info.blue_bright);
    ser.insert_int16(info.x_adjust);
    ser.insert_int16(info.eye_separation);
    ser.insert_int16(info.glasses_type);
    ser.insert_int16(info.outside);
    ser.insert_double(info.x3rd);
    ser.insert_double(info.y3rd);
    ser.insert_chars(&info.std_calc_mode, 1);
    ser.insert_chars(&info.use_init_orbit, 1);
    ser.insert_int16(info.calc_status);
    ser.insert_int32(info.tot_extend_len);
    ser.insert_int16(info.dist_est_old);
    ser.insert_int16(info.float_flag);
    ser.insert_int16(info.bailout_old);
    ser.insert_int32(info.calc_time);
    ser.insert_byte(info.trig_index);
    ser.insert_int16(info.finite_attractor);
    ser.insert_double(info.init_orbit);
    ser.insert_int16(info.periodicity);
    ser.insert_int16(info.pot16bit);
    ser.insert_float(info.final_aspect_ratio);
    ser.insert_int16(info.system);
    ser.insert_int16(info.release);
    ser.insert_int16(info.display_3d);
    ser.insert_int16(info.transparent);
    ser.insert_int16(info.ambient);
    ser.insert_int16(info.haze);
    ser.insert_int16(info.randomize);
    ser.insert_int16(info.rotate_lo);
    ser.insert_int16(info.rotate_hi);
    ser.insert_int16(info.dist_est_width);
    ser.insert_double(info.d_param3);
    ser.insert_double(info.d_param4);
    ser.insert_int16(info.fill_color);
    ser.insert_double(info.julibrot_x_max);
    ser.insert_double(info.julibrot_x_min);
    ser.insert_double(info.julibrot_y_max);
    ser.insert_double(info.julibrot_y_min);
    ser.insert_int16(info.julibrot_z_dots);
    ser.insert_float(info.julibrot_origin_fp);
    ser.insert_float(info.julibrot_depth_fp);
    ser.insert_float(info.julibrot_height_fp);
    ser.insert_float(info.julibrot_width_fp);
    ser.insert_float(info.julibrot_dist_fp);
    ser.insert_float(info.eyes_fp);
    ser.insert_int16(info.orbit_type);
    ser.insert_int16(info.juli3d_mode);
    ser.insert_int16(info.max_fn);
    ser.insert_int16(info.inverse_julia);
    ser.insert_double(info.d_param5);
    ser.insert_double(info.d_param6);
    ser.insert_double(info.d_param7);
    ser.insert_double(info.d_param8);
    ser.insert_double(info.d_param9);
    ser.insert_double(info.d_param10);
    ser.insert_int32(info.bailout);
    ser.insert_int16(info.bailout_test);
    ser.insert_int32(info.iterations);
    ser.insert_int16(info.bf_math);
    ser.insert_int16(info.bf_length);
    ser.insert_int16(info.y_adjust);
    ser.insert_int16(info.old_demm_colors);
    ser.insert_int32(info.log_map);
    ser.insert_int32(info.dist_est);
    ser.insert_double(info.d_invert);
    ser.insert_int16(info.log_calc);
    ser.insert_int16(info.stop_pass);
    ser.insert_int16(info.quick_calc);
    ser.insert_double(info.close_prox);
    ser.insert_int16(info.no_bof);
    ser.insert_int32(info.orbit_interval);
    ser.insert_int16(info.orbit_delay);
    ser.insert_double(info.math_tol);
    for (int16_t zero : info.future)
    {
        zero = 0;
        ser.insert_int16(zero);
    }
    ser.add_extension(gif, "fractint001");
}

FormulaInfo get_formula_info(const GifFileType *gif)
{
    FormulaInfo result{};
    ExtensionDeserializer<GIF_EXTENSION3_ITEM_NAME_INFO_LENGTH> deser(gif, "fractint003");
    deser.extract_chars(result.form_name);
    result.uses_p1 = deser.extract_int16();
    result.uses_p2 = deser.extract_int16();
    result.uses_p3 = deser.extract_int16();
    result.uses_ismand = deser.extract_int16();
    result.ismand = deser.extract_int16();
    result.uses_p4 = deser.extract_int16();
    result.uses_p5 = deser.extract_int16();
    return result;
}

void put_formula_info(GifFileType *gif, const FormulaInfo &info)
{
    ExtensionSerializer<GIF_EXTENSION3_ITEM_NAME_INFO_LENGTH> ser;
    ser.insert_chars(info.form_name);
    ser.insert_int16(info.uses_p1);
    ser.insert_int16(info.uses_p2);
    ser.insert_int16(info.uses_p3);
    ser.insert_int16(info.uses_ismand);
    ser.insert_int16(info.ismand);
    ser.insert_int16(info.uses_p4);
    ser.insert_int16(info.uses_p5);
    for (int16_t zero : info.future)
    {
        zero = 0;
        ser.insert_int16(zero);
    }
    ser.add_extension(gif, "fractint003");
}

std::vector<int> get_ranges_info(const GifFileType *gif)
{
    std::vector<Byte> bytes;
    for (int i = 0; i < gif->ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &block{gif->ExtensionBlocks[i]};
        if (is_fractint_extension(block, "fractint004"))
        {
            int ext{i + 1};
            while (ext < gif->ExtensionBlockCount                                //
                && gif->ExtensionBlocks[ext].Function == CONTINUE_EXT_FUNC_CODE) //
            {
                const ExtensionBlock &src = gif->ExtensionBlocks[ext];
                std::copy_n(src.Bytes, src.ByteCount, std::back_inserter(bytes));
                ++ext;
            }
            break;
        }
    }

    if (bytes.size() % 2 != 0)
    {
        throw std::runtime_error("Odd number of bytes in ranges extension block");
    }

    std::vector<int> result;
    result.resize(bytes.size() / 2);
    const Byte *src{bytes.data()};
    for (int &i : result)
    {
        i = extract_int16(src);
        src += 2;
    }
    return result;
}

void put_ranges_info(GifFileType *gif, const std::vector<int> &info)
{
    std::vector<unsigned char> bytes;
    bytes.resize(info.size() * 2);
    unsigned char *dest{bytes.data()};
    for (const int value : info)
    {
        insert_int16(dest, static_cast<std::int16_t>(value));
        dest += sizeof(std::int16_t);
    }
    add_gif_extension(gif, "fractint004", bytes.data(), static_cast<int>(bytes.size()));
}

std::vector<char> get_extended_param_info(const GifFileType *gif)
{
    std::vector<Byte> bytes;
    for (int i = 0; i < gif->ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &block{gif->ExtensionBlocks[i]};
        if (is_fractint_extension(block, "fractint005"))
        {
            int ext{i + 1};
            while (ext < gif->ExtensionBlockCount                                //
                && gif->ExtensionBlocks[ext].Function == CONTINUE_EXT_FUNC_CODE) //
            {
                const ExtensionBlock &src = gif->ExtensionBlocks[ext];
                std::copy_n(src.Bytes, src.ByteCount, std::back_inserter(bytes));
                ++ext;
            }
            break;
        }
    }

    std::vector<char> parameters;
    parameters.resize(bytes.size());
    std::transform(bytes.begin(), bytes.end(), parameters.begin(), [](const Byte byte) { return static_cast<char>(byte); });
    return parameters;
}

void put_extended_param_info(GifFileType *gif, const std::vector<char> &params)
{
    add_gif_extension(gif, "fractint005", //
        reinterpret_cast<unsigned char *>(const_cast<char *>(params.data())), static_cast<int>(params.size()));
}

EvolutionInfo get_evolution_info(const GifFileType *gif)
{
    EvolutionInfo result{};
    ExtensionDeserializer<GIF_EXTENSION6_EVOLVER_INFO_LENGTH> deser(gif, "fractint006");
    result.evolving = deser.extract_int16();
    result.image_grid_size = deser.extract_int16();
    result.this_generation_random_seed = deser.extract_uint16();
    result.max_random_mutation = deser.extract_double();
    result.x_parameter_range = deser.extract_double();
    result.y_parameter_range = deser.extract_double();
    result.x_parameter_offset = deser.extract_double();
    result.y_parameter_offset = deser.extract_double();
    result.discrete_x_parameter_offset = deser.extract_int16();
    result.discrete_y_parameter_offset = deser.extract_int16();
    result.px = deser.extract_int16();
    result.py = deser.extract_int16();
    result.screen_x_offset = deser.extract_int16();
    result.screen_y_offset = deser.extract_int16();
    result.x_dots = deser.extract_int16();
    result.y_dots = deser.extract_int16();
    deser.extract_int16(result.mutate);
    result.count = deser.extract_int16();
    return result;
}

void put_evolution_info(GifFileType *gif, const EvolutionInfo &info)
{
    ExtensionSerializer<GIF_EXTENSION6_EVOLVER_INFO_LENGTH> ser;
    ser.insert_int16(info.evolving);
    ser.insert_int16(info.image_grid_size);
    ser.insert_uint16(info.this_generation_random_seed);
    ser.insert_double(info.max_random_mutation);
    ser.insert_double(info.x_parameter_range);
    ser.insert_double(info.y_parameter_range);
    ser.insert_double(info.x_parameter_offset);
    ser.insert_double(info.y_parameter_offset);
    ser.insert_int16(info.discrete_x_parameter_offset);
    ser.insert_int16(info.discrete_y_parameter_offset);
    ser.insert_int16(info.px);
    ser.insert_int16(info.py);
    ser.insert_int16(info.screen_x_offset);
    ser.insert_int16(info.screen_y_offset);
    ser.insert_int16(info.x_dots);
    ser.insert_int16(info.y_dots);
    ser.insert_int16(info.mutate);
    ser.insert_int16(info.count);
    for (int16_t zero : info.future)
    {
        zero = 0;
        ser.insert_int16(zero);
    }
    ser.add_extension(gif, "fractint006");
}

OrbitsInfo get_orbits_info(const GifFileType *gif)
{
    OrbitsInfo result{};
    ExtensionDeserializer<GIF_EXTENSION7_ORBIT_INFO_LENGTH> deser(gif, "fractint007");
    result.orbit_corner_min_x = deser.extract_double();
    result.orbit_corner_max_x = deser.extract_double();
    result.orbit_corner_min_y = deser.extract_double();
    result.orbit_corner_max_y = deser.extract_double();
    result.orbit_corner_3rd_x = deser.extract_double();
    result.orbit_corner_3rd_y = deser.extract_double();
    result.keep_screen_coords = deser.extract_int16();
    deser.extract_chars(&result.draw_mode, 1);
    return result;
}

void put_orbits_info(GifFileType *gif, const OrbitsInfo &info)
{
    ExtensionSerializer<GIF_EXTENSION7_ORBIT_INFO_LENGTH> ser;
    ser.insert_double(info.orbit_corner_min_x);
    ser.insert_double(info.orbit_corner_max_x);
    ser.insert_double(info.orbit_corner_min_y);
    ser.insert_double(info.orbit_corner_max_y);
    ser.insert_double(info.orbit_corner_3rd_x);
    ser.insert_double(info.orbit_corner_3rd_y);
    ser.insert_int16(info.keep_screen_coords);
    ser.insert_chars(&info.draw_mode, 1);
    ser.insert_chars(&info.dummy, 1);
    for (int16_t zero : info.future)
    {
        zero = 0;
        ser.insert_int16(zero);
    }
    ser.add_extension(gif, "fractint007");
}

} // namespace id::io
