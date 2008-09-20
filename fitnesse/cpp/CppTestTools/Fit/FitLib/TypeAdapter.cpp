
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#include "Platform.h"
#include "TypeAdapter.h"

#include "Fixture.h"

int TypeAdapter::referenceCount = 0;
/*const*/
vector<string> TypeAdapter::noArgs;

TypeAdapter *TypeAdapter::on(Fixture *f, const string& member)
{
  return f->adapterFor(member);
}


TypeAdapter::TypeAdapter(const string& name)
    : name(name)
{
  referenceCount++;
}

TypeAdapter::~TypeAdapter()
{
  if (--referenceCount == 0)
    noArgs.clear();
}


const string TypeAdapter::getName() const
  {
    return name;
  }


