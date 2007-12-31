#pragma once

class Globals
{
public:
	Globals();
	~Globals();

	int Adapter() const					{ return _adapter; }
	void SetAdapter(int value)			{ _adapter = value; }
	int InitialAdapter() const			{ return _initialAdapter; }
	void SetInitialAdapter(int value)	{ _initialAdapter = value; }
	const VIDEOINFO &VideoEntry() const	{ return _videoEntry; }
	void SetVideoEntry(const VIDEOINFO &value) { _videoEntry = value; }
	void SetVideoEntrySize(int width, int height)
	{
		_videoEntry.x_dots = width;
		_videoEntry.y_dots = height;
	}
	void SetVideoEntryXDots(int value)	{ _videoEntry.x_dots = value; }
	void SetVideoEntryColors(int value) { _videoEntry.colors = value; }
	int VideoTableLength() const		{ return _videoTableLength; }
	void IncrementVideoTableLength()	{ _videoTableLength++; }
	const VIDEOINFO &VideoTable(int i) const { return _videoTable[i]; }
	void SetVideoTableKey(int i, int key) { _videoTable[i].keynum = key; }
	void SetVideoTable(int i, const VIDEOINFO &value) { _videoTable[i] = value; }
	bool GoodMode() const				{ return _goodMode; }
	void SetGoodMode(bool value)		{ _goodMode = value; }

private:
	int _adapter;
	int _initialAdapter;
	VIDEOINFO _videoEntry;
	VIDEOINFO _videoTable[MAXVIDEOMODES];
	int _videoTableLength;
	bool _goodMode;
};

extern Globals g_;
