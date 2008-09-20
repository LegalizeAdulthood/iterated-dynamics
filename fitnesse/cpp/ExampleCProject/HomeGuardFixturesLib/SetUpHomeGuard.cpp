//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Platform.h"
#include "SetUpHomeGuard.h"
#include "HomeGuardInit.h"

extern "C"
{
#include "MockFrontPanel.h"
#include "HomeGuard.h"
}

HomeGuard* homeGuard = 0;
FrontPanel* frontPanel = 0;


SetUpHomeGuard::SetUpHomeGuard() 
{ 			
	HomeGuardInit();
}

void HomeGuardInit()
{
    frontPanel = MockFrontPanel_Create();
    homeGuard = HomeGuard_Create(frontPanel);
}

void HomeGuardDestroy()
{
    homeGuard = send(homeGuard, Destroy);
    frontPanel = send(frontPanel, Destroy);
}

FrontPanel* SetUpHomeGuard::GetFrontPanel() 
{
  return frontPanel;
}
