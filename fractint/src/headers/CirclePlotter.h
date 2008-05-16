#if !defined(CIRCLE_PLOTTER_H)
#define CIRCLE_PLOTTER_H

class IPlotter
{
public:
	virtual ~IPlotter() { }

	virtual void plot(int x, int y, int color) = 0;
};

// circle routines from Dr. Dobbs June 1990
class CirclePlotter
{
public:
	CirclePlotter(IPlotter &plotter) : _x_base(0), _y_base(0), _x_aspect(0), _y_aspect(0),
		_plotter(plotter)
	{
	}

	void circle(int radius, int color);
	void SetBase(int x, int y)
	{
		_x_base = x;
		_y_base = y;
	}
	void SetAspect(double aspect);

private:
	int _x_base;
	int _y_base;
	unsigned int _x_aspect;
	unsigned int _y_aspect;
	IPlotter &_plotter;

	void circle_plot(int x, int y, int color);
	void plot8(int x, int y, int color);
};

#endif
