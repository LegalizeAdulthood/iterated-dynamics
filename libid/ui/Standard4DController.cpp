// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/Standard4DController.h"

#include "engine/NonStandardFractal.h"
#include "engine/spindac.h"
#include "fractals/julibrot.h"
#include "io/loadmap.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"
#include "ui/get_3d_params.h"
#include "ui/starfield.h"
#include "ui/stop_msg.h"

#include <stdexcept>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

Standard4DController::Standard4DController(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

void Standard4DController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The Standard4D controller has already started");
    }
    m_phase = Phase::ACTIVE;
    try
    {
        const bool red_blue{g_julibrot_3d_mode == Julibrot3DMode::RED_BLUE};
        if (red_blue)
        {
            g_save_dac = SaveDAC::NO;
        }
        if (g_save_dac != SaveDAC::YES)
        {
            const PaletteLoadResult palette{validate_luts(red_blue ? GLASSES1_MAP_NAME : ALTERN_MAP_NAME)};
            if (palette.failed)
            {
                throw std::runtime_error{palette.diagnostic};
            }
            if (!palette.diagnostic.empty())
            {
                stop_msg(palette.diagnostic);
            }
            refresh_dac();
            if (g_save_dac == SaveDAC::NEXT_TIME)
            {
                g_save_dac = SaveDAC::YES;
            }
        }
        if (m_calculation.done())
        {
            close(0);
        }
    }
    catch (...)
    {
        close(-1);
        m_calculation.finish(true);
        throw;
    }
}

void Standard4DController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The Standard4D controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        m_calculation.iterate();
        if (m_calculation.done())
        {
            close(0);
            return;
        }
        display_disk_storage_progress();
        if (driver_key_pressed())
        {
            suspend();
        }
    }
    catch (...)
    {
        close(-1);
        m_calculation.finish(true);
        throw;
    }
}

bool Standard4DController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int Standard4DController::result() const
{
    if (!done())
    {
        throw std::logic_error("The Standard4D controller has not completed");
    }
    return m_result;
}

void Standard4DController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active Standard4D controller");
    }
    m_calculation.suspend();
    close(-1);
}

void Standard4DController::close(const int result)
{
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
