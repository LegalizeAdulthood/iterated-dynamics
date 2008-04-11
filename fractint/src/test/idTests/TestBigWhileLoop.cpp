#include "stdafx.h"
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

#include <boost/test/unit_test.hpp>

#include "FakeDriver.h"
#include "FakeGlobals.h"
#include "FakeExternals.h"
#include "FakeBigWhileLoopApplication.h"
#include "FakeBigWhileLoopGlobals.h"

BOOST_AUTO_TEST_CASE(BigWhileLoop_Construct)
{
	bool keyboardMore = false;
	bool screenStacked = false;
	FakeDriver driver;
	FakeBigWhileLoopApplication app;
	FakeBigWhileLoopGlobals data;
	FakeGlobals globals;
	FakeExternals externs;
	BigWhileLoopImpl looper(keyboardMore, screenStacked, false, &app, data, &driver, globals, externs);

	BOOST_CHECK_MESSAGE(true, "should have constructed a BigWhileLoop");
}

BOOST_AUTO_TEST_CASE(BigWhileLoop_Execute)
{
	bool keyboardMore = false;
	bool screenStacked = false;
	FakeDriver driver;
	FakeBigWhileLoopApplication app;
	FakeBigWhileLoopGlobals data;
	FakeGlobals globals;
	FakeExternals externs;
	BigWhileLoopImpl looper(keyboardMore, screenStacked, false, &app, data, &driver, globals, externs);

	externs.SetShowFileFakeResult(SHOWFILE_DONE);
	externs.SetCalculationStatusFakeResult(CALCSTAT_RESUMABLE);
	app.SetMainMenuSwitchFakeResult(APPSTATE_IMAGE_START);
	ApplicationStateType result = looper.Execute();

	BOOST_CHECK(externs.CalculationStatusCalled());
	BOOST_CHECK(externs.ShowFileCalled());
	BOOST_CHECK(globals.SaveDACCalled());
	BOOST_CHECK(globals.SetSaveDACCalled());
	BOOST_CHECK(APPSTATE_IMAGE_START == result);
}
