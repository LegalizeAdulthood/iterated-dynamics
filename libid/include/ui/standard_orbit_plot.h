// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

class OrbitPlot;

}

namespace id::ui
{

unsigned long display_orbit_delay_units(unsigned long delay_100us);
bool drive_orbit_plot(engine::OrbitPlot &orbit_plot);
void mute_orbit_display();
void pace_orbit_display();
void reset_orbit_delay();
void scrub_orbit_display();

} // namespace id::ui
