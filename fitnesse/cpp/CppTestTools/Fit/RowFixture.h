
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef ROWFIXTURE_H
#define ROWFIXTURE_H

#include "ColumnFixture.h"
#include "RowMap.h"

#include <vector>
#include <string>
#include <list>
#include <map>
#include <set>

using std::vector;
using std::string;
using std::list;
using std::map;
using std::set
  ;
using std::auto_ptr;

class RowFixture : public ColumnFixture
  {
  public:

    typedef Fixture Object;
    typedef vector<Object *> ObjectList;

    typedef vector<Parse *> ParseList;

    virtual ObjectList query() const = 0;

    virtual void doRows(ParsePtr rows);
    virtual void match(ParseList& expected, ObjectList& computed, unsigned int col);

    static ParseList listOf(ParsePtr rows);
    static list<string> listOf(vector<string>& rows);
    static set
      <string> unionOfKeys(const RowMap<Parse>& parses, const RowMap<Object>& objects);

    auto_ptr<RowMap<Parse> >  eSort(ParseList& parses, int col);
    auto_ptr<RowMap<Object> > cSort(ObjectList& objects, int col);

    void check(ParseList& eList, ObjectList& cList);

    virtual ParsePtr buildRows(ObjectList& rows);
    virtual ParsePtr buildCells(Object *rows);
    virtual void mark(ParsePtr rows, string message);
    virtual void mark(ParseList::iterator begin, ParseList::iterator end, string message);
    const Fixture *getTargetClass() const = 0;

    ObjectList					results;
    ObjectList					surplus;
    ParseList					missing;

  };

template<typename L>
void addAll(vector<L*>& container, vector<L *> *source)
{
  if (source == 0)
    return;
  copy(source->begin(), source->end(), back_inserter(container));
}


#endif
