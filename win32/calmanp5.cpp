#include "port.h"
#include "prototyp.h"

extern long cdecl calcmandfpasm_c();

long cdecl calcmandfpasm_p5(void)
{
    /* TODO: optimize for pentium? */
    return calcmandfpasm_c();
}

void cdecl calcmandfpasmstart_p5(void)
{
    /* TODO: optimize for pentium? */
}

