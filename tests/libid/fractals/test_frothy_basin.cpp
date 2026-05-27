// SPDX-License-Identifier: GPL-3.0-only
//
#define private public
#include <fractals/FrothyBasin.h>
#undef private

#include <engine/calcfrac.h>
#include <engine/color_state.h>
#include <engine/VideoInfo.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::test
{

class ParamsSaver
{
public:
    ParamsSaver();
    ~ParamsSaver();

private:
    std::array<double, MAX_PARAMS> m_params{};
};

ParamsSaver::ParamsSaver()
{
    std::copy(&g_params[0], &g_params[MAX_PARAMS], m_params.begin());
    std::fill(&g_params[0], &g_params[MAX_PARAMS], 0.0);
}

ParamsSaver::~ParamsSaver()
{
    std::copy(m_params.begin(), m_params.end(), &g_params[0]);
}

struct TestFrothyBasin : testing::Test
{
    ~TestFrothyBasin() override = default;

    ParamsSaver saved_params{};
    ValueSaver<Version> saved_version{g_version, parse_legacy_version(2000)};
    ValueSaver<double> saved_magnitude_limit{g_magnitude_limit, 1.0};
    ValueSaver<int> saved_colors{g_colors, 256};
    ValueSaver<int> saved_orbit_color{g_orbit_color, 15};
    ValueSaver<ColorState> saved_color_state{g_color_state, ColorState::UNKNOWN_MAP};
};

TEST_F(TestFrothyBasin, legacyVersionUsesOldSixAttractorParameters)
{
    g_version = parse_legacy_version(1730);
    g_params[0] = 6.0;
    g_params[1] = 1.0;
    g_params[2] = 999.0;

    EXPECT_TRUE(g_frothy_basin.per_image());

    EXPECT_EQ(6.0, g_params[0]);
    EXPECT_EQ(1.0, g_params[1]);
    EXPECT_EQ(0.0, g_params[2]);
    EXPECT_TRUE(g_frothy_basin.m_repeat_mapping);
    EXPECT_EQ(1, g_frothy_basin.m_alt_color);
    EXPECT_EQ(6, g_frothy_basin.m_attractors);
    EXPECT_EQ(42, g_frothy_basin.m_shades);
    EXPECT_NEAR(1.02871376822, g_frothy_basin.m_a, 1e-12);
    EXPECT_NEAR(0.51435688411, g_frothy_basin.m_half_a, 1e-12);
    EXPECT_NEAR(-1.04368901270, g_frothy_basin.m_top_x1, 1e-12);
    EXPECT_NEAR(1.33928675524, g_frothy_basin.m_top_x2, 1e-12);
    EXPECT_NEAR(-0.339286755220, g_frothy_basin.m_top_x3, 1e-12);
    EXPECT_NEAR(-0.339286755220, g_frothy_basin.m_top_x4, 1e-12);
    EXPECT_NEAR(0.07639837810, g_frothy_basin.m_left_x1, 1e-12);
    EXPECT_NEAR(-1.11508950586, g_frothy_basin.m_left_x2, 1e-12);
    EXPECT_NEAR(-0.27580275066, g_frothy_basin.m_left_x3, 1e-12);
    EXPECT_NEAR(-0.27580275066, g_frothy_basin.m_left_x4, 1e-12);
    EXPECT_NEAR(0.96729063460, g_frothy_basin.m_right_x1, 1e-12);
    EXPECT_NEAR(-0.22419724936, g_frothy_basin.m_right_x2, 1e-12);
    EXPECT_NEAR(0.61508950585, g_frothy_basin.m_right_x3, 1e-12);
    EXPECT_NEAR(0.61508950585, g_frothy_basin.m_right_x4, 1e-12);
    EXPECT_EQ(7.0, g_magnitude_limit);
    EXPECT_EQ(255, g_orbit_color);
}

TEST_F(TestFrothyBasin, post1821VersionUsesCurrentParameters)
{
    g_version = parse_legacy_version(1822);
    g_params[0] = 6.0;
    g_params[1] = 7.0;
    g_params[2] = 0.5;

    EXPECT_TRUE(g_frothy_basin.per_image());

    EXPECT_EQ(1.0, g_params[0]);
    EXPECT_EQ(1.0, g_params[1]);
    EXPECT_EQ(0.5, g_params[2]);
    EXPECT_FALSE(g_frothy_basin.m_repeat_mapping);
    EXPECT_EQ(1, g_frothy_basin.m_alt_color);
    EXPECT_EQ(3, g_frothy_basin.m_attractors);
    EXPECT_EQ(85, g_frothy_basin.m_shades);
    EXPECT_EQ(0.5, g_frothy_basin.m_a);
    EXPECT_EQ(0.25, g_frothy_basin.m_half_a);
    EXPECT_EQ(7.0, g_magnitude_limit);
    EXPECT_EQ(171, g_orbit_color);
}

} // namespace id::test
