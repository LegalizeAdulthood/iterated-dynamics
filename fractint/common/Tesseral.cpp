#include <cstring>

#include "port.h"
#include "fractint.h"
#include "prototyp.h"
#include "externs.h"
#include "fracsubr.h"

#include "Tesseral.h"
#include "WorkList.h"

/**************** tesseral method by CJLT begins here*********************/
/*  reworked by PB for speed and resumeability */

struct tess  /* one of these per box to be done gets stacked */
{
	int x1;
	int x2;
	int y1;
	int y2;      /* left/right top/bottom x/y coords  */
	int top;
	int bottom;
	int left;
	int right;  /* edge colors, -1 mixed, -2 unknown */
};

static int _fastcall tesseral_check_column(int x, int y1, int y2)
{
	int i;
	i = getcolor(x, ++y1);
	while (--y2 > y1)
	{
		if (getcolor(x, y2) != i)
		{
			return -1;
		}
	}
	return i;
}

static int _fastcall tesseral_check_row(int x1, int x2, int y)
{
	int i;
	i = getcolor(x1, y);
	while (x2 > x1)
	{
		if (getcolor(x2, y) != i)
		{
			return -1;
		}
		--x2;
	}
	return i;
}

static int _fastcall tesseral_column(int x, int y1, int y2)
{
	int colcolor;
	int i;
	g_col = x;
	g_row = y1;
	g_reset_periodicity = 1;
	colcolor = (*g_calculate_type)();
	g_reset_periodicity = 0;
	while (++g_row <= y2)  /* generate the column */
	{
		i = (*g_calculate_type)();
		if (i < 0)
		{
			return -3;
		}
		if (i != colcolor)
		{
			colcolor = -1;
		}
	}
	return colcolor;
}

static int _fastcall tesseral_row(int x1, int x2, int y)
{
	int rowcolor;
	int i;
	g_row = y;
	g_col = x1;
	g_reset_periodicity = 1;
	rowcolor = (*g_calculate_type)();
	g_reset_periodicity = 0;
	while (++g_col <= x2)  /* generate the row */
	{
		i = (*g_calculate_type)();
		if (i < 0)
		{
			return -3;
		}
		if (i != rowcolor)
		{
			rowcolor = -1;
		}
	}
	return rowcolor;
}

