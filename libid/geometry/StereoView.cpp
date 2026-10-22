// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "geometry/StereoView.h"

#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/VideoInfo.h"
#include "engine/Viewport.h"
#include "fractals/lorenz.h"
#include "geometry/line3d.h"

#include <stdexcept>

using namespace id::engine;
using namespace id::fractals;

namespace id::geometry
{

StereoView::~StereoView()
{
    finish();
}

void StereoView::start(const bool combine_eyes)
{
    if (m_started)
    {
        throw std::logic_error("The stereo view has already started");
    }
    m_glasses = g_glasses_type;
    m_eye = m_glasses != GlassesType::NONE ? StereoImage::RED : StereoImage::NONE;
    m_both_eyes = combine_eyes && alternating_or_superimpose(m_glasses);
    m_started = true;
    setup_plot();
}

bool StereoView::both_eyes() const
{
    return m_started && m_both_eyes;
}

bool StereoView::next_view_required() const
{
    return m_started && m_eye == StereoImage::RED && m_glasses != GlassesType::NONE && !m_both_eyes &&
        g_display_3d != Display3DMode::NONE;
}

void StereoView::next_view()
{
    if (!next_view_required())
    {
        throw std::logic_error("No next stereo view is required");
    }
    m_eye = StereoImage::BLUE;
    setup_plot();
}

void StereoView::setup_plot()
{
    g_which_image = m_eye;
    plot_setup();
    g_plot = g_standard_plot;
}

void StereoView::finish()
{
    if (!m_started)
    {
        return;
    }
    if (m_glasses == GlassesType::STEREO_PAIR && g_screen_x_dots >= 2 * g_logical_screen.x_dots)
    {
        // Save stereo pairs as the full screen, not just the last eye's viewport.
        g_logical_screen.x_offset = 0;
        g_logical_screen.y_offset = 0;
        g_logical_screen.x_dots = g_screen_x_dots;
        g_logical_screen.y_dots = g_screen_y_dots;
        g_viewport.enabled = false;
    }
    m_started = false;
}

} // namespace id::geometry
