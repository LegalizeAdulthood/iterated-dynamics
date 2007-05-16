#include <cstring>

#include "port.h"
#include "fractint.h"
#include "cmplx.h"
#include "externs.h"
#include "prototyp.h"
#include "DiffusionScan.h"
#include "WorkList.h"

/* vars for diffusion scan */
unsigned long g_diffusion_limit; 	/* the diffusion counter */

static unsigned s_bits = 0; 		/* number of bits in the counter */
static unsigned long s_diffusion_counter; 	/* the diffusion counter */

/* lookup tables to avoid too much bit fiddling : */
static char s_diffusion_la[] =
{
	0, 8, 0, 8,4,12,4,12,0, 8, 0, 8,4,12,4,12, 2,10, 2,10,6,14,6,14,2,10,
	2,10, 6,14,6,14,0, 8,0, 8, 4,12,4,12,0, 8, 0, 8, 4,12,4,12,2,10,2,10,
	6,14, 6,14,2,10,2,10,6,14, 6,14,1, 9,1, 9, 5,13, 5,13,1, 9,1, 9,5,13,
	5,13, 3,11,3,11,7,15,7,15, 3,11,3,11,7,15, 7,15, 1, 9,1, 9,5,13,5,13,
	1, 9, 1, 9,5,13,5,13,3,11, 3,11,7,15,7,15, 3,11, 3,11,7,15,7,15,0, 8,
	0, 8, 4,12,4,12,0, 8,0, 8, 4,12,4,12,2,10, 2,10, 6,14,6,14,2,10,2,10,
	6,14, 6,14,0, 8,0, 8,4,12, 4,12,0, 8,0, 8, 4,12, 4,12,2,10,2,10,6,14,
	6,14, 2,10,2,10,6,14,6,14, 1, 9,1, 9,5,13, 5,13, 1, 9,1, 9,5,13,5,13,
	3,11, 3,11,7,15,7,15,3,11, 3,11,7,15,7,15, 1, 9, 1, 9,5,13,5,13,1, 9,
	1, 9, 5,13,5,13,3,11,3,11, 7,15,7,15,3,11, 3,11, 7,15,7,15
};

static char s_diffusion_lb[] =
{
	0, 8, 8, 0, 4,12,12, 4, 4,12,12, 4, 8, 0, 0, 8, 2,10,10, 2, 6,14,14,
	6, 6,14,14, 6,10, 2, 2,10, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2,
	2,10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 1, 9, 9, 1, 5,
	13,13, 5, 5,13,13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,
	11, 3, 3,11, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13,
	5, 9, 1, 1, 9, 9, 1, 1, 9,13, 5, 5,13, 1, 9, 9, 1, 5,13,13, 5, 5,13,
	13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 3,
	11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13, 5, 9, 1, 1, 9,
	9, 1, 1, 9,13, 5, 5,13, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2, 2,
	10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 4,12,12, 4, 8, 0,
	0, 8, 8, 0, 0, 8,12, 4, 4,12, 6,14,14, 6,10, 2, 2,10,10, 2, 2,10,14,
	6, 6,14
};

/* function: count_to_int(dif_offset, g_diffusion_counter, colo, rowo) */
static void count_to_int(int dif_offset, unsigned long C, int *x, int *y)
{
	*x = s_diffusion_la[C & 0xFF];
	*y = s_diffusion_lb[C & 0xFF];
	C >>= 8;
	*x <<= 4;
	*x += s_diffusion_la[C & 0xFF];
	*y <<= 4;
	*y += s_diffusion_lb[C & 0xFF];
	C >>= 8;
	*x <<= 4;
	*x += s_diffusion_la[C & 0xFF];
	*y <<= 4;
	*y += s_diffusion_lb[C & 0xFF];
	C >>= 8;
	*x >>= dif_offset;
	*y >>= dif_offset;
}

/* REMOVED: counter byte 3 */                                                          \
/* (x) < <= 4; (x) += s_diffusion_la[tC&0(x)FF]; (y) < <= 4; (y) += s_diffusion_lb[tC&0(x)FF]; tC >>= 8;
	--> eliminated this and made (*) because fractint user coordinates up to
	2048(x)2048 what means a counter of 24 bits or 3 bytes */

