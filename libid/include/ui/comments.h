// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <ctime>
#include <functional>
#include <string>
#include <string_view>

namespace id::ui
{

extern std::function<std::string()> g_get_cpu_id;

const std::string &expand_command_comment(int i, std::time_t local_time);
const std::string &expand_command_comment(int i);

} // namespace id::ui
