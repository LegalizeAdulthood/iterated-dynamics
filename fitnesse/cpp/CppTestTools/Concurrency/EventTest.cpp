//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "UnitTestHarness/TestHarness.h"
#include "Event.h"
#include "Action.h"
#include "WaitingAction.h"
#include "ActiveObject.h"

EXPORT_TEST_GROUP(Event);

namespace
{
  Event* e;
  void SetUp()
  {
    e = new Event();
  }
  void TearDown()
  {
  	delete e;
  }
};

TEST(Event, Create)
{
  CHECK(!e->isSignaled());
}

TEST(Event, SignalAndWaitInSameThread)
{
  e->signal();
  e->wait();
  CHECK(!e->isSignaled());
}

TEST(Event, IsSignaledDoesnNotResetEvent)
{
  e->signal();
  CHECK(e->isSignaled());
  CHECK(e->isSignaled());
  CHECK(e->isSignaled());
}

TEST(Event, WaitDoesResetEvent)
{
  e->signal();
  e->wait();
  CHECK(!e->isSignaled());
}

TEST(Event, MultiplSignalsNeedOneWait)
{
  e->signal();
  e->signal();
  e->wait();
  CHECK(!e->isSignaled());
}

TEST(Event, SignalAndWaitInDifferentThreads)
{
  ActiveObject* activeObject = new ActiveObject();
  Action* a = new WaitingAction(*e);
  activeObject->add(a);
  activeObject->start();
  e->signal();
  activeObject->terminate();
  CHECK(!e->isSignaled());
  delete activeObject;
}
