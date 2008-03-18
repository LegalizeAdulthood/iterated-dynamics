#include <fstream>
#include <string>

#include <string.h>
#include <time.h>
#include <ctype.h>

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
#include "fihelp.h"
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

	virtual int EvolvingFlags()											{ return g_evolving_flags; }
	virtual void SetEvolvingFlags(int value)							{ g_evolving_flags = value; }
	virtual bool QuickCalculate()										{ return g_quick_calculate; }
	virtual void SetQuickCalculate(bool value)							{ g_quick_calculate = value; }
	virtual void SetUserStandardCalculationMode(CalculationMode value)	{ g_user_standard_calculation_mode = value; }
	virtual CalculationMode StandardCalculationModeOld()				{ return g_standard_calculation_mode_old; }
	virtual CalculationStatusType CalculationStatus()					{ return CalculationStatusType(g_calculation_status); }
	virtual void SetCalculationStatus(CalculationStatusType value)		{ g_calculation_status = int(value); }
	virtual ShowFileType ShowFile()										{ return g_show_file; }
	virtual void SetShowFile(ShowFileType value)						{ g_show_file = value; }
	virtual int ScreenWidth()											{ return g_screen_width; }
	virtual void SetScreenWidth(int value)								{ g_screen_width = value; }
	virtual int ScreenHeight()											{ return g_screen_height; }
	virtual void SetScreenHeight(int value)								{ g_screen_height = value; }
	virtual int XDots()													{ return g_x_dots; }
	virtual void SetXDots(int value)									{ g_x_dots = value; }
	virtual int YDots()													{ return g_y_dots; }
	virtual void SetYDots(int value)									{ g_y_dots = value; }
	virtual float ScreenAspectRatio()									{ return g_screen_aspect_ratio; }
	virtual int ScreenXOffset()											{ return g_screen_x_offset; }
	virtual int ScreenYOffset()											{ return g_screen_y_offset; }
	virtual void SetScreenOffset(int x, int y)
	{
		g_screen_x_offset = x;
		g_screen_y_offset = y;
	}
	virtual int Colors()												{ return g_colors; }
	virtual void SetColors(int value)									{ g_colors = value; }
	virtual ViewWindow &GetViewWindow()									{ return g_viewWindow; }
	virtual int RotateHigh()											{ return g_rotate_hi; }
	virtual void SetRotateHigh(int value)								{ g_rotate_hi = value; }
	virtual bool Overlay3D()											{ return g_overlay_3d; }
	virtual void SetOverlay3D(bool value)								{ g_overlay_3d = value; }
	virtual bool ColorPreloaded()										{ return g_color_preloaded; }
	virtual void SetColorPreloaded(bool value)							{ g_color_preloaded = value; }
	virtual void SetDXSize(double value)								{ g_dx_size = value; }
	virtual void SetDYSize(double value)								{ g_dy_size = value; }
	virtual InitializeBatchType InitializeBatch()						{ return g_initialize_batch; }
	virtual void SetInitializeBatch(InitializeBatchType value)			{ g_initialize_batch = value; }
	virtual Display3DType Display3D()									{ return g_display_3d; }
	virtual void SetDisplay3D(Display3DType value)						{ g_display_3d = value; }
	virtual void SetOutLine(int (*value)(BYTE *pixels, int length))		{ g_out_line = value; }
	virtual void OutLineCleanup()										{ g_out_line_cleanup(); }
	virtual void SetOutLineCleanup(void (*value)())						{ g_out_line_cleanup = value; }
	virtual bool CompareGIF()											{ return g_compare_gif; }
	virtual bool Potential16Bit()										{ return g_potential_16bit; }
	virtual void SetPotential16Bit(bool value)							{ g_potential_16bit = value; }
	virtual void SetPotentialFlag(bool value)							{ g_potential_flag = value; }
	virtual int DebugMode()												{ return g_debug_mode; }
	virtual bool ZoomOff()												{ return g_zoom_off; }
	virtual void SetZoomOff(bool value)									{ g_zoom_off = value; }
	virtual bool Loaded3D()												{ return g_loaded_3d; }
	virtual int SaveTime()												{ return g_save_time; }
	virtual int CurrentFractalSpecificFlags()
	{ return g_current_fractal_specific->flags; }
	virtual void SetSaveBase(long value)								{ g_save_base = value; }
	virtual void SetSaveTicks(long value)								{ g_save_ticks = value; }
	virtual void SetFinishRow(int value)								{ g_finish_row = value; }
	virtual void SetNameStackPointer(int value)							{ g_name_stack_ptr = value; }
};

ApplicationStateType big_while_loop(bool &keyboardMore, bool &screenStacked, bool resumeFlag)
{
	BigWhileLoopApplication app;
	BigWhileLoopGlobals data;
	return BigWhileLoopImpl(keyboardMore, screenStacked, resumeFlag, &app, data).Execute();
}
