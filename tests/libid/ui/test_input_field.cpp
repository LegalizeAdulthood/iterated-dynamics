// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/input_field.h>

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

class TestInputField : public Test
{
protected:
    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
};

TEST_F(TestInputField, scrollsRightWhenCursorMovesPastDisplayLength)
{
    char field[7]{"abcdef"};

    InSequence sequence;
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq("abc")));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return(ID_KEY_RIGHT_ARROW));
    EXPECT_CALL(m_driver, key_cursor(4, 11)).WillOnce(Return(ID_KEY_RIGHT_ARROW));
    EXPECT_CALL(m_driver, key_cursor(4, 12)).WillOnce(Return(ID_KEY_RIGHT_ARROW));
    EXPECT_CALL(m_driver, key_cursor(4, 13)).WillOnce(Return(ID_KEY_RIGHT_ARROW));
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq("bcd")));
    EXPECT_CALL(m_driver, key_cursor(4, 13)).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(0, input_field(InputFieldFlags::NONE, 7, field, 6, 3, 4, 10, nullptr));
}

TEST_F(TestInputField, scrollsLeftWhenCursorMovesBeforeDisplayWindow)
{
    char field[7]{"abcdef"};

    InSequence sequence;
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq("abc")));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return(ID_KEY_END));
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq("def")));
    EXPECT_CALL(m_driver, key_cursor(4, 13)).WillOnce(Return(ID_KEY_LEFT_ARROW));
    EXPECT_CALL(m_driver, key_cursor(4, 12)).WillOnce(Return(ID_KEY_LEFT_ARROW));
    EXPECT_CALL(m_driver, key_cursor(4, 11)).WillOnce(Return(ID_KEY_LEFT_ARROW));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return(ID_KEY_LEFT_ARROW));
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq("cde")));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(0, input_field(InputFieldFlags::NONE, 7, field, 6, 3, 4, 10, nullptr));
}

} // namespace id::test
