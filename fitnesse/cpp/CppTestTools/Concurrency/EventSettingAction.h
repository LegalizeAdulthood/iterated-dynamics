//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_EventSettingAction_h
#define D_EventSettingAction_h

#include "Action.h"
#include "Event.h"

class EventSettingAction : public Action
  {

  public:
    EventSettingAction(Event& e) : event(e)
    {
    };

    virtual ~EventSettingAction()
    {}
    ;


    void execute()
    {
      event.signal();
    }

  private:
    Event& event;

    EventSettingAction(const EventSettingAction&);
    EventSettingAction& operator=(const EventSettingAction&);

  };

#endif  // D_EventSettingAction_h


