// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <array>
#include <string>
#include <string_view>

namespace id::engine
{

constexpr int MAX_COMMENT_LEN{57};

extern std::array<std::string, 4> g_par_comment;
extern std::string g_command_comment[4];

void clear_command_comments();
void init_comments();
void parse_comments(std::string_view value);

} // namespace id::engine
