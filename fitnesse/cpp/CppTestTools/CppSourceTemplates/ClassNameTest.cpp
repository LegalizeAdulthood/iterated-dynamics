#include "UnitTestHarness/TestHarness.h"
#include "Helpers/SimpleStringExtensions.h"
#include "ClassName.h"

// to make sure this file gets linked in to your test main
int i = move_the_IMPORT_TEST_GROUP_to_AllTests_dot_cpp;
IMPORT_TEST_GROUP(ClassName);
EXPORT_TEST_GROUP(ClassName);

namespace
  {
  ClassName* aClassName;

  void SetUp()
  {
    aClassName = new ClassName();
  }
  void TearDown()
  {
    delete aClassName;
  }
}

TEST(ClassName, Create)
{
  FAIL("Start here");
}

