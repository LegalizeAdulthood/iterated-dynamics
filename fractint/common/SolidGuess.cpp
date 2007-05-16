#include <cstring>

#include "port.h"
#include "fractint.h"
#include "cmplx.h"
#include "externs.h"
#include "prototyp.h"
#include "drivers.h"
#include "SolidGuess.h"
#include "WorkList.h"

static int s_max_block;
static int s_half_block;
static int s_guess_plot;                   /* paint 1st pass row at a time?   */
static int s_right_guess;
static int s_bottom_guess;
static unsigned int s_t_prefix[2][MAX_Y_BLOCK][MAX_X_BLOCK]; /* common temp */

#define calcadot(c, x, y) \
	do \
	{ \
		g_col = x; \
		g_row = y; \
		c = (*g_calculate_type)(); \
		if (c == -1) \
		{ \
			return -1; \
		} \
	} \
	while (0)

static void _fastcall plot_block(int buildrow, int x, int y, int color)
{
	int xlim;
	int ylim;
	xlim = x + s_half_block;
	if (xlim > g_x_stop)
	{
		xlim = g_x_stop + 1;
	}
	if (buildrow >= 0 && !s_guess_plot) /* save it for later put_line */
	{
		if (buildrow == 0)
		{
			for (int i = x; i < xlim; ++i)
			{
				g_stack[i] = (BYTE)color;
			}
		}
		else
		{
			for (int i = x; i < xlim; ++i)
			{
				g_stack[i + OLD_MAX_PIXELS] = (BYTE)color;
			}
		}
		if (x >= g_xx_start) /* when x reduced for alignment, paint those dots too */
		{
			return; /* the usual case */
		}
	}
	/* paint it */
	ylim = y + s_half_block;
	if (ylim > g_y_stop)
	{
		if (y > g_y_stop)
		{
			return;
		}
		ylim = g_y_stop + 1;
	}
	for (int i = x; ++i < xlim; )
	{
		(*g_plot_color)(i, y, color); /* skip 1st dot on 1st row */
	}
	while (++y < ylim)
	{
		for (int i = x; i < xlim; ++i)
		{
			(*g_plot_color)(i, y, color);
		}
	}
}

