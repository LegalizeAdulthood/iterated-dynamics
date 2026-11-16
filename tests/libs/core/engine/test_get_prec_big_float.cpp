// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/get_prec_big_float.h>

#include <engine/convert_center_mag.h>
#include <engine/calc_frac_init.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/VideoInfo.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <limits>
#include <ostream>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{

struct MagnificationPrecisionInput
{
    const char *name;
    double magnification;
    int expected;
};

void PrintTo(const MagnificationPrecisionInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestGetMagnificationPrecision : public testing::TestWithParam<MagnificationPrecisionInput>
{
};

TEST_P(TestGetMagnificationPrecision, computesRequiredPrecision)
{
    EXPECT_EQ(GetParam().expected, get_magnification_precision(GetParam().magnification));
}

INSTANTIATE_TEST_SUITE_P(Magnifications, TestGetMagnificationPrecision,
    testing::Values(MagnificationPrecisionInput{"One", 1.0, 4}, MagnificationPrecisionInput{"Ten", 10.0, 5},
        MagnificationPrecisionInput{"Hundred", 100.0, 6}, MagnificationPrecisionInput{"Tenth", 0.1, 3},
        MagnificationPrecisionInput{"Hundredth", 0.01, 2}, MagnificationPrecisionInput{"Two", 2.0, 4},
        MagnificationPrecisionInput{"Four", 4.0, 4}, MagnificationPrecisionInput{"Eight", 8.0, 4},
        MagnificationPrecisionInput{"Sixteen", 16.0, 5}, MagnificationPrecisionInput{"ThirtyTwo", 32.0, 5},
        MagnificationPrecisionInput{"Zero", 0.0, 4},
        MagnificationPrecisionInput{"MinimumNormal", std::numeric_limits<double>::min(), -304},
        MagnificationPrecisionInput{"Maximum", std::numeric_limits<double>::max(), 312},
        MagnificationPrecisionInput{"Epsilon", std::numeric_limits<double>::epsilon(), -12}),
    [](const testing::TestParamInfo<MagnificationPrecisionInput> &info) { return info.param.name; });

class TestBigFloatViewportPrecision : public testing::Test
{
protected:
    void SetUp() override
    {
        init_bf_dec(80);
    }

    void TearDown() override
    {
        free_bf_vars();
    }

    void set_corners(const ImageRegion &region)
    {
        float_to_bf(g_bf_x_min, region.m_min.x);
        float_to_bf(g_bf_x_max, region.m_max.x);
        float_to_bf(g_bf_y_min, region.m_min.y);
        float_to_bf(g_bf_y_max, region.m_max.y);
        float_to_bf(g_bf_x_3rd, region.m_3rd.x);
        float_to_bf(g_bf_y_3rd, region.m_3rd.y);
    }

    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<int> m_saved_digits{g_bf_digits, 0};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<ImageRegion> m_saved_region{g_image_region};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{101, 51, 0, 0, 100.0, 50.0}};
};

TEST_F(TestBigFloatViewportPrecision, retainsResolutionAndMagnificationDigitSelection)
{
    set_corners({{-2.0, -1.5}, {2.0, 1.5}, {-2.0, -1.5}});
    const auto stack{save_stack()};

    EXPECT_EQ(3, get_prec_bf_mag());
    EXPECT_EQ(3, get_prec_bf(ResolutionFlag::CURRENT));
    EXPECT_EQ(4, get_prec_bf(ResolutionFlag::MAX));
    EXPECT_EQ(stack, save_stack());

    str_to_bf(g_bf_x_min, "-2e-40");
    str_to_bf(g_bf_x_max, "2e-40");
    str_to_bf(g_bf_x_3rd, "-2e-40");
    str_to_bf(g_bf_y_min, "-1.5e-40");
    str_to_bf(g_bf_y_max, "1.5e-40");
    str_to_bf(g_bf_y_3rd, "-1.5e-40");

    EXPECT_EQ(43, get_prec_bf_mag());
    EXPECT_EQ(43, get_prec_bf(ResolutionFlag::CURRENT));
    EXPECT_EQ(44, get_prec_bf(ResolutionFlag::MAX));
    EXPECT_EQ(stack, save_stack());
}

