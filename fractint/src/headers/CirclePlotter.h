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
	CirclePlotter(IPlotter &plotter) : s_x_base(0), s_y_base(0), s_x_aspect(0), s_y_aspect(0),
		_plotter(plotter)
	{
	}

	void circle(int radius, int color);
	void SetBase(int x, int y)
	{
		s_x_base = x;
		s_y_base = y;
	}
	void SetAspect(double aspect);

private:
	int s_x_base;
	int s_y_base;
	unsigned int s_x_aspect;
	unsigned int s_y_aspect;
	IPlotter &_plotter;

	void circle_plot(int x, int y, int color);
	void plot8(int x, int y, int color);
};

#endif
