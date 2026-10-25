// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/pickover_mandelbrot.h>

#include <engine/fractals.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <complex>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;
using namespace testing;

namespace id::test
{

class TestMandelZPowerPerturbation : public TestWithParam<int>
{
};

TEST_P(TestMandelZPowerPerturbation, generatesCompleteCoefficientRow)
{
    ValueSaver saved_exponent{g_c_exponent, GetParam()};

    // Generation bounds-checks every write, including the final coefficient.
    EXPECT_NO_THROW(pascal_triangle());
}

TEST_P(TestMandelZPowerPerturbation, matchesDirectPowerUpdate)
{
    ValueSaver saved_exponent{g_c_exponent, GetParam()};
    pascal_triangle();
    const std::complex<double> reference{0.75, 0.125};
    const std::complex<double> delta{0.125, -0.0625};
    const std::complex<double> parameter_delta{0.003, -0.002};
    const std::complex<double> expected{
        std::pow(reference + delta, GetParam()) - std::pow(reference, GetParam()) + parameter_delta};
    std::complex<double> actual{delta};

    mandel_z_power_perturb(reference, actual, parameter_delta);

    EXPECT_NEAR(expected.real(), actual.real(), 1.0e-13);
    EXPECT_NEAR(expected.imag(), actual.imag(), 1.0e-13);
}

TEST_P(TestMandelZPowerPerturbation, matchesPowerUpdateWithZeroReference)
{
    ValueSaver saved_exponent{g_c_exponent, GetParam()};
    pascal_triangle();
    const std::complex<double> delta{0.875, -0.0625};
    const std::complex<double> parameter_delta{0.003, -0.002};
    const std::complex<double> expected{std::pow(delta, GetParam()) + parameter_delta};
    std::complex<double> actual{delta};

    mandel_z_power_perturb({}, actual, parameter_delta);

    EXPECT_NEAR(expected.real(), actual.real(), 1.0e-13);
    EXPECT_NEAR(expected.imag(), actual.imag(), 1.0e-13);
}

INSTANTIATE_TEST_SUITE_P(SupportedPowers, TestMandelZPowerPerturbation, Values(2, 3, 26, 27, 28));

} // namespace id::test
