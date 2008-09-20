// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef FIXTUREMAKER_H
#define FIXTUREMAKER_H


#include <string>
#include <map>

class Fixture;

class FixtureMaker
  {
  public:
    virtual				~FixtureMaker() = 0;
    virtual Fixture		*make(const std::string& name) = 0;

    static std::pair<std::string,std::string>  splitName(const std::string& name);

  };

#define PUBLISH_FIXTURE(fixture) if (name == #fixture)	return new fixture;

#endif
