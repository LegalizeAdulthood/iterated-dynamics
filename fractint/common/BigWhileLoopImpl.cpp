#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "port.h"
#include "id.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#include "ant.h"
#include "Browse.h"
#include "calcfrac.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "encoder.h"
#include "evolve.h"
#include "idhelp.h"
#include "filesystem.h"
#include "fracsubr.h"
#include "fractals.h"
#include "framain2.h"
#include "gifview.h"
#include "history.h"
#include "jiim.h"
#include "line3d.h"
#include "loadfile.h"
#include "lorenz.h"
#include "miscovl.h"
#include "miscfrac.h"
#include "miscres.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"
#include "rotate.h"
#include "stereo.h"
#include "zoom.h"
#include "ZoomBox.h"

#include "BigWhileLoop.h"
#include "EscapeTime.h"
#include "CommandParser.h"
#include "FrothyBasin.h"
#include "SoundState.h"
#include "ViewWindow.h"
#include "BigWhileLoopImpl.h"

extern ApplicationStateType main_menu_switch(int &kbdchar, bool &frommandel, bool &kbdmore, bool &screen_stacked);
extern ApplicationStateType evolver_menu_switch(int &kbdchar, bool &julia_entered_from_manelbrot, bool &kbdmore, bool &stacked);

class BigWhileLoopApplication : public IBigWhileLoopApplication
{
public:
	BigWhileLoopApplication() {}
	virtual ~BigWhileLoopApplication() {}

	virtual ApplicationStateType evolver_menu_switch(int &kbdchar, bool &julia_entered_from_manelbrot, bool &kbdmore, bool &stacked)
	{ return ::evolver_menu_switch(kbdchar, julia_entered_from_manelbrot, kbdmore, stacked); }
	virtual ApplicationStateType main_menu_switch(int &kbdchar, bool &frommandel, bool &kbdmore, bool &screen_stacked)
	{ return ::main_menu_switch(kbdchar, frommandel, kbdmore, screen_stacked); }
	virtual int stop_message(int flags, const std::string &message)
	{ return ::stop_message(flags, message); }
	virtual void change_video_mode(const VIDEOINFO &mode)
	{ DriverManager::change_video_mode(mode); }
	virtual void load_dac()
	{ ::load_dac(); }
	virtual int disk_start_potential()
	{ return ::disk_start_potential(); }
	virtual void disk_video_status(int line, const char *msg)
	{ ::disk_video_status(line, msg); }
	virtual int get_fractal_type()
	{ return ::get_fractal_type(); }
	virtual int funny_glasses_call(int (*calc)())
	{ return ::funny_glasses_call(calc); }
	virtual int text_temp_message(const char *msgparm)
	{ return ::text_temp_message(msgparm); }
	virtual bf_t copy_bf(bf_t r, bf_t n)
	{ return ::copy_bf(r, n); }
	virtual void history_save_info()
	{ ::history_save_info(); }
	virtual void save_parameter_history()
	{ ::save_parameter_history(); }
	virtual void restore_parameter_history()
	{ ::restore_parameter_history(); }
	virtual void fiddle_parameters(GENEBASE *gene, int ecount)
	{ ::fiddle_parameters(gene, ecount); }

	virtual void calculate_fractal_initialize()
	{ ::calculate_fractal_initialize(); }
	virtual int calculate_fractal()
	{ return ::calculate_fractal(); }

	virtual void spiral_map(int count)
	{ ::spiral_map(count); }
	virtual int unspiral_map()
	{ return ::unspiral_map(); }

	virtual int main_menu(bool full_menu)
	{ return ::main_menu(full_menu); }
	virtual void goodbye()
	{ ::goodbye(); }
	virtual void zoom_box_draw(bool drawit)
	{ ::zoom_box_draw(drawit); }
	virtual long read_ticker()
	{ return ::read_ticker(); }
	virtual int check_video_mode_key(int k)
	{ return ::check_video_mode_key(k); }
	virtual int clock_ticks()
	{ return ::clock_ticks(); }
};

class BigWhileLoopGlobals : public IBigWhileLoopGlobals
{
public:
	BigWhileLoopGlobals() { }
	virtual ~BigWhileLoopGlobals() { }

	virtual CalculationMode StandardCalculationModeOld()				{ return g_standard_calculation_mode_old; }
	virtual int ScreenXOffset()											{ return g_screen_x_offset; }
	virtual int ScreenYOffset()											{ return g_screen_y_offset; }
	virtual void SetScreenOffset(int x, int y)
	{
		g_screen_x_offset = x;
		g_screen_y_offset = y;
	}
	virtual ViewWindow &GetViewWindow()									{ return g_viewWindow; }
	virtual int RotateHigh()											{ return g_rotate_hi; }
	virtual void SetRotateHigh(int value)								{ g_rotate_hi = value; }
	virtual bool CompareGIF()											{ return g_compare_gif; }
	virtual int CurrentFractalSpecificFlags()
	{ return g_current_fractal_specific->flags; }
	virtual void SetNameStackPointer(int value)							{ g_name_stack_ptr = value; }
};

ApplicationStateType big_while_loop(bool &keyboardMore, bool &screenStacked, bool resumeFlag)
{
	BigWhileLoopApplication app;
	BigWhileLoopGlobals data;
	return BigWhileLoopImpl(keyboardMore, screenStacked, resumeFlag, &app, data).Execute();
}
