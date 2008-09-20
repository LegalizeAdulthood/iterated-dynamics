#include "UnitTestHarness/TestHarness.h"

extern "C"
{
#include "PhoneDialer.h"
}

EXPORT_TEST_GROUP(PhoneDialer);

namespace 
{
    PhoneDialer* phoneDialer;

    void SetUp()
    {
        phoneDialer = PhoneDialer_Create(0);
    }
    void TearDown()
    {
        send(phoneDialer, Destroy);
    }
}

TEST(PhoneDialer, Create)
{
    STRCMP_EQUAL("", phoneDialer->data.police);
}

