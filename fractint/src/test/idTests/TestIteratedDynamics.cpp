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
#include "drivers.h"

#include "IteratedDynamicsImpl.h"

#include <boost/test/unit_test.hpp>

#include "FakeIteratedDynamicsApp.h"
#include "FakeDriver.h"
#include "FakeExternals.h"
#include "FakeGlobals.h"

BOOST_AUTO_TEST_CASE(IteratedDynamics_Construct)
{
	FakeIteratedDynamicsApp app;
	FakeDriver driver;
	FakeExternals externals;
	FakeGlobals globals;
	IteratedDynamicsImpl id(app, &driver, externals, globals, 0, 0);
}
