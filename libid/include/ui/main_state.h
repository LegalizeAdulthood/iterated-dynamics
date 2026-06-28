// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::ui
{

enum class MainState
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
    bool orbits_window_active{};
    bool orbits_window_key_pending{};
    bool stacked{};     // flag to indicate screen stacked
    bool resume{};      //
};

} // namespace id::ui
