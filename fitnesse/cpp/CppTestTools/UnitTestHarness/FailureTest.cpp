//
// Copyright (c) 200 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Platform.h"
#include "TestHarness.h"
#include "MockTestOutput.h"
#include "NullTest.h"

EXPORT_TEST_GROUP(Failure);

namespace
  {
  const int failLineNumber = 2;
}

namespace
  {
  Utest* test;
  MockTestOutput* printer;

  void SetUp()
  {
    test = new NullTest();
    printer = new MockTestOutput();
  }
  void TearDown()
  {
    delete test;
    delete printer;
  };
};


TEST(Failure, CreateFailure)
{
  Failure f1(test, failLineNumber, "the failure message");
  Failure f2(test, "the failure message");
  Failure f3(test, failLineNumber);
}

TEST(Failure, CreatePassingEqualsFailure)
{
  EqualsFailure f(test, failLineNumber, "expected", "actual");
}
