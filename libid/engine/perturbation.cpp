// SPDX-License-Identifier: GPL-3.0-only
//
// Thanks to Shirom Makkad fractaltodesktop@gmail.com

#include "engine/perturbation.h"

#include "engine/PertEngine.h"
#include "engine/convert_center_mag.h"
#include "engine/id_data.h"
#include "math/biginit.h"

#include <config/port.h>

#include <stdexcept>
#include <string>
#include <math.h>

static PertEngine s_pert_engine;

PerturbationMode g_perturbation{PerturbationMode::AUTO};
double g_perturbation_tolerance{1e-6};

extern DComplex g_old_z;
extern DComplex g_new_z;
extern int g_row;
extern int g_col;
extern long g_color_iter;
extern double g_magnitude_limit;

bool perturbation()
{
    int saved = save_stack();
    double mandel_width{}; // width of display
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
        if (isnan(g_x_max) || isnan(g_x_min))
        {
            throw std::runtime_error("Failed to calculate x_max or x_min ");
        }
        if (isnan(g_y_max) || isnan(g_y_min))
        {
            throw std::runtime_error("Failed to calculate y_max or y_min ");
        }

        cvt_center_mag(center.x, center.y, magnification_ld, x_mag_factor, rotation, skew);
        if (isnan(center.x) || isnan(center.y))
        {
            throw std::runtime_error("Failed to calculate perturbation center ");
        }
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
    restore_stack(saved);
    perturbation_per_image();
//    g_calc_status = CalcStatus::COMPLETED;
    return false;
}

int perturbation_per_orbit()
{
    int status = s_pert_engine.calculate_orbit(g_col, g_row, g_color_iter);
    return status;
}

int perturbation_per_pixel()
{
    int result = s_pert_engine.perturbation_per_pixel(g_col, g_row, g_magnitude_limit);
    return result;
}

bool perturbation_per_image()
{
    if (const int result = s_pert_engine.calculate_one_frame(); result < 0)
    {
        throw std::runtime_error("Failed to initialize perturbation engine (" + std::to_string(result) + ")");
    }

    s_pert_engine.set_glitch_points_count(0);
    return 0;
}

bool is_pixel_finished(int x, int y)
{
    return (s_pert_engine.is_pixel_complete(x, y));
}

long get_glitch_point_count()
{
    return (s_pert_engine.get_glitch_point_count());
}

int calculate_reference()
{
    return (s_pert_engine.calculate_reference());
}

void cleanup_perturbation()
{
    s_pert_engine.cleanup();
}
