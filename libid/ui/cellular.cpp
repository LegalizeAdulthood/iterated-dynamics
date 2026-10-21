// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
//****************** standalone engine for "cellular" *******************

#include "ui/cellular.h"

#include "engine/calcfrac.h"
#include "engine/NonStandardFractal.h"
#include "fractals/Cellular.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"
#include "ui/stop_msg.h"
#include "ui/thinking.h"

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

namespace
{

class CellularThinkingScreen
{
public:
    CellularThinkingScreen() = default;
    CellularThinkingScreen(const CellularThinkingScreen &) = delete;
    CellularThinkingScreen(CellularThinkingScreen &&) = delete;
    CellularThinkingScreen &operator=(const CellularThinkingScreen &) = delete;
    CellularThinkingScreen &operator=(CellularThinkingScreen &&) = delete;

    ~CellularThinkingScreen()
    {
        clear();
    }

    void update(bool preparing);

private:
    void clear();

    bool m_active{};
};

void CellularThinkingScreen::update(const bool preparing)
{
    if (preparing)
    {
        m_active = true;
        thinking("Cellular thinking (higher start row takes longer)");
    }
    else
    {
        clear();
    }
}

void CellularThinkingScreen::clear()
{
    if (m_active)
    {
        m_active = false;
        thinking_end();
    }
}

} // namespace

int cellular_type(NonStandardFractal &calculation)
{
    try
    {
        CellularThinkingScreen thinking_screen;

        while (!calculation.done())
        {
            thinking_screen.update(calculation.preparing());
            display_disk_storage_progress();
            calculation.iterate();
            if (calculation.done())
            {
                break;
            }
            if (driver_key_pressed())
            {
                calculation.suspend();
                return -1;
            }
        }
        return 1;
    }
    catch (const CellularError &e)
    {
        stop_msg(e.what());
    }
    catch (const std::bad_alloc &)
    {
        stop_msg("Insufficient free memory for calculation");
    }
    catch (const std::exception &e)
    {
        stop_msg(e.what());
    }
    catch (...)
    {
        stop_msg("Unknown error in cellular calculation");
    }
    g_calc_status = CalcStatus::NON_RESUMABLE;
    return -1;
}

} // namespace id::ui
