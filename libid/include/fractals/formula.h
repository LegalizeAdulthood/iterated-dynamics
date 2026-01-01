// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string>

namespace id::fractals
{

extern std::filesystem::path g_formula_filename;
extern std::string           g_formula_name;

bool formula_per_image();
int formula_per_pixel();
int formula_orbit();
int bad_formula();

} // namespace id::fractals
