
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef SUMMARY_H
#define SUMMARY_H

#include <string>
#include <map>

using std::string;

#include "Fixture.h"

class Summary : public Fixture
  {
  public:
    virtual void				doTable(ParsePtr table);

    static ParsePtr				rows(std::map<string,string>::iterator keys,
                            std::map<string,string>::iterator end);

    static ParsePtr				tr(ParsePtr parts, ParsePtr more);
    static ParsePtr				td(string body, ParsePtr more);


  };


#endif
