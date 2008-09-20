
// Copyright (c) 2003 Cunningham & Cunningham, Inc.
// Released under the terms of the GNU General Public License version 2 or later.
//
// C++ translation by Michael Feathers <mfeathers@objectmentor.com>

#ifndef ACTIONFIXTURE_H
#define ACTIONFIXTURE_H

#include "Fixture.h"
#include "EnterAdapter.h"
#include "CommandAdapter.h"


class ActionFixture : public Fixture
  {
  public:

    ActionFixture();
    virtual ~ActionFixture();

    virtual void			doCells(ParsePtr cells);
    virtual void			start();
    virtual void			enter();
    virtual void			press();
    virtual void			check();

    virtual TypeAdapter		*method() const;

    ParsePtr				cells;
    Fixture			        &getActor() const;
    static void			    freeActor();

  private:
    static Fixture	        *actor;
  };

#define PUBLISH_CHECK(testClass,type,name) PUBLISH(testClass,type,name)
#define PUBLISH_PRESS(testClass,name) PUBLISH_COMMAND(testClass,name)
#define PUBLISH_ENTER(testClass,type,name)\
                adapters.push_back(new EnterAdapter<testClass, type>(#name, &testClass::name, this));
#define PUBLISH_COMMAND(testClass,name)\
		adapters.push_back(new CommandAdapter<testClass>(#name, &testClass::name, this));

#endif
