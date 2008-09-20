
// Copyright (c) 2003 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.

#include "Platform.h"
#include "UnitTestHarness/TestHarness.h"
#include "Parse.h"
#include "Fixture.h"
#include "TestFixtureMaker.h"



EXPORT_TEST_GROUP(Summary)

namespace
  {
  void SetUp()
  {}
  void TearDown()
  {}
  ;
};

TEST(Summary, summary)
{

  Parse p(
    "<table>"
    "<TR><TD>Summary</TD></TR>"
    "</table>");

  Fixture fixture;
  TestFixtureMaker maker;

  fixture.setMaker(&maker);
  fixture.doTables(&p);

  string out;

  p.print(out);
  CHECK(string::npos != out.find("<td>counts</td>"));
  CHECK(string::npos != out.find("<td>run date</td>"));
}

