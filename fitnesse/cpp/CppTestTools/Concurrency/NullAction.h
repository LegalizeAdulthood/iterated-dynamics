//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_NullAction_h
#define D_NullAction_h

#include "Action.h"

class NullAction : public Action
  {

  public:
    NullAction();

    virtual ~NullAction();

    virtual void execute();

  private:
    NullAction(const NullAction&);
    NullAction& operator=(const NullAction&);

  };

#endif  // D_NullAction_h


