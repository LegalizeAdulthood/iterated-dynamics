//
//	Standalone engine for "popcorn"
//
#include <string>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "calcfrac.h"
#include "Externals.h"
#include "resume.h"

int popcorn()   // subset of std engine
{
	int start_row;
	start_row = 0;
	if (g_resuming)
	{
		start_resume();
		get_resume(sizeof(start_row), &start_row);
		end_resume();
	}
	g_input_counter = g_max_input_counter;
	g_plot_color = plot_color_none;
	g_temp_sqr.real(0.0); // PB added this to cover weird BAILOUTs
	g_temp_sqr_l.real(0L);
	for (g_row = start_row; g_row <= g_y_stop; g_row++)
	{
		g_reset_periodicity = true;
		for (g_col = 0; g_col <= g_x_stop; g_col++)
		{
			if (standard_fractal() == -1) // interrupted
			{
				alloc_resume(10, 1);
				put_resume(sizeof(g_row), &g_row);
				return -1;
			}
			g_reset_periodicity = false;
		}
	}
	g_externs.SetCalculationStatus(CALCSTAT_COMPLETED);
	return 0;
}
