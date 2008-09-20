//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Platform.h"
#include "TearDownHomeGuard.h"
#include "HomeGuardContext.h"


TearDownHomeGuard::TearDownHomeGuard()
{
	HomeGuardContext::DestroyHomeGuard();
}

