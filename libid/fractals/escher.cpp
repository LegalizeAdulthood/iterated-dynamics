// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/escher.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "math/cmplx.h"
#include "math/sqr.h"

using namespace id::engine;
using namespace id::math;

namespace id::fractals
{

int escher_orbit() // Science of Fractal Images pp. 185, 187
{
    DComplex old_test;
    DComplex test_sqr;
    double test_size = 0.0;
    long test_iter = 0;

    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y; // standard Julia with C == (0.0, 0.0i)
    g_new_z.y = 2.0 * g_old_z.x * g_old_z.y;
    old_test.x = g_new_z.x * 15.0;    // scale it
    old_test.y = g_new_z.y * 15.0;
    test_sqr.x = sqr(old_test.x);  // set up to test with user-specified ...
    test_sqr.y = sqr(old_test.y);  //    ... Julia as the target set
    while (test_size <= g_magnitude_limit && test_iter < g_max_iterations) // nested Julia loop
    {
        DComplex new_test;
        new_test.x = test_sqr.x - test_sqr.y + g_params[0];
        new_test.y = 2.0 * old_test.x * old_test.y + g_params[1];
        test_sqr.x = sqr(new_test.x);
        test_sqr.y = sqr(new_test.y);
        test_size = test_sqr.x + test_sqr.y;
        old_test = new_test;
        test_iter++;
    }
    if (test_size > g_magnitude_limit)
    {
        return g_bailout_float(); // point not in target set
    }
    // make distinct level sets if point stayed in target set
    g_color_iter = 3L * g_color_iter % 255L + 1L;
    return 1;
}

} // namespace id::fractals
