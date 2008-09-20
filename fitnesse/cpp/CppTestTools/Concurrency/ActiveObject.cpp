//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "ActiveObject.h"
#include "Action.h"
#include "NullAction.h"
#include "Event.h"
#include "Thread.h"
#include "Mutex.h"
#include "Runnable.h"


//----------------------------------------------------------------
// This class is used by Thread to call the associated ActiveObject
// Runnables are deleted by Thread
class ActiveObjectRunnable : public Runnable
  {
  public:
    ActiveObjectRunnable(ActiveObject* ao) : itsActiveObject(ao)
    {}
    ~ActiveObjectRunnable()
    {}
    void run()
    {
      itsActiveObject->run();
    }

  private:
    ActiveObject* itsActiveObject;
  };


//----------------------------------------------------------------
ActiveObject::ActiveObject()
    : itsTerminateAction(0)
    , started(false)
{
  itsThread = new Thread(new ActiveObjectRunnable(this));
  newActionEvent = new Event();
  itsMutex = new Mutex();
}

//----------------------------------------------------------------
ActiveObject::~ActiveObject()
{
  itsMutex->acquire();
  while (!actionQueue.empty())
    {
      Action* a = actionQueue.front();
      actionQueue.pop();
      delete a;
    }
  itsMutex->release();

  delete newActionEvent;
  delete itsThread;
  delete itsMutex;
}

//----------------------------------------------------------------
void ActiveObject::add
  (Action* a)
  {
    itsMutex->acquire();
    actionQueue.push(a);
    itsMutex->release();
    newActionEvent->signal();
  }

//----------------------------------------------------------------
void ActiveObject::run()
{
  for (Action* a = waitForAction(); a; a = waitForAction())
    {
      a->execute();
      delete a;
    }

}

//----------------------------------------------------------------
void ActiveObject::start()
{
  started = true;
  itsThread->start();
}

//----------------------------------------------------------------
void ActiveObject::terminate()
{
  if (started)
    {
      add
        (itsTerminateAction);
      itsThread->join();
    }
  started = false;
}

//----------------------------------------------------------------
Action* ActiveObject::waitForAction()
{
  while (actionQueue.empty())
    {
      newActionEvent->wait();
    }

  itsMutex->acquire();
  Action* a = actionQueue.front();
  actionQueue.pop();
  itsMutex->release();

  return a;
}

