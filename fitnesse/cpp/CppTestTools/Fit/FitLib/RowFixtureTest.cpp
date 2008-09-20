
// Copyright (c) 2003 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.

#include "Platform.h"

#include <set>

#include "UnitTestHarness/TestHarness.h"
#include "RowFixture.h"
#include "Parse.h"
#include "DomainObjectWrapper.h"
#include "TestFixtureMaker.h"
#include "TestAccount.h"
#include "TestRowFixture.h"

EXPORT_TEST_GROUP(RowFixture)


class RowTestFixtureMaker : public TestFixtureMaker
  {
    virtual Fixture *make(const string& name)
    {
      if (name == "TestRowFixture")
        return new TestRowFixture;
      return TestFixtureMaker::make(name);
    }

  };


namespace
  {
  void SetUp()
  {}
  void TearDown()
  {}
  ;
};

TEST(RowFixture, listOf)
{
  Parse p("<table><tr><td>one</td></tr><tr><td>two</td></tr></table>");

  RowFixture::ParseList  rows = RowFixture::listOf(p.parts);

  LONGS_EQUAL(2, rows.size());
}




TEST(RowFixture,  binObjects)
{
  RowMap<RowFixture::Object>  theMap;

  string key = "a";

  Fixture *f = new Fixture;
  Fixture *f2 = new Fixture;
  theMap.bin(key, f);

  CHECK(theMap.getRow(key) != 0);
  CHECK((*theMap.getRow(key))[0] == f);

  theMap.bin(key, f2);
  CHECK((*theMap.getRow(key))[1] == f2);

  delete f;
  delete f2;
}


TEST(RowFixture, binParses)
{
  RowMap<Parse> theMap;
  string key = "a";

  Parse *p = new Parse("", "", 0, 0);
  Parse *p2 = new Parse("", "", 0, 0);
  theMap.bin(key, p);

  CHECK(theMap.getRow(key) != 0);
  CHECK((*theMap.getRow(key))[0] == p);

  theMap.bin(key, p2);
  CHECK((*theMap.getRow(key))[1] == p2);


  delete p;
  delete p2;
}



TEST(RowFixture, unionOfKeys)
{
  RowMap<Parse>					parses;
  RowMap<RowFixture::Object>		objects;
  set
    <string>						keys;

  auto_ptr<Parse>  p1(new Parse("","",0,0));
  auto_ptr<Parse>  p2(new Parse("","",0,0));

  auto_ptr<RowFixture::Object> o1;
  auto_ptr<RowFixture::Object> o2;

  keys = RowFixture::unionOfKeys(parses,objects);
  LONGS_EQUAL(0, keys.size());

  parses.bin("a",p1.get());
  keys = RowFixture::unionOfKeys(parses, objects);
  LONGS_EQUAL(1, keys.size());

  parses.bin("a",p2.get());
  keys = RowFixture::unionOfKeys(parses, objects);
  LONGS_EQUAL(1, keys.size());

  objects.bin("b",o1.get());
  keys = RowFixture::unionOfKeys(parses, objects);
  LONGS_EQUAL(2, keys.size());

  objects.bin("a",o2.get());
  keys = RowFixture::unionOfKeys(parses, objects);
  LONGS_EQUAL(2, keys.size());

}


TEST(RowFixture, addAllParses)
{
  Parse p("<table><tr><td>one</td></tr><tr><td>two</td></tr></table>");
  RowFixture::ParseList parses = RowFixture::listOf(p.parts.safeGet());
  RowFixture::ParseList container;

  LONGS_EQUAL(2, parses.size());
  addAll<Parse>(container,&parses);
  LONGS_EQUAL(2, container.size());

}


TEST(RowFixture, addAllObjects)
{
  RowFixture::ObjectList objects;
  RowFixture::ObjectList container;

  Fixture f, f2;
  objects.push_back(&f);
  objects.push_back(&f2);

  LONGS_EQUAL(2, objects.size());
  addAll<RowFixture::Object>(container,&objects);
  LONGS_EQUAL(2, container.size());
}



TEST(RowFixture, a)
{
  TestRowFixture f;

  const Fixture *d = f.getTargetClass();
  d->hasAdapterFor("field");

  CHECK(d->hasAdapterFor("field"));
}



TEST(RowFixture, simpleRows)
{
  Parse p(
    "<table><TR><TD>TestRowFixture</TD></TR>"
    "<TR><TD>field</TD></TR>"
    "<TR><TD>0</TD></TD></TR>"
    "<TR><TD>3</TD></TD></TR>"
    "</table>");

  Fixture fixture;
  RowTestFixtureMaker maker;

  fixture.setMaker(&maker);
  fixture.doTables(&p);

  string out;

  p.print(out);
  CHECK(string::npos != out.find(Fixture::rightColor));

}



