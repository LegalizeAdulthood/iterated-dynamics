#pragma once

class GlobalImpl;

class Globals
{
public:
	Globals();
	~Globals();

	int Adapter() const;
	void SetAdapter(int value);
	int InitialAdapter() const;
	void SetInitialAdapter(int value);
	void SetInitialAdapterNone();
	const VIDEOINFO &VideoEntry() const;
	void SetVideoEntry(const VIDEOINFO &value);
	void SetVideoEntrySize(int width, int height);
	void SetVideoEntryXDots(int value);
	void SetVideoEntryColors(int value);
	int VideoTableLength() const;
	void AddVideoModeToTable(const VIDEOINFO &mode);
	const VIDEOINFO &VideoTable(int i) const;
	void SetVideoTableKey(int i, int key);
	void SetVideoTable(int i, const VIDEOINFO &value);
	bool GoodMode() const;
	void SetGoodMode(bool value);

private:
	GlobalImpl *_impl;
};

extern Globals g_;
