#include "UnitTestHarness/TestHarness.h"

extern "C"
{
#include "HomeGuard.h"
#include "MockFrontPanel.h"
}

EXPORT_TEST_GROUP(HomeGuard);

static HomeGuard* homeGuard;
static FrontPanel* frontPanel;

static void SetUp()
{
    frontPanel = MockFrontPanel_Create();
    homeGuard = HomeGuard_Create(frontPanel);
}

static void TearDown()
{
    homeGuard = send(homeGuard, Destroy);
    frontPanel = send(frontPanel, Destroy);
}


TEST(HomeGuard, Create)
{
}

TEST(HomeGuard, PowerUp)
{
//    LONGS_EQUAL(0, MockFrontPanel_IsArmed());
//    STRCMP_EQUAL("READY", MockFrontPanel_GetDisplay());
}

#if 0
TEST(HomeGuard, ArmDisarm)
{
    send(homeGuard, Arm);
    CHECK(1 == MockFrontPanel_IsArmed());
    STRCMP_EQUAL("ARMED", MockFrontPanel_GetDisplay());

    send(homeGuard, DisArm);
    CHECK(0 == MockFrontPanel_IsArmed());
    STRCMP_EQUAL("READY", MockFrontPanel_GetDisplay());
}

TEST(HomeGuard, BreakInWithOneSensorDefined)
{
  //your turn to write the test. Have the instructor over to see your work
}

TEST(HomeGuard, BreakInWithMultipleSensorsDefined)
{
}

#endif


