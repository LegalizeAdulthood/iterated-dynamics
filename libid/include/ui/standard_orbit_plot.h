// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

class StandardFractal;

}

namespace id::ui
{

void drive_pending_orbit_plot(engine::StandardFractal &standard_fractal);
void reset_orbit_delay();

} // namespace id::ui
