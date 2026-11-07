// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <NonStandardFractalTest.h>

#include <ui/KeyboardHandler.h>

namespace id::test
{

class NonStandardControllerState
{
public:
    ~NonStandardControllerState()
    {
        ui::reset_calc_interrupted();
    }
};

} // namespace id::test
