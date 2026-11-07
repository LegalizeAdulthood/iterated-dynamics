// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Thanks to Shirom Makkad fractaltodesktop@gmail.com

#include <engine/ImageRegion.h>
#include <engine/StandardFractal.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/biginit.h>

#include <complex>

using namespace id::math;

namespace id::engine
{

void StandardFractal::start_perturbation_frame()
{
    m_pert_engine.set_glitch_tolerance(g_user.perturbation_tolerance);

    std::complex<double> half_horizontal;
    std::complex<double> half_vertical;
    if (g_bf_math != BFMathType::NONE)
    {
        m_pert_engine.initialize_frame_bf({g_bf_x_min, g_bf_y_min}, {g_bf_x_max, g_bf_y_max}, {g_bf_x_3rd, g_bf_y_3rd});
        m_pert_engine.initialize_pixel_strategy();
    }
    else
    {
        DComplex center{g_image_region.center()};
        if (fractals::g_fractal_type == fractals::FractalType::BURNING_SHIP)
        {
            center.y = -center.y;
        }
        half_horizontal = {(g_image_region.m_max.x - g_image_region.m_3rd.x) / 2.0,
            (g_image_region.m_min.y - g_image_region.m_3rd.y) / 2.0};
        half_vertical = {(g_image_region.m_min.x - g_image_region.m_3rd.x) / 2.0,
            (g_image_region.m_max.y - g_image_region.m_3rd.y) / 2.0};
        m_pert_engine.initialize_frame({center.x, center.y}, half_horizontal, half_vertical);
        m_pert_engine.initialize_pixel_strategy();
    }
    m_perturbation_active = true;
}

} // namespace id::engine
