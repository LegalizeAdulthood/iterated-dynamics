#include <cstring>
#include <string>
#include <vector>

#include "port.h"
#include "id.h"
#include "prototyp.h"
#include "externs.h"

#include "calcfrac.h"
#include "fracsubr.h"
#include "realdos.h"

#include "BoundaryTrace.h"
#include "WorkList.h"

// boundary trace method

class IBoundaryTrace
{
public:
	virtual ~IBoundaryTrace() { }

	virtual int Execute() = 0;
};

class IBoundaryTraceApp
{
public:
	virtual ~IBoundaryTraceApp() { }

	virtual int stop_message(int flags, const std::string &msg) = 0;
	virtual int get_color(int xdot, int ydot) = 0;
	virtual int calculate_type() = 0;
	virtual void plot_color(int x, int y, int color) = 0;
	virtual void sym_fill_line(int row, int left, int right, BYTE const *str) = 0;
};

class IBoundaryTraceExterns
{
public:
	virtual ~IBoundaryTraceExterns() { }

	virtual int Row() const = 0;
	virtual void SetRow(int value) = 0;
	virtual int Column() const = 0;
	virtual void SetColumn(int value) = 0;
	virtual int Inside() const = 0;
	virtual int Outside() const = 0;
	virtual void SetGotStatus(int value) = 0;
	virtual int IXStart() const = 0;
	virtual int IYStart() const = 0;
	virtual int XStop() const = 0;
	virtual int YStop() const = 0;
	virtual void SetYStop(int value) = 0;
	virtual void SetResetPeriodicity(bool value) = 0;
	virtual int Color() const = 0;
	virtual void SetColor(int value) = 0;
	virtual int ShowDot() const = 0;
	virtual int WorkSymmetry() const = 0;
	virtual int FillColor() const = 0;
	virtual WorkList const &GetWorkList() const = 0;
	virtual WorkList &GetWorkList() = 0;
};

class BoundaryTraceApp : public IBoundaryTraceApp
{
public:
	virtual ~BoundaryTraceApp() { }

	virtual int stop_message(int flags, const std::string &message)
	{ return ::stop_message(flags, message); }
	virtual int get_color(int xdot, int ydot)
	{ return ::get_color(xdot, ydot); }
	virtual int calculate_type()
	{ return g_calculate_type(); }
	virtual void plot_color(int x, int y, int color)
	{ g_plot_color(x, y, color); }
	virtual void sym_fill_line(int row, int left, int right, BYTE const *str)
	{ ::sym_fill_line(row, left, right, str); }
};

class BoundaryTraceExterns : public IBoundaryTraceExterns
{
public:
	virtual ~BoundaryTraceExterns() { }

	virtual int Row() const						{ return g_row; }
	virtual void SetRow(int value)				{ g_row = value; }
	virtual int Column() const					{ return g_col; }
	virtual void SetColumn(int value)			{ g_col = value; }
	virtual int Inside() const					{ return g_inside; }
	virtual int Outside() const					{ return g_outside; }
	virtual void SetGotStatus(int value)		{ g_got_status = value; }
	virtual int IXStart() const					{ return g_ix_start; }
	virtual int IYStart() const					{ return g_iy_start; }
	virtual int XStop() const					{ return g_x_stop; }
	virtual int YStop() const					{ return g_y_stop; }
	virtual void SetYStop(int value)			{ g_y_stop = value; }
	virtual void SetResetPeriodicity(bool value) { g_reset_periodicity = value; }
	virtual int Color() const					{ return g_color; }
	virtual void SetColor(int value)			{ g_color = value; }
	virtual int ShowDot() const					{ return g_show_dot; }
	virtual int WorkSymmetry() const			{ return g_work_sym; }
	virtual int FillColor() const				{ return g_fill_color; }
	virtual WorkList const &GetWorkList() const	{ return g_WorkList; }
	virtual WorkList &GetWorkList()				{ return g_WorkList; }
};

class BoundaryTrace : public IBoundaryTrace
{
public:
	BoundaryTrace(IBoundaryTraceApp &app, IBoundaryTraceExterns &externs)
		: _trailRow(0), _trailCol(0),
		_goingTo(North),
		_app(app),
		_externs(externs)
	{
	}
	virtual ~BoundaryTrace() { }
	virtual int Execute();

private:
	enum
	{
		BACKGROUND_COLOR = 0
	};
	enum Direction
	{
		North, East, South, West
	};
	Direction advance(Direction value)
	{
		return Direction((value - 1) & 0x3);
	}
	void advance_match(Direction &coming_from)
	{
		_goingTo = advance(_goingTo);
		coming_from = advance(_goingTo);
	}
	void advance_no_match()
	{
		_goingTo = Direction((_goingTo + 1) & 0x3);
	}
	void step_col_row();

