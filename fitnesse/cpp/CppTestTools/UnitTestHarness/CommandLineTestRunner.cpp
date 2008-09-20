//
// Copyright (c) 200 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//


#include "CommandLineTestRunner.h"
#include "MemoryLeakWarning.h"
#include "TestOutput.h"
#include "RealTestOutput.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int CommandLineTestRunner::repeat = 0;
TestOutput* CommandLineTestRunner::output;

int CommandLineTestRunner::RunAllTests(int ac, char** av)
{
  MemoryLeakWarning::Enable();
  RealTestOutput realTestOutput;
  output = &realTestOutput;

  SetOptions(ac, av);
  bool repeating = (repeat > 1);
  int loopCount = 1;
  int failureCount = 0;

  do
    {
      if (repeating)
        {
          output->print("Test run ");
          output->print(loopCount);
          output->print("\n");
        }

      TestResult tr(*output);
      TestRegistry::runAllTests(tr, output);
      failureCount = tr.getFailureCount();
    }
  while (++loopCount <= repeat);

  realTestOutput << MemoryLeakWarning::FinalReport();
  
  return failureCount;
}

void CommandLineTestRunner::SetOptions(int ac, char** av)
{
  repeat = 1;
  for (int i = 1; i < ac; i++)
    {
      if (0 == strcmp("-v", av[i]))
        TestRegistry::verbose();
      else if (av[i] == strstr(av[i], "-r"))
        SetRepeatCount(ac, av, i);
      else if (av[i] == strstr(av[i], "-g"))
        SetGroupFilter(ac, av, i);
      else if (av[i] == strstr(av[i], "-n"))
        SetNameFilter(ac, av, i);
      else
        {
          output->print("usage [-v] [-r#] [-g groupName] [-n testName]\n");
        }
    }
}


void CommandLineTestRunner::SetRepeatCount(int ac, char** av, int& i)
{
  repeat = 0;

  if (strlen(av[i]) > 2)
    repeat = atoi(av[i] + 2);
  else if (i + 1 < ac)
    {
      repeat = atoi(av[i+1]);
      if (repeat != 0)
        i++;
    }

  if (0 == repeat)
    repeat = 2;

}

void CommandLineTestRunner::SetGroupFilter(int ac, char** av, int& i)
{
  const char* groupFilter;

  if (strlen(av[i]) > 2)
    groupFilter = av[i] + 2;
  else if (i + 1 < ac)
    {
      groupFilter = av[++i];
    }
  TestRegistry::groupFilter(groupFilter);

}

void CommandLineTestRunner::SetNameFilter(int ac, char** av, int& i)
{
  const char* nameFilter;

  if (strlen(av[i]) > 2)
    nameFilter = av[i] + 2;
  else if (i + 1 < ac)
    {
      nameFilter = av[++i];
    }
  TestRegistry::nameFilter(nameFilter);

}

