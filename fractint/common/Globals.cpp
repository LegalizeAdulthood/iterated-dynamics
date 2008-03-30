#include <vector>

#include "port.h"
#include "id.h"

#include "globals.h"

bool g_color_preloaded;					// if g_dac_box preloaded for next mode select

class Globals : public IGlobals
{
public:
	Globals();
	virtual ~Globals();

	// video mode/table stuff
	int Adapter() const					{ return _adapter; }
	void SetAdapter(int value)			{ _adapter = value; }
	int InitialVideoMode() const		{ return _initialVideoMode; }
	void SetInitialVideoMode(int value)	{ _initialVideoMode = value; }
	void SetInitialVideoModeNone()		{ _initialVideoMode = -1; }
	const VIDEOINFO &VideoEntry() const	{ return _videoEntry; }
	void SetVideoEntry(int idx)			{ _videoEntry = _videoTable[idx]; }
	void SetVideoEntrySize(int width, int height)
	{
		_videoEntry.x_dots = width;
		_videoEntry.y_dots = height;
	}
	void SetVideoEntryXDots(int value)	{ _videoEntry.x_dots = value; }
	void SetVideoEntryColors(int value) { _videoEntry.colors = value; }
	int VideoTableLength() const		{ return int(_videoTable.size()); }
	void AddVideoModeToTable(const VIDEOINFO &mode) { _videoTable.push_back(mode); }
	const VIDEOINFO &VideoTable(int i) const { return _videoTable[i]; }
	void SetVideoTableKey(int i, int key) { _videoTable[i].keynum = key; }
	void SetVideoTable(int i, const VIDEOINFO &value) { _videoTable[i] = value; }
	bool GoodMode() const				{ return _goodMode; }
	void SetGoodMode(bool value)		{ _goodMode = value; }

	// DAC/colormap stuff
	ColormapTable &DAC()				{ return _dac; }
	const ColormapTable &DAC() const	{ return _dac; }
	ColormapTable &OldDAC()				{ return _oldDAC; }
	const ColormapTable &OldDAC() const { return _oldDAC; }
	void PushDAC()						{ _oldDAC = _dac; }
	void PopDAC()						{ _dac = _oldDAC; }
	ColormapTable *MapDAC()				{ return _mapDAC; }
	const ColormapTable *MapDAC() const { return _mapDAC; }
	void SetMapDAC(ColormapTable *value) { _mapDAC = value; }
	bool RealDAC() const				{ return _realDAC; }
	void SetRealDAC(bool value)			{ _realDAC = value; }
	SaveDACType SaveDAC() const			{ return _saveDAC; }
	void SetSaveDAC(SaveDACType value)	{ _saveDAC = value; }
	int DACSleepCount() const			{ return _dacSleepCount; }
	void SetDACSleepCount(int value)	{ _dacSleepCount = value; }
	void IncreaseDACSleepCount()
	{
		extern int g_colors;
		if (_dacSleepCount < g_colors)
		{
			++_dacSleepCount;
		}
	}
	void DecreaseDACSleepCount()
	{
		if (_dacSleepCount > 1)
		{
			_dacSleepCount--;
		}
	}
	ColorStateType ColorState() const	{ return _colorState; }
	void SetColorState(ColorStateType value) { _colorState = value; }
	bool MapSet() const					{ return _mapSet; }
	void SetMapSet(bool value)			{ _mapSet = value; }
	std::string const &MapName() const	{ return _mapName; }
	void SetMapName(const std::string &value) { _mapName = value; }
	int NumColors() const				{ return _numColors; }
	void SetNumColors(int value)		{ _numColors = value; }

private:
	int _adapter;
	int _initialVideoMode;
	VIDEOINFO _videoEntry;
	std::vector<VIDEOINFO> _videoTable;
	bool _goodMode;
	ColormapTable _dac;
	ColormapTable _oldDAC;
	ColormapTable *_mapDAC;				// map= (default colors)
	bool _realDAC;
	SaveDACType _saveDAC;
	int _dacSleepCount;
	ColorStateType _colorState;			// 0, g_dac_box matches default (bios or map=)
										// 1, g_dac_box matches no known defined map
										// 2, g_dac_box matches the color file map
	bool _mapSet;
	std::string _mapName;
	int _numColors;
};

static Globals s_globals;
IGlobals &g_(s_globals);


Globals::Globals()
	: _adapter(0),
	_initialVideoMode(0),
	_videoEntry(),
	_videoTable(),
	_goodMode(false),
	_dac(),
	_oldDAC(),
	_mapDAC(0),
	_realDAC(false),
	_saveDAC(SAVEDAC_NO),
	_dacSleepCount(256),
	_colorState(COLORSTATE_DEFAULT),
	_mapSet(false),
	_mapName(""),
	_numColors(0)
{
	_videoTable.reserve(MAXVIDEOMODES);
}

Globals::~Globals()
{
	delete _mapDAC;
	_mapDAC = 0;
}
