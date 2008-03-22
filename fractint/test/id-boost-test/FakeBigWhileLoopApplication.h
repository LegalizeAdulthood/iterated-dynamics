#pragma once

class FakeBigWhileLoopApplication : public IBigWhileLoopApplication
{
public:
	FakeBigWhileLoopApplication() : IBigWhileLoopApplication(),
		_mainMenuSwitchCalled(false),
		_mainMenuSwitchLastKbdChar(0),
		_mainMenuSwitchLastFromMandel(false),
		_mainMenuSwitchLastKbdMore(false),
		_mainMenuSwitchLastScreenStacked(false),
		_mainMenuSwitchFakeResult()
	{}
	virtual ~FakeBigWhileLoopApplication() {}

	virtual ApplicationStateType evolver_menu_switch(int &kbdchar, bool &julia_entered_from_manelbrot, bool &kbdmore, bool &stacked)
	{ return ApplicationStateType(); }
	virtual ApplicationStateType main_menu_switch(int &kbdchar, bool &frommandel, bool &kbdmore, bool &screen_stacked)
	{
		_mainMenuSwitchCalled = true;
		_mainMenuSwitchLastKbdChar = kbdchar;
		_mainMenuSwitchLastFromMandel = frommandel;
		_mainMenuSwitchLastKbdMore = kbdmore;
		_mainMenuSwitchLastScreenStacked = screen_stacked;
		return _mainMenuSwitchFakeResult;
	}
	virtual int stop_message(int flags, const std::string &message)
	{ return 0; }
	virtual void change_video_mode(const VIDEOINFO &mode)
	{ }
	virtual void load_dac()
	{ }
	virtual int disk_start_potential()
	{ return 0; }
	virtual void disk_video_status(int line, const char *msg)
	{ }
	virtual int get_fractal_type()
	{ return 0; }
	virtual int funny_glasses_call(int (*calc)())
	{ return 0; }
	virtual int text_temp_message(const char *msgparm)
	{ return 0; }
	virtual bf_t copy_bf(bf_t r, bf_t n)
	{ return 0; }
	virtual void history_save_info()
	{ }
	virtual void save_parameter_history()
	{ }
	virtual void restore_parameter_history()
	{ }
	virtual void fiddle_parameters(GENEBASE gene[], int ecount)
	{ }

	virtual void calculate_fractal_initialize()
	{ }
	virtual int calculate_fractal()
	{ return 0; }

	virtual void spiral_map(int count)
	{ }
	virtual int unspiral_map()
	{ return 0; }

	virtual int main_menu(bool full_menu)
	{ return 0; }
	virtual void goodbye()
	{ }
	virtual void zoom_box_draw(bool drawit)
	{ }
	virtual long read_ticker()
	{ return 0; }
	virtual int check_video_mode_key(int k)
	{ return 0; }
	virtual int clock_ticks()
	{ return 0; }

	bool MainMenuSwitchCalled() const { return _mainMenuSwitchCalled; }
	int MainMenuSwitchLastKbdChar()const { return _mainMenuSwitchLastKbdChar; }
	bool MainMenuSwitchLastFromMandel() const { return _mainMenuSwitchLastFromMandel; }
	bool MainMenuSwitchLastKbdMore() const { return _mainMenuSwitchLastKbdMore; }
	bool MainMenuSwitchLastScreenStacked() const { return _mainMenuSwitchLastScreenStacked; }
	void SetMainMenuSwitchFakeResult(ApplicationStateType value) { _mainMenuSwitchFakeResult = value; }

private:
	bool _mainMenuSwitchCalled;
	int _mainMenuSwitchLastKbdChar;
	bool _mainMenuSwitchLastFromMandel;
	bool _mainMenuSwitchLastKbdMore;
	bool _mainMenuSwitchLastScreenStacked;
	ApplicationStateType _mainMenuSwitchFakeResult;
};
