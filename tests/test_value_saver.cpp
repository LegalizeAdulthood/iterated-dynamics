#include <value_saver.h>

#include <gtest/gtest.h>

TEST(TestValueSaver, setsNewValue)
{
    bool data{true};

    ValueSaver saved_data(data, false);

    EXPECT_FALSE(data);
}

TEST(TestValueSaver, restoresOriginalValue)
{
    bool data{true};
    {
        ValueSaver saved_data(data, false);
    }

    EXPECT_TRUE(data);
}
