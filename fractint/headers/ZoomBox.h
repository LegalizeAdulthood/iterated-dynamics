#if !defined(ZOOM_BOX_H)
#define ZOOM_BOX_H

enum
{
	NUM_BOXES = 4096
};

class ZoomBox
{
public:
	void display();
	void clear();
	void push(const Coordinate &point);

	void save(int *x, int *y, int *values, int count);
	void restore(const int *x, const int *y, const int *values, int count);

	int color() const { return _box_color; }
	int count() const { return _box_count; }

	void set_color(int val) { _box_color = val; }
	void set_count(int val) { _box_count = val; }

private:
	int _box_x[NUM_BOXES];
	int _box_y[NUM_BOXES];
	int _box_values[NUM_BOXES];
	int _box_color;
	int	_box_count;
};

extern ZoomBox g_zoomBox;

#endif
