// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/make_batch_file.h>

#include "ColorMapSaver.h"
#include "FractalParamSaver.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/color_utils.h"
#include "engine/SolidGuess.h"
#include "engine/UserData.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "geometry/stereo_params.h"
#include "misc/debug_flags.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"
#include "ui/stereo.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <numeric>
#include <ostream>
#include <string>
#include <string_view>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::misc;
using namespace id::ui;

namespace id::test
{

class TestPutEncodedColors : public testing::Test
{
protected:
    void fill_dac_from_values(int count);
    void up_down_values(int count, int up, int down);
    void iota4(int count);
    ColorMapSaver m_saved_dac_box;
    ValueSaver<Version> m_saved_version{g_version, current_id_version()};
    std::array<Byte, 256> values{};
    WriteBatchData m_data{};
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

    FractalParamSaver m_saved_params;
    ValueSaver<FractalType> m_saved_fractal_type{g_fractal_type, FractalType::ANT};
    ValueSaver<const FractalSpecific *> m_saved_fractal_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::ANT)};
    ValueSaver<DebugFlags> m_saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    WriteBatchData m_data{};
};

class TestPutRdsParams : public testing::Test
{
protected:
    WriteBatchData m_data{};
};

class TestPutCalculationModeParams : public testing::Test
{
protected:
    WriteBatchData m_data{};
    ValueSaver<CalcMode> m_saved_std_calc_mode{g_user.std_calc_mode, CalcMode::SOLID_GUESS};
    ValueSaver<int> m_saved_stop_pass{g_stop_pass, 0};
    ValueSaver<PerturbationMode> m_saved_perturbation{g_user.perturbation, PerturbationMode::AUTO};
    ValueSaver<double> m_saved_perturbation_tolerance{g_user.perturbation_tolerance, DEFAULT_PERTURBATION_TOLERANCE};
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
    for (int i = 0; i < count; ++i)
    {
        values[i] = static_cast<Byte>(i * 4);
    }
}

struct EncodedSixBitRampInput
{
    const char *name;
    Version version;
    bool expanded;
};

