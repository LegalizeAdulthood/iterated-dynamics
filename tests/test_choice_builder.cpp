#include <choice_builder.h>

#include <port.h>
#include <prototyp.h>

#include <id.h>
#include <id_keys.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <functional>
#include <memory>

using namespace testing;

namespace
{

struct Prompter
{
    MOCK_METHOD(int, prompt,
        (char const *hdg, int numprompts, char const **prompts, fullscreenvalues *values, int fkeymask,
            char *extrainfo),
        (const));
};

using MockPrompter = StrictMock<Prompter>;

MockPrompter *g_prompter{};

struct Shim
{
    int operator()(char const *hdg, int numprompts, char const **prompts, fullscreenvalues *values, int fkeymask,
        char *extrainfo) const
    {
        return g_prompter->prompt(hdg, numprompts, prompts, values, fkeymask, extrainfo);
    }
};

template <typename T>
using ListenerPredicate = std::function<bool(MatchResultListener *listener, const T&value)>;

using FullScreenValuePredicate = ListenerPredicate<fullscreenvalues>;

template <typename T>
ListenerPredicate<T> has_all(const ListenerPredicate<T> &pred)
{
    return pred;
}

template <typename T, typename... Predicates>
ListenerPredicate<T> has_all(ListenerPredicate<T> &head, Predicates... tail)
{
    return [=](MatchResultListener *listener, const T &value)
    {
        const bool head_result = head(listener, value);
        *listener << "; ";
        const bool tail_result = has_all(tail...)(listener, value);
        return head_result && tail_result;
    };
}

struct FullScreenValueType
{
    int type;
};

MatchResultListener &operator<<(MatchResultListener &listener, const FullScreenValueType &value)
{
    if (value.type < 0x100)
    {
        return listener << "'" << static_cast<char>(value.type) << "'";
    }
    return listener << "0x100 + " << (value.type & 0xFF);
}

FullScreenValuePredicate has_type(int type)
{
    return [=](MatchResultListener *listener, const fullscreenvalues &value)
    {
        if (value.type != type)
        {
            *listener << "expected type " << FullScreenValueType{type} << ", got " << FullScreenValueType{value.type};
            return false;
        }

        *listener << "has type " << FullScreenValueType{value.type};
        return true;
    };
}

bool check_type(MatchResultListener *listener, const fullscreenvalues &value, int type)
{
    if (value.type != type)
    {
        *listener << "expected type " << FullScreenValueType{type} << ", got " << FullScreenValueType{value.type};
        return false;
    }

    *listener << "has type " << FullScreenValueType{value.type};
    return true;
}

FullScreenValuePredicate has_yes_no(bool yes)
{
    return [=](MatchResultListener *listener, const fullscreenvalues &value)
    {
        bool result{check_type(listener, value, 'y')};
        if (value.uval.ch.val != (yes ? 1 : 0))
        {
            *listener << "; expected value " << (yes ? 1 : 0) << ", got " << value.uval.ch.val;
            result = false;
        }
        else
        {
            *listener << "; has value " << value.uval.ch.val;
        }
        return result;
    };
}

FullScreenValuePredicate has_int_number(int number)
{
    return [=](MatchResultListener *listener, const fullscreenvalues &value)
    {
        bool result{check_type(listener, value, 'i')};

        if (value.uval.ival != number)
        {
            *listener << "; expected value " << number << ", got " << value.uval.ival;
            result = false;
        }
        else
        {
            *listener << "; has value " << value.uval.ival;
        }
        return result;
    };
}

FullScreenValuePredicate has_long_number(long number)
{
    return [=](MatchResultListener *listener, const fullscreenvalues &value)
    {
        bool result{check_type(listener, value, 'L')};

        if (value.uval.Lval != number)
        {
            *listener << "; expected value " << number << ", got " << value.uval.Lval;
            result = false;
        }
        else
        {
            *listener << "; has value " << value.uval.Lval;
        }
        return result;
    };
}

FullScreenValuePredicate has_float_number(float number)
{
    return [=](MatchResultListener *listener, const fullscreenvalues &value)
    {
        bool result{check_type(listener, value, 'f')};

        if (value.uval.dval != number)
        {
            *listener << "; expected value " << number << ", got " << value.uval.dval;
            result = false;
        }
        else
        {
            *listener << "; has value " << value.uval.dval;
        }
        return result;
    };
}

FullScreenValuePredicate has_double_number(double number)
{
    return [=](MatchResultListener *listener, const fullscreenvalues &value)
    {
        bool result{check_type(listener, value, 'd')};

        if (value.uval.dval != number)
        {
            *listener << "; expected value " << number << ", got " << value.uval.dval;
            result = false;
        }
        else
        {
            *listener << "; has value " << value.uval.dval;
        }
        return result;
    };
}

FullScreenValuePredicate has_int_double_number(double number)
{
    return [=](MatchResultListener *listener, const fullscreenvalues &value)
    {
        bool result{check_type(listener, value, 'D')};

        if (value.uval.dval != number)
        {
            *listener << "; expected value " << number << ", got " << value.uval.dval;
            result = false;
        }
        else
        {
            *listener << "; has value " << value.uval.dval;
        }
        return result;
    };
}

FullScreenValuePredicate has_list(int list_len, int list_value_len, const char *list[], int choice)
{
    return [=](MatchResultListener *listener, const fullscreenvalues &value)
    {
        bool result{check_type(listener, value, 'l')};

        if (value.uval.ch.llen != list_len)
        {
            *listener << "; expected list length " << list_len << ", got " << value.uval.ch.llen;
            result = false;
        }
        else
        {
            *listener << "; has list length " << value.uval.ch.llen;
        }

        if (value.uval.ch.vlen != list_value_len)
        {
            *listener << "; expected list value length " << list_value_len << ", got " << value.uval.ch.vlen;
            result = false;
        }
        else
        {
            *listener << "; has list value length " << value.uval.ch.vlen;
        }

        for (int i = 0; i < list_len; ++i)
        {
            if (value.uval.ch.list[i] == nullptr)
            {
                *listener << "; expected list[" << i << "] to be non-null";
                result = false;
            }
            else if (std::strcmp(value.uval.ch.list[i], list[i]) != 0)
            {
                *listener << "; expected list[" << i << "] '" << list[i] << "', got '" << value.uval.ch.list[i] << "'";
                result = false;
            }
            else
            {
                *listener << "; has list[" << i << "] '" << list[i] << "'";
            }
        }

        if (value.uval.ch.val != choice)
        {
            *listener << "; expected value " << choice << ", got " << value.uval.ch.val;
            result = false;
        }
        else
        {
            *listener << "; has value " << value.uval.ch.val;
        }

        return result;
    };
}

FullScreenValuePredicate has_string(const char *text)
{
    return [=](MatchResultListener *listener, const fullscreenvalues &value)
    {
        bool result{check_type(listener, value, 's')};

        if (std::strcmp(value.uval.sval, text) != 0)
        {
            *listener << "; expected '" << text << "', got '" << value.uval.sval << "'";
            result = false;
        }
        else
        {
            *listener << "; has value '" << value.uval.sval << "'";
        }
        return result;
    };
}

FullScreenValuePredicate has_string_buff(char *buff, int len)
{
    return [=](MatchResultListener *listener, const fullscreenvalues &value)
    {
        bool result{check_type(listener, value, 0x100 + len)};

        if (value.uval.sbuf != buff)
        {
            *listener << "; expected buffer " << static_cast<void *>(buff) << ", got "
                      << static_cast<void *>(value.uval.sbuf);
            result = false;
        }
        else
        {
            *listener << "; has buffer " << static_cast<void *>(value.uval.sbuf);
        }
        return result;
    };
}

MATCHER_P2(has_choice, n, str, "")
{
    if (std::strcmp(arg[n], str) != 0)
    {
        *result_listener << "Expected '" << str << "', got '" << arg[n] << "'";
        return false;
    }
    *result_listener << "has '" << str << "'";
    return true;
}

MATCHER_P2(has_value, n, predicate, "")
{
    *result_listener << "Index " << n << ' ';
    return predicate(result_listener, arg[n]);
}

class TestChoiceBuilderPrompting : public Test
{
protected:
    void SetUp() override
    {
        g_prompter = &prompter;
    }
    void TearDown() override
    {
        g_prompter = nullptr;
    }
    void expect_choice_value(const char *choice, const FullScreenValuePredicate &pred)
    {
        EXPECT_CALL(prompter, prompt(_, 1, has_choice(0, choice), has_value(0, pred), _, _))
            .WillOnce(Return(FIK_ENTER));
    }

