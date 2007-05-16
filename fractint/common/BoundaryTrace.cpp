#include <cstring>

#include "port.h"
#include "fractint.h"
#include "prototyp.h"
#include "externs.h"
#include "BoundaryTrace.h"
#include "WorkList.h"

enum direction
{
	North, East, South, West
};

static int s_trail_row;
static int s_trail_col;
static enum direction s_going_to;

/******************* boundary trace method ***************************/

#define bkcolor 0
#define advance_match()     coming_from = (direction) (((s_going_to = (direction) ((s_going_to - 1) & 0x03)) - 1) & 0x03)
#define advance_no_match()  s_going_to = (direction) ((s_going_to + 1) & 0x03)

/*******************************************************************/
/* take one step in the direction of s_going_to */
static void step_col_row()
{
	switch (s_going_to)
	{
	case North:
		g_col = s_trail_col;
		g_row = s_trail_row - 1;
		break;
	case East:
		g_col = s_trail_col + 1;
		g_row = s_trail_row;
		break;
	case South:
		g_col = s_trail_col;
		g_row = s_trail_row + 1;
		break;
		case West:
			g_col = s_trail_col - 1;
			g_row = s_trail_row;
			break;
	}
}

int boundary_trace_main()
{
	enum direction coming_from;
	unsigned int match_found;
	unsigned int continue_loop;
	int trail_color;
	int fillcolor_used;
	int last_fillcolor_used = -1;
	int max_putline_length;
	int right;
	int left;
	int length;
	if (g_inside == 0 || g_outside == 0)
	{
		stop_message(0, "Boundary tracing cannot be used with inside=0 or outside=0");
		return -1;
	}
	if (g_colors < 16)
	{
		stop_message(0, "Boundary tracing cannot be used with < 16 colors");
		return -1;
	}

	g_got_status = GOT_STATUS_BOUNDARY_TRACE;
	max_putline_length = 0; /* reset max_putline_length */
	for (g_current_row = s_iy_start; g_current_row <= g_y_stop; g_current_row++)
	{
		g_reset_periodicity = 1; /* reset for a new row */
		g_color = bkcolor;
		for (g_current_col = s_ix_start; g_current_col <= g_x_stop; g_current_col++)
		{
			if (getcolor(g_current_col, g_current_row) != bkcolor)
			{
				continue;
			}
			trail_color = g_color;
			g_row = g_current_row;
			g_col = g_current_col;
			if ((*g_calculate_type)() == -1) /* color, row, col are global */
			{
				if (g_show_dot != bkcolor) /* remove g_show_dot pixel */
				{
					(*g_plot_color)(g_col, g_row, bkcolor);
				}
				if (g_y_stop != g_yy_stop)  /* DG */
				{
					g_y_stop = g_yy_stop - (g_current_row - g_yy_start); /* allow for sym */
				}
				work_list_add(g_xx_start, g_xx_stop, g_current_col, g_current_row, g_y_stop, g_current_row, 0, s_work_sym);
				return -1;
			}
			g_reset_periodicity = 0; /* normal periodicity checking */

			/*
			This next line may cause a few more pixels to be calculated,
			but at the savings of quite a bit of overhead
			*/
			if (g_color != trail_color)  /* DG */
			{
				continue;
			}

			/* sweep clockwise to trace outline */
			s_trail_row = g_current_row;
			s_trail_col = g_current_col;
			trail_color = g_color;
			fillcolor_used = g_fill_color > 0 ? g_fill_color : trail_color;
			coming_from = West;
			s_going_to = East;
			match_found = 0;
			continue_loop = TRUE;
			do
			{
				step_col_row();
				if (g_row >= g_current_row
					&& g_col >= s_ix_start
					&& g_col <= g_x_stop
					&& g_row <= g_y_stop)
				{
					/* the order of operations in this next line is critical */
					g_color = getcolor(g_col, g_row);
					if (g_color == bkcolor && (*g_calculate_type)() == -1)
								/* color, row, col are global for (*g_calculate_type)() */
					{
						if (g_show_dot != bkcolor) /* remove g_show_dot pixel */
						{
							(*g_plot_color)(g_col, g_row, bkcolor);
						}
						if (g_y_stop != g_yy_stop)  /* DG */
						{
							g_y_stop = g_yy_stop - (g_current_row - g_yy_start); /* allow for sym */
						}
						work_list_add(g_xx_start, g_xx_stop, g_current_col, g_current_row, g_y_stop, g_current_row, 0, s_work_sym);
						return -1;
					}
					else if (g_color == trail_color)
					{
						if (match_found < 4) /* to keep it from overflowing */
						{
							match_found++;
						}
						s_trail_row = g_row;
						s_trail_col = g_col;
						advance_match();
					}
					else
					{
						advance_no_match();
						continue_loop = s_going_to != coming_from || match_found;
					}
				}
				else
				{
					advance_no_match();
					continue_loop = s_going_to != coming_from || match_found;
				}
			}
			while (continue_loop && (g_col != g_current_col || g_row != g_current_row));

			if (match_found <= 3)  /* DG */
			{
				/* no hole */
				g_color = bkcolor;
				g_reset_periodicity = 1;
				continue;
			}

			/*
			Fill in region by looping around again, filling lines to the left
			whenever s_going_to is South or West
			*/
			s_trail_row = g_current_row;
			s_trail_col = g_current_col;
			coming_from = West;
			s_going_to = East;
			do
			{
				match_found = FALSE;
				do
				{
					step_col_row();
					if (g_row >= g_current_row
						&& g_col >= s_ix_start
						&& g_col <= g_x_stop
						&& g_row <= g_y_stop
						&& getcolor(g_col, g_row) == trail_color)
						/* getcolor() must be last */
					{
						if (s_going_to == South
							|| (s_going_to == West && coming_from != East))
						{ /* fill a row, but only once */
							right = g_col;
							while (--right >= s_ix_start)
							{
								g_color = getcolor(right, g_row);
								if (g_color != trail_color)
								{
									break;
								}
							}
							if (g_color == bkcolor) /* check last color */
							{
								left = right;
								while (getcolor(--left, g_row) == bkcolor)
									/* Should NOT be possible for left < s_ix_start */
								{
									/* do nothing */
								}
								left++; /* one pixel too far */
								if (right == left) /* only one hole */
								{
									(*g_plot_color)(left, g_row, fillcolor_used);
								}
								else
								{ /* fill the line to the left */
									length = right-left + 1;
									if (fillcolor_used != last_fillcolor_used || length > max_putline_length)
									{ /* only reset g_stack if necessary */
										memset(g_stack, fillcolor_used, length);
										last_fillcolor_used = fillcolor_used;
										max_putline_length = length;
									}
									sym_fill_line(g_row, left, right, g_stack);
								}
							} /* end of fill line */

#if 0 /* don't interupt with a check_key() during fill */
							if (--g_input_counter <= 0)
							{
								if (check_key())
								{
									if (g_y_stop != g_yy_stop)
									{
										g_y_stop = g_yy_stop - (g_current_row - g_yy_start); /* allow for sym */
									}
									work_list_add(g_xx_start, g_xx_stop, g_current_col, g_current_row, g_y_stop, g_current_row, 0, s_work_sym);
									return -1;
								}
								g_input_counter = g_max_input_counter;
							}
#endif
						}
						s_trail_row = g_row;
						s_trail_col = g_col;
						advance_match();
						match_found = TRUE;
					}
					else
					{
						advance_no_match();
					}
				}
				while (!match_found && s_going_to != coming_from);

				if (!match_found)
				{ /* next one has to be a match */
					step_col_row();
					s_trail_row = g_row;
					s_trail_col = g_col;
					advance_match();
				}
			}
			while (s_trail_col != g_current_col || s_trail_row != g_current_row);
			g_reset_periodicity = 1; /* reset after a trace/fill */
			g_color = bkcolor;
		}
	}

	return 0;
}
