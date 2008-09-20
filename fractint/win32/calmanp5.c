#include "port.h"
#include "prototyp.h"

long cdecl calcmandfpasm_p5(void)
{
	/* TODO: optimize for pentium? */
	extern long cdecl calcmandfpasm_c();
	return calcmandfpasm_c();
}

void cdecl calcmandfpasmstart_p5(void)
{
	/* TODO: optimize for pentium? */
}

