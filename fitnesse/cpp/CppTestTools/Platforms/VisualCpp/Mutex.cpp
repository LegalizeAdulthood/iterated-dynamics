//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Concurrency/Mutex.h"
#include "Windows.h"

struct MutexInnards
  {
public:
    int nestLevel;
    CRITICAL_SECTION  itsCriticalSection;
  };


Mutex::Mutex()
{
  innards = new MutexInnards();
  innards->nestLevel = 0;
  InitializeCriticalSection(&innards->itsCriticalSection);
}

Mutex::~Mutex()
{
  while (innards->nestLevel > 0)
    {
      innards->nestLevel--;
      LeaveCriticalSection(&innards->itsCriticalSection);
    }

  DeleteCriticalSection(&innards->itsCriticalSection);
  delete innards;
}

void Mutex::acquire()
{
  EnterCriticalSection(&innards->itsCriticalSection);
  innards->nestLevel++;
}

void Mutex::release()
{
  if (innards->nestLevel > 0)
    {
      innards->nestLevel--;
      LeaveCriticalSection(&innards->itsCriticalSection);
    }

}

