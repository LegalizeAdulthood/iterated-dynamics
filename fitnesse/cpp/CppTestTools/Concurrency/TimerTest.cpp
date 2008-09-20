#include "UnitTestHarness/TestHarness.h"
#include "Timer.h"
#include "FlagSettingAction.h"

EXPORT_TEST_GROUP(Timer)

namespace
  {
  Timer* timer;

  void SetUp()
  {
    timer = new Timer();
  }
  void TearDown()
  {
    delete timer;
  }
}

TEST(Timer, Create)
{}

TEST(Timer, OneSubscriberEveryTic)
{
  bool wasExecuted = false;
  FlagSettingAction* action = new FlagSettingAction(wasExecuted);

  timer->notifyPeriodically(1, action);
  CHECK(!wasExecuted);

  for (int i = 0; i < 5; i++)
    {
      timer->tic();
      CHECK(wasExecuted);
    }

}

TEST(Timer, OneSubscriberNotEveryTic)
{
  bool wasExecuted = false;
  FlagSettingAction* action = new FlagSettingAction(wasExecuted);

  timer->notifyPeriodically(2, action);
  CHECK(!wasExecuted);

  for (int i = 0; i < 5; i++)
    {
      wasExecuted = false;
      timer->tic();
      CHECK(!wasExecuted);
      timer->tic();
      CHECK(wasExecuted);
    }

}

