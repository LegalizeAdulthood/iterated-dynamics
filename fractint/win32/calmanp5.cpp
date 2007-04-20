extern "C"
{
#include "port.h"
#include "prototyp.h"
}

extern "C" long cdecl calculate_mandelbrot_fp_p5_asm(void)
{
	/* TODO: optimize for pentium? */
	extern long cdecl calculate_mandelbrot_fp_asm();
	return calculate_mandelbrot_fp_asm();
}

extern "C" void cdecl calculate_mandelbrot_fp_p5_asm_start(void)
{
	/* TODO: optimize for pentium? */
}
