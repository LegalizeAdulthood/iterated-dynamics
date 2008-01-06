/*
	This file includes miscellaneous plot functions and logic
	for 3D, used by lorenz.c and line3d.c
	By Tim Wegner and Marc Reinig.
*/
#include <assert.h>
#include <string>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "diskvid.h"
#include "line3d.h"
#include "loadmap.h"
#include "plot3d.h"
#include "prompts1.h"

#include "ThreeDimensionalState.h"

/* Use these palette indices for red/blue - same on ega/vga */
enum
{
	PAL_BLUE	= 1,
	PAL_RED		= 2,
	PAL_MAGENTA = 3
};

int g_which_image;
int g_xx_adjust1;
int g_yy_adjust1;
int g_x_shift1;
int g_y_shift1;

static int s_red_local_left;
static int s_red_local_right;
static int s_blue_local_left;
static int s_blue_local_right;
static BYTE s_targa_red;

/* Bresenham's algorithm for drawing line */
void cdecl draw_line (int X1, int Y1, int X2, int Y2, int color)

{               /* uses Bresenham algorithm to draw a line */
	int dX;
	int dY;			/* vector components */
	int row;
	int col;
	int final;		/* final row or column number */
	int G;			/* used to test for new row or column */
	int inc1;		/* G increment when row or column doesn't change */
	int inc2;		/* G increment when row or column changes */
	

	dX = X2 - X1;							/* find vector components */
	dY = Y2 - Y1;
	bool positive_slope = (dX > 0);
	if (dY < 0)
	{
		positive_slope = !positive_slope;
	}
	if (abs(dX) > abs(dY))                /* shallow line case */
	{
		if (dX > 0)         /* determine start point and last column */
		{
			col = X1;
			row = Y1;
			final = X2;
		}
		else
		{
			col = X2;
			row = Y2;
			final = X1;
		}
		inc1 = 2*abs(dY);            /* determine increments and initial G */
		G = inc1 - abs(dX);
		inc2 = 2*(abs(dY) - abs(dX));
		if (positive_slope)
		{
			while (col <= final)    /* step through columns checking for new row */
			{
				(*g_plot_color)(col, row, color);
				col++;
				if (G >= 0)             /* it's time to change rows */
				{
					row++;      /* positive slope so increment through the rows */
					G += inc2;
				}
				else                        /* stay at the same row */
				{
					G += inc1;
				}
			}
		}
		else
		{
			while (col <= final)    /* step through columns checking for new row */
			{
				(*g_plot_color)(col, row, color);
				col++;
				if (G > 0)              /* it's time to change rows */
				{
					row--;      /* negative slope so decrement through the rows */
					G += inc2;
				}
				else                        /* stay at the same row */
				{
					G += inc1;
				}
			}
		}
	}   /* if |dX| > |dY| */
	else                            /* steep line case */
	{
		if (dY > 0)             /* determine start point and last row */
		{
			col = X1;
			row = Y1;
			final = Y2;
		}
		else
		{
			col = X2;
			row = Y2;
			final = Y1;
		}
		inc1 = 2*abs(dX);            /* determine increments and initial G */
		G = inc1 - abs(dY);
		inc2 = 2*(abs(dX) - abs(dY));
		if (positive_slope)
		{
			while (row <= final)    /* step through rows checking for new column */
			{
				(*g_plot_color)(col, row, color);
				row++;
				if (G >= 0)                 /* it's time to change columns */
				{
					col++;  /* positive slope so increment through the columns */
					G += inc2;
				}
				else                    /* stay at the same column */
				{
					G += inc1;
				}
			}
		}
		else
		{
			while (row <= final)    /* step through rows checking for new column */
			{
				(*g_plot_color)(col, row, color);
				row++;
				if (G > 0)                  /* it's time to change columns */
				{
					col--;  /* negative slope so decrement through the columns */
					G += inc2;
				}
				else                    /* stay at the same column */
				{
					G += inc1;
				}
			}
		}
	}
}   /* draw_line */

