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

private:
	int _adapter;
	int _initialAdapter;
	VIDEOINFO _videoEntry;
	std::vector<VIDEOINFO> _videoTable;
	bool _goodMode;
};

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
