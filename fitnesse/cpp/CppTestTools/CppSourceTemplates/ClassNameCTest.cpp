#include "Helpers/SimpleStringExtensions.h"
#include "UnitTestHarness/TestHarness.h"

extern "C"
  {
#include "ClassName.h"
  }

// to make sure this file gets linked in
int i = move_the_IMPORT_TEST_GROUP_to_AllTests_dot_cpp;
IMPORT_TEST_GROUP(ClassName);
EXPORT_TEST_GROUP(ClassName);

static struct ClassName* aClassName;

static void SetUp()
{
  aClassName = ClassName_create();
}
static void TearDown()
{
  ClassName_destroy(aClassName);
}


TEST(ClassName, Create)
{
  FAIL("Start here");
}

