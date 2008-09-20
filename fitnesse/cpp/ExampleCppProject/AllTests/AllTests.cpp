//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "UnitTestHarness/CommandLineTestRunner.h"

int main(int ac, char** av)
{
    return CommandLineTestRunner::RunAllTests(ac, av);
}

IMPORT_TEST_GROUP(HomeGuardFixtureMaker);
IMPORT_TEST_GROUP(HomeGuard);
IMPORT_TEST_GROUP(MockObjects);
IMPORT_TEST_GROUP(LogEntry);
IMPORT_TEST_GROUP(Stack);

