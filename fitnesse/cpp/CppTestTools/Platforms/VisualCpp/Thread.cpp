//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Concurrency/Thread.h"
#include "Concurrency/Runnable.h"
#include "Windows.h"


static DWORD WINAPI PrivateThreadEntry(LPVOID param)
{
  Runnable* r = static_cast<Runnable*>(param);
  r->run();

  return 0;
}


class ThreadImpl
  {
  public:

    ThreadImpl(Runnable*);
    ~ThreadImpl();
    HANDLE threadHandle;
    Runnable* itsRunnable;
    bool started;

  };

ThreadImpl::ThreadImpl(Runnable* runnable)
    : itsRunnable(runnable)
    , started(false)
{

  DWORD threadId;
  threadHandle = CreateThread(NULL, NULL, PrivateThreadEntry,
                              runnable, CREATE_SUSPENDED, &threadId);
}


ThreadImpl::~ThreadImpl()
{
  delete itsRunnable;
  CloseHandle(threadHandle);
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
  ResumeThread(innards->threadHandle);
  innards->started = true;
}

void Thread::join()
{
  if (innards->started)
    {
      WaitForSingleObject(innards->threadHandle, INFINITE);
      innards->started = false;
    }
}

