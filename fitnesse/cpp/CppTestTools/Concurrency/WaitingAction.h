//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_WaitingAction_h
#define D_WaitingAction_h

#include "Action.h"

class WaitingAction : public Action
  {

  public:
    WaitingAction(Event& e) : itsEvent(e)
    {}
    ;

    virtual ~WaitingAction()
    {}
    ;

    virtual void execute()
    {
      itsEvent.wait();
    }

  private:

    Event& itsEvent;
    WaitingAction(const WaitingAction&);
    WaitingAction& operator=(const WaitingAction&);

  };

#endif  // D_WaitingAction_h


