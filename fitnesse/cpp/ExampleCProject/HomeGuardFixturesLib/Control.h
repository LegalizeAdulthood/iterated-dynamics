//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_Control_H
#define D_Control_H

#include "Fit/ActionFixture.h"
#include "HomeGuardInit.h"
#include <string>

class Control : public Fixture 
{
public:

	Control()
	{
	PUBLISH_COMMAND(Control,armSystem);
	PUBLISH_COMMAND(Control,disarmSystem);
  PUBLISH_ENTER(Control,std::string, digits);
  PUBLISH_CHECK(Control,std::string, display);
	}

  ~Control() {}
	 
  virtual void armSystem() {  } 
	virtual void disarmSystem() {  }
  virtual void digits(std::string) {};
  virtual std::string display() { return "OK";};
};

#endif

