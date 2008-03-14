#pragma once
#include <cstdlib>
#include "port.h"
#include "id.h"
#include "drivers.h"

class Globals;

class IBigWhileLoopApplication
{
public:
	virtual ~IBigWhileLoopApplication() {}

	virtual ApplicationStateType evolver_menu_switch(int &kbdchar, bool &julia_entered_from_manelbrot, bool &kbdmore, bool &stacked) = 0;
	virtual ApplicationStateType main_menu_switch(int &kbdchar, bool &frommandel, bool &kbdmore, bool &screen_stacked) = 0;
	virtual int stop_message(int flags, const std::string &message) = 0;
	virtual void change_video_mode(const VIDEOINFO &mode) = 0;
	virtual void load_dac() = 0;
	virtual int disk_start_potential() = 0;
	virtual void disk_video_status(int line, const char *msg) = 0;
	virtual int get_fractal_type() = 0;
	virtual int funny_glasses_call(int (*calc)()) = 0;
	virtual int text_temp_message(const char *msgparm) = 0;
	virtual bf_t copy_bf(bf_t r, bf_t n) = 0;
	virtual void history_save_info() = 0;
	virtual void save_parameter_history() = 0;
	virtual void restore_parameter_history() = 0;
	virtual void fiddle_parameters(GENEBASE gene[], int ecount) = 0;

	virtual void calculate_fractal_initialize() = 0;
	virtual int calculate_fractal() = 0;

	virtual void spiral_map(int count) = 0;
	virtual int unspiral_map() = 0;

	virtual int main_menu(bool full_menu) = 0;
	virtual void goodbye() = 0;
	virtual void zoom_box_draw(bool drawit) = 0;

	virtual long read_ticker() = 0;
};

class ViewWindow;

class IBigWhileLoopGlobals
{
public:
	virtual ~IBigWhileLoopGlobals() {}

	virtual int EvolvingFlags() = 0;
	virtual void SetEvolvingFlags(int value) = 0;
	virtual bool QuickCalculate() = 0;
	virtual void SetQuickCalculate(bool value) = 0;
	virtual void SetUserStandardCalculationMode(CalculationMode value) = 0;
	virtual CalculationMode StandardCalculationModeOld() = 0;
	virtual CalculationStatusType CalculationStatus() = 0;
	virtual void SetCalculationStatus(CalculationStatusType value) = 0;
	virtual ShowFileType ShowFile() = 0;
	virtual void SetShowFile(ShowFileType value) = 0;
	virtual int ScreenWidth() = 0;
	virtual void SetScreenWidth(int value) = 0;
	virtual int ScreenHeight() = 0;
	virtual void SetScreenHeight(int value) = 0;
	virtual int XDots() = 0;
	virtual void SetXDots(int value) = 0;
	virtual int YDots() = 0;
	virtual void SetYDots(int value) = 0;
	virtual float ScreenAspectRatio() = 0;
	virtual int ScreenXOffset() = 0;
	virtual int ScreenYOffset() = 0;
	virtual void SetScreenOffset(int x, int y) = 0;
	virtual int Colors() = 0;
	virtual void SetColors(int value) = 0;
	virtual ViewWindow &GetViewWindow() = 0;
	virtual int RotateHigh() = 0;
	virtual void SetRotateHigh(int value) = 0;
	virtual bool Overlay3D() = 0;
	virtual void SetOverlay3D(bool value) = 0;
	virtual bool ColorPreloaded() = 0;
	virtual void SetColorPreloaded(bool value) = 0;
	virtual void SetDXSize(double value) = 0;
	virtual void SetDYSize(double value) = 0;
	virtual InitializeBatchType InitializeBatch() = 0;
	virtual void SetInitializeBatch(InitializeBatchType value) = 0;
	virtual Display3DType Display3D() = 0;
	virtual void SetDisplay3D(Display3DType value) = 0;
	virtual void SetOutLine(int (*value)(BYTE *pixels, int length)) = 0;
	virtual void OutLineCleanup() = 0;
	virtual void SetOutLineCleanup(void (*value)()) = 0;
	virtual bool CompareGIF() = 0;
	virtual bool Potential16Bit() = 0;
	virtual void SetPotential16Bit(bool value) = 0;
	virtual void SetPotentialFlag(bool value) = 0;
	virtual int DebugMode() = 0;
	virtual bool ZoomOff() = 0;
	virtual void SetZoomOff(bool value) = 0;
	virtual bool Loaded3D() = 0;
	virtual int SaveTime() = 0;
	virtual int CurrentFractalSpecificFlags() = 0;
	virtual void SetSaveBase(long value) = 0;
	virtual void SetSaveTicks(long value) = 0;
	virtual void SetFinishRow(int value) = 0;
	virtual void SetNameStackPointer(int value) = 0;
};

class BigWhileLoop
{
public:
	BigWhileLoop(bool &kbdmore, bool &screen_stacked, bool resume_flag,
		IBigWhileLoopApplication *app,
		IBigWhileLoopGlobals &data,
		AbstractDriver *driver = DriverManager::current(),
		Globals &globals = g_)
		: _keyboardMore(kbdmore),
		_screenStacked(screen_stacked),
		_resumeFlag(resume_flag),
		_driver(driver),
		_app(app),
		_data(data),
		_g(g_)
	{
	}
	ApplicationStateType Execute();

private:
	ApplicationStateType GetMainMenuState(bool julia_entered_from_mandelbrot, int kbdchar);
	bool StatusNotResumableOrShowFilePending();
	void HandleVisibleViewWindow();

	bool &_keyboardMore;
	bool &_screenStacked;
	bool _resumeFlag;
	AbstractDriver *_driver;
	IBigWhileLoopApplication *_app;
	IBigWhileLoopGlobals &_data;
	Globals &_g;
};

extern ApplicationStateType big_while_loop(bool &keyboardMore, bool &screenStacked, bool resumeFlag);
