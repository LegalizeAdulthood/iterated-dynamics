//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_FlagSettingAction_h
#define D_FlagSettingAction_h

#include "Action.h"

class FlagSettingAction : public Action
  {

  public:
    FlagSettingAction(bool& aWasExecutedFlag) : wasExecuted(aWasExecutedFlag)
    {
      reset();
    };

    virtual ~FlagSettingAction()
    {}
    ;


    void execute()
    {
      wasExecuted = true;
    }

    void reset()
    {
      wasExecuted = false;
    }

  private:
    bool& wasExecuted;

    FlagSettingAction(const FlagSettingAction&);
    FlagSettingAction& operator=(const FlagSettingAction&);

  };

#endif  // D_FlagSettingAction_h


