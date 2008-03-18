#pragma once

enum
{
	NUM_BOXES = 4096
};

class ZoomBox
{
public:
	virtual ~ZoomBox() {}

	virtual void display() = 0;
	virtual void clear() = 0;
	virtual void push(const Coordinate &point) = 0;

	virtual void save(int *x, int *y, int *values, int count) = 0;
	virtual void restore(const int *x, const int *y, const int *values, int count) = 0;

	virtual int color() const = 0;
	virtual int count() const = 0;

	virtual void set_color(int val) = 0;
	virtual void set_count(int val) = 0;
};

extern ZoomBox &g_zoomBox;
