
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#include "Platform.h"
#include "FixtureMaker.h"

using namespace std;

FixtureMaker::~FixtureMaker()
{}


pair<string,string> FixtureMaker::splitName(const string& name)
{
  unsigned int breakPoint = name.find(".");
  if (breakPoint == string::npos)
    return pair<string,string>(name,"");

  return pair<string,string>(
           name.substr(0,breakPoint),
           name.substr(breakPoint+1));
}

