//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_Thread_h
#define D_Thread_h

class Runnable;
class ThreadImpl;

class Thread
  {

  public:
    Thread(Runnable* r);

    virtual ~Thread();

    void start();
    void join();

  private:

    ThreadImpl* innards;
    Thread(const Thread&);
    Thread& operator=(const Thread&);

  };

#endif // D_Thread_h


