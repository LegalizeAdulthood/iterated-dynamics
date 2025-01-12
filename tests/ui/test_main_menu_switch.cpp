// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/main_menu_switch.h>

#include "ValueUnchanged.h"

#include <engine/calcfrac.h>
#include <engine/id_data.h>
#include <misc/ValueSaver.h>
#include <ui/framain2.h>

#include <gtest/gtest.h>

using namespace testing;

namespace
{

class TestMainMenuSwitch : public Test
{
protected:
    MainState execute(int key)
    {
        m_context.key = key;
        return main_menu_switch(m_context);
    }

    MainContext m_context{};
};

} // namespace

TEST_F(TestMainMenuSwitch, nothingChangedOnLowerCaseM)
{
    VALUE_UNCHANGED(g_quick_calc, false);
    VALUE_UNCHANGED(g_user_std_calc_mode, 'g');

    const MainState result{execute('m')};

    EXPECT_EQ(MainState::NOTHING, result);
    EXPECT_EQ('m', m_context.key);
    EXPECT_FALSE(m_context.from_mandel);
    EXPECT_FALSE(m_context.more_keys);
    EXPECT_FALSE(m_context.stacked);
}

TEST_F(TestMainMenuSwitch, quickCalcResetOnImageCompleted)
{
    ValueSaver saved_quick_calc{g_quick_calc, true};
    ValueSaver saved_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver saved_user_std_calc_mode{g_user_std_calc_mode, 'g'};
    ValueSaver saved_old_std_calc_mode{g_old_std_calc_mode, '1'};

    const MainState result{execute('m')};

    EXPECT_EQ(MainState::NOTHING, result);
    EXPECT_EQ('m', m_context.key);
    EXPECT_FALSE(m_context.from_mandel);
    EXPECT_FALSE(m_context.more_keys);
    EXPECT_FALSE(m_context.stacked);
    EXPECT_FALSE(g_quick_calc);
    EXPECT_EQ('1', g_user_std_calc_mode);
}

TEST_F(TestMainMenuSwitch, userCalcModeResetOnQuickCalcImageNotComplete)
{
    ValueSaver saved_quick_calc{g_quick_calc, true};
    ValueSaver saved_calc_status{g_calc_status, CalcStatus::IN_PROGRESS};
    ValueSaver saved_user_std_calc_mode{g_user_std_calc_mode, 'g'};
    ValueSaver saved_old_std_calc_mode{g_old_std_calc_mode, '1'};

    const MainState result{execute('m')};

    EXPECT_EQ(MainState::NOTHING, result);
    EXPECT_EQ('m', m_context.key);
    EXPECT_FALSE(m_context.from_mandel);
    EXPECT_FALSE(m_context.more_keys);
    EXPECT_FALSE(m_context.stacked);
    EXPECT_TRUE(g_quick_calc);
    EXPECT_EQ('1', g_user_std_calc_mode);
}
