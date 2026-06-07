// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/wait_until.h>

#include "MockDriver.h"

#include <misc/Driver.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::engine;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::test
{

class TestWaitUntil : public Test
{
protected:
    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
};

TEST_F(TestWaitUntil, firstCallSchedulesNextWaitWithoutPollingKeyboard)
{
    reset_wait_until();

    wait_until(1000);
}

TEST_F(TestWaitUntil, pendingWaitPollsKeyboard)
{
    reset_wait_until();
    wait_until(1000);

    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(1));

    wait_until(1000);
}

TEST_F(TestWaitUntil, resetClearsPendingWait)
{
    reset_wait_until();
    wait_until(1000);
    reset_wait_until();

    wait_until(1000);
}

} // namespace id::test
