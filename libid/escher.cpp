#include "escher.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "fractals.h"
#include "id_data.h"
#include "sqr.h"

int EscherfpFractal() // Science of Fractal Images pp. 185, 187
{
    DComplex oldtest, newtest, testsqr;
    double testsize = 0.0;
    long testiter = 0;

    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y; // standard Julia with C == (0.0, 0.0i)
    g_new_z.y = 2.0 * g_old_z.x * g_old_z.y;
    oldtest.x = g_new_z.x * 15.0;    // scale it
    oldtest.y = g_new_z.y * 15.0;
    testsqr.x = sqr(oldtest.x);  // set up to test with user-specified ...
    testsqr.y = sqr(oldtest.y);  //    ... Julia as the target set
    while (testsize <= g_magnitude_limit && testiter < g_max_iterations) // nested Julia loop
    {
        newtest.x = testsqr.x - testsqr.y + g_params[0];
        newtest.y = 2.0 * oldtest.x * oldtest.y + g_params[1];
        testsqr.x = sqr(newtest.x);
        testsqr.y = sqr(newtest.y);
        testsize = testsqr.x + testsqr.y;
        oldtest = newtest;
        testiter++;
    }
    if (testsize > g_magnitude_limit)
    {
        return g_bailout_float(); // point not in target set
    }
    else   // make distinct level sets if point stayed in target set
    {
        g_color_iter = ((3L * g_color_iter) % 255L) + 1L;
        return 1;
    }
}
