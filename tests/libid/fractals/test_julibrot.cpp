// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/julibrot.h>

#include <engine/fractals.h>
#include <engine/LogicalScreen.h>
#include <engine/VideoInfo.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <stdexcept>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::test
{

struct TestJulibrotSetup : testing::Test
{
    math::DComplex parameter{};
    ValueSaver<Driver *> saved_driver{g_driver, nullptr};
    ValueSaver<int> saved_colors{g_colors, 256};
    ValueSaver<LogicalScreen> saved_screen{g_logical_screen, LogicalScreen{64, 48, 0, 0, 63.0, 47.0}};
    ValueSaver<SaveDAC> saved_palette{g_save_dac, SaveDAC::YES};
    ValueSaver<Julibrot3DMode> saved_mode{g_julibrot_3d_mode, Julibrot3DMode::MONOCULAR};
    ValueSaver<math::DComplex *> saved_parameter{g_float_param, &parameter};
};

TEST_F(TestJulibrotSetup, rejectsLowColorModeBeforeInitializingWithoutUI)
{
    for (const int colors : {2, 4, 16, 254})
    {
        g_colors = colors;
        try
        {
            julibrot_per_image();
            FAIL() << "Expected a video-mode diagnostic";
        }
        catch (const std::runtime_error &error)
        {
            EXPECT_STREQ("Sorry, but Julibrots require a 256-color video mode", error.what());
        }
        EXPECT_EQ(&parameter, g_float_param);
        EXPECT_EQ(SaveDAC::YES, g_save_dac);
    }
}

TEST_F(TestJulibrotSetup, initializesWith256ColorsAndPreloadedPaletteWithoutUI)
{
    EXPECT_TRUE(julibrot_per_image());

    EXPECT_NE(nullptr, g_float_param);
    EXPECT_NE(&parameter, g_float_param);
    EXPECT_EQ(SaveDAC::YES, g_save_dac);
}

} // namespace id::test