#if 0
/* use this for continuous colors later */
void plot3dsuperimpose16b(int x, int y, int color)
{
	int tmp;
	if (color != 0)         /* Keeps index 0 still 0 */
	{
		color = g_colors - color; /*  Reverses color order */
		color /= 4;
		if (color == 0)
		{
			color = 1;
		}
	}
	color = 3;
	tmp = getcolor(x, y);

	/* map to 4 colors */
	if (g_which_image == WHICHIMAGE_RED)
	{
		if (s_red_local_left < x && x < s_red_local_right)
		{
			g_plot_color_put_color(x, y, color|tmp);
			if (g_targa_output)
			{
				targa_color(x, y, color|tmp);
			}
		}
	}
	else if (g_which_image == WHICHIMAGE_BLUE)
	{
		if (s_blue_local_left < x && x < s_blue_local_right)
		{
			color <<= 2;
			g_plot_color_put_color(x, y, color|tmp);
			if (g_targa_output)
			{
				targa_color(x, y, color|tmp);
			}
		}
	}
}

#endif

void plot_3d_superimpose_16(int x, int y, int color)
{
	int tmp;

	tmp = getcolor(x, y);

	if (g_which_image == WHICHIMAGE_RED)
	{
		color = PAL_RED;
		if (tmp > 0 && tmp != color)
		{
			color = PAL_MAGENTA;
		}
		if (s_red_local_left < x && x < s_red_local_right)
		{
			g_plot_color_put_color(x, y, color);
			if (g_targa_output)
			{
				targa_color(x, y, color);
			}
		}
	}
	else if (g_which_image == 2) /* BLUE */
	{
		if (s_blue_local_left < x && x < s_blue_local_right)
		{
			color = PAL_BLUE;
			if (tmp > 0 && tmp != color)
			{
				color = PAL_MAGENTA;
			}
			g_plot_color_put_color(x, y, color);
			if (g_targa_output)
			{
				targa_color(x, y, color);
			}
		}
	}
}


void plot_3d_superimpose_256(int x, int y, int color)
{
	int tmp;
	BYTE t_c;

	t_c = BYTE(255-color);

	if (color != 0)         /* Keeps index 0 still 0 */
	{
		color = g_colors - color; /*  Reverses color order */
		color = (g_max_colors == 236) ?
			(1 + color/21) /*  Maps colors 1-255 to 13 even ranges */
			: 
			(1 + color/18); /*  Maps colors 1-255 to 15 even ranges */
	}

	tmp = getcolor(x, y);
	/* map to 16 colors */
	if (g_which_image == WHICHIMAGE_RED)
	{
		if (s_red_local_left < x && x < s_red_local_right)
		{
			/* Overwrite prev Red don't mess w/blue */
			g_plot_color_put_color(x, y, color|(tmp&240));
			if (g_targa_output)
			{
				if (!(g_3d_state.fill_type() > FillType::Bars))
				{
					targa_color(x, y, color|(tmp&240));
				}
				else
				{
					disk_write_targa (x + g_sx_offset, y + g_sy_offset, t_c, 0, 0);
				}
			}
		}
	}
	else if (g_which_image == WHICHIMAGE_BLUE)
	{
		if (s_blue_local_left < x && x < s_blue_local_right)
		{
			/* Overwrite previous blue, don't mess with existing red */
			color <<= 4;
			g_plot_color_put_color(x, y, color|(tmp&15));
			if (g_targa_output)
			{
				if (!(g_3d_state.fill_type() > FillType::Bars))
				{
					targa_color(x, y, color|(tmp&15));
				}
				else
				{
					disk_read_targa (x + g_sx_offset, y + g_sy_offset, &s_targa_red, (BYTE *)&tmp, (BYTE *)&tmp);
					disk_write_targa (x + g_sx_offset, y + g_sy_offset, s_targa_red, 0, t_c);
				}
			}
		}
	}
}

