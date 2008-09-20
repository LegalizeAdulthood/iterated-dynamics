//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_FrontPanel_H
#define D_FrontPanel_H

#include <string>

class FrontPanel
{
  public: 
    FrontPanel() {}; 
    virtual ~FrontPanel() {};

   virtual void armOn() = 0;
   virtual void armOff() = 0;
   virtual void display(char*) = 0;
   virtual void audibleAlarmOn() = 0;
   virtual void audibleAlarmOff() = 0;
   virtual void visualAlarmOn() = 0;
   virtual void visualAlarmOff() = 0;

  private:

    FrontPanel(const FrontPanel&);
    FrontPanel& operator=(const FrontPanel&);
};
#endif
