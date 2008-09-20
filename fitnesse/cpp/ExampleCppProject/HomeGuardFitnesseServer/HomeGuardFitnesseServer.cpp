//
// Copyright (c) 2004 Micahel Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//
#include "Platform.h"
#include <iostream>
#include "Fit/FitnesseServer.h"
#include "../HomeGuardFixturesLib/HomeGuardFixtureMaker.h"
#include "UnitTestHarness/MemoryLeakWarning.h"

int main(int argc, char* argv[])
{
	MemoryLeakWarning::Enable();
  FixtureMaker* maker = new HomeGuardFixtureMaker();
	int status = FitnesseServer::Main(argc, argv, maker);
	return status;
}

