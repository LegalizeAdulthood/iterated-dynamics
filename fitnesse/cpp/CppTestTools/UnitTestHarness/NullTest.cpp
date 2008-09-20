//
// Copyright (c) 2005 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "NullTest.h"

static void stub()
{}

NullTest::NullTest()
    :Utest("NullGroup", "NullName", "NullFile", -1, stub, stub)
{}

NullTest::~NullTest()
{}

