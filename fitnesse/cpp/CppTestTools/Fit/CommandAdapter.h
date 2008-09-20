

// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef COMMANDADAPTER_H
#define COMMANDADAPTER_H

#include "TypeAdapter.h"
#include "IllegalAccessException.h"

#include <string>


template<typename AFixture>
class CommandAdapter : public TypeAdapter
  {
    typedef void (AFixture::*Command)();

  public:
    CommandAdapter(const string& name, Command command, AFixture *fixture)
        : TypeAdapter(name), command(command), fixture(fixture)
    {}

    ~CommandAdapter()
    {}

    virtual void set
      (const string& newValue)
    {}

    virtual void invoke(const vector<string>& args = noArgs)
    {
      if(args.size() != 0)
        throw IllegalAccessException("Expected zero arguments in CommandAdapter");
      (fixture->*command)();
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
    Command				command;
    AFixture			*fixture;

  };




#endif
