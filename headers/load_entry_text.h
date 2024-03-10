#pragma once

#include <cstdio>

void load_entry_text(
    std::FILE *entfile,
    char *buf,
    int maxlines,
    int startrow,
    int startcol);
