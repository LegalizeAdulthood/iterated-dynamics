// SPDX-License-Identifier: GPL-3.0-only
//
// Thanks to Shirom Makkad fractaltodesktop@gmail.com

#include "engine/perturbation.h"

#include "engine/PertEngine.h"
#include "engine/convert_center_mag.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "math/biginit.h"

#include <config/port.h>

#include <stdexcept>
#include <string>

static PertEngine s_pert_engine;

PerturbationMode g_perturbation{PerturbationMode::AUTO};
double g_perturbation_tolerance{1e-6};
bool g_use_perturbation{}; // select perturbation code
int g_number_referrences{}; // number of references used

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
        BigFloat tmp_bf{alloc_stack(g_bf_length + 2)};
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

int perturbation_per_pixel()
{
    int result = s_pert_engine.perturbation_per_pixel(g_col, g_row, g_magnitude_limit);
    return result;
}

int perturbation_per_orbit()
{
    int status = s_pert_engine.calculate_orbit(g_col, g_row, g_color_iter);
    return status;
}

int get_number_references()
{
    return s_pert_engine.get_number_references();
}

bool is_perturbation()
{
    if (bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
    {
        if (g_perturbation == PerturbationMode::AUTO && g_bf_math != BFMathType::NONE)
        {
            g_use_perturbation = true;
        }
        else if (g_perturbation == PerturbationMode::YES)
        {
            g_use_perturbation = true;
        }
        else
        {
            g_use_perturbation = false;
        }
    }
    else
    {
        g_use_perturbation = false;
    }
    return g_use_perturbation;
}
