
// Copyright (c) 2004 Micahel Feathers, James Grenning, Micah Martin, Robert Martin.
// Released under the terms of the GNU General Public License version 2 or later.

#ifndef ENTERADAPTER_H
#define ENTERADAPTER_H

#include "TypeAdapter.h"

template<typename AFixture, typename ParameterType>
class EnterAdapter : public TypeAdapter
  {
    typedef void (AFixture::*Call)(ParameterType);

  public:
    EnterAdapter(const string& name, Call call, AFixture *fixture)
        : TypeAdapter(name), call(call), fixture(fixture)
    {}

    ~EnterAdapter()
    {}

    virtual void set
      (const string& newValue)
    {}

    virtual void invoke(const vector<string>& args = noArgs)
    {
      if(args.size() != 1)
        throw IllegalAccessException("Expected one argument in EnterAdapter");
      stringstream stream;
      ParameterType value;
      stream << args[0];
      stream >> value;
      (fixture->*call)(value);
    }

    virtual string valueAsString()
    {
      return "";
    }

    virtual bool isObject() const
      {
        return false;
      }

  private:
    Call				call;
    AFixture			*fixture;

  };

#endif
