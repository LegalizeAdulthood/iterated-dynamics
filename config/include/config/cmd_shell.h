// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

namespace id::config
{

using TimeoutCallback = void();

std::string cmd_shell_command();
bool cmd_shell(TimeoutCallback *timeout);
int get_cmd_shell_error();

} // namespace id::config
