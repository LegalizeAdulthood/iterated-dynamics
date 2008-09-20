//
// Copyright (c) 200 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Failure.h"
#include "Test.h"
#include "TestOutput.h"


Failure::Failure(Utest* test, long lineNumber, const SimpleString& theMessage)
    : testName (test->getFormattedName())
    , fileName (test->getFile())
    , lineNumber (lineNumber)
    , message (theMessage)
{}


Failure::Failure(Utest* test, const SimpleString& theMessage)
    : testName (test->getFormattedName())
    , fileName (test->getFile())
    , lineNumber (test->getLineNumber())
    , message (theMessage)
{}

Failure::Failure(Utest* test, long lineNum)
    : testName (test->getFormattedName())
    , fileName (test->getFile())
    , lineNumber (lineNum)
    , message("no message")
{}

Failure::~Failure()
{}


void Failure::PrintLeader(TestOutput& p)const
  {
    p << "\nFailure in " << testName.asCharString()
    << "\n    "  << fileName.asCharString()
    << "(" <<lineNumber <<")\n";
  }

void Failure::Print(TestOutput& p)const
  {
    PrintLeader(p);
    PrintSpecifics(p);
    PrintTrailer(p);
  }

void Failure::PrintSpecifics(TestOutput& p)const
  {
    p << "    " << message.asCharString();
  }

void Failure::PrintTrailer(TestOutput& p)const
  {
    p << "\n\n";
  }


