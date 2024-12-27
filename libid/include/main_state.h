// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum class main_state
{
    NOTHING = 0,
    RESTART,
    IMAGE_START,
    RESTORE_START,
    CONTINUE,
    RESUME_LOOP
};

struct MainContext
{
    int key{};          //
    bool from_mandel{}; //
    bool more_keys{};   // continuation variable
    bool stacked{};     // flag to indicate screen stacked
    bool resume{};      //
};
