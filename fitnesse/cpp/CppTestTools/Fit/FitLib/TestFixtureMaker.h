
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef TESTFIXTUREMAKER_H
#define TESTFIXTUREMAKER_H

#include "FixtureMaker.h"

#include <string>

using namespace std;


class TestFixtureMaker : public FixtureMaker
  {
  public:
    virtual Fixture *make(const string& name);

  };

#endif
