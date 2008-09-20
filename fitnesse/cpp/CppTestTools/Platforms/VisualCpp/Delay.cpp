//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Concurrency/Delay.h"
#include "Windows.h"

void Delay::ms(long time)
{
  Sleep(time);
}

void Delay::yield()
{
  Sleep(0);
}

