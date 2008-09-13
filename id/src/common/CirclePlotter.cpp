#include <cmath>
#include "CirclePlotter.h"

void CirclePlotter::circle_plot(int x, int y, int color)
{
	if (_x_aspect == 0)
	{
		if (_y_aspect == 0)
		{
			_plotter.plot(x + _x_base, y + _y_base, color);
		}
		else
		{
			_plotter.plot(x + _x_base, short(_y_base + ((long(y)*long(_y_aspect)) >> 16)), color);
		}
	}
	else
	{
		_plotter.plot(int(_x_base + ((long(x)*long(_x_aspect)) >> 16)), y + _y_base, color);
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
	_x_aspect = 0;
	_y_aspect = 0;
	aspect = std::abs(aspect);
	if (aspect != 1.0)
	{
		if (aspect > 1.0)
		{
			_y_aspect = (unsigned int)(65536.0/aspect);
		}
		else
		{
			_x_aspect = (unsigned int)(65536.0*aspect);
		}
	}
}

