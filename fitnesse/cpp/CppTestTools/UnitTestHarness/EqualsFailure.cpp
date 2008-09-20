//
// Copyright (c) 2005 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "EqualsFailure.h"
#include "TestOutput.h"

#include <string.h> // for strlen()
#include <stdio.h>

EqualsFailure::EqualsFailure(Utest* test, long lineNumber,
                             const SimpleString& expected,
                             const SimpleString& actual)
    : Failure(test, lineNumber)
{
  char *format = "\texpected <%s>\n\tbut was  <%s>";

  char *stage = new char [strlen(format) - (2 * strlen("%s"))
                          + expected.size ()
                          + actual.size ()
                          + 1];

  sprintf(stage, format,
          expected.asCharString(),
          actual.asCharString());

  message = SimpleString(stage);

  delete [] stage;
}



void EqualsFailure::PrintSpecifics(TestOutput& p)const
  {
    p << message.asCharString();
  }
