// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/full_screen_prompt.h>

#include "MockDriver.h"

#include <misc/ValueSaver.h>
#include <ui/id_keys.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestFullScreenPrompt : public Test
{
protected:
    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
};

TEST_F(TestFullScreenPrompt, truncatesStringBufferWriteBackToDeclaredLength)
{
    char field[5]{"abcd"};
    const char *prompts[1]{"Field"};
    FullScreenValues value{};
    value.type = 0x100 + 3;
    value.uval.sbuf = field;

    EXPECT_CALL(m_driver, set_clear());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, hide_text_cursor());
    EXPECT_CALL(m_driver, key_cursor(_, _)).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(ID_KEY_ENTER, full_screen_prompt("Title", 1, prompts, &value, 0, nullptr));
    EXPECT_THAT(field, StrEq("abc"));
}

} // namespace id::test
