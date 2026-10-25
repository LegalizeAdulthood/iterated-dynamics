// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>

#include <engine/calcfrac.h>
#include <engine/fractals.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace id::engine;
using namespace id::math;
using namespace id::misc;
using namespace testing;

namespace id::test
{
namespace
{

struct BailoutSample
{
    DComplex point;
    std::array<double, 7> metrics;
};

constexpr std::array<BailoutSample, 4> SAMPLES{{
    {{1.5, -1.0}, {3.25, 2.25, 1.0, 2.25, 1.0, 6.25, 0.25}},
    {{2.0, 0.0}, {4.0, 4.0, 0.0, 4.0, 0.0, 4.0, 4.0}},
    {{0.0, -2.0}, {4.0, 0.0, 4.0, 4.0, 0.0, 4.0, 4.0}},
    {{2.0, -2.0}, {8.0, 4.0, 4.0, 4.0, 4.0, 16.0, 0.0}},
}};

} // namespace

class TestBailoutFormula : public TestWithParam<Bailout>
{
protected:
    ValueSaver<decltype(g_bailout_float)> m_float{g_bailout_float};
    ValueSaver<decltype(g_bailout_bignum)> m_bignum{g_bailout_bignum};
    ValueSaver<decltype(g_bailout_bigfloat)> m_bigfloat{g_bailout_bigfloat};
    ValueSaver<DComplex> m_old{g_old_z, DComplex{-7.0, 9.0}};
    ValueSaver<DComplex> m_new{g_new_z, DComplex{11.0, -13.0}};
    ValueSaver<double> m_magnitude{g_magnitude, -17.0};
    ValueSaver<double> m_square_x{g_temp_sqr_x, -19.0};
    ValueSaver<double> m_square_y{g_temp_sqr_y, -23.0};
    ValueSaver<double> m_limit{g_magnitude_limit};
};

TEST_P(TestBailoutFormula, purePredicatePreservesOrbitState)
{
    for (const BailoutSample &sample : SAMPLES)
    {
        const double metric{sample.metrics[static_cast<std::size_t>(GetParam())]};
        SCOPED_TRACE(metric);
        EXPECT_TRUE(bailout_reached(GetParam(), sample.point.x, sample.point.y, metric));
        EXPECT_TRUE(bailout_reached(GetParam(), sample.point.x, sample.point.y,
            std::nextafter(metric, -std::numeric_limits<double>::infinity())));
        EXPECT_FALSE(bailout_reached(GetParam(), sample.point.x, sample.point.y,
            std::nextafter(metric, std::numeric_limits<double>::infinity())));
    }

    EXPECT_DOUBLE_EQ(-7.0, g_old_z.x);
    EXPECT_DOUBLE_EQ(9.0, g_old_z.y);
    EXPECT_DOUBLE_EQ(11.0, g_new_z.x);
    EXPECT_DOUBLE_EQ(-13.0, g_new_z.y);
    EXPECT_DOUBLE_EQ(-17.0, g_magnitude);
    EXPECT_DOUBLE_EQ(-19.0, g_temp_sqr_x);
    EXPECT_DOUBLE_EQ(-23.0, g_temp_sqr_y);
}

TEST_P(TestBailoutFormula, statefulPredicatePreservesMagnitudeAndAdvanceRules)
{
    set_bailout_formula(GetParam());
    for (const BailoutSample &sample : SAMPLES)
    {
        const double metric{sample.metrics[static_cast<std::size_t>(GetParam())]};
        for (const bool escape : {false, true})
        {
            SCOPED_TRACE(metric);
            SCOPED_TRACE(escape);
            g_magnitude_limit = escape ? metric : std::nextafter(metric, std::numeric_limits<double>::infinity());
            g_old_z = {-7.0, 9.0};
            g_new_z = sample.point;
            g_magnitude = g_temp_sqr_x = g_temp_sqr_y = -1.0;

            EXPECT_EQ(escape ? 1 : 0, g_bailout_float());

            EXPECT_DOUBLE_EQ(escape ? -7.0 : sample.point.x, g_old_z.x);
            EXPECT_DOUBLE_EQ(escape ? 9.0 : sample.point.y, g_old_z.y);
            EXPECT_DOUBLE_EQ(sample.point.x, g_new_z.x);
            EXPECT_DOUBLE_EQ(sample.point.y, g_new_z.y);
            EXPECT_DOUBLE_EQ(sample.metrics[0], g_magnitude);
            EXPECT_DOUBLE_EQ(sample.point.x * sample.point.x, g_temp_sqr_x);
            EXPECT_DOUBLE_EQ(sample.point.y * sample.point.y, g_temp_sqr_y);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(SelectedPredicate, TestBailoutFormula,
    Values(Bailout::MOD, Bailout::REAL, Bailout::IMAG, Bailout::OR, Bailout::AND, Bailout::MANH, Bailout::MANR));

TEST(TestBailoutPredicate, rejectsUnknownTest)
{
    EXPECT_THROW(bailout_reached(static_cast<Bailout>(-1), 1.0, 2.0, 4.0), std::logic_error);
}

} // namespace id::test
