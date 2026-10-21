// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "io/disk_storage.h"

#include <cstdint>
#include <string>
#include <vector>

namespace id::ui
{

extern bool g_good_mode;

int start_disk();
bool prepare_disk_storage(std::int64_t row_size, std::int64_t col_size, int colors,
    const std::vector<Byte> &header = {}, const std::filesystem::path &backing_path = {});
bool prepare_potential_storage();
bool prepare_targa_storage(const io::TargaStorageRequest &request);
void display_disk_storage_progress();
void dvid_status(int line, const char *msg);
inline void dvid_status(const int line, const std::string &msg)
{
    dvid_status(line, msg.c_str());
}

} // namespace id::ui
