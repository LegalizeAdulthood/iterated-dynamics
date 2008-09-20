//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_Action_h
#define D_Action_h

class Action
  {

  public:
    Action();

    virtual ~Action();

    virtual void execute() = 0;

  private:
    Action(const Action&);
    Action& operator=(const Action&);

  };

#endif  // D_Action_h


