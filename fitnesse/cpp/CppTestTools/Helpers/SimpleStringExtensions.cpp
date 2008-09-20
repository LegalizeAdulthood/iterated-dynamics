//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//


#include "SimpleStringExtensions.h"

SimpleString StringFrom (const std::string& value)
{
  return SimpleString(value.c_str());
}
