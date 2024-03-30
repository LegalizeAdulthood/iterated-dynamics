#include "gif_file.h"

#include "loadfile.h"
#include "port.h"

#include <boost/endian/buffers.hpp>
#include <boost/endian/conversion.hpp>
#include <gif_lib.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <string>

inline std::int16_t extract_int16(const unsigned char *src)
{
    return boost::endian::load_little_s16(src);
}

inline void insert_int16(unsigned char *dest, std::int16_t value)
{
    boost::endian::store_little_s16(dest, value);
}

inline bool is_fractint_extension(const ExtensionBlock &block, const char *name)
{
    return block.Function == APPLICATION_EXT_FUNC_CODE                           //
        && block.ByteCount == 11                                                 //
        && std::string(reinterpret_cast<const char *>(block.Bytes), 11) == name; //
}

void add_gif_extension(GifFileType *gif, const char *name, unsigned char *bytes, int length)
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
    ExtensionDeserializer(GifFileType *gif, const char *name);

    template <int N>
    void extract_chars(char (&dest)[N])
    {
        extract_chars(dest, N);
    }
    void extract_chars(char *dest, int count)
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
        const std::int16_t result = ::extract_int16(current());
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
    void check_overflow(int count)
    {
        if (m_count < count)
        {
            throw std::runtime_error("Buffer overflow");
        }
    }
    void advance(int size)
    {
        m_offset += size;
        m_count -= size;
    }

    int m_offset{};
    int m_count{Size};
    std::array<std::uint8_t, Size> m_bytes{};
};

template <int Size>
ExtensionDeserializer<Size>::ExtensionDeserializer(GifFileType *gif, const char *name)
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

