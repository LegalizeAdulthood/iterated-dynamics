
// Copyright (c) 2003 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.

#include "Platform.h"
#include "UnitTestHarness/TestHarness.h"
#include "Parse.h"
#include "ColumnFixture.h"
#include "TypeAdapter.h"
#include "ParseException.h"
#include "TestFixture.h"
#include "TestExtras.h"
#include "Helpers/SimpleStringExtensions.h"

EXPORT_TEST_GROUP(Framework);


namespace
  {
  void SetUp()
  {}
  void TearDown()
  {}
  ;
};


TEST(Framework, parsing)
{
  vector<string> tags;

  tags.push_back("table");
  Parse p("leader<Table foo=2>body</table>trailer", tags);

  StringFrom(string("hey there"));

  CHECK_EQUAL("leader", p.leader);
  CHECK_EQUAL("<Table foo=2>", p.tag);
  CHECK_EQUAL("trailer", p.trailer);
}


TEST(Framework, recursing)
{
  Parse p("leader<table><TR><Td>body</tD></TR></table>trailer");

  CHECK_EQUAL("", p.body);
  CHECK_EQUAL("", p.parts->body);
  CHECK_EQUAL("body", p.parts->parts->body);
}



TEST(FrameWork, iterating)
{
  Parse p("leader<table><tr><td>one</td><td>two</td><td>three</td></tr></table>trailer");

  CHECK_EQUAL("one", p.parts->parts->body);
  CHECK_EQUAL("two", p.parts->parts->more->body);
  CHECK_EQUAL("three", p.parts->parts->more->more->body);
}


TEST(Framework, indexing)
{
  Parse p("leader<table><tr><td>one</td><td>two</td><td>three</td></tr><tr><td>four</td></tr></table>trailer");
  CHECK_EQUAL("one", p.at(0,0,0)->body);
  CHECK_EQUAL("two", p.at(0,0,1)->body);
  CHECK_EQUAL("three", p.at(0,0,2)->body);
  CHECK_EQUAL("three", p.at(0,0,3)->body);
  CHECK_EQUAL("three", p.at(0,0,4)->body);
  CHECK_EQUAL("four", p.at(0,1,0)->body);
  CHECK_EQUAL("four", p.at(0,1,1)->body);
  CHECK_EQUAL("four", p.at(0,2,0)->body);
  LONGS_EQUAL(1, p.size());
  LONGS_EQUAL(2, p.parts->size());
  LONGS_EQUAL(3, p.parts->parts->size());
  CHECK_EQUAL("one", p.leaf()->body);
  CHECK_EQUAL("four", p.parts->last()->leaf()->body);
}



TEST(Framework, parseException )
{
  try
    {
      Parse p("leader<table><tr><th>one</th><th>two</th><th>three</th></tr><tr><td>four</td></tr></table>trailer");
    }
  catch (ParseException& e)
    {
      LONGS_EQUAL(17, e.getErrorOffset());
      CHECK_EQUAL("Can't find tag: td", string(e.what()));
      return;
    }
  CHECK(!"expected exception not thrown");
}


TEST(Framework, text)
{
  vector<string> tags;
  tags.push_back("td");

  {
    Parse p("<td>a&lt;b</td>", tags);

    CHECK_EQUAL("a&lt;b", p.body);
    CHECK_EQUAL("a<b", p.text());
  }

  {
    Parse p("<td>\ta&gt;b&nbsp;&amp;&nbsp;b>c &&&nbsp;</td>", tags);

    CHECK_EQUAL("a>b & b>c &&", p.text());
  }

  {
    Parse p("<TD><P><FONT FACE=\"Arial\" SIZE=2>GroupTestFixture</FONT></TD>", tags);

    CHECK_EQUAL("GroupTestFixture", p.text());

  }
}


TEST(Framework, unescape)
{
  CHECK_EQUAL("a<b", Parse::unescape("a&lt;b"));
  CHECK_EQUAL("a&lt", Parse::unescape("a&lt"));
  CHECK_EQUAL("alt;b", Parse::unescape("alt;b"));
  CHECK_EQUAL("> ",Parse::unescape("&gt;&nbsp;"));
  CHECK_EQUAL("a>b & b>c &&", Parse::unescape("a&gt;b&nbsp;&amp;&nbsp;b>c &&"));
}


TEST(Framework, unformat)
{
  CHECK_EQUAL("ab",Parse::unformat("<font size=+1>a</font>b"));
  CHECK_EQUAL("ab",Parse::unformat("a<font size=+1>b</font>"));
  CHECK_EQUAL("a<b",Parse::unformat("a<b"));
}


