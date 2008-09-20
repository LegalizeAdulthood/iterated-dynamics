#include "Platform.h"
#include "Fit/Fit.h"
#include "Fit/Summary.h"
#include "Fit/ResolutionException.h"

#include "../HomeGuardFixturesLib/HomeGuardFixtureMaker.h"
#include "Control.h"
#include "SetUpHomeGuard.h"
#include "TearDownHomeGuard.h"
#include "CheckFrontPanel.h"
#include "CheckPhoneLine.h"
#include "DefineSensors.h"
#include "portControl.h"
#include "EventLogOutput.h"
#include "FrontPanelController.h"


#include <iostream>

using namespace std;

HomeGuardFixtureMaker::HomeGuardFixtureMaker()
{
}

HomeGuardFixtureMaker::~HomeGuardFixtureMaker()
{
}

Fixture* HomeGuardFixtureMaker::make(const string& fullName)
{
    string name = fullName;
	  string libraryName	= splitName(fullName).first;
    if (libraryName != fullName)
        name = splitName(fullName).second;

	PUBLISH_FIXTURE(Fixture);
	PUBLISH_FIXTURE(ColumnFixture);
    PUBLISH_FIXTURE(ActionFixture)
	PUBLISH_FIXTURE(PrimitiveFixture);
	PUBLISH_FIXTURE(Summary);

    PUBLISH_FIXTURE(Control)
    PUBLISH_FIXTURE(FrontPanelController)
    PUBLISH_FIXTURE(SetUpHomeGuard)
    PUBLISH_FIXTURE(TearDownHomeGuard)
    PUBLISH_FIXTURE(CheckFrontPanel)
    PUBLISH_FIXTURE(CheckPhoneLine)
    PUBLISH_FIXTURE(DefineSensors)
    PUBLISH_FIXTURE(PortControl)
    PUBLISH_FIXTURE(EventLogOutput)

    throw ResolutionException(name);

    return 0;
}
