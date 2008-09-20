#include "UnitTestHarness/TestHarness.h"

extern "C"
{
#include "MockFrontPanel.h"
}

EXPORT_TEST_GROUP(MockFrontPanel);

static FrontPanel* frontPanel;

static void SetUp()
{
    frontPanel = MockFrontPanel_Create();
}

static void TearDown()
{
    frontPanel = frontPanel->Destroy(frontPanel);
}

TEST(MockFrontPanel, Create)
{
    LONGS_EQUAL(0, MockFrontPanel_IsArmed());
    STRCMP_EQUAL("none", MockFrontPanel_GetDisplay());
}

#if 0
TEST(MockFrontPanel, ArmDisarm)
{
    send(frontPanel, ArmOn);
    LONGS_EQUAL(1, MockFrontPanel_IsArmed());
    send(frontPanel, ArmOff);
    LONGS_EQUAL(0, MockFrontPanel_IsArmed());
}

TEST(MockFrontPanel, Display)
{
    send1(frontPanel, Display, "Door open");
    STRCMP_EQUAL("Door open", MockFrontPanel_GetDisplay());
    send1(frontPanel, Display, "Basement flooding");
    STRCMP_EQUAL("Basement flooding", MockFrontPanel_GetDisplay());
}

#endif

