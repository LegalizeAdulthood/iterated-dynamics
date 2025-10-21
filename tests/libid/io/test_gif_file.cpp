// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/gif_file.h>

#include "test_data.h"

#include <engine/calcfrac.h>
#include <fractals/fractype.h>
#include <io/loadfile.h>

#include <boost/endian/buffers.hpp>
#include <boost/endian/conversion.hpp>
#include <gif_lib.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <numeric>
#include <string>

using namespace id::engine;
using namespace id::io;
using namespace id::test::data;
using namespace id::ui;

namespace id::test
{

template <int N>
static std::string trim(const char (&field)[N])
{
    const std::string text{field, N};
    const auto pos = text.find_last_not_of('\0');
    return pos == std::string::npos ? (text[0] == 0 ? std::string{} : text) : text.substr(0, pos + 1);
}

template <typename T, size_t N>
class ArrayPrinter
{
public:
    explicit ArrayPrinter(const T (&value)[N]) :
        m_value(value)
    {
    }

    ArrayPrinter(const ArrayPrinter &rhs) = delete;
    ArrayPrinter(ArrayPrinter &&rhs) = delete;
    ~ArrayPrinter() = default;
    ArrayPrinter &operator=(const ArrayPrinter &rhs) = delete;
    ArrayPrinter &operator=(ArrayPrinter &&rhs) = delete;

    const T (&m_value)[N];
};

template <size_t N, typename T>
std::ostream &operator<<(std::ostream &str, const ArrayPrinter<T, N> &value)
{
    str << "[ ";
    bool first{true};
    for (int i = 0; i < N; ++i)
    {
        if (!first)
        {
            str << ", ";
        }
        str << value.m_value[i];
        first = false;
    }
    return str << " ]";
}

template <size_t N>
std::ostream &operator<<(std::ostream &str, const ArrayPrinter<std::uint8_t, N> &value)
{
    str << "[ ";
    bool first{true};
    for (int i = 0; i < N; ++i)
    {
        if (!first)
        {
            str << ", ";
        }
        str << static_cast<int>(value.m_value[i]);
        first = false;
    }
    return str << " ]";
}

} // namespace id::test

