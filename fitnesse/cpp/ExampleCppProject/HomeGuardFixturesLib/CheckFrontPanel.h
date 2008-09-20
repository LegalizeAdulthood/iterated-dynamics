//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_CheckFrontPanel_H
#define D_CheckFrontPanel_H


#include "Fit/ColumnFixture.h"
#include "HomeGuardContext.h"
#include "MockFrontPanel.h"

class CheckFrontPanel : public ColumnFixture
{
public:
    CheckFrontPanel() 
    {
        panel = HomeGuardContext::GetFrontPanel();
        initialize();
    }

    ~CheckFrontPanel() {};

    void initialize()
    {
        PUBLISH(CheckFrontPanel,string,armed);
        PUBLISH(CheckFrontPanel,string,visualAlarm);
        PUBLISH(CheckFrontPanel,string,siren);
        PUBLISH(CheckFrontPanel,string,display);
    }

    void doRow(ParsePtr row)
    {
      ColumnFixture::doRow(row);
    }
 
  string armed() 
  { return boolToString(false); }
  string visualAlarm() 
    { return boolToString(false); }
  string siren() 
   { return boolToString(false); }
  string display() 
    { return "none"; }

private:

  string boolToString(bool condition) 
    { return condition ? "on" : "off"; }

  MockFrontPanel* panel;

};

#endif


