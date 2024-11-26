// SPDX-License-Identifier: GPL-3.0-only
//
#include <gif_file.h>

#include "test_data.h"

#include <fractype.h>
#include <loadfile.h>

#include <boost/endian/buffers.hpp>
#include <boost/endian/conversion.hpp>
#include <gif_lib.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <numeric>
#include <string>

constexpr double eps{1.0e-6};

enum
{
    GIF_EXTENSION2_RESUME_INFO_LENGTH = 196,
};

class TestOpenGIF : public ::testing::Test
{
public:
    TestOpenGIF() :
        m_name(ID_TEST_GIF_EXT1_FILE)
    {
    }
    TestOpenGIF(const char *name) :
        m_name(name)
    {
    }

protected:
    void SetUp() override;
    void TearDown() override;

    int extension_total(int begin, int end) const;
    std::string get_extension_name(int ext) const
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

int TestOpenGIF::extension_total(int begin, int end) const
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
    TestSlurpGIF(const char *name);
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

template <typename T, size_t N>
class array_printer
{
public:
    array_printer(const T (&value)[N]) :
        m_value(value)
    {
    }

    const T (&m_value)[N];
};

template <size_t N, typename T>
std::ostream &operator<<(std::ostream &str, const array_printer<T, N> &value)
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
std::ostream &operator<<(std::ostream &str, const array_printer<std::uint8_t, N> &value)
{
    str << "[ ";
    bool first{true};
    for (int i = 0; i < N; ++i)
    {
        if (!first)
        {
            str << ", ";
        }
        str << int(value.m_value[i]);
        first = false;
    }
    return str << " ]";
}

template <int N>
std::string trim(const char (&field)[N])
{
    const std::string text{field, N};
    const auto pos = text.find_last_not_of('\0');
    return pos == std::string::npos ? (text[0] == 0 ? std::string{} : text) : text.substr(0, pos + 1);
}

std::ostream &operator<<(std::ostream &str, const FRACTAL_INFO &value)
{
    return str << "{ "                                                       //
               << R"("info_id": ")" << trim(value.info_id) << '"'            //
               << R"(, "iterationsold": )" << value.iterationsold            //
               << R"(, "fractal_type": )" << value.fractal_type              //
               << R"(, "xmin": )" << value.xmin                              //
               << R"(, "xmax": )" << value.xmax                              //
               << R"(, "ymin": )" << value.ymin                              //
               << R"(, "ymax": )" << value.ymax                              //
               << R"(, "creal": )" << value.creal                            //
               << R"(, "cimag": )" << value.cimag                            //
               << R"(, "videomodeax": )" << value.videomodeax                //
               << R"(, "videomodebx": )" << value.videomodebx                //
               << R"(, "videomodecx": )" << value.videomodecx                //
               << R"(, "videomodedx": )" << value.videomodedx                //
               << R"(, "dotmode": )" << value.dotmode                        //
               << R"(, "xdots": )" << value.xdots                            //
               << R"(, "ydots": )" << value.ydots                            //
               << R"(, "colors": )" << value.colors                          //
               << R"(, "version": )" << value.version                        //
               << R"(, "parm3": )" << value.parm3                            //
               << R"(, "parm4": )" << value.parm4                            //
               << R"(, "potential": )" << array_printer(value.potential)     //
               << R"(, "rseed": )" << value.rseed                            //
               << R"(, "rflag": )" << value.rflag                            //
               << R"(, "biomorph": )" << value.biomorph                      //
               << R"(, "inside": )" << value.inside                          //
               << R"(, "logmapold": )" << value.logmapold                    //
               << R"(, "invert": )" << array_printer(value.invert)           //
               << R"(, "decomp": )" << array_printer(value.decomp)           //
               << R"(, "symmetry": )" << value.symmetry                      //
               << R"(, "init3d": )" << array_printer(value.init3d)           //
               << R"(, "previewfactor": )" << value.previewfactor            //
               << R"(, "xtrans": )" << value.xtrans                          //
               << R"(, "ytrans": )" << value.ytrans                          //
               << R"(, "red_crop_left": )" << value.red_crop_left            //
               << R"(, "red_crop_right": )" << value.red_crop_right          //
               << R"(, "blue_crop_left": )" << value.blue_crop_left          //
               << R"(, "blue_crop_right": )" << value.blue_crop_right        //
               << R"(, "red_bright": )" << value.red_bright                  //
               << R"(, "blue_bright": )" << value.blue_bright                //
               << R"(, "xadjust": )" << value.xadjust                        //
               << R"(, "eyeseparation": )" << value.eyeseparation            //
               << R"(, "glassestype": )" << value.glassestype                //
               << R"(, "outside": )" << value.outside                        //
               << R"(, "x3rd": )" << value.x3rd                              //
               << R"(, "y3rd": )" << value.y3rd                              //
               << R"(, "stdcalcmode": )" << int(value.stdcalcmode)           //
               << R"(, "useinitorbit": )" << int(value.useinitorbit)         //
               << R"(, "calc_status": )" << value.calc_status                //
               << R"(, "tot_extend_len": )" << value.tot_extend_len          //
               << R"(, "distestold": )" << value.distestold                  //
               << R"(, "floatflag": )" << value.floatflag                    //
               << R"(, "bailoutold": )" << value.bailoutold                  //
               << R"(, "calctime": )" << value.calctime                      //
               << R"(, "trigndx": )" << array_printer(value.trigndx)         //
               << R"(, "finattract": )" << value.finattract                  //
               << R"(, "initorbit": )" << array_printer(value.initorbit)     //
               << R"(, "periodicity": )" << value.periodicity                //
               << R"(, "pot16bit": )" << value.pot16bit                      //
               << R"(, "faspectratio": )" << value.faspectratio              //
               << R"(, "system": )" << value.system                          //
               << R"(, "release": )" << value.release                        //
               << R"(, "display_3d": )" << value.display_3d                  //
               << R"(, "transparent": )" << array_printer(value.transparent) //
               << R"(, "ambient": )" << value.ambient                        //
               << R"(, "haze": )" << value.haze                              //
               << R"(, "randomize": )" << value.randomize                    //
               << R"(, "rotate_lo": )" << value.rotate_lo                    //
               << R"(, "rotate_hi": )" << value.rotate_hi                    //
               << R"(, "distestwidth": )" << value.distestwidth              //
               << R"(, "dparm3": )" << value.dparm3                          //
               << R"(, "dparm4": )" << value.dparm4                          //
               << R"(, "fillcolor": )" << value.fillcolor                    //
               << R"(, "mxmaxfp": )" << value.mxmaxfp                        //
               << R"(, "mxminfp": )" << value.mxminfp                        //
               << R"(, "mymaxfp": )" << value.mymaxfp                        //
               << R"(, "myminfp": )" << value.myminfp                        //
               << R"(, "zdots": )" << value.zdots                            //
               << R"(, "originfp": )" << value.originfp                      //
               << R"(, "depthfp": )" << value.depthfp                        //
               << R"(, "heightfp": )" << value.heightfp                      //
               << R"(, "widthfp": )" << value.widthfp                        //
               << R"(, "distfp": )" << value.distfp                          //
               << R"(, "eyesfp": )" << value.eyesfp                          //
               << R"(, "orbittype": )" << value.orbittype                    //
               << R"(, "juli3Dmode": )" << value.juli3Dmode                  //
               << R"(, "maxfn": )" << value.maxfn                            //
               << R"(, "inversejulia": )" << value.inversejulia              //
               << R"(, "dparm5": )" << value.dparm5                          //
               << R"(, "dparm6": )" << value.dparm6                          //
               << R"(, "dparm7": )" << value.dparm7                          //
               << R"(, "dparm8": )" << value.dparm8                          //
               << R"(, "dparm9": )" << value.dparm9                          //
               << R"(, "dparm10": )" << value.dparm10                        //
               << R"(, "bailout": )" << value.bailout                        //
               << R"(, "bailoutest": )" << value.bailoutest                  //
               << R"(, "iterations": )" << value.iterations                  //
               << R"(, "bf_math": )" << value.bf_math                        //
               << R"(, "g_bf_length": )" << value.g_bf_length                      //
               << R"(, "yadjust": )" << value.yadjust                        //
               << R"(, "old_demm_colors": )" << value.old_demm_colors        //
               << R"(, "logmap": )" << value.logmap                          //
               << R"(, "distest": )" << value.distest                        //
               << R"(, "dinvert": )" << array_printer(value.dinvert)         //
               << R"(, "logcalc": )" << value.logcalc                        //
               << R"(, "stoppass": )" << value.stoppass                      //
               << R"(, "quick_calc": )" << value.quick_calc                  //
               << R"(, "closeprox": )" << value.closeprox                    //
               << R"(, "nobof": )" << value.nobof                            //
               << R"(, "orbit_interval": )" << value.orbit_interval          //
               << R"(, "orbit_delay": )" << value.orbit_delay                //
               << R"(, "math_tol": )" << array_printer(value.math_tol)       //
               << " }";                                                      //
}

