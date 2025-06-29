// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>

extern std::filesystem::path g_save_dir;

std::filesystem::path get_executable_dir();
std::filesystem::path get_documents_dir();
