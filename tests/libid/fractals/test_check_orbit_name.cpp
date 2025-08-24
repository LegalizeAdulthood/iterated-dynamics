// SPDX-License-Identifier: GPL-3.0-only
//
#include <fractals/check_orbit_name.h>

#include <fractals/fractalp.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

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
        EXPECT_FALSE(check_orbit_name(name.c_str())) << name;
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
        EXPECT_TRUE(check_orbit_name(name.c_str())) << name;
    }
}
