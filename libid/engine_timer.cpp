// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine_timer.h"

#include "cmdfiles.h"
#include "debug_flags.h"
#include "decoder.h"
#include "dir_file.h"
#include "encoder.h"
#include "fractalp.h"
#include "id_data.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>

bool g_timer_flag{};     // you didn't see this, either
long g_timer_start{};    // timer(...) start & total
long g_timer_interval{}; //

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
static int timer(TimerType timertype, int (*subrtn)(), ...)
{
    std::va_list arg_marker; // variable arg list
    std::time_t ltime;
    std::FILE *fp = nullptr;
    int out = 0;
    int i;

    va_start(arg_marker, subrtn);

    bool do_bench = g_timer_flag;        // record time?
    if (timertype == TimerType::ENCODER) // encoder, record time only if debug flag set
    {
        do_bench = (g_debug_flag == DebugFlags::BENCHMARK_ENCODER);
    }
    if (do_bench)
    {
        fp = dir_fopen(g_working_dir.c_str(), "bench", "a");
    }
    g_timer_start = std::clock();
    switch (timertype)
    {
    case TimerType::ENGINE:
        out = subrtn();
        break;
    case TimerType::DECODER:
        i = va_arg(arg_marker, int);
        out = (int) decoder((short) i); // not indirect, safer with overlays
        break;
    case TimerType::ENCODER:
        out = encoder(); // not indirect, safer with overlays
        break;
    }
    // next assumes CLOCKS_PER_SEC is 10^n, n>=2
    g_timer_interval = (std::clock() - g_timer_start) / (CLOCKS_PER_SEC / 100);

    if (do_bench)
    {
        std::time(&ltime);
        char *timestring = std::ctime(&ltime);
        timestring[24] = 0; // clobber newline in time string
        switch (timertype)
        {
        case TimerType::DECODER:
            std::fprintf(fp, "decode ");
            break;
        case TimerType::ENCODER:
            std::fprintf(fp, "encode ");
            break;
        default:
            break;
        }
        std::fprintf(fp, "%s type=%s resolution = %dx%d maxiter=%ld", timestring,
            g_cur_fractal_specific->name, g_logical_screen_x_dots, g_logical_screen_y_dots, g_max_iterations);
        std::fprintf(fp, " time= %ld.%02ld secs\n", g_timer_interval / 100, g_timer_interval % 100);
        std::fclose(fp);
    }
    return out;
}

int engine_timer(int (*fn)())
{
    return timer(TimerType::ENGINE, fn);
}

int encoder_timer()
{
    return timer(TimerType::ENCODER, nullptr);
}

int decoder_timer(int width)
{
    return timer(TimerType::DECODER, nullptr, width);
}