    MockPrompter prompter;
};

} // namespace

TEST(TestFullScreenValueMatchers, hasValue)
{
    fullscreenvalues values{};
    auto always = [](MatchResultListener *, const fullscreenvalues &) { return true; };
    auto never = [](MatchResultListener *, const fullscreenvalues &) { return false; };

    EXPECT_THAT(&values, has_value(0, always));
    EXPECT_THAT(&values, Not(has_value(0, never)));
}

TEST(TestFullScreenValueMatchers, hasType)
{
    fullscreenvalues value{};
    value.type = 'f';

    EXPECT_THAT(&value, has_value(0, has_type('f')));
    EXPECT_THAT(&value, Not(has_value(0, has_type('i'))));
}

TEST(TestFullScreenValueMatchers, hasAllHead)
{
    fullscreenvalues value{};
    value.type = 'f';

    EXPECT_THAT(&value, has_value(0, has_all(has_type('f'))));
    EXPECT_THAT(&value, Not(has_value(0, has_all(has_type('i')))));
}

TEST_F(TestChoiceBuilderPrompting, yesNoChoice)
{
    ChoiceBuilder<1, Shim> builder;
    expect_choice_value("Evolution mode? (no for full screen)", has_yes_no(true));

    builder.yes_no("Evolution mode? (no for full screen)", true);
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_EQ(1, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, intChoice)
{
    ChoiceBuilder<1, Shim> builder;
    expect_choice_value("Image grid size (odd numbers only)", has_int_number(50));

    builder.int_number("Image grid size (odd numbers only)", 50);
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_EQ(1, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, longChoice)
{
    ChoiceBuilder<1, Shim> builder;
    expect_choice_value("Bailout value (0 means use default)", has_long_number(50L));

    builder.long_number("Bailout value (0 means use default)", 50L);
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_EQ(1, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, floatChoice)
{
    ChoiceBuilder<1, Shim> builder;
    expect_choice_value("x parameter range (across screen)", has_float_number(123.456f));

    builder.float_number("x parameter range (across screen)", 123.456f);
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_EQ(1, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, doubleChoice)
{
    ChoiceBuilder<1, Shim> builder;
    expect_choice_value("x parameter range (across screen)", has_double_number(123.456));

    builder.double_number("x parameter range (across screen)", 123.456);
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_EQ(1, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, intDoubleChoice)
{
    ChoiceBuilder<1, Shim> builder;
    expect_choice_value("x parameter range (across screen)", has_int_double_number(123.0));

    builder.int_double_number("x parameter range (across screen)", 123.0);
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_EQ(1, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, comment)
{
    ChoiceBuilder<1, Shim> builder;
    expect_choice_value("Press F4 to reset view parameters to defaults.", has_type('*'));

    builder.comment("Press F4 to reset view parameters to defaults.");
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_EQ(1, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, listChoice)
{
    ChoiceBuilder<1, Shim> builder;
    const char *list[]{"one", "two", "three"};
    constexpr int list_len = 3;
    constexpr int list_value_len = 5;
    expect_choice_value("First Function", has_list(list_len, list_value_len, list, 1));

    builder.list("First Function", list_len, list_value_len, list, 1);
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_EQ(1, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, stringChoice)
{
    ChoiceBuilder<1, Shim> builder;
    expect_choice_value("Browse search filename mask ", has_string("*.gif"));

    builder.string("Browse search filename mask ", "*.gif");
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_EQ(1, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, stringBufferChoice)
{
    ChoiceBuilder<1, Shim> builder;
    constexpr int len{80};
    char buff[len];
    expect_choice_value("Browse search filename mask ", has_string_buff(buff, len));

    builder.string_buff("Browse search filename mask ", buff, len);
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_EQ(1, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, multipleChoices)
{
    ChoiceBuilder<2, Shim> builder;
    EXPECT_CALL(prompter,
        prompt(_, 2, AllOf(has_choice(0, "First"), has_choice(1, "Second")),
            AllOf(has_value(0, has_type('*')), has_value(1, has_type('*'))), _, _))
        .WillOnce(Return(FIK_ENTER));

    builder.comment("First").comment("Second");
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_EQ(2, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, buildOverflow)
{
    ChoiceBuilder<1, Shim> builder;
    builder.comment("First");

    EXPECT_THROW(builder.comment("Second"), std::runtime_error);
}

MATCHER_P(has_null_choice, n, "")
{
    if (arg == nullptr)
    {
        *result_listener << "expected non-null argument";
        return false;
    }
    if (arg[n] != nullptr)
    {
        *result_listener << "expected choice[" << n << "] to be null, got " << arg[n];
        return false;
    }
    *result_listener << "choice[" << n << "] is null";
    return true;
}

TEST_F(TestChoiceBuilderPrompting, buildReset)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter,
        prompt(_, 0, AllOf(NotNull(), has_null_choice(0)), AllOf(NotNull(), has_value(0, has_type(0))), _, _))
        .WillOnce(Return(FIK_ENTER));
    builder.comment("First");

    builder.reset();
    builder.prompt("Oops");

    EXPECT_EQ(0, builder.count());
}

TEST_F(TestChoiceBuilderPrompting, promptCallsFullScreenPrompt)
{
    ChoiceBuilder<1, Shim> builder;
    char extra[20]{};
    builder.yes_no("Evolution mode? (no for full screen)", true);
    EXPECT_CALL(prompter,
        prompt(StrEq("Evolution Mode Options"), 1, NotNull(), NotNull(), 255, extra))
        .WillOnce(Return(FIK_ENTER));

    const int result = builder.prompt("Evolution Mode Options", 255, extra);

    EXPECT_EQ(FIK_ENTER, result);
}

TEST_F(TestChoiceBuilderPrompting, readChoiceWithoutPrompting)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, _, _, _, _, _)).Times(0);
    builder.yes_no("Evolution mode? (no for full screen)", true);

    EXPECT_THROW(builder.read_yes_no(), std::runtime_error);
}

TEST_F(TestChoiceBuilderPrompting, readChoiceWrongType)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, _, _, _, _, _)).WillOnce(Return(FIK_ENTER));
    builder.int_number("Image grid size (odd numbers only)", 50);
    builder.prompt("Evolution Mode Options", 255);

    EXPECT_THROW(builder.read_yes_no(), std::runtime_error);
}

TEST_F(TestChoiceBuilderPrompting, readTooManyChoices)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, _, _, _, _, _)).WillOnce(Return(FIK_ENTER));
    builder.yes_no("Evolution mode? (no for full screen)", true);
    builder.prompt("Evolution Mode Options", 255);
    EXPECT_EQ(true, builder.read_yes_no());

    EXPECT_THROW(builder.read_yes_no(), std::runtime_error);
}

