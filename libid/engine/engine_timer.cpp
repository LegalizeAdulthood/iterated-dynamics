// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/engine_timer.h"

#include "engine/LogicalScreen.h"
#include "fractals/fractalp.h"
#include "io/check_write_file.h"
#include "io/library.h"

#include <fmt/format.h>

#include <cstdio>
#include <ctime>

using namespace id::fractals;
using namespace id::io;
using namespace id::misc;

namespace id::engine
{

bool g_timer_flag{};         // you didn't see this, either
long g_engine_timer_start{}; // timer(...) start & total
long g_timer_interval{};     //

/* timer function:
     timer((*fractal)())              fractal engine
  */
void engine_timer(int (*fn)())
{
    std::FILE *fp = nullptr;

    bool do_bench = g_timer_flag;   // record time?
    if (do_bench)
    {
        fp = std::fopen(get_append_save_path(WriteFile::DEBUG, "id-bench").string().c_str(), "a");
    }
    g_engine_timer_start = std::clock();
    const int out = fn();
    // next assumes CLOCKS_PER_SEC is 10^n, n>=2
    g_timer_interval = (std::clock() - g_engine_timer_start) / (CLOCKS_PER_SEC / 100);

    if (do_bench)
    {
        std::time_t now;
        std::time(&now);
        char *text = std::ctime(&now);
        text[24] = 0; // clobber newline in time string
        fmt::print(fp, "{:s} type={:s} resolution = {:d}x{:d} maxiter={:d}", //
            text,                                                     //
            g_cur_fractal_specific->name,                             //
            g_logical_screen.x_dots, g_logical_screen.y_dots,         //
            g_max_iterations);
        fmt::print(fp, " time= {:d}.{:02d} secs\n", g_timer_interval / 100, g_timer_interval % 100);
        std::fclose(fp);
    }
}

} // namespace id::engine
