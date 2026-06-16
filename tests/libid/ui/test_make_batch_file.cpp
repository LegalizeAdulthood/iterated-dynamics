// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/make_batch_file.h>

#include "ColorMapSaver.h"
#include "engine/calcfrac.h"
#include "engine/color_utils.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "misc/debug_flags.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"
#include "ui/stereo.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <numeric>
#include <string>
#include <string_view>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;
using namespace id::ui;

namespace id::test
{

class TestPutEncodedColors : public testing::Test
{
protected:
    void fill_dac_from_values(int count);
    void up_down_values(int count);
    void up_down_values(int count, int up, int down);
    void iota4(int count);
    ColorMapSaver m_saved_dac_box;
    ValueSaver<Version> m_saved_version{g_version, current_id_version()};
    std::array<Byte, 256> values{};
    WriteBatchData m_data{};
};

class MakeBatchParamSaver
{
public:
    MakeBatchParamSaver()
    {
        std::copy(&g_params[0], &g_params[MAX_PARAMS], m_params.begin());
        m_param_text = g_param_text;
    }
    ~MakeBatchParamSaver()
    {
        std::copy(m_params.begin(), m_params.end(), &g_params[0]);
        g_param_text = m_param_text;
    }

private:
    std::array<double, MAX_PARAMS> m_params{};
    std::array<std::string, MAX_PARAMS> m_param_text;
};

class TestPutFractalParams : public testing::Test
{
protected:
    void SetUp() override
    {
        std::fill(&g_params[0], &g_params[MAX_PARAMS], 0.0);
        g_param_text.fill({});
        g_params[0] = 1100.0;
        g_params[1] = 1200000.0;
        g_params[2] = 3.0;
        g_params[3] = 2.0;
    }

    MakeBatchParamSaver m_saved_params;
    ValueSaver<FractalType> m_saved_fractal_type{g_fractal_type, FractalType::ANT};
    ValueSaver<FractalSpecific *> m_saved_fractal_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::ANT)};
    ValueSaver<DebugFlags> m_saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    WriteBatchData m_data{};
};

class TestPutRdsParams : public testing::Test
{
protected:
    WriteBatchData m_data{};
};

void TestPutEncodedColors::fill_dac_from_values(const int count)
{
    for (int i =0; i < count; ++i)
    {
        g_dac_box[i][0] = values[i];
        g_dac_box[i][1] = values[i];
        g_dac_box[i][2] = values[i];
    }
}

void TestPutEncodedColors::up_down_values(const int count)
{
    constexpr int up{0};
    const int down{static_cast<Byte>(count * 2 - 1)};
    up_down_values(count, up, down);
}

void TestPutEncodedColors::up_down_values(const int count, int up, int down)
{
    int offset{};
    for (int i = 0; i < count; ++i)
    {
        values[offset] = static_cast<Byte>(up * 4);
        ++up;
        ++offset;
        values[offset] = static_cast<Byte>(down * 4);
        --down;
        ++offset;
    }
}

void TestPutEncodedColors::iota4(const int count)
{
    std::generate_n(values.begin(), count,
        []
        {
            static int value{};
            const int result{value};
            value += 4;
            return result;
        });
}

TEST_F(TestPutEncodedColors, increasingSixBitRamp)
{
    for (int i = 0; i < 256; ++i)
    {
        const Byte val{static_cast<Byte>(i % 64 * 4)};
        g_dac_box[i][0] = val;
        g_dac_box[i][1] = val;
        g_dac_box[i][2] = val;
    }

    put_encoded_colors(m_data, 256);

    EXPECT_EQ("000<56>tttuuuvvv<3>zzz000<57>uuu<4>zzz000<57>uuu<4>zzz000<62>zzz", m_data.buf);
}

TEST_F(TestPutEncodedColors, values0Through9EncodedAsDigits)
{
    up_down_values(5);
    fill_dac_from_values(10);

    put_encoded_colors(m_data, 10);

    constexpr std::string_view expected{"000999111888222777333666444555"};
    EXPECT_EQ(expected, m_data.buf);
}

TEST_F(TestPutEncodedColors, values10Through35EncodedAsUpperCaseLetters)
{
    up_down_values(13, 10, 35);
    fill_dac_from_values(26);

    put_encoded_colors(m_data, 26);

    constexpr std::string_view expected{
        "AAAZZZBBBYYYCCCXXXDDDWWWEEEVVVFFFUUUGGGTTTHHHSSSIIIRRRJJJQQQKKKPPPLLLOOOMMMNNN"};
    EXPECT_EQ(expected, m_data.buf);
}