TEST_F(TestChoiceBuilderPrompting, readYesNoChoice)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter,
        prompt(StrEq("Evolution Mode Options"), 1, NotNull(), NotNull(), 255, nullptr))
        .WillOnce(DoAll(WithArg<3>([](fullscreenvalues *uvalues) { uvalues[0].uval.ch.val = 0; }), Return(FIK_ENTER)));
    builder.yes_no("Evolution mode? (no for full screen)", true);
    const int result = builder.prompt("Evolution Mode Options", 255);

    const bool yes_no = builder.read_yes_no();

    EXPECT_EQ(FIK_ENTER, result);
    EXPECT_EQ(false, yes_no);
}

TEST_F(TestChoiceBuilderPrompting, readIntChoice)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, 1, _, NotNull(), _, _))
        .WillOnce(DoAll(WithArg<3>([](fullscreenvalues *uvalues) { uvalues[0].uval.ival = 12; }), Return(FIK_ENTER)));
    builder.int_number("Image grid size (odd numbers only)", 50);
    const int result = builder.prompt("Evolution Mode Options", 255);

    const int value = builder.read_int_number();

    EXPECT_EQ(FIK_ENTER, result);
    EXPECT_EQ(12, value);
}

TEST_F(TestChoiceBuilderPrompting, readLongChoice)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, 1, _, NotNull(), _, _))
        .WillOnce(DoAll(WithArg<3>([](fullscreenvalues *uvalues) { uvalues[0].uval.Lval = 46L; }), Return(FIK_ENTER)));
    builder.long_number("Image grid size (odd numbers only)", 66L);
    const int result = builder.prompt("Evolution Mode Options", 255);

    const long value = builder.read_long_number();

    EXPECT_EQ(FIK_ENTER, result);
    EXPECT_EQ(46L, value);
}

