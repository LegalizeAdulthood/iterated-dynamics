#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <iostream>
#include <crtdbg.h>
#include <windows.h>
#include "UnitTestHarness/MemoryLeakWarning.h"

#ifdef   _DEBUG
#define  SET_CRT_DEBUG_FIELD(a) \
            _CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#else
#define  SET_CRT_DEBUG_FIELD(a)   ((void) 0)
#endif

static int ignoreCount = 0;


void MemoryLeakWarning::Enable()
{
  _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_FILE );
  _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
  _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_FILE );
  _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
  _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_FILE );
  _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
  SET_CRT_DEBUG_FIELD( _CRTDBG_LEAK_CHECK_DF );
}

const char* MemoryLeakWarning::FinalReport()
{
  //windows reports leaks automatically when set up as above
  return "";
}

static _CrtMemState s1, s2, s3;


void MemoryLeakWarning::CheckPointUsage()
{
  ignoreCount = 0;
  _CrtMemCheckpoint( &s1 );
}

static char message[50] = "";
bool MemoryLeakWarning::UsageIsNotBalanced()
{
  _CrtMemCheckpoint( &s2 );
  if (_CrtMemDifference( &s3, &s1, &s2) )
  {
      if (s3.lCounts[1] == ignoreCount)
        return false;
      else
      {
        sprintf(message, "this test leaks %d blocks", s3.lCounts[1]);
        return true;
      }
  }
  else
  {
    return false;
  }
}

const char* MemoryLeakWarning::Message()
{
  return message;
}

void MemoryLeakWarning::IgnoreLeaks(int n)
{
    ignoreCount = n;
}
