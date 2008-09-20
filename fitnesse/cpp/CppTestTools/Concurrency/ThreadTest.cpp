//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "UnitTestHarness/TestHarness.h"
#include "ActionExecutingRunnable.h"
#include "FlagSettingAction.h"
#include "EventSettingAction.h"
#include "Thread.h"
#include "Delay.h"
#include "Event.h"

EXPORT_TEST_GROUP(Thread);

namespace
  {
  void SetUp()
  {}
  void TearDown()
  {}
  ;
};

TEST(Thread, StartRunJoin)
{
  bool runnableRan = false;
  Runnable* r = new ActionExecutingRunnable(new FlagSettingAction(runnableRan));
  Thread thread(r);
  CHECK(!runnableRan);
  thread.start();
  thread.join();
  CHECK(runnableRan);
}

TEST(Thread, StartYieldCheckJoinTest)
{
  Event e;
  Runnable* r = new ActionExecutingRunnable(new EventSettingAction(e));
  Thread thread(r);
  CHECK(!e.isSignaled());
  thread.start();
  e.wait();
  thread.join();
}

TEST(Thread, JoinANeverStartedThread)
{
  bool runnableRan = false;
  Runnable* r = new ActionExecutingRunnable(new FlagSettingAction(runnableRan));
  Thread thread(r);
  thread.join();
}



