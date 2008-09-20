//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Platform.h"
#include "UnitTestHarness/CommandLineTestRunner.h"
#include "UnitTestHarness/UnitTestHarnessTests.h"
#include "Fit/FitUnitTests.h"
#include "Concurrency/ConcurrencyTests.h"
#include "Helpers/HelpersTests.h"

int main(int ac, char** av)
{
  return CommandLineTestRunner::RunAllTests(ac, av);
}
