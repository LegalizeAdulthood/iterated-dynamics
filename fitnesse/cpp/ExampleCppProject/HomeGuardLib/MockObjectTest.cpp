#include "UnitTestHarness/TestHarness.h"

#include "MockFrontPanel.h"

EXPORT_TEST_GROUP(MockObjects);

static MockFrontPanel* frontPanel;
static void SetUp()
{
    frontPanel = new MockFrontPanel();
}

static void TearDown()
{
    delete frontPanel;
}


TEST(MockObjects, Create)
{
}

TEST(MockObjects, PowerUp)
{
// the mock should come up in the oppisite state desired, to make sure it is 
// initialized properly

//    CHECK(true == frontPanel->isArmedIndicatorOn());
//    CHECK(true == frontPanel->isVisualAlarmOn());
//    CHECK(true == frontPanel->isAudioAlarmOn());
//    STRCMP_EQUAL("none", frontPanel->GetDisplay());
}

