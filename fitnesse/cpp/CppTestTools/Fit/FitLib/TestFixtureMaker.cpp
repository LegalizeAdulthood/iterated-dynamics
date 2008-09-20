
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#include "Platform.h"
#include "TestFixtureMaker.h"

#include "ResolutionException.h"
#include "TestFixture.h"
#include "ActionFixture.h"
#include "Summary.h"


Fixture *TestFixtureMaker::make(const string& name)
{
  if (name == "TestFixture")
    return new TestFixture;
  if (name == "ActionFixture")
    return new ActionFixture;
  if (name == "Summary")
    return new Summary;

  throw ResolutionException("Unable to make fixture " + name);
}

