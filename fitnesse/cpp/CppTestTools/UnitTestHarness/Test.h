//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

///////////////////////////////////////////////////////////////////////////////
//
// TEST.H
//
// This file contains the Test class along with the macros which make effective
// in the harness.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TEST_H
#define TEST_H


#include <math.h>
#include "SimpleString.h"
#include "TestInstaller.h"

class TestResult;

class Utest
  {
  public:

    Utest(const char* groupName,
         const char* testName,
         const char* fileName,
         int lineNumber,
         void (*setUp)(),
         void (*tearDown)());

    virtual ~Utest();

    virtual void testBody(TestResult& result) = 0;

    virtual void run (TestResult& result);
    virtual SimpleString getFormattedName() const;

    void setNext(Utest *test);
    Utest *getNext () const;
    const SimpleString getName() const;
    bool shouldRun(const SimpleString& groupFilter, const SimpleString& nameFilter) const;
    const SimpleString getFile() const;
    int getLineNumber() const;
    void setUp();
    void tearDown();

    static TestResult* getTestResult()
    {
      return testResult_;
    }

    static Utest* getCurrent()
    {
      return currentTest_;
    }

    virtual bool assertTrue(bool condition, const char* conditionString, int lineNumber);
    virtual bool assertCstrEqual(const char* expected, const char* actual, int lineNumber);
    virtual bool assertLongsEqual(long expected, long actual, int lineNumber);
    virtual bool assertDoublesEqual(double expected, double actual, double threshold, int lineNumber);
    virtual void fail(const char* text, int lineNumber);



  protected:

    virtual SimpleString getMacroName() const
      {
        return "TEST";
      }

  private:

    const char* group_;
    const char* name_;
    const char* file_;
    int	lineNumber_;
    Utest *next_;
    void (*setUp_)();
    void (*tearDown_)();
    static TestResult* testResult_;
    static Utest* currentTest_;

  };

class IgnoredTest : public Utest
  {
  public:
    IgnoredTest(const char* groupName,
                const char* testName,
                const char* fileName,
                int lineNumber);

    virtual ~IgnoredTest();
    virtual void testBody(TestResult& result);
    virtual void run (TestResult& result);

  protected:
    virtual SimpleString getMacroName() const
      {
        return "IGNORE_TEST";
      }

  };



#define TEST(testGroup, testName) \
  class testGroup##testName##Test : public Utest \
{ public: testGroup##testName##Test () : Utest (#testGroup, #testName, __FILE__,__LINE__, &SetUp, &TearDown) {} \
            void testBody(TestResult& result_); } \
    testGroup##testName##Instance; \
  TestInstaller testGroup##testName##Installer(&testGroup##testName##Instance); \
	void testGroup##testName##Test::testBody(TestResult& result_)

#define IGNORE_TEST(testGroup, testName)\
  class testGroup##testName##Test : public IgnoredTest \
{ public: testGroup##testName##Test () : IgnoredTest (#testGroup, #testName, __FILE__,__LINE__) {} \
            void thisNeverRuns (TestResult& result_); } \
    testGroup##testName##Instance; \
  TestInstaller testGroup##testName##Installer(&testGroup##testName##Instance); \
	void testGroup##testName##Test::thisNeverRuns (TestResult& result_)

#define EXPORT_TEST_GROUP(testGroup)\
  int externTestGroup##testGroup = 0;

#define IMPORT_TEST_GROUP(testGroup) \
  extern int externTestGroup##testGroup;\
  int* p##testGroup = &externTestGroup##testGroup;

//Check any boolean condition
#define CHECK(condition)\
  {if (!Utest::getCurrent()->assertTrue(condition, #condition, __LINE__)) return;}



//This check needs the equality operator, and a StringFrom(YourType) function
#define CHECK_EQUAL(expected,actual)\
  if ((expected) != (actual))\
  {\
	 Utest::getTestResult()->countCheck();\
  	 EqualsFailure _f(Utest::getCurrent(), __LINE__, StringFrom(expected), StringFrom(actual)); \
     Utest::getTestResult()->addFailure(_f);\
     return;\
  }\
  else\
	 Utest::getTestResult()->countCheck();

//This check checks for char* string equality using strcmp.
//This makes up for the fact that CHECK_EQUAL only compares the pointers to char*'s
#define STRCMP_EQUAL(expected,actual)\
  {if (!Utest::getCurrent()->assertCstrEqual(expected, actual, __LINE__)) return;}

//Check two long integers for equality
#define LONGS_EQUAL(expected,actual)\
  { if (!Utest::getCurrent()->assertLongsEqual(expected, actual, __LINE__)) return; }

//Check two doubles for equality within a tolerance threshold
#define DOUBLES_EQUAL(expected,actual,threshold)\
  { if (!Utest::getCurrent()->assertDoublesEqual(expected, actual, threshold, __LINE__)) return; }

//Fail if you get to this macro
#define FAIL(text)\
  { Utest::getCurrent()->fail(text, __LINE__); return; }

#endif
