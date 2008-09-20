//
// Copyright (c) 2005 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "TestHarness.h"

EXPORT_TEST_GROUP(MemoryLeakWarningTest);

namespace
{
  char* arrayToLeak1;
  char* arrayToLeak2;
  long* nonArrayToLeak;
  void SetUp()
  {
  }
  void TearDown()
  {
  }
}

//tests run backwards and this one cares because it has to cleanup after the 
//leaky tests
TEST(MemoryLeakWarningTest, CleanUpLeaks)
{
    IGNORE_N_LEAKS(-3);
    delete [] arrayToLeak1;
    delete [] arrayToLeak2;
    delete nonArrayToLeak;
}

TEST(MemoryLeakWarningTest, Ignore1)
{
    IGNORE_N_LEAKS(1);
    arrayToLeak1 = new char[100];
}

TEST(MemoryLeakWarningTest, Ignore2)
{
    IGNORE_N_LEAKS(2);
    arrayToLeak2 = new char[10];
    nonArrayToLeak = new long;
}