FRACTAL_INFO get_fractal_info(GifFileType *gif)
{
    FRACTAL_INFO result{};
    ExtensionDeserializer<GIF_EXTENSION1_FRACTAL_INFO_LENGTH> deser(gif, "fractint001");
    deser.extract_chars(result.info_id);
    result.iterationsold = deser.extract_int16();
    result.fractal_type = deser.extract_int16();
    result.xmin = deser.extract_double();
    result.xmax = deser.extract_double();
    result.ymin = deser.extract_double();
    result.ymax = deser.extract_double();
    result.creal = deser.extract_double();
    result.cimag = deser.extract_double();
    result.videomodeax = deser.extract_int16();
    result.videomodebx = deser.extract_int16();
    result.videomodecx = deser.extract_int16();
    result.videomodedx = deser.extract_int16();
    result.dotmode = deser.extract_int16();
    result.xdots = deser.extract_int16();
    result.ydots = deser.extract_int16();
    result.colors = deser.extract_int16();
    result.version = deser.extract_int16();
    result.parm3 = deser.extract_float();
    result.parm4 = deser.extract_float();
    deser.extract_float(result.potential);
    result.rseed = deser.extract_int16();
    result.rflag = deser.extract_int16();
    result.biomorph = deser.extract_int16();
    result.inside = deser.extract_int16();
    result.logmapold = deser.extract_int16();
    deser.extract_float(result.invert);
    deser.extract_int16(result.decomp);
    result.symmetry = deser.extract_int16();
    deser.extract_int16(result.init3d);
    result.previewfactor = deser.extract_int16();
    result.xtrans = deser.extract_int16();
    result.ytrans = deser.extract_int16();
    result.red_crop_left = deser.extract_int16();
    result.red_crop_right = deser.extract_int16();
    result.blue_crop_left = deser.extract_int16();
    result.blue_crop_right = deser.extract_int16();
    result.red_bright = deser.extract_int16();
    result.blue_bright = deser.extract_int16();
    result.xadjust = deser.extract_int16();
    result.eyeseparation = deser.extract_int16();
    result.glassestype = deser.extract_int16();
    result.outside = deser.extract_int16();
    result.x3rd = deser.extract_double();
    result.y3rd = deser.extract_double();
    deser.extract_chars(&result.stdcalcmode, 1);
    deser.extract_chars(&result.useinitorbit, 1);
    result.calc_status = deser.extract_int16();
    result.tot_extend_len = deser.extract_int32();
    result.distestold = deser.extract_int16();
    result.floatflag = deser.extract_int16();
    result.bailoutold = deser.extract_int16();
    result.calctime = deser.extract_int32();
    deser.extract_byte(result.trigndx);
    result.finattract = deser.extract_int16();
    deser.extract_double(result.initorbit);
    result.periodicity = deser.extract_int16();
    result.pot16bit = deser.extract_int16();
    result.faspectratio = deser.extract_float();
    result.system = deser.extract_int16();
    result.release = deser.extract_int16();
    result.display_3d = deser.extract_int16();
    deser.extract_int16(result.transparent);
    result.ambient = deser.extract_int16();
    result.haze = deser.extract_int16();
    result.randomize = deser.extract_int16();
    result.rotate_lo = deser.extract_int16();
    result.rotate_hi = deser.extract_int16();
    result.distestwidth = deser.extract_int16();
    result.dparm3 = deser.extract_double();
    result.dparm4 = deser.extract_double();
    result.fillcolor = deser.extract_int16();
    result.mxmaxfp = deser.extract_double();
    result.mxminfp = deser.extract_double();
    result.mymaxfp = deser.extract_double();
    result.myminfp = deser.extract_double();
    result.zdots = deser.extract_int16();
    result.originfp = deser.extract_float();
    result.depthfp = deser.extract_float();
    result.heightfp = deser.extract_float();
    result.widthfp = deser.extract_float();
    result.distfp = deser.extract_float();
    result.eyesfp = deser.extract_float();
    result.orbittype = deser.extract_int16();
    result.juli3Dmode = deser.extract_int16();
    result.maxfn = deser.extract_int16();
    result.inversejulia = deser.extract_int16();
    result.dparm5 = deser.extract_double();
    result.dparm6 = deser.extract_double();
    result.dparm7 = deser.extract_double();
    result.dparm8 = deser.extract_double();
    result.dparm9 = deser.extract_double();
    result.dparm10 = deser.extract_double();
    result.bailout = deser.extract_int32();
    result.bailoutest = deser.extract_int16();
    result.iterations = deser.extract_int32();
    result.bf_math = deser.extract_int16();
    result.bflength = deser.extract_int16();
    result.yadjust = deser.extract_int16();
    result.old_demm_colors = deser.extract_int16();
    result.logmap = deser.extract_int32();
    result.distest = deser.extract_int32();
    deser.extract_double(result.dinvert);
    result.logcalc = deser.extract_int16();
    result.stoppass = deser.extract_int16();
    result.quick_calc = deser.extract_int16();
    result.closeprox = deser.extract_double();
    result.nobof = deser.extract_int16();
    result.orbit_interval = deser.extract_int32();
    result.orbit_delay = deser.extract_int16();
    deser.extract_double(result.math_tol);
    deser.extract_int16(result.future);
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
    void insert_chars(const char *src, int count)
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

    void insert_int16(std::int16_t value)
    {
        check_overflow(2);
        ::insert_int16(current(), value);
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

    void insert_uint16(std::uint16_t value)
    {
        check_overflow(2);
        boost::endian::store_little_u16(current(), value);
        advance(2);
    }

    void insert_int32(std::int32_t value)
    {
        check_overflow(4);
        boost::endian::store_little_s32(current(), value);
        advance(4);
    }

    void insert_float(float value)
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

    void insert_double(double value)
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
    void check_overflow(int count)
    {
        if (m_count < count)
        {
            throw std::runtime_error("Buffer overflow");
        }
    }
    void advance(int size)
    {
        m_offset += size;
        m_count -= size;
    }

    int m_offset{};
    int m_count{Size};
    std::array<std::uint8_t, Size> m_bytes{};
};

void put_fractal_info(GifFileType *gif, const FRACTAL_INFO &info)
{
    ExtensionSerializer<GIF_EXTENSION1_FRACTAL_INFO_LENGTH> ser;
    ser.insert_chars(info.info_id);
    ser.insert_int16(info.iterationsold);
    ser.insert_int16(info.fractal_type);
    ser.insert_double(info.xmin);
    ser.insert_double(info.xmax);
    ser.insert_double(info.ymin);
    ser.insert_double(info.ymax);
    ser.insert_double(info.creal);
    ser.insert_double(info.cimag);
    ser.insert_int16(info.videomodeax);
    ser.insert_int16(info.videomodebx);
    ser.insert_int16(info.videomodecx);
    ser.insert_int16(info.videomodedx);
    ser.insert_int16(info.dotmode);
    ser.insert_int16(info.xdots);
    ser.insert_int16(info.ydots);
    ser.insert_int16(info.colors);
    ser.insert_int16(info.version);
    ser.insert_float(info.parm3);
    ser.insert_float(info.parm4);
    ser.insert_float(info.potential);
    ser.insert_int16(info.rseed);
    ser.insert_int16(info.rflag);
    ser.insert_int16(info.biomorph);
    ser.insert_int16(info.inside);
    ser.insert_int16(info.logmapold);
    ser.insert_float(info.invert);
    ser.insert_int16(info.decomp[0]);
    ser.insert_int16(info.decomp[1]);
    ser.insert_int16(info.symmetry);
    ser.insert_int16(info.init3d);
    ser.insert_int16(info.previewfactor);
    ser.insert_int16(info.xtrans);
    ser.insert_int16(info.ytrans);
    ser.insert_int16(info.red_crop_left);
    ser.insert_int16(info.red_crop_right);
    ser.insert_int16(info.blue_crop_left);
    ser.insert_int16(info.blue_crop_right);
    ser.insert_int16(info.red_bright);
    ser.insert_int16(info.blue_bright);
    ser.insert_int16(info.xadjust);
    ser.insert_int16(info.eyeseparation);
    ser.insert_int16(info.glassestype);
    ser.insert_int16(info.outside);
    ser.insert_double(info.x3rd);
    ser.insert_double(info.y3rd);
    ser.insert_chars(&info.stdcalcmode, 1);
    ser.insert_chars(&info.useinitorbit, 1);
    ser.insert_int16(info.calc_status);
    ser.insert_int32(info.tot_extend_len);
    ser.insert_int16(info.distestold);
    ser.insert_int16(info.floatflag);
    ser.insert_int16(info.bailoutold);
    ser.insert_int32(info.calctime);
    ser.insert_byte(info.trigndx);
    ser.insert_int16(info.finattract);
    ser.insert_double(info.initorbit);
    ser.insert_int16(info.periodicity);
    ser.insert_int16(info.pot16bit);
    ser.insert_float(info.faspectratio);
    ser.insert_int16(info.system);
    ser.insert_int16(info.release);
    ser.insert_int16(info.display_3d);
    ser.insert_int16(info.transparent);
    ser.insert_int16(info.ambient);
    ser.insert_int16(info.haze);
    ser.insert_int16(info.randomize);
    ser.insert_int16(info.rotate_lo);
    ser.insert_int16(info.rotate_hi);
    ser.insert_int16(info.distestwidth);
    ser.insert_double(info.dparm3);
    ser.insert_double(info.dparm4);
    ser.insert_int16(info.fillcolor);
    ser.insert_double(info.mxmaxfp);
    ser.insert_double(info.mxminfp);
    ser.insert_double(info.mymaxfp);
    ser.insert_double(info.myminfp);
    ser.insert_int16(info.zdots);
    ser.insert_float(info.originfp);
    ser.insert_float(info.depthfp);
    ser.insert_float(info.heightfp);
    ser.insert_float(info.widthfp);
    ser.insert_float(info.distfp);
    ser.insert_float(info.eyesfp);
    ser.insert_int16(info.orbittype);
    ser.insert_int16(info.juli3Dmode);
    ser.insert_int16(info.maxfn);
    ser.insert_int16(info.inversejulia);
    ser.insert_double(info.dparm5);
    ser.insert_double(info.dparm6);
    ser.insert_double(info.dparm7);
    ser.insert_double(info.dparm8);
    ser.insert_double(info.dparm9);
    ser.insert_double(info.dparm10);
    ser.insert_int32(info.bailout);
    ser.insert_int16(info.bailoutest);
    ser.insert_int32(info.iterations);
    ser.insert_int16(info.bf_math);
    ser.insert_int16(info.bflength);
    ser.insert_int16(info.yadjust);
    ser.insert_int16(info.old_demm_colors);
    ser.insert_int32(info.logmap);
    ser.insert_int32(info.distest);
    ser.insert_double(info.dinvert);
    ser.insert_int16(info.logcalc);
    ser.insert_int16(info.stoppass);
    ser.insert_int16(info.quick_calc);
    ser.insert_double(info.closeprox);
    ser.insert_int16(info.nobof);
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

formula_info get_formula_info(GifFileType *gif)
{
    formula_info result{};
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

void put_formula_info(GifFileType *gif, const formula_info &info)
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

std::vector<int> get_ranges_info(GifFileType *gif)
{
    std::vector<BYTE> bytes;
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
    const BYTE *src{bytes.data()};
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

EVOLUTION_INFO get_evolution_info(GifFileType *gif)
{
    EVOLUTION_INFO result{};
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
    result.discrete_y_paramter_offset = deser.extract_int16();
    result.px = deser.extract_int16();
    result.py = deser.extract_int16();
    result.sxoffs = deser.extract_int16();
    result.syoffs = deser.extract_int16();
    result.xdots = deser.extract_int16();
    result.ydots = deser.extract_int16();
    for (std::int16_t &i : result.mutate)
    {
        i = deser.extract_int16();
    }
    result.ecount = deser.extract_int16();
    return result;
}

void put_evolution_info(GifFileType *gif, const EVOLUTION_INFO &info)
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
    ser.insert_int16(info.discrete_y_paramter_offset);
    ser.insert_int16(info.px);
    ser.insert_int16(info.py);
    ser.insert_int16(info.sxoffs);
    ser.insert_int16(info.syoffs);
    ser.insert_int16(info.xdots);
    ser.insert_int16(info.ydots);
    ser.insert_int16(info.mutate);
    ser.insert_int16(info.ecount);
    for (int16_t zero : info.future)
    {
        zero = 0;
        ser.insert_int16(zero);
    }
    ser.add_extension(gif, "fractint006");
}

ORBITS_INFO get_orbits_info(GifFileType *gif)
{
    ORBITS_INFO result{};
    ExtensionDeserializer<GIF_EXTENSION7_ORBIT_INFO_LENGTH> deser(gif, "fractint007");
    result.oxmin = deser.extract_double();
    result.oxmax = deser.extract_double();
    result.oymin = deser.extract_double();
    result.oymax = deser.extract_double();
    result.ox3rd = deser.extract_double();
    result.oy3rd = deser.extract_double();
    result.keep_scrn_coords = deser.extract_int16();
    deser.extract_chars(&result.drawmode, 1);
    return result;
}

void put_orbits_info(GifFileType *gif, const ORBITS_INFO &info)
{
    ExtensionSerializer<GIF_EXTENSION7_ORBIT_INFO_LENGTH> ser;
    ser.insert_double(info.oxmin);
    ser.insert_double(info.oxmax);
    ser.insert_double(info.oymin);
    ser.insert_double(info.oymax);
    ser.insert_double(info.ox3rd);
    ser.insert_double(info.oy3rd);
    ser.insert_int16(info.keep_scrn_coords);
    ser.insert_chars(&info.drawmode, 1);
    ser.insert_chars(&info.dummy, 1);
    for (int16_t zero : info.future)
    {
        zero = 0;
        ser.insert_int16(zero);
    }
    ser.add_extension(gif, "fractint007");
}
