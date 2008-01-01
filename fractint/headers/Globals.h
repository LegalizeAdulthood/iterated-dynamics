#pragma once

#include "ColormapTable.h"

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

	ColormapTable &DAC();
	const ColormapTable &DAC() const;
	ColormapTable &OldDAC();
	const ColormapTable &OldDAC() const;
	ColormapTable *MapDAC();
	const ColormapTable *MapDAC() const;
	void SetMapDAC(ColormapTable *value);
	bool RealDAC() const;
	void SetRealDAC(bool value);
	SaveDACType SaveDAC() const;
	void SetSaveDAC(SaveDACType value);
	int DACSleepCount() const;
	void SetDACSleepCount(int value);
	void IncreaseDACSleepCount();
	void DecreaseDACSleepCount();
	ColorStateType ColorState() const;
	void SetColorState(ColorStateType value);
	bool MapSet() const;
	void SetMapSet(bool value);

private:
	GlobalImpl *_impl;
};

extern Globals g_;
