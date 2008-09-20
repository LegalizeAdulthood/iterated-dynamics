//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//


#include "Test.h"
#include "TestResult.h"
#include "TestRegistry.h"
#include "Failure.h"
#include "MemoryLeakWarning.h"
#include "TestOutput.h"


TestRegistry::TestRegistry()
    : tests(0)
    , verbose_(false)
    , nameFilter_("")
    , groupFilter_("")
    , dotCount(0)
{}

void TestRegistry::addTest (Utest *test)
{
  instance ().add (test);
}


void TestRegistry::runAllTests (TestResult& result, TestOutput* output)
{

  instance ().run (result, output);
  result.testsEnded ();
}

TestRegistry& TestRegistry::instance ()
{
  static TestRegistry registry;
  return registry;
}


void TestRegistry::add
  (Utest *test)
  {
    if (tests == 0)
      {
        tests = test;
        return;
      }

    test->setNext (tests);
    tests = test;
  }

void TestRegistry::unDoLastAddTest()
{
  instance().unDoAdd();

}

void TestRegistry::unDoAdd()
{
  if (tests == 0)
    {
      return;
    }
  tests = tests->getNext();
}


void TestRegistry::run (TestResult& result, TestOutput* p)
{
  output = p;

  dotCount = 0;
  result.testsStarted ();

  for (Utest *test = tests; test != 0; test = test->getNext ())
    {
      result.countTest();
      if (testShouldRun(test, result))
        {
          print(test);
          MemoryLeakWarning::CheckPointUsage();
          test->run(result);

          if (MemoryLeakWarning::UsageIsNotBalanced())
            {
              Failure f(test, MemoryLeakWarning::Message());
              result.addFailure(f);
            }
        }
    }
}

void TestRegistry::verbose()
{
  instance().verbose_ = 1;
}

void TestRegistry::nameFilter(const char* f)
{
  instance().nameFilter_ = f;
}

void TestRegistry::groupFilter(const char* f)
{
  instance().groupFilter_ = f;
}

bool TestRegistry::testShouldRun(Utest* test, TestResult& result)
{

  if (test->shouldRun(groupFilter_, nameFilter_) )
    return true;
  else
    {
      result.countFilteredOut();
      return false;
      ;
    }

}


void TestRegistry::print(Utest* test)
{
  if (verbose_)
    {
      output->print(test->getFormattedName().asCharString());
      output->print("\n");
    }
  else
    {
      output->print(".");
      if (++dotCount % 50 == 0)
        output->print("\n");
    }

}
