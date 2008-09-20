
// Copyright (c) 2003 Michael Feathers
// Based on FIT by Cunningham & Cunningham, Inc
// Released under the terms of the GNU General Public License version 2 or later.

#ifndef TESTACCOUNT_H
#define TESTACCOUNT_H

class TestAccount
  {
  public:
    TestAccount(int f) : field(f), field2(f+1)
    {}

    int field;
    int field2;
  };

#endif
