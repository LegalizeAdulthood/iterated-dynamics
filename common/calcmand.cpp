#include <float.h>
#include "port.h"
#include "prototyp.h"

long
calcmandasm(void)
{
    static int been_here = 0;
    if (!been_here)
    {
        stopmsg(0, "This integer fractal type is unimplemented;\n"
                "Use float=yes to get a real image.");
        been_here = 1;
    }
    return 0;
}