namespace id::io
{

std::ostream &operator<<(std::ostream &str, const FractalInfo &value)
{
    return str << "{ "                                                              //
               << R"("info_id": ")" << test::trim(value.info_id) << '"'             //
               << R"(, "iterationsold": )" << value.iterations_old                  //
               << R"(, "fractal_type": )" << value.fractal_type                     //
               << R"(, "xmin": )" << value.x_min                                    //
               << R"(, "xmax": )" << value.x_max                                    //
               << R"(, "ymin": )" << value.y_min                                    //
               << R"(, "ymax": )" << value.y_max                                    //
               << R"(, "creal": )" << value.c_real                                  //
               << R"(, "cimag": )" << value.c_imag                                  //
               << R"(, "ax": )" << value.ax                                         //
               << R"(, "bx": )" << value.bx                                         //
               << R"(, "cx": )" << value.cx                                         //
               << R"(, "dx": )" << value.dx                                         //
               << R"(, "dotmode": )" << value.dot_mode                              //
               << R"(, "xdots": )" << value.x_dots                                  //
               << R"(, "ydots": )" << value.y_dots                                  //
               << R"(, "colors": )" << value.colors                                 //
               << R"(, "version": )" << value.info_version                          //
               << R"(, "param3": )" << value.param3                                 //
               << R"(, "param4": )" << value.param4                                 //
               << R"(, "potential": )" << test::ArrayPrinter(value.potential)       //
               << R"(, "rseed": )" << value.random_seed                             //
               << R"(, "rflag": )" << value.random_seed_flag                        //
               << R"(, "biomorph": )" << value.biomorph                             //
               << R"(, "inside": )" << value.inside                                 //
               << R"(, "logmapold": )" << value.log_map_old                         //
               << R"(, "invert": )" << test::ArrayPrinter(value.invert)             //
               << R"(, "decomp": )" << test::ArrayPrinter(value.decomp)             //
               << R"(, "symmetry": )" << value.symmetry                             //
               << R"(, "init3d": )" << test::ArrayPrinter(value.init3d)             //
               << R"(, "previewfactor": )" << value.preview_factor                  //
               << R"(, "xtrans": )" << value.x_trans                                //
               << R"(, "ytrans": )" << value.y_trans                                //
               << R"(, "red_crop_left": )" << value.red_crop_left                   //
               << R"(, "red_crop_right": )" << value.red_crop_right                 //
               << R"(, "blue_crop_left": )" << value.blue_crop_left                 //
               << R"(, "blue_crop_right": )" << value.blue_crop_right               //
               << R"(, "red_bright": )" << value.red_bright                         //
               << R"(, "blue_bright": )" << value.blue_bright                       //
               << R"(, "xadjust": )" << value.x_adjust                              //
               << R"(, "eyeseparation": )" << value.eye_separation                  //
               << R"(, "glassestype": )" << value.glasses_type                      //
               << R"(, "outside": )" << value.outside                               //
               << R"(, "x3rd": )" << value.x3rd                                     //
               << R"(, "y3rd": )" << value.y3rd                                     //
               << R"(, "stdcalcmode": )" << static_cast<int>(value.std_calc_mode)   //
               << R"(, "useinitorbit": )" << static_cast<int>(value.use_init_orbit) //
               << R"(, "calc_status": )" << value.calc_status                       //
               << R"(, "tot_extend_len": )" << value.tot_extend_len                 //
               << R"(, "distestold": )" << value.dist_est_old                       //
               << R"(, "floatflag": )" << value.float_flag                          //
               << R"(, "bailoutold": )" << value.bailout_old                        //
               << R"(, "calctime": )" << value.calc_time                            //
               << R"(, "trigndx": )" << test::ArrayPrinter(value.trig_index)        //
               << R"(, "finattract": )" << value.finite_attractor                   //
               << R"(, "initorbit": )" << test::ArrayPrinter(value.init_orbit)      //
               << R"(, "periodicity": )" << value.periodicity                       //
               << R"(, "pot16bit": )" << value.pot16bit                             //
               << R"(, "faspectratio": )" << value.final_aspect_ratio               //
               << R"(, "system": )" << value.system                                 //
               << R"(, "release": )" << value.release                               //
               << R"(, "display_3d": )" << value.display_3d                         //
               << R"(, "transparent": )" << test::ArrayPrinter(value.transparent)   //
               << R"(, "ambient": )" << value.ambient                               //
               << R"(, "haze": )" << value.haze                                     //
               << R"(, "randomize": )" << value.randomize                           //
               << R"(, "rotate_lo": )" << value.rotate_lo                           //
               << R"(, "rotate_hi": )" << value.rotate_hi                           //
               << R"(, "distestwidth": )" << value.dist_est_width                   //
               << R"(, "d_param3": )" << value.d_param3                             //
               << R"(, "d_param4": )" << value.d_param4                             //
               << R"(, "fillcolor": )" << value.fill_color                          //
               << R"(, "mxmaxfp": )" << value.julibrot_x_max                        //
               << R"(, "mxminfp": )" << value.julibrot_x_min                        //
               << R"(, "mymaxfp": )" << value.julibrot_y_max                        //
               << R"(, "myminfp": )" << value.julibrot_y_min                        //
               << R"(, "zdots": )" << value.julibrot_z_dots                         //
               << R"(, "originfp": )" << value.julibrot_origin_fp                   //
               << R"(, "depthfp": )" << value.julibrot_depth_fp                     //
               << R"(, "heightfp": )" << value.julibrot_height_fp                   //
               << R"(, "widthfp": )" << value.julibrot_width_fp                     //
               << R"(, "distfp": )" << value.julibrot_dist_fp                       //
               << R"(, "eyesfp": )" << value.eyes_fp                                //
               << R"(, "orbittype": )" << value.orbit_type                          //
               << R"(, "juli3Dmode": )" << value.juli3d_mode                        //
               << R"(, "maxfn": )" << value.max_fn                                  //
               << R"(, "inversejulia": )" << value.inverse_julia                    //
               << R"(, "d_param5": )" << value.d_param5                             //
               << R"(, "d_param6": )" << value.d_param6                             //
               << R"(, "d_param7": )" << value.d_param7                             //
               << R"(, "d_param8": )" << value.d_param8                             //
               << R"(, "d_param9": )" << value.d_param9                             //
               << R"(, "d_param10": )" << value.d_param10                           //
               << R"(, "bailout": )" << value.bailout                               //
               << R"(, "bailoutest": )" << value.bailout_test                       //
               << R"(, "iterations": )" << value.iterations                         //
               << R"(, "bf_math": )" << value.bf_math                               //
               << R"(, "g_bf_length": )" << value.bf_length                         //
               << R"(, "yadjust": )" << value.y_adjust                              //
               << R"(, "old_demm_colors": )" << value.old_demm_colors               //
               << R"(, "logmap": )" << value.log_map                                //
               << R"(, "distest": )" << value.dist_est                              //
               << R"(, "dinvert": )" << test::ArrayPrinter(value.d_invert)          //
               << R"(, "logcalc": )" << value.log_calc                              //
               << R"(, "stoppass": )" << value.stop_pass                            //
               << R"(, "quick_calc": )" << value.quick_calc                         //
               << R"(, "closeprox": )" << value.close_prox                          //
               << R"(, "nobof": )" << value.no_bof                                  //
               << R"(, "orbit_interval": )" << value.orbit_interval                 //
               << R"(, "orbit_delay": )" << value.orbit_delay                       //
               << R"(, "math_tol": )" << test::ArrayPrinter(value.math_tol)         //
               << R"(, "version_major": )" << value.version_major                   //
               << R"(, "version_minor": )" << value.version_minor                   //
               << R"(, "version_patch": )" << value.version_patch                   //
               << R"(, "version_tweak": )" << value.version_tweak                   //
               << " }";                                                             //
}

