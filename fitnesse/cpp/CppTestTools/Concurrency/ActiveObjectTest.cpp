//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "UnitTestHarness/TestHarness.h"
#include "ActiveObject.h"
#include "FlagSettingAction.h"
#include "EventSettingAction.h"
#include "Event.h"
#include "Delay.h"

EXPORT_TEST_GROUP(ActiveObject);

namespace
  {
  void SetUp()
  {}
  void TearDown()
  {}
  ;
};

TEST(ActiveObject, Create)
{
  ActiveObject* activeObject = new ActiveObject();

  delete activeObject;
}

TEST(ActiveObject, AddActionNoStart)
{
  ActiveObject* activeObject = new ActiveObject();

  bool wasActionExecuted = false;
  FlagSettingAction* a = new FlagSettingAction(wasActionExecuted);
  activeObject->add
  (a);

  CHECK(wasActionExecuted == false);

  delete activeObject;
}

TEST(ActiveObject, AddActionThenStart)
{
  ActiveObject* activeObject = new ActiveObject();

  bool wasActionExecuted;
  FlagSettingAction* a = new FlagSettingAction(wasActionExecuted);
  activeObject->add
  (a);

  CHECK(wasActionExecuted == false);
  activeObject->start();
  activeObject->terminate();
  CHECK(wasActionExecuted == true);

  delete activeObject;
}


TEST(ActiveObject, StartThenAdd)
{
  ActiveObject* activeObject = new ActiveObject();

  bool wasActionExecuted;
  FlagSettingAction* a = new FlagSettingAction(wasActionExecuted);

  activeObject->start();
  activeObject->add
  (a);
  activeObject->terminate();
  CHECK(wasActionExecuted == true);

  delete activeObject;
}


TEST(ActiveObject, RunTwoActions)
{
  ActiveObject* activeObject = new ActiveObject();

  bool wasAction1Executed;
  FlagSettingAction* a1 = new FlagSettingAction(wasAction1Executed);
  activeObject->add
  (a1);

  bool wasAction2Executed;
  FlagSettingAction* a2 = new FlagSettingAction(wasAction2Executed);
  activeObject->add
  (a2);

  activeObject->start();
  activeObject->terminate();

  CHECK(wasAction1Executed == true);
  CHECK(wasAction2Executed == true);

  delete activeObject;
}


TEST(ActiveObject, ProveItsConcurrent)
{
  ActiveObject* activeObject = new ActiveObject();

  Event event;
  EventSettingAction* a1 = new EventSettingAction(event);

  activeObject->add(a1);
  CHECK(!event.isSignaled());
  activeObject->start();
  event.wait();

  bool wasActionExecuted;
  FlagSettingAction* a2 = new FlagSettingAction(wasActionExecuted);
  activeObject->add(a2);
  activeObject->terminate();
  CHECK(wasActionExecuted == true);

  delete activeObject;
}
