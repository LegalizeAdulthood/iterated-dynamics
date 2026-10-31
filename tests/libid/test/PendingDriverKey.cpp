// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <PendingDriverKey.h>

namespace id::test
{

void expect_pending_driver_key(misc::test::MockDriver &driver, int &key)
{
    EXPECT_CALL(driver, key_pressed()).WillRepeatedly(testing::Invoke([&key] { return key; }));
    EXPECT_CALL(driver, get_key())
        .WillRepeatedly(testing::Invoke(
            [&key]
            {
                const int result{key};
                key = 0;
                return result;
            }));
}

} // namespace id::test