TEST(Framework, intAdapter)
{
  TestFixture			   *f = new TestFixture;
  TypeAdapter			   *a = TypeAdapter::on(f, "sampleInt");
  a->set
  ("123456");

  LONGS_EQUAL(123456, f->sampleInt);
  delete f;
}



TEST(Framework, methodAdapter)
{
  TestFixture			   *f = new TestFixture;
  TypeAdapter			   *a = TypeAdapter::on(f, "sum");
  a->invoke();

  CHECK_EQUAL("7", a->valueAsString());
  //	LONGS_EQUAL(1, f->invokeCount);
  delete f;
}



TEST(Framework, parseAddToTag)
{
  vector<string> tags;

  tags.push_back("table");
  auto_ptr<Parse> p(new Parse("<table></table>", tags));

  p->addToTag(" piece");
  CHECK_EQUAL("<table piece>", p->tag);

}


TEST(Framework, badAccess)
{
  Parse p(
    "<table><TR><TD>TestFixture</TD></TR>"
    "<TR><TD>arg1</TD><TD>arg2</TD><TD>sum</TD></TR>"
    "<TR><TD>1</TD><TD>5</TD><TD>6</TD></TR>"
    "</table>");

  try
    {
      p.more->more->more;
    }
  catch(exception)
    {
      return;
    }
  CHECK(!"access should have failed");

}



TEST(Framework, simpleRightColumn)
{
  Parse p(
    "<table><TR><TD>TestFixture</TD></TR>"
    "<TR><TD>arg1</TD><TD>arg2</TD><TD>sum</TD></TR>"
    "<TR><TD>1</TD><TD>5</TD><TD>6</TD></TR>"
    "</table>");

  Fixture fixture;
  TestFixtureMaker maker;

  fixture.setMaker(&maker);
  fixture.doTables(&p);

  string out;

  p.print(out);
  CHECK(string::npos != out.find(Fixture::rightColor));
}


TEST(Framework, simpleWrongColumn)
{
  Parse p(
    "<table><TR><TD>TestFixture</TD></TR>"
    "<TR><TD>arg1</TD><TD>arg2</TD><TD>sum</TD></TR>"
    "<TR><TD>3</TD><TD>4</TD><TD>6</TD></TR>"
    "</table>");

  Fixture fixture;
  TestFixtureMaker maker;

  fixture.setMaker(&maker);
  fixture.doTables(&p);

  string out;

  p.print(out);
  CHECK(string::npos != out.find(Fixture::wrongColor));
}

TEST(Framework, stringWithSPaces)
{
  Parse p(
    "<table><TR><TD>TestFixture</TD></TR>"
    "<TR><TD>stringInput</TD><TD>stringOutput()</TD></TR>"
    "<TR><TD>String with spaces</TD><TD>String with spaces</TD></TR>"
    "</table>");

  Fixture fixture;
  TestFixtureMaker maker;

  fixture.setMaker(&maker);
  fixture.doTables(&p);

  string out;

  p.print(out);
  CHECK(string::npos != out.find(Fixture::rightColor));
}

TEST(Framework, queryOperator)
{
  Parse p(
    "<table><TR><TD>TestFixture</TD></TR>"
    "<TR><TD>stringInput</TD><TD>stringOutput?</TD></TR>"
    "<TR><TD>Some arbitrary string</TD><TD>Some arbitrary string</TD></TR>"
    "</table>");

  Fixture fixture;
  TestFixtureMaker maker;

  fixture.setMaker(&maker);
  fixture.doTables(&p);

  string out;

  p.print(out);
  CHECK(string::npos != out.find(Fixture::rightColor));
}

TEST(Framework, bangOperator)
{
  Parse p(
    "<table><TR><TD>TestFixture</TD></TR>"
    "<TR><TD>stringInput</TD><TD>stringOutput!</TD></TR>"
    "<TR><TD>Some arbitrary string</TD><TD>Some arbitrary string</TD></TR>"
    "</table>");

  Fixture fixture;
  TestFixtureMaker maker;

  fixture.setMaker(&maker);
  fixture.doTables(&p);

  string out;

  p.print(out);
  CHECK(string::npos != out.find(Fixture::rightColor));
}

TEST(Framework, friendlyNames)
{
  Parse p(
    "<table><TR><TD>Test Fixture</TD></TR>"
    "<TR><TD>    String Input    </TD><TD>   String Output?   </TD></TR>"
    "<TR><TD>Some arbitrary string</TD><TD>Some arbitrary string</TD></TR>"
    "</table>");

  Fixture fixture;
  TestFixtureMaker maker;

  fixture.setMaker(&maker);
  fixture.doTables(&p);

  string out;

  p.print(out);
  CHECK(string::npos != out.find(Fixture::rightColor));
}

