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
	virtual int CurrentRow() const = 0;
	virtual void SetCurrentRow(int value) = 0;
	virtual void NextCurrentRow() = 0;
	virtual int CurrentColumn() const = 0;
	virtual void SetCurrentColumn(int value) = 0;
	virtual void NextCurrentColumn() = 0;
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
	virtual int CurrentRow() const				{ return g_current_row; }
	virtual void SetCurrentRow(int value)		{ g_current_row = value; }
	virtual int CurrentColumn() const			{ return g_current_col; }
	virtual void SetCurrentColumn(int value)	{ g_current_col = value; }
	virtual void NextCurrentRow()				{ g_current_row++; }
	virtual void NextCurrentColumn()			{ g_current_col++; }
};

class BoundaryTrace : public IBoundaryTrace
{
public:
	BoundaryTrace(IBoundaryTraceApp &app, IBoundaryTraceExterns &externs)
		: _trailRow(0), _trailCol(0),
		_goingTo(North),
		_app(app),
		_externs(externs),
		_trailColor(0),
		_result(0),
		_fillColorUsed(0),
		_numMatchesFound(0),
		_needMoreTracing(true),
		_lastFillColorUsed(-1),
		_outputPixels(),
		_right(0),
		_left(0),
		_length(0),
		_maxPutLineLength(0)
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
	bool TraceRow(int row);
	bool TraceColumn(int row, int column);
	bool TraceStep(int row, int column);
	Direction advance(Direction value)
	{
		return Direction((value - 1) & 0x3);
	}
	void advance_match()
	{
		_goingTo = advance(_goingTo);
		_comingFrom = advance(_goingTo);
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
	int _trailColor;
	int _result;
	int _fillColorUsed;
	Direction _comingFrom;
	int _numMatchesFound;
	bool _needMoreTracing;
	int _lastFillColorUsed;
	std::vector<BYTE> _outputPixels;
	int _right;
	int _left;
	int _length;
	int _maxPutLineLength;
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
	if (_externs.Inside() == 0 || _externs.Outside() == 0)
	{
		_app.stop_message(STOPMSG_NORMAL, "Boundary tracing cannot be used with inside=0 or outside=0");
		return -1;
	}

	_externs.SetGotStatus(GOT_STATUS_BOUNDARY_TRACE);
	for (_externs.SetCurrentRow(_externs.IYStart()); _externs.CurrentRow() <= _externs.YStop(); _externs.NextCurrentRow())
	{
		if (TraceRow(_externs.CurrentRow()))
		{
			break;
		}
	}

	return _result;
}

bool BoundaryTrace::TraceRow(int row)
{
	_externs.SetResetPeriodicity(true); // reset for a new row 
	_externs.SetColor(BACKGROUND_COLOR);
	for (_externs.SetCurrentColumn(_externs.IXStart()); _externs.CurrentColumn() <= _externs.XStop(); _externs.NextCurrentColumn())
	{
		if (TraceColumn(row, _externs.CurrentColumn()))
		{
			return true;
		}
	}
	return false;
}

bool BoundaryTrace::TraceColumn(int row, int column)
{
	if (_app.get_color(column, row) != BACKGROUND_COLOR)
	{
		return false;
	}
	_trailColor = _externs.Color();
	_externs.SetRow(row);
	_externs.SetColumn(column);
	if (_app.calculate_type() == -1) // color, row, col are global 
	{
		if (_externs.ShowDot() != BACKGROUND_COLOR) // remove show dot pixel 
		{
			_app.plot_color(_externs.Column(), _externs.Row(), BACKGROUND_COLOR);
		}
		if (_externs.YStop() != _externs.GetWorkList().yy_stop())  // DG 
		{
			_externs.SetYStop(_externs.GetWorkList().yy_stop() - (row - _externs.GetWorkList().yy_start())); // allow for sym 
		}
		_externs.GetWorkList().add(_externs.GetWorkList().xx_start(), _externs.GetWorkList().xx_stop(), column,
			row, _externs.YStop(), row,
			0, _externs.WorkSymmetry());
		_result = -1;
		return true;
	}
	_externs.SetResetPeriodicity(false); // normal periodicity checking 

	// This next line may cause a few more pixels to be calculated,
	// but at the savings of quite a bit of overhead
	if (_externs.Color() != _trailColor)  // DG 
	{
		return false;
	}

	// sweep clockwise to trace outline 
	_trailRow = row;
	_trailCol = column;
	_trailColor = _externs.Color();
	_fillColorUsed = _externs.FillColor() > 0 ? _externs.FillColor() : _trailColor;
	_comingFrom = West;
	_goingTo = East;
	_numMatchesFound = 0;
	_needMoreTracing = true;
	do
	{
		if (TraceStep(row, column))
		{
			return true;
		}
	}
	while (_needMoreTracing && (_externs.Column() != column || _externs.Row() != row));

	if (_numMatchesFound <= 3)  // DG 
	{
		// no hole 
		_externs.SetColor(BACKGROUND_COLOR);
		_externs.SetResetPeriodicity(true);
		return false;
	}

	// Fill in region by looping around again, filling lines to the left
	// whenever _goingTo is South or West
	_trailRow = row;
	_trailCol = column;
	_comingFrom = West;
	_goingTo = East;
	do
	{
		_numMatchesFound = 0;
		do
		{
			step_col_row();
			if (_externs.Row() >= row
				&& _externs.Column() >= _externs.IXStart()
				&& _externs.Column() <= _externs.XStop()
				&& _externs.Row() <= _externs.YStop()
				&& _app.get_color(_externs.Column(), _externs.Row()) == _trailColor)
				// get_color() must be last 
			{
				if (_goingTo == South
					|| (_goingTo == West && _comingFrom != East))
				{ // fill a row, but only once 
					_right = _externs.Column();
					while (--_right >= _externs.IXStart())
					{
						_externs.SetColor(_app.get_color(_right, _externs.Row()));
						if (_externs.Color() != _trailColor)
						{
							break;
						}
					}
					if (_externs.Color() == BACKGROUND_COLOR) // check last color 
					{
						_left = _right;
						while (_app.get_color(--_left, _externs.Row()) == BACKGROUND_COLOR)
							// Should NOT be possible for _left < _externs.IXStart() 
						{
							// do nothing 
						}
						_left++; // one pixel too far 
						if (_right == _left) // only one hole 
						{
							_app.plot_color(_left, _externs.Row(), _fillColorUsed);
						}
						else
						{ // fill the line to the _left 
							_length = _right-_left + 1;
							if (_fillColorUsed != _lastFillColorUsed || _length > _maxPutLineLength)
							{
								_outputPixels.resize(_length);
								std::fill(&_outputPixels[0], &_outputPixels[_length], _fillColorUsed);
								_lastFillColorUsed = _fillColorUsed;
								_maxPutLineLength = _length;
							}
							_app.sym_fill_line(_externs.Row(), _left, _right, &_outputPixels[0]);
						}
					} // end of fill line 

#if 0 // don't interupt with a check_key() during fill 
					if (--g_input_counter <= 0)
					{
						if (check_key())
						{
							if (_externs.YStop() != _externs.GetWorkList().yy_stop())
							{
								_externs.SetYStop(_externs.GetWorkList().yy_stop() - (row - _externs.GetWorkList().yy_start())); // allow for sym 
							}
							_externs.GetWorkList().add(_externs.GetWorkList().xx_start(), _externs.GetWorkList().xx_stop(), column,
								row, _externs.YStop(), row,
								0, _externs.WorkSymmetry());
							return -1;
						}
						g_input_counter = g_max_input_counter;
					}
#endif
				}
				_trailRow = _externs.Row();
				_trailCol = _externs.Column();
				advance_match();
				_numMatchesFound = 1;
			}
			else
			{
				advance_no_match();
			}
		}
		while ((_numMatchesFound == 0) && _goingTo != _comingFrom);

		if (_numMatchesFound == 0)
		{ // next one has to be a match 
			step_col_row();
			_trailRow = _externs.Row();
			_trailCol = _externs.Column();
			advance_match();
		}
	}
	while (_trailCol != column || _trailRow != row);
	_externs.SetResetPeriodicity(true); // reset after a trace/fill 
	_externs.SetColor(BACKGROUND_COLOR);

	return false;
}

bool BoundaryTrace::TraceStep(int row, int column)
{
	step_col_row();
	if (_externs.Row() >= row
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
				_externs.SetYStop(_externs.GetWorkList().yy_stop() - (row - _externs.GetWorkList().yy_start())); // allow for sym 
			}
			_externs.GetWorkList().add(_externs.GetWorkList().xx_start(), _externs.GetWorkList().xx_stop(), column,
				row, _externs.YStop(), row,
				0, _externs.WorkSymmetry());
			_result = -1;
			return true;
		}
		else if (_externs.Color() == _trailColor)
		{
			if (_numMatchesFound < 4) // to keep it from overflowing 
			{
				_numMatchesFound++;
			}
			_trailRow = _externs.Row();
			_trailCol = _externs.Column();
			advance_match();
		}
		else
		{
			advance_no_match();
			_needMoreTracing = (_goingTo != _comingFrom) || (_numMatchesFound > 0);
		}
	}
	else
	{
		advance_no_match();
		_needMoreTracing = (_goingTo != _comingFrom) || (_numMatchesFound > 0);
	}

	return false;
}

int boundary_trace_main()
{
	return s_boundaryTrace.Execute();
}
