
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#include "Platform.h"
#include "Summary.h"

using namespace std;

void Summary::doTable(ParsePtr table)
{
  summary.insert(pair<string,string>("counts",counts()));
  table->parts->more.free();
  table->parts->more = rows(summary.begin(), summary.end());
}


ParsePtr Summary::rows(map<string,string>::iterator entries,
                       map<string,string>::iterator end)
{
  if (entries != end)
    {
      pair<string,string>	entry = *entries++;
      return tr(
               td(entry.first,
                  td(entry.second,
                     0)),
               rows(entries, end));
    }
  else
    return 0;
}


ParsePtr Summary::tr(ParsePtr parts, ParsePtr more)
{
  return new Parse("tr", "(null)", parts, more);
}

ParsePtr Summary::td(string body, ParsePtr more)
{
  return new Parse("td", body, 0, more);
}

