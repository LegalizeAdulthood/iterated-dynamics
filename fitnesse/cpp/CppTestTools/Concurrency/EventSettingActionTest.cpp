//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "UnitTestHarness/TestHarness.h"
#include "EventSettingAction.h"

EXPORT_TEST_GROUP(EventSettingAction);

namespace
  {
  void SetUp()
  {}
  void TearDown()
  {}
  ;
};

TEST(EventSettingAction, Create)
{
  Event e;
  EventSettingAction* a = new EventSettingAction(e);

  CHECK(!e.isSignaled());
  a->execute();
  CHECK(e.isSignaled());
  delete a;
}