void PrintTo(const EncodedSixBitRampInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestEncodedSixBitRamp : public TestPutEncodedColors, public testing::WithParamInterface<EncodedSixBitRampInput>
{
};

TEST_P(TestEncodedSixBitRamp, compressesRepeatedRamp)
{
    g_version = GetParam().version;
    for (int i = 0; i < 256; ++i)
    {
        const Byte val{
            GetParam().expanded ? expand_6bit_color(static_cast<Byte>(i % 64)) : static_cast<Byte>(i % 64 * 4)};
        g_dac_box[i][0] = val;
        g_dac_box[i][1] = val;
        g_dac_box[i][2] = val;
    }

    put_encoded_colors(m_data, 256);

    EXPECT_EQ("000<56>tttuuuvvv<3>zzz000<57>uuu<4>zzz000<57>uuu<4>zzz000<62>zzz", m_data.buf);
}

INSTANTIATE_TEST_SUITE_P(Representation, TestEncodedSixBitRamp,
    testing::Values(EncodedSixBitRampInput{"Modern", current_id_version(), false},
        EncodedSixBitRampInput{"Legacy", parse_legacy_version(2000), true}),
    [](const testing::TestParamInfo<EncodedSixBitRampInput> &info) { return info.param.name; });

struct EncodedColorRampInput
{
    const char *name;
    int count;
    int up;
    int down;
    const char *expected;
};

void PrintTo(const EncodedColorRampInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestEncodedColorRamp : public TestPutEncodedColors, public testing::WithParamInterface<EncodedColorRampInput>
{
};

TEST_P(TestEncodedColorRamp, encodesAlternatingRamp)
{
    up_down_values(GetParam().count / 2, GetParam().up, GetParam().down);
    fill_dac_from_values(GetParam().count);

    put_encoded_colors(m_data, GetParam().count);

    EXPECT_EQ(GetParam().expected, m_data.buf);
}

INSTANTIATE_TEST_SUITE_P(Range, TestEncodedColorRamp,
    testing::Values(EncodedColorRampInput{"Digits", 10, 0, 9, "000999111888222777333666444555"},
        EncodedColorRampInput{
            "UpperCase", 26, 10, 35, "AAAZZZBBBYYYCCCXXXDDDWWWEEEVVVFFFUUUGGGTTTHHHSSSIIIRRRJJJQQQKKKPPPLLLOOOMMMNNN"},
        EncodedColorRampInput{"LowerCase", 28, 36, 63,
            "___zzz"
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
            "lllmmm"}),
    [](const testing::TestParamInfo<EncodedColorRampInput> &info) { return info.param.name; });

struct SmoothColorInput
{
    int count;
    const char *expected;
};

void PrintTo(const SmoothColorInput &input, std::ostream *out)
{
    *out << input.count;
}

class TestEncodedSmoothColors : public TestPutEncodedColors, public testing::WithParamInterface<SmoothColorInput>
{
};

TEST_P(TestEncodedSmoothColors, compressesSmoothRamp)
{
    iota4(GetParam().count);
    fill_dac_from_values(GetParam().count);

    put_encoded_colors(m_data, GetParam().count);

    EXPECT_EQ(GetParam().expected, m_data.buf);
}

INSTANTIATE_TEST_SUITE_P(ColorCount, TestEncodedSmoothColors,
    testing::Values(SmoothColorInput{4, "000<2>333"}, SmoothColorInput{5, "000<3>444"}),
    [](const testing::TestParamInfo<SmoothColorInput> &info) { return "Colors" + std::to_string(info.param.count); });

TEST_F(TestPutEncodedColors, twoLeastLSBSetEncodedAsHex)
{
    static constexpr int NUM_COLORS{1};
    std::iota(values.begin(), values.begin() + NUM_COLORS, 64U + 3U);
    fill_dac_from_values(NUM_COLORS);

    put_encoded_colors(m_data, NUM_COLORS);

    constexpr std::string_view expected{"#434343"};
    EXPECT_EQ(expected, m_data.buf);
}

struct AntParameterInput
{
    const char *name;
    const char *rule;
    const char *expected;
};

void PrintTo(const AntParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestAntParameterSerialization : public TestPutFractalParams, public testing::WithParamInterface<AntParameterInput>
{
};

TEST_P(TestAntParameterSerialization, writesRuleParameter)
{
    g_param_text[0] = GetParam().rule;

    put_fractal_params(m_data);

    EXPECT_EQ(GetParam().expected, m_data.buf);
}

INSTANTIATE_TEST_SUITE_P(Rule, TestAntParameterSerialization,
    testing::Values(AntParameterInput{"Text", "101001011001", "params=101001011001/1200000.0/3.0/2.0/0.0/0.0"},
        AntParameterInput{"Numeric", "", "params=1100.0/1200000.0/3.0/2.0/0.0/0.0"}),
    [](const testing::TestParamInfo<AntParameterInput> &info) { return info.param.name; });

struct CalculationModeParameterInput
{
    const char *name;
    CalcMode mode;
    PerturbationMode perturbation;
    double tolerance;
    const char *expected;
};

void PrintTo(const CalculationModeParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestCalculationModeSerialization : public TestPutCalculationModeParams,
                                         public testing::WithParamInterface<CalculationModeParameterInput>
{
};

TEST_P(TestCalculationModeSerialization, writesNonDefaultSettings)
{
    g_user.std_calc_mode = GetParam().mode;
    g_user.perturbation = GetParam().perturbation;
    g_user.perturbation_tolerance = GetParam().tolerance;

    put_calculation_mode_params(m_data);

    EXPECT_EQ(GetParam().expected, m_data.buf);
}

INSTANTIATE_TEST_SUITE_P(Settings, TestCalculationModeSerialization,
    testing::Values(CalculationModeParameterInput{"Defaults", CalcMode::SOLID_GUESS, PerturbationMode::AUTO,
                        DEFAULT_PERTURBATION_TOLERANCE, ""},
        CalculationModeParameterInput{
            "OnePass", CalcMode::ONE_PASS, PerturbationMode::AUTO, DEFAULT_PERTURBATION_TOLERANCE, "passes=1"},
        CalculationModeParameterInput{"PerturbationYes", CalcMode::SOLID_GUESS, PerturbationMode::YES,
            DEFAULT_PERTURBATION_TOLERANCE, "perturbation=yes"},
        CalculationModeParameterInput{"PerturbationNo", CalcMode::SOLID_GUESS, PerturbationMode::NO,
            DEFAULT_PERTURBATION_TOLERANCE, "perturbation=no"},
        CalculationModeParameterInput{
            "Tolerance", CalcMode::SOLID_GUESS, PerturbationMode::AUTO, 0.001, "perturbation-tolerance=0.001"},
        CalculationModeParameterInput{"CompatibilityPass", CalcMode::PERTURBATION, PerturbationMode::AUTO,
            DEFAULT_PERTURBATION_TOLERANCE, "perturbation=yes"}),
    [](const testing::TestParamInfo<CalculationModeParameterInput> &info) { return info.param.name; });

struct RdsParameterInput
{
    const char *name;
    bool enabled;
    bool texture;
    int depth;
    CalibrationBars bars;
    double width;
    bool grayscale;
    const char *filename;
    const char *expected;
};

void PrintTo(const RdsParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRdsParameterSerialization : public TestPutRdsParams, public testing::WithParamInterface<RdsParameterInput>
{
protected:
    void SetUp() override;

    ValueSaver<bool> m_saved_enabled{g_save_rds_params};
    ValueSaver<bool> m_saved_texture{g_use_stereo_texture};
    ValueSaver<int> m_saved_depth{g_auto_stereo_depth};
    ValueSaver<CalibrationBars> m_saved_bars{g_calibrate};
    ValueSaver<double> m_saved_width{g_auto_stereo_width};
    ValueSaver<bool> m_saved_grayscale{g_gray_flag};
    ValueSaver<std::string> m_saved_filename{g_stereo_texture_filename};
};

void TestRdsParameterSerialization::SetUp()
{
    g_save_rds_params = GetParam().enabled;
    if (GetParam().enabled)
    {
        g_use_stereo_texture = GetParam().texture;
        g_auto_stereo_depth = GetParam().depth;
        g_calibrate = GetParam().bars;
        g_auto_stereo_width = GetParam().width;
        g_gray_flag = GetParam().grayscale;
        g_stereo_texture_filename = GetParam().filename;
    }
}

TEST_P(TestRdsParameterSerialization, writesSelectedSettings)
{
    put_rds_params(m_data);

    EXPECT_EQ(GetParam().expected, m_data.buf);
}

INSTANTIATE_TEST_SUITE_P(Settings, TestRdsParameterSerialization,
    testing::Values(RdsParameterInput{"Disabled", false, false, 0, CalibrationBars::NONE, 0.0, false, "", ""},
        RdsParameterInput{"RandomDots", true, false, -120, CalibrationBars::TOP, 4.5, true, "",
            "rds=random/-120/top stereowidth=4.5 usegrayscale=y"},
        RdsParameterInput{"Texture", true, true, 80, CalibrationBars::NONE, 10.0, false, "textures/Texture.GIF",
            "rds=texture/80/none rds-texture=Texture.GIF"}),
    [](const testing::TestParamInfo<RdsParameterInput> &info) { return info.param.name; });

} // namespace
