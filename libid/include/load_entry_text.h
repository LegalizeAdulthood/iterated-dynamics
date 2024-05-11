#pragma once

#include <cstdio>

void load_entry_text(
    std::FILE *entry_file,
    char *buf,
    int max_lines,
    int start_row,
    int start_col);
