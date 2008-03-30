// Diffusion limited aggregation
#include <string>

#include "port.h"
#include "cmplx.h"
#include "id.h"
#include "externs.h"
#include "prototyp.h"

#include "Diffusion.h"
#include "drivers.h"
#include "fpu.h"
#include "fracsubr.h"
#include "framain2.h"
#include "MathUtil.h"
#include "miscres.h"
#include "resume.h"

// for diffusion type
enum DiffusionType
{
	DIFFUSION_CENTRAL	= 0,
	DIFFUSION_LINE		= 1,
	DIFFUSION_SQUARE	= 2
};

static int s_keyboard_check;                        // to limit kbd checking

inline int RANDOM(int x)
{
	return rand() % x;
}

// standalone engine for "diffusion"

int diffusion()
{
	int x_max;
	int y_max;
	int x_min;
	int y_min;     // Current maximum coordinates
	int border;   // Distance between release point and fractal
	// Determines diffusion type:  0 = central (classic)
	// 1 = falling particles
	// 2 = square cavity
	DiffusionType mode;
	int colorshift; // If zero, select colors at random, otherwise shift
					// the color every colorshift points
	int colorcount;
	int currentcolor;
	int i;
	double cosine;
	double sine;
	double angle;
	int x;
	int y;
	float r;
	float radius;

	if (driver_diskp())
	{
		not_disk_message();
	}

	x = -1;
	y = -1;
	g_bit_shift = 16;
	g_fudge = 1L << 16;

	border = int(g_parameters[0]);
	if (border <= 0)
	{
		border = 10;
	}

	mode = DiffusionType(int(g_parameters[1]));
	if ((mode > DIFFUSION_SQUARE) || (mode < DIFFUSION_CENTRAL))
	{
		mode = DIFFUSION_CENTRAL;
	}

	colorshift = int(g_parameters[2]);
	colorcount = colorshift; // Counts down from colorshift
	currentcolor = 1;  // Start at color 1 (color 0 is probably invisible)

	srand(g_random_seed);
	if (!g_use_fixed_random_seed)
	{
		++g_random_seed;
	}

	switch (mode)
	{
	case DIFFUSION_CENTRAL:
		x_max = g_x_dots/2 + border;  // Initial box
		x_min = g_x_dots/2 - border;
		y_max = g_y_dots/2 + border;
		y_min = g_y_dots/2 - border;
		break;

	case DIFFUSION_LINE:
		x_max = g_x_dots/2 + border;  // Initial box
		x_min = g_x_dots/2 - border;
		y_min = g_y_dots - border;
		break;

	case DIFFUSION_SQUARE:
		radius = (g_x_dots > g_y_dots) ? float(g_y_dots - border) : float(g_x_dots - border);
		break;
	}

	if (g_resuming) // restore g_work_list, if we can't the above will stay in place
	{
		start_resume();
		if (mode != DIFFUSION_SQUARE)
		{
			get_resume(sizeof(x_max), &x_max);
			get_resume(sizeof(x_min), &x_min);
			get_resume(sizeof(y_max), &y_max);
			get_resume(sizeof(y_min), &y_min);
		}
		else
		{
			get_resume(sizeof(x_max), &x_max);
			get_resume(sizeof(x_min), &x_min);
			get_resume(sizeof(y_max), &y_max);
			get_resume(sizeof(radius), &radius);
		}
		end_resume();
	}

	switch (mode)
	{
	case DIFFUSION_CENTRAL: // Single seed point in the center
		g_plot_color_put_color(g_x_dots/2, g_y_dots/2, currentcolor);
		break;
	case DIFFUSION_LINE: // Line along the bottom
		for (i = 0; i <= g_x_dots; i++)
		{
			g_plot_color_put_color(i, g_y_dots-1, currentcolor);
		}
		break;
	case DIFFUSION_SQUARE: // Large square that fills the screen
		if (g_x_dots > g_y_dots)
		{
			for (i = 0; i < g_y_dots; i++)
			{
				g_plot_color_put_color(g_x_dots/2-g_y_dots/2 , i , currentcolor);
				g_plot_color_put_color(g_x_dots/2 + g_y_dots/2 , i , currentcolor);
				g_plot_color_put_color(g_x_dots/2-g_y_dots/2 + i , 0 , currentcolor);
				g_plot_color_put_color(g_x_dots/2-g_y_dots/2 + i , g_y_dots-1 , currentcolor);
			}
		}
		else
		{
			for (i = 0; i < g_x_dots; i++)
			{
				g_plot_color_put_color(0 , g_y_dots/2-g_x_dots/2 + i , currentcolor);
				g_plot_color_put_color(g_x_dots-1 , g_y_dots/2-g_x_dots/2 + i , currentcolor);
				g_plot_color_put_color(i , g_y_dots/2-g_x_dots/2 , currentcolor);
				g_plot_color_put_color(i , g_y_dots/2 + g_x_dots/2 , currentcolor);
			}
		}
		break;
	}

	while (true)
	{
		switch (mode)
		{
		case DIFFUSION_CENTRAL: // Release new point on a circle inside the box
			angle = 2*double(rand())/(RAND_MAX/MathUtil::Pi);
			FPUsincos(&angle, &sine, &cosine);
			x = int(cosine*(x_max-x_min) + g_x_dots);
			y = int(sine  *(y_max-y_min) + g_y_dots);
			x /= 2;
			y /= 2;
			break;
		case DIFFUSION_LINE: // Release new point on the line y_min somewhere between x_min and x_max
			y = y_min;
			x = RANDOM(x_max-x_min) + (g_x_dots-x_max + x_min)/2;
			break;
		case DIFFUSION_SQUARE:
			// Release new point on a circle inside the box with radius
			// given by the radius variable
			angle = 2*double(rand())/(RAND_MAX/MathUtil::Pi);
			FPUsincos(&angle, &sine, &cosine);
			x = int(cosine*radius + g_x_dots);
			y = int(sine  *radius + g_y_dots);
			x /= 2;
			y /= 2;
			break;
		}

		// Loop as long as the point (x, y) is surrounded by color 0
		// on all eight sides

		while ((get_color(x + 1, y + 1) == 0) && (get_color(x + 1, y) == 0) &&
			(get_color(x + 1, y-1) == 0) && (get_color(x  , y + 1) == 0) &&
			(get_color(x  , y-1) == 0) && (get_color(x-1, y + 1) == 0) &&
			(get_color(x-1, y) == 0) && (get_color(x-1, y-1) == 0))
		{
			// Erase moving point
			if (g_show_orbit)
			{
				g_plot_color_put_color(x, y, 0);
			}

			if (mode == DIFFUSION_CENTRAL) // Make sure point is inside the box
			{
				if (x == x_max)
				{
					x--;
				}
				else if (x == x_min)
				{
					x++;
				}
				if (y == y_max)
				{
					y--;
				}
				else if (y == y_min)
				{
					y++;
				}
			}

			// Make sure point is on the screen below g_y_min, but
			// we need a 1 pixel margin because of the next random step.
			if (mode == DIFFUSION_LINE)
			{
				if (x >= g_x_dots-1)
				{
					x--;
				}
				else if (x <= 1)
				{
					x++;
				}
				if (y < y_min)
				{
					y++;
				}
			}

			// Take one random step
			x += RANDOM(3) - 1;
			y += RANDOM(3) - 1;

			// Check keyboard
			if ((++s_keyboard_check & 0x7f) == 1)
			{
				if (check_key())
				{
					alloc_resume(20, 1);
					if (mode != DIFFUSION_SQUARE)
					{
						put_resume(sizeof(x_max), &x_max);
						put_resume(sizeof(x_min), &x_min);
						put_resume(sizeof(y_max), &y_max);
						put_resume(sizeof(y_min), &y_min);
					}
					else
					{
						put_resume(sizeof(x_max), &x_max);
						put_resume(sizeof(x_min), &x_min);
						put_resume(sizeof(y_max), &y_max);
						put_resume(sizeof(radius), &radius);
					}
					s_keyboard_check--;
					return 1;
				}
			}

			// Show the moving point
			if (g_show_orbit)
			{
				g_plot_color_put_color(x, y, RANDOM(g_colors-1) + 1);
			}

		} // End of loop, now fix the point

		// If we're doing colorshifting then use currentcolor, otherwise
		// pick one at random
		g_plot_color_put_color(x, y, colorshift ? currentcolor : RANDOM(g_colors-1) + 1);

		// If we're doing colorshifting then check to see if we need to shift
		if (colorshift)
		{
			if (!--colorcount) // If the counter reaches zero then shift
			{
				currentcolor++;      // Increase the current color and wrap
				currentcolor %= g_colors;  // around skipping zero
				if (!currentcolor)
				{
					currentcolor++;
				}
				colorcount = colorshift;  // and reset the counter
			}
		}

		// If the new point is close to an edge, we may need to increase
		//	some limits so that the limits expand to match the growing
		//	fractal.

		switch (mode)
		{
		case DIFFUSION_CENTRAL:
			if (((x + border) > x_max) || ((x-border) < x_min)
				|| ((y-border) < y_min) || ((y + border) > y_max))
			{
				// Increase box size, but not past the edge of the screen
				y_min--;
				y_max++;
				x_min--;
				x_max++;
				if ((y_min == 0) || (x_min == 0))
				{
					return 0;
				}
			}
			break;
		case DIFFUSION_LINE: // Decrease g_y_min, but not past top of screen
			if (y-border < y_min)
			{
				y_min--;
			}
			if (y_min == 0)
			{
				return 0;
			}
			break;
		case DIFFUSION_SQUARE:
			// Decrease the radius where points are released to stay away
			// from the fractal.  It might be decreased by 1 or 2
			r = sqr(float(x)-g_x_dots/2) + sqr(float(y)-g_y_dots/2);
			if (r <= border*border)
			{
				return 0;
			}
			while ((radius-border)*(radius-border) > r)
			{
				radius--;
			}
			break;
		}
	}
}
