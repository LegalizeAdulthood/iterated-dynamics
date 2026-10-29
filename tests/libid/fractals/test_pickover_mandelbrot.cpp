// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/pickover_mandelbrot.h>

#include <engine/fractals.h>
#include <engine/fractalb.h>
#include <engine/LogicalScreen.h>
#include <fractals/fractalp.h>
#include <fractals/frasetup.h>
#include <fractals/fractype.h>
#include <math/biginit.h>
#include <misc/ValueSaver.h>
#include <misc/debug_flags.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <complex>
#include <limits>
#include <optional>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace testing;

namespace id::test
{

class TestMandelPowerState : public Test
{
protected:
    void TearDown() override;
    void initialize_precision();

    bool m_big_float{};
    ValueSaver<int> m_saved_exponent{g_c_exponent};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL_Z_POWER};
};

void TestMandelPowerState::TearDown()
{
    if (m_big_float)
    {
        free_bf_vars();
    }
}

void TestMandelPowerState::initialize_precision()
{
    init_bf_dec(60);
    m_big_float = true;
}

class TestMandelZPowerPerturbation : public TestMandelPowerState, public WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestMandelZPowerPerturbation::SetUp()
{
    g_c_exponent = GetParam();
}

TEST_P(TestMandelZPowerPerturbation, setupPreservesTheRequestedExponent)
{
    ValueSaver saved_power{g_params[2], static_cast<double>(GetParam())};
    ValueSaver saved_mode{g_std_calc_mode, CalcMode::PERTURBATION};
    ValueSaver saved_specific{g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL_Z_POWER)};
    ValueSaver saved_dispatch{g_dispatch};
    ValueSaver saved_symmetry{g_symmetry};
    ValueSaver saved_parameter{g_float_param};
    ValueSaver saved_derivative_power{g_power_z};

    EXPECT_FALSE(mandel_per_image());
    EXPECT_EQ(GetParam(), g_c_exponent);
}

struct PositivePowerDeltaInput
{
    int exponent;
    const char *name;
    std::complex<double> reference;
    std::complex<double> delta;
};

std::vector<PositivePowerDeltaInput> positive_power_delta_inputs()
{
    std::vector<PositivePowerDeltaInput> inputs;
    for (const int exponent : {2, 3, 26, 27, 28, 29, 32, 64, 128, 1024, 2048})
    {
        inputs.push_back({exponent, "Direct", {0.75, 0.125}, {0.125, -0.0625}});
        inputs.push_back({exponent, "ZeroReference", {}, {0.875, -0.0625}});
        inputs.push_back({exponent, "UnitCircle", {0.999, 0.000125}, {0.000125, -0.0000625}});
    }
    return inputs;
}

class TestPositivePowerDelta : public TestMandelPowerState, public WithParamInterface<PositivePowerDeltaInput>
{
protected:
    void SetUp() override;
};

void TestPositivePowerDelta::SetUp()
{
    g_c_exponent = GetParam().exponent;
}

TEST_P(TestPositivePowerDelta, matchesDirectPowerUpdate)
{
    const PositivePowerDeltaInput &input{GetParam()};
    const std::complex<double> parameter_delta{0.003, -0.002};
    const std::complex<double> expected{std::pow(input.reference + input.delta, input.exponent) -
        std::pow(input.reference, input.exponent) + parameter_delta};
    std::complex<double> actual{input.delta};

    mandel_z_power_perturb(input.reference, actual, parameter_delta);

    EXPECT_NEAR(expected.real(), actual.real(), 1.0e-13);
    EXPECT_NEAR(expected.imag(), actual.imag(), 1.0e-13);
}

INSTANTIATE_TEST_SUITE_P(SupportedPowers, TestPositivePowerDelta, ValuesIn(positive_power_delta_inputs()),
    [](const TestParamInfo<PositivePowerDeltaInput> &info)
    { return std::string{info.param.name} + "Power" + std::to_string(info.param.exponent); });