TEST_F(TestChoiceBuilderPrompting, readFloatChoice)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, 1, _, NotNull(), _, _))
        .WillOnce(DoAll(WithArg<3>([](fullscreenvalues *uvalues) { uvalues[0].uval.dval = 46.5; }), Return(FIK_ENTER)));
    builder.float_number("Image grid size (odd numbers only)", 66.0f);
    const int result = builder.prompt("Evolution Mode Options", 255);

    const float value = builder.read_float_number();

    EXPECT_EQ(FIK_ENTER, result);
    EXPECT_EQ(46.5f, value);
}

TEST_F(TestChoiceBuilderPrompting, readDoubleChoice)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, 1, _, NotNull(), _, _))
        .WillOnce(DoAll(WithArg<3>([](fullscreenvalues *uvalues) { uvalues[0].uval.dval = 46.5; }), Return(FIK_ENTER)));
    builder.double_number("Image grid size (odd numbers only)", 66.0);
    const int result = builder.prompt("Evolution Mode Options", 255);

    const double value = builder.read_double_number();

    EXPECT_EQ(FIK_ENTER, result);
    EXPECT_EQ(46.5, value);
}

TEST_F(TestChoiceBuilderPrompting, readIntDoubleChoice)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, 1, _, NotNull(), _, _))
        .WillOnce(DoAll(WithArg<3>([](fullscreenvalues *uvalues) { uvalues[0].uval.dval = 46; }), Return(FIK_ENTER)));
    builder.int_double_number("Image grid size (odd numbers only)", 66.0);
    const int result = builder.prompt("Evolution Mode Options", 255);

    const double value = builder.read_int_double_number();

    EXPECT_EQ(FIK_ENTER, result);
    EXPECT_EQ(46, value);
}

