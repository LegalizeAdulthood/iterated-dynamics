// SPDX-License-Identifier: GPL-3.0-only
//
#include "MockDriver.h"

#include <ui/KeyboardInput.h>

#include <misc/Driver.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestKeyboardInput : public Test
{
protected:
    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
};

TEST_F(TestKeyboardInput, pendingKeyForwardsToDriver)
{
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(17));

    EXPECT_EQ(17, g_kb_input->pending_key());
}

TEST_F(TestKeyboardInput, readKeyForwardsToDriver)
{
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(23));

    EXPECT_EQ(23, g_kb_input->read_key());
}

TEST_F(TestKeyboardInput, waitForKeyForwardsToDriver)
{
    EXPECT_CALL(m_driver, wait_key_pressed(true)).WillOnce(Return(31));

    EXPECT_EQ(31, g_kb_input->wait_for_key(true));
}

TEST_F(TestKeyboardInput, pushKeyForwardsToDriver)
{
    EXPECT_CALL(m_driver, unget_key(47));

    g_kb_input->push_key(47);
}

} // namespace id::test
