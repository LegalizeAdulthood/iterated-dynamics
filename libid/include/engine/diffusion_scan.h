// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id
{

extern unsigned int          g_diffusion_bits;
extern unsigned long         g_diffusion_counter;
extern unsigned long         g_diffusion_limit;

int diffusion_scan();

} // namespace id
