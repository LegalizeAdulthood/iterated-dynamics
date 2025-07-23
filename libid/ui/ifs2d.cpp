#include "ui/ifs2d.h"

#include "fractals/lorenz.h"
#include "misc/Driver.h"

int ifs2d()
{
    id::fractals::IFS2D ifs;

    while (!ifs.done())
    {
        if (driver_key_pressed())
        {
            return -1;
        }

        ifs.iterate();
    }
    return 0;
}

