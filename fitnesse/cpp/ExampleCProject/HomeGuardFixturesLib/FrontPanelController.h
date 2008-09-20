//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef D_FrontPanelController_H
#define D_FrontPanelController_H

#include "Fit/ActionFixture.h"
#include <string>

class FrontPanelController : public ActionFixture 
{
public:

	FrontPanelController()
    {
        thedigits = "";
        display = "not sure";
        armLed = "not sure";

		PUBLISH_ENTER(FrontPanelController,std::string,digits);

		PUBLISH_COMMAND(FrontPanelController,armButton);
		PUBLISH_COMMAND(FrontPanelController,enterButton);

		PUBLISH_CHECK(FrontPanelController,std::string,display);
		PUBLISH_CHECK(FrontPanelController,std::string,armLed);
      	PUBLISH_CHECK(FrontPanelController, std::string, display);      
    }
    
    void armButton() { display = "arm pressed";}
    void enterButton() {};
    
    std::string display;
    std::string armLed;
    std::string thedigits;
    void digits(std::string d) { display = d; }
	
};

#endif

