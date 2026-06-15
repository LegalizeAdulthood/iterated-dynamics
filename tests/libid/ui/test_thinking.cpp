// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/thinking.h>

#include "MockDriver.h"

#include <engine/text_color.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>
#include <ui/text_screen.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>

using namespace id::engine;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::ui
{

class TestThinking : public Test
{
protected:
    void expect_initial_display(const char *message)
    {
        InSequence sequence;
        EXPECT_CALL(m_driver, stack_screen());
        EXPECT_CALL(m_driver, set_clear());
        EXPECT_CALL(m_driver, put_string(0, 0, _, _)).WillOnce(Invoke(update_text_col));
        EXPECT_CALL(m_driver, put_string(4, 10, C_GENERAL_HI, StrEq(message))).WillOnce(Invoke(update_text_col));
    }

    static void update_text_col(int, const int col, int, const char *text)
    {
        g_text_col = col + static_cast<int>(std::strlen(text));
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
};

TEST_F(TestThinking, displaysPaddedMessage)
{
    expect_initial_display("  Cellular thinking    ");

    EXPECT_FALSE(thinking("Cellular thinking"));

    EXPECT_CALL(m_driver, unstack_screen());
    thinking_end();
}

TEST_F(TestThinking, updatesSpinnerAfterDelay)
{
    expect_initial_display("  work    ");
    EXPECT_FALSE(thinking("work"));
    for (int i = 0; i < 99; ++i)
    {
        EXPECT_FALSE(thinking("work"));
    }

    EXPECT_CALL(m_driver, put_string(4, 17, C_GENERAL_HI, StrEq("-")));
    EXPECT_CALL(m_driver, hide_text_cursor());
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return('x'));

    EXPECT_TRUE(thinking("work"));

    EXPECT_CALL(m_driver, unstack_screen());
    thinking_end();
}

} // namespace id::ui
