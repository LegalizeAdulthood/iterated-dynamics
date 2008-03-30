#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "id.h"
#include "cmplx.h"
#include "externs.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "Externals.h"
#include "fracsubr.h"
#include "miscres.h"

#include "DiffusionScan.h"
#include "WorkList.h"

static void diffusion_scan();

class DiffusionScanImpl : public DiffusionScan
{
public:
	virtual ~DiffusionScanImpl() { }

	virtual void Scan();
	virtual std::string CalculationTime() const;
	virtual std::string Status() const;

private:
	int Engine();

	static unsigned long s_diffusionLimit;
	// number of bits in the counter
	static unsigned int s_bits;
	// the diffusion counter
	static unsigned long s_diffusionCounter;
};

static DiffusionScanImpl s_diffusionScanImpl;
DiffusionScan &g_diffusionScan(s_diffusionScanImpl);

// vars for diffusion scan
unsigned long DiffusionScanImpl::s_diffusionLimit;
unsigned int DiffusionScanImpl::s_bits = 0;
unsigned long DiffusionScanImpl::s_diffusionCounter = 0;

// lookup tables to avoid too much bit fiddling :
static char const s_diffusion_la[] =
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

static char const s_diffusion_lb[] =
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

// Calculate the point
static bool diffusion_point(int row, int col)
{
	g_reset_periodicity = true;
	if (g_calculate_type() == -1)
	{
		return true;
	}
	g_reset_periodicity = false;
	g_plot_color(col, row, g_color);
	return false;
}

// little function that plots a filled square of color c, size s with
// top left cornet at (x, y) with optimization from sym_fill_line
static void diffusion_plot_block(int x, int y, int size, int color)
{
	static std::vector<BYTE> scanline;
	scanline.resize(size);
	std::fill(scanline.begin(), scanline.end(), BYTE(color));
	for (int ty = y; ty < y + size; ty++)
	{
		sym_fill_line(ty, x, x + size - 1, &scanline[0]);
	}
}

static bool diffusion_block(int row, int col, int sqsz)
{
	g_reset_periodicity = true;
	if (g_calculate_type() == -1)
	{
		return true;
	}
	g_reset_periodicity = false;
	diffusion_plot_block(col, row, sqsz, g_color);
	return false;
}

// function that does the same as above, but checks the limits in x and y
static void plot_block_lim(int x, int y, int size, int color)
{
	static std::vector<BYTE> scanline;
	scanline.resize(size);
	std::fill(scanline.begin(), scanline.end(), BYTE(color));
	for (int ty = y; ty < std::min(y + size, g_y_stop + 1); ty++)
	{
		sym_fill_line(ty, x, std::min(x + size - 1, g_x_stop), &scanline[0]);
	}
}

static bool diffusion_block_lim(int row, int col, int sqsz)
{
	g_reset_periodicity = true;
	if (g_calculate_type() == -1)
	{
		return true;
	}
	g_reset_periodicity = false;
	plot_block_lim(col, row, sqsz, g_color);
	return false;
}

