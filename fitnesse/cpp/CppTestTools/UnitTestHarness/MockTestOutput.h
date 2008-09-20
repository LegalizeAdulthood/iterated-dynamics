//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_MockTestOutput_h
#define D_MockTestOutput_h

#include "TestOutput.h"
#include "SimpleString.h"

///////////////////////////////////////////////////////////////////////////////
//
//  MockTestOutput.h
//
//  TestOutput for test purposes
//
///////////////////////////////////////////////////////////////////////////////


class MockTestOutput : public TestOutput
  {
  public:
    explicit MockTestOutput()
    {}
    ;
    virtual ~MockTestOutput()
    {}
    ;

    void print(const char* s)
    {
      output += s;
    }

    const SimpleString& getOutput()
    {
      return output;
    }

  private:
    SimpleString output;

    MockTestOutput(const MockTestOutput&);
    MockTestOutput& operator=(const MockTestOutput&);

  };

#endif  // D_MockTestOutput_h
