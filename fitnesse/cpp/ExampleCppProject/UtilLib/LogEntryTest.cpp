#include "UnitTestHarness/TestHarness.h"
#include "Helpers/SimpleStringExtensions.h"
#include "LogEntry.h"

EXPORT_TEST_GROUP(LogEntry);

namespace
  {
  LogEntry* logEntry;

  void SetUp()
  {
    logEntry = new LogEntry();
  }
  void TearDown()
  {
    delete logEntry;
  }
}

TEST(LogEntry, Create)
{
  LogEntry entries[10];
  STRCMP_EQUAL("", entries[0].GetEvent());
  LONGS_EQUAL(0, entries[0].GetParameter());
}

TEST(LogEntry, SetGet)
{
  logEntry->Set("hello", 1234);
  STRCMP_EQUAL("hello", logEntry->GetEvent());
  LONGS_EQUAL(1234, logEntry->GetParameter());
}

