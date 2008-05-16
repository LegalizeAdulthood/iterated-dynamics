#include <cmath>
#include "CirclePlotter.h"

void CirclePlotter::circle_plot(int x, int y, int color)
{
	if (s_x_aspect == 0)
	{
		if (s_y_aspect == 0)
		{
			_plotter.plot(x + s_x_base, y + s_y_base, color);
		}
		else
		{
			_plotter.plot(x + s_x_base, short(s_y_base + ((long(y)*long(s_y_aspect)) >> 16)), color);
		}
	}
	else
	{
		_plotter.plot(int(s_x_base + ((long(x)*long(s_x_aspect)) >> 16)), y + s_y_base, color);
	}
}

void CirclePlotter::plot8(int x, int y, int color)
{
	circle_plot(x, y, color);
	circle_plot(-x, y, color);
	circle_plot(x, -y, color);
	circle_plot(-x, -y, color);
	circle_plot(y, x, color);
	circle_plot(-y, x, color);
	circle_plot(y, -x, color);
	circle_plot(-y, -x, color);
}

void CirclePlotter::circle(int radius, int color)
{
	int x;
	int y;
	int sum;

	x = 0;
	y = radius << 1;
	sum = 0;

	while (x <= y)
	{
		if (!(x & 1))   // plot if x is even
		{
			plot8(x >> 1, (y + 1) >> 1, color);
		}
		sum += (x << 1) + 1;
		x++;
		if (sum > 0)
		{
			sum -= (y << 1) - 1;
			y--;
		}
	}
}

void CirclePlotter::SetAspect(double aspect)
{
	s_x_aspect = 0;
	s_y_aspect = 0;
	aspect = std::abs(aspect);
	if (aspect != 1.0)
	{
		if (aspect > 1.0)
		{
			s_y_aspect = (unsigned int)(65536.0/aspect);
		}
		else
		{
			s_x_aspect = (unsigned int)(65536.0*aspect);
		}
	}
}

