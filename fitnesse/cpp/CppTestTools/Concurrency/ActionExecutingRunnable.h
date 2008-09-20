//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_ActionExecutingRunnable_h
#define D_ActionExecutingRunnable_h

#include "Runnable.h"
#include "Action.h"

class ActionExecutingRunnable : public Runnable
  {

  public:
    ActionExecutingRunnable(Action* action) : itsAction(action)
    {}
    ;

    virtual ~ActionExecutingRunnable()
    {
      delete itsAction;
    };


    void run()
    {
      itsAction->execute();
    }

  private:
    Action* itsAction;

    ActionExecutingRunnable(const ActionExecutingRunnable&);
    ActionExecutingRunnable& operator=(const ActionExecutingRunnable&);

  };

#endif  // D_ActionExecutingRunnable_h


