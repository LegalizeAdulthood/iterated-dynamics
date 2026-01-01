// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string>

namespace id::fractals
{

extern std::filesystem::path g_formula_filename;
extern std::string           g_formula_name;
extern bool                  g_frm_is_mandelbrot;
extern bool                  g_frm_uses_ismand;
extern bool                  g_frm_uses_p1;
extern bool                  g_frm_uses_p2;
extern bool                  g_frm_uses_p3;
extern bool                  g_frm_uses_p4;
extern bool                  g_frm_uses_p5;

bool formula_per_image();
int formula_per_pixel();
int formula_orbit();
int bad_formula();

} // namespace id::fractals
