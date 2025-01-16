// SPDX-License-Identifier: GPL-3.0-only
//
#include <misc/drivers.h>

#include "MockDriver.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

TEST(TestDriver, loadClose)
{
    MockDriver gdi;
    EXPECT_CALL(gdi, init(nullptr, nullptr)).WillOnce(Return(true));
    EXPECT_CALL(gdi, terminate());

    load_driver(&gdi, nullptr, nullptr);

    EXPECT_EQ(&gdi, g_driver);

    close_drivers();
    EXPECT_EQ(nullptr, g_driver);
}
