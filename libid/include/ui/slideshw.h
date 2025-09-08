// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>

namespace id::ui
{

enum class SlidesMode
{
    OFF = 0,
    PLAY = 1,
    RECORD = 2
};

extern std::filesystem::path g_auto_name;
extern bool                  g_busy;
extern SlidesMode            g_slides;

int slide_show();
SlidesMode start_slide_show();
void stop_slide_show();
void record_show(int key);
int handle_special_keys(int ch);

} // namespace id::ui
