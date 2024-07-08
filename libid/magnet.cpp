#include "magnet.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "fractals.h"

#include <cfloat>

/*
**  pre-calculated values for fractal types Magnet2M & Magnet2J
*/
static DComplex s_t_cm1{};     // 3 * (floatparm - 1)
static DComplex s_t_cm2{};     // 3 * (floatparm - 2)
static DComplex s_t_cm1_cm2{}; // (floatparm - 1) * (floatparm - 2)

/*
**  details of finite attractors (required for Magnet Fractals)
**  (can also be used in "coloring in" the lakes of Julia types)
*/

void FloatPreCalcMagnet2() // precalculation for Magnet2 (M & J) for speed
{
    s_t_cm1.x = g_float_param->x - 1.0;
    s_t_cm1.y = g_float_param->y;
    s_t_cm2.x = g_float_param->x - 2.0;
    s_t_cm2.y = g_float_param->y;
    s_t_cm1_cm2.x = (s_t_cm1.x * s_t_cm2.x) - (s_t_cm1.y * s_t_cm2.y);
    s_t_cm1_cm2.y = (s_t_cm1.x * s_t_cm2.y) + (s_t_cm1.y * s_t_cm2.x);
    s_t_cm1.x += s_t_cm1.x + s_t_cm1.x;
    s_t_cm1.y += s_t_cm1.y + s_t_cm1.y;
    s_t_cm2.x += s_t_cm2.x + s_t_cm2.x;
    s_t_cm2.y += s_t_cm2.y + s_t_cm2.y;
}

//    Z = ((Z**2 + C - 1)/(2Z + C - 2))**2
int Magnet1Fractal()
{
    //  In "Beauty of Fractals", code by Kev Allen.
    DComplex top;
    DComplex bot;
    DComplex tmp;
    double div;

    top.x = g_temp_sqr_x - g_temp_sqr_y + g_float_param->x - 1; // top = Z**2+C-1
    top.y = g_old_z.x * g_old_z.y;
    top.y = top.y + top.y + g_float_param->y;

    bot.x = g_old_z.x + g_old_z.x + g_float_param->x - 2;       // bot = 2*Z+C-2
    bot.y = g_old_z.y + g_old_z.y + g_float_param->y;

    div = bot.x*bot.x + bot.y*bot.y;                // tmp = top/bot
    if (div < FLT_MIN)
    {
        return 1;
    }
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    g_new_z.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);      // Z = tmp**2
    g_new_z.y = tmp.x * tmp.y;
    g_new_z.y += g_new_z.y;

    return g_bailout_float();
}

// Z = ((Z**3 + 3(C-1)Z + (C-1)(C-2)  ) /
//       (3Z**2 + 3(C-2)Z + (C-1)(C-2)+1) )**2
int Magnet2Fractal()
{
    //   In "Beauty of Fractals", code by Kev Allen.
    DComplex top;
    DComplex bot;
    DComplex tmp;
    double div;

    top.x = g_old_z.x * (g_temp_sqr_x-g_temp_sqr_y-g_temp_sqr_y-g_temp_sqr_y + s_t_cm1.x)
            - g_old_z.y * s_t_cm1.y + s_t_cm1_cm2.x;
    top.y = g_old_z.y * (g_temp_sqr_x+g_temp_sqr_x+g_temp_sqr_x-g_temp_sqr_y + s_t_cm1.x)
            + g_old_z.x * s_t_cm1.y + s_t_cm1_cm2.y;

    bot.x = g_temp_sqr_x - g_temp_sqr_y;
    bot.x = bot.x + bot.x + bot.x
            + g_old_z.x * s_t_cm2.x - g_old_z.y * s_t_cm2.y
            + s_t_cm1_cm2.x + 1.0;
    bot.y = g_old_z.x * g_old_z.y;
    bot.y += bot.y;
    bot.y = bot.y + bot.y + bot.y
            + g_old_z.x * s_t_cm2.y + g_old_z.y * s_t_cm2.x
            + s_t_cm1_cm2.y;

    div = bot.x*bot.x + bot.y*bot.y;                // tmp = top/bot
    if (div < FLT_MIN)
    {
        return 1;
    }
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    g_new_z.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);      // Z = tmp**2
    g_new_z.y = tmp.x * tmp.y;
    g_new_z.y += g_new_z.y;

    return g_bailout_float();
}
