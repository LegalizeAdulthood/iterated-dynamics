//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

///////////////////////////////////////////////////////////////////////////////
//
// TESTRESULT.H
//
// A TestResult is a collection of the history of some test runs.  Right now
// it just collects failures.  Really it just prints the failures.
//
///////////////////////////////////////////////////////////////////////////////



#ifndef TESTRESULT_H
#define TESTRESULT_H

class Failure;
class TestOutput;

class TestResult
  {
  public:
    TestResult (TestOutput&);
    virtual ~TestResult();
    virtual void testsStarted ();
    virtual void countTest();
    virtual void countRun();
    virtual void countCheck();
    virtual void countFilteredOut();
    virtual void countIgnored();
    virtual void addFailure (const Failure& failure);
    virtual void testsEnded ();
    int getFailureCount() {return failureCount;}

  private:

    TestOutput& output;
    int testCount;
    int runCount;
    int checkCount;
    int	failureCount;
    int filteredOutCount;
    int ignoredCount;
  };

#endif
