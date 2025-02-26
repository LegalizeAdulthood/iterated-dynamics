// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// used by solid guessing and by zoom panning
int ssg_block_size();
int solid_guess();

extern bool g_pixel_is_complete;        // flag to indicate no further calculations are required on this pixel
extern bool g_use_perturbation;         // select perturbation code
