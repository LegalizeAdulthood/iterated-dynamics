
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef TYPEADAPTER_H
#define TYPEADAPTER_H

#include <string>
#include <vector>

class Fixture;

using std::string;
using std::vector;

class TypeAdapter
  {
  public:
    TypeAdapter(const string& name);
    virtual ~TypeAdapter() =  0;

    virtual void				set
    (const string& newValue) = 0;
    virtual const string		getName() const;
    virtual void				invoke(const vector<string>& args = noArgs) = 0;
    virtual string				valueAsString() = 0;
    virtual bool				isObject() const = 0;

    static TypeAdapter			*on(Fixture *f, const string& member);

    string						name;
    static /*const*/ vector<string>	noArgs;

  private:

    static int referenceCount;
    TypeAdapter(const TypeAdapter&);
    TypeAdapter& operator=(TypeAdapter&);


  };


#endif