std::ostream &operator<<(std::ostream &str, const FormulaInfo &value)
{
    return str << R"({ "form_name": ")" << test::trim(value.form_name) << '"' //
               << R"(, "uses_p1": )" << value.uses_p1                         //
               << R"(, "uses_p2": )" << value.uses_p2                         //
               << R"(, "uses_p3": )" << value.uses_p3                         //
               << R"(, "uses_ismand": )" << value.uses_ismand                 //
               << R"(, "ismand": )" << value.ismand                           //
               << R"(, "uses_p4": )" << value.uses_p4                         //
               << R"(, "uses_p5": )" << value.uses_p5                         //
               << " }";
}

std::ostream &operator<<(std::ostream &str, const OrbitsInfo &value)
{
    return str                                                                              //
        << R"({ "oxmin": )" << value.orbit_corner_min_x                                     //
        << R"( "oxmax": )" << value.orbit_corner_max_x                                      //
        << R"( "oymin": )" << value.orbit_corner_min_y                                      //
        << R"( "oymax": )" << value.orbit_corner_max_y                                      //
        << R"( "ox3rd": )" << value.orbit_corner_3rd_x                                      //
        << R"( "oy3rd": )" << value.orbit_corner_3rd_y                                      //
        << R"( "keep_scrn_coords": )" << (value.keep_screen_coords != 0 ? "true" : "false") //
        << R"( "drawmode": ")" << value.draw_mode << '"'                                    //
        << " }";                                                                            //
}

} // namespace id::io

namespace id::ui
{

std::ostream &operator<<(std::ostream &str, const EvolutionInfo &value)
{
    return str                                                                         //
        << R"({ "evolving": )" << value.evolving                                       //
        << R"(, "image_grid_size": )" << value.image_grid_size                         //
        << R"(, "this_generation_random_seed": )" << value.this_generation_random_seed //
        << R"(, "max_random_mutation": )" << value.max_random_mutation                 //
        << R"(, "x_parameter_range": )" << value.x_parameter_range                     //
        << R"(, "y_parameter_range": )" << value.y_parameter_range                     //
        << R"(, "x_parameter_offset": )" << value.x_parameter_offset                   //
        << R"(, "y_parameter_offset": )" << value.y_parameter_offset                   //
        << R"(, "discrete_x_parameter_offset": )" << value.discrete_x_parameter_offset //
        << R"(, "discrete_y_paramter_offset": )" << value.discrete_y_parameter_offset  //
        << R"(, "px": )" << value.px                                                   //
        << R"(, "py": )" << value.py                                                   //
        << R"(, "sxoffs": )" << value.screen_x_offset                                  //
        << R"(, "syoffs": )" << value.screen_y_offset                                  //
        << R"(, "xdots": )" << value.x_dots                                            //
        << R"(, "ydots": )" << value.y_dots                                            //
        << R"(, "mutate": )" << test::ArrayPrinter(value.mutate)                       //
        << R"(, "ecount": )" << value.count                                            //
        << " }";                                                                       //
}

} // namespace id::ui

namespace id::test
{

constexpr double EPS{1.0e-6};

enum
{
    GIF_EXTENSION2_RESUME_INFO_LENGTH = 196,
};

class TestOpenGIF : public testing::Test
{
public:
    TestOpenGIF() :
        m_name(ID_TEST_GIF_EXT1_FILE)
    {
    }

    explicit TestOpenGIF(const char *name) :
        m_name(name)
    {
    }

protected:
    void SetUp() override;
    void TearDown() override;

    int extension_total(int begin, int end) const;

    std::string get_extension_name(const int ext) const
    {
        return std::string{reinterpret_cast<char *>(m_gif->ExtensionBlocks[ext].Bytes), 11};
    }

    const char *m_name;
    GifFileType *m_gif{};
};

void TestOpenGIF::SetUp()
{
    Test::SetUp();
    m_gif = DGifOpenFileName(m_name, nullptr);
}

void TestOpenGIF::TearDown()
{
    DGifCloseFile(m_gif, nullptr);
    Test::TearDown();
}

int TestOpenGIF::extension_total(const int begin, const int end) const
{
    int length{};
    for (int i = begin; i < end; ++i)
    {
        length += m_gif->ExtensionBlocks[i].ByteCount;
    }
    return length;
}

class TestSlurpGIF : public TestOpenGIF
{
public:
    explicit TestSlurpGIF(const char *name);
    ~TestSlurpGIF() override = default;

protected:
    void SetUp() override;

    int m_result{};
};

TestSlurpGIF::TestSlurpGIF(const char *name) :
    TestOpenGIF(name)
{
}

void TestSlurpGIF::SetUp()
{
    TestOpenGIF::SetUp();
    m_result = DGifSlurp(m_gif);
}

class TestGIFFractalInfoExtension : public TestSlurpGIF
{
public:
    TestGIFFractalInfoExtension() :
        TestSlurpGIF(ID_TEST_GIF_EXT1_FILE)
    {
    }

    ~TestGIFFractalInfoExtension() override = default;
};

class TestGIFResumeInfoExtension : public TestSlurpGIF
{
public:
    TestGIFResumeInfoExtension() :
        TestSlurpGIF(ID_TEST_GIF_EXT2_FILE)
    {
    }

