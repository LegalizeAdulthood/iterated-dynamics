//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//
#include "Platform.h"
#include "HomeGuardContext.h"
#include "HomeGuard.h"
#include "MockFrontPanel.h"

using namespace std;

HomeGuard* HomeGuardContext::homeGuard = 0;
MockFrontPanel* HomeGuardContext::panel = 0;

void HomeGuardContext::CreateHomeGuard()
{
  panel = new MockFrontPanel();
  homeGuard = new HomeGuard(panel);
}

void HomeGuardContext::DestroyHomeGuard()
{
  delete homeGuard;

  homeGuard = 0;
  panel = 0;
}
