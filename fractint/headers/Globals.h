#if !defined(GLOBALS_H)
#define GLOBALS_H

#include "id.h"
#include "ColormapTable.h"

class IGlobals
{
public:
	virtual ~IGlobals() { }

	// video mode table stuff
	virtual int Adapter() const = 0;
	virtual void SetAdapter(int value) = 0;
	virtual int InitialVideoMode() const = 0;
	virtual void SetInitialVideoMode(int value) = 0;
	virtual void SetInitialVideoModeNone() = 0;
	virtual const VIDEOINFO &VideoEntry() const = 0;
	virtual void SetVideoEntry(int idx) = 0;
	virtual void SetVideoEntrySize(int width, int height) = 0;
	virtual void SetVideoEntryXDots(int value) = 0;
	virtual void SetVideoEntryColors(int value) = 0;
	virtual int VideoTableLength() const = 0;
	virtual void AddVideoModeToTable(const VIDEOINFO &mode) = 0;
	virtual const VIDEOINFO &VideoTable(int i) const = 0;
	virtual void SetVideoTableKey(int i, int key) = 0;
	virtual void SetVideoTable(int i, const VIDEOINFO &value) = 0;
	virtual bool GoodMode() const = 0;
	virtual void SetGoodMode(bool value) = 0;

	// colormap stuff
	virtual ColormapTable &DAC() = 0;
	virtual const ColormapTable &DAC() const = 0;
	virtual ColormapTable &OldDAC() = 0;
	virtual const ColormapTable &OldDAC() const = 0;
	virtual ColormapTable *MapDAC() = 0;
	virtual const ColormapTable *MapDAC() const = 0;
	virtual void SetMapDAC(ColormapTable *value) = 0;
	virtual bool RealDAC() const = 0;
	virtual void SetRealDAC(bool value) = 0;
	virtual SaveDACType SaveDAC() const = 0;
	virtual void SetSaveDAC(SaveDACType value) = 0;
	virtual int DACSleepCount() const = 0;
	virtual void SetDACSleepCount(int value) = 0;
	virtual void IncreaseDACSleepCount() = 0;
	virtual void DecreaseDACSleepCount() = 0;
	virtual ColorStateType ColorState() const = 0;
	virtual void SetColorState(ColorStateType value) = 0;
	virtual bool MapSet() const = 0;
	virtual void SetMapSet(bool value) = 0;
	virtual std::string const &MapName() const = 0;
	virtual void SetMapName(const std::string &value) = 0;
	virtual int NumColors() const = 0;
	virtual void SetNumColors(int value) = 0;
	virtual void PushDAC() = 0;
	virtual void PopDAC() = 0;
};

extern IGlobals &g_;

#endif
