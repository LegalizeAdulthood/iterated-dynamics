
// Copyright (c) 2003 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.

#include "Platform.h"

#include "UnitTestHarness/TestHarness.h"
#include "Fixture.h"
#include "TestExtras.h"
#include "Helpers/SimpleStringExtensions.h"

EXPORT_TEST_GROUP(Fixture);


namespace
  {
  void SetUp()
  {}
  ;
  void TearDown()
  {}
  ;
}

TEST(Fixture,endsWith)
{
  Fixture f;

  CHECK(!f.endsWith("abc","d"));
  CHECK(f.endsWith("abc","c"));
  CHECK(f.endsWith("abc","abc"));
  CHECK(f.endsWith("abc","bc"));
  CHECK(!f.endsWith("abc","ab"));
}


TEST(Fixture, escape)
{
  Fixture f;

  CHECK_EQUAL("a&amp;b", f.escape("a<b",'<',"&amp;"));
  CHECK_EQUAL("&amp;b", f.escape("<b",'<',"&amp;"));
  CHECK_EQUAL("<b", f.escape("<b",'>',"&amp;"));
}

