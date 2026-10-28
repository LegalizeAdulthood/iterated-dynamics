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
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

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
    const char *name;
    DComplex point;
    std::array<double, 7> metrics;
};

constexpr std::array<BailoutSample, 4> SAMPLES{{
    {"AsymmetricPoint", {1.5, -1.0}, {3.25, 2.25, 1.0, 2.25, 1.0, 6.25, 0.25}},
    {"RealAxis", {2.0, 0.0}, {4.0, 4.0, 0.0, 4.0, 0.0, 4.0, 4.0}},
    {"ImaginaryAxis", {0.0, -2.0}, {4.0, 0.0, 4.0, 4.0, 0.0, 4.0, 4.0}},
    {"OppositeComponents", {2.0, -2.0}, {8.0, 4.0, 4.0, 4.0, 4.0, 16.0, 0.0}},
}};

struct NamedBailout
{
    const char *name;
    Bailout value;
};

constexpr std::array<NamedBailout, 7> PREDICATES{{
    {"Modulus", Bailout::MOD},
    {"Real", Bailout::REAL},
    {"Imaginary", Bailout::IMAG},
    {"Or", Bailout::OR},
    {"And", Bailout::AND},
    {"Manhattan", Bailout::MANH},
    {"ManhattanReal", Bailout::MANR},
}};

} // namespace

class TestBailoutFormula : public Test
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

struct BailoutComparisonInput
{
    std::string name;
    Bailout predicate;
    BailoutSample sample;
    double limit;
    bool inclusive_expected;
    bool strict_expected;
};

void PrintTo(const BailoutComparisonInput &value, std::ostream *str)
{
    *str << value.name;
}

struct ThresholdInput
{
    const char *name;
    double limit;
    bool inclusive_expected;
    bool strict_expected;
};

static std::vector<BailoutComparisonInput> comparison_inputs()
{
    std::vector<BailoutComparisonInput> inputs;
    for (const NamedBailout &predicate : PREDICATES)
    {
        for (const BailoutSample &sample : SAMPLES)
        {
            const double metric{sample.metrics[static_cast<std::size_t>(predicate.value)]};
            const std::array<ThresholdInput, 3> thresholds{{
                {"Equal", metric, true, false},
                {"Below", std::nextafter(metric, -std::numeric_limits<double>::infinity()), true, true},
                {"Above", std::nextafter(metric, std::numeric_limits<double>::infinity()), false, false},
            }};
            for (const ThresholdInput &threshold : thresholds)
            {
                inputs.push_back({std::string{predicate.name} + "_" + sample.name + "_" + threshold.name,
                    predicate.value, sample, threshold.limit, threshold.inclusive_expected, threshold.strict_expected});
            }
        }
    }
    return inputs;
}

class TestBailoutComparisons : public TestBailoutFormula, public WithParamInterface<BailoutComparisonInput>
{
};

TEST_P(TestBailoutComparisons, purePredicatePreservesOrbitState)
{
    const BailoutComparisonInput &input{GetParam()};

    EXPECT_EQ(input.inclusive_expected,
        bailout_reached(input.predicate, input.sample.point.x, input.sample.point.y, input.limit));

    EXPECT_DOUBLE_EQ(-7.0, g_old_z.x);
    EXPECT_DOUBLE_EQ(9.0, g_old_z.y);
    EXPECT_DOUBLE_EQ(11.0, g_new_z.x);
    EXPECT_DOUBLE_EQ(-13.0, g_new_z.y);
    EXPECT_DOUBLE_EQ(-17.0, g_magnitude);
    EXPECT_DOUBLE_EQ(-19.0, g_temp_sqr_x);
    EXPECT_DOUBLE_EQ(-23.0, g_temp_sqr_y);
}

TEST_P(TestBailoutComparisons, strictPredicateUsesExclusiveLimit)
{
    const BailoutComparisonInput &input{GetParam()};

    EXPECT_EQ(input.strict_expected,
        bailout_reached(input.predicate, input.sample.point.x, input.sample.point.y, input.limit, true));
}

INSTANTIATE_TEST_SUITE_P(ComparisonInputs, TestBailoutComparisons, ValuesIn(comparison_inputs()),
    [](const TestParamInfo<BailoutComparisonInput> &info) { return info.param.name; });

struct BailoutStateInput
{
    std::string name;
    Bailout predicate;
    BailoutSample sample;
    double limit;
    bool escape;
};

void PrintTo(const BailoutStateInput &value, std::ostream *str)
{
    *str << value.name;
}

static std::vector<BailoutStateInput> stateful_inputs()
{
    std::vector<BailoutStateInput> inputs;
    for (const NamedBailout &predicate : PREDICATES)
    {
        for (const BailoutSample &sample : SAMPLES)
        {
            const double metric{sample.metrics[static_cast<std::size_t>(predicate.value)]};
            for (const bool escape : {false, true})
            {
                inputs.push_back({std::string{predicate.name} + "_" + sample.name + (escape ? "_Escapes" : "_Advances"),
                    predicate.value, sample,
                    escape ? metric : std::nextafter(metric, std::numeric_limits<double>::infinity()), escape});
            }
        }
    }
    return inputs;
}

class TestBailoutStatefulPredicate : public TestBailoutFormula, public WithParamInterface<BailoutStateInput>
{
protected:
    void SetUp() override;
};

void TestBailoutStatefulPredicate::SetUp()
{
    TestBailoutFormula::SetUp();
    const BailoutStateInput &input{GetParam()};
    set_bailout_formula(input.predicate);
    g_magnitude_limit = input.limit;
    g_old_z = {-7.0, 9.0};
    g_new_z = input.sample.point;
    g_magnitude = g_temp_sqr_x = g_temp_sqr_y = -1.0;
}

TEST_P(TestBailoutStatefulPredicate, preservesMagnitudeAndAdvanceRules)
{
    const BailoutStateInput &input{GetParam()};
    const DComplex &point{input.sample.point};

    EXPECT_EQ(input.escape ? 1 : 0, g_bailout_float());

    EXPECT_DOUBLE_EQ(input.escape ? -7.0 : point.x, g_old_z.x);
    EXPECT_DOUBLE_EQ(input.escape ? 9.0 : point.y, g_old_z.y);
    EXPECT_DOUBLE_EQ(point.x, g_new_z.x);
    EXPECT_DOUBLE_EQ(point.y, g_new_z.y);
    EXPECT_DOUBLE_EQ(input.sample.metrics[0], g_magnitude);
    EXPECT_DOUBLE_EQ(point.x * point.x, g_temp_sqr_x);
    EXPECT_DOUBLE_EQ(point.y * point.y, g_temp_sqr_y);
}

INSTANTIATE_TEST_SUITE_P(StatefulInputs, TestBailoutStatefulPredicate, ValuesIn(stateful_inputs()),
    [](const TestParamInfo<BailoutStateInput> &info) { return info.param.name; });

TEST(TestBailoutPredicate, rejectsUnknownTest)
{
    EXPECT_THROW(bailout_reached(static_cast<Bailout>(-1), 1.0, 2.0, 4.0), std::logic_error);
}

} // namespace id::test
