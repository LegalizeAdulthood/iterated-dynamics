//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Concurrency/Event.h"
#include "Windows.h"

struct EventInnards
  {
    HANDLE itsHandle;
  };

Event::Event()
{
  innards = new EventInnards;
  innards->itsHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
}

Event::~Event()
{
  CloseHandle(innards->itsHandle);
  delete innards;
}


void Event::signal()
{
  SetEvent(innards->itsHandle);
}


void Event::wait()
{
  WaitForSingleObject(innards->itsHandle, INFINITE);
  ResetEvent(innards->itsHandle);
}

bool Event::isSignaled()
{
  return WAIT_TIMEOUT != WaitForSingleObject(innards->itsHandle, 0);
}