TEST_F(TestChoiceBuilderPrompting, readComment)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, _, _, _, _, _)).WillOnce(Return(FIK_ENTER));
    builder.comment("Press F4 to reset view parameters to defaults.");
    const int result = builder.prompt("Evolution Mode Options", 255);

    builder.read_comment();

    EXPECT_EQ(FIK_ENTER, result);
}

TEST_F(TestChoiceBuilderPrompting, readList)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, 1, NotNull(), _, _, _))
        .WillOnce(DoAll(WithArg<3>([](fullscreenvalues *uvalues) { uvalues[0].uval.ch.val = 0; }), Return(FIK_ENTER)));
    const char *list[]{"one", "two", "three"};
    const int list_len = 3;
    const int list_value_len = 5;
    builder.list("First Function", list_len, list_value_len, list, 1);
    const int result = builder.prompt("Evolution Mode Options", 255);

    const int value = builder.read_list();

    EXPECT_EQ(FIK_ENTER, result);
    EXPECT_EQ(0, value);
}

TEST_F(TestChoiceBuilderPrompting, readString)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, 1, NotNull(), _, _, _))
        .WillOnce(DoAll(WithArg<3>([](fullscreenvalues *uvalues) { std::strncpy(uvalues[0].uval.sval, "*.pot", 16); }),
            Return(FIK_ENTER)));
    builder.string("Browse search filename mask ", "*.gif");
    const int result = builder.prompt("Evolution Mode Options", 255);

    const char *value = builder.read_string();

    EXPECT_EQ(FIK_ENTER, result);
    EXPECT_STREQ("*.pot", value);
}

TEST_F(TestChoiceBuilderPrompting, readStringBuff)
{
    ChoiceBuilder<1, Shim> builder;
    EXPECT_CALL(prompter, prompt(_, 1, NotNull(), _, _, _))
        .WillOnce(DoAll(WithArg<3>([](fullscreenvalues *uvalues) { std::strcpy(uvalues[0].uval.sbuf, "*.pot"); }),
            Return(FIK_ENTER)));
    constexpr int len{80};
    char buff[len]{};
    builder.string_buff("Browse search filename mask ", buff, len);
    const int result = builder.prompt("Evolution Mode Options", 255);

    const char *value = builder.read_string_buff(len);

    EXPECT_EQ(FIK_ENTER, result);
    EXPECT_STREQ("*.pot", value);
    EXPECT_STREQ("*.pot", buff);
}
