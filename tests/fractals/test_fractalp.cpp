// SPDX-License-Identifier: GPL-3.0-only
//
#include <fractals/fractalp.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>

using namespace testing;

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
        if (FractalType julia_type = from.to_julia; julia_type != FractalType::NO_FRACTAL)
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
        if (FractalType mandel_type = from.to_mandel; mandel_type != FractalType::NO_FRACTAL)
        {
            EXPECT_NO_THROW(get_fractal_specific(mandel_type)) << "index " << i << " (" << from.name << ")";
            // type=inverse_julia is a special case;
            // it has a link to Mandelbrot, but doesn't have a link back.
            if (from.type != FractalType::INVERSE_JULIA && from.type != FractalType::INVERSE_JULIA_FP)
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

TEST(TestFractalSpecific, noIntegerTypesWithFloatEquivalent)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific &from{g_fractal_specific[i]};
        EXPECT_FALSE(from.is_integer != 0 && from.to_float != FractalType::NO_FRACTAL)
            << "index " << i << " (" << from.name << "), is_integer " //
            << from.is_integer << ", to_float " << +from.to_float;
    }
}

TEST(TestFractalSpecific, floatTypeHasNoIntegerEquivalent)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific &from{g_fractal_specific[i]};
        EXPECT_TRUE(
            (from.is_integer == 0 && from.to_float == FractalType::NO_FRACTAL) || from.is_integer != 0)
            << "index " << i << " (" << from.name << "), is_integer " //
            << from.is_integer << ", to_float " << +from.to_float;
    }
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
