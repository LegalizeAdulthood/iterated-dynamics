//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//


#include "TestResult.h"
#include "Failure.h"
#include "TestOutput.h"
#include <stdio.h>


TestResult::TestResult (TestOutput& p)
    : output(p)
    , testCount(0)
    , runCount(0)
    , checkCount(0)
    , failureCount (0)
    , filteredOutCount(0)
    , ignoredCount(0)
{}


TestResult::~TestResult()
{}


void TestResult::testsStarted ()
{}


void TestResult::addFailure (const Failure& failure)
{
  failure.Print(output);
  failureCount++;
}

void TestResult::countTest()
{
  testCount++;
}

void TestResult::countRun()
{
  runCount++;
}

void TestResult::countCheck()
{
  checkCount++;
}

void TestResult::countFilteredOut()
{
  filteredOutCount++;
}

void TestResult::countIgnored()
{
  ignoredCount++;
}

void TestResult::testsEnded ()
{
  if (failureCount > 0)
    {
      output.print("\nErrors (");
      output.print(failureCount);
      output.print(" failures, ");
    }
  else
    {
      output.print("\nOK (");
    }
  output.print(testCount);
  output.print(" tests, ");
  output.print(runCount);
  output.print(" ran, ");
  output.print(checkCount);
  output.print(" checks, ");
  output.print(ignoredCount);
  output.print(" ignored, ");
  output.print(filteredOutCount);
  output.print(" filtered out)\n\n");
}
