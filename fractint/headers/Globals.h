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

private:
	int _adapter;
	int _initialAdapter;
	VIDEOINFO _videoEntry;
	VIDEOINFO _videoTable[MAXVIDEOMODES];
};

extern Globals g_;
