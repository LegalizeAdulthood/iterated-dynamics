// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum class main_state
{
    NOTHING = 0,
    RESTART,
    IMAGE_START,
    RESTORE_START,
    CONTINUE
};
