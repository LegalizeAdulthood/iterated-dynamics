//
// Copyright (c) 2005 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "TestOutput.h"
#include <stdio.h>
#include "SimpleString.h"

TestOutput::TestOutput()
{}

TestOutput::~TestOutput()
{}

void TestOutput::print(const char* s)
{
  printf(s);
}

void TestOutput::print(long n)
{
  print(StringFrom(n).asCharString());
}

TestOutput& operator<<(TestOutput& p, const char* s)
{
  p.print(s);
  return p;
}

TestOutput& operator<<(TestOutput& p, long int i)
{
  p.print(i);
  return p;
}

