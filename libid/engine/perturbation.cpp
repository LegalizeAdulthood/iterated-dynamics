// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Thanks to Shirom Makkad fractaltodesktop@gmail.com

#include "engine/convert_center_mag.h"
#include "engine/fractals.h"
#include "engine/ImageRegion.h"
#include "engine/StandardFractal.h"
#include "engine/UserData.h"
#include "fractals/fractype.h"
#include "math/biginit.h"

#include <config/port.h>

#include <algorithm>

using namespace id::math;

namespace id::engine
{

void StandardFractal::start_perturbation_frame()
{
    m_pert_engine.set_glitch_tolerance(g_user.perturbation_tolerance);

    if (fractals::g_fractal_type == fractals::FractalType::MANDEL_Z_POWER)
    {
        constexpr int MAX_POWER{28};
        g_c_exponent = std::clamp(g_c_exponent, 2, MAX_POWER);
    }

    double half_width{};
    double half_height{};
    double x_mag_factor{};
    double rotation{};
    double skew{};
    if (g_bf_math != BFMathType::NONE)
    {
        {
            BigStackSaver saved;
            BigFloat tmp_bf{alloc_stack(g_bf_length + 2)};
            sub_bf(tmp_bf, g_bf_x_max, g_bf_x_min);
            half_width = bf_to_float(tmp_bf) / 2.0;
            sub_bf(tmp_bf, g_bf_y_max, g_bf_y_min);
            half_height = bf_to_float(tmp_bf) / 2.0;
        }

        BFComplex center_bf{m_pert_engine.initialize_frame_bf(half_width, half_height)};
        LDouble magnification_ld{};
        cvt_center_mag_bf(center_bf.x, center_bf.y, magnification_ld, x_mag_factor, rotation, skew);
        if (fractals::g_fractal_type == fractals::FractalType::BURNING_SHIP)
        {
            neg_bf(center_bf.y, center_bf.y);
        }
        m_pert_engine.initialize_pixel_strategy();
    }
    else
    {
        DComplex center{};
        LDouble magnification_ld;
        cvt_center_mag(center.x, center.y, magnification_ld, x_mag_factor, rotation, skew);
        if (fractals::g_fractal_type == fractals::FractalType::BURNING_SHIP)
        {
            center.y = -center.y;
        }
        half_width = (g_image_region.m_max.x - g_image_region.m_min.x) / 2.0;
        half_height = (g_image_region.m_max.y - g_image_region.m_min.y) / 2.0;
        m_pert_engine.initialize_frame({center.x, center.y}, half_width, half_height);
        m_pert_engine.initialize_pixel_strategy();
    }
    m_perturbation_active = true;
}

} // namespace id::engine
