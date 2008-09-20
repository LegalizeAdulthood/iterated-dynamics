//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

///////////////////////////////////////////////////////////////////////////////
//
// TESTREGISTRY.H
//
// TestRegistry is a singleton collection of all the tests to run in a system.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TESTREGISTRY_H
#define TESTREGISTRY_H

#include "SimpleString.h"

class Utest;
class TestResult;
class TestOutput;


class TestRegistry
  {
  public:
    TestRegistry();
    static void addTest(Utest *test);
    static void unDoLastAddTest();
    static void runAllTests(TestResult& result, TestOutput*);
    static void verbose();
    static void nameFilter(const char*);
    static void groupFilter(const char*);

  private:

    static TestRegistry& instance();
    void add
      (Utest* test);
    void unDoAdd();
    void run(TestResult& result, TestOutput*);
    bool testShouldRun(Utest* test, TestResult& result);
    void print(Utest* test);

    TestOutput* output;
    Utest *tests;
    bool verbose_;
    const char* nameFilter_;
    const char* groupFilter_;
    int dotCount;

  };

#endif
