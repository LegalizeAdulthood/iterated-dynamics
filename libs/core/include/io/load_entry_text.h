// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>

namespace id::io
{

constexpr char SCROLL_MARKER = '\021';

void load_entry_text(
    std::FILE *entry_file,
    char *buf,
    int max_lines,
    int start_row,
    int start_col);

} // namespace id::io
