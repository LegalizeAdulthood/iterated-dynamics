// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/bailout_formula.h>
#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/convert_center_mag.h>
#include <engine/fractalb.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/biginit.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <ui/id_keys.h>
#include <ui/precision_diagnostics.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iterator>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestPrecisionDiagnostics : public Test
{
protected:
    void SetUp() override
    {
        init_bf_dec(20);
        const BigFloat corners[]{g_bf_x_min, g_bf_x_max, g_bf_y_min, g_bf_y_max, g_bf_x_3rd, g_bf_y_3rd,
            g_bf_save_x_min, g_bf_save_x_max, g_bf_save_y_min, g_bf_save_y_max, g_bf_save_x_3rd, g_bf_save_y_3rd};
        const BigNum bn_corners[]{g_x_min_bn, g_x_max_bn, g_y_min_bn, g_y_max_bn, g_x_3rd_bn, g_y_3rd_bn};
        const double values[]{-2.0, 2.0, -1.5, 1.5, -2.0, -1.5};
        for (size_t i = 0; i < std::size(corners); ++i)
        {
            float_to_bf(corners[i], values[i % std::size(values)]);
        }
        for (size_t i = 0; i < std::size(bn_corners); ++i)
        {
            float_to_bn(bn_corners[i], values[i]);
        }
    }

    void TearDown() override
    {
        free_bf_vars();
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<long> m_saved_bailout{g_user.bailout_value, 0};
    ValueSaver<Bailout> m_saved_bailout_test{g_bailout_test, Bailout::MOD};
    ValueSaver<int> m_saved_digits{g_bf_digits, 0};
    ValueSaver<ImageRegion> m_saved_region{g_image_region, ImageRegion{{-2.0, -1.5}, {2.0, 1.5}, {-2.0, -1.5}}};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{2, 2, 0, 0, 1.0, 1.0}};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status};
    ValueSaver<LDouble> m_saved_delta_x{g_delta_x, 0.125L};
    ValueSaver<LDouble> m_saved_delta_y{g_delta_y, 0.25L};
    ValueSaver<LDouble> m_saved_delta_x2{g_delta_x2, 0.5L};
    ValueSaver<LDouble> m_saved_delta_y2{g_delta_y2, 0.75L};
    ValueSaver<std::vector<std::string>> m_saved_diagnostics{g_precision_diagnostics, std::vector<std::string>{}};
};

TEST_F(TestPrecisionDiagnostics, formatsDoubleCornersAndPixelDeltasWithoutInput)
{
    const std::string message{format_corners_dbl("corners")};

    EXPECT_THAT(message, StartsWith("corners\n"));
    EXPECT_THAT(message, HasSubstr("x_min= -2.00000000000000000000"));
    EXPECT_THAT(message, HasSubstr("y_max= 1.50000000000000000000"));
    EXPECT_THAT(message, HasSubstr("delta_x= 0.12500000000000000000"));
    EXPECT_THAT(message, HasSubstr("delta_y2= 0.75000000000000000000"));
    EXPECT_TRUE(g_precision_diagnostics.empty());
}

TEST_F(TestPrecisionDiagnostics, formatsArbitraryPrecisionCornersAndSavedCorners)
{
    EXPECT_THAT(format_corners_bn("bn"), AllOf(StartsWith("bn\n"), HasSubstr("x_min_bn=-2.")));
    EXPECT_THAT(format_corners_bf("bf"), AllOf(StartsWith("bf\n"), HasSubstr("bf_y_min=-1.5")));
    EXPECT_THAT(format_corners_bf_save("saved"), AllOf(StartsWith("saved\n"), HasSubstr("bf_save_x_max=2.0")));
    EXPECT_THAT(
        format_globals_bf("globals"), AllOf(StartsWith("globals\n"), HasSubstr("bn_step=4"), HasSubstr("decimals=20")));
}

TEST_F(TestPrecisionDiagnostics, formatsValuesAndComparisonsWithoutPolling)
{
    EXPECT_THAT(format_var_bn("value", g_y_min_bn), StartsWith("value -1.5"));
    EXPECT_THAT(format_var_bf("value", g_bf_y_min), StartsWith("-1.5e0"));
    EXPECT_THAT(format_two_bf("left", g_bf_x_min, "right", g_bf_x_max, 8),
        AllOf(HasSubstr("left->-2.0e0"), HasSubstr("right->2.0e0")));
    EXPECT_THAT(
        format_three_bf("left", g_bf_x_min, "right", g_bf_x_max, "bottom", g_bf_y_min, 8), HasSubstr("bottom->-1.5e0"));
    EXPECT_THAT(format_compare_values("bn", -1.5L, g_y_min_bn),
        AllOf(HasSubstr("bignum=-1.5"), HasSubstr("double=-1.50000000000000000000")));
    EXPECT_THAT(format_compare_values_bf("bf", -1.5L, g_bf_y_min), HasSubstr("bignum=-1.5e0"));
}

