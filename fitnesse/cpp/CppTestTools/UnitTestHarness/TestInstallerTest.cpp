#include "TestHarness.h"
#include "TestInstaller.h"
#include "NullTest.h"

EXPORT_TEST_GROUP(TestInstaller);

// this is file scope because the test is installed
// with all other tests, which also happen to be
// created as static instances at file scope

static NullTest nullTest;

namespace
  {
  TestInstaller* testInstaller;

  void SetUp()
  {
    testInstaller = new TestInstaller(&nullTest);
  }
  void TearDown()
  {
    testInstaller->unDo();
    delete testInstaller;
  }
}

TEST(TestInstaller, Create)
{}