static int _fastcall guess_row(int firstpass, int y, int blocksize)
{
	int j;
	int color;
	int xplushalf;
	int xplusblock;
	int ylessblock;
	int ylesshalf;
	int yplushalf;
	int yplusblock;
	int c21;
	int c31;
	int c41;         /* cxy is the color of pixel at (x, y) */
	int c12;
	int c22;
	int c32;
	int c42;         /* where c22 is the topleft corner of */
	int c13;
	int c23;
	int c33;             /* the g_block being handled in current */
	int c24;
	int c44;         /* iteration                          */
	int guessed23;
	int guessed32;
	int guessed33;
	int guessed12;
	int guessed13;
	int prev11;
	int fix21;
	int fix31;
	unsigned int *pfxptr;
	unsigned int pfxmask;

	c44 = 0;
	c41 = 0;
	c42 = 0;  /* just for warning */

	s_half_block = blocksize/2;
	{
		int i = y/s_max_block;
		pfxptr = (unsigned int *) &s_t_prefix[firstpass][(i >> 4) + 1][g_ix_start/s_max_block];
		pfxmask = 1 << (i & 15);
	}
	ylesshalf = y - s_half_block;
	ylessblock = y - blocksize; /* constants, for speed */
	yplushalf = y + s_half_block;
	yplusblock = y + blocksize;
	prev11 = -1;
	c24 = c12 = c13 = c22 = getcolor(g_ix_start, y);
	c31 = c21 = getcolor(g_ix_start, (y > 0) ? ylesshalf : 0);
	if (yplusblock <= g_y_stop)
	{
		c24 = getcolor(g_ix_start, yplusblock);
	}
	else if (!s_bottom_guess)
	{
		c24 = -1;
	}
	guessed12 = guessed13 = 0;

	for (int x = g_ix_start; x <= g_x_stop; )  /* increment at end, or when doing continue */
	{
		if ((x & (s_max_block-1)) == 0)  /* time for skip flag stuff */
		{
			++pfxptr;
			if (firstpass == 0 && (*pfxptr&pfxmask) == 0)  /* check for fast skip */
			{
				/* next useful in testing to make skips visible */
				/*
				if (s_half_block == 1)
				{
					(*g_plot_color)(x + 1, y, 0);
					(*g_plot_color)(x, y + 1, 0);
					(*g_plot_color)(x + 1, y + 1, 0);
				}
				*/
				x += s_max_block;
				prev11 = c31 = c21 = c24 = c12 = c13 = c22;
				guessed12 = guessed13 = 0;
				continue;
			}
		}

		if (firstpass)  /* 1st pass, paint topleft corner */
		{
			plot_block(0, x, y, c22);
		}
		/* setup variables */
		xplushalf = x + s_half_block;
		xplusblock = xplushalf + s_half_block;
		if (xplushalf > g_x_stop)
		{
			if (!s_right_guess)
			{
				c31 = -1;
			}
		}
		else if (y > 0)
		{
			c31 = getcolor(xplushalf, ylesshalf);
		}
		if (xplusblock <= g_x_stop)
		{
			if (yplusblock <= g_y_stop)
			{
				c44 = getcolor(xplusblock, yplusblock);
			}
			c41 = getcolor(xplusblock, (y > 0) ? ylesshalf : 0);
			c42 = getcolor(xplusblock, y);
		}
		else if (!s_right_guess)
		{
			c41 = c42 = c44 = -1;
		}
		if (yplusblock > g_y_stop)
		{
			c44 = s_bottom_guess ? c42 : -1;
		}

		/* guess or calc the remaining 3 quarters of current g_block */
		guessed23 = guessed32 = guessed33 = 1;
		c23 = c32 = c33 = c22;
		if (yplushalf > g_y_stop)
		{
			if (!s_bottom_guess)
			{
				c23 = c33 = -1;
			}
			guessed23 = guessed33 = -1;
			guessed13 = 0; /* fix for g_y_dots not divisible by four bug TW 2/16/97 */
		}
		if (xplushalf > g_x_stop)
		{
			if (!s_right_guess)
			{
				c32 = c33 = -1;
			}
			guessed32 = guessed33 = -1;
		}
		while (1) /* go around till none of 23, 32, 33 change anymore */
		{
			if (guessed33 > 0
				&& (c33 != c44 || c33 != c42 || c33 != c24 || c33 != c32 || c33 != c23))
			{
				calcadot(c33, xplushalf, yplushalf);
				guessed33 = 0;
			}
			if (guessed32 > 0
				&& (c32 != c33 || c32 != c42 || c32 != c31 || c32 != c21
					|| c32 != c41 || c32 != c23))
			{
				calcadot(c32, xplushalf, y);
				guessed32 = 0;
				continue;
			}
			if (guessed23 > 0
				&& (c23 != c33 || c23 != c24 || c23 != c13 || c23 != c12 || c23 != c32))
			{
				calcadot(c23, x, yplushalf);
				guessed23 = 0;
				continue;
			}
			break;
		}

		if (firstpass) /* note whether any of g_block's contents were calculated */
			if (guessed23 == 0 || guessed32 == 0 || guessed33 == 0)
			{
				*pfxptr |= pfxmask;
			}

		if (s_half_block > 1)  /* not last pass, check if something to display */
		{
			if (firstpass)  /* display guessed corners, fill in g_block */
			{
				if (s_guess_plot)
				{
					if (guessed23 > 0)
					{
						(*g_plot_color)(x, yplushalf, c23);
					}
					if (guessed32 > 0)
					{
						(*g_plot_color)(xplushalf, y, c32);
					}
					if (guessed33 > 0)
					{
						(*g_plot_color)(xplushalf, yplushalf, c33);
					}
				}
				plot_block(1, x, yplushalf, c23);
				plot_block(0, xplushalf, y, c32);
				plot_block(1, xplushalf, yplushalf, c33);
			}
			else  /* repaint changed blocks */
			{
				if (c23 != c22)
				{
					plot_block(-1, x, yplushalf, c23);
				}
				if (c32 != c22)
				{
					plot_block(-1, xplushalf, y, c32);
				}
				if (c33 != c22)
				{
					plot_block(-1, xplushalf, yplushalf, c33);
				}
			}
		}

		/* check if some calcs in this g_block mean earlier guesses need fixing */
		fix21 = ((c22 != c12 || c22 != c32)
			&& c21 == c22 && c21 == c31 && c21 == prev11
			&& y > 0
			&& (x == g_ix_start || c21 == getcolor(x-s_half_block, ylessblock))
			&& (xplushalf > g_x_stop || c21 == getcolor(xplushalf, ylessblock))
			&& c21 == getcolor(x, ylessblock));
		fix31 = (c22 != c32
			&& c31 == c22 && c31 == c42 && c31 == c21 && c31 == c41
			&& y > 0 && xplushalf <= g_x_stop
			&& c31 == getcolor(xplushalf, ylessblock)
			&& (xplusblock > g_x_stop || c31 == getcolor(xplusblock, ylessblock))
			&& c31 == getcolor(x, ylessblock));
		prev11 = c31; /* for next time around */
		if (fix21)
		{
			calcadot(c21, x, ylesshalf);
			if (s_half_block > 1 && c21 != c22)
			{
				plot_block(-1, x, ylesshalf, c21);
			}
		}
		if (fix31)
		{
			calcadot(c31, xplushalf, ylesshalf);
			if (s_half_block > 1 && c31 != c22)
			{
				plot_block(-1, xplushalf, ylesshalf, c31);
			}
		}
		if (c23 != c22)
		{
			if (guessed12)
			{
				calcadot(c12, x-s_half_block, y);
				if (s_half_block > 1 && c12 != c22)
				{
					plot_block(-1, x-s_half_block, y, c12);
				}
			}
			if (guessed13)
			{
				calcadot(c13, x-s_half_block, yplushalf);
				if (s_half_block > 1 && c13 != c22)
				{
					plot_block(-1, x-s_half_block, yplushalf, c13);
				}
			}
		}
		c22 = c42;
		c24 = c44;
		c13 = c33;
		c31 = c21 = c41;
		c12 = c32;
		guessed12 = guessed32;
		guessed13 = guessed33;
		x += blocksize;
	} /* end x loop */

	if (firstpass == 0 || s_guess_plot)
	{
		return 0;
	}

	/* paint rows the fast way */
	for (int i = 0; i < s_half_block; ++i)
	{
		j = y + i;
		if (j <= g_y_stop)
		{
			put_line(j, g_xx_start, g_x_stop, &g_stack[g_xx_start]);
		}
		j = y + i + s_half_block;
		if (j <= g_y_stop)
		{
			put_line(j, g_xx_start, g_x_stop, &g_stack[g_xx_start + OLD_MAX_PIXELS]);
		}
		if (driver_key_pressed())
		{
			return -1;
		}
	}
	if (g_plot_color != g_put_color)  /* symmetry, just vertical & origin the fast way */
	{
		if (g_plot_color == symplot2J) /* origin sym, reverse lines */
		{
			for (int i = (g_x_stop + g_xx_start + 1)/2; --i >= g_xx_start; )
			{
				color = g_stack[i];
				j = g_x_stop-(i-g_xx_start);
				g_stack[i] = g_stack[j];
				g_stack[j] = (BYTE)color;
				j += OLD_MAX_PIXELS;
				color = g_stack[i + OLD_MAX_PIXELS];
				g_stack[i + OLD_MAX_PIXELS] = g_stack[j];
				g_stack[j] = (BYTE)color;
			}
		}
		for (int i = 0; i < s_half_block; ++i)
		{
			j = g_yy_stop-(y + i-g_yy_start);
			if (j > g_y_stop && j < g_y_dots)
			{
				put_line(j, g_xx_start, g_x_stop, &g_stack[g_xx_start]);
			}
			j = g_yy_stop-(y + i + s_half_block-g_yy_start);
			if (j > g_y_stop && j < g_y_dots)
			{
				put_line(j, g_xx_start, g_x_stop, &g_stack[g_xx_start + OLD_MAX_PIXELS]);
			}
			if (driver_key_pressed())
			{
				return -1;
			}
		}
	}
	return 0;
}

