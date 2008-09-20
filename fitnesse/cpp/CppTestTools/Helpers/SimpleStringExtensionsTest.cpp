//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Platform.h"
#include "UnitTestHarness/TestHarness.h"
#include "SimpleStringExtensions.h"

EXPORT_TEST_GROUP(SimpleStringExtensions);

namespace
  {
  void SetUp()
  {}
  void TearDown()
  {}
  ;
};

using namespace std;

TEST(SimpleString, fromStdString)
{
  string s("hello");
  SimpleString s1(StringFrom(s));

  STRCMP_EQUAL("hello", s1.asCharString());
}
