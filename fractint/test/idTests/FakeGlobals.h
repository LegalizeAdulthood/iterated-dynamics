#pragma once

#include "Globals.h"
#include "NotImplementedException.h"

class FakeGlobals : public IGlobals
{
public:
	FakeGlobals() : _saveDACCalled(false),
		_saveDACFakeResult(SAVEDAC_NO),
		_setSaveDACCalled(false),
		_setSaveDACLastValue(SAVEDAC_NO)
	{
	}
	virtual ~FakeGlobals() { }

	// video mode table stuff
	virtual int Adapter() const { throw not_implemented("Adapter"); };
	virtual void SetAdapter(int value) { throw not_implemented("SetAdapter"); };
	virtual int InitialVideoMode() const { throw not_implemented("InitialVideoMode"); };
	virtual void SetInitialVideoMode(int value) { throw not_implemented("SetInitialVideoMode"); };
	virtual void SetInitialVideoModeNone() { throw not_implemented("SetInitialVideoModeNone"); };
	virtual const VIDEOINFO &VideoEntry() const { throw not_implemented("VIDEOINFO &VideoEntry"); };
	virtual void SetVideoEntry(int idx) { throw not_implemented("SetVideoEntry"); };
	virtual void SetVideoEntrySize(int width, int height) { throw not_implemented("SetVideoEntrySize"); };
	virtual void SetVideoEntryXDots(int value) { throw not_implemented("SetVideoEntryXDots"); };
	virtual void SetVideoEntryColors(int value) { throw not_implemented("SetVideoEntryColors"); };
	virtual int VideoTableLength() const { throw not_implemented("VideoTableLength"); };
	virtual void AddVideoModeToTable(const VIDEOINFO &mode) { throw not_implemented("AddVideoModeToTable"); };
	virtual const VIDEOINFO &VideoTable(int i) const { throw not_implemented("VideoTable"); };
	virtual void SetVideoTableKey(int i, int key) { throw not_implemented("SetVideoTableKey"); };
	virtual void SetVideoTable(int i, const VIDEOINFO &value) { throw not_implemented("SetVideoTable"); };
	virtual bool GoodMode() const { throw not_implemented("GoodMode"); };
	virtual void SetGoodMode(bool value) { throw not_implemented("SetGoodMode"); };

	// colormap stuff
	virtual ColormapTable &DAC() { throw not_implemented("&DAC"); };
	virtual const ColormapTable &DAC() const { throw not_implemented("ColormapTable &DAC"); };
	virtual ColormapTable &OldDAC() { throw not_implemented("&OldDAC"); };
	virtual const ColormapTable &OldDAC() const { throw not_implemented("ColormapTable &OldDAC"); };
	virtual ColormapTable *MapDAC() { throw not_implemented("*MapDAC"); };
	virtual const ColormapTable *MapDAC() const { throw not_implemented("ColormapTable *MapDAC"); };
	virtual void SetMapDAC(ColormapTable *value) { throw not_implemented("SetMapDAC"); };
	virtual bool RealDAC() const { throw not_implemented("RealDAC"); };
	virtual void SetRealDAC(bool value) { throw not_implemented("SetRealDAC"); };
	virtual SaveDACType SaveDAC() const
	{
		const_cast<FakeGlobals *>(this)->_saveDACCalled = true;
		return _saveDACFakeResult;
	};
	void SetSaveDACFakeResult(SaveDACType value)
	{ _saveDACFakeResult = value; }
	bool SaveDACCalled() const
	{ return _saveDACCalled; }
	virtual void SetSaveDAC(SaveDACType value)	{ _setSaveDACCalled = true; _setSaveDACLastValue = value; }
	bool SetSaveDACCalled() const				{ return _setSaveDACCalled; }
	SaveDACType SetSaveDACLastValue() const		{ return _setSaveDACLastValue; }
	virtual int DACSleepCount() const { throw not_implemented("DACSleepCount"); };
	virtual void SetDACSleepCount(int value) { throw not_implemented("SetDACSleepCount"); };
	virtual void IncreaseDACSleepCount() { throw not_implemented("IncreaseDACSleepCount"); };
	virtual void DecreaseDACSleepCount() { throw not_implemented("DecreaseDACSleepCount"); };
	virtual ColorStateType ColorState() const { throw not_implemented("ColorState"); };
	virtual void SetColorState(ColorStateType value) { throw not_implemented("SetColorState"); };
	virtual bool MapSet() const { throw not_implemented("MapSet"); };
	virtual void SetMapSet(bool value) { throw not_implemented("SetMapSet"); };
	virtual std::string const &MapName() const { throw not_implemented("MapName"); };
	virtual void SetMapName(const std::string &value) { throw not_implemented("SetMapName"); };
	virtual int NumColors() const { throw not_implemented("NumColors"); };
	virtual void SetNumColors(int value) { throw not_implemented("SetNumColors"); };
	virtual void PushDAC() { throw not_implemented("PushDAC"); };
	virtual void PopDAC() { throw not_implemented("PopDAC"); };

private:
	bool _saveDACCalled;
	SaveDACType _saveDACFakeResult;
	bool _setSaveDACCalled;
	SaveDACType _setSaveDACLastValue;
};
