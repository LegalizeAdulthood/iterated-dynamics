
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef ROWMAP_H
#define ROWMAP_H

#include <vector>
#include <string>
#include <map>
#include <set>

using namespace std;

template<typename TYPE>
class RowMap
  {
  public:
    typedef vector<TYPE *> ElementList;
    typedef map<string, ElementList *> ElementMap;

    ElementList *getRow(const string& key) const
      {
        ElementList *result = 0;
        typename ElementMap::const_iterator it = theMap.find(key);
        if (it != theMap.end())
          result = it->second;
        return result;
      }

    void addKeysTo(set
                   <string>& keys) const
      {
        for (typename ElementMap::const_iterator it = theMap.begin();
             it != theMap.end();
             ++it)
          {
            keys.insert(it->first);
          }
      }

    void bin(string key, TYPE *row)
    {
      ElementList *values = 0;
      if (theMap.find(key) == theMap.end())
        {
          values = new ElementList;
          //			theMap.insert(ElementMap::value_type(key,values));
          theMap.insert(pair<string, ElementList *>(key,values));
        }
      else
        {
          values = theMap.find(key)->second;
        }
      values->push_back(row);
    }

    virtual ~RowMap()
    {
      for (typename ElementMap::iterator it = theMap.begin(); it != theMap.end(); ++it)
        delete it->second;
    }

  private:
    ElementMap theMap;

  };

#endif
