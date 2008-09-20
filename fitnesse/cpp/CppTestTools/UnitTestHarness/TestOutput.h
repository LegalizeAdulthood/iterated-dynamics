//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//
#ifndef D_TestOutput_h
#define D_TestOutput_h

///////////////////////////////////////////////////////////////////////////////
//
//  TestOutput.h
//
//  This is a minimal printer inteface.
//  We kept streams out too keep footprint small, and so the test
//  harness could be used with less capable compilers so more
//  platforms could use this test harness
//
///////////////////////////////////////////////////////////////////////////////

class TestOutput
  {
  public:
    explicit TestOutput();
    virtual ~TestOutput();

    virtual void print(const char*);
    void print(long int);

  private:

    TestOutput(const TestOutput&);
    TestOutput& operator=(const TestOutput&);

  };

TestOutput& operator<<(TestOutput&, const char*);
TestOutput& operator<<(TestOutput&, long int);

#endif  // D_TestOutput_h