	int _trailRow;
	int _trailCol;
	Direction _goingTo;
	IBoundaryTraceApp &_app;
	IBoundaryTraceExterns &_externs;
};

static BoundaryTraceApp s_boundaryTraceApp;
static BoundaryTraceExterns s_boundaryTraceExterns;
static BoundaryTrace s_boundaryTrace(s_boundaryTraceApp, s_boundaryTraceExterns);

// take one step in the direction of _goingTo 
void BoundaryTrace::step_col_row()
{
	switch (_goingTo)
	{
	case North:
		_externs.SetColumn(_trailCol);
		_externs.SetRow(_trailRow - 1);
		break;
	case East:
		_externs.SetColumn(_trailCol + 1);
		_externs.SetRow(_trailRow);
		break;
	case South:
		_externs.SetColumn(_trailCol);
		_externs.SetRow(_trailRow + 1);
		break;
	case West:
		_externs.SetColumn(_trailCol - 1);
		_externs.SetRow(_trailRow);
		break;
	}
}

int BoundaryTrace::Execute()
{
	Direction coming_from;
	int matches_found;
	bool continue_loop;
	int trail_color;
	int fillcolor_used;
	int last_fillcolor_used = -1;
	int right;
	int left;
	int length;
	if (_externs.Inside() == 0 || _externs.Outside() == 0)
	{
		_app.stop_message(STOPMSG_NORMAL, "Boundary tracing cannot be used with inside=0 or outside=0");
		return -1;
	}

	_externs.SetGotStatus(GOT_STATUS_BOUNDARY_TRACE);
	int max_putline_length = 0;
	std::vector<BYTE> output;
	for (g_current_row = _externs.IYStart(); g_current_row <= _externs.YStop(); g_current_row++)
	{
		_externs.SetResetPeriodicity(true); // reset for a new row 
		_externs.SetColor(BACKGROUND_COLOR);
		for (g_current_col = _externs.IXStart(); g_current_col <= _externs.XStop(); g_current_col++)
		{
			if (_app.get_color(g_current_col, g_current_row) != BACKGROUND_COLOR)
			{
				continue;
			}
			trail_color = _externs.Color();
			_externs.SetRow(g_current_row);
			_externs.SetColumn(g_current_col);
			if (_app.calculate_type() == -1) // color, row, col are global 
			{
				if (_externs.ShowDot() != BACKGROUND_COLOR) // remove show dot pixel 
				{
					_app.plot_color(_externs.Column(), _externs.Row(), BACKGROUND_COLOR);
				}
				if (_externs.YStop() != _externs.GetWorkList().yy_stop())  // DG 
				{
					_externs.SetYStop(_externs.GetWorkList().yy_stop() - (g_current_row - _externs.GetWorkList().yy_start())); // allow for sym 
				}
				_externs.GetWorkList().add(_externs.GetWorkList().xx_start(), _externs.GetWorkList().xx_stop(), g_current_col,
					g_current_row, _externs.YStop(), g_current_row,
					0, _externs.WorkSymmetry());
				return -1;
			}
			_externs.SetResetPeriodicity(false); // normal periodicity checking 

			// This next line may cause a few more pixels to be calculated,
			// but at the savings of quite a bit of overhead
			if (_externs.Color() != trail_color)  // DG 
			{
				continue;
			}

			// sweep clockwise to trace outline 
			_trailRow = g_current_row;
			_trailCol = g_current_col;
			trail_color = _externs.Color();
			fillcolor_used = _externs.FillColor() > 0 ? _externs.FillColor() : trail_color;
			coming_from = West;
			_goingTo = East;
			matches_found = 0;
			continue_loop = true;
			do
			{
				step_col_row();
				if (_externs.Row() >= g_current_row
					&& _externs.Column() >= _externs.IXStart()
					&& _externs.Column() <= _externs.XStop()
					&& _externs.Row() <= _externs.YStop())
				{
					// the order of operations in this next line is critical 
					_externs.SetColor(_app.get_color(_externs.Column(), _externs.Row()));
					if (_externs.Color() == BACKGROUND_COLOR && _app.calculate_type() == -1)
								// color, row, col are global for calculate_type() 
					{
						if (_externs.ShowDot() != BACKGROUND_COLOR) // remove show dot pixel 
						{
							_app.plot_color(_externs.Column(), _externs.Row(), BACKGROUND_COLOR);
						}
						if (_externs.YStop() != _externs.GetWorkList().yy_stop())  // DG 
						{
							_externs.SetYStop(_externs.GetWorkList().yy_stop() - (g_current_row - _externs.GetWorkList().yy_start())); // allow for sym 
						}
						_externs.GetWorkList().add(_externs.GetWorkList().xx_start(), _externs.GetWorkList().xx_stop(), g_current_col,
							g_current_row, _externs.YStop(), g_current_row,
							0, _externs.WorkSymmetry());
						return -1;
					}
					else if (_externs.Color() == trail_color)
					{
						if (matches_found < 4) // to keep it from overflowing 
						{
							matches_found++;
						}
						_trailRow = _externs.Row();
						_trailCol = _externs.Column();
						advance_match(coming_from);
					}
					else
					{
						advance_no_match();
						continue_loop = (_goingTo != coming_from) || (matches_found > 0);
					}
				}
				else
				{
					advance_no_match();
					continue_loop = (_goingTo != coming_from) || (matches_found > 0);
				}
			}
			while (continue_loop && (_externs.Column() != g_current_col || _externs.Row() != g_current_row));

			if (matches_found <= 3)  // DG 
			{
				// no hole 
				_externs.SetColor(BACKGROUND_COLOR);
				_externs.SetResetPeriodicity(true);
				continue;
			}

			// Fill in region by looping around again, filling lines to the left
			// whenever _goingTo is South or West
			_trailRow = g_current_row;
			_trailCol = g_current_col;
			coming_from = West;
			_goingTo = East;
			do
			{
				matches_found = 0;
				do
				{
					step_col_row();
					if (_externs.Row() >= g_current_row
						&& _externs.Column() >= _externs.IXStart()
						&& _externs.Column() <= _externs.XStop()
						&& _externs.Row() <= _externs.YStop()
						&& _app.get_color(_externs.Column(), _externs.Row()) == trail_color)
						// get_color() must be last 
					{
						if (_goingTo == South
							|| (_goingTo == West && coming_from != East))
						{ // fill a row, but only once 
							right = _externs.Column();
							while (--right >= _externs.IXStart())
							{
								_externs.SetColor(_app.get_color(right, _externs.Row()));
								if (_externs.Color() != trail_color)
								{
									break;
								}
							}
							if (_externs.Color() == BACKGROUND_COLOR) // check last color 
							{
								left = right;
								while (_app.get_color(--left, _externs.Row()) == BACKGROUND_COLOR)
									// Should NOT be possible for left < _externs.IXStart() 
								{
									// do nothing 
								}
								left++; // one pixel too far 
								if (right == left) // only one hole 
								{
									_app.plot_color(left, _externs.Row(), fillcolor_used);
								}
								else
								{ // fill the line to the left 
									length = right-left + 1;
									if (fillcolor_used != last_fillcolor_used || length > max_putline_length)
									{
										output.resize(length);
										std::fill(&output[0], &output[length], fillcolor_used);
										last_fillcolor_used = fillcolor_used;
										max_putline_length = length;
									}
									_app.sym_fill_line(_externs.Row(), left, right, &output[0]);
								}
							} // end of fill line 

#if 0 // don't interupt with a check_key() during fill 
							if (--g_input_counter <= 0)
							{
								if (check_key())
								{
									if (_externs.YStop() != _externs.GetWorkList().yy_stop())
									{
										_externs.SetYStop(_externs.GetWorkList().yy_stop() - (g_current_row - _externs.GetWorkList().yy_start())); // allow for sym 
									}
									_externs.GetWorkList().add(_externs.GetWorkList().xx_start(), _externs.GetWorkList().xx_stop(), g_current_col,
										g_current_row, _externs.YStop(), g_current_row,
										0, _externs.WorkSymmetry());
									return -1;
								}
								g_input_counter = g_max_input_counter;
							}
#endif
						}
						_trailRow = _externs.Row();
						_trailCol = _externs.Column();
						advance_match(coming_from);
						matches_found = 1;
					}
					else
					{
						advance_no_match();
					}
				}
				while ((matches_found == 0) && _goingTo != coming_from);

				if (matches_found == 0)
				{ // next one has to be a match 
					step_col_row();
					_trailRow = _externs.Row();
					_trailCol = _externs.Column();
					advance_match(coming_from);
				}
			}
			while (_trailCol != g_current_col || _trailRow != g_current_row);
			_externs.SetResetPeriodicity(true); // reset after a trace/fill 
			_externs.SetColor(BACKGROUND_COLOR);
		}
	}

	return 0;
}

int boundary_trace_main()
{
	return s_boundaryTrace.Execute();
}
