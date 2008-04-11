#include <string>

#include "port.h"
#include "id.h"
#include "prototyp.h"

long calculate_mandelbrot_fp_p5_asm()
{
	// TODO: optimize for pentium?
	extern long calculate_mandelbrot_fp_asm();
	return calculate_mandelbrot_fp_asm();
}

void calculate_mandelbrot_fp_p5_asm_start()
{
	// TODO: optimize for pentium?
}
