// SPDX-License-Identifier: GPL-3.0-only
//
// Thanks to Shirom Makkad fractaltodesktop@gmail.com

#include "PertEngine.h"
#include "biginit.h"
#include "cmdfiles.h"
#include "convert_center_mag.h"
#include "fractalp.h"
#include "id_data.h"

static PertEngine s_pert_engine;

/**************************************************************************
        The Perturbation engine
**************************************************************************/
static int perturbation(int subtype)
{
    // power
    int degree = (int) g_params[2];
    if (s_pert_engine.calculate_one_frame(g_magnitude_limit, degree, g_inside_color, g_outside_color,
            g_biomorph, subtype, g_plot, potential) < 0)
    {
        return -1;
    }
    return 0;
}

// Initialize perturbation engine
bool init_perturbation(int subtype)
{
    double mandel_width; // width of display
    double x_center{};
    double y_center{};
    double x_mag_factor{};
    double rotation{};
    double skew{};
    bf_t x_center_bf{};
    bf_t y_center_bf{};
    bf_t tmp_bf{};
    int saved{};
    LDBL magnification{};
    if (g_bf_math != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
    {
        saved = save_stack();
        x_center_bf = alloc_stack(g_bf_length + 2);
        y_center_bf = alloc_stack(g_bf_length + 2);
        tmp_bf = alloc_stack(g_bf_length + 2);
        cvtcentermagbf(x_center_bf, y_center_bf, &magnification, &x_mag_factor, &rotation, &skew);
        neg_bf(y_center_bf, y_center_bf);
        x_center = 0.0;
        y_center = 0.0;
    }
    else
    {
        LDBL magnification_ld;
        cvtcentermag(&x_center, &y_center, &magnification_ld, &x_mag_factor, &rotation, &skew);
        y_center = -y_center;
    }

    if (g_bf_math == bf_math_type::NONE)
    {
        mandel_width = g_y_max - g_y_min;
    }
    else
    {
        sub_bf(tmp_bf, g_bf_y_max, g_bf_y_min);
        mandel_width = bftofloat(tmp_bf);
    }

    s_pert_engine.initialize_frame(x_center_bf, y_center_bf, x_center, y_center, mandel_width / 2.0);
    perturbation(subtype);
    if (g_bf_math != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
    {
        restore_stack(saved);
    }
    g_calc_status = calc_status_value::COMPLETED;
    return false;
}
