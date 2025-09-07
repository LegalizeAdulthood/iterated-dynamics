// SPDX-License-Identifier: GPL-3.0-only
//
//****************** standalone engine for "cellular" *******************

#include "ui/cellular.h"

#include "fractals/Cellular.h"
#include "misc/Driver.h"
#include "ui/stop_msg.h"

using namespace id::misc;

int cellular_type()
{
    try
    {
        id::fractals::Cellular cellular;

        while (cellular.iterate())
        {
            if (driver_key_pressed())
            {
                cellular.suspend();
                return -1;
            }
        }
        return 1;
    }
    catch (const id::fractals::CellularError &e)
    {
        stop_msg(e.what());
        return -1;
    }
    catch (const std::bad_alloc &)
    {
        stop_msg("Insufficient free memory for calculation");
        return -1;
    }
    catch (const std::exception &e)
    {
        stop_msg(e.what());
        return -1;
    }
    catch (...)
    {
        stop_msg("Unknown error in cellular calculation");
        return -1;
    }
}
