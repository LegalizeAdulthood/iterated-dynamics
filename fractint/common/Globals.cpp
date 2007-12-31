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

private:
	int _adapter;
	int _initialAdapter;
	VIDEOINFO _videoEntry;
	std::vector<VIDEOINFO> _videoTable;
	bool _goodMode;
};

ColormapTable g_dac_box;
ColormapTable *g_map_dac_box;					/* map= (default colors) */
ColormapTable g_old_dac_box;
int g_save_dac;
int g_dac_count;
bool g_got_real_dac;							/* loaddac worked, really got a dac */
int g_color_state;								/* 0, g_dac_box matches default (bios or map=) */
												/* 1, g_dac_box matches no known defined map   */
												/* 2, g_dac_box matches the g_color_file map      */
bool g_color_preloaded;							/* if g_dac_box preloaded for next mode select */
int g_color_dark = 0;		/* darkest color in palette */
int g_color_bright = 0;		/* brightest color in palette */
int g_color_medium = 0;		/* nearest to medbright grey in palette Zoom-Box values (2K x 2K screens max) */

GlobalImpl::GlobalImpl()
	: _adapter(0),
	_initialAdapter(0),
	_videoEntry(),
	_videoTable(MAXVIDEOMODES),
	_goodMode(false)
{
}

GlobalImpl::~GlobalImpl()
{
}

Globals::Globals()
	: _impl(new GlobalImpl)
{
}

Globals::~Globals()
{
	delete _impl;
}

int Globals::Adapter() const
{
	return _impl->Adapter();
}
void Globals::SetAdapter(int value)
{
	_impl->SetAdapter(value);
}
int Globals::InitialAdapter() const
{
	return _impl->InitialAdapter();
}
void Globals::SetInitialAdapter(int value)
{
	_impl->SetInitialAdapter(value);
}
void Globals::SetInitialAdapterNone()
{
	_impl->SetInitialAdapterNone();
}
const VIDEOINFO &Globals::VideoEntry() const
{
	return _impl->VideoEntry();
}
void Globals::SetVideoEntry(const VIDEOINFO &value)
{
	_impl->SetVideoEntry(value);
}
void Globals::SetVideoEntrySize(int width, int height)
{
	_impl->SetVideoEntrySize(width, height);
}
void Globals::SetVideoEntryXDots(int value)
{
	_impl->SetVideoEntryXDots(value);
}
void Globals::SetVideoEntryColors(int value)
{
	_impl->SetVideoEntryColors(value);
}
int Globals::VideoTableLength() const
{
	return _impl->VideoTableLength();
}
void Globals::AddVideoModeToTable(const VIDEOINFO &mode)
{
	_impl->AddVideoModeToTable(mode);
}
const VIDEOINFO &Globals::VideoTable(int i) const
{
	return _impl->VideoTable(i);
}
void Globals::SetVideoTableKey(int i, int key)
{
	_impl->SetVideoTableKey(i, key);
}
void Globals::SetVideoTable(int i, const VIDEOINFO &value)
{
	_impl->SetVideoTable(i, value);
}
bool Globals::GoodMode() const
{
	return _impl->GoodMode();
}
void Globals::SetGoodMode(bool value)
{
	_impl->SetGoodMode(value);
}