/* Calculate the point */
static int diffusion_point(int row, int col)
{
	g_reset_periodicity = 1;
	if ((*g_calculate_type)() == -1)
	{
		return TRUE;
	}
	g_reset_periodicity = 0;
	(*g_plot_color)(col, row, g_color);
	return FALSE;
}

/* little function that plots a filled square of color c, size s with
	top left cornet at (x, y) with optimization from sym_fill_line */
static void diffusion_plot_block(int x, int y, int s, int c)
{
	memset(g_stack, c, s);
	for (int ty = y; ty < y + s; ty++)
	{
		sym_fill_line(ty, x, x + s - 1, g_stack);
	}
}

static int diffusion_block(int row, int col, int sqsz)
{
	g_reset_periodicity = 1;
	if ((*g_calculate_type)() == -1)
	{
		return TRUE;
	}
	g_reset_periodicity = 0;
	diffusion_plot_block(col, row, sqsz, g_color);
	return FALSE;
}

/* function that does the same as above, but checks the limits in x and y */
static void plot_block_lim(int x, int y, int s, int c)
{
	memset(g_stack, (c), (s));
	for (int ty = y; ty < min(y + s, g_y_stop + 1); ty++)
	{
		sym_fill_line(ty, x, min(x + s - 1, g_x_stop), g_stack);
	}
}

static int diffusion_block_lim(int row, int col, int sqsz)
{
	g_reset_periodicity = 1;
	if ((*g_calculate_type)() == -1)
	{
		return TRUE;
	}
	g_reset_periodicity = 0;
	plot_block_lim(col, row, sqsz, g_color);
	return FALSE;
}