TEST_F(TestBigFloatViewportPrecision, collapsedCoordinatesRetainTheFailureSentinel)
{
    set_corners({{0.25, 0.125}, {0.25, 0.125}, {0.25, 0.125}});

    EXPECT_EQ(-1, get_prec_bf_mag());
    EXPECT_EQ(-1, get_prec_bf(ResolutionFlag::CURRENT));
    EXPECT_EQ(-1, get_prec_bf(ResolutionFlag::MAX));
}

TEST_F(TestBigFloatViewportPrecision, centerConversionPreservesInputsAndTheOutputArena)
{
    const std::array<ImageRegion, 3> regions{{{{-2.0, -1.5}, {2.0, 1.5}, {-2.0, -1.5}},
        {{-2.0, -1.5}, {2.0, 1.5}, {-1.75, -1.25}}, {{-2.0, -1.5}, {2.0, 1.5}, {1.75, 1.25}}}};
    for (const ImageRegion &region : regions)
    {
        g_image_region = region;
        set_corners(region);
        const auto original{pack_bf_parameters()};
        const auto stack{save_stack()};
        double expected_x;
        double expected_y;
        LDouble expected_mag;
        double expected_factor;
        double expected_rot;
        double expected_skew;
        cvt_center_mag(expected_x, expected_y, expected_mag, expected_factor, expected_rot, expected_skew);
        BigFloat x{bf_arena(BigLifetime::PERSISTENT)};
        BigFloat y{bf_arena(BigLifetime::PERSISTENT)};
        LDouble mag;
        double factor;
        double rot;
        double skew;

        cvt_center_mag_bf(x, y, mag, factor, rot, skew);

        EXPECT_NEAR(expected_x, static_cast<double>(x.to_float()), 1e-12);
        EXPECT_NEAR(expected_y, static_cast<double>(y.to_float()), 1e-12);
        EXPECT_NEAR(expected_mag, mag, 1e-12);
        EXPECT_NEAR(expected_factor, factor, 1e-12);
        EXPECT_NEAR(expected_rot, rot, 1e-12);
        EXPECT_NEAR(expected_skew, skew, 1e-12);
        EXPECT_EQ(original, pack_bf_parameters());
        EXPECT_EQ(&bf_arena(BigLifetime::PERSISTENT), &x.arena());
        EXPECT_EQ(&bf_arena(BigLifetime::PERSISTENT), &y.arena());
        EXPECT_EQ(stack, save_stack());
    }
}

TEST_F(TestBigFloatViewportPrecision, subtractsDeepCornersBeforeMachineConversion)
{
    set_corners({{0.25, 0.125}, {0.25, 0.125}, {0.25, 0.125}});
    BigFloat offset{"1e-40"};
    add_a_bf(g_bf_x_max.data(), offset.data());
    sub_a_bf(g_bf_x_min.data(), offset.data());
    sub_a_bf(g_bf_x_3rd.data(), offset.data());
    add_a_bf(g_bf_y_max.data(), offset.data());
    sub_a_bf(g_bf_y_min.data(), offset.data());
    sub_a_bf(g_bf_y_3rd.data(), offset.data());
    BigFloat x{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat y{bf_arena(BigLifetime::TEMPORARY)};
    LDouble mag;
    double factor;
    double rot;
    double skew;

    cvt_center_mag_bf(x, y, mag, factor, rot, skew);

    EXPECT_NEAR(0.25, static_cast<double>(x.to_float()), 1e-14);
    EXPECT_NEAR(0.125, static_cast<double>(y.to_float()), 1e-14);
    EXPECT_NEAR(1e40, mag, 1e28);
    EXPECT_NEAR(1.0 / DEFAULT_ASPECT, factor, 1e-12);
    EXPECT_NEAR(0.0, rot, 1e-12);
    EXPECT_NEAR(0.0, skew, 1e-12);
}

} // namespace id::test
