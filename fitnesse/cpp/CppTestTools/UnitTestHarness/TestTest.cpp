
// Copyright (c) 2003 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.

#include "Platform.h"
#include "TestHarness.h"
#include "MockTestOutput.h"

EXPORT_TEST_GROUP(Utest);

namespace
  {
  void stub()
  {}
  const int testLineNumber = 1;
}

class PassingTestImplementation : public Utest
  {
  public:
    PassingTestImplementation()
        :Utest("Group", "Name", "File", testLineNumber, stub, stub)
    {}
    void testBody(TestResult& result)
    {}
    ;
  };

class FailingTestImplementation : public Utest
  {
  public:
    FailingTestImplementation()
        :Utest("Group", "Name", "File", testLineNumber, stub, stub)
    {}
    void testBody(TestResult& result_)
    {
      FAIL("This test fails");
    };
  };

namespace
  {
  Utest* passingTest;
  Utest* failingTest;
  MockTestOutput* output;

  void SetUp()
  {
    passingTest = new PassingTestImplementation();
    failingTest = new FailingTestImplementation();
    output = new MockTestOutput();
  }
  void TearDown()
  {
    delete passingTest;
    delete failingTest;
    delete output;
  };

  void testHelper()
  {
    CHECK(0 == 0);
    LONGS_EQUAL(1,1);
    CHECK_EQUAL("THIS", "THIS");
    STRCMP_EQUAL("THIS", "THIS");
    DOUBLES_EQUAL(1.0, 1.0, .01);
  }

  void assertPrintContains(MockTestOutput* output, SimpleString& contains)
  {
    if (output->getOutput().contains(contains))
      return;
    SimpleString message("\tActual <");
    message += output->getOutput().asCharString();
    message += ">\n";
    message += "\tdid not contain <";
    message += contains.asCharString();
    message += ">\n";

    FAIL(message.asCharString());

  }
};


TEST(Utest, FailurePrintsSomething)
{
  TestResult result(*output);
  failingTest->run(result);
  SimpleString contains("This test fails");
  assertPrintContains(output, contains);
}

TEST(Utest, SuccessPrintsNothing)
{
  TestResult result(*output);
  passingTest->run(result);
  SimpleString expected = "";
  CHECK_EQUAL(expected, output->getOutput());
}

TEST(Utest, OutOfMacroHelper)
{
  testHelper();
}

TEST(Utest, AssertsActLikeStatements)
{
  if (output != 0)
    CHECK(true)
  else
    CHECK(false)

  if (output != 0)
    CHECK_EQUAL(true, true)
  else
    CHECK_EQUAL(false, false)

  if (output != 0)
    STRCMP_EQUAL("", "")
    else
      STRCMP_EQUAL("", " ")

  if (output != 0)
    LONGS_EQUAL(1, 1)
    else
      LONGS_EQUAL(1, 0)

  if (output != 0)
    DOUBLES_EQUAL(1, 1, 0.01)
    else
      DOUBLES_EQUAL(1, 0, 0.01)

  if (false)
    FAIL("")
    else
      ;

  if (true)
    ;
  else
    FAIL("")

}


IGNORE_TEST(Utest, IgnoreTestSupportsAllMacros)
{
  CHECK(true);
  CHECK_EQUAL(true, true);
  STRCMP_EQUAL("", "");
  LONGS_EQUAL(1, 1);
  DOUBLES_EQUAL(1, 1, 0.01);
  FAIL("");
}