TEST_F(TestPrecisionDiagnostics, shortVariablesDoNotRequireSeventyNineCharacters)
{
    EXPECT_LT(format_var_bn("n", g_x_min_bn).size(), 79U);
    EXPECT_LT(format_var_bf("n", g_bf_x_min).size(), 79U);
    EXPECT_EQ(std::string(79, 'x'), format_var_bn(std::string(100, 'x').c_str(), g_x_min_bn));
}

TEST_F(TestPrecisionDiagnostics, aspectFormattingRestoresTemporaryStack)
{
    const int stack{save_stack()};

    EXPECT_THAT(format_aspect("ratio"), AllOf(StartsWith("aspect ratio\n"), HasSubstr("bf     0.75")));

    EXPECT_EQ(stack, save_stack());
}

TEST_F(TestPrecisionDiagnostics, collapsedCoordinatesCollectDebugSnapshotWithoutInput)
{
    g_image_region = {};

    fractal_float_to_bf();

#ifndef NDEBUG
    ASSERT_EQ(1U, g_precision_diagnostics.size());
    EXPECT_THAT(g_precision_diagnostics.front(), StartsWith("getprecdbl\n"));
#else
    EXPECT_TRUE(g_precision_diagnostics.empty());
#endif
    EXPECT_EQ(0.0L, bf_to_float(g_bf_x_min));
}

TEST_F(TestPrecisionDiagnostics, conversionDebugSnapshotPreservesOriginalRegion)
{
    g_image_region = {
        {-2.123456789e80, -1.456789012e80}, {1.654321098e80, 1.987654321e80}, {-1.999876543e80, -1.123456789e80}};
    const ImageRegion original{g_image_region};
    double x{}, y{}, factor{}, rotation{}, skew{};
    LDouble magnification{};

    cvt_center_mag(x, y, magnification, factor, rotation, skew);

    EXPECT_EQ(original, g_image_region);
#ifndef NDEBUG
    ASSERT_EQ(1U, g_precision_diagnostics.size());
    EXPECT_THAT(g_precision_diagnostics.front(), StartsWith("cvtcentermag problem\n"));
#else
    EXPECT_TRUE(g_precision_diagnostics.empty());
#endif
}

class TestPrecisionDiagnosticDisplay : public Test
{
protected:
    void expect_dialogs(const int count)
    {
        EXPECT_CALL(m_driver, stack_screen()).Times(count);
        EXPECT_CALL(m_driver, unstack_screen()).Times(count);
        EXPECT_CALL(m_driver, move_cursor(_, _)).Times(count);
        EXPECT_CALL(m_driver, hide_text_cursor()).Times(count);
        EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(count);
        EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM)).Times(count);
        EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<std::vector<std::string>> m_saved_diagnostics{g_precision_diagnostics, std::vector<std::string>{}};
};

TEST_F(TestPrecisionDiagnosticDisplay, emptyCollectionDoesNotPollOrDisplay)
{
    EXPECT_FALSE(show_precision_diagnostics());
}

TEST_F(TestPrecisionDiagnosticDisplay, acknowledgementDrainsSnapshotsInOrder)
{
    g_precision_diagnostics = {"first snapshot", "second snapshot"};
    expect_dialogs(2);
    EXPECT_CALL(m_driver, get_key()).WillRepeatedly(Return(ID_KEY_ENTER));
    {
        InSequence sequence;
        EXPECT_CALL(m_driver, put_string(_, _, _, StrEq("first snapshot")));
        EXPECT_CALL(m_driver, put_string(_, _, _, StrEq("second snapshot")));
    }

    EXPECT_FALSE(show_precision_diagnostics());

    EXPECT_TRUE(g_precision_diagnostics.empty());
    EXPECT_FALSE(show_precision_diagnostics());
}

TEST_F(TestPrecisionDiagnosticDisplay, escapeReturnsExitDecisionToUiOwner)
{
    g_precision_diagnostics = {"first snapshot", "second snapshot"};
    expect_dialogs(1);
    EXPECT_CALL(m_driver, put_string(_, _, _, StrEq("first snapshot")));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));

    EXPECT_TRUE(show_precision_diagnostics());

    EXPECT_TRUE(g_precision_diagnostics.empty());
}

} // namespace id::test
