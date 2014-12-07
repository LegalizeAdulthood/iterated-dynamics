#include <float.h>

#include "port.h"
#include "prototyp.h"

extern long calcmandfpasm_c();

long calcmandfpasm_p5(void)
{
    /* TODO: optimize for pentium? */
    return calcmandfpasm_c();
}
