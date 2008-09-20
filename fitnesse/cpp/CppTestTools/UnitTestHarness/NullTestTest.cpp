//
// Copyright (c) 2005 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "TestHarness.h"
#include "NullTest.h"

EXPORT_TEST_GROUP(NullTest);

namespace
  {
  NullTest* nullTest;

  void SetUp()
  {
    nullTest = new NullTest();
  }
  void TearDown()
  {
    delete nullTest;
  }
}

TEST(NullTest, Create)
{}

