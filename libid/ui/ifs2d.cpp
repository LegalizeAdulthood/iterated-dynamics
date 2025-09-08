#include "ui/ifs2d.h"

#include "fractals/lorenz.h"
#include "misc/Driver.h"

using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

int ifs2d()
{
    IFS2D ifs;

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

} // namespace id::ui