std::ostream &operator<<(std::ostream &str, const formula_info &value)
{
    return str << R"({ "form_name": ")" << trim(value.form_name) << '"' //
               << R"(, "uses_p1": )" << value.uses_p1                 //
               << R"(, "uses_p2": )" << value.uses_p2                 //
               << R"(, "uses_p3": )" << value.uses_p3                 //
               << R"(, "uses_ismand": )" << value.uses_ismand         //
               << R"(, "ismand": )" << value.ismand                   //
               << R"(, "uses_p4": )" << value.uses_p4                 //
               << R"(, "uses_p5": )" << value.uses_p5                 //
               << " }";
}

template <typename T>
class vec_printer
{
public:
    vec_printer(const std::vector<T> &values) :
        m_values(values)
    {
    }
    const std::vector<T> &m_values;
};

std::ostream &operator<<(std::ostream &str, const vec_printer<int> &value)
{
    if (value.m_values.empty())
    {
        return str << "[]";
    }
    
    str << "[ ";
    bool first{true};
    for (int i : value.m_values)
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

std::ostream &operator<<(std::ostream&str, const EvolutionInfo &value)
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
        << R"(, "discrete_y_paramter_offset": )" << value.discrete_y_paramter_offset   //
        << R"(, "px": )" << value.px                                                   //
        << R"(, "py": )" << value.py                                                   //
        << R"(, "sxoffs": )" << value.sxoffs                                           //
        << R"(, "syoffs": )" << value.syoffs                                           //
        << R"(, "xdots": )" << value.xdots                                             //
        << R"(, "ydots": )" << value.ydots                                             //
        << R"(, "mutate": )" << array_printer(value.mutate)                            //
        << R"(, "ecount": )" << value.ecount                                           //
        << " }";                                                                       //
}

