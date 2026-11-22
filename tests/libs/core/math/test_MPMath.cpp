// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/MPMath.h>

#include <math/BigArena.h>

#include <gtest/gtest.h>

#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace id::math
{

static_assert(std::is_same_v<decltype(std::declval<MPReal>() + std::declval<MPReal>()), MPReal>);
static_assert(std::is_same_v<decltype(std::declval<MPComplex>() * std::declval<MPComplex>()), MPComplex>);

struct PrecisionInput
{
    const char *name;
    int required;
    int user;
    unsigned expected;
};

void PrintTo(const PrecisionInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestMPPrecision : public testing::TestWithParam<PrecisionInput>
{
};

TEST_P(TestMPPrecision, selectsDecimalDigitsAndAddsGuards)
{
    const PrecisionInput &input = GetParam();
    const MPPrecision precision{input.required, input.user};

    EXPECT_EQ(input.expected, precision.requested_digits());
    EXPECT_EQ(input.expected + MPPrecision::GUARD_DIGITS, precision.working_digits());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestMPPrecision,
    testing::Values(PrecisionInput{"ViewportEstimate", 40, 0, 40}, PrecisionInput{"Minimum", 1, 0, 3},
        PrecisionInput{"HigherOverride", 40, 200, 200}, PrecisionInput{"LowerOverride", 200, 40, 40},
        PrecisionInput{"SmallOverride", 40, 1, 1}, PrecisionInput{"OverrideWithoutEstimate", -1, 40, 40},
        PrecisionInput{"Maximum", 2000, 0, 2000}, PrecisionInput{"MaximumOverride", 40, 2000, 2000}),
    [](const testing::TestParamInfo<PrecisionInput> &info) { return info.param.name; });

TEST(TestMPMath, rejectsInvalidPrecisionRequests)
{
    EXPECT_THROW((MPPrecision{0}), PrecisionError);
    EXPECT_THROW((MPPrecision{-1}), PrecisionError);
    EXPECT_THROW((MPPrecision{2001}), PrecisionError);
    EXPECT_THROW((MPPrecision{40, -1}), PrecisionError);
    EXPECT_THROW((MPPrecision{40, 2001}), PrecisionError);
}

TEST(TestMPMath, constructsRealAndBothComplexComponentsAtWorkingPrecision)
{
    const MPPrecision precision{200};
    const auto value = precision.real("-1.25");
    const auto zero = precision.real();
    const auto orbit = precision.complex("-1.25", "0.5");
    const auto complex_zero = precision.complex();

    EXPECT_EQ(precision.working_digits(), value.precision());
    EXPECT_EQ(precision.working_digits(), orbit.precision());
    EXPECT_EQ(precision.working_digits(), orbit.real().precision());
    EXPECT_EQ(precision.working_digits(), orbit.imag().precision());
    EXPECT_EQ(value, orbit.real());
    EXPECT_EQ(precision.real("0.5"), orbit.imag());
    EXPECT_EQ(0, zero);
    EXPECT_EQ(0, complex_zero);
}

TEST(TestMPMath, parsesOnlyTheProvidedStringView)
{
    const MPPrecision precision{40};
    const std::string text{"1.25invalid"};
    const std::string_view part{text.data(), 4};

    EXPECT_EQ(precision.real("1.25"), precision.real(part));
    EXPECT_EQ(precision.real("1.25"), precision.complex(part, part).imag());
}

TEST(TestMPMath, keepsIndependentPrecisionsAndSubDoubleDifferences)
{
    const auto real_default = MPReal::default_precision();
    const auto complex_default = MPComplex::default_precision();
    const auto real_thread_default = MPReal::thread_default_precision();
    const auto complex_thread_default = MPComplex::thread_default_precision();
    const MPPrecision low{40};
    const MPPrecision high{200};
    const std::string coordinate = "1." + std::string(79, '0') + '1';
    const auto retained = high.complex(coordinate, coordinate);
    const auto rounded = low.complex(coordinate, coordinate);

    EXPECT_NE(high.real("1"), retained.real());
    EXPECT_NE(high.real("1"), retained.imag());
    EXPECT_EQ(low.real("1"), rounded.real());
    EXPECT_EQ(low.working_digits(), rounded.precision());
    EXPECT_EQ(high.working_digits(), retained.precision());
    EXPECT_EQ(high.working_digits(), high.real().precision());
    EXPECT_EQ(real_default, MPReal::default_precision());
    EXPECT_EQ(complex_default, MPComplex::default_precision());
    EXPECT_EQ(real_thread_default, MPReal::thread_default_precision());
    EXPECT_EQ(complex_thread_default, MPComplex::thread_default_precision());
}

TEST(TestMPMath, preservesDefaultsWhenConstructionFails)
{
    const auto real_default = MPReal::default_precision();
    const auto complex_default = MPComplex::default_precision();
    const auto real_thread_default = MPReal::thread_default_precision();
    const auto complex_thread_default = MPComplex::thread_default_precision();
    const MPPrecision precision{40};

    EXPECT_THROW(precision.complex("1.25", "invalid"), std::runtime_error);

    EXPECT_EQ(real_default, MPReal::default_precision());
    EXPECT_EQ(complex_default, MPComplex::default_precision());
    EXPECT_EQ(real_thread_default, MPReal::thread_default_precision());
    EXPECT_EQ(complex_thread_default, MPComplex::thread_default_precision());
}

TEST(TestMPMath, constructsValuesAtTheMaximumPolicyPrecision)
{
    const MPPrecision precision{40, 2000};
    const auto value = precision.real();
    const auto orbit = precision.complex();

    EXPECT_EQ(precision.working_digits(), value.precision());
    EXPECT_EQ(precision.working_digits(), orbit.real().precision());
    EXPECT_EQ(precision.working_digits(), orbit.imag().precision());
}

} // namespace id::math
