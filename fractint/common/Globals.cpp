#include <vector>

#include "port.h"
#include "id.h"

#include "globals.h"

Globals g_;

class GlobalImpl
{
public:
	GlobalImpl();
	~GlobalImpl();

	// video mode/table stuff
	int Adapter() const					{ return _adapter; }
	void SetAdapter(int value)			{ _adapter = value; }
	int InitialAdapter() const			{ return _initialAdapter; }
	void SetInitialAdapter(int value)	{ _initialAdapter = value; }
	void SetInitialAdapterNone()		{ _initialAdapter = -1; }
	const VIDEOINFO &VideoEntry() const	{ return _videoEntry; }
	void SetVideoEntry(const VIDEOINFO &value) { _videoEntry = value; }
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
	std::string &MapName()				{ return _mapName; }
	void SetMapName(const std::string &value) { _mapName = value; }

private:
	int _adapter;
	int _initialAdapter;
	VIDEOINFO _videoEntry;
	std::vector<VIDEOINFO> _videoTable;
	bool _goodMode;
	ColormapTable _dac;
	ColormapTable _oldDAC;
	ColormapTable *_mapDAC;				/* map= (default colors) */
	bool _realDAC;
	SaveDACType _saveDAC;
	int _dacSleepCount;
	ColorStateType _colorState;			/* 0, g_dac_box matches default (bios or map=) */
										/* 1, g_dac_box matches no known defined map */
										/* 2, g_dac_box matches the g_color_file map */
	bool _mapSet;
	std::string _mapName;
};


bool g_color_preloaded;					/* if g_dac_box preloaded for next mode select */

GlobalImpl::GlobalImpl()
	: _adapter(0),
	_initialAdapter(0),
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
	_mapName("")
{
	_videoTable.reserve(MAXVIDEOMODES);
}

GlobalImpl::~GlobalImpl()
{
	delete _mapDAC;
	_mapDAC = 0;
}

Globals::Globals()
	: _impl(new GlobalImpl)
{
}

Globals::~Globals()
{
	delete _impl;
}

int Globals::Adapter() const								{ return _impl->Adapter(); }
void Globals::SetAdapter(int value)							{ _impl->SetAdapter(value); }
int Globals::InitialAdapter() const							{ return _impl->InitialAdapter(); }
void Globals::SetInitialAdapter(int value)					{ _impl->SetInitialAdapter(value); }
void Globals::SetInitialAdapterNone()						{ _impl->SetInitialAdapterNone(); }
const VIDEOINFO &Globals::VideoEntry() const				{ return _impl->VideoEntry(); }
void Globals::SetVideoEntry(const VIDEOINFO &value)			{ _impl->SetVideoEntry(value); }
void Globals::SetVideoEntrySize(int width, int height)		{ _impl->SetVideoEntrySize(width, height); }
void Globals::SetVideoEntryXDots(int value)					{ _impl->SetVideoEntryXDots(value); }
void Globals::SetVideoEntryColors(int value)				{ _impl->SetVideoEntryColors(value); }
int Globals::VideoTableLength() const						{ return _impl->VideoTableLength(); }
void Globals::AddVideoModeToTable(const VIDEOINFO &mode)	{ _impl->AddVideoModeToTable(mode); }
const VIDEOINFO &Globals::VideoTable(int i) const			{ return _impl->VideoTable(i); }
void Globals::SetVideoTableKey(int i, int key)				{ _impl->SetVideoTableKey(i, key); }
void Globals::SetVideoTable(int i, const VIDEOINFO &value)	{ _impl->SetVideoTable(i, value); }
bool Globals::GoodMode() const								{ return _impl->GoodMode(); }
void Globals::SetGoodMode(bool value)						{ _impl->SetGoodMode(value); }
ColormapTable &Globals::DAC()								{ return _impl->DAC(); }
const ColormapTable &Globals::DAC() const					{ return _impl->DAC(); }
bool Globals::RealDAC() const								{ return _impl->RealDAC(); }
void Globals::SetRealDAC(bool value)						{ _impl->SetRealDAC(value); }
SaveDACType Globals::SaveDAC() const						{ return _impl->SaveDAC(); }
void Globals::SetSaveDAC(SaveDACType value)					{ _impl->SetSaveDAC(value); }
int Globals::DACSleepCount() const							{ return _impl->DACSleepCount(); }
void Globals::SetDACSleepCount(int value)					{ _impl->SetDACSleepCount(value); }
void Globals::IncreaseDACSleepCount()						{ _impl->IncreaseDACSleepCount(); }
void Globals::DecreaseDACSleepCount()						{ _impl->DecreaseDACSleepCount(); }
void Globals::SetMapDAC(ColormapTable *value)				{ _impl->SetMapDAC(value); }
const ColormapTable *Globals::MapDAC() const				{ return _impl->MapDAC(); }
ColormapTable *Globals::MapDAC()							{ return _impl->MapDAC(); }
const ColormapTable &Globals::OldDAC() const				{ return _impl->OldDAC(); }
ColormapTable &Globals::OldDAC()							{ return _impl->OldDAC(); }
ColorStateType Globals::ColorState() const					{ return _impl->ColorState(); }
void Globals::SetColorState(ColorStateType value)			{ _impl->SetColorState(value); }
bool Globals::MapSet() const								{ return _impl->MapSet(); }
void Globals::SetMapSet(bool value)							{ _impl->SetMapSet(value); }
std::string &Globals::MapName()								{ return _impl->MapName(); }
void Globals::SetMapName(const std::string &value)			{ _impl->SetMapName(value); }
