#include <string>

#include "port.h"
#include "cmplx.h"
#include "id.h"
#include "externs.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "drivers.h"
#include "fracsubr.h"
#include "resume.h"
#include "Test.h"
#include "testpt.h"

/***************** standalone engine for "test" ********************/

int test()
{
	int startrow;
	int startpass;
	int numpasses;
	startrow = 0;
	startpass = 0;
	if (g_resuming)
	{
		start_resume();
		get_resume(sizeof(startrow), &startrow);
		get_resume(sizeof(startpass), &startpass);
		end_resume();
	}
	if (test_start()) /* assume it was stand-alone, doesn't want passes logic */
	{
		return 0;
	}
	numpasses = (g_standard_calculation_mode == '1') ? 0 : 1;
	for (g_passes = startpass; g_passes <= numpasses; g_passes++)
	{
		for (g_row = startrow; g_row <= g_y_stop; g_row = g_row + 1 + numpasses)
		{
			for (g_col = 0; g_col <= g_x_stop; g_col++)       /* look at each point on screen */
			{
				register int color;
				g_initial_z.x = g_dx_pixel();
				g_initial_z.y = g_dy_pixel();
				if (driver_key_pressed())
				{
					test_end();
					alloc_resume(20, 1);
					put_resume(sizeof(g_row), &g_row);
					put_resume(sizeof(g_passes), &g_passes);
					return -1;
				}
				color = test_per_pixel(g_initial_z.x, g_initial_z.y, g_parameter.x, g_parameter.y, g_max_iteration, g_inside);
				if (color >= g_colors)  /* avoid trouble if color is 0 */
				{
					color = ((color-1) % g_and_color) + 1; /* skip color zero */
				}
				g_plot_color(g_col, g_row, color);
				if (numpasses && (g_passes == 0))
				{
					g_plot_color(g_col, g_row + 1, color);
				}
			}
		}
		startrow = g_passes + 1;
	}
	test_end();
	return 0;
}

