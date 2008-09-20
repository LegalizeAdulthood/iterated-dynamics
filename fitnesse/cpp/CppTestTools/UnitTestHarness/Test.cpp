//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//


#include "Test.h"
#include "TestRegistry.h"
#include "TestResult.h"
#include "Failure.h"
#include "EqualsFailure.h"

#include <string.h>
#include <stdio.h>

namespace
  {
  void doNothing()
  {}
}

TestResult* Utest::testResult_ = 0;
Utest* Utest::currentTest_ = 0;

Utest::Utest (const char* groupName,
            const char* testName,
            const char* fileName,
            int lineNumber,
            void (*setUp)(),
            void (*tearDown)())
    : group_(groupName)
    , name_(testName)
    , file_(fileName)
    , lineNumber_(lineNumber)
    , setUp_(setUp)
    , tearDown_(tearDown)
{}

Utest::~Utest ()
{}

IgnoredTest::IgnoredTest(
  const char* groupName,
  const char* testName,
  const char* fileName,
  int lineNumber)
    : Utest(groupName, testName, fileName, lineNumber, doNothing, doNothing)
{}

IgnoredTest::~IgnoredTest ()
{}

void Utest::run(TestResult& result)
{
  //save test context, so that test class can be tested
  Utest* savedTest = currentTest_;
  TestResult* savedResult = testResult_;

  result.countRun();
  setUp();
  testResult_ = &result;
  currentTest_ = this;
  testBody(result);
  tearDown();

  //restore
  currentTest_ = savedTest;
  testResult_ = savedResult;
}

void IgnoredTest::run(TestResult& result)
{
  result.countIgnored();
}

void IgnoredTest::testBody(TestResult& result)
{}


Utest *Utest::getNext() const
  {
    return next_;
  }


void Utest::setNext(Utest *test)
{
  next_ = test;
}

const SimpleString Utest::getName() const
  {
    return SimpleString(name_);
  }

SimpleString Utest::getFormattedName() const
  {
    SimpleString formattedName(getMacroName());
    formattedName += "(";
    formattedName += group_;
    formattedName += ", ";
    formattedName += name_;
    formattedName += ")";

    return formattedName;
  }

const SimpleString Utest::getFile() const
  {
    return SimpleString(file_);
  }


int Utest::getLineNumber() const
  {
    return lineNumber_;
  }

void Utest::setUp()
{
  (*setUp_)();
}

void Utest::tearDown()
{
  (*tearDown_)();
}

bool Utest::shouldRun(const SimpleString& groupFilter, const SimpleString& nameFilter) const
  {
    SimpleString group(group_);
    SimpleString name(name_);
    if (group.contains(groupFilter) && name.contains(nameFilter))
      return true;

    return false;
  }

bool Utest::assertTrue(bool condition, const char* conditionString, int lineNumber)
{
  testResult_->countCheck();
  if (!(condition))
    {
      SimpleString message("CHECK(");
      message += conditionString;
      message += ") failed";
      Failure _f(this, lineNumber, message);
      testResult_->addFailure (_f);
      return false;
    }
  return true;
}

bool Utest::assertCstrEqual(const char* expected, const char* actual, int lineNumber)
{
  testResult_->countCheck();
  if (strcmp(expected, actual) != 0)
    {
      EqualsFailure _f(this, lineNumber, StringFrom(expected), StringFrom(actual));
      testResult_->addFailure (_f);
      return false;
    }
  return true;
}

bool Utest::assertLongsEqual(long expected, long actual, int lineNumber)
{
  testResult_->countCheck();
  if (expected != actual)
    {
      EqualsFailure _f(this, lineNumber, StringFrom(expected), StringFrom(actual));
      testResult_->addFailure (_f);
      return false;
    }
  return true;
}

bool Utest::assertDoublesEqual(double expected, double actual, double threshold, int lineNumber)
{
  testResult_->countCheck();
  if (fabs(expected-actual) > threshold)
    {
      EqualsFailure _f(this, lineNumber, StringFrom(expected), StringFrom(actual));
      testResult_->addFailure (_f);
      return false;
    }
  return true;
}

void Utest::fail(const char *text, int lineNumber)
{
  Failure _f(this, lineNumber, text);
  testResult_->addFailure (_f);
}


