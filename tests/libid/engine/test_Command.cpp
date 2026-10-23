// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/Command.h>

#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

using namespace id::engine;
using namespace id::misc;
using namespace testing;

namespace id::test
{

class TestCommand : public Test
{
protected:
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    CommandParseResult m_result;
};

TEST_F(TestCommand, retainsContextAndResultDestination)
{
    const Command cmd{"MAXITER=150", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_EQ("maxiter", cmd.variable);
    EXPECT_EQ("150", cmd.value);
    EXPECT_EQ(CmdFile::AT_AFTER_STARTUP, cmd.mode);
    EXPECT_EQ(&m_result, &cmd.result);
    EXPECT_EQ(CmdArgFlags::NONE, cmd.status);
    EXPECT_EQ(CmdArgFlags::NONE, m_result.flags);
    EXPECT_TRUE(m_result.diagnostics.errors.empty());
    EXPECT_TRUE(m_result.diagnostics.warnings.empty());
}

TEST_F(TestCommand, decodesIntegerFloatingAndTextFields)
{
    const Command cmd{"PARAMS=+12/-34/0.125/1E3/garbage", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(12, cmd.num_val);
    EXPECT_EQ(5, cmd.total_params);
    EXPECT_EQ(2, cmd.num_int_params);
    EXPECT_EQ(4, cmd.num_float_params);
    EXPECT_EQ(12, cmd.int_vals[0]);
    EXPECT_EQ(-34, cmd.int_vals[1]);
    EXPECT_TRUE(cmd.has_val[0]);
    EXPECT_TRUE(cmd.has_val[1]);
    EXPECT_FALSE(cmd.has_val[2]);
    EXPECT_FALSE(cmd.has_val[3]);
    EXPECT_FALSE(cmd.has_val[4]);
    EXPECT_DOUBLE_EQ(0.125, cmd.float_vals[2]);
    EXPECT_DOUBLE_EQ(1000.0, cmd.float_vals[3]);
    EXPECT_EQ('+', cmd.char_val[0]);
    EXPECT_EQ("+12", cmd.string_val(0));
    EXPECT_EQ("-34", cmd.string_val(1));
    EXPECT_EQ("1e3", cmd.string_val(3));
    EXPECT_EQ("garbage", cmd.string_val(4));
    EXPECT_EQ(CmdArgFlags::NONE, cmd.status);
}

TEST_F(TestCommand, retainsLeadingSpaceAndPlusNumericRecognition)
{
    const Command cmd{"params= +12/ -34/ +0.125", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(12, cmd.num_val);
    EXPECT_EQ(2, cmd.num_int_params);
    EXPECT_EQ(3, cmd.num_float_params);
    EXPECT_EQ(-34, cmd.int_vals[1]);
    EXPECT_DOUBLE_EQ(0.125, cmd.float_vals[2]);
    EXPECT_EQ(" +12", cmd.string_val(0));
    EXPECT_EQ(" +0.125", cmd.string_val(2));
}

TEST_F(TestCommand, decodesYesNoFields)
{
    const Command cmd{"sound=YES/NO", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(1, cmd.yes_no_val[0]);
    EXPECT_EQ(0, cmd.yes_no_val[1]);
    EXPECT_EQ('y', cmd.char_val[0]);
    EXPECT_EQ('n', cmd.char_val[1]);
    EXPECT_EQ("yes", cmd.string_val(0));
    EXPECT_EQ("no", cmd.string_val(1));
}

TEST_F(TestCommand, absentAndEmptyValuesHaveNoFields)
{
    for (const char *text : {"reset", "reset="})
    {
        SCOPED_TRACE(text);
        const Command cmd{text, CmdFile::AT_CMD_LINE, m_result};

        EXPECT_EQ("reset", cmd.variable);
        EXPECT_TRUE(cmd.value.empty());
        EXPECT_EQ(0, cmd.num_val);
        EXPECT_EQ(0, cmd.total_params);
        EXPECT_EQ(0, cmd.num_int_params);
        EXPECT_EQ(0, cmd.num_float_params);
        EXPECT_EQ('\0', cmd.char_val[0]);
        EXPECT_EQ(-1, cmd.yes_no_val[0]);
        EXPECT_EQ(CmdArgFlags::NONE, cmd.status);
    }
}

TEST_F(TestCommand, retainsEmptyInteriorFieldClassification)
{
    const Command cmd{"params=1//3", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(3, cmd.total_params);
    EXPECT_EQ(2, cmd.num_int_params);
    EXPECT_EQ(3, cmd.num_float_params);
    EXPECT_TRUE(cmd.string_val(1).empty());
    EXPECT_FALSE(cmd.has_val[1]);
    EXPECT_DOUBLE_EQ(std::numeric_limits<float>::max(), cmd.float_vals[1]);
    EXPECT_EQ(3, cmd.int_vals[2]);
}

TEST_F(TestCommand, retainsOmittedZeroBeforeRepeatedSlashes)
{
    const Command cmd{"params=//2", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(0, cmd.num_val);
    EXPECT_EQ(3, cmd.total_params);
    EXPECT_EQ(2, cmd.num_int_params);
    EXPECT_EQ(3, cmd.num_float_params);
    EXPECT_TRUE(cmd.string_val(0).empty());
    EXPECT_TRUE(cmd.has_val[0]);
    EXPECT_EQ(0, cmd.int_vals[0]);
    EXPECT_DOUBLE_EQ(0.0, cmd.float_vals[0]);
    EXPECT_STREQ("0", cmd.float_val_strs[0]);
    EXPECT_FALSE(cmd.has_val[1]);
    EXPECT_EQ(2, cmd.int_vals[2]);
}

TEST_F(TestCommand, trailingSlashDoesNotAddAField)
{
    const Command cmd{"params=1/2/", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(2, cmd.total_params);
    EXPECT_EQ(2, cmd.num_int_params);
    EXPECT_EQ(2, cmd.num_float_params);
    EXPECT_EQ("2", cmd.string_val(1));
}

TEST_F(TestCommand, nonnumericValueDoesNotRejectSyntax)
{
    const Command cmd{"maxiter=garbage", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(NON_NUMERIC, cmd.num_val);
    EXPECT_EQ(0, cmd.num_int_params);
    EXPECT_EQ(0, cmd.num_float_params);
    EXPECT_EQ(-1, cmd.yes_no_val[0]);
    EXPECT_EQ(CmdArgFlags::NONE, cmd.status);
    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestCommand, retainsExactNumericTextForPrecisionConversion)
{
    constexpr std::string_view first{"1.23456789012345678901234567890123456789"};
    constexpr std::string_view second{"2.50000000000000000000000000001"};
    const Command cmd{"params=" + std::string{first} + '/' + std::string{second}, CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(first, cmd.string_val(0));
    EXPECT_EQ(second, cmd.string_val(1));
    EXPECT_EQ(cmd.value.data(), cmd.float_val_strs[0]);
    EXPECT_EQ(cmd.value.data() + first.size() + 1, cmd.float_val_strs[1]);
    EXPECT_EQ(2, cmd.num_float_params);
}

TEST_F(TestCommand, recognizesNumbersOutsideDoubleRangeWithoutConvertingPrecision)
{
    const Command cmd{"params=1e1000/-2e1000", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(NON_NUMERIC, cmd.num_val);
    EXPECT_EQ(0, cmd.num_int_params);
    EXPECT_EQ(2, cmd.num_float_params);
    EXPECT_DOUBLE_EQ(std::numeric_limits<float>::max(), cmd.float_vals[0]);
    EXPECT_DOUBLE_EQ(std::numeric_limits<float>::max(), cmd.float_vals[1]);
    EXPECT_EQ("1e1000", cmd.string_val(0));
    EXPECT_EQ("-2e1000", cmd.string_val(1));
    EXPECT_EQ(cmd.value.data(), cmd.float_val_strs[0]);
    EXPECT_EQ(cmd.value.data() + 7, cmd.float_val_strs[1]);
    EXPECT_EQ(CmdArgFlags::NONE, cmd.status);
}

TEST_F(TestCommand, countsFieldsBeyondStoredArrayLimits)
{
    std::string text{"params="};
    for (int i = 0; i < 65; ++i)
    {
        if (i != 0)
        {
            text += '/';
        }
        text += std::to_string(i);
    }
    const Command cmd{text, CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(65, cmd.total_params);
    EXPECT_EQ(65, cmd.num_int_params);
    EXPECT_EQ(65, cmd.num_float_params);
    EXPECT_EQ(63, cmd.int_vals[63]);
    EXPECT_TRUE(cmd.has_val[63]);
    EXPECT_DOUBLE_EQ(15.0, cmd.float_vals[15]);
    EXPECT_EQ("15", cmd.string_val(15));
    EXPECT_EQ(CmdArgFlags::NONE, cmd.status);
}

TEST_F(TestCommand, ownsTextIndependentlyOfSourceChanges)
{
    std::string text{"params=+00012/3.5"};
    const Command cmd{text, CmdFile::AT_CMD_LINE, m_result};

    text.assign(4096, 'x');

    EXPECT_EQ("+00012/3.5", cmd.value);
    EXPECT_EQ("+00012", cmd.string_val(0));
    EXPECT_EQ("3.5", cmd.string_val(1));
    EXPECT_EQ(cmd.value.data(), cmd.float_val_strs[0]);
    EXPECT_STREQ("3.5", cmd.float_val_strs[1]);
}

TEST_F(TestCommand, ownsTemporarySourceText)
{
    const Command cmd{std::string{"params=+00012/3.5"}, CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ("+00012/3.5", cmd.value);
    EXPECT_EQ("+00012", cmd.string_val(0));
    EXPECT_STREQ("3.5", cmd.float_val_strs[1]);
}

TEST_F(TestCommand, honorsSourceStringViewLength)
{
    const std::string text{"params=12/34unused"};
    const std::string_view argument{text.data(), text.find("unused")};
    const Command cmd{argument, CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ("12/34", cmd.value);
    EXPECT_EQ(2, cmd.total_params);
    EXPECT_EQ(34, cmd.int_vals[1]);
    EXPECT_STREQ("34", cmd.float_val_strs[1]);
}

TEST_F(TestCommand, preservesCaseSensitiveValueDuringNormalization)
{
    const Command cmd{"RDS-TEXTURE=Textures/Mixed.GIF", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ("rds-texture", cmd.variable);
    EXPECT_EQ("Textures/Mixed.GIF", cmd.value);
    EXPECT_EQ("Mixed.GIF", cmd.string_val(1));
}

TEST_F(TestCommand, acceptsMaximumKeywordLength)
{
    const Command cmd{std::string(32, 'a') + "=1", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(std::string(32, 'a'), cmd.variable);
    EXPECT_EQ(CmdArgFlags::NONE, cmd.status);
    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestCommand, excessiveKeywordLengthReportsOneSyntaxDiagnostic)
{
    const std::string text{std::string(33, 'a') + "=1"};
    const Command cmd{text, CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, cmd.status);
    ASSERT_THAT(m_result.diagnostics.errors, SizeIs(1));
    EXPECT_EQ("Oops. I couldn't understand the argument:\n  " + text, m_result.diagnostics.errors.front());
    EXPECT_EQ(CmdArgFlags::NONE, m_result.flags);
}

TEST_F(TestCommand, badArgumentRetainsDiagnosticTruncation)
{
    const std::string text{"unknown=" + std::string(90, 'x')};
    const Command cmd{text, CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, cmd.bad_arg());

    ASSERT_THAT(m_result.diagnostics.errors, SizeIs(1));
    EXPECT_EQ(
        "Oops. I couldn't understand the argument:\n  " + text.substr(0, 70), m_result.diagnostics.errors.front());
    EXPECT_EQ(CmdArgFlags::NONE, m_result.flags);
}

TEST_F(TestCommand, startupBadArgumentRetainsHelpSuffix)
{
    const ValueSaver first_init{g_first_init, true};
    const Command cmd{"unknown=value", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, cmd.bad_arg());

    ASSERT_THAT(m_result.diagnostics.errors, SizeIs(1));
    EXPECT_THAT(m_result.diagnostics.errors.front(), HasSubstr("see the Startup Help screens"));
}

TEST_F(TestCommand, missingFileWarningRetainsSourceContext)
{
    struct Context
    {
        CmdFile mode;
        const char *name;
    };

    constexpr Context contexts[]{{CmdFile::AT_CMD_LINE, "command line"}, {CmdFile::SSTOOLS_INI, "sstools.ini"},
        {CmdFile::AT_AFTER_STARTUP, "PAR file"}, {CmdFile::AT_CMD_LINE_SET_NAME, "PAR file"}};
    for (const Context &context : contexts)
    {
        SCOPED_TRACE(context.name);
        CommandParseResult result;
        const Command cmd{"IFSFILE=Mixed.IFS", context.mode, result};

        cmd.warn_missing_file();

        ASSERT_THAT(result.diagnostics.warnings, SizeIs(1));
        EXPECT_EQ("Can't find ifsfile=Mixed.IFS, please check " + std::string{context.name},
            result.diagnostics.warnings.front());
        EXPECT_TRUE(result.diagnostics.errors.empty());
    }
}

TEST_F(TestCommand, cannotCopyOrMoveInternalViews)
{
    EXPECT_FALSE(std::is_copy_constructible_v<Command>);
    EXPECT_FALSE(std::is_move_constructible_v<Command>);
    EXPECT_FALSE(std::is_copy_assignable_v<Command>);
    EXPECT_FALSE(std::is_move_assignable_v<Command>);
}

} // namespace id::test