TEST_P(TestMandelZPowerPerturbation, retainsDisplacementsTooSmallToAddToTheReference)
{
    const std::complex<double> delta{1e-40, -2e-40};
    std::complex<double> actual{delta};

    mandel_z_power_perturb({1.0, 0.0}, actual, {});

    EXPECT_NEAR(GetParam() * delta.real(), actual.real(), GetParam() * 1e-54);
    EXPECT_NEAR(GetParam() * delta.imag(), actual.imag(), GetParam() * 1e-54);
}

TEST_P(TestMandelZPowerPerturbation, referenceSequenceMatchesRepeatedMultiplication)
{
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
    initialize_precision();
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

class TestMaximumMandelPower : public TestMandelPowerState
{
protected:
    void SetUp() override;
};

void TestMaximumMandelPower::SetUp()
{
    g_c_exponent = std::numeric_limits<int>::max();
    EXPECT_EQ(std::numeric_limits<int>::max(), g_c_exponent);
}

TEST_F(TestMaximumMandelPower, retainsSubDoubleDisplacementsWithoutCoefficientStorage)
{
    std::complex<double> delta{1e-40, -2e-40};

    mandel_z_power_perturb({1.0, 0.0}, delta, {});

    EXPECT_NEAR(g_c_exponent * 1e-40, delta.real(), g_c_exponent * 1e-54);
    EXPECT_NEAR(g_c_exponent * -2e-40, delta.imag(), g_c_exponent * 1e-54);
}

TEST_F(TestMaximumMandelPower, updatesUnitReferenceWithoutCoefficientStorage)
{
    std::complex<double> reference{1.0, 0.0};

    mandel_z_power_ref_pt({}, reference);

    EXPECT_EQ((std::complex<double>{1.0, 0.0}), reference);
}

class TestSignedPowerState : public TestMandelPowerState
{
protected:
    std::complex<double> ordinary_power(const std::complex<double> &z);
    void initialize_signed_precision();
    std::complex<double> precise_power(const std::complex<double> &z);
};

std::complex<double> TestSignedPowerState::ordinary_power(const std::complex<double> &z)
{
    DComplex value{z.real(), z.imag()};
    DComplex result;
    id::engine::pow(&value, g_c_exponent, &result);
    return {result.x, result.y};
}

void TestSignedPowerState::initialize_signed_precision()
{
    initialize_precision();
    init_big_pi();
}

std::complex<double> TestSignedPowerState::precise_power(const std::complex<double> &z)
{
    BigStackSaver saved;
    BFComplex value{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex exponent{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex result{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    float_to_bf(value.x, z.real());
    float_to_bf(value.y, z.imag());
    int_to_bf(exponent.x, g_c_exponent);
    clear_bf(exponent.y);
    cmplx_pow_bf(&result, &value, &exponent);
    return {static_cast<double>(bf_to_float(result.x)), static_cast<double>(bf_to_float(result.y))};
}

std::string signed_power_name(const int exponent)
{
    return exponent < 0 ? "NegativePower" + std::to_string(-exponent) : "Power" + std::to_string(exponent);
}

struct SignedPowerReferenceInput
{
    std::string name;
    int exponent;
    std::complex<double> seed;
};

std::vector<SignedPowerReferenceInput> signed_reference_inputs()
{
    std::vector<SignedPowerReferenceInput> inputs;
    for (const int exponent : {0, 1, -1, -2, -3, -29})
    {
        for (const SignedPowerReferenceInput &input : {SignedPowerReferenceInput{"Zero", exponent, {}},
                 {"Positive", exponent, {0.75, 0.125}}, {"Negative", exponent, {-0.75, 0.125}},
                 {"Tiny", exponent, {1e-160, 0.0}}, {"Large", exponent, {1e10, 0.0}}})
        {
            inputs.push_back({input.name + signed_power_name(exponent), exponent, input.seed});
        }
    }
    return inputs;
}

class TestSignedPowerReference : public TestSignedPowerState, public WithParamInterface<SignedPowerReferenceInput>
{
protected:
    void SetUp() override;
};

void TestSignedPowerReference::SetUp()
{
    g_c_exponent = GetParam().exponent;
}

TEST_P(TestSignedPowerReference, matchesTheNativeIntegerKernel)
{
    const std::complex<double> center{0.125, -0.0625};
    const std::complex<double> expected{ordinary_power(GetParam().seed) + center};
    std::complex<double> actual{GetParam().seed};

    mandel_z_power_ref_pt(center, actual);

    EXPECT_NEAR(expected.real(), actual.real(), 1e-12 * std::max(1.0, std::abs(expected)));
    EXPECT_NEAR(expected.imag(), actual.imag(), 1e-12 * std::max(1.0, std::abs(expected)));
}

INSTANTIATE_TEST_SUITE_P(SignedPowers, TestSignedPowerReference, ValuesIn(signed_reference_inputs()),
    [](const TestParamInfo<SignedPowerReferenceInput> &info) { return info.param.name; });

struct SignedPowerDeltaInput
{
    std::string name;
    int exponent;
    std::complex<double> reference;
    std::complex<double> delta;
    std::complex<double> parameter_delta;
    std::optional<std::complex<double>> expected;
};

std::vector<SignedPowerDeltaInput> signed_delta_inputs()
{
    std::vector<SignedPowerDeltaInput> inputs;
    for (const int exponent : {0, 1, -1, -2, -3, -29})
    {
        for (const SignedPowerReferenceInput &input :
            {SignedPowerReferenceInput{"Zero", exponent, {}}, {"Positive", exponent, {0.75, 0.125}},
                {"Half", exponent, {0.5, 0.0}}, {"Tiny", exponent, {1e-160, 0.0}}, {"Large", exponent, {1e10, 0.0}}})
        {
            inputs.push_back({input.name + signed_power_name(exponent), exponent, input.seed,
                input.seed == std::complex<double>{} ? std::complex<double>{0.5, 0.0} : -input.seed, {0.003, -0.002},
                std::nullopt});
        }
        inputs.push_back({"SubDouble" + signed_power_name(exponent), exponent, {1.0, 0.0}, {1e-40, -2e-40}, {},
            std::complex<double>{exponent * 1e-40, exponent * -2e-40}});
        inputs.push_back({"OpposingPowers" + signed_power_name(exponent), exponent, {1.25, -0.25},
            std::complex<double>{0.75, 0.125} - std::complex<double>{1.25, -0.25}, {}, std::nullopt});
    }
    return inputs;
}

class TestSignedPowerDelta : public TestSignedPowerState, public WithParamInterface<SignedPowerDeltaInput>
{
protected:
    void SetUp() override;
};

void TestSignedPowerDelta::SetUp()
{
    g_c_exponent = GetParam().exponent;
}

TEST_P(TestSignedPowerDelta, retainsNativeReciprocalAndDisplacementRules)
{
    const SignedPowerDeltaInput &input{GetParam()};
    const std::complex<double> expected{input.expected
            ? *input.expected
            : ordinary_power(input.reference + input.delta) - ordinary_power(input.reference) + input.parameter_delta};
    const double tolerance{input.expected ? 1e-53 : 1e-12 * std::max(1.0, std::abs(expected))};
    std::complex<double> actual{input.delta};

    mandel_z_power_perturb(input.reference, actual, input.parameter_delta);

    EXPECT_NEAR(expected.real(), actual.real(), tolerance);
    EXPECT_NEAR(expected.imag(), actual.imag(), tolerance);
}

INSTANTIATE_TEST_SUITE_P(SignedPowers, TestSignedPowerDelta, ValuesIn(signed_delta_inputs()),
    [](const TestParamInfo<SignedPowerDeltaInput> &info) { return info.param.name; });

struct PreciseSignedReferenceInput
{
    std::string name;
    int exponent;
    const char *real;
};

std::vector<PreciseSignedReferenceInput> precise_signed_reference_inputs()
{
    std::vector<PreciseSignedReferenceInput> inputs;
    for (const int exponent : {0, 1, -1, -2, -3, -29})
    {
        inputs.push_back({"Zero" + signed_power_name(exponent), exponent, "0"});
        inputs.push_back(
            {"Positive" + signed_power_name(exponent), exponent, "0.75000000000000000000000000000000000000001"});
        inputs.push_back({"Negative" + signed_power_name(exponent), exponent, "-0.75"});
    }
    return inputs;
}

class TestPreciseSignedPowerReference : public TestSignedPowerState,
                                        public WithParamInterface<PreciseSignedReferenceInput>
{
protected:
    void SetUp() override;
};

void TestPreciseSignedPowerReference::SetUp()
{
    g_c_exponent = GetParam().exponent;
    initialize_signed_precision();
}

TEST_P(TestPreciseSignedPowerReference, matchesTheNativePowerKernel)
{
    BigStackSaver saved;
    BFComplex exponent{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex actual{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex seed{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex expected{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const BFComplex center{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const BigFloat difference{alloc_stack(g_r_bf_length + 2)};
    int_to_bf(exponent.x, GetParam().exponent);
    clear_bf(exponent.y);
    float_to_bf(center.x, 0.125);
    float_to_bf(center.y, -0.0625);
    str_to_bf(seed.x, GetParam().real);
    if (is_bf_zero(seed.x))
    {
        clear_bf(seed.y);
    }
    else
    {
        float_to_bf(seed.y, 0.125);
    }
    copy_bf(actual.x, seed.x);
    copy_bf(actual.y, seed.y);
    cmplx_pow_bf(&expected, &seed, &exponent);
    add_a_bf(expected.x, center.x);
    add_a_bf(expected.y, center.y);

    mandel_z_power_ref_pt_bf(center, actual);

    sub_bf(difference, actual.x, expected.x);
    EXPECT_NEAR(0.0, static_cast<double>(bf_to_float(difference)), 1e-48);
    sub_bf(difference, actual.y, expected.y);
    EXPECT_NEAR(0.0, static_cast<double>(bf_to_float(difference)), 1e-48);
}

INSTANTIATE_TEST_SUITE_P(SignedPowers, TestPreciseSignedPowerReference, ValuesIn(precise_signed_reference_inputs()),
    [](const TestParamInfo<PreciseSignedReferenceInput> &info) { return info.param.name; });

std::vector<SignedPowerDeltaInput> precise_signed_delta_inputs()
{
    std::vector<SignedPowerDeltaInput> inputs;
    for (const int exponent : {0, 1, -1, -2, -3, -29})
    {
        for (const SignedPowerReferenceInput &input : {SignedPowerReferenceInput{"Zero", exponent, {}},
                 {"Positive", exponent, {0.75, 0.125}}, {"Small", exponent, {1e-10, 0.0}}})
        {
            inputs.push_back({input.name + "Cancelled" + signed_power_name(exponent), exponent, input.seed,
                input.seed == std::complex<double>{} ? std::complex<double>{0.5, 0.0} : -input.seed, {0.003, -0.002},
                std::nullopt});
            inputs.push_back({input.name + "Relative" + signed_power_name(exponent), exponent, input.seed,
                input.seed * 0.125, {0.003, -0.002}, std::nullopt});
        }
    }
    return inputs;
}

class TestPreciseSignedPowerDelta : public TestSignedPowerState, public WithParamInterface<SignedPowerDeltaInput>
{
protected:
    void SetUp() override;
};

void TestPreciseSignedPowerDelta::SetUp()
{
    g_c_exponent = GetParam().exponent;
    initialize_signed_precision();
}

TEST_P(TestPreciseSignedPowerDelta, preservesZeroAndSmallNonzeroInputs)
{
    const SignedPowerDeltaInput &input{GetParam()};
    const std::complex<double> expected{
        precise_power(input.reference + input.delta) - precise_power(input.reference) + input.parameter_delta};
    std::complex<double> actual{input.delta};

    mandel_z_power_perturb(input.reference, actual, input.parameter_delta);

    EXPECT_NEAR(expected.real(), actual.real(), 1e-12 * std::max(1.0, std::abs(expected)));
    EXPECT_NEAR(expected.imag(), actual.imag(), 1e-12 * std::max(1.0, std::abs(expected)));
}

INSTANTIATE_TEST_SUITE_P(SignedPowers, TestPreciseSignedPowerDelta, ValuesIn(precise_signed_delta_inputs()),
    [](const TestParamInfo<SignedPowerDeltaInput> &info) { return info.param.name; });

class TestComplexMandelZPowerPerturbation : public TestWithParam<std::complex<double>>
{
protected:
    void SetUp() override
    {
        g_params[2] = GetParam().real();
        g_params[3] = GetParam().imag();
        g_c_exponent = static_cast<int>(g_params[2]);
    }

    void TearDown() override
    {
        if (m_big_float)
        {
            free_bf_vars();
        }
    }

    std::complex<double> native_power(const std::complex<double> &z)
    {
        const DComplex result{id::math::pow(DComplex{z.real(), z.imag()}, DComplex{g_params[2], g_params[3]})};
        return {result.x, result.y};
    }

    void initialize_precision()
    {
        init_bf_dec(60);
        m_big_float = true;
        init_big_pi();
    }

    bool m_big_float{};
    ValueSaver<double> m_saved_real{g_params[2]};
    ValueSaver<double> m_saved_imag{g_params[3]};
    ValueSaver<int> m_saved_exponent{g_c_exponent};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL_Z_POWER};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
};

TEST_P(TestComplexMandelZPowerPerturbation, referenceUsesBothExponentComponentsAndTheNativeBranch)
{
    const std::complex<double> center{0.0625, -0.03125};
    for (const std::complex<double> seed :
        {std::complex<double>{}, {0.75, 0.125}, {-0.75, 0.0001}, {-0.75, -0.0001}, {-0.75, 0.0}})
    {
        SCOPED_TRACE(seed);
        const std::complex<double> expected{native_power(seed) + center};
        std::complex<double> actual{seed};
        mandel_z_power_ref_pt(center, actual);
        EXPECT_EQ(expected, actual);
    }
}

TEST_P(TestComplexMandelZPowerPerturbation, legacyPerturbationSetupRetainsNativeSymmetryAndOrbitSelection)
{
    ValueSaver saved_mode{g_std_calc_mode, CalcMode::PERTURBATION};
    ValueSaver saved_specific{g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL_Z_POWER)};
    ValueSaver saved_dispatch{g_dispatch, FractalDispatch{*g_cur_fractal_specific}};
    ValueSaver saved_parameter{g_float_param};
    ValueSaver saved_power{g_power_z};
    ValueSaver saved_symmetry{g_symmetry, g_cur_fractal_specific->symmetry};
    ValueSaver saved_periodicity{g_periodicity_check, 0};
    ValueSaver saved_screen{g_logical_screen, LogicalScreen{8, 6, 0, 0, 7.0, 5.0}};
    EXPECT_FALSE(mandel_per_image());
    EXPECT_EQ(mandel_z_power_cmplx_orbit, g_dispatch.orbit_calc());
    if (GetParam().imag() != 0.0)
    {
        EXPECT_EQ(SymmetryType::NONE, g_symmetry);
    }
    initialize_precision();
    g_symmetry = g_cur_fractal_specific->symmetry;
    EXPECT_FALSE(mandel_per_image_bf());
    if (GetParam().imag() != 0.0)
    {
        EXPECT_EQ(SymmetryType::NONE, g_symmetry);
    }
}

TEST_P(TestComplexMandelZPowerPerturbation, deltaMatchesNativeUpdatesWithinAndAcrossTheCut)
{
    const std::complex<double> parameter_delta{0.003, -0.002};

    struct Sample
    {
        std::complex<double> ref;
        std::complex<double> delta;
    };

    for (const Sample &sample : {Sample{{0.75, 0.125}, {0.002, -0.001}}, {{-0.75, 0.0001}, {0.002, -0.0002}},
             {{-0.75, -0.0001}, {0.002, 0.0002}}, {{-0.75, 0.0001}, {0.0001, 0.00005}}, {{}, {0.125, 0.0625}},
             {{0.125, 0.0625}, {-0.125, -0.0625}}, {{1e-300, 0.0}, {1e-301, 0.0}}})
    {
        SCOPED_TRACE(sample.ref);
        SCOPED_TRACE(sample.delta);
        const std::complex<double> expected{
            native_power(sample.ref + sample.delta) - native_power(sample.ref) + parameter_delta};
        std::complex<double> actual{sample.delta};
        mandel_z_power_perturb(sample.ref, actual, parameter_delta);
        EXPECT_NEAR(expected.real(), actual.real(), 1e-12 * std::max(1.0, std::abs(expected)));
        EXPECT_NEAR(expected.imag(), actual.imag(), 1e-12 * std::max(1.0, std::abs(expected)));
    }
}

TEST_P(TestComplexMandelZPowerPerturbation, deltaRetainsSubDoubleDisplacements)
{
    const std::complex<double> delta{1e-40, -2e-40};
    for (const std::complex<double> ref : {std::complex<double>{1.0, 0.0}, {-1.0, 1e-30}, {-1.0, -1e-30}})
    {
        SCOPED_TRACE(ref);
        const std::complex<double> expected{GetParam() * native_power(ref) * delta / ref};
        std::complex<double> actual{delta};
        mandel_z_power_perturb(ref, actual, {});
        EXPECT_NEAR(expected.real(), actual.real(), 1e-53);
        EXPECT_NEAR(expected.imag(), actual.imag(), 1e-53);
    }
}

TEST_P(TestComplexMandelZPowerPerturbation, preciseReferenceUsesTheNativeArbitraryPrecisionKernel)
{
    initialize_precision();
    BigStackSaver saved;
    BFComplex seed{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex actual{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex expected{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex exponent{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const BFComplex center{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const BigFloat difference{alloc_stack(g_r_bf_length + 2)};
    float_to_bf(exponent.x, GetParam().real());
    float_to_bf(exponent.y, GetParam().imag());
    float_to_bf(center.x, 0.0625);
    float_to_bf(center.y, -0.03125);
    for (const char *imag : {"1e-40", "-1e-40", "0"})
    {
        SCOPED_TRACE(imag);
        str_to_bf(seed.x, "-0.75000000000000000000000000000000000000001");
        str_to_bf(seed.y, imag);
        copy_bf(actual.x, seed.x);
        copy_bf(actual.y, seed.y);
        cmplx_pow_bf(&expected, &seed, &exponent);
        add_a_bf(expected.x, center.x);
        add_a_bf(expected.y, center.y);
        mandel_z_power_ref_pt_bf(center, actual);
        sub_bf(difference, actual.x, expected.x);
        EXPECT_NEAR(0.0, static_cast<double>(bf_to_float(difference)), 1e-48);
        sub_bf(difference, actual.y, expected.y);
        EXPECT_NEAR(0.0, static_cast<double>(bf_to_float(difference)), 1e-48);
    }
    clear_bf(actual.x);
    clear_bf(actual.y);
    mandel_z_power_ref_pt_bf(center, actual);
    EXPECT_EQ(0, cmp_bf(center.x, actual.x));
    EXPECT_EQ(0, cmp_bf(center.y, actual.y));
}

INSTANTIATE_TEST_SUITE_P(ComplexPowers, TestComplexMandelZPowerPerturbation,
    Values(std::complex<double>{2.5, 0.0}, std::complex<double>{0.5, 0.0}, std::complex<double>{-0.5, 0.0},
        std::complex<double>{2.5, 0.25}, std::complex<double>{0.25, -0.5}, std::complex<double>{0.0, 0.5},
        std::complex<double>{0.03125, 0.015625}));

TEST(TestMandelZPowerPerturbationLimits, forcedComplexIntegerPowersRetainTheComplexZeroConvention)
{
    ValueSaver saved_debug{g_debug_flag, DebugFlags::FORCE_COMPLEX_POWER};
    ValueSaver saved_real{g_params[2]};
    ValueSaver saved_imag{g_params[3], 0.0};
    ValueSaver saved_exponent{g_c_exponent};
    ValueSaver saved_math{g_bf_math, BFMathType::NONE};
    for (const int exponent : {0, 1, -1, -2, 2})
    {
        SCOPED_TRACE(exponent);
        g_c_exponent = exponent;
        g_params[2] = exponent;
        std::complex<double> reference{};
        mandel_z_power_ref_pt({}, reference);
        EXPECT_EQ(std::complex<double>{}, reference);
        std::complex<double> delta{0.5, 0.0};
        const DComplex expected{id::math::pow(DComplex{0.5, 0.0}, DComplex{static_cast<double>(exponent), 0.0})};
        mandel_z_power_perturb({}, delta, {});
        EXPECT_NEAR(expected.x, delta.real(), 1e-13);
        EXPECT_NEAR(expected.y, delta.imag(), 1e-13);
    }
}

} // namespace id::test
