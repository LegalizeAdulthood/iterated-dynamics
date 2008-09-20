
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef PRIMITIVEFIXTURE_H
#define PRIMITIVEFIXTURE_H

#include "Fixture.h"

class PrimitiveFixture : public Fixture
  {
  public:

    static long parseLong(Parse *cell);
    static double parseDouble(Parse *cell);
    void check(Parse *cell, string value);
    void check(Parse *cell, long value);
    void check(Parse *cell, double value);

  };


#endif
