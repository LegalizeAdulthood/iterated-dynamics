#include "choice_builder.h"

#include <gtest/gtest.h>

TEST(TestChoiceBuilder, yesNoChoice)
{
    ChoiceBuilder<1> builder;

    const ChoiceBuilder<1> &result = builder.yes_no("Evolution mode? (no for full screen)", true);

    EXPECT_STREQ("Evolution mode? (no for full screen)", result.choices[0]);
    EXPECT_EQ('y', result.uvalues[0].type);
    EXPECT_EQ(1, result.uvalues[0].uval.ch.val);
    EXPECT_EQ(1, result.count());
}

TEST(TestChoiceBuilder, intChoice)
{
    ChoiceBuilder<1> builder;

    const ChoiceBuilder<1> &result = builder.int_number("Image grid size (odd numbers only)", 50);

    EXPECT_STREQ("Image grid size (odd numbers only)", result.choices[0]);
    EXPECT_EQ('i', result.uvalues[0].type);
    EXPECT_EQ(50, result.uvalues[0].uval.ival);
    EXPECT_EQ(1, result.count());
}

TEST(TestChoiceBuilder, longChoice)
{
    ChoiceBuilder<1> builder;

    const ChoiceBuilder<1> &result = builder.long_number("Bailout value (0 means use default)", 50L);

    EXPECT_STREQ("Bailout value (0 means use default)", result.choices[0]);
    EXPECT_EQ('L', result.uvalues[0].type);
    EXPECT_EQ(50L, result.uvalues[0].uval.Lval);
    EXPECT_EQ(1, result.count());
}

TEST(TestChoiceBuilder, floatChoice)
{
    ChoiceBuilder<1> builder;

    const ChoiceBuilder<1> &result = builder.float_number("x parameter range (across screen)", 123.456f);

    EXPECT_STREQ("x parameter range (across screen)", result.choices[0]);
    EXPECT_EQ('f', result.uvalues[0].type);
    EXPECT_EQ(123.456f, result.uvalues[0].uval.dval);
    EXPECT_EQ(1, result.count());
}

TEST(TestChoiceBuilder, doubleChoice)
{
    ChoiceBuilder<1> builder;

    const ChoiceBuilder<1> &result = builder.double_number("x parameter range (across screen)", 123.456);

    EXPECT_STREQ("x parameter range (across screen)", result.choices[0]);
    EXPECT_EQ('d', result.uvalues[0].type);
    EXPECT_EQ(123.456, result.uvalues[0].uval.dval);
    EXPECT_EQ(1, result.count());
}

TEST(TestChoiceBuilder, intDoubleChoice)
{
    ChoiceBuilder<1> builder;

    const ChoiceBuilder<1> &result = builder.int_double_number("x parameter range (across screen)", 123.456);

    EXPECT_STREQ("x parameter range (across screen)", result.choices[0]);
    EXPECT_EQ('D', result.uvalues[0].type);
    EXPECT_EQ(123.456, result.uvalues[0].uval.dval);
    EXPECT_EQ(1, result.count());
}

TEST(TestChoiceBuilder, comment)
{
    ChoiceBuilder<1> builder;

    const ChoiceBuilder<1> &result = builder.comment("Press F4 to reset view parameters to defaults.");

    EXPECT_STREQ("Press F4 to reset view parameters to defaults.", result.choices[0]);
    EXPECT_EQ('*', result.uvalues[0].type);
    EXPECT_EQ(1, result.count());
}

TEST(TestChoiceBuilder, listChoice)
{
    ChoiceBuilder<1> builder;
    const char *list[]{"one", "two", "three"};
    const int list_len = 3;
    const int list_value_len = 5;

    const ChoiceBuilder<1> &result = builder.list("First Function", list_len, list_value_len, list, 1);

    EXPECT_STREQ("First Function", result.choices[0]);
    EXPECT_EQ('l', result.uvalues[0].type);
    EXPECT_EQ(list_len, result.uvalues[0].uval.ch.llen);
    EXPECT_EQ(list_value_len, result.uvalues[0].uval.ch.vlen);
    EXPECT_EQ(1, result.uvalues[0].uval.ch.val);
    EXPECT_EQ(1, result.count());
}

TEST(TestChoiceBuilder, stringChoice)
{
    ChoiceBuilder<1> builder;

    const ChoiceBuilder<1> &result = builder.string("Browse search filename mask ", "*.gif");

    EXPECT_STREQ("Browse search filename mask ", result.choices[0]);
    EXPECT_EQ('s', result.uvalues[0].type);
    EXPECT_STREQ("*.gif", result.uvalues[0].uval.sval);
    EXPECT_EQ(1, result.count());
}

TEST(TestChoiceBuilder, stringBufferChoice)
{
    ChoiceBuilder<1> builder;
    constexpr int len{80};
    char buff[len];

    const ChoiceBuilder<1> &result = builder.string_buff("Browse search filename mask ", buff, len);

    EXPECT_STREQ("Browse search filename mask ", result.choices[0]);
    EXPECT_EQ(0x100 + len, result.uvalues[0].type);
    EXPECT_EQ(buff, result.uvalues[0].uval.sbuf);
    EXPECT_EQ(1, result.count());
}

TEST(TestChoiceBuilder, multipleChoices)
{
    ChoiceBuilder<2> builder;

    const ChoiceBuilder<2> &result = builder.comment("First").comment("Second");

    EXPECT_STREQ("First", result.choices[0]);
    EXPECT_EQ('*', result.uvalues[0].type);
    EXPECT_STREQ("Second", result.choices[1]);
    EXPECT_EQ('*', result.uvalues[1].type);
    EXPECT_EQ(2, result.count());
}

TEST(TestChoiceBuilder, overflow)
{
    ChoiceBuilder<1> builder;
    ChoiceBuilder<1> &result = builder.comment("First");

    EXPECT_THROW(result.comment("Second"), std::runtime_error);
}

TEST(TestChoiceBuilder, reset)
{
    ChoiceBuilder<1> builder;
    ChoiceBuilder<1> &result = builder.comment("First");

    result.reset();

    EXPECT_EQ(nullptr, result.choices[0]);
    EXPECT_EQ(0, result.count());
    EXPECT_EQ(0, result.uvalues[0].type);
}
