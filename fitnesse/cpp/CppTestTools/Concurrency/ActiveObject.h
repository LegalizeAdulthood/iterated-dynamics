//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_ActiveObject_h
#define D_ActiveObject_h

#include <queue>

class Thread;
class Action;
class Event;
class Mutex;

class ActiveObject
  {

  public:
    ActiveObject();

    virtual ~ActiveObject();

    void add
      (Action* a);
    void start();
    void terminate();

  private:
    Action* itsTerminateAction;
    std::queue<Action*> actionQueue;
    bool started;

    Thread* itsThread;
    Event* newActionEvent;
    Mutex* itsMutex;

    Action* waitForAction();
    void run();

    friend void activeObjectEntry(ActiveObject*);
    friend class ActiveObjectRunnable;

    ActiveObject(const ActiveObject&);
    ActiveObject& operator=(const ActiveObject&);

  };

#endif  // D_ActiveObject_h


