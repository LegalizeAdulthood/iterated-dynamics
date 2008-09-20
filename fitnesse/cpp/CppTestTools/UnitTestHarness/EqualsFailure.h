//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//


///////////////////////////////////////////////////////////////////////////////
//
// EqualsFailure.H
//
// EqualsFailure is a class which holds information for a specific
// equality test failure.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EQUALS_FAILURE_H
#define EQUALS_FAILURE_H

#include "SimpleString.h"
#include "Failure.h"

class TestOutput;


class EqualsFailure : public Failure
  {

  public:

    EqualsFailure(Utest*, long lineNumber,
                  const SimpleString& expected,
                  const SimpleString& actual);

  protected:
    virtual void PrintSpecifics(TestOutput&) const;

  private:
    SimpleString message;

    EqualsFailure(const EqualsFailure&);
    EqualsFailure& operator=(const EqualsFailure&);

  };


#endif
