//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_Event_h
#define D_Event_h

struct EventInnards;

class Event
  {

  public:
    Event();

    virtual ~Event();

    void signal();
    void wait();
    bool isSignaled();

  private:

    EventInnards* innards;

    Event(const Event&);
    Event& operator=(const Event&);

  };

#endif  // D_Event_h


