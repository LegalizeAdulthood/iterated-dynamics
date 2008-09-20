
// Copyright (c) 2003 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.

#ifndef TESTROWFIXTURE_H
#define TESTROWFIXTURE_H


#include "RowFixture.h"

class TestAccount;

class TestRowFixture : public RowFixture
  {
  public:
    TestRowFixture();
    virtual					~TestRowFixture();
    const Fixture			*getTargetClass() const;

    virtual ObjectList		query() const;

  private:
    ObjectList objects;
    std::vector<TestAccount *> accounts;
  }
;

#endif
