// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/calculate_image.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "ui/stop_msg.h"

#include <new>
#include <stdexcept>
#include <string>

using namespace id::engine;

namespace id::ui
{

// TODO: removed later
// Slice 52 replaces this diagnostic boundary and calc_fract dispatch with
// outer UI ownership of the selected incremental render operation.
int calculate_image()
{
    ImageCalculation image;
    std::string failure;
    int result{-1};
    try
    {
        for (const std::string &warning : image.start())
        {
            stop_msg(warning);
        }
        result = calc_fract();
    }
    catch (const std::bad_alloc &)
    {
        failure = "Insufficient free memory for calculation";
    }
    catch (const std::runtime_error &error)
    {
        failure = error.what();
    }
    if (!failure.empty())
    {
        g_calc_status = CalcStatus::NON_RESUMABLE;
    }
    image.finish();
    if (!failure.empty())
    {
        stop_msg(failure);
        if (g_init_batch != BatchMode::NONE)
        {
            g_init_batch = BatchMode::BAILOUT_ERROR_NO_SAVE;
        }
    }
    return result;
}

} // namespace id::ui
