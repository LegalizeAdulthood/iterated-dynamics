// SPDX-License-Identifier: GPL-3.0-only
//
//      Header file for L-system code.
//
#pragma once

#include <config/port.h>

#include <filesystem>
#include <string>

namespace id::fractals
{

extern std::filesystem::path g_l_system_filename;
extern std::string           g_l_system_name;
extern char                  g_max_angle;

int lsystem_type();
bool lsystem_load();

} // namespace id::fractals
