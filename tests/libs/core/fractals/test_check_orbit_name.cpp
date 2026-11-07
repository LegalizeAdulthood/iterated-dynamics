// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/check_orbit_name.h>

#include <fractals/fractalp.h>

#include <gtest/gtest.h>

#include <cctype>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

using namespace id::fractals;

namespace id::test
{

struct OrbitNameInput
{
    std::string name;
    int type;
    bool invalid;
};

void PrintTo(const OrbitNameInput &input, std::ostream *out)
{
    *out << input.name << "_" << input.type;
}

std::vector<OrbitNameInput> orbit_name_inputs()
{
    std::vector<OrbitNameInput> inputs;
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific &specific{g_fractal_specific[i]};
        inputs.push_back(
            {specific.name, static_cast<int>(specific.type), !bit_set(specific.flags, FractalFlags::OK_JB)});
    }
    return inputs;
}

class TestOrbitNameValidity : public testing::TestWithParam<OrbitNameInput>
{
};

TEST_P(TestOrbitNameValidity, matchesJulibrotEligibility)
{
    EXPECT_EQ(GetParam().invalid, check_orbit_name(GetParam().name)) << GetParam().name;
}

INSTANTIATE_TEST_SUITE_P(Names, TestOrbitNameValidity, testing::ValuesIn(orbit_name_inputs()),
    [](const testing::TestParamInfo<OrbitNameInput> &info)
    {
        std::string name{info.param.invalid ? "Invalid_" : "Julibrot_"};
        for (const unsigned char ch : info.param.name)
        {
            name += std::isalnum(ch) ? static_cast<char>(ch) : '_';
        }
        return name + "_Type" + std::to_string(info.param.type);
    });

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
