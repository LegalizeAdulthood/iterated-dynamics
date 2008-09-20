//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_MockFrontPanel_H
#define D_MockFrontPanel_H

#include "FrontPanel.h"
#include <string>

class MockFrontPanel : public FrontPanel
{
  public: 
    MockFrontPanel() 
      : audibleAlarmIndicator(false)
      , alarmIndicator(false)
      , armedIndicator(false) {};

    virtual ~MockFrontPanel();

    void armOn() { 
      armedIndicator = true;
    }

    void armOff() { 
      armedIndicator = false;
    }

    void audibleAlarmOn() {audibleAlarmIndicator = true;}
    void audibleAlarmOff() {audibleAlarmIndicator = false;}
    void visualAlarmOn() {alarmIndicator = true;}
    void visualAlarmOff() {alarmIndicator = false;}


    void display(char* message) {displayString = message;}

    bool isAudibleAlarmOn() {return audibleAlarmIndicator;}
    bool isVisualAlarmOn() {return alarmIndicator; }
    bool isArmedIndicatorOn() {return armedIndicator; }
    std::string getDisplayString() {return displayString;}

  private:
    bool audibleAlarmIndicator;
    bool alarmIndicator;
    bool armedIndicator;
    std::string displayString;

    MockFrontPanel(const MockFrontPanel&);
    MockFrontPanel& operator=(const MockFrontPanel&);
};
#endif
