//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef HomeGuardContext_H
#define HomeGuardContext_H


#include "../HomeGuardLib/HomeGuard.h"

class MockFrontPanel;
class MockInputPort;

class HomeGuardContext
{
public:

  static void CreateHomeGuard();
  static void DestroyHomeGuard();
  static MockFrontPanel* GetFrontPanel() {return panel;}
  static HomeGuard* GetHomeGuard() {return homeGuard;}

private:
  static HomeGuard* homeGuard;
  static MockFrontPanel* panel;

  HomeGuardContext();
  HomeGuardContext(const HomeGuardContext&);
  ~HomeGuardContext();
  HomeGuardContext& operator=(const HomeGuardContext&);
};


#endif
