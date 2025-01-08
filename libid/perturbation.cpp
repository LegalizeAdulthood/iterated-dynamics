// SPDX-License-Identifier: GPL-3.0-only
//
// Thanks to Shirom Makkad fractaltodesktop@gmail.com

#include "perturbation.h"

#include "PertEngine.h"
#include "convert_center_mag.h"
#include "id_data.h"
#include "math/biginit.h"
#include "port.h"

#include <stdexcept>
#include <string>

static PertEngine s_pert_engine;

bool perturbation()
{
    BigStackSaver saved;
    double mandel_width; // width of display
    DComplex center{};
    BFComplex center_bf{};
    double x_mag_factor{};
    double rotation{};
    double skew{};
    LDouble magnification{};
    if (g_bf_math != BFMathType::NONE)
    {
        center_bf.x = alloc_stack(g_bf_length + 2);
        center_bf.y = alloc_stack(g_bf_length + 2);
        cvt_center_mag_bf(center_bf.x, center_bf.y, magnification, x_mag_factor, rotation, skew);
        neg_bf(center_bf.y, center_bf.y);
    }
    else
    {
        LDouble magnification_ld;
        cvt_center_mag(center.x, center.y, magnification_ld, x_mag_factor, rotation, skew);
        center.y = -center.y;
    }

    if (g_bf_math == BFMathType::NONE)
    {
        mandel_width = g_y_max - g_y_min;
    }
    else
    {
        bf_t tmp_bf{alloc_stack(g_bf_length + 2)};
        sub_bf(tmp_bf, g_bf_y_max, g_bf_y_min);
        mandel_width = bf_to_float(tmp_bf);
    }

    s_pert_engine.initialize_frame(center_bf, {center.x, center.y}, mandel_width / 2.0);
    if (const int result = s_pert_engine.calculate_one_frame(); result < 0)
    {
        throw std::runtime_error("Failed to initialize perturbation engine (" + std::to_string(result) + ")");
    }
    g_calc_status = CalcStatus::COMPLETED;
    return false;
}
