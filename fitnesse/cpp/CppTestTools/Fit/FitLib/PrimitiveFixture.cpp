
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#include "Platform.h"
#include "PrimitiveFixture.h"

#include "Parse.h"

#include <sstream>
#include <string>

using namespace std;


long PrimitiveFixture::parseLong(Parse *cell)
{
  stringstream stage;
  long value;

  stage << cell->text();
  stage >> value;
  return value;
}


double PrimitiveFixture::parseDouble(Parse *cell)
{
  stringstream stage;
  double value;

  stage << cell->text();
  stage >> value;
  return value;
}



void PrimitiveFixture::check(Parse *cell, string value)
{
  if (cell->text() == value)
    {
      right(cell);
    }
  else
    {
      wrong(cell, value);
    }

}


void PrimitiveFixture::check(Parse *cell, long value)
{
  if (parseLong(cell) == value)
    {
      right(cell);
    }
  else
    {
      stringstream stage;
      string s;
      stage << value;
      stage >> s;
      wrong(cell, s);
    }
}


void PrimitiveFixture::check(Parse *cell, double value)
{
  if (parseDouble(cell) == value)
    {
      right(cell);
    }
  else
    {
      stringstream stage;
      string s;
      stage << value;
      stage >> s;
      wrong(cell, s);
    }
}


