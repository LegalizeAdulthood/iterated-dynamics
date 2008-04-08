#if !defined(BIG_WHILE_LOOP_H)
#define BIG_WHILE_LOOP_H

#include <cstdlib>
#include "port.h"
#include "id.h"

#include "big.h"
#include "calcfrac.h"
#include "drivers.h"

class Globals;
class Externals;
extern Externals &g_externs;

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
	virtual void fiddle_parameters(GENEBASE *gene, int ecount) = 0;

	virtual void calculate_fractal_initialize() = 0;
	virtual int calculate_fractal() = 0;

	virtual void spiral_map(int count) = 0;
	virtual int unspiral_map() = 0;

	virtual int main_menu(bool full_menu) = 0;
	virtual void goodbye() = 0;
	virtual void zoom_box_draw(bool drawit) = 0;

	virtual long read_ticker() = 0;

	virtual int check_video_mode_key(int k) = 0;

	virtual int clock_ticks() = 0;
};

class ViewWindow;

class IBigWhileLoopGlobals
{
public:
	virtual ~IBigWhileLoopGlobals() {}

	virtual CalculationMode StandardCalculationModeOld() = 0;
	virtual int ScreenXOffset() = 0;
	virtual int ScreenYOffset() = 0;
	virtual void SetScreenOffset(int x, int y) = 0;
	virtual ViewWindow &GetViewWindow() = 0;
	virtual int RotateHigh() = 0;
	virtual void SetRotateHigh(int value) = 0;
	virtual bool CompareGIF() = 0;
	virtual int CurrentFractalSpecificFlags() = 0;
	virtual void SetNameStackPointer(int value) = 0;
};

class BigWhileLoop
{
public:
	virtual ~BigWhileLoop() { }

	virtual ApplicationStateType Execute() = 0;
};

extern ApplicationStateType big_while_loop(bool &keyboardMore, bool &screenStacked, bool resumeFlag);

#endif
