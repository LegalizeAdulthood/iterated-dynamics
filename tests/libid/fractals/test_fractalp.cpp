// SPDX-License-Identifier: GPL-3.0-only
//
#include <fractals/fractalp.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>

using namespace testing;
using namespace id::fractals;

namespace id::test
{

static int test_orbit_calc()
{
    return 17;
}

static int test_per_pixel()
{
    return 23;
}

static bool test_per_image()
{
    return true;
}

static int test_calc_type()
{
    return 31;
}

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

TEST(TestFractalDispatch, seededDispatchMatchesDefaults)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific &from{g_fractal_specific[i]};
        const FractalDispatch dispatch{make_fractal_dispatch(from)};

        EXPECT_EQ(&from, dispatch.specific) << "index " << i << ", " << from.name;
        EXPECT_EQ(from.orbit_calc, dispatch.orbit_calc) << "index " << i << ", " << from.name;
        EXPECT_EQ(from.per_pixel, dispatch.per_pixel) << "index " << i << ", " << from.name;
        EXPECT_EQ(from.per_image, dispatch.per_image) << "index " << i << ", " << from.name;
        EXPECT_EQ(from.calc_type, dispatch.calc_type) << "index " << i << ", " << from.name;
        EXPECT_EQ(from.symmetry, dispatch.symmetry) << "index " << i << ", " << from.name;
    }
}

TEST(TestFractalDispatch, setFractalTypeSeedsCurrentDispatch)
{
    ValueSaver saved_type{g_fractal_type};
    ValueSaver saved_specific{g_cur_fractal_specific};
    ValueSaver saved_dispatch{g_fractal_dispatch};

    set_fractal_type(FractalType::MANDEL);

    const FractalSpecific &from{*get_fractal_specific(FractalType::MANDEL)};
    EXPECT_EQ(&from, g_fractal_dispatch.specific);
    EXPECT_EQ(from.orbit_calc, g_fractal_dispatch.orbit_calc);
    EXPECT_EQ(from.per_pixel, g_fractal_dispatch.per_pixel);
    EXPECT_EQ(from.per_image, g_fractal_dispatch.per_image);
    EXPECT_EQ(from.calc_type, g_fractal_dispatch.calc_type);
    EXPECT_EQ(from.symmetry, g_fractal_dispatch.symmetry);
}

TEST(TestFractalDispatch, accessorsUseCurrentDispatch)
{
    FractalDispatch dispatch{};
    dispatch.orbit_calc = test_orbit_calc;
    dispatch.per_pixel = test_per_pixel;
    dispatch.per_image = test_per_image;
    dispatch.calc_type = test_calc_type;

    ValueSaver saved_specific{g_cur_fractal_specific, nullptr};
    ValueSaver saved_dispatch{g_fractal_dispatch, dispatch};

    EXPECT_TRUE(per_image());
    EXPECT_EQ(23, per_pixel());
    EXPECT_EQ(17, orbit_calc());
    EXPECT_EQ(test_calc_type, current_calc_type());
    EXPECT_EQ(31, current_calc_type()());
}

TEST(TestFractalDispatch, settersUpdateDispatchAndCompatibilityTable)
{
    FractalSpecific specific{};
    ValueSaver saved_specific{g_cur_fractal_specific, &specific};
    ValueSaver saved_dispatch{g_fractal_dispatch};

    set_current_orbit_calc(test_orbit_calc);
    set_current_per_pixel(test_per_pixel);
    set_current_per_image(test_per_image);

    EXPECT_EQ(test_orbit_calc, g_fractal_dispatch.orbit_calc);
    EXPECT_EQ(test_per_pixel, g_fractal_dispatch.per_pixel);
    EXPECT_EQ(test_per_image, g_fractal_dispatch.per_image);
    EXPECT_EQ(test_orbit_calc, specific.orbit_calc);
    EXPECT_EQ(test_per_pixel, specific.per_pixel);
    EXPECT_EQ(test_per_image, specific.per_image);
}

TEST(TestFractalDispatch, alternateMathOnlyUpdatesDispatch)
{
    FractalSpecific specific{};
    FractalDispatch dispatch{};
    AlternateMath alternate{
        FractalType::MANDEL, math::BFMathType::BIG_NUM, test_orbit_calc, test_per_pixel, test_per_image};

    ValueSaver saved_specific{g_cur_fractal_specific, &specific};
    ValueSaver saved_dispatch{g_fractal_dispatch, dispatch};

    set_current_alternate_math(alternate);

    EXPECT_EQ(test_orbit_calc, g_fractal_dispatch.orbit_calc);
    EXPECT_EQ(test_per_pixel, g_fractal_dispatch.per_pixel);
    EXPECT_EQ(test_per_image, g_fractal_dispatch.per_image);
    EXPECT_EQ(nullptr, specific.orbit_calc);
    EXPECT_EQ(nullptr, specific.per_pixel);
    EXPECT_EQ(nullptr, specific.per_image);
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
