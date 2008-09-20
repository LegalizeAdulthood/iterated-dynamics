//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Platform.h"
#include "TestHarness.h"
#include "SimpleString.h"

EXPORT_TEST_GROUP(SimpleString);

namespace
  {

  void SetUp()
  {}
  void TearDown()
  {}
  ;
};


TEST(SimpleString, Create)
{
  SimpleString s("hello");
}

TEST(SimpleString, Copy)
{
  SimpleString s1("hello");
  SimpleString s2(s1);

  CHECK_EQUAL(s1, s2);
}

TEST(SimpleString, Assignment)
{
  SimpleString s1("hello");
  SimpleString s2("goodbye");

  s2 = s1;

  CHECK_EQUAL(s1, s2);
}

TEST(SimpleString, Equality)
{
  SimpleString s1("hello");
  SimpleString s2("hello");

  CHECK(s1 == s2);
}

TEST(SimpleString, InEquality)
{
  SimpleString s1("hello");
  SimpleString s2("goodbye");

  CHECK(s1 != s2);
}

TEST(SimpleString, asCharString)
{
  SimpleString s1("hello");

  STRCMP_EQUAL("hello", s1.asCharString());
}

TEST(SimpleString, Size)
{
  SimpleString s1("hello!");

  LONGS_EQUAL(6, s1.size());
}

TEST(SimpleString, Addition)
{
  SimpleString s1("hello!");
  SimpleString s2("goodbye!");
  SimpleString s3("hello!goodbye!");
  SimpleString s4;
  s4 = s1 + s2;

  CHECK_EQUAL(s3, s4);
}

TEST(SimpleString, Concatenation)
{
  SimpleString s1("hello!");
  SimpleString s2("goodbye!");
  SimpleString s3("hello!goodbye!");
  SimpleString s4;
  s4 += s1;
  s4 += s2;

  CHECK_EQUAL(s3, s4);

  SimpleString s5("hello!goodbye!hello!goodbye!");
  s4 += s4;

  CHECK_EQUAL(s5, s4);
}


TEST(SimpleString, Contains)
{
  SimpleString s("hello!");
  SimpleString empty("");
  SimpleString beginning("hello");
  SimpleString end("lo!");
  SimpleString mid("l");
  SimpleString notPartOfString("xxxx");

  CHECK(s.contains(empty));
  CHECK(s.contains(beginning));
  CHECK(s.contains(end));
  CHECK(s.contains(mid));
  CHECK(!s.contains(notPartOfString));

  CHECK(empty.contains(empty));
  CHECK(!empty.contains(s));
}

