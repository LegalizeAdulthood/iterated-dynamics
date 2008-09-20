#include "UnitTestHarness/TestHarness.h"

extern "C"
{
#include "MockPhoneDialer.h"
}

EXPORT_TEST_GROUP(MockPhoneDialer);

static PhoneDialer* mockPhoneDialer;

static void SetUp()
{
    mockPhoneDialer = MockPhoneDialer_Create(0);
}

static void TearDown()
{
    send(mockPhoneDialer, Destroy);
}

TEST(MockPhoneDialer, Create)
{
}

TEST(MockPhoneDialer, OverrideExample)
{
    CHECK(!MockPhoneDialer_WerePoliceCalled()); 
    send(mockPhoneDialer, CallPolice);
    CHECK(MockPhoneDialer_WerePoliceCalled()); 
}

