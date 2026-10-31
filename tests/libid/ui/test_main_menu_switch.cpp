// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/main_menu_switch.h>

#include <ValueUnchanged.h>

#include <engine/calcfrac.h>
#include <engine/UserData.h>
#include <misc/ValueSaver.h>
#include <ui/big_while_loop.h>

#include <gtest/gtest.h>

using namespace id::engine;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestMainMenuSwitch : public Test
{
protected:
    CommandResult execute(const int key)
    {
        m_context.key = key;
        return main_menu_switch(m_context);
    }

    MainContext m_context{};
};

TEST_F(TestMainMenuSwitch, nothingChangedOnLowerCaseM)
{
    VALUE_UNCHANGED(g_quick_calc, false);
    ValueUnchanged saved_g_user_std_calc_mode("g_user.std_calc_mode", g_user.std_calc_mode, CalcMode::SOLID_GUESS);

    const CommandResult result{execute('m')};

    EXPECT_EQ(CommandResult::IGNORED, result);
    EXPECT_EQ('m', m_context.key);
    EXPECT_FALSE(m_context.from_mandel);
    EXPECT_FALSE(m_context.more_keys);
    EXPECT_FALSE(m_context.stacked);
}

class TestMainMenuSwitchQuickCalc : public TestMainMenuSwitch, public WithParamInterface<CalcStatus>
{
protected:
    ValueSaver<bool> m_saved_quick_calc{g_quick_calc, true};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, GetParam()};
    ValueSaver<CalcMode> m_saved_mode{g_user.std_calc_mode, CalcMode::SOLID_GUESS};
    ValueSaver<CalcMode> m_saved_old_mode{g_old_std_calc_mode, CalcMode::ONE_PASS};
};

TEST_P(TestMainMenuSwitchQuickCalc, restoresCalculationMode)
{
    const CommandResult result{execute('m')};

    EXPECT_EQ(CommandResult::IGNORED, result);
    EXPECT_EQ('m', m_context.key);
    EXPECT_FALSE(m_context.from_mandel);
    EXPECT_FALSE(m_context.more_keys);
    EXPECT_FALSE(m_context.stacked);
    EXPECT_EQ(GetParam() != CalcStatus::COMPLETED, g_quick_calc);
    EXPECT_EQ(CalcMode::ONE_PASS, g_user.std_calc_mode);
}

INSTANTIATE_TEST_SUITE_P(Status, TestMainMenuSwitchQuickCalc, Values(CalcStatus::COMPLETED, CalcStatus::IN_PROGRESS),
    [](const TestParamInfo<CalcStatus> &info)
    { return info.param == CalcStatus::COMPLETED ? "Completed" : "InProgress"; });

} // namespace id::test
