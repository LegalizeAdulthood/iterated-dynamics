//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "UnitTestHarness/TestHarness.h"
#include "FlagSettingAction.h"

EXPORT_TEST_GROUP(FlagSettingAction);

namespace
  {
  void SetUp()
  {}
  void TearDown()
  {}
  ;
};

TEST(FlagSettingAction, Create)
{
  bool wasActionExecuted = false;
  FlagSettingAction* a = new FlagSettingAction(wasActionExecuted);

  CHECK(wasActionExecuted == false);
  a->execute();
  CHECK(wasActionExecuted == true);
  a->reset();
  CHECK(wasActionExecuted == false);
  delete a;
}