TEST_F(TestPutEncodedColors, values36Through63EncodedAsLowerCaseLetters)
{
    up_down_values(14, 36, 63);
    fill_dac_from_values(28);

    put_encoded_colors(m_data, 28);

    constexpr std::string_view expected{"___zzz"
                                        "```yyy"
                                        "aaaxxx"
                                        "bbbwww"
                                        "cccvvv"
                                        "ddduuu"
                                        "eeettt"
                                        "fffsss"
                                        "gggrrr"
                                        "hhhqqq"
                                        "iiippp"
                                        "jjjooo"
                                        "kkknnn"
                                        "lllmmm"};
    EXPECT_EQ(expected, m_data.buf);
}

TEST_F(TestPutEncodedColors, smoothFourColors)
{
    static constexpr int NUM_COLORS{4};
    iota4(NUM_COLORS);
    fill_dac_from_values(NUM_COLORS);

    put_encoded_colors(m_data, NUM_COLORS);

    constexpr std::string_view expected{"000<2>333"};
    EXPECT_EQ(expected, m_data.buf);
}

TEST_F(TestPutEncodedColors, smoothFiveColors)
{
    static constexpr int NUM_COLORS{5};
    iota4(NUM_COLORS);
    fill_dac_from_values(NUM_COLORS);

    put_encoded_colors(m_data, NUM_COLORS);

    constexpr std::string_view expected{"000<3>444"};
    EXPECT_EQ(expected, m_data.buf);
}

TEST_F(TestPutEncodedColors, twoLeastLSBSetEncodedAsHex)
{
    static constexpr int NUM_COLORS{1};
    std::iota(values.begin(), values.begin() + NUM_COLORS, 64U + 3U);
    fill_dac_from_values(NUM_COLORS);

    put_encoded_colors(m_data, NUM_COLORS);

    constexpr std::string_view expected{"#434343"};
    EXPECT_EQ(expected, m_data.buf);
}

TEST_F(TestPutEncodedColors, legacyVersionWritesExpandedSixBitRampAsLegacyColors)
{
    ValueSaver saved_version{g_version, parse_legacy_version(2000)};
    for (int i = 0; i < 256; ++i)
    {
        const Byte val{expand_6bit_color(static_cast<Byte>(i % 64))};
        g_dac_box[i][0] = val;
        g_dac_box[i][1] = val;
        g_dac_box[i][2] = val;
    }

    put_encoded_colors(m_data, 256);

    EXPECT_EQ("000<56>tttuuuvvv<3>zzz000<57>uuu<4>zzz000<57>uuu<4>zzz000<62>zzz", m_data.buf);
}

TEST_F(TestPutFractalParams, antUsesTextRuleForFirstParam)
{
    g_param_text[0] = "101001011001";

    put_fractal_params(m_data);

    constexpr std::string_view expected{"params=101001011001/1200000.0/3.0/2.0/0.0/0.0"};
    EXPECT_EQ(expected, m_data.buf);
}

TEST_F(TestPutFractalParams, antUsesNumericRuleWhenTextIsEmpty)
{
    put_fractal_params(m_data);

    constexpr std::string_view expected{"params=1100.0/1200000.0/3.0/2.0/0.0/0.0"};
    EXPECT_EQ(expected, m_data.buf);
}

TEST_F(TestPutRdsParams, skippedWhenImageIsNotRds)
{
    ValueSaver saved_save_rds_params{g_save_rds_params, false};

    put_rds_params(m_data);

    EXPECT_TRUE(m_data.buf.empty());
}

TEST_F(TestPutRdsParams, randomDotsIncludeDepthBarsWidthAndGrayscale)
{
    ValueSaver saved_save_rds_params{g_save_rds_params, true};
    ValueSaver saved_use_stereo_texture{g_use_stereo_texture, false};
    ValueSaver saved_auto_stereo_depth{g_auto_stereo_depth, -120};
    ValueSaver saved_calibrate{g_calibrate, CalibrationBars::TOP};
    ValueSaver saved_auto_stereo_width{g_auto_stereo_width, 4.5};
    ValueSaver saved_gray_flag{g_gray_flag, true};

    put_rds_params(m_data);

    constexpr std::string_view expected{"rds=random/-120/top stereowidth=4.5 usegrayscale=y"};
    EXPECT_EQ(expected, m_data.buf);
}

TEST_F(TestPutRdsParams, textureIncludesFilename)
{
    ValueSaver saved_save_rds_params{g_save_rds_params, true};
    ValueSaver saved_use_stereo_texture{g_use_stereo_texture, true};
    ValueSaver saved_stereo_texture_filename{g_stereo_texture_filename, "textures/Texture.GIF"};
    ValueSaver saved_auto_stereo_depth{g_auto_stereo_depth, 80};
    ValueSaver saved_calibrate{g_calibrate, CalibrationBars::NONE};
    ValueSaver saved_auto_stereo_width{g_auto_stereo_width, 10.0};
    ValueSaver saved_gray_flag{g_gray_flag, false};

    put_rds_params(m_data);

    constexpr std::string_view expected{"rds=texture/80/none rds-texture=Texture.GIF"};
    EXPECT_EQ(expected, m_data.buf);
}

} // namespace
