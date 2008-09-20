#include "TestInstaller.h"
#include "Test.h"
#include "TestRegistry.h"

TestInstaller::TestInstaller(Utest* t)
{
  TestRegistry::addTest(t);
}

TestInstaller::~TestInstaller()
{}

void TestInstaller::unDo()
{
  TestRegistry::unDoLastAddTest();
}
