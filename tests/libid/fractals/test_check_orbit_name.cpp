// SPDX-License-Identifier: GPL-3.0-only
//
#include <fractals/check_orbit_name.h>

#include <fractals/fractalp.h>

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

using namespace id::fractals;

namespace id::test
{

TEST(TestCheckOrbitName, julibrotNames)
{
    std::vector<std::string> names;
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        if (bit_set(g_fractal_specific[i].flags, FractalFlags::OK_JB))
        {
            names.emplace_back(g_fractal_specific[i].name);
        }
    }

    for (const std::string &name : names)
    {
        EXPECT_FALSE(check_orbit_name(name)) << name;
    }
}

TEST(TestCheckOrbitName, notValidNames)
{
    std::vector<std::string> names;
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        if (!bit_set(g_fractal_specific[i].flags, FractalFlags::OK_JB))
        {
            names.emplace_back(g_fractal_specific[i].name);
        }
    }

    for (const std::string &name : names)
    {
        EXPECT_TRUE(check_orbit_name(name)) << name;
    }
}

TEST(TestCheckOrbitName, honorsStringViewLength)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        if (bit_set(g_fractal_specific[i].flags, FractalFlags::OK_JB))
        {
            const std::string storage{std::string{g_fractal_specific[i].name} + "-suffix"};
            const std::string_view name{storage.data(), std::string_view{g_fractal_specific[i].name}.size()};
            EXPECT_FALSE(check_orbit_name(name)) << name;
            return;
        }
    }
}

} // namespace id::test
