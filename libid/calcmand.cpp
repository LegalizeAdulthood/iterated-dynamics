#include "calcmand.h"

#include "stop_msg.h"

long calcmandasm()
{
    static bool been_here = false;
    if (!been_here)
    {
        stopmsg(STOPMSG_NONE,
            "This integer fractal type is unimplemented;\n"
            "Use float=yes to get a real image.");
        been_here = true;
    }
    return 0;
}
