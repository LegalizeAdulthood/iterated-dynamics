// SPDX-License-Identifier: GPL-3.0-only
//
#include <misc/Driver.h>

#include "MockDriver.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::test
{

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

} // namespace id::test