    ~TestGIFResumeInfoExtension() override = default;
};

class TestGIFItemNameInfoExtension : public TestSlurpGIF
{
public:
    TestGIFItemNameInfoExtension() :
        TestSlurpGIF(ID_TEST_GIF_EXT3_FILE)
    {
    }

    ~TestGIFItemNameInfoExtension() override = default;
};

class TestGIFRangesInfoExtension : public TestSlurpGIF
{
public:
    TestGIFRangesInfoExtension() :
        TestSlurpGIF(ID_TEST_GIF_EXT4_FILE)
    {
    }

    ~TestGIFRangesInfoExtension() override = default;
};

class TestGIFBigNumParametersExtension : public TestSlurpGIF
{
public:
    TestGIFBigNumParametersExtension() :
        TestSlurpGIF(ID_TEST_GIF_EXT5_FILE)
    {
    }

    ~TestGIFBigNumParametersExtension() override = default;
};

class TestGIFEvolutionInfoExtension : public TestSlurpGIF
{
public:
    TestGIFEvolutionInfoExtension() :
        TestSlurpGIF(ID_TEST_GIF_EXT6_FILE)
    {
    }

    ~TestGIFEvolutionInfoExtension() override = default;
};

class TestGIFOrbitInfoExtension : public TestSlurpGIF
{
public:
    TestGIFOrbitInfoExtension() :
        TestSlurpGIF(ID_TEST_GIF_EXT7_FILE)
    {
    }

    ~TestGIFOrbitInfoExtension() override = default;
};

template <typename T>
class VecPrinter
{
public:
    explicit VecPrinter(const std::vector<T> &values) :
        m_values(values)
    {
    }

    VecPrinter(const VecPrinter &rhs) = delete;
    VecPrinter(VecPrinter &&rhs) = delete;
    ~VecPrinter() = default;
    VecPrinter &operator=(const VecPrinter &rhs) = delete;
    VecPrinter &operator=(VecPrinter &&rhs) = delete;

    const std::vector<T> &m_values;
};

std::ostream &operator<<(std::ostream &str, const VecPrinter<int> &value)
{
    if (value.m_values.empty())
    {
        return str << "[]";
    }

    str << "[ ";
    bool first{true};
    for (const int i : value.m_values)
    {
        if (!first)
        {
            str << ", ";
        }
        str << i;
        first = false;
    }
    return str << " ]";
}

class GIFOutputFile
{
public:
    explicit GIFOutputFile(const char *path) :
        m_gif(EGifOpenFileName(path, false, &m_gif_error))
    {
        if (m_gif_error != E_GIF_SUCCEEDED)
        {
            throw std::runtime_error("Unexpected error opening " + std::string{path} +
                " for writing: " + std::to_string(m_gif_error));
        }
    }

    ~GIFOutputFile()
    {
        close(nullptr);
    }