int DiffusionScanImpl::Engine()
{
	double log2 = double(log(2.0));
	int i;
	int j;
	int nx;
	int ny; // number of tiles to build in x and y dirs
	// made this to complete the area that is not
	// a square with sides like 2 ** n
	int rem_x;
	int rem_y; // what is left on the last tile to draw
	int dif_offset; // offset for adjusting looked-up values
	int sqsz;  // size of the block being filled
	int colo;
	int rowo; // original col and row
	int s = 1 << (s_bits/2); // size of the square

	nx = int(floor(double(g_x_stop - g_ix_start + 1)/s));
	ny = int(floor(double(g_y_stop - g_iy_start + 1)/s));

	rem_x = (g_x_stop - g_ix_start + 1) - nx*s;
	rem_y = (g_y_stop - g_iy_start + 1) - ny*s;

	if (g_WorkList.yy_begin() == g_iy_start && g_work_pass == 0)  // if restarting on pan:
	{
		s_diffusionCounter = 0L;
	}
	else
	{
		// g_WorkList.yy_begin() and passes contain data for resuming the type:
		s_diffusionCounter = ((long((unsigned) g_WorkList.yy_begin())) << 16) | ((unsigned) g_work_pass);
	}

	dif_offset = 12-(s_bits/2); // offset to adjust coordinates
				// (*) for 4 bytes use 16 for 3 use 12 etc.

	// only the points (dithering only) :
	if (g_fill_color == 0 )
	{
		while (s_diffusionCounter < (s_diffusionLimit >> 1))
		{
			count_to_int(dif_offset, s_diffusionCounter, &colo, &rowo);
			i = 0;
			g_col = g_ix_start + colo; // get the right tiles
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
					g_row += s;                  // next tile
				}
				while (j < ny);
				// in the last y tile we may not need to plot the point
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
			// in the last x tiles we may not need to plot the point
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
					g_row += s; // next tile
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
			s_diffusionCounter++;
		}
	}
	else
	{
		// with progressive filling :
		while (s_diffusionCounter < (s_diffusionLimit >> 1))
		{
			sqsz = 1 << (int(s_bits - int(log(s_diffusionCounter + 0.5)/log2 )-1)/2 );
			count_to_int(dif_offset, s_diffusionCounter, &colo, &rowo);

			i = 0;
			do
			{
				j = 0;
				do
				{
					g_col = g_ix_start + colo + i*s; // get the right tiles
					g_row = g_iy_start + rowo + j*s;

					if (diffusion_block(g_row, g_col, sqsz))
					{
						return -1;
					}
					j++;
				}
				while (j < ny);
				// in the last tile we may not need to plot the point
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
			// in the last tile we may not need to plot the point
			if (colo < rem_x)
			{
				g_col = g_ix_start + colo + nx*s;
				j = 0;
				do
				{
					g_row = g_iy_start + rowo + j*s; // get the right tiles
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

			s_diffusionCounter++;
		}
	}
	// from half g_diffusion_limit on we only plot 1x1 points :-)
	while (s_diffusionCounter < s_diffusionLimit)
	{
		count_to_int(dif_offset, s_diffusionCounter, &colo, &rowo);

		i = 0;
		do
		{
			j = 0;
			do
			{
				g_col = g_ix_start + colo + i*s; // get the right tiles
				g_row = g_iy_start + rowo + j*s;
				if (diffusion_point(g_row, g_col))
				{
					return -1;
				}
				j++;
			}
			while (j < ny);
			// in the last tile we may not need to plot the point
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
		// in the last tile we may nnt need to plot the point
		if (colo < rem_x)
		{
			g_col = g_ix_start + colo + nx*s;
			j = 0;
			do
			{
				g_row = g_iy_start + rowo + j*s; // get the right tiles
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
		s_diffusionCounter++;
	}

	return 0;
}

inline double log_length(int start, int stop)
{
	return log(double(stop - start + 1));
}

void DiffusionScanImpl::Scan()
{
	double log2 = double(log(2.0));

	g_externs.SetTabStatus(TAB_STATUS_DIFFUSION);

	// note: the max size of 2048x2048 gives us a 22 bit counter that will
	// fit any 32 bit architecture, the maxinum limit for this case would
	// be 65536x65536 (HB)

	s_bits = unsigned(std::min(log_length(g_iy_start, g_y_stop),
								log_length(g_ix_start, g_x_stop))/log2);
	s_bits <<= 1; // double for two axes
	s_diffusionLimit = 1l << s_bits;

	if (Engine() == -1)
	{
		g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_WorkList.xx_start(),
			g_WorkList.yy_start(), g_WorkList.yy_stop(),
			int(s_diffusionCounter >> 16),            // high,
			int(s_diffusionCounter & 0xffff),         // low order words
			g_work_sym);
	}
}

std::string DiffusionScanImpl::CalculationTime() const
{
	char buffer[80];
	get_calculation_time(buffer, long(g_calculation_time*((s_diffusionLimit*1.0)/s_diffusionCounter)));
	return buffer;
}

std::string DiffusionScanImpl::Status() const
{
	return str(boost::format("%2.2f%% done, counter at %lu of %lu (%u bits)")
		% ((100.0*s_diffusionCounter)/s_diffusionLimit)
		% s_diffusionCounter
		% s_diffusionLimit
		% s_bits);
}
