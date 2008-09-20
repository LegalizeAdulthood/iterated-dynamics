
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#include "Platform.h"
#include "ColumnFixture.h"

#include "Parse.h"
#include "TypeAdapter.h"



ColumnFixture::~ColumnFixture()
{}

const Fixture	*ColumnFixture::getTargetClass() const
  {
    return this;
  }

void ColumnFixture::reset()
{}

void ColumnFixture::doRows(ParsePtr rows)
{
  bind(rows->parts);
  Fixture::doRows(rows->more);
}

void ColumnFixture::doRow(ParsePtr row)
{
  reset();
  Fixture::doRow(row);
  execute();
}

void ColumnFixture::execute()
{}

void ColumnFixture::doCell(ParsePtr cell, int column)
{
  TypeAdapter *a = columnBindings[column];
  try
    {
      if (a == 0)
        {
          ignore(cell);
        }
      else if (a->isObject())
        {
          string text = cell->text();
          if (text != "")
            {
              a->set
              (text);
            }
        }
      else if (!a->isObject())
        {
          check(cell, a);
        }
    }
  catch(exception& e)
    {
      handleException(cell, e);
    }

}


void ColumnFixture::bind (ParsePtr heads)
{
  for (int i=0; heads; i++, heads = heads->more)
    {
      columnBindings.push_back(0);
      string name = heads->text();

      string suffix1 = "()";
      string suffix2 = "?";
      string suffix3 = "!";
      try
        {
          if (endsWith(name, suffix1))
            {
              columnBindings[i] = bindFunctionCall(name, suffix1);
            }
          else if (endsWith(name, suffix2))
            {
              columnBindings[i] = bindFunctionCall(name, suffix2);
            }
          else if (endsWith(name, suffix3))
            {
              columnBindings[i] = bindFunctionCall(name, suffix3);
            }
          else
            {
              columnBindings[i] = getTargetClass()->adapterFor(name);
            }
        }
      catch (exception& e)
        {
          handleException (heads, e);
        }
    }
}

TypeAdapter* ColumnFixture::bindFunctionCall (string& name, string& suffix)
{
  string methodName = name.substr(0,name.size() - suffix.size());
  return getTargetClass()->adapterFor(methodName);
}

