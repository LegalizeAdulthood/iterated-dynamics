
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#include "Platform.h"
#include "Fixture.h"

#include <ctime>
#include <cstdio>
#include <algorithm>

#include "TypeAdapter.h"
#include "Parse.h"
#include "ResolutionException.h"
#include "FixtureMaker.h"
#include "ActionFixture.h"

using namespace std;

const string	Fixture::rightColor		= "#cfffcf";
const string	Fixture::wrongColor		= "#ffcfcf";
const string  Fixture::exceptionColor = "#ffffcf";
const string	Fixture::ignoreColor	= "#efefef";

map<string,string> Fixture::summary;
int Fixture::fixtureCount = 0;

Fixture::Fixture()
    : maker(0)
    , rights(0)
    , wrongs(0)
    , ignores(0)
    , exceptions(0)
{
  fixtureCount++;
}


Fixture::~Fixture()
{
  for (vector<TypeAdapter *>::iterator it = adapters.begin();
       it != adapters.end();
       ++it)
    {
      delete *it;
    }
  if (--fixtureCount == 0)
    {
      summary.clear();
    }
}


bool Fixture::hasAdapterFor(const string& member) const
  {
    for (vector<TypeAdapter *>::const_iterator it = adapters.begin();
         it != adapters.end();
         ++it)
      {
        if (member == (*it)->getName())
          return true;
      }
    return false;
  }

TypeAdapter *Fixture::adapterFor(const string& member) const
  {
    string name = friendlyName(member);

    for (vector<TypeAdapter *>::const_iterator it = adapters.begin();
         it != adapters.end();
         ++it)
      if (name == (*it)->getName())
        return *it;
    throw ResolutionException("Unable to find adapter for \""
                              + name
                              + "\"");
  }

string Fixture::friendlyName(const string& originalName) const
  {
    string name = friendlyFixtureName(originalName);
    *(name.begin()) = tolower(*(name.begin()));
    return name;
  }


string Fixture::friendlyFixtureName(const string& originalName) const
  {
    string name;
    bool lastWasSpace = false;

    string::const_iterator i = originalName.begin();

    for (; i != originalName.end(); i++)
      {
        if ((*i) == ' ')
          {
            lastWasSpace = true;
          }
        else
          {
            if (lastWasSpace)
              name.append(1, (char)toupper(*i));
            else
              name.append(1, *i);
            lastWasSpace = false;
          }
      }

    return name;
  }


void Fixture::setMaker(FixtureMaker *newMaker)
{
  maker = newMaker;
}


Fixture *Fixture::makeFixture(const string& name) const
  {
    if (!maker)
      throw ResolutionException("No FixtureMaker set");
    return maker->make(friendlyFixtureName(name));
  };


string Fixture::currentTime()
{
  time_t theClock;

  time(&theClock);
  struct tm *current = localtime(&theClock);

  return asctime(current);
}



void Fixture::doTables(ParsePtr tables)
{
  summary.insert(pair<string,string>("run date", currentTime()));
  while (tables)
    {

      Parse *heading = tables->at(0,0,0);

      if (heading)
        {
          try
            {
              auto_ptr<Fixture> fixture(makeFixture(heading->text()));
              fixture->setMaker(maker);
              fixture->doTable(tables);
              rights += fixture->rights;
              wrongs += fixture->wrongs;
              ignores += fixture->ignores;
              exceptions += fixture->exceptions;

            }
          catch (exception& e)
            {
              handleException (heading, e);
            }
        }
      tables = tables->more;
    }
  ActionFixture::freeActor();
}


void Fixture::doTable(ParsePtr table)
{
  doRows(table->parts->more);
}

void Fixture::doRows(ParsePtr rows)
{
  while (rows)
    {
      doRow(rows);
      rows = rows->more;
    }
}

void Fixture::doRow(ParsePtr row)
{
  doCells(row->parts);
}

void Fixture::doCells(ParsePtr cells)
{
  for (int i = 0; cells; i++)
    {
      try
        {
          doCell(cells, i);
        }
      catch (exception& e)
        {
          handleException(cells, e);
        }
      cells = cells->more;
    }
}

void Fixture::doCell(ParsePtr cell, int columnNumber)
{
  ignore(cell);
}


void Fixture::handleException(ParsePtr cell, exception& e)
{
  string addition = "<hr><font size=-2><pre>";
  addition += e.what();
  addition += "</pre></font>";
  cell->addToBody(addition);
  cell->addToTag(" bgcolor=\"" + exceptionColor + "\"");
  exceptions++;
}


void Fixture::ignore(ParsePtr cell)
{
  color(cell,ignoreColor);
  ignores++;
}



void Fixture::check(ParsePtr cell, TypeAdapter *a)
{
  if (a == 0)
    {
      ignore(cell);
      return;
    }

  string text = cell->text();
  if (text == "")
    {
      return;
    }
  if (text == "error")
    {
      try
        {
          wrong(cell, a->valueAsString());
        }
      catch (ResolutionException& e)
        {
          handleException (cell, e);
        }
      catch (exception&)
        {
          right(cell);
        }
      return;
    }
  try
    {
      string v = a->valueAsString();
      if (v == text)
        {
          right(cell);
        }
      else
        {
          wrong(cell, v);
        }
    }
  catch (exception& e)
    {
      handleException(cell, e);
    }
}


void Fixture::right(ParsePtr cell)
{
  color(cell, rightColor);
  rights++;
}


void Fixture::wrong(ParsePtr cell)
{
  color(cell, wrongColor);
  wrongs++;
}


void Fixture::wrong(ParsePtr cell, string actual)
{
  wrong(cell);
  cell->addToBody(label("expected") + "<hr>" + escape(actual) + label("actual"));
}


void Fixture::color(ParsePtr cell, string colorCode)
{
  cell->addToTag(" bgcolor=\"" + colorCode + "\"");
}


string Fixture::label(string s)
{
  return " <font size=-1 color=#c08080><i>" + s + "</i></font>";
}


string Fixture::escape (string s)
{
  return escape(escape(s, '&', "&amp;"), '<', "&lt;");
}

string Fixture::escape(string s, char from, string to)
{
  int i = -1;
  while ((i = s.find(from, i+1)) >= 0)
    {
      if (i == 0)
        {
          s = to + s.substr(1);
        }
      else if (i == static_cast<int>(s.size()))
        {
          s = s.substr(0, i) + to;
        }
      else
        {
          s = s.substr(0, i) + to + s.substr(i+1);
        }
    }
  return s;
}


bool Fixture::endsWith(string s, string suffix)
{
  if (suffix.size() > s.size())
    return false;
  return s.substr(s.size() - suffix.size()) == suffix;
}

string Fixture::counts()
{
  char buffer[120];
  sprintf(buffer,"%d right, %d wrong, %d ignored, %d exceptions",
          rights, wrongs, ignores, exceptions);
  return buffer;
}

