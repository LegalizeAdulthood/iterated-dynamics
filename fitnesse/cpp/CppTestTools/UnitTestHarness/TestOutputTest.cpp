//
// Copyright (c) 2005 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "TestHarness.h"
#include "TestOutput.h"
#include "MockTestOutput.h"

EXPORT_TEST_GROUP(TestOutput);

namespace
  {
  TestOutput* printer;
  MockTestOutput* mock;

  void SetUp()
  {
    mock = new MockTestOutput();
    printer = mock;
  }
  void TearDown()
  {
    delete printer;
  }
}

TEST(TestOutput, PrintConstCharStar)
{
  printer->print("hello");
  printer->print("hello\n");
  STRCMP_EQUAL("hellohello\n", mock->getOutput().asCharString());
}

TEST(TestOutput, PrintLong)
{
  printer->print(1234);
  STRCMP_EQUAL("1234", mock->getOutput().asCharString());
}

TEST(TestOutput, StreamOperators)
{
  *printer << "n=" << 1234;
  STRCMP_EQUAL("n=1234", mock->getOutput().asCharString());
}

