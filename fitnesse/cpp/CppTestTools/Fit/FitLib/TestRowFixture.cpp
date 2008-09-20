// Copyright (c) 2003 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.

#include "Platform.h"

#include "DomainObjectWrapper.h"
#include "TestRowFixture.h"
#include "TestAccount.h"


TestRowFixture::TestRowFixture()
{
  class TestAccountWrapper : public DomainObjectWrapper<TestAccount>
    {
    public:
      TestAccountWrapper(TestAccount *a)
          : DomainObjectWrapper<TestAccount>(a)
      {
        PUBLISH_MEMBER_OF(TestAccount,int,field,a);
        PUBLISH_MEMBER_OF(TestAccount,int,field2,a);
      }

    };

  for (int n = 0; n < 1; ++n)
    {
      TestAccount *account;
      account = new TestAccount(n);
      accounts.push_back(account);
      objects.push_back(new TestAccountWrapper(account));
    }
}



const Fixture *TestRowFixture::getTargetClass() const
  {
    // At this point, RowFixtures need an example object so that
    // column headings can be set in ColumnFixture::bind.  The
    // type adapters that are created there are only used for their
    // names in RowFixture; no one accesses through them.  However,
    // the type adapters are really used in ColumnFixture.  Looking
    // for a better way to handle this in C++.

    return objects[0];
  }


RowFixture::ObjectList TestRowFixture::query() const
  {
    return objects;
  }


TestRowFixture::~TestRowFixture()
{
  {
    for (ObjectList::iterator it = objects.begin(); it != objects.end(); ++it)
      {
        delete *it;
      }
  }

  {
    for (std::vector<TestAccount *>::iterator it = accounts.begin(); it != accounts.end(); ++it)
      {
        delete *it;
      }
  }

}

