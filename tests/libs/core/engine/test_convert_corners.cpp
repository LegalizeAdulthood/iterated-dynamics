// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/convert_corners.h>

#include <engine/ImageRegion.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <tuple>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace testing;

namespace id::test
{

class TestBigFloatCornerConversion : public Test
{
protected:
    void TearDown() override;

private:
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL_Z_POWER};
    ValueSaver<ImageRegion> m_saved_region{g_image_region};
};

void TestBigFloatCornerConversion::TearDown()
{
    free_bf_vars();
}

TEST_F(TestBigFloatCornerConversion, simplePathProducesExactCornerValues)
{
    init_bf_dec(50);
    const BigFloat x{"1.25"};
    const BigFloat y{"-2.5"};

    cvt_corners_bf(x, y, 2, 2.0 / 3.0, 0, 0);

    const std::array<const BigFloat *, 6> corners{
        &g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd, &g_bf_y_3rd};
    const std::array<const char *, 6> texts{"0.25", "2.25", "-3", "-2", "0.25", "-3"};
    for (std::size_t i = 0; i < corners.size(); ++i)
    {
        SCOPED_TRACE(i);
        const BigFloat expected{texts[i]};
        EXPECT_EQ(expected, *corners[i]);
    }
}

struct CornerTransform
{
    const char *name;
    double factor;
    double rotation;
    double skew;
};

class TestBigFloatCornerPaths : public TestBigFloatCornerConversion,
                                public WithParamInterface<std::tuple<int, CornerTransform>>
{
};

TEST_P(TestBigFloatCornerPaths, preservesCentersCornerBytesAndTemporaryLifetime)
{
    init_bf_length(std::get<0>(GetParam()));
    const CornerTransform &transform = std::get<1>(GetParam());
    const BigFloat x{"1.25"};
    const BigFloat y{"-2.5"};
    const BigFloat original_x{x};
    const BigFloat original_y{y};
    cvt_corners(1.25, -2.5, 2, transform.factor, transform.rotation, transform.skew);
    const std::array<double, 6> expected{g_image_region.m_min.x, g_image_region.m_max.x, g_image_region.m_min.y,
        g_image_region.m_max.y, g_image_region.m_3rd.x, g_image_region.m_3rd.y};

    cvt_corners_bf(x, y, 2, transform.factor, transform.rotation, transform.skew);

    const auto corner_bytes = pack_bf_parameters();
    const std::array<const BigFloat *, 6> corners{
        &g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd, &g_bf_y_3rd};
    for (std::size_t i = 0; i < corners.size(); ++i)
    {
        SCOPED_TRACE(i);
        EXPECT_NEAR(expected[i], static_cast<double>(corners[i]->to_float()), 1e-12);
    }
    EXPECT_EQ(original_x, x);
    EXPECT_EQ(original_y, y);

    cvt_corners_bf(x, y, 2, transform.factor, transform.rotation, transform.skew);

    EXPECT_EQ(corner_bytes, pack_bf_parameters());
    EXPECT_EQ(original_x, x);
    EXPECT_EQ(original_y, y);
}

INSTANTIATE_TEST_SUITE_P(Paths, TestBigFloatCornerPaths,
    Combine(Values(4, 708),
        Values(CornerTransform{"Simple", 1, 0, 0}, CornerTransform{"ZeroFactor", 0, 0, 0},
            CornerTransform{"Rotated", 2, 30, 0}, CornerTransform{"Skewed", 2, 0, 15},
            CornerTransform{"RotatedSkewed", 2, 30, 15})),
    [](const TestParamInfo<std::tuple<int, CornerTransform>> &info)
    { return std::string{std::get<1>(info.param).name} + (std::get<0>(info.param) == 4 ? "Minimum" : "Maximum"); });

} // namespace id::test
