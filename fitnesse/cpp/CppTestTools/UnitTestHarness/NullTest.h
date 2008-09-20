//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//
#ifndef D_NullTest_h
#define D_NullTest_h

#include "Test.h"
#include "TestResult.h"

///////////////////////////////////////////////////////////////////////////////
//
//  NullTest.h
//
//  This class provides a Test class for testing
//
///////////////////////////////////////////////////////////////////////////////

class NullTest : public Utest
  {
  public:
    explicit NullTest();
    virtual ~NullTest();

    void testBody(TestResult&)
    {}

  private:

    NullTest(const NullTest&);
    NullTest& operator=(const NullTest&);

  };

#endif  // D_NullTest_h
