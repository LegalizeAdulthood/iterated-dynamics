// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/input_field.h>

#include "MockDriver.h"

#include <misc/ValueSaver.h>
#include <ui/id_keys.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
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

TEST_F(TestInputField, displaysFieldsWiderThanScratchBuffer)
{
    std::array<char, 96> field{};
    const std::string expected(90, 'x');
    expected.copy(field.data(), expected.size());

    InSequence sequence;
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(expected)));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(0, input_field(InputFieldFlags::NONE, 7, field.data(), 90, 90, 4, 10, nullptr));
}

TEST_F(TestInputField, f5RestoresFieldsWiderThanScratchBuffer)
{
    std::array<char, 96> field{};
    const std::string expected(90, 'a');
    expected.copy(field.data(), expected.size());
    std::string changed{"b"};
    changed.resize(90, ' ');

    InSequence sequence;
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(expected)));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return('b'));
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(changed)));
    EXPECT_CALL(m_driver, key_cursor(4, 11)).WillOnce(Return(ID_KEY_F5));
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(expected)));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(0, input_field(InputFieldFlags::NONE, 7, field.data(), 90, 90, 4, 10, nullptr));
    EXPECT_THAT(field.data(), StrEq(expected));
}

TEST_F(TestInputField, piExpansionWorksInLongField)
{
    std::array<char, 96> field{};
    const std::string initial(40, ' ');
    std::string expected{"3.141593"};
    expected.resize(40, ' ');

    InSequence sequence;
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(initial)));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return('p'));
    EXPECT_CALL(m_driver, put_string(4, 10, 7, StrEq(expected)));
    EXPECT_CALL(m_driver, key_cursor(4, 10)).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(0, input_field(InputFieldFlags::NUMERIC, 7, field.data(), 40, 40, 4, 10, nullptr));
    EXPECT_THAT(field.data(), StrEq("3.141593"));
}

} // namespace id::test
