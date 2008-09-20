//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_RealTestOutput_h
#define D_RealTestOutput_h

#include "TestOutput.h"
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
//
//  RealTestOutput.h
//
//  Printf Based Solution
//
///////////////////////////////////////////////////////////////////////////////

class RealTestOutput : public TestOutput
  {
  public:
    explicit RealTestOutput()
    {}
    ;
    virtual ~RealTestOutput()
    {}
    ;

    void print(const char* s)
    {
      while(*s)
        {
          if ('\n' == *s)
            putchar('\r');
          putchar(*s);
          s++;
        }
    }

  private:

    RealTestOutput(const RealTestOutput&);
    RealTestOutput& operator=(const RealTestOutput&);

  };

#endif  // D_RealTestOutput_h
