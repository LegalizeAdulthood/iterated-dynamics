// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "StopMessageTestSupport.h"

using namespace testing;

namespace id::test
{

void expect_stop_messages(misc::test::MockDriver &driver, const int count)
{
    EXPECT_CALL(driver, stack_screen()).Times(count);
    EXPECT_CALL(driver, unstack_screen()).Times(count);
    EXPECT_CALL(driver, move_cursor(_, _)).Times(count);
    EXPECT_CALL(driver, hide_text_cursor()).Times(count);
    EXPECT_CALL(driver, set_attr(_, _, _, _)).Times(count);
    EXPECT_CALL(driver, buzzer(misc::Buzzer::PROBLEM)).Times(count);
    EXPECT_CALL(driver, put_string(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(driver, key_pressed()).WillRepeatedly(Return(0));
}

} // namespace id::test
