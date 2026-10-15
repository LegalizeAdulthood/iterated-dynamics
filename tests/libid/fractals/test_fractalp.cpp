// SPDX-License-Identifier: GPL-3.0-only
//
#include <fractals/fractalp.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>

using namespace testing;
using namespace id::fractals;

namespace id::test
{

struct NumTrigFnsParam
{
    FractalFlags flags;
    int expected;
};

class TestNumTrigFns : public TestWithParam<NumTrigFnsParam>
{
};

TEST_P(TestNumTrigFns, countsFunctions)
{
    EXPECT_EQ(GetParam().expected, num_trig_fns(GetParam().flags));
}

INSTANTIATE_TEST_SUITE_P(Flags, TestNumTrigFns,
    Values(NumTrigFnsParam{FractalFlags::NONE, 0}, NumTrigFnsParam{FractalFlags::NO_ZOOM, 0},
        NumTrigFnsParam{FractalFlags::NO_GUESS, 0}, NumTrigFnsParam{FractalFlags::NO_TRACE, 0},
        NumTrigFnsParam{FractalFlags::NO_ROTATE, 0}, NumTrigFnsParam{FractalFlags::NO_RESUME, 0},
        NumTrigFnsParam{FractalFlags::INF_CALC, 0}, NumTrigFnsParam{FractalFlags::TRIG1, 1},
        NumTrigFnsParam{FractalFlags::TRIG2, 2}, NumTrigFnsParam{FractalFlags::TRIG3, 3},
        NumTrigFnsParam{FractalFlags::TRIG4, 4}, NumTrigFnsParam{FractalFlags::PARAMS_3D, 0},
        NumTrigFnsParam{FractalFlags::OK_JB, 0}, NumTrigFnsParam{FractalFlags::MORE, 0},
        NumTrigFnsParam{FractalFlags::BAIL_TEST, 0}, NumTrigFnsParam{FractalFlags::BF_MATH, 0},
        NumTrigFnsParam{FractalFlags::LD_MATH, 0}, NumTrigFnsParam{FractalFlags::PERTURB, 0}));

TEST(TestFractalSpecific, perturbationFlagRequiresPerturbationFunctions)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        if (!bit_set(g_fractal_specific[i].flags, FractalFlags::PERTURB))
        {
            continue;
        }

        EXPECT_NE(nullptr, g_fractal_specific[i].pert_ref)
            << "Fractal type " << g_fractal_specific[i].name << " (" << i
            << ") has no perturbation reference function";
        EXPECT_NE(nullptr, g_fractal_specific[i].pert_ref_bf)
            << "Fractal type " << g_fractal_specific[i].name << " (" << i
            << ") has no perturbation bigfloat reference function";
        EXPECT_NE(nullptr, g_fractal_specific[i].pert_pt)
            << "Fractal type " << g_fractal_specific[i].name << " (" << i
            << ") has no perturbation point function";
    }
}

TEST(TestFractalSpecific, typeMatchesGet)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific &from{g_fractal_specific[i]};
        EXPECT_EQ(&from, get_fractal_specific(from.type)) << "index " << i << ", " << g_fractal_specific[i].name;
    }
}

TEST(TestFractalSpecific, toJuliaExists)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific &from{g_fractal_specific[i]};
        if (const FractalType julia_type = from.to_julia; julia_type != FractalType::NO_FRACTAL)
        {
            EXPECT_NO_THROW(get_fractal_specific(julia_type)) << "index " << i << " (" << from.name << ")";
            EXPECT_EQ(from.type, get_fractal_specific(julia_type)->to_mandel)
                << "index " << i << " (" << from.name << ") mismatched Julia/Mandelbrot toggle";
        }
    }
}

TEST(TestFractalSpecific, toMandelbrotExists)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific &from{g_fractal_specific[i]};
        if (const FractalType mandel_type = from.to_mandel; mandel_type != FractalType::NO_FRACTAL)
        {
            EXPECT_NO_THROW(get_fractal_specific(mandel_type)) << "index " << i << " (" << from.name << ")";
            // type=inverse_julia is a special case;
            // it has a link to Mandelbrot, but doesn't have a link back.
            if (from.type != FractalType::INVERSE_JULIA)
            {
                EXPECT_EQ(from.type, get_fractal_specific(mandel_type)->to_julia)
                    << "index " << i << " (" << from.name << ") mismatched Julia/Mandelbrot toggle";
            }
        }
    }
}

std::ostream &operator<<(std::ostream &str, const FractalSpecific &value)
{
    return str << "type: " << +value.type << " '" << value.name << "'";
}

TEST(TestFractalSpecific, fractalSpecificEntriesAreSortedByType)
{
    EXPECT_TRUE(std::is_sorted(&g_fractal_specific[0], &g_fractal_specific[g_num_fractal_types],
        [](const FractalSpecific &lhs, const FractalSpecific &rhs) { return lhs.type < rhs.type; }));
}

TEST(TestGetFractalSpecific, resultMatchesType)
{
    const FractalSpecific *entry = get_fractal_specific(FractalType::CELLULAR);

    ASSERT_NE(nullptr, entry);
    EXPECT_EQ(FractalType::CELLULAR, entry->type);
}

TEST(TestGetFractalSpecific, unknownTypeThrowsRuntimeError)
{
    EXPECT_THROW(get_fractal_specific(static_cast<FractalType>(-100)), std::runtime_error);
}

} // namespace id::test
