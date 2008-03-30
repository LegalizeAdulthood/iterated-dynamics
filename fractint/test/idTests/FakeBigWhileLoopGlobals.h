#if !defined(FAKE_BIG_WHILE_LOOP_GLOBALS_H)
#define FAKE_BIG_WHILE_LOOP_GLOBALS_H

#include "FakeViewWindow.h"

class FakeBigWhileLoopGlobals : public IBigWhileLoopGlobals
{
public:
	FakeBigWhileLoopGlobals() : _viewWindow()
	{}
	virtual ~FakeBigWhileLoopGlobals() {}

	virtual int EvolvingFlags() { return 0; }
	virtual void SetEvolvingFlags(int value) {}
	virtual bool QuickCalculate() { return false; }
	virtual void SetQuickCalculate(bool value) {}
	virtual void SetUserStandardCalculationMode(CalculationMode value) {}
	virtual CalculationMode StandardCalculationModeOld() { return CalculationMode(); }
	virtual CalculationStatusType CalculationStatus() { return CalculationStatusType(); }
	virtual void SetCalculationStatus(CalculationStatusType value) {}
	virtual ShowFileType ShowFile() { return ShowFileType(); }
	virtual void SetShowFile(ShowFileType value) {}
	virtual int ScreenWidth() { return 0; }
	virtual void SetScreenWidth(int value) {}
	virtual int ScreenHeight() { return 0; }
	virtual void SetScreenHeight(int value) {}
	virtual int XDots() { return 0; }
	virtual void SetXDots(int value) {}
	virtual int YDots() { return 0; }
	virtual void SetYDots(int value) {}
	virtual float ScreenAspectRatio() { return 0; }
	virtual int ScreenXOffset() { return 0; }
	virtual int ScreenYOffset() { return 0; }
	virtual void SetScreenOffset(int x, int y) {}
	virtual int Colors() { return 0; }
	virtual void SetColors(int value) {}
	virtual ViewWindow &GetViewWindow() { return _viewWindow; }
	virtual int RotateHigh() { return 0; }
	virtual void SetRotateHigh(int value) {}
	virtual bool Overlay3D() { return false; }
	virtual void SetOverlay3D(bool value) {}
	virtual bool ColorPreloaded() { return false; }
	virtual void SetColorPreloaded(bool value) {}
	virtual void SetDXSize(double value) {}
	virtual void SetDYSize(double value) {}
	virtual InitializeBatchType InitializeBatch() { return InitializeBatchType(); }
	virtual void SetInitializeBatch(InitializeBatchType value) {}
	virtual Display3DType Display3D() { return Display3DType(); }
	virtual void SetDisplay3D(Display3DType value) {}
	virtual void SetOutLine(int (*value)(BYTE *pixels, int length)) {}
	virtual void OutLineCleanup() {}
	virtual void SetOutLineCleanup(void (*value)()) {}
	virtual bool CompareGIF() { return false; }
	virtual bool Potential16Bit() { return false; }
	virtual void SetPotential16Bit(bool value) {}
	virtual void SetPotentialFlag(bool value) {}
	virtual int DebugMode() { return 0; }
	virtual bool ZoomOff() { return false; }
	virtual void SetZoomOff(bool value) {}
	virtual bool Loaded3D() { return false; }
	virtual int SaveTime() { return 0; }
	virtual int CurrentFractalSpecificFlags() { return 0; }
	virtual void SetSaveBase(long value) {}
	virtual void SetSaveTicks(long value) {}
	virtual void SetFinishRow(int value) {}
	virtual void SetNameStackPointer(int value) {}

private:
	FakeViewWindow _viewWindow;
};

#endif