static int diffusion_engine()
{
	double log2 = (double) log (2.0);
	int i;
	int j;
	int nx;
	int ny; /* number of tiles to build in x and y dirs */
	/* made this to complete the area that is not */
	/* a square with sides like 2 ** n */
	int rem_x;
	int rem_y; /* what is left on the last tile to draw */
	int dif_offset; /* offset for adjusting looked-up values */
	int sqsz;  /* size of the block being filled */
	int colo;
	int rowo; /* original col and row */
	int s = 1 << (s_bits/2); /* size of the square */

	nx = (int) floor((double) (g_x_stop - g_ix_start + 1)/s );
	ny = (int) floor((double) (g_y_stop - g_iy_start + 1)/s );

	rem_x = (g_x_stop - g_ix_start + 1) - nx*s;
	rem_y = (g_y_stop - g_iy_start + 1) - ny*s;

	if (g_yy_begin == g_iy_start && g_work_pass == 0)  /* if restarting on pan: */
	{
		s_diffusion_counter = 0L;
	}
	else
	{
		/* g_yy_begin and passes contain data for resuming the type: */
		s_diffusion_counter = (((long) ((unsigned) g_yy_begin)) << 16) | ((unsigned) g_work_pass);
	}

	dif_offset = 12-(s_bits/2); /* offset to adjust coordinates */
				/* (*) for 4 bytes use 16 for 3 use 12 etc. */

	/*************************************/
	/* only the points (dithering only) :*/
	if (g_fill_color == 0 )
	{
		while (s_diffusion_counter < (g_diffusion_limit >> 1))
		{
			count_to_int(dif_offset, s_diffusion_counter, &colo, &rowo);
			i = 0;
			g_col = g_ix_start + colo; /* get the right tiles */
			do
			{
				j = 0;
				g_row = g_iy_start + rowo;
				do
				{
					if (diffusion_point(g_row, g_col))
					{
						return -1;
					}
					j++;
					g_row += s;                  /* next tile */
				}
				while (j < ny);
				/* in the last y tile we may not need to plot the point */
				if (rowo < rem_y)
				{
					if (diffusion_point(g_row, g_col))
					{
						return -1;
					}
				}
				i++;
				g_col += s;
			}
			while (i < nx);
			/* in the last x tiles we may not need to plot the point */
			if (colo < rem_x)
			{
				g_row = g_iy_start + rowo;
				j = 0;
				do
				{
					if (diffusion_point(g_row, g_col))
					{
						return -1;
					}
					j++;
					g_row += s; /* next tile */
				}
				while (j < ny);
				if (rowo < rem_y)
				{
					if (diffusion_point(g_row, g_col))
					{
						return -1;
					}
				}
			}
			s_diffusion_counter++;
		}
	}
	else
	{
		/*********************************/
		/* with progressive filling :    */
		while (s_diffusion_counter < (g_diffusion_limit >> 1))
		{
			sqsz = 1 << ((int) (s_bits - (int) (log(s_diffusion_counter + 0.5)/log2 )-1)/2 );
			count_to_int(dif_offset, s_diffusion_counter, &colo, &rowo);

			i = 0;
			do
			{
				j = 0;
				do
				{
					g_col = g_ix_start + colo + i*s; /* get the right tiles */
					g_row = g_iy_start + rowo + j*s;

					if (diffusion_block(g_row, g_col, sqsz))
					{
						return -1;
					}
					j++;
				}
				while (j < ny);
				/* in the last tile we may not need to plot the point */
				if (rowo < rem_y)
				{
					g_row = g_iy_start + rowo + ny*s;
					if (diffusion_block(g_row, g_col, sqsz))
					{
						return -1;
					}
				}
				i++;
			}
			while (i < nx);
			/* in the last tile we may not need to plot the point */
			if (colo < rem_x)
			{
				g_col = g_ix_start + colo + nx*s;
				j = 0;
				do
				{
					g_row = g_iy_start + rowo + j*s; /* get the right tiles */
					if (diffusion_block_lim(g_row, g_col, sqsz))
					{
						return -1;
					}
					j++;
				}
				while (j < ny);
				if (rowo < rem_y)
				{
					g_row = g_iy_start + rowo + ny*s;
					if (diffusion_block_lim(g_row, g_col, sqsz))
					{
						return -1;
					}
				}
			}

			s_diffusion_counter++;
		}
	}
	/* from half g_diffusion_limit on we only plot 1x1 points :-) */
	while (s_diffusion_counter < g_diffusion_limit)
	{
		count_to_int(dif_offset, s_diffusion_counter, &colo, &rowo);

		i = 0;
		do
		{
			j = 0;
			do
			{
				g_col = g_ix_start + colo + i*s; /* get the right tiles */
				g_row = g_iy_start + rowo + j*s;
				if (diffusion_point(g_row, g_col))
				{
					return -1;
				}
				j++;
			}
			while (j < ny);
			/* in the last tile we may not need to plot the point */
			if (rowo < rem_y)
			{
				g_row = g_iy_start + rowo + ny*s;
				if (diffusion_point(g_row, g_col))
				{
					return -1;
				}
			}
			i++;
		}
		while (i < nx);
		/* in the last tile we may nnt need to plot the point */
		if (colo < rem_x)
		{
			g_col = g_ix_start + colo + nx*s;
			j = 0;
			do
			{
				g_row = g_iy_start + rowo + j*s; /* get the right tiles */
				if (diffusion_point(g_row, g_col))
				{
					return -1;
				}
				j++;
			}
			while (j < ny);
			if (rowo < rem_y)
			{
				g_row = g_iy_start + rowo + ny*s;
				if (diffusion_point(g_row, g_col))
				{
					return -1;
				}
			}
		}
		s_diffusion_counter++;
	}

	return 0;
}

int diffusion_scan()
{
	double log2;

	log2 = (double) log (2.0);

	g_got_status = GOT_STATUS_DIFFUSION;

	/* note: the max size of 2048x2048 gives us a 22 bit counter that will */
	/* fit any 32 bit architecture, the maxinum limit for this case would  */
	/* be 65536x65536 (HB) */

	s_bits = (unsigned) (min(log((double) (g_y_stop - g_iy_start + 1)),
							 log((double) (g_x_stop - g_ix_start + 1)))/log2);
	s_bits <<= 1; /* double for two axes */
	g_diffusion_limit = 1l << s_bits;

	if (diffusion_engine() == -1)
	{
		work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop,
			(int) (s_diffusion_counter >> 16),            /* high, */
			(int) (s_diffusion_counter & 0xffff),         /* low order words */
			g_work_sym);
		return -1;
	}

	return 0;
}

void diffusion_get_calculation_time(char *msg)
{
	get_calculation_time(msg, (long)(g_calculation_time*((g_diffusion_limit*1.0)/s_diffusion_counter)));
}

void diffusion_get_status(char *msg)
{
	sprintf(msg, "%2.2f%% done, counter at %lu of %lu (%u bits)",
			(100.0*s_diffusion_counter)/g_diffusion_limit,
			s_diffusion_counter, g_diffusion_limit, s_bits);
}
