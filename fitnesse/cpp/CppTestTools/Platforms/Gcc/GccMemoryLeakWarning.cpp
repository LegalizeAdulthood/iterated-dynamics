#include "UnitTestHarness/MemoryLeakWarning.h"

#include <stdlib.h>
#include <stdio.h>

static int allocatedBlocks = 0;
static int initialBlocksUsed = 0;
static int allocatedArrays = 0;
static int initialArraysUsed = 0;

static char message[100] = "";


void reportMemoryBallance()
{
  int blockBalance = allocatedBlocks - initialBlocksUsed;
  int arrayBalance = allocatedArrays - initialArraysUsed;
  if (blockBalance == 0 && arrayBalance == 0)
    ;
  else if (blockBalance + arrayBalance == 0)
    printf("No leaks but some arrays were deleted without []\n");
  else
    {
      if (blockBalance > 0)
        printf("Memory leak! %d blocks not deleted\n", blockBalance);
      if (arrayBalance > 0)
        printf("Memory leak! %d arrays not deleted\n", arrayBalance);
      if (blockBalance < 0)
        printf("More blocks deleted than newed! %d extra deletes\n", blockBalance);
      if (arrayBalance < 0)
        printf("More arrays deleted than newed! %d extra deletes\n", arrayBalance);

      printf("NOTE - some memory leaks appear to be allocated statics that are not released\n"
             "     - by the standard library\n"
             "     - Use the -r switch on your unit tests to repeat the test sequence\n"
             "     - If no leaks are reported on the second pass, it is likely a static\n"
             "     - that is not released\n");
    }
}


void MemoryLeakWarning::Enable()
{
  initialBlocksUsed = allocatedBlocks;
  initialArraysUsed = allocatedArrays;
  atexit(reportMemoryBallance);
}

const char*  MemoryLeakWarning::FinalReport()
{

  if (initialBlocksUsed != allocatedBlocks || initialArraysUsed != allocatedArrays )
    {
      printf("initial blocks=%d, allocated blocks=%d\ninitial arrays=%d, allocated arrays=%d\n",
             initialBlocksUsed, allocatedBlocks, initialArraysUsed, allocatedArrays);

      return "Memory new/delete imbalance after running tests\n";
    }
  else
    return "";
}

static int blockUsageCheckPoint = 0;
static int arrayUsageCheckPoint = 0;
static int ignoreCount = 0;

void MemoryLeakWarning::CheckPointUsage()
{
  blockUsageCheckPoint = allocatedBlocks;
  arrayUsageCheckPoint = allocatedArrays;
}

bool MemoryLeakWarning::UsageIsNotBalanced()
{
  int arrayBalance = allocatedArrays - arrayUsageCheckPoint;
  int blockBalance = allocatedBlocks - blockUsageCheckPoint;

  if (ignoreCount != 0 && blockBalance + arrayBalance == ignoreCount)
    return false;
  if (blockBalance == 0 && arrayBalance == 0)
    return false;
  else if (blockBalance + arrayBalance == 0)
    sprintf(message, "No leaks but some arrays were deleted without []\n");
  else
    {
      int nchars = 0;
      if (blockUsageCheckPoint != allocatedBlocks)
        nchars = sprintf(message, "this test leaks %d blocks",
                         allocatedBlocks - blockUsageCheckPoint);

      if (arrayUsageCheckPoint != allocatedArrays)
        sprintf(message + nchars, "this test leaks %d arrays",
                allocatedArrays - arrayUsageCheckPoint);
    }

  return true;
}

const char* MemoryLeakWarning::Message()
{
  return message;
}

void* operator new(size_t size)
{
  allocatedBlocks++;
  return malloc(size);
}

void operator delete(void* mem)
{
  allocatedBlocks--;
  free(mem);
}

void* operator new[](size_t size)
{
  allocatedArrays++;
  return malloc(size);
}

void operator delete[](void* mem)
{
  allocatedArrays--;
  free(mem);
}

void MemoryLeakWarning::IgnoreLeaks(int n)
{
    ignoreCount = n;
}
