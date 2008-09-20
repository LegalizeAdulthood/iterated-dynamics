
// Copyright (c) 2006 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.

#include "Platform.h"
#include "UnitTestHarness/TestHarness.h"
#include "Fit/FixtureMaker.h"
#include "Fit/ColumnFixture.h"
#include "Fit/ResolutionException.h"
#include "Helpers/SimpleStringExtensions.h"
#include <memory>

EXPORT_TEST_GROUP(FixtureMaker);

using namespace std;

namespace
  {
  void SetUp()
  {}

  void TearDown()
  {}
}


TEST(FixtureMaker,splitNameDouble)
{
  CHECK_EQUAL("John", FixtureMaker::splitName("John").first);
  CHECK_EQUAL("Smith", FixtureMaker::splitName("John.Smith").second);
}

TEST(FixtureMaker,splitNameSingle)
{
  CHECK_EQUAL("John", FixtureMaker::splitName("John").first);
  CHECK_EQUAL("", FixtureMaker::splitName("John").second);
}

