#include "busy.h"
#include "drivers.h"
#include "Externals.h"
#include "GaussianDistribution.h"
#include "helpdefs.h"
#include "loadmap.h"
#include "prompts2.h"
#include "prototyp.h"
#include "Starfield.h"
#include "StopMessage.h"
#include "UIChoices.h"

static double starfield_values[4] =
{
	30.0, 100.0, 5.0, 0.0
};

int starfield()
{
	int c;
	BusyMarker marker;
	if (starfield_values[0] <   1.0)
	{
		starfield_values[0] =   1.0;
	}
	if (starfield_values[0] > 100.0)
	{
		starfield_values[0] = 100.0;
	}
	if (starfield_values[1] <   1.0)
	{
		starfield_values[1] =   1.0;
	}
	if (starfield_values[1] > 100.0)
	{
		starfield_values[1] = 100.0;
	}
	if (starfield_values[2] <   1.0)
	{
		starfield_values[2] =   1.0;
	}
	if (starfield_values[2] > 100.0)
	{
		starfield_values[2] = 100.0;
	}

	GaussianDistribution::SetDistribution(int(starfield_values[0]));
	GaussianDistribution::SetConstant(long(((starfield_values[1])/100.0)*(1L << 16)));
	GaussianDistribution::SetSlope(int(starfield_values[2]));

	if (validate_luts(GREY_MAP))
	{
		stop_message(STOPMSG_NORMAL, "Unable to load ALTERN.MAP");
		return -1;
	}
	load_dac();
	for (g_row = 0; g_row < g_y_dots; g_row++)
	{
		for (g_col = 0; g_col < g_x_dots; g_col++)
		{
			if (driver_key_pressed())
			{
				driver_buzzer(BUZZER_INTERRUPT);
				return 1;
			}
			c = get_color(g_col, g_row);
			if (c == g_externs.Inside())
			{
				c = g_colors-1;
			}
			g_plot_color_put_color(g_col, g_row, GaussianDistribution::Evaluate(c, g_colors));
		}
	}
	driver_buzzer(BUZZER_COMPLETE);
	return 0;
}

int get_starfield_params()
{
	UIChoices dialog(IDHELP_STARFIELDS, "Starfield Parameters", 0);
	const char *starfield_prompts[3] =
	{
		"Star Density in Pixels per Star",
		"Percent Clumpiness",
		"Ratio of Dim stars to Bright"
	};
	for (int i = 0; i < 3; i++)
	{
		dialog.push(starfield_prompts[i], float(starfield_values[i]));
	}
	ScreenStacker stacker;
	if (dialog.prompt() < 0)
	{
		return -1;
	}
	for (int i = 0; i < 3; i++)
	{
		starfield_values[i] = dialog.values(i).uval.dval;
	}

	return 0;
}
