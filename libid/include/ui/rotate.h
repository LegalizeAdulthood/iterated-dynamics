// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/spindac.h"

#include <config/port.h>

#include <string>

namespace id::ui
{

extern Byte                  g_dac_box[256][3];
extern bool                  g_got_real_dac;    // load_dac worked, really got a dac
extern Byte                  g_old_dac_box[256][3];
extern std::string           g_map_name;
extern bool                  g_map_set;

void rotate(engine::SpinDirection direction);
void save_palette();
bool load_palette();

} // namespace id::ui
