#include "port.h"
#include "prototyp.h"

long cdecl calculate_mandelbrot_fp_p5_asm()
{
	/* TODO: optimize for pentium? */
	extern long cdecl calculate_mandelbrot_fp_asm();
	return calculate_mandelbrot_fp_asm();
}

void cdecl calculate_mandelbrot_fp_p5_asm_start()
{
	/* TODO: optimize for pentium? */
}
