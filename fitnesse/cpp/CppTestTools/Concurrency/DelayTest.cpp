//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "UnitTestHarness/TestHarness.h"
#include "Delay.h"
#include "Thread.h"
#include "ActionExecutingRunnable.h"
#include "FlagSettingAction.h"

EXPORT_TEST_GROUP(Delay);

namespace
  {
  void SetUp()
  {}
  void TearDown()
  {}
}

TEST(Delay, Delay)
{
  Delay::ms(1);
}

TEST(Delay, Yield)
{
  Delay::yield();
}
//This test is not deterministic on a dual/multi core machine
IGNORE_TEST(Delay, YieldGivesUpProcessor)
{
  bool runnableRan = false;
  Runnable* r = new ActionExecutingRunnable(new FlagSettingAction(runnableRan));
  Thread thread(r);
  CHECK(!runnableRan);
  thread.start();
  Delay::yield();
  CHECK(runnableRan);
  thread.join();
}


