#include "Helpers/SimpleStringExtensions.h"
#include "UnitTestHarness/TestHarness.h"
#include "ClassName.h"
#include "MockClassName.h"

// to make sure this file gets linked in to your test main
int i = move_the_IMPORT_TEST_GROUP_to_AllTests_dot_cpp;
IMPORT_TEST_GROUP(ClassName);
EXPORT_TEST_GROUP(ClassName);

namespace
  {
  ClassName* aClassName;
  MockClassName* mockClassName;

  void SetUp()
  {
    mockClassName = new MockClassName();
    aClassName = mockClassName;
  }
  void TearDown()
  {
    delete aClassName;
  }
}

TEST(ClassName, Create)
{
  FAIL("Making sure ClassNameTest is hooked in");
}

