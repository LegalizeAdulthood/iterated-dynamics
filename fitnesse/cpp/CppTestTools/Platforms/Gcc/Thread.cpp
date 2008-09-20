//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Concurrency/Thread.h"
#include "Concurrency/Runnable.h"
#include <pthread.h>


static void* PrivateThreadEntry(void* param)
{
  Runnable* r = static_cast<Runnable*>(param);
  r->run();
  pthread_exit(0);

  return 0;
}

class ThreadImpl
  {
  public:

    ThreadImpl(Runnable*);
    ~ThreadImpl();
    pthread_t thread;
    Runnable* itsRunnable;
    bool started;

  };

ThreadImpl::ThreadImpl(Runnable* runnable)
    : itsRunnable(runnable)
    , started(false)
{}


ThreadImpl::~ThreadImpl()
{
  delete itsRunnable;
}


Thread::Thread(Runnable* runnable)
{
  innards = new ThreadImpl(runnable);
}

Thread::~Thread()
{
  delete innards;
}

void Thread::start()
{
  if (!innards->started)
    {
      pthread_create(&innards->thread, NULL, PrivateThreadEntry, innards->itsRunnable);
      innards->started = true;
    }
}

void Thread::join()
{
  if (innards->started)
    {
      int status;
      pthread_join(innards->thread, (void**)&status);
      innards->started = false;
    }
}