std::ostream &operator<<(std::ostream &str, const ORBITS_INFO &value)
{
    return str                                                                            //
        << R"({ "oxmin": )" << value.oxmin                                                //
        << R"( "oxmax": )" << value.oxmax                                                 //
        << R"( "oymin": )" << value.oymin                                                 //
        << R"( "oymax": )" << value.oymax                                                 //
        << R"( "ox3rd": )" << value.ox3rd                                                 //
        << R"( "oy3rd": )" << value.oy3rd                                                 //
        << R"( "keep_scrn_coords": )" << (value.keep_scrn_coords != 0 ? "true" : "false") //
        << R"( "drawmode": ")" << value.drawmode << '"'                                   //
        << " }";                                                                          //
}

class GIFOutputFile
{
public:
    GIFOutputFile(const char *path) :
        m_gif(EGifOpenFileName(path, false, &m_gif_error))
    {
        if (m_gif_error != E_GIF_SUCCEEDED)
        {
            throw std::runtime_error(
                "Unexpected error opening " + std::string{path} + " for writing: " + std::to_string(m_gif_error));
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
    const FRACTAL_INFO lhs{};
    FRACTAL_INFO rhs{};
    std::int16_t value{16};
    for (size_t i = 0; i < std::size(rhs.future); ++i)
    {
        rhs.future[i] = value;
        ++value;
    }

    EXPECT_TRUE(lhs == rhs) << lhs << " != " << rhs;
}

TEST(TestCompareFormulaInfo, futureFieldsAreIgnored)
{
    const formula_info lhs{};
    formula_info rhs{};
    std::int16_t value{16};
    for (size_t i = 0; i < std::size(rhs.future); ++i)
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
    const FRACTAL_INFO info{get_fractal_info(m_gif)};

    EXPECT_EQ("Fractal", trim(info.info_id));
    EXPECT_EQ(150, info.iterationsold);
    EXPECT_EQ(+fractal_type::MANDEL, info.fractal_type);
    EXPECT_NEAR(-2.5, info.xmin, eps);
    EXPECT_NEAR(1.5, info.xmax, eps);
    EXPECT_NEAR(-1.5, info.ymin, eps);
    EXPECT_NEAR(1.5, info.ymax, eps);
    EXPECT_NEAR(0.0, info.creal, eps);
    EXPECT_NEAR(0.0, info.cimag, eps);
    EXPECT_EQ(0, info.videomodeax);
    EXPECT_EQ(0, info.videomodebx);
    EXPECT_EQ(0, info.videomodecx);
    EXPECT_EQ(0, info.videomodedx);
    EXPECT_EQ(27, info.dotmode);
    EXPECT_EQ(640, info.xdots);
    EXPECT_EQ(480, info.ydots);
    EXPECT_EQ(256, info.colors);
    EXPECT_EQ(17, info.version);
    EXPECT_NEAR(0.0f, info.parm3, eps);
    EXPECT_NEAR(0.0f, info.parm4, eps);
    EXPECT_NEAR(0.0f, info.potential[0], eps);
    EXPECT_NEAR(0.0f, info.potential[1], eps);
    EXPECT_NEAR(0.0f, info.potential[2], eps);
    EXPECT_EQ(29582, info.rseed);
    EXPECT_EQ(0, info.rflag);
    EXPECT_EQ(-1, info.biomorph);
    EXPECT_EQ(0, info.inside);
    EXPECT_EQ(0, info.logmapold);
    EXPECT_NEAR(0.0f, info.invert[0], eps);
    EXPECT_NEAR(0.0f, info.invert[1], eps);
    EXPECT_NEAR(0.0f, info.invert[2], eps);
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
    EXPECT_EQ(20, info.previewfactor);
    EXPECT_EQ(0, info.xtrans);
    EXPECT_EQ(0, info.ytrans);
    EXPECT_EQ(4, info.red_crop_left);
    EXPECT_EQ(0, info.red_crop_right);
    EXPECT_EQ(0, info.blue_crop_left);
    EXPECT_EQ(4, info.blue_crop_right);
    EXPECT_EQ(80, info.red_bright);
    EXPECT_EQ(100, info.blue_bright);
    EXPECT_EQ(0, info.xadjust);
    EXPECT_EQ(0, info.eyeseparation);
    EXPECT_EQ(0, info.glassestype);
    EXPECT_EQ(-1, info.outside);
    EXPECT_NEAR(-2.5, info.x3rd, eps);
    EXPECT_NEAR(-1.5, info.y3rd, eps);
    EXPECT_EQ('g', info.stdcalcmode);
    EXPECT_EQ(0, info.useinitorbit);
    EXPECT_EQ(4, info.calc_status);
    EXPECT_EQ(521, info.tot_extend_len);
    EXPECT_EQ(0, info.distestold);
    EXPECT_EQ(0, info.floatflag);
    EXPECT_EQ(0, info.bailoutold);
    EXPECT_EQ(105, info.calctime);
    EXPECT_EQ(0, info.trigndx[0]);
    EXPECT_EQ(6, info.trigndx[1]);
    EXPECT_EQ(2, info.trigndx[2]);
    EXPECT_EQ(3, info.trigndx[3]);
    EXPECT_EQ(0, info.finattract);
    EXPECT_NEAR(0.0, info.initorbit[0], eps);
    EXPECT_NEAR(0.0, info.initorbit[1], eps);
    EXPECT_EQ(1, info.periodicity);
    EXPECT_EQ(0, info.pot16bit);
    EXPECT_NEAR(0.75, info.faspectratio, eps);
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
    EXPECT_EQ(71, info.distestwidth);
    EXPECT_NEAR(0.0, info.dparm3, eps);
    EXPECT_NEAR(0.0, info.dparm4, eps);
    EXPECT_EQ(-1, info.fillcolor);
    EXPECT_NEAR(-0.83, info.mxmaxfp, eps);
    EXPECT_NEAR(-0.83, info.mxminfp, eps);
    EXPECT_NEAR(0.25, info.mymaxfp, eps);
    EXPECT_NEAR(-0.25, info.myminfp, eps);
    EXPECT_EQ(128, info.zdots);
    EXPECT_NEAR(8.0, info.originfp, eps);
    EXPECT_NEAR(8.0, info.depthfp, eps);
    EXPECT_NEAR(7.0, info.heightfp, eps);
    EXPECT_NEAR(10.0, info.widthfp, eps);
    EXPECT_NEAR(24.0, info.distfp, eps);
    EXPECT_NEAR(2.5, info.eyesfp, eps);
    EXPECT_EQ(1, info.orbittype);
    EXPECT_EQ(0, info.juli3Dmode);
    EXPECT_EQ(0, info.maxfn);
    EXPECT_EQ(0, info.inversejulia);
    EXPECT_NEAR(0.0, info.dparm5, eps);
    EXPECT_NEAR(0.0, info.dparm6, eps);
    EXPECT_NEAR(0.0, info.dparm7, eps);
    EXPECT_NEAR(0.0, info.dparm8, eps);
    EXPECT_NEAR(0.0, info.dparm9, eps);
    EXPECT_NEAR(0.0, info.dparm10, eps);
    EXPECT_EQ(0, info.bailout);
    EXPECT_EQ(0, info.bailoutest);
    EXPECT_EQ(150, info.iterations);
    EXPECT_EQ(0, info.bf_math);
    EXPECT_EQ(0, info.g_bf_length);
    EXPECT_EQ(0, info.yadjust);
    EXPECT_EQ(0, info.old_demm_colors);
    EXPECT_EQ(0, info.logmap);
    EXPECT_EQ(0, info.distest);
    EXPECT_NEAR(0, info.dinvert[0], eps);
    EXPECT_NEAR(0, info.dinvert[1], eps);
    EXPECT_NEAR(0, info.dinvert[2], eps);
    EXPECT_EQ(0, info.logcalc);
    EXPECT_EQ(0, info.stoppass);
    EXPECT_EQ(0, info.quick_calc);
    EXPECT_NEAR(0.01, info.closeprox, eps);
    EXPECT_EQ(0, info.nobof);
    EXPECT_EQ(1, info.orbit_interval);
    EXPECT_EQ(0, info.orbit_delay);
    EXPECT_NEAR(0.05, info.math_tol[0], eps);
    EXPECT_NEAR(0.05, info.math_tol[1], eps);
}

TEST_F(TestGIFFractalInfoExtension, encode)
{
    const FRACTAL_INFO info{get_fractal_info(m_gif)};
    GIFOutputFile out{ID_TEST_GIF_WRITE1_FILE};

    put_fractal_info(out, info);

    const FRACTAL_INFO info2{get_fractal_info(out)};
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
    const formula_info info{get_formula_info(m_gif)};

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
    const formula_info info{get_formula_info(m_gif)};
    GIFOutputFile out{ID_TEST_GIF_WRITE3_FILE};

    put_formula_info(out, info);

    const formula_info info2{get_formula_info(out)};
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
    EXPECT_EQ(info1, info2) << vec_printer(info1) << " != " << vec_printer(info2);
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
    EXPECT_NEAR(1.0, info.max_random_mutation, eps);
    EXPECT_NEAR(4.0, info.x_parameter_range, eps);
    EXPECT_NEAR(3.0, info.y_parameter_range, eps);
    EXPECT_NEAR(-2.0, info.x_parameter_offset, eps);
    EXPECT_NEAR(-1.5, info.y_parameter_offset, eps);
    EXPECT_EQ(0, info.discrete_x_parameter_offset);
    EXPECT_EQ(0, info.discrete_y_paramter_offset);
    EXPECT_EQ(4, info.px);
    EXPECT_EQ(4, info.py);
    EXPECT_EQ(0, info.sxoffs);
    EXPECT_EQ(0, info.syoffs);
    EXPECT_EQ(640, info.xdots);
    EXPECT_EQ(480, info.ydots);
    constexpr std::int16_t expected_mutate[NUM_GENES]{5, 5};
    for (size_t i = 0; i < std::size(expected_mutate); ++i)
    {
        EXPECT_EQ(expected_mutate[i], info.mutate[i]) << '[' << i << ']';
    }
    EXPECT_EQ(81, info.ecount);
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
    const ORBITS_INFO info{get_orbits_info(m_gif)};
    EXPECT_NEAR(-1.0, info.oxmin, eps);
    EXPECT_NEAR(1.0, info.oxmax, eps);
    EXPECT_NEAR(-1.0, info.oymin, eps);
    EXPECT_NEAR(1.0, info.oymax, eps);
    EXPECT_NEAR(-1.0, info.ox3rd, eps);
    EXPECT_NEAR(-1.0, info.oy3rd, eps);
    EXPECT_EQ(1, info.keep_scrn_coords);
    EXPECT_EQ('r', info.drawmode);
}

TEST_F(TestGIFOrbitInfoExtension, encode)
{
    const ORBITS_INFO info1{get_orbits_info(m_gif)};
    GIFOutputFile out{ID_TEST_GIF_WRITE6_FILE};

    put_orbits_info(out, info1);

    const ORBITS_INFO info2{get_orbits_info(out)};
    EXPECT_EQ(info1, info2) << info1 << " != " << info2;
}
