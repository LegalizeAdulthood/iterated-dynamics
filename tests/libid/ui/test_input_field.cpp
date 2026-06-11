// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/input_field.h>

#include "MockDriver.h"

#include <misc/ValueSaver.h>
#include <ui/id_keys.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

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
    std::string field{"abcdef"};

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
    std::string field{"abcdef"};

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

TEST_F(TestInputField, displaysFieldsWiderThanScratchBuffer)
{
    std::string field(90, 'x');
    const std::string expected(90, 'x');

    InSequence sequence;
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(expected)));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(0, input_field(InputFieldFlags::NONE, 7, field, 90, 90, 4, 10, nullptr));
}

TEST_F(TestInputField, f5RestoresFieldsWiderThanScratchBuffer)
{
    std::string field(90, 'a');
    const std::string expected(90, 'a');
    std::string changed{"b"};
    changed.resize(90, ' ');

    InSequence sequence;
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(expected)));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return('b'));
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(changed)));
    EXPECT_CALL(m_driver, key_cursor(4, 11)).WillOnce(Return(ID_KEY_F5));
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(expected)));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(0, input_field(InputFieldFlags::NONE, 7, field, 90, 90, 4, 10, nullptr));
    EXPECT_THAT(field, Eq(expected));
}

TEST_F(TestInputField, piExpansionWorksInLongField)
{
    std::string field;
    const std::string initial(40, ' ');
    std::string expected{"3.141593"};
    expected.resize(40, ' ');

    InSequence sequence;
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(initial)));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return('p'));
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(expected)));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(0, input_field(InputFieldFlags::NUMERIC, 7, field, 40, 40, 4, 10, nullptr));
    EXPECT_THAT(field, Eq("3.141593"));
}

} // namespace id::test
