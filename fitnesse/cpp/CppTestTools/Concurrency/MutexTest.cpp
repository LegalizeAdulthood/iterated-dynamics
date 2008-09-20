//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "UnitTestHarness/TestHarness.h"
#include "Mutex.h"
#include "Action.h"
#include "Thread.h"
#include "ActionExecutingRunnable.h"
#include "Delay.h"
#include "Event.h"

EXPORT_TEST_GROUP(Mutex);

class MutexTestingAction : public Action
  {
  public:
    enum { INITIAL=0, STARTED, DONE };
    MutexTestingAction(Mutex* m, int& state) : itsMutex(m), theState(state)
    {
      theState = MutexTestingAction::INITIAL;
    }

    void execute()
    {
      theState = MutexTestingAction::STARTED;
      started.signal();
      itsMutex->acquire();
      itsMutex->release();
      theState = MutexTestingAction::DONE;
      done.signal();
    }
    
    void waitStarted()
    {
        started.wait();
    }
    
    void waitDone()
    {
        done.wait();
    }

  private:
    Mutex* itsMutex;
    int& theState;
    Event started;
    Event done;
  };

namespace
  {
  void SetUp()
  {}
  void TearDown()
  {}
  ;
};

TEST(Mutex, BasicInterface)
{
  Mutex* m = new Mutex();
  m->acquire();
  m->release();
  delete m;
}


TEST(Mutex, BlockSeparateThread)
{
  Mutex* m = new Mutex();

  int state;

  MutexTestingAction* a = new MutexTestingAction(m, state);
  Thread* t = new Thread(new ActionExecutingRunnable(a));

  m->acquire();
  CHECK(state == MutexTestingAction::INITIAL);
  t->start();
  a->waitStarted();
  CHECK(state != MutexTestingAction::DONE);
  m->release();
  a->waitDone();
  m->acquire();
  t->join();

  delete m;
  delete t;
}

//these ignored tests have cuased trouble due to environment specific
//behavior differences.  CppTestTools does not depend on these tests 
//passing so they have been put into IGNORE_ state
TEST(Mutex, NestedAcquireDoesNotDeadlock)
{
  Mutex* m = new Mutex();

  m->acquire();
  m->acquire();
  delete m;
}


TEST(Mutex, TwoAcquiresNeedsTwoReleases)
{
  Mutex* m = new Mutex();

  int state;

  MutexTestingAction* a = new MutexTestingAction(m, state);
  Thread* t = new Thread(new ActionExecutingRunnable(a));

  m->acquire();
  m->acquire();
  CHECK(state == MutexTestingAction::INITIAL);
  t->start();
  a->waitStarted();
  CHECK(state == MutexTestingAction::STARTED);
  m->release();
  Delay::yield();
  CHECK(state == MutexTestingAction::STARTED);
  m->release();
  a->waitDone();
  LONGS_EQUAL(MutexTestingAction::DONE, state);
  t->join();

  delete m;
  delete t;
}

TEST(Mutex, MultipleReleasesDoNoHarm)
{
  Mutex* m = new Mutex();

  int state;

  MutexTestingAction* a = new MutexTestingAction(m, state);
  Thread* t = new Thread(new ActionExecutingRunnable(a));

  m->acquire();
  m->release();
  m->release();

  m->acquire();
  CHECK(state == MutexTestingAction::INITIAL);
  t->start();
  a->waitStarted();
  m->release();
  a->waitDone();
  t->join();

  delete m;
  delete t;
}

