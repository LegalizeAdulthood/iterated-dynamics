// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/engine_timer.h"

#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "io/decoder.h"
#include "io/encoder.h"
#include "io/library.h"
#include "misc/debug_flags.h"

#include <fmt/format.h>

#include <cstdarg>
#include <cstdio>
#include <ctime>

bool g_timer_flag{};         // you didn't see this, either
long g_engine_timer_start{}; // timer(...) start & total
long g_timer_interval{};     //

namespace
{

enum class TimerType
{
    ENGINE = 0,
    DECODER = 1,
    ENCODER = 2
};

} // namespace

/* timer function:
     timer(timer_type::ENGINE,(*fractal)())              fractal engine
     timer(timer_type::DECODER,nullptr,int width)        decoder
     timer(timer_type::ENCODER)                          encoder
  */
static int timer(TimerType type, int (*fn)(), ...)
{
    std::va_list arg_marker; // variable arg list
    std::FILE *fp = nullptr;
    int out = 0;
    int i;

    va_start(arg_marker, fn);

    bool do_bench = g_timer_flag;   // record time?
    if (type == TimerType::ENCODER) // encoder, record time only if debug flag set
    {
        do_bench = (g_debug_flag == DebugFlags::BENCHMARK_ENCODER);
    }
    if (do_bench)
    {
        fp = std::fopen(id::io::get_save_path(id::io::WriteFile::ROOT, "id-bench.txt").string().c_str(), "a");
    }
    g_engine_timer_start = std::clock();
    switch (type)
    {
    case TimerType::ENGINE:
        out = fn();
        break;
    case TimerType::DECODER:
        i = va_arg(arg_marker, int);
        out = (int) decoder((short) i); // not indirect, safer with overlays
        break;
    case TimerType::ENCODER:
        out = encoder();                // not indirect, safer with overlays
        break;
    }
    // next assumes CLOCKS_PER_SEC is 10^n, n>=2
    g_timer_interval = (std::clock() - g_engine_timer_start) / (CLOCKS_PER_SEC / 100);

    if (do_bench)
    {
        std::time_t now;
        std::time(&now);
        char *text = std::ctime(&now);
        text[24] = 0; // clobber newline in time string
        switch (type)
        {
        case TimerType::DECODER:
            fmt::print(fp, "decode ");
            break;
        case TimerType::ENCODER:
            fmt::print(fp, "encode ");
            break;
        default:
            break;
        }
        fmt::print(fp, "{:s} type={:s} resolution = {:d}x{:d} maxiter={:d}", //
            text,                                                     //
            g_cur_fractal_specific->name,                             //
            g_logical_screen_x_dots, g_logical_screen_y_dots,         //
            g_max_iterations);
        fmt::print(fp, " time= {:d}.{:02d} secs\n", g_timer_interval / 100, g_timer_interval % 100);
        std::fclose(fp);
    }
    return out;
}

void engine_timer(int (*fn)())
{
    (void) timer(TimerType::ENGINE, fn);
}

int encoder_timer()
{
    return timer(TimerType::ENCODER, nullptr);
}

int decoder_timer(int width)
{
    return timer(TimerType::DECODER, nullptr, width);
}
