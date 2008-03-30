#include <string>
#include <vector>

#include "port.h"
#include "prototyp.h"

#include "drivers.h"
#include "ZoomBox.h"

class ZoomBoxImpl : public ZoomBox
{
public:
	virtual ~ZoomBoxImpl() { }

	virtual void display();
	virtual void clear();
	virtual void push(const Coordinate &point);

	virtual void save(int *x, int *y, int *values, int count);
	virtual void restore(const int *x, const int *y, const int *values, int count);

	virtual int color() const { return _box_color; }
	virtual int count() const { return _box_count; }

	virtual void set_color(int val) { _box_color = val; }
	virtual void set_count(int val) { _box_count = val; }

private:
	int _box_x[NUM_BOXES];
	int _box_y[NUM_BOXES];
	int _box_values[NUM_BOXES];
	int _box_color;
	int	_box_count;
};

static ZoomBoxImpl s_zoomBox;
ZoomBox &g_zoomBox(s_zoomBox);

static std::vector<unsigned char> s_values;

void ZoomBoxImpl::display()
{
	int i;
	int boxColor = (g_colors - 1) & color();
	int rgb[3];
	s_values.resize(count());
	for (i = 0; i < count(); i++)
	{
		if (g_is_true_color && g_true_mode_iterates)
		{
			int alpha = 0;
			driver_get_truecolor(_box_x[i] - g_screen_x_offset, _box_y[i] - g_screen_y_offset,
				rgb[0], rgb[1], rgb[2], alpha);
			driver_put_truecolor(_box_x[i] - g_screen_x_offset, _box_y[i] - g_screen_y_offset,
				rgb[0]^255, rgb[1]^255, rgb[2]^255, 255);
		}
		else
		{
			s_values[i] = (unsigned char) get_color(_box_x[i] - g_screen_x_offset, _box_y[i] - g_screen_y_offset);
		}
	}
	// There is an interaction between get_color and g_plot_color_put_color, so separate them
	if (!(g_is_true_color && g_true_mode_iterates)) // don't need this for truecolor with truemode set
	{
		for (i = 0; i < count(); i++)
		{
			g_plot_color_put_color(_box_x[i] - g_screen_x_offset, _box_y[i] - g_screen_y_offset, boxColor);
		}
	}
}

void ZoomBoxImpl::clear()
{
	int i;
	if (g_is_true_color && g_true_mode_iterates)
	{
		display();
	}
	else
	{
		for (i = 0; i < g_zoomBox.count(); i++)
		{
			g_plot_color_put_color(_box_x[i]-g_screen_x_offset, _box_y[i]-g_screen_y_offset, s_values[i]);
		}
	}
}

void ZoomBoxImpl::push(const Coordinate &point)
{
	_box_x[_box_count] = point.x;
	_box_y[_box_count] = point.y;
	_box_count++;
}

void ZoomBoxImpl::save(int *destx, int *desty, int *destvalues, int count)
{
	const int bytes = count*sizeof(int);
	memcpy(destx, _box_x, bytes);
	memcpy(desty, _box_y, bytes);
	memcpy(destvalues, _box_values, bytes);
}

void ZoomBoxImpl::restore(const int *x, const int *y, const int *values, int count)
{
	const int bytes = count*sizeof(int);
	memcpy(_box_x, x, bytes);
	memcpy(_box_y, y, bytes);
	memcpy(_box_values, values, bytes);
}
