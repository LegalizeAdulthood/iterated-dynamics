// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

class OrbitPlot;

}

namespace id::ui
{

bool drive_orbit_plot(engine::OrbitPlot &orbit_plot);
void reset_orbit_delay();

} // namespace id::ui