void plot_ifs_3d_superimpose_256(int x, int y, int color)
{
	int tmp;
	BYTE t_c;

	t_c = BYTE(255-color);

	if (color != 0)         /* Keeps index 0 still 0 */
	{
		/* my mind is fried - lower indices = darker colors is EASIER! */
		color = g_colors - color; /*  Reverses color order */
		color = (g_max_colors == 236) ?
			(1 + color/21) /*  Maps colors 1-255 to 13 even ranges */
			:
			(1 + color/18); /*  Looks weird but maps colors 1-255 to 15
								relatively even ranges */
	}

	tmp = getcolor(x, y);
	/* map to 16 colors */
	if (g_which_image == WHICHIMAGE_RED)
	{
		if (s_red_local_left < x && x < s_red_local_right)
		{
			g_plot_color_put_color(x, y, color|tmp);
			if (g_targa_output)
			{
				if (!(g_3d_state.fill_type() > FillType::Bars))
				{
					targa_color(x, y, color|tmp);
				}
				else
				{
					disk_write_targa (x + g_sx_offset, y + g_sy_offset, t_c, 0, 0);
				}
			}
		}
	}
	else if (g_which_image == WHICHIMAGE_BLUE)
	{
		if (s_blue_local_left < x && x < s_blue_local_right)
		{
			color <<= 4;
			g_plot_color_put_color(x, y, color|tmp);
			if (g_targa_output)
			{
				if (!(g_3d_state.fill_type() > FillType::Bars))
				{
					targa_color(x, y, color|tmp);
				}
				else
				{
					disk_read_targa (x + g_sx_offset, y + g_sy_offset, &s_targa_red, (BYTE *)&tmp, (BYTE *)&tmp);
					disk_write_targa (x + g_sx_offset, y + g_sy_offset, s_targa_red, 0, t_c);
				}
			}
		}
	}
}

void plot_3d_alternate(int x, int y, int color)
{
	BYTE t_c;

	t_c = BYTE(255-color);
	/* lorez high color red/blue 3D plot function */
	/* if which image = 1, compresses color to lower 128 colors */

	/* my mind is STILL fried - lower indices = darker colors is EASIER! */
	color = g_colors - color;
	if ((g_which_image == WHICHIMAGE_RED) && !((x + y) & 1)) /* - lower half palette */
	{
		if (s_red_local_left < x && x < s_red_local_right)
		{
			g_plot_color_put_color(x, y, color >> 1);
			if (g_targa_output)
			{
				if (!(g_3d_state.fill_type() > FillType::Bars))
				{
					targa_color(x, y, color >> 1);
				}
				else
				{
					disk_write_targa (x + g_sx_offset, y + g_sy_offset, t_c, 0, 0);
				}
			}
		}
	}
	else if ((g_which_image == WHICHIMAGE_BLUE) && ((x + y) & 1)) /* - upper half palette */
	{
		if (s_blue_local_left < x && x < s_blue_local_right)
		{
			g_plot_color_put_color(x, y, (color >> 1) + (g_colors >> 1));
			if (g_targa_output)
			{
				if (!(g_3d_state.fill_type() > FillType::Bars))
				{
					targa_color(x, y, (color >> 1) + (g_colors >> 1));
				}
				else
				{
					disk_write_targa (x + g_sx_offset, y + g_sy_offset, s_targa_red, 0, t_c);
				}
			}
		}
	}
}

void plot_3d_cross_eyed_A(int x, int y, int color)
{
	x /= 2;
	y /= 2;
	if (g_which_image == WHICHIMAGE_BLUE)
	{
		x += g_x_dots/2;
	}
	if (g_row_count >= g_y_dots/2)
	{
		/* hidden surface kludge */
		if (getcolor(x, y) != 0)
		{
			return;
		}
	}
	g_plot_color_put_color(x, y, color);
}

void plot_3d_cross_eyed_B(int x, int y, int color)
{
	x /= 2;
	y /= 2;
	if (g_which_image == WHICHIMAGE_BLUE)
	{
		x += g_x_dots/2;
	}
	g_plot_color_put_color(x, y, color);
}

void plot_3d_cross_eyed_C(int x, int y, int color)
{
	if (g_row_count >= g_y_dots/2)
	{
		/* hidden surface kludge */
		if (getcolor(x, y) != 0)
		{
			return;
		}
	}
	g_plot_color_put_color(x, y, color);
}

