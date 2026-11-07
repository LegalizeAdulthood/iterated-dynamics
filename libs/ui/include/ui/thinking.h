// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string_view>

namespace id::ui
{

bool thinking(std::string_view msg);
bool thinking_active();

void thinking_end();

} // namespace id::ui
