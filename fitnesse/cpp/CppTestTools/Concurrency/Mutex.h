//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_Mutex_h
#define D_Mutex_h

struct MutexInnards;


class Mutex
  {

  public:
    Mutex();

    virtual ~Mutex();

    void acquire();
    void release();

  private:

    MutexInnards* innards;
    Mutex(const Mutex&);
    Mutex& operator=(const Mutex&);

  };

#endif  // D_Mutex_h


