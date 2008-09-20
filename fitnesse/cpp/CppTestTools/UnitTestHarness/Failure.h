//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//


///////////////////////////////////////////////////////////////////////////////
//
// FAILURE.H
//
// Failure is a class which holds information for a specific
// test failure. It can be overriden for more complex failure messages
//
///////////////////////////////////////////////////////////////////////////////


#ifndef FAILURE_H
#define FAILURE_H

#include "SimpleString.h"

class Utest;
class TestOutput;

class Failure
  {

  public:
    Failure(Utest*, long lineNumber, const SimpleString& theMessage);
    Failure(Utest*, const SimpleString& theMessage);
    Failure(Utest*, long lineNumber);
    virtual ~Failure();

    virtual void Print(TestOutput&) const;

  protected:
    virtual void PrintLeader(TestOutput&) const;
    virtual void PrintSpecifics(TestOutput&) const;
    virtual void PrintTrailer(TestOutput&) const;

  private:
    SimpleString testName;
    SimpleString fileName;
    long lineNumber;
    SimpleString message;

    Failure(const Failure&);
    Failure& operator=(const Failure&);

  };


#endif
