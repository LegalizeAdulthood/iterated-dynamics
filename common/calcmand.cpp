#include <float.h>
#include "port.h"
#include "prototyp.h"

long
calcmandasm(void)
{
    static bool been_here = false;
    if (!been_here)
    {
        stopmsg(0, "This integer fractal type is unimplemented;\n"
                "Use float=yes to get a real image.");
        been_here = true;
    }
    return 0;
}
