
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>


// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>


#include "Platform.h"
#include "ActionFixture.h"

#include "CommandAdapter.h"
#include "Parse.h"
#include "ResolutionException.h"

Fixture *ActionFixture::actor = 0;


ActionFixture::ActionFixture()
    : cells(0)
{
  PUBLISH_COMMAND(ActionFixture,start);
  PUBLISH_COMMAND(ActionFixture,enter);
  PUBLISH_COMMAND(ActionFixture,press);
  PUBLISH_COMMAND(ActionFixture,check);
}

ActionFixture::~ActionFixture()
{}

void ActionFixture::doCells(ParsePtr cells)
{

  this->cells = cells;
  try
    {
      TypeAdapter *action = TypeAdapter::on(this,cells->text());

      action->invoke();
    }
  catch(exception& e)
    {
      handleException(cells, e);
    }
}

void ActionFixture::start()
{
  freeActor();
  actor = makeFixture(cells->more->text());
}

void ActionFixture::enter()
{
  vector<string> args;
  args.push_back(cells->more->more->text());
  TypeAdapter *method = getActor().adapterFor(cells->more->text());
  method->invoke(args);
}

void ActionFixture::press()
{
  TypeAdapter *method = getActor().adapterFor(cells->more->text());
  method->invoke();
}

void ActionFixture::check()
{
  Fixture::check(cells->more->more, method());
}

TypeAdapter *ActionFixture::method() const
  {
    return getActor().adapterFor(cells->more->text());
  }

void ActionFixture::freeActor()
{
  delete actor;
  actor = 0;
}

Fixture& ActionFixture::getActor() const
  {
    if(actor)
      {
        return *actor;
      }
    else
      {
        throw ResolutionException("Fixture undefined");
      }
  }

