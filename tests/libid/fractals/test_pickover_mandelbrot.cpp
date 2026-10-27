// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/pickover_mandelbrot.h>

#include <engine/fractals.h>
#include <engine/fractalb.h>
#include <fractals/frasetup.h>
#include <fractals/fractype.h>
#include <math/biginit.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <complex>
#include <limits>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace testing;

namespace id::test
{

class TestMandelZPowerPerturbation : public TestWithParam<int>
{
protected:
    void TearDown() override
    {
        if (m_big_float)
        {
            free_bf_vars();
        }
    }

    bool m_big_float{};
    ValueSaver<BFMathType> m_saved_math{g_bf_math};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL_Z_POWER};
};

TEST_P(TestMandelZPowerPerturbation, setupPreservesTheRequestedExponent)
{
    ValueSaver saved_exponent{g_c_exponent, GetParam()};

    EXPECT_FALSE(mandel_z_power_perturbation_per_image());
    EXPECT_EQ(GetParam(), g_c_exponent);
}

TEST_P(TestMandelZPowerPerturbation, matchesDirectPowerUpdate)
{
    ValueSaver saved_exponent{g_c_exponent, GetParam()};
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
    const std::complex<double> delta{0.875, -0.0625};
    const std::complex<double> parameter_delta{0.003, -0.002};
    const std::complex<double> expected{std::pow(delta, GetParam()) + parameter_delta};
    std::complex<double> actual{delta};

    mandel_z_power_perturb({}, actual, parameter_delta);

    EXPECT_NEAR(expected.real(), actual.real(), 1.0e-13);
    EXPECT_NEAR(expected.imag(), actual.imag(), 1.0e-13);
}

TEST_P(TestMandelZPowerPerturbation, matchesPowerUpdateNearTheUnitCircle)
{
    ValueSaver saved_exponent{g_c_exponent, GetParam()};
    const std::complex<double> reference{0.999, 0.000125};
    const std::complex<double> delta{0.000125, -0.0000625};
    const std::complex<double> parameter_delta{0.003, -0.002};
    const std::complex<double> expected{
        std::pow(reference + delta, GetParam()) - std::pow(reference, GetParam()) + parameter_delta};
    std::complex<double> actual{delta};

    mandel_z_power_perturb(reference, actual, parameter_delta);

    EXPECT_NEAR(expected.real(), actual.real(), 1.0e-13);
    EXPECT_NEAR(expected.imag(), actual.imag(), 1.0e-13);
}

TEST_P(TestMandelZPowerPerturbation, retainsDisplacementsTooSmallToAddToTheReference)
{
    ValueSaver saved_exponent{g_c_exponent, GetParam()};
    const std::complex<double> delta{1e-40, -2e-40};
    std::complex<double> actual{delta};

    mandel_z_power_perturb({1.0, 0.0}, actual, {});

    EXPECT_NEAR(GetParam() * delta.real(), actual.real(), GetParam() * 1e-54);
    EXPECT_NEAR(GetParam() * delta.imag(), actual.imag(), GetParam() * 1e-54);
}

TEST_P(TestMandelZPowerPerturbation, referenceSequenceMatchesRepeatedMultiplication)
{
    ValueSaver saved_exponent{g_c_exponent, GetParam()};
    const std::complex<double> center{-0.125, 0.000125};
    std::complex<double> actual{0.999, -0.000125};
    std::complex<double> expected{actual};

    for (int step = 0; step < 8; ++step)
    {
        SCOPED_TRACE(step);
        std::complex<double> powered{1.0, 0.0};
        for (int factor = 0; factor < GetParam(); ++factor)
        {
            powered *= expected;
        }
        expected = powered + center;
        mandel_z_power_ref_pt(center, actual);
        ASSERT_NEAR(expected.real(), actual.real(), 1e-12 * std::abs(expected));
        ASSERT_NEAR(expected.imag(), actual.imag(), 1e-12 * std::abs(expected));
        if (std::norm(expected) >= 4.0)
        {
            break;
        }
    }
}

TEST_P(TestMandelZPowerPerturbation, preciseReferenceSequenceMatchesRepeatedMultiplication)
{
    ValueSaver saved_exponent{g_c_exponent, GetParam()};
    init_bf_dec(60);
    m_big_float = true;
    BigStackSaver saved;
    const BFComplex center{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex actual{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex expected{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex powered{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex product{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const BigFloat difference{alloc_stack(g_r_bf_length + 2)};
    str_to_bf(center.x, "-0.12500000000000000000000000000000000000001");
    str_to_bf(center.y, "0.000125");
    str_to_bf(actual.x, "0.99900000000000000000000000000000000000001");
    str_to_bf(actual.y, "-0.000125");
    copy_bf(expected.x, actual.x);
    copy_bf(expected.y, actual.y);

    for (int step = 0; step < 8; ++step)
    {
        SCOPED_TRACE(step);
        int_to_bf(powered.x, 1);
        clear_bf(powered.y);
        for (int factor = 0; factor < GetParam(); ++factor)
        {
            cmplx_mul_bf(&product, &powered, &expected);
            copy_bf(powered.x, product.x);
            copy_bf(powered.y, product.y);
        }
        add_bf(expected.x, powered.x, center.x);
        add_bf(expected.y, powered.y, center.y);
        mandel_z_power_ref_pt_bf(center, actual);
        sub_bf(difference, actual.x, expected.x);
        EXPECT_NEAR(0.0, static_cast<double>(bf_to_float(difference)), 1e-50);
        sub_bf(difference, actual.y, expected.y);
        EXPECT_NEAR(0.0, static_cast<double>(bf_to_float(difference)), 1e-50);
        if (std::norm(std::complex<double>{
                static_cast<double>(bf_to_float(expected.x)), static_cast<double>(bf_to_float(expected.y))}) >= 4.0)
        {
            break;
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    SupportedPowers, TestMandelZPowerPerturbation, Values(2, 3, 26, 27, 28, 29, 32, 64, 128, 1024, 2048));

TEST(TestMandelZPowerPerturbationLimits, maximumIntegerExponentNeedsNoCoefficientStorage)
{
    ValueSaver saved_exponent{g_c_exponent, std::numeric_limits<int>::max()};
    EXPECT_FALSE(mandel_z_power_perturbation_per_image());
    EXPECT_EQ(std::numeric_limits<int>::max(), g_c_exponent);
    std::complex<double> delta{1e-40, -2e-40};

    mandel_z_power_perturb({1.0, 0.0}, delta, {});

    EXPECT_NEAR(g_c_exponent * 1e-40, delta.real(), g_c_exponent * 1e-54);
    EXPECT_NEAR(g_c_exponent * -2e-40, delta.imag(), g_c_exponent * 1e-54);
    std::complex<double> reference{1.0, 0.0};
    mandel_z_power_ref_pt({}, reference);
    EXPECT_EQ((std::complex<double>{1.0, 0.0}), reference);
}

} // namespace id::test