/************************ super solid guessing *****************************/
int solid_guess()
{
	int i;
	int xlim;
	int ylim;
	int blocksize;
	unsigned int *pfxp0;
	unsigned int *pfxp1;
	unsigned int u;

	s_guess_plot = (g_plot_color != g_put_color && g_plot_color != symplot2 && g_plot_color != symplot2J);
	/* check if guessing at bottom & right edges is ok */
	s_bottom_guess = (g_plot_color == symplot2 || (g_plot_color == g_put_color && g_y_stop + 1 == g_y_dots));
	s_right_guess  = (g_plot_color == symplot2J
		|| ((g_plot_color == g_put_color || g_plot_color == symplot2) && g_x_stop + 1 == g_x_dots));

	/* there seems to be a bug in solid guessing at bottom and side */
	if (g_debug_flag != DEBUGFLAG_SOLID_GUESS_BR)
	{
		s_bottom_guess = FALSE;
		s_right_guess = FALSE;  /* TIW march 1995 */
	}

	blocksize = solid_guess_block_size();
	i = blocksize;
	s_max_block = blocksize;
	g_total_passes = 1;
	while ((i >>= 1) > 1)
	{
		++g_total_passes;
	}

	/* ensure window top and left are on required boundary, treat window
			as larger than it really is if necessary (this is the reason symplot
			routines must check for > g_x_dots/g_y_dots before plotting sym points) */
	g_ix_start &= -1 - (s_max_block - 1);
	g_iy_start = g_yy_begin;
	g_iy_start &= -1 - (s_max_block - 1);

	g_got_status = GOT_STATUS_GUESSING;

	if (g_work_pass == 0) /* otherwise first pass already done */
	{
		/* first pass, calc every blocksize**2 pixel, quarter result & paint it */
		g_current_pass = 1;
		if (g_iy_start <= g_yy_start) /* first time for this window, init it */
		{
			g_current_row = 0;
			memset(&s_t_prefix[1][0][0], 0, MAX_X_BLOCK*MAX_Y_BLOCK*2); /* noskip flags off */
			g_reset_periodicity = 1;
			g_row = g_iy_start;
			for (g_col = g_ix_start; g_col <= g_x_stop; g_col += s_max_block)
			{ /* calc top row */
				if ((*g_calculate_type)() == -1)
				{
					work_list_add(g_xx_start, g_xx_stop, g_xx_begin, g_yy_start, g_yy_stop, g_yy_begin, 0, g_work_sym);
					goto exit_solid_guess;
				}
				g_reset_periodicity = 0;
			}
		}
		else
		{
			memset(&s_t_prefix[1][0][0], -1, MAX_X_BLOCK*MAX_Y_BLOCK*2); /* noskip flags on */
		}
		for (int y = g_iy_start; y <= g_y_stop; y += blocksize)
		{
			g_current_row = y;
			i = 0;
			if (y + blocksize <= g_y_stop)
			{ /* calc the row below */
				g_row = y + blocksize;
				g_reset_periodicity = 1;
				for (g_col = g_ix_start; g_col <= g_x_stop; g_col += s_max_block)
				{
					i = (*g_calculate_type)();
					if (i == -1)
					{
						break;
					}
					g_reset_periodicity = 0;
				}
			}
			g_reset_periodicity = 0;
			if (i == -1 || guess_row(1, y, blocksize) != 0) /* interrupted? */
			{
				if (y < g_yy_start)
				{
					y = g_yy_start;
				}
				work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, y, 0, g_work_sym);
				goto exit_solid_guess;
			}
		}

		if (g_num_work_list) /* work list not empty, just do 1st pass */
		{
			work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, g_yy_start, 1, g_work_sym);
			goto exit_solid_guess;
		}
		++g_work_pass;
		g_iy_start = g_yy_start & (-1 - (s_max_block-1));

		/* calculate skip flags for skippable blocks */
		xlim = (g_x_stop + s_max_block)/s_max_block + 1;
		ylim = ((g_y_stop + s_max_block)/s_max_block + 15)/16 + 1;
		if (!s_right_guess) /* no right edge guessing, zap border */
		{
			for (int y = 0; y <= ylim; ++y)
			{
				s_t_prefix[1][y][xlim] = 0xffff;
			}
		}
		if (!s_bottom_guess) /* no bottom edge guessing, zap border */
		{
			i = (g_y_stop + s_max_block)/s_max_block + 1;
			int y = i/16 + 1;
			i = 1 << (i&15);
			for (int x = 0; x <= xlim; ++x)
			{
				s_t_prefix[1][y][x] |= i;
			}
		}
		/* set each bit in s_t_prefix[0] to OR of it & surrounding 8 in s_t_prefix[1] */
		for (int y = 0; ++y < ylim; )
		{
			pfxp0 = (unsigned int *)&s_t_prefix[0][y][0];
			pfxp1 = (unsigned int *)&s_t_prefix[1][y][0];
			for (int x = 0; ++x < xlim; )
			{
				++pfxp1;
				u = *(pfxp1-1)|*pfxp1|*(pfxp1 + 1);
				*(++pfxp0) = u | (u >> 1) | (u << 1)
					| ((*(pfxp1-(MAX_X_BLOCK + 1))
					| *(pfxp1-MAX_X_BLOCK)
					| *(pfxp1-(MAX_X_BLOCK-1))) >> 15)
					| ((*(pfxp1 + (MAX_X_BLOCK-1))
					| *(pfxp1 + MAX_X_BLOCK)
					| *(pfxp1 + (MAX_X_BLOCK + 1))) << 15);
			}
		}
	}
	else /* first pass already done */
	{
		memset(&s_t_prefix[0][0][0], -1, MAX_X_BLOCK*MAX_Y_BLOCK*2); /* noskip flags on */
	}
	if (g_three_pass)
	{
		goto exit_solid_guess;
	}

	/* remaining pass(es), halve blocksize & quarter each blocksize**2 */
	i = g_work_pass;
	while (--i > 0) /* allow for already done passes */
	{
		blocksize = blocksize >> 1;
	}
	g_reset_periodicity = 0;
	while ((blocksize = blocksize >> 1) >= 2)
	{
		if (g_stop_pass > 0)
		{
			if (g_work_pass >= g_stop_pass)
			{
				goto exit_solid_guess;
			}
		}
		g_current_pass = g_work_pass + 1;
		for (int y = g_iy_start; y <= g_y_stop; y += blocksize)
		{
			g_current_row = y;
			if (guess_row(0, y, blocksize) != 0)
			{
				if (y < g_yy_start)
				{
					y = g_yy_start;
				}
				work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, y, g_work_pass, g_work_sym);
				goto exit_solid_guess;
			}
		}
		++g_work_pass;
		if (g_num_work_list /* work list not empty, do one pass at a time */
			&& blocksize > 2) /* if 2, we just did last pass */
		{
			work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, g_yy_start, g_work_pass, g_work_sym);
			goto exit_solid_guess;
		}
		g_iy_start = g_yy_start & (-1 - (s_max_block-1));
	}

	exit_solid_guess:
	return 0;
}
