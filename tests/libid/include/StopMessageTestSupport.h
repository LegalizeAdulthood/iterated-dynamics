// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <MockDriver.h>

namespace id::test
{

void expect_stop_messages(misc::test::MockDriver &driver, int count = 1);

} // namespace id::test