    int close(int *error)
    {
        int result{GIF_OK};
        if (m_gif != nullptr)
        {
            result = EGifCloseFile(m_gif, error);
            m_gif = nullptr;
        }
        return result;
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    operator GifFileType *() const
    {
        return m_gif;
    }

private:
    GifFileType *m_gif;
    int m_gif_error{};
};

TEST(TestCompareFractalInfo, futureFieldsAreIgnored)
{
    const FractalInfo lhs{};
    FractalInfo rhs{};
    std::int16_t value{16};
    for (size_t i = 0; i < std::size(rhs.future); ++i) // NOLINT(modernize-loop-convert)
    {
        rhs.future[i] = value;
        ++value;
    }

    EXPECT_TRUE(lhs == rhs) << lhs << " != " << rhs;
}

TEST(TestCompareFormulaInfo, futureFieldsAreIgnored)
{
    const FormulaInfo lhs{};
    FormulaInfo rhs{};
    std::int16_t value{16};
    for (size_t i = 0; i < std::size(rhs.future); ++i) // NOLINT(modernize-loop-convert)
    {
        rhs.future[i] = value;
        ++value;
    }

    EXPECT_TRUE(lhs == rhs) << lhs << " != " << rhs;
}

TEST(TestGIF, openClose)
{
    int open_error{};
    GifFileType *gif = DGifOpenFileName(ID_TEST_GIF_EXT1_FILE, &open_error);
    int close_error{};
    const int close_result = DGifCloseFile(gif, &close_error);

    EXPECT_NE(nullptr, gif);
    EXPECT_EQ(D_GIF_SUCCEEDED, open_error);
    EXPECT_NE(GIF_ERROR, close_result);
    EXPECT_EQ(D_GIF_SUCCEEDED, close_error);
}

TEST_F(TestOpenGIF, checkScreen)
{
    EXPECT_EQ(640, m_gif->SWidth);
    EXPECT_EQ(480, m_gif->SHeight);
    EXPECT_EQ(256, m_gif->SColorMap->ColorCount);
    EXPECT_EQ(0, m_gif->ExtensionBlockCount);
}

TEST_F(TestGIFFractalInfoExtension, check)
{
    EXPECT_NE(GIF_ERROR, m_result);
    EXPECT_EQ(3, m_gif->ExtensionBlockCount);
    EXPECT_EQ(11, m_gif->ExtensionBlocks[0].ByteCount);
    EXPECT_EQ("fractint001", get_extension_name(0));
    EXPECT_EQ(GIF_EXTENSION1_FRACTAL_INFO_LENGTH, extension_total(1, 3));
}

TEST_F(TestGIFFractalInfoExtension, decode)
{
    const FractalInfo info{get_fractal_info(m_gif)};

    EXPECT_EQ("Fractal", trim(info.info_id));
    EXPECT_EQ(INITIAL_MAX_ITERATIONS, info.iterations_old);
    constexpr int MANDEL{0};
    EXPECT_EQ(MANDEL, info.fractal_type);
    EXPECT_NEAR(-2.5, info.x_min, EPS);
    EXPECT_NEAR(1.5, info.x_max, EPS);
    EXPECT_NEAR(-1.5, info.y_min, EPS);
    EXPECT_NEAR(1.5, info.y_max, EPS);
    EXPECT_NEAR(0.0, info.c_real, EPS);
    EXPECT_NEAR(0.0, info.c_imag, EPS);
    EXPECT_EQ(0, info.ax);
    EXPECT_EQ(0, info.bx);
    EXPECT_EQ(0, info.cx);
    EXPECT_EQ(0, info.dx);
    EXPECT_EQ(27, info.dot_mode);
    EXPECT_EQ(640, info.x_dots);
    EXPECT_EQ(480, info.y_dots);
    EXPECT_EQ(256, info.colors);
    EXPECT_EQ(17, info.info_version);
    EXPECT_NEAR(0.0f, info.param3, EPS);
    EXPECT_NEAR(0.0f, info.param4, EPS);
    EXPECT_NEAR(0.0f, info.potential[0], EPS);
    EXPECT_NEAR(0.0f, info.potential[1], EPS);
    EXPECT_NEAR(0.0f, info.potential[2], EPS);
    EXPECT_EQ(29582, info.random_seed);
    EXPECT_EQ(0, info.random_seed_flag);
    EXPECT_EQ(-1, info.biomorph);
    EXPECT_EQ(0, info.inside);
    EXPECT_EQ(0, info.log_map_old);
    EXPECT_NEAR(0.0f, info.invert[0], EPS);
    EXPECT_NEAR(0.0f, info.invert[1], EPS);
    EXPECT_NEAR(0.0f, info.invert[2], EPS);
    EXPECT_EQ(0, info.decomp[0]);
    EXPECT_EQ(192, info.decomp[1]);
    EXPECT_EQ(999, info.symmetry);
    EXPECT_EQ(0, info.init3d[0]);
    EXPECT_EQ(60, info.init3d[1]);
    EXPECT_EQ(30, info.init3d[2]);
    EXPECT_EQ(0, info.init3d[3]);
    EXPECT_EQ(90, info.init3d[4]);
    EXPECT_EQ(90, info.init3d[5]);
    EXPECT_EQ(30, info.init3d[6]);
    EXPECT_EQ(0, info.init3d[7]);
    EXPECT_EQ(0, info.init3d[8]);
    EXPECT_EQ(0, info.init3d[9]);
    EXPECT_EQ(0, info.init3d[10]);
    EXPECT_EQ(0, info.init3d[11]);
    EXPECT_EQ(1, info.init3d[12]);
    EXPECT_EQ(-1, info.init3d[13]);
    EXPECT_EQ(1, info.init3d[14]);
    EXPECT_EQ(0, info.init3d[15]);
    EXPECT_EQ(20, info.preview_factor);
    EXPECT_EQ(0, info.x_trans);
    EXPECT_EQ(0, info.y_trans);
    EXPECT_EQ(4, info.red_crop_left);
    EXPECT_EQ(0, info.red_crop_right);
    EXPECT_EQ(0, info.blue_crop_left);
    EXPECT_EQ(4, info.blue_crop_right);
    EXPECT_EQ(80, info.red_bright);
    EXPECT_EQ(100, info.blue_bright);
    EXPECT_EQ(0, info.x_adjust);
    EXPECT_EQ(0, info.eye_separation);
    EXPECT_EQ(0, info.glasses_type);
    EXPECT_EQ(-1, info.outside);
    EXPECT_NEAR(-2.5, info.x3rd, EPS);
    EXPECT_NEAR(-1.5, info.y3rd, EPS);
    EXPECT_EQ('g', info.std_calc_mode);
    EXPECT_EQ(0, info.use_init_orbit);
    EXPECT_EQ(4, info.calc_status);
    EXPECT_EQ(521, info.tot_extend_len);
    EXPECT_EQ(0, info.dist_est_old);
    EXPECT_EQ(0, info.float_flag);
    EXPECT_EQ(0, info.bailout_old);
    EXPECT_EQ(105, info.calc_time);
    EXPECT_EQ(0, info.trig_index[0]);
    EXPECT_EQ(6, info.trig_index[1]);
    EXPECT_EQ(2, info.trig_index[2]);
    EXPECT_EQ(3, info.trig_index[3]);
    EXPECT_EQ(0, info.finite_attractor);
    EXPECT_NEAR(0.0, info.init_orbit[0], EPS);
    EXPECT_NEAR(0.0, info.init_orbit[1], EPS);
    EXPECT_EQ(1, info.periodicity);
    EXPECT_EQ(0, info.pot16bit);
    EXPECT_NEAR(0.75, info.final_aspect_ratio, EPS);
    EXPECT_EQ(0, info.system);
    EXPECT_EQ(2004, info.release);
    EXPECT_EQ(0, info.display_3d);
    EXPECT_EQ(0, info.transparent[0] & 0xFF);
    EXPECT_EQ(0, info.transparent[1] & 0xFF);
    EXPECT_EQ(20, info.ambient);
    EXPECT_EQ(0, info.haze);
    EXPECT_EQ(0, info.randomize);
    EXPECT_EQ(1, info.rotate_lo);
    EXPECT_EQ(255, info.rotate_hi);
    EXPECT_EQ(71, info.dist_est_width);
    EXPECT_NEAR(0.0, info.d_param3, EPS);
    EXPECT_NEAR(0.0, info.d_param4, EPS);
    EXPECT_EQ(-1, info.fill_color);
    EXPECT_NEAR(-0.83, info.julibrot_x_max, EPS);
    EXPECT_NEAR(-0.83, info.julibrot_x_min, EPS);
    EXPECT_NEAR(0.25, info.julibrot_y_max, EPS);
    EXPECT_NEAR(-0.25, info.julibrot_y_min, EPS);
    EXPECT_EQ(128, info.julibrot_z_dots);
    EXPECT_NEAR(8.0, info.julibrot_origin_fp, EPS);
    EXPECT_NEAR(8.0, info.julibrot_depth_fp, EPS);
    EXPECT_NEAR(7.0, info.julibrot_height_fp, EPS);
    EXPECT_NEAR(10.0, info.julibrot_width_fp, EPS);
    EXPECT_NEAR(24.0, info.julibrot_dist_fp, EPS);
    EXPECT_NEAR(2.5, info.eyes_fp, EPS);
    EXPECT_EQ(1, info.orbit_type);
    EXPECT_EQ(0, info.juli3d_mode);
    EXPECT_EQ(0, info.max_fn);
    EXPECT_EQ(0, info.inverse_julia);
    EXPECT_NEAR(0.0, info.d_param5, EPS);
    EXPECT_NEAR(0.0, info.d_param6, EPS);
    EXPECT_NEAR(0.0, info.d_param7, EPS);
    EXPECT_NEAR(0.0, info.d_param8, EPS);
    EXPECT_NEAR(0.0, info.d_param9, EPS);
    EXPECT_NEAR(0.0, info.d_param10, EPS);
    EXPECT_EQ(0, info.bailout);
    EXPECT_EQ(0, info.bailout_test);
    EXPECT_EQ(INITIAL_MAX_ITERATIONS, info.iterations);
    EXPECT_EQ(0, info.bf_math);
    EXPECT_EQ(0, info.bf_length);
    EXPECT_EQ(0, info.y_adjust);
    EXPECT_EQ(0, info.old_demm_colors);
    EXPECT_EQ(0, info.log_map);
    EXPECT_EQ(0, info.dist_est);
    EXPECT_NEAR(0, info.d_invert[0], EPS);
    EXPECT_NEAR(0, info.d_invert[1], EPS);
    EXPECT_NEAR(0, info.d_invert[2], EPS);
    EXPECT_EQ(0, info.log_calc);
    EXPECT_EQ(0, info.stop_pass);
    EXPECT_EQ(0, info.quick_calc);
    EXPECT_NEAR(0.01, info.close_prox, EPS);
    EXPECT_EQ(0, info.no_bof);
    EXPECT_EQ(1, info.orbit_interval);
    EXPECT_EQ(0, info.orbit_delay);
    EXPECT_NEAR(0.05, info.math_tol[0], EPS);
    EXPECT_NEAR(0.05, info.math_tol[1], EPS);
}

TEST_F(TestGIFFractalInfoExtension, encode)
{
    const FractalInfo info{get_fractal_info(m_gif)};
    GIFOutputFile out{ID_TEST_GIF_WRITE1_FILE};

    put_fractal_info(out, info);

    const FractalInfo info2{get_fractal_info(out)};
    EXPECT_EQ(info, info2);
}

TEST_F(TestGIFResumeInfoExtension, check)
{
    EXPECT_NE(GIF_ERROR, m_result);
    EXPECT_EQ(7, m_gif->ExtensionBlockCount);
    EXPECT_EQ(11, m_gif->ExtensionBlocks[0].ByteCount);
    EXPECT_EQ("fractint002", get_extension_name(0));
    EXPECT_EQ(GIF_EXTENSION2_RESUME_INFO_LENGTH, extension_total(1, 2));
    EXPECT_EQ(11, m_gif->ExtensionBlocks[2].ByteCount);
    EXPECT_EQ("fractint003", get_extension_name(2));
    EXPECT_EQ(GIF_EXTENSION3_ITEM_NAME_INFO_LENGTH, extension_total(3, 4));
    EXPECT_EQ(11, m_gif->ExtensionBlocks[4].ByteCount);
    EXPECT_EQ("fractint001", get_extension_name(4));
    EXPECT_EQ(GIF_EXTENSION1_FRACTAL_INFO_LENGTH, extension_total(5, 7));
}

TEST_F(TestGIFItemNameInfoExtension, check)
{
    EXPECT_NE(GIF_ERROR, m_result);
    EXPECT_EQ(5, m_gif->ExtensionBlockCount);
    EXPECT_EQ(11, m_gif->ExtensionBlocks[0].ByteCount);
    EXPECT_EQ("fractint003", get_extension_name(0));
    EXPECT_EQ(GIF_EXTENSION3_ITEM_NAME_INFO_LENGTH, extension_total(1, 2));
    EXPECT_EQ(11, m_gif->ExtensionBlocks[2].ByteCount);
    EXPECT_EQ("fractint001", get_extension_name(2));
    EXPECT_EQ(GIF_EXTENSION1_FRACTAL_INFO_LENGTH, extension_total(3, 5));
}

TEST_F(TestGIFItemNameInfoExtension, decode)
{
    const FormulaInfo info{get_formula_info(m_gif)};

    EXPECT_EQ("Mandel3", trim(info.form_name));
    EXPECT_EQ(0, info.uses_p1);
    EXPECT_EQ(0, info.uses_p2);
    EXPECT_EQ(0, info.uses_p3);
    EXPECT_EQ(0, info.uses_ismand);
    EXPECT_EQ(1, info.ismand);
    EXPECT_EQ(0, info.uses_p4);
    EXPECT_EQ(0, info.uses_p5);
}

TEST_F(TestGIFItemNameInfoExtension, encode)
{
    const FormulaInfo info{get_formula_info(m_gif)};
    GIFOutputFile out{ID_TEST_GIF_WRITE3_FILE};

    put_formula_info(out, info);

    const FormulaInfo info2{get_formula_info(out)};
    EXPECT_EQ(info, info2);
}

TEST_F(TestGIFRangesInfoExtension, check)
{
    constexpr int num_ranges = 5;
    constexpr int GIF_EXTENSION4_RANGES_INFO_LENGTH = num_ranges * 2;
    EXPECT_NE(GIF_ERROR, m_result);
    EXPECT_EQ(5, m_gif->ExtensionBlockCount);
    EXPECT_EQ(11, m_gif->ExtensionBlocks[0].ByteCount);
    EXPECT_EQ("fractint004", get_extension_name(0));
    EXPECT_EQ(GIF_EXTENSION4_RANGES_INFO_LENGTH, extension_total(1, 2));
    EXPECT_EQ(11, m_gif->ExtensionBlocks[2].ByteCount);
    EXPECT_EQ("fractint001", get_extension_name(2));
    EXPECT_EQ(GIF_EXTENSION1_FRACTAL_INFO_LENGTH, extension_total(3, 5));
}

TEST_F(TestGIFRangesInfoExtension, decode)
{
    const std::vector info{get_ranges_info(m_gif)};

    ASSERT_EQ(5, info.size());
    EXPECT_EQ(10, info[0]);
    EXPECT_EQ(20, info[1]);
    EXPECT_EQ(30, info[2]);
    EXPECT_EQ(40, info[3]);
    EXPECT_EQ(50, info[4]);
}

TEST_F(TestGIFRangesInfoExtension, encode)
{
    const std::vector info1{get_ranges_info(m_gif)};
    GIFOutputFile out{ID_TEST_GIF_WRITE4_FILE};

    put_ranges_info(out, info1);

    const std::vector info2{get_ranges_info(out)};
    EXPECT_EQ(info1, info2) << VecPrinter(info1) << " != " << VecPrinter(info2);
}

TEST_F(TestGIFBigNumParametersExtension, check)
{
    EXPECT_NE(GIF_ERROR, m_result);
    EXPECT_EQ(6, m_gif->ExtensionBlockCount);
    EXPECT_EQ(11, m_gif->ExtensionBlocks[0].ByteCount);
    EXPECT_EQ("fractint005", get_extension_name(0));
    // Extension length is determined at runtime based on bignum digit length;
    // 6 + 6 + 10 bignums are stored in the block.
    EXPECT_EQ(0, extension_total(1, 3) % 22);
    EXPECT_EQ(11, m_gif->ExtensionBlocks[3].ByteCount);
    EXPECT_EQ("fractint001", get_extension_name(3));
    EXPECT_EQ(GIF_EXTENSION1_FRACTAL_INFO_LENGTH, extension_total(4, 6));
}

TEST_F(TestGIFBigNumParametersExtension, decode)
{
    const std::vector info{get_extended_param_info(m_gif)};

    ASSERT_EQ(396, info.size());
}

TEST_F(TestGIFBigNumParametersExtension, encode)
{
    const std::vector info{get_extended_param_info(m_gif)};
    GIFOutputFile out{ID_TEST_GIF_WRITE5_FILE};

    put_extended_param_info(out, info);

    const std::vector info2{get_extended_param_info(m_gif)};
    EXPECT_EQ(info, info2);
}

TEST_F(TestGIFEvolutionInfoExtension, check)
{
    EXPECT_NE(GIF_ERROR, m_result);
    EXPECT_EQ(5, m_gif->ExtensionBlockCount);
    EXPECT_EQ(11, m_gif->ExtensionBlocks[0].ByteCount);
    EXPECT_EQ("fractint006", get_extension_name(0));
    EXPECT_EQ(GIF_EXTENSION6_EVOLVER_INFO_LENGTH, extension_total(1, 2));
    EXPECT_EQ(11, m_gif->ExtensionBlocks[2].ByteCount);
    EXPECT_EQ("fractint001", get_extension_name(2));
    EXPECT_EQ(GIF_EXTENSION1_FRACTAL_INFO_LENGTH, extension_total(3, 5));
}

TEST_F(TestGIFEvolutionInfoExtension, decode)
{
    const EvolutionInfo info{get_evolution_info(m_gif)};

    EXPECT_EQ(1, info.evolving);
    EXPECT_EQ(9, info.image_grid_size);
    EXPECT_EQ(20596U, info.this_generation_random_seed);
    EXPECT_NEAR(1.0, info.max_random_mutation, EPS);
    EXPECT_NEAR(4.0, info.x_parameter_range, EPS);
    EXPECT_NEAR(3.0, info.y_parameter_range, EPS);
    EXPECT_NEAR(-2.0, info.x_parameter_offset, EPS);
    EXPECT_NEAR(-1.5, info.y_parameter_offset, EPS);
    EXPECT_EQ(0, info.discrete_x_parameter_offset);
    EXPECT_EQ(0, info.discrete_y_parameter_offset);
    EXPECT_EQ(4, info.px);
    EXPECT_EQ(4, info.py);
    EXPECT_EQ(0, info.screen_x_offset);
    EXPECT_EQ(0, info.screen_y_offset);
    EXPECT_EQ(640, info.x_dots);
    EXPECT_EQ(480, info.y_dots);
    constexpr std::int16_t expected_mutate[NUM_GENES]{5, 5};
    for (size_t i = 0; i < std::size(expected_mutate); ++i)
    {
        EXPECT_EQ(expected_mutate[i], info.mutate[i]) << '[' << i << ']';
    }
    EXPECT_EQ(81, info.count);
}

TEST_F(TestGIFEvolutionInfoExtension, encode)
{
    const EvolutionInfo info1{get_evolution_info(m_gif)};
    GIFOutputFile out{ID_TEST_GIF_WRITE6_FILE};

    put_evolution_info(out, info1);

    const EvolutionInfo info2{get_evolution_info(out)};
    EXPECT_EQ(info1, info2) << info1 << " != " << info2;
}

TEST_F(TestGIFOrbitInfoExtension, check)
{
    EXPECT_NE(GIF_ERROR, m_result);
    EXPECT_EQ(7, m_gif->ExtensionBlockCount);
    EXPECT_EQ(11, m_gif->ExtensionBlocks[0].ByteCount);
    EXPECT_EQ("fractint002", get_extension_name(0));
    EXPECT_EQ(GIF_EXTENSION2_RESUME_INFO_LENGTH, extension_total(1, 2));
    EXPECT_EQ(11, m_gif->ExtensionBlocks[2].ByteCount);
    EXPECT_EQ("fractint007", get_extension_name(2));
    EXPECT_EQ(GIF_EXTENSION7_ORBIT_INFO_LENGTH, extension_total(3, 4));
    EXPECT_EQ(11, m_gif->ExtensionBlocks[4].ByteCount);
    EXPECT_EQ("fractint001", get_extension_name(4));
    EXPECT_EQ(GIF_EXTENSION1_FRACTAL_INFO_LENGTH, extension_total(5, 7));
}

TEST_F(TestGIFOrbitInfoExtension, decode)
{
    const OrbitsInfo info{get_orbits_info(m_gif)};
    EXPECT_NEAR(-1.0, info.orbit_corner_min_x, EPS);
    EXPECT_NEAR(1.0, info.orbit_corner_max_x, EPS);
    EXPECT_NEAR(-1.0, info.orbit_corner_min_y, EPS);
    EXPECT_NEAR(1.0, info.orbit_corner_max_y, EPS);
    EXPECT_NEAR(-1.0, info.orbit_corner_3rd_x, EPS);
    EXPECT_NEAR(-1.0, info.orbit_corner_3rd_y, EPS);
    EXPECT_EQ(1, info.keep_screen_coords);
    EXPECT_EQ('r', info.draw_mode);
}

TEST_F(TestGIFOrbitInfoExtension, encode)
{
    const OrbitsInfo info1{get_orbits_info(m_gif)};
    GIFOutputFile out{ID_TEST_GIF_WRITE6_FILE};

    put_orbits_info(out, info1);

    const OrbitsInfo info2{get_orbits_info(out)};
    EXPECT_EQ(info1, info2) << info1 << " != " << info2;
}

} // namespace id::test
