//
// Copyright (c) 2004 Micahel Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//
#include <iostream>
#include "Fit/FitnesseServer.h"
#include "Platform.h"
#include "UnitTestHarness/MemoryLeakWarning.h"
#include "Fit/FitFixtureMaker.h"

int main(int argc, char* argv[])
{
  MemoryLeakWarning::Enable();
  int status = FitnesseServer::Main(argc, argv, new FitFixtureMaker());
  return status;
}