void plot_setup()
{
	double d_red_bright  = 0;
	double d_blue_bright = 0;

	/* set funny glasses plot function */
	switch (g_3d_state.glasses_type())
	{
	case STEREO_ALTERNATE:
		g_plot_color_standard = plot_3d_alternate;
		break;

	case STEREO_SUPERIMPOSE:
		g_plot_color_standard = (g_fractal_type != FRACTYPE_IFS_3D) ?
			plot_3d_superimpose_256 : plot_ifs_3d_superimpose_256;
		break;

	case STEREO_PAIR: /* crosseyed mode */
		if (g_screen_width < 2*g_x_dots)
		{
			g_plot_color_standard = (g_3d_state.x_rotation() == 0 && g_3d_state.y_rotation() == 0) ? /* use hidden surface kludge */
				plot_3d_cross_eyed_A : plot_3d_cross_eyed_B;
		}
		else if (g_3d_state.x_rotation() == 0 && g_3d_state.y_rotation() == 0)
		{
			g_plot_color_standard = plot_3d_cross_eyed_C; /* use hidden surface kludge */
		}
		else
		{
			g_plot_color_standard = g_plot_color_put_color;
		}
		break;

	default:
		g_plot_color_standard = g_plot_color_put_color;
		break;
	}
	assert(g_plot_color_standard);

	g_x_shift = int((g_3d_state.x_shift()*double(g_x_dots))/100);
	g_y_shift = int((g_3d_state.y_shift()*double(g_y_dots))/100);
	g_x_shift1 = g_x_shift;
	g_y_shift1 = g_y_shift;

	if (g_3d_state.glasses_type())
	{
		s_red_local_left  =   int((g_3d_state.red().crop_left()*double(g_x_dots))/100.0);
		s_red_local_right =   int(((100 - g_3d_state.red().crop_right())*double(g_x_dots))/100.0);
		s_blue_local_left =   int((g_3d_state.blue().crop_left()*double(g_x_dots))/100.0);
		s_blue_local_right =  int(((100 - g_3d_state.blue().crop_right())*double(g_x_dots))/100.0);
		d_red_bright = double(g_3d_state.red().bright())/100.0;
		d_blue_bright = double(g_3d_state.blue().bright())/100.0;

		if (g_which_image == WHICHIMAGE_RED)
		{
			g_x_shift += int((g_3d_state.eye_separation() * double(g_x_dots)) / 200);
			g_xx_adjust = int(((g_3d_state.x_trans() + g_3d_state.x_adjust()) * double(g_x_dots)) / 100);
			g_x_shift1 -= int((g_3d_state.eye_separation() * double(g_x_dots)) / 200);
			g_xx_adjust1 = int(((g_3d_state.x_trans() - g_3d_state.x_adjust()) * double(g_x_dots)) / 100);
			if (g_3d_state.glasses_type() == STEREO_PAIR && g_screen_width >= 2 * g_x_dots)
			{
				g_sx_offset = g_screen_width / 2 - g_x_dots;
			}
		}
		else if (g_which_image == WHICHIMAGE_BLUE)
		{
			g_x_shift -= int((g_3d_state.eye_separation() * double(g_x_dots)) / 200);
			g_xx_adjust = int(((g_3d_state.x_trans() - g_3d_state.x_adjust()) * double(g_x_dots)) / 100);
			if (g_3d_state.glasses_type() == STEREO_PAIR && g_screen_width >= 2 * g_x_dots)
			{
				g_sx_offset = g_screen_width / 2;
			}
		}
	}
	else
	{
		g_xx_adjust = int((g_3d_state.x_trans()*double(g_x_dots))/100);
	}
	g_yy_adjust = int(-(g_3d_state.y_trans()*double(g_y_dots))/100);

	if (g_.MapSet())
	{
		validate_luts(g_.MapName()); /* read the palette file */
		if (g_3d_state.glasses_type() == STEREO_ALTERNATE || g_3d_state.glasses_type() == STEREO_SUPERIMPOSE)
		{
			if (g_3d_state.glasses_type() == STEREO_SUPERIMPOSE && g_colors < 256)
			{
				g_.DAC().Set(PAL_RED, COLOR_CHANNEL_MAX, 0, 0);
				g_.DAC().Set(PAL_BLUE, 0, 0, COLOR_CHANNEL_MAX);
				g_.DAC().Set(PAL_MAGENTA, COLOR_CHANNEL_MAX, 0, COLOR_CHANNEL_MAX);
			}
			for (int i = 0; i < 256; i++)
			{
				g_.DAC().SetRed(i, BYTE(g_.DAC().Red(i)*d_red_bright));
				g_.DAC().SetBlue(i, BYTE(g_.DAC().Blue(i)*d_blue_bright));
			}
		}
		spindac(0, 1); /* load it, but don't spin */
	}
}
