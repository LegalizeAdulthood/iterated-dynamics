// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <cstdio>
#include <filesystem>
#include <string>

namespace id::ui
{

void set_make_mig_script_executable(const char *path);
std::filesystem::path make_mig_script_filename();
void write_make_mig_script_header(std::FILE *script_file);
void write_make_mig_script_piece(
    std::FILE *script_file, const std::filesystem::path &parameter_file, const std::string &entry_name);
void write_make_mig_script_finish(std::FILE *script_file, int x_multiple, int y_multiple);
void finish_make_mig_script(const std::filesystem::path &script_path);

} // namespace id::ui
