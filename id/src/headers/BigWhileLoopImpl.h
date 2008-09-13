#if !defined(BIG_WHILE_LOOP_IMPL_H)
#define BIG_WHILE_LOOP_IMPL_H

#include "drivers.h"
#include "globals.h"
#include "Externals.h"
#include "BigWhileLoop.h"

class BigWhileLoopImpl : public BigWhileLoop
{
public:
	BigWhileLoopImpl(bool &kbdmore, bool &screen_stacked, bool resume_flag,
			IBigWhileLoopApplication *app,
			IBigWhileLoopGlobals &data,
			AbstractDriver *driver = DriverManager::current(),
			IGlobals &globals = g_,
			Externals &externs = g_externs)
		: BigWhileLoop(),
		_keyboardMore(kbdmore),
		_screenStacked(screen_stacked),
		_resumeFlag(resume_flag),
		_driver(driver),
		_app(app),
		_data(data),
		_g(globals),
		_externs(externs)
	{
	}

	virtual ApplicationStateType Execute();

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
	IGlobals &_g;
	Externals &_externs;
};

#endif