int tesseral()
{
	struct tess *tp;

	bool guess_plot = (g_plot_color != g_put_color && g_plot_color != symplot2);
	tp = (struct tess *)&g_stack[0];
	tp->x1 = g_ix_start;                              /* set up initial box */
	tp->x2 = g_x_stop;
	tp->y1 = g_iy_start;
	tp->y2 = g_y_stop;

	if (g_work_pass == 0)  /* not resuming */
	{
		tp->top = tesseral_row(g_ix_start, g_x_stop, g_iy_start);     /* Do top row */
		tp->bottom = tesseral_row(g_ix_start, g_x_stop, g_y_stop);      /* Do bottom row */
		tp->left = tesseral_column(g_ix_start, g_iy_start + 1, g_y_stop-1); /* Do left column */
		tp->right = tesseral_column(g_x_stop, g_iy_start + 1, g_y_stop-1);  /* Do right column */
		if (check_key())  /* interrupt before we got properly rolling */
		{
			work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, g_yy_start, 0, g_work_sym);
			return -1;
		}
	}
	else  /* resuming, rebuild work stack */
	{
		int i;
		int mid;
		int curx;
		int cury;
		int xsize;
		int ysize;
		struct tess *tp2;
		tp->top = tp->bottom = tp->left = tp->right = -2;
		cury = g_yy_begin & 0xfff;
		ysize = 1;
		i = (unsigned)g_yy_begin >> 12;
		while (--i >= 0)
		{
			ysize <<= 1;
		}
		curx = g_work_pass & 0xfff;
		xsize = 1;
		i = (unsigned)g_work_pass >> 12;
		while (--i >= 0)
		{
			xsize <<= 1;
		}
		while (1)
		{
			tp2 = tp;
			if (tp->x2 - tp->x1 > tp->y2 - tp->y1)  /* next divide down middle */
			{
				if (tp->x1 == curx && (tp->x2 - tp->x1 - 2) < xsize)
				{
					break;
				}
				mid = (tp->x1 + tp->x2) >> 1;                /* Find mid point */
				if (mid > curx)  /* stack right part */
				{
					memcpy(++tp, tp2, sizeof(*tp));
					tp->x2 = mid;
				}
				tp2->x1 = mid;
			}
			else  /* next divide across */
			{
				if (tp->y1 == cury && (tp->y2 - tp->y1 - 2) < ysize)
				{
					break;
				}
				mid = (tp->y1 + tp->y2) >> 1;                /* Find mid point */
				if (mid > cury)  /* stack bottom part */
				{
					memcpy(++tp, tp2, sizeof(*tp));
					tp->y2 = mid;
				}
				tp2->y1 = mid;
			}
		}
	}

	g_got_status = GOT_STATUS_TESSERAL; /* for tab_display */

	while (tp >= (struct tess *)&g_stack[0])  /* do next box */
	{
		g_current_col = tp->x1; /* for tab_display */
		g_current_row = tp->y1;

		if (tp->top == -1 || tp->bottom == -1 || tp->left == -1 || tp->right == -1)
		{
			goto tess_split;
		}
		/* for any edge whose color is unknown, set it */
		if (tp->top == -2)
		{
			tp->top = tesseral_check_row(tp->x1, tp->x2, tp->y1);
		}
		if (tp->top == -1)
		{
			goto tess_split;
		}
		if (tp->bottom == -2)
		{
			tp->bottom = tesseral_check_row(tp->x1, tp->x2, tp->y2);
		}
		if (tp->bottom != tp->top)
		{
			goto tess_split;
		}
		if (tp->left == -2)
		{
			tp->left = tesseral_check_column(tp->x1, tp->y1, tp->y2);
		}
		if (tp->left != tp->top)
		{
			goto tess_split;
		}
		if (tp->right == -2)
		{
			tp->right = tesseral_check_column(tp->x2, tp->y1, tp->y2);
		}
		if (tp->right != tp->top)
		{
			goto tess_split;
		}

		{
			int mid;
			int midcolor;
			if (tp->x2 - tp->x1 > tp->y2 - tp->y1)  /* divide down the middle */
			{
				mid = (tp->x1 + tp->x2) >> 1;           /* Find mid point */
				midcolor = tesseral_column(mid, tp->y1 + 1, tp->y2-1); /* Do mid column */
				if (midcolor != tp->top)
				{
					goto tess_split;
				}
			}
			else  /* divide across the middle */
			{
				mid = (tp->y1 + tp->y2) >> 1;           /* Find mid point */
				midcolor = tesseral_row(tp->x1 + 1, tp->x2-1, mid); /* Do mid row */
				if (midcolor != tp->top)
				{
					goto tess_split;
				}
			}
		}

		{  /* all 4 edges are the same color, fill in */
			int i;
			int j;
			i = 0;
			if (g_fill_color != 0)
			{
				if (g_fill_color > 0)
				{
					tp->top = g_fill_color % g_colors;
				}
				j = tp->x2 - tp->x1 - 1;
				if (guess_plot || j < 2)  /* paint dots */
				{
					for (g_col = tp->x1 + 1; g_col < tp->x2; g_col++)
					{
						for (g_row = tp->y1 + 1; g_row < tp->y2; g_row++)
						{
							(*g_plot_color)(g_col, g_row, tp->top);
							if (++i > 500)
							{
								if (check_key())
								{
									goto tess_end;
								}
								i = 0;
							}
						}
					}
				}
				else  /* use put_line for speed */
				{
					memset(&g_stack[OLD_MAX_PIXELS], tp->top, j);
					for (g_row = tp->y1 + 1; g_row < tp->y2; g_row++)
					{
						put_line(g_row, tp->x1 + 1, tp->x2-1, &g_stack[OLD_MAX_PIXELS]);
						if (g_plot_color != g_put_color) /* symmetry */
						{
							j = g_yy_stop-(g_row-g_yy_start);
							if (j > g_y_stop && j < g_y_dots)
							{
								put_line(j, tp->x1 + 1, tp->x2-1, &g_stack[OLD_MAX_PIXELS]);
							}
						}
						if (++i > 25)
						{
							if (check_key())
							{
								goto tess_end;
							}
							i = 0;
						}
					}
				}
			}
			--tp;
		}
		continue;

tess_split:
		{  /* box not surrounded by same color, sub-divide */
			int mid;
			int midcolor;
			struct tess *tp2;
			if (tp->x2 - tp->x1 > tp->y2 - tp->y1)  /* divide down the middle */
			{
				mid = (tp->x1 + tp->x2) >> 1;                /* Find mid point */
				midcolor = tesseral_column(mid, tp->y1 + 1, tp->y2-1); /* Do mid column */
				if (midcolor == -3)
				{
					goto tess_end;
				}
				if (tp->x2 - mid > 1)  /* right part >= 1 column */
				{
					if (tp->top == -1)
					{
						tp->top = -2;
					}
					if (tp->bottom == -1)
					{
						tp->bottom = -2;
					}
					tp2 = tp;
					if (mid - tp->x1 > 1)  /* left part >= 1 col, stack right */
					{
						memcpy(++tp, tp2, sizeof(*tp));
						tp->x2 = mid;
						tp->right = midcolor;
					}
					tp2->x1 = mid;
					tp2->left = midcolor;
				}
				else
				{
					--tp;
				}
			}
			else  /* divide across the middle */
			{
				mid = (tp->y1 + tp->y2) >> 1;                /* Find mid point */
				midcolor = tesseral_row(tp->x1 + 1, tp->x2-1, mid); /* Do mid row */
				if (midcolor == -3)
				{
					goto tess_end;
				}
				if (tp->y2 - mid > 1)  /* bottom part >= 1 column */
				{
					if (tp->left == -1)
					{
						tp->left = -2;
					}
					if (tp->right == -1)
					{
						tp->right = -2;
					}
					tp2 = tp;
					if (mid - tp->y1 > 1)  /* top also >= 1 col, stack bottom */
					{
						memcpy(++tp, tp2, sizeof(*tp));
						tp->y2 = mid;
						tp->bottom = midcolor;
					}
					tp2->y1 = mid;
					tp2->top = midcolor;
				}
				else
				{
					--tp;
				}
			}
		}
	}

tess_end:
	if (tp >= (struct tess *)&g_stack[0])  /* didn't complete */
	{
		int i;
		int xsize;
		int ysize;
		xsize = ysize = 1;
		i = 2;
		while (tp->x2 - tp->x1 - 2 >= i)
		{
			i <<= 1;
			++xsize;
		}
		i = 2;
		while (tp->y2 - tp->y1 - 2 >= i)
		{
			i <<= 1;
			++ysize;
		}
		work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop,
			(ysize << 12) + tp->y1, (xsize << 12) + tp->x1, g_work_sym);
		return -1;
	}
	return 0;
} /* tesseral */
