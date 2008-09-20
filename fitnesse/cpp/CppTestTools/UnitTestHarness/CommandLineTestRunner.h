//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_CommandLineTestRunner_H
#define D_CommandLineTestRunner_H

#include "TestHarness.h"
#include "TestOutput.h"

///////////////////////////////////////////////////////////////////////////////
//
// Main entry point for running a collection of unit tests
//
///////////////////////////////////////////////////////////////////////////////

class CommandLineTestRunner
  {
  public:
    static int RunAllTests(int ac, char** av);

  private:
    static TestOutput* output;
    static void SetOptions(int ac, char** av);
    static void SetRepeatCount(int ac, char** av, int& index);
    static void SetGroupFilter(int ac, char** av, int& index);
    static void SetNameFilter(int ac, char** av, int& index);
    static int repeat;
  };

#endif
