// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

int one_or_two_pass();

extern bool g_pixel_is_complete;  // flag to indicate no further calculations are required on this pixel
extern bool g_use_perturbation;   // select perturbation code
