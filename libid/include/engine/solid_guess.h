// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

extern int g_stop_pass; // stop at this guessing pass early

// used by solid guessing and by zoom panning
int ssg_block_size();
int solid_guess();

} // namespace id::engine
