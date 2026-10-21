// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

namespace id::ui
{

extern bool g_good_mode;

int start_disk();
// TODO: removed later
// Slice 41.7 replaces this temporary storage setup bridge.
int pot_start_disk();
// TODO: removed later
// Slice 41.7 moves Targa-storage setup to the restore controller.
int targa_start_disk(std::FILE *file, int overhead);
// TODO: removed later
// Slice 41.7 removes the remaining storage setup bridge.
int common_start_disk(std::int64_t row_size, std::int64_t col_size, int colors);
void display_disk_storage_progress();
void dvid_status(int line, const char *msg);
inline void dvid_status(const int line, const std::string &msg)
{
    dvid_status(line, msg.c_str());
}

} // namespace id::ui
