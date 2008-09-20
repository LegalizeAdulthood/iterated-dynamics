//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "UnitTestHarness/TestHarness.h"
#include "Mutex.h"
#include "Action.h"
#include "Thread.h"
#include "ActionExecutingRunnable.h"
#include "EventSettingAction.h"
#include "ScopeLock.h"
#include "Delay.h"
#include "Event.h"

EXPORT_TEST_GROUP(ScopeLock);

class ScopeLockTestingAction : public Action
  {
  public:
    ScopeLockTestingAction(Mutex* m) : itsMutex(m), itsState(0)
    {}

    void execute()
    {
      itsState = 1;
      running.signal();
      {
        ScopeLock lock(itsMutex)
          ;
        itsState = 2;
      }
      unlocked.signal();
    }

    int getState()
    {
      return itsState;
    }
    
    void waitTilRunning()
    {
        running.wait();
    }
    
    void waitTilUnlocked()
    {
        unlocked.wait();
    }

  private:
    Mutex* itsMutex;
    Event running;
    Event unlocked;
    int itsState;
  };


namespace
  {
  void SetUp()
  {}
  void TearDown()
  {}
  ;
};

TEST(ScopeLock, BasicInterface)
{
  Mutex* m = new Mutex();
  {
    ScopeLock lock(m)
      ;
  }
  delete m;
}


TEST(ScopeLock, NestedLockDoesNotDeadlock)
{
  Mutex* m = new Mutex();
  {
    ScopeLock lock(m)
      ;
    {
      ScopeLock lock(m)
        ;
    }
  }
  delete m;
}


TEST(ScopeLock, InteractionWithOtherThread)
{
  Mutex* m = new Mutex();
  ScopeLockTestingAction* a = new ScopeLockTestingAction(m);
  Thread* t = new Thread(new ActionExecutingRunnable(a));

  {
    ScopeLock lock(m)
      ;
    LONGS_EQUAL(0, a->getState());
    t->start();
    a->waitTilRunning();
    LONGS_EQUAL(1, a->getState());
  }

  a->waitTilUnlocked();
  LONGS_EQUAL(2, a->getState());
  t->join();

  delete m;
  delete t;
}

