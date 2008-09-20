
// Copyright (c) 2003 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.

#include "Platform.h"
#include "UnitTestHarness/TestHarness.h"
#include "ActionFixture.h"
#include "Parse.h"
#include "IllegalAccessException.h"
#include "TypeAdapter.h"
#include "TestFixtureMaker.h"

class AccountFixture : public Fixture
  {
  public:

    AccountFixture()
        : balance (0)
    {
      PUBLISH_ENTER(AccountFixture,int,deposit);
      PUBLISH_CHECK(AccountFixture,int,balance);
      PUBLISH_PRESS(AccountFixture,bonus);
    }

    void deposit(int amount)
    {
      balance += amount;
    }

    int balance;

    void bonus()
    {
      balance++;
    }

  };


class ActionTestMaker : public TestFixtureMaker
  {
    virtual Fixture *make(const string& name)
    {
      if (name == "AccountFixture")
        return new AccountFixture;
      return TestFixtureMaker::make(name);
    }

  };

EXPORT_TEST_GROUP(ActionFixture);

namespace
  {
  void SetUp()
  {}
  ;
  void TearDown()
  {}
  ;
}

TEST(ActionFixture,rightNotWrong)
{
  Parse p(
    "<table><TR><TD>ActionFixture</TD></TR>"
    "<TR><TD>start</TD><TD>AccountFixture</TD><TD></TD></TR>"
    "<TR><TD>enter</TD><TD>deposit</TD><TD>5</TD></TR>"
    "<TR><TD>check</TD><TD>balance</TD><TD>5</TD></TR>"
    "<TR><TD>press</TD><TD>bonus</TD></TR>"
    "<TR><TD>check</TD><TD>balance</TD><TD>6</TD></TR>"
    "</table>");

  Fixture fixture;
  ActionTestMaker maker;

  fixture.setMaker(&maker);
  fixture.doTables(&p);

  string out;

  p.print(out);
  CHECK(string::npos != out.find(Fixture::rightColor));
  CHECK(string::npos == out.find(Fixture::wrongColor));

}



