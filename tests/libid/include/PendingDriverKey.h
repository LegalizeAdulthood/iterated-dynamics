// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <MockDriver.h>

namespace id::test
{

void expect_pending_driver_key(misc::test::MockDriver &driver, int &key);

} // namespace id::test
