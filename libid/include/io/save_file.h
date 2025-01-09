// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>
#include <filesystem>
#include <string>

std::filesystem::path get_save_name(const std::string &name);
std::FILE *open_save_file(const std::string &name, const std::string &mode);
