//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_ScopeLock_h
#define D_ScopeLock_h

#include "Mutex.h"

class ScopeLock
  {

  public:
    ScopeLock(Mutex* m) : itsMutex(m)
    {
      itsMutex->acquire();
    }

    virtual ~ScopeLock()
    {
      itsMutex->release();
    }

  private:

    Mutex* itsMutex;

    ScopeLock(const ScopeLock&);
    ScopeLock& operator=(const ScopeLock&);

  };

#endif  // D_ScopeLock_h


