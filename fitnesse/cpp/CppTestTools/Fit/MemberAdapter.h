
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef MEMBERADAPTER_H
#define MEMBERADAPTER_H

#include "TypeAdapter.h"

#include <sstream>
#include <string>

using std::string;
using std::stringstream;

template<typename AFixture, typename AType>
class MemberAdapter : public TypeAdapter
  {
    typedef AType (AFixture::*MemberFunction)();
    typedef AType (AFixture::*MemberVariable);


  public:
    MemberAdapter(const string& name, MemberFunction function, AFixture *fixture)
        : TypeAdapter(name), function(function), variable(0), fixture(fixture)
    {}

    MemberAdapter(const string& name, MemberVariable variable, AFixture *fixture)
        : TypeAdapter(name), function(0), variable(variable), fixture(fixture)
    {}

    ~MemberAdapter()
    {}

    virtual void invoke(const vector<string>& args = TypeAdapter::noArgs)
    {
      value = (fixture->*function)();
    }

    virtual void set
      (const string& newValue)
      {
        Convert(newValue, value);
        fixture->*variable = value;
      }

    virtual string valueAsString()
    {
      if (isObject())
        {
          value = fixture->*variable;
        }
      else
        {
          invoke();
        }

      string result;
      Convert(value, result);

      return result;
    }

    virtual bool isObject() const
      {
        return variable != 0;
      }


  private:
    MemberFunction		function;
    MemberVariable		variable;
    AType				value;
    AFixture			*fixture;


  };

namespace
  {
  template<class T1, class T2>
  void Convert(const T1& in, T2& out)
  {
    stringstream s;
    s << in;
    s >> out;
  }

  //  template <> void Convert<string>(const string& in, string& out)
  void Convert(const string& in, string& out)
  {
    out = in;
  }
}

#endif
