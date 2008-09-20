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

static ClassName* aClassName;

static void SetUp()
{
  aClassName = ClassName_Create(42);
}

static void TearDown()
{
  aClassName = send(aClassName, Destroy);
}

TEST(ClassName, Create)
{
  FAIL("Start here");
}

static int SomeFunctionOverride(ClassName* p, int v)
{
  return v;
}

TEST(ClassName, OverrideExample)
{
  LONGS_EQUAL(42, send1(aClassName, SomeFunction, 41));

  vBind(aClassName, SomeFunction, SomeFunctionOverride);
  LONGS_EQUAL(41, send1(aClassName, SomeFunction, 41));
}

