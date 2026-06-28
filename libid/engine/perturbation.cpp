// SPDX-License-Identifier: GPL-3.0-only
//
// Thanks to Shirom Makkad fractaltodesktop@gmail.com

#include "engine/convert_center_mag.h"
#include "engine/fractals.h"
#include "engine/ImageRegion.h"
#include "engine/StandardFractal.h"
#include "fractals/fractype.h"
#include "math/biginit.h"

#include <config/port.h>

#include <algorithm>

using namespace id::math;

namespace id::engine
{

void StandardFractal::start_perturbation_frame()
{
    if (fractals::g_fractal_type == fractals::FractalType::MANDEL_Z_POWER)
    {
        constexpr int MAX_POWER{28};
        g_c_exponent = std::clamp(g_c_exponent, 2, MAX_POWER);
    }

    double mandel_width{}; // width of display
    double x_mag_factor{};
    double rotation{};
    double skew{};
    if (g_bf_math != BFMathType::NONE)
    {
        {
            BigStackSaver saved;
            BigFloat tmp_bf{alloc_stack(g_bf_length + 2)};
            sub_bf(tmp_bf, g_bf_y_max, g_bf_y_min);
            mandel_width = bf_to_float(tmp_bf);
        }

        BFComplex center_bf{m_pert_engine.initialize_frame_bf(mandel_width / 2.0)};
        LDouble magnification_ld{};
        cvt_center_mag_bf(center_bf.x, center_bf.y, magnification_ld, x_mag_factor, rotation, skew);
        neg_bf(center_bf.y, center_bf.y);
    }
    else
    {
        DComplex center{};
        LDouble magnification_ld;
        cvt_center_mag(center.x, center.y, magnification_ld, x_mag_factor, rotation, skew);
        center.y = -center.y;
        mandel_width = g_image_region.height();
        m_pert_engine.initialize_frame({center.x, center.y}, mandel_width / 2.0);
    }
    m_perturbation_active = true;
}

} // namespace id::engine
