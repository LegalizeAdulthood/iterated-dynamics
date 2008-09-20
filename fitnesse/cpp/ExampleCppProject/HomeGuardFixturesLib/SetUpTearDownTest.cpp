//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Platform.h"
#include "UnitTestHarness/TestHarness.h"
#include "SetUpHomeGuard.h"
#include "TearDownHomeGuard.h"
#include "HomeGuardContext.h"

EXPORT_TEST_GROUP(SetUpTearDown)

namespace
{
  void SetUp() 
  {
  }
  void TearDown()
  {
  }
}

TEST(SetUpTearDown, Create)
{
  SetUpHomeGuard s;
  CHECK(0 != HomeGuardContext::GetHomeGuard());
  TearDownHomeGuard t;
  CHECK(0 == HomeGuardContext::GetHomeGuard());
}

