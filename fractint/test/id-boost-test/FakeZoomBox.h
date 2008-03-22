#pragma once

#include "ZoomBox.h"
#include "NotImplementedException.h"

class FakeZoomBox : public ZoomBox
{
public:
	FakeZoomBox() : ZoomBox(),
		_setCountCalled(false),
		_setCountLastValue(0)
	{
	}
	virtual ~FakeZoomBox() { }

	virtual void display() { throw not_implemented("display"); }
	virtual void clear() { throw not_implemented("clear"); }
	virtual void push(const Coordinate &point) { throw not_implemented("push"); }

	virtual void save(int *x, int *y, int *values, int count) { throw not_implemented("save"); }
	virtual void restore(const int *x, const int *y, const int *values, int count) { throw not_implemented("restore"); }

	virtual int color() const { throw not_implemented("color"); }
	virtual int count() const { throw not_implemented("count"); }

	virtual void set_color(int val) { throw not_implemented("set_color"); }
	virtual void set_count(int val)				{ _setCountCalled = true; _setCountLastValue = val; }
	bool SetCountCalled() const					{ return _setCountCalled; }
	int SetCountLastValue() const				{ return _setCountLastValue; }

private:
	bool _setCountCalled;
	int _setCountLastValue;
};
