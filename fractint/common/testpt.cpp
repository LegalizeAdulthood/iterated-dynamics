/*

Write your fractal program here. initreal and initimag are the values in
the complex plane; parm1, and parm2 are paramaters to be entered with the
"params=" option (if needed). The function should return the color associated
with initreal and initimag.  FRACTINT will repeatedly call your function with
the values of initreal and initimag ranging over the rectangle defined by the
"corners=" option. Assuming your formula is iterative, "g_max_iteration" is the maximum
iteration. If "g_max_iteration" is hit, color "inside" should be returned.

Note that this routine could be sped up using external variables/arrays
rather than the current parameter-passing scheme.  The goal, however was
to make it as easy as possible to add fractal types, and this looked like
the easiest way.

This module is part of an overlay, with calcfrac.c.  The routines in it
must not be called by any part of Fractint other than calcfrac.

The sample code below is a straightforward Mandelbrot routine.

*/

int  driver_get_key();

int test_start()     /* this routine is called just before the fractal starts */
{
	return  0;
}

void test_end()       /* this routine is called just after the fractal ends */
{
}

/* this routine is called once for every pixel */
/* (note: possibly using the dual-pass / solid-guessing options */

int test_per_pixel(double initreal, double initimag, double parm1, double parm2,
	long g_max_iteration, int inside)
{
	double oldreal, oldimag, newreal, newimag, magnitude;
	long color;
	oldreal = parm1;
	oldimag = parm2;
	magnitude = 0.0;
	color = 0;
	while ((magnitude < 4.0) && (color < g_max_iteration))
	{
		newreal = oldreal*oldreal - oldimag*oldimag + initreal;
		newimag = 2*oldreal*oldimag + initimag;
		color++;
		oldreal = newreal;
		oldimag = newimag;
		magnitude = newreal*newreal + newimag*newimag;
	}
	if (color >= g_max_iteration)
	{
		color = inside;
	}
	return (int)color;
}
