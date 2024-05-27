#include <cmdfiles_test.h>

#include "current_path_saver.h"
#include "test_data.h"

#include <history.h>
#include <id.h>
#include <stop_msg.h>
#include <value_saver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <rotate.h>

using namespace testing;

class TestParameterCommand : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    int exec_cmd_arg(const char *curarg, cmd_file mode);
    int exec_cmd_arg(const std::string &curarg, cmd_file mode)
    {
        return exec_cmd_arg(curarg.c_str(), mode);
    }
    void expect_stop_msg();

    StrictMock<MockFunction<cmd_arg::StopMsg>> m_stop_msg;
    StrictMock<MockFunction<cmd_arg::Goodbye>> m_goodbye;
    StrictMock<MockFunction<cmd_arg::PrintDoc>> m_print_document;
    cmd_arg::StopMsgFn m_prev_stop_msg;
    cmd_arg::GoodbyeFn m_prev_goodbye;
    cmd_arg::PrintDocFn m_prev_print_document;
    char m_buffer[FILE_MAX_PATH*2]{};
};

void TestParameterCommand::SetUp()
{
    Test::SetUp();
    m_prev_stop_msg = cmd_arg::get_stop_msg();
    m_prev_goodbye = cmd_arg::get_goodbye();
    m_prev_print_document = cmd_arg::get_print_document();
    cmd_arg::set_stop_msg(m_stop_msg.AsStdFunction());
    cmd_arg::set_goodbye(m_goodbye.AsStdFunction());
    cmd_arg::set_print_document(m_print_document.AsStdFunction());
}

void TestParameterCommand::TearDown()
{
    cmd_arg::set_print_document(m_prev_print_document);
    cmd_arg::set_goodbye(m_prev_goodbye);
    cmd_arg::set_stop_msg(m_prev_stop_msg);
    Test::TearDown();
}

int TestParameterCommand::exec_cmd_arg(const char *curarg, cmd_file mode)
{
    std::strcpy(m_buffer, curarg);
    return cmdarg(m_buffer, mode);
}

void TestParameterCommand::expect_stop_msg()
{
    EXPECT_CALL(m_stop_msg, Call(STOPMSG_NONE, _)).WillOnce(Return(false));
}

TEST_F(TestParameterCommand, parameterTooLong)
{
    expect_stop_msg();

    EXPECT_EQ(CMDARG_ERROR, exec_cmd_arg("maximumoftwentycharactersinparametername", cmd_file::SSTOOLS_INI));
}

TEST_F(TestParameterCommand, batchBadArg)
{
    expect_stop_msg();

    EXPECT_EQ(CMDARG_ERROR, exec_cmd_arg("batch=g", cmd_file::SSTOOLS_INI));
}

TEST_F(TestParameterCommand, batchYes)
{
    ValueSaver saved_init_batch(g_init_batch, batch_modes::NONE);

    const int result = exec_cmd_arg("batch=yes", cmd_file::SSTOOLS_INI);

    EXPECT_EQ(CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM, result);
    EXPECT_EQ(batch_modes::NORMAL, g_init_batch);
}

TEST_F(TestParameterCommand, batchNo)
{
    ValueSaver saved_init_batch(g_init_batch, batch_modes::NORMAL);

    const int result = exec_cmd_arg("batch=no", cmd_file::SSTOOLS_INI);

    EXPECT_EQ(CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM, result);
    EXPECT_EQ(batch_modes::NONE, g_init_batch);
}

TEST_F(TestParameterCommand, batchAfterStartup)
{
    ValueSaver saved_init_batch(g_init_batch, batch_modes::NONE);
    expect_stop_msg();

    EXPECT_EQ(CMDARG_ERROR, exec_cmd_arg("batch=yes", cmd_file::AT_AFTER_STARTUP));
}

TEST_F(TestParameterCommand, maxHistoryNonNumeric)
{
    ValueSaver saved_max_image_history(g_max_image_history, 0);
    expect_stop_msg();

    EXPECT_EQ(CMDARG_ERROR, exec_cmd_arg("maxhistory=yes", cmd_file::SSTOOLS_INI));
}

TEST_F(TestParameterCommand, maxHistoryNegative)
{
    ValueSaver saved_max_image_history(g_max_image_history, 0);
    expect_stop_msg();

    EXPECT_EQ(CMDARG_ERROR, exec_cmd_arg("maxhistory=-10", cmd_file::SSTOOLS_INI));
}

TEST_F(TestParameterCommand, maxHistory)
{
    ValueSaver saved_max_image_history(g_max_image_history, 0);

    const int result = exec_cmd_arg("maxhistory=10", cmd_file::SSTOOLS_INI);

    EXPECT_EQ(CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM, result);
    EXPECT_EQ(10, g_max_image_history);
}

TEST_F(TestParameterCommand, maxHistoryAfterStartup)
{
    ValueSaver saved_max_image_history(g_max_image_history, 0);
    expect_stop_msg();

    EXPECT_EQ(CMDARG_ERROR, exec_cmd_arg("maxhistory=10", cmd_file::AT_AFTER_STARTUP));
}

TEST_F(TestParameterCommand, makeDocDefaultFile)
{
    EXPECT_CALL(m_print_document, Call(StrEq("id.txt"), NotNull()));
    EXPECT_CALL(m_goodbye, Call());

    EXPECT_EQ(CMDARG_GOODBYE, exec_cmd_arg("makedoc", cmd_file::SSTOOLS_INI));
}

TEST_F(TestParameterCommand, makeDocCustomFile)
{
    EXPECT_CALL(m_print_document, Call(StrEq("foo.txt"), NotNull()));
    EXPECT_CALL(m_goodbye, Call());

    EXPECT_EQ(CMDARG_GOODBYE, exec_cmd_arg("makedoc=foo.txt", cmd_file::SSTOOLS_INI));
}

TEST_F(TestParameterCommand, makeParTooFewValues)
{
    expect_stop_msg();

    EXPECT_EQ(CMDARG_ERROR, exec_cmd_arg("makepar", cmd_file::SSTOOLS_INI));
}

TEST_F(TestParameterCommand, makeParTooManyValues)
{
    expect_stop_msg();

    EXPECT_EQ(CMDARG_ERROR, exec_cmd_arg("makepar=foo/bar/fmeh", cmd_file::SSTOOLS_INI));
}

// TODO: test makepar with valid arguments

TEST_F(TestParameterCommand, resetBadArg)
{
    ValueSaver saved_escape_exit(g_escape_exit, true);
    expect_stop_msg();

    const int result = exec_cmd_arg("reset=foo", cmd_file::AT_AFTER_STARTUP);

    EXPECT_EQ(CMDARG_ERROR, result);
    EXPECT_TRUE(g_escape_exit);
}

TEST_F(TestParameterCommand, filenameExtensionTooLong)
{
    ValueSaver saved_gif_filename_mask(g_gif_filename_mask, "*.pot");
    expect_stop_msg();

    const int result = exec_cmd_arg("filename=.foobar", cmd_file::AT_AFTER_STARTUP);

    EXPECT_EQ(CMDARG_ERROR, result);
    EXPECT_EQ("*.pot", g_gif_filename_mask);
}

TEST_F(TestParameterCommand, filenameExtension)
{
    ValueSaver saved_gif_filename_mask(g_gif_filename_mask, "*.pot");

    const int result = exec_cmd_arg("filename=.gif", cmd_file::AT_AFTER_STARTUP);

    EXPECT_EQ(CMDARG_NONE, result);
    EXPECT_EQ("*.gif", g_gif_filename_mask);
}

TEST_F(TestParameterCommand, filenameValueTooLong)
{
    ValueSaver saved_gif_filename_mask(g_gif_filename_mask, "*.pot");
    expect_stop_msg();
    const std::string too_long{"filename=" + std::string(FILE_MAX_PATH, 'f') + ".gif"};

    const int result = exec_cmd_arg(too_long, cmd_file::AT_AFTER_STARTUP);

    EXPECT_EQ(CMDARG_ERROR, result);
    EXPECT_EQ("*.pot", g_gif_filename_mask);
}

TEST_F(TestParameterCommand, mapTooLong)
{
    ValueSaver saved_map_name(g_map_name, "foo.map");
    expect_stop_msg();
    const std::string too_long{"map=" + std::string(FILE_MAX_PATH, 'f') + ".map"};

    const int result = exec_cmd_arg(too_long, cmd_file::SSTOOLS_INI);

    EXPECT_EQ(CMDARG_ERROR, result);
    EXPECT_EQ("foo.map", g_map_name);
}

TEST_F(TestParameterCommand, mapSpecifiesSubdir)
{
    ValueSaver saved_map_name(g_map_name, "foo.map");
    current_path_saver cur_dir(ID_TEST_HOME_DIR);

    const int result = exec_cmd_arg("map=" ID_TEST_MAP_SUBDIR, cmd_file::SSTOOLS_INI);

    EXPECT_EQ(CMDARG_NONE, result);
    EXPECT_EQ(ID_TEST_MAP_SUBDIR SLASH "foo.map", g_map_name);
}

TEST_F(TestParameterCommand, mapSpecifiesExistingFile)
{
    ValueSaver saved_map_name(g_map_name, "foo.map");
    current_path_saver cur_dir(ID_TEST_HOME_DIR);

    const int result = exec_cmd_arg("map=" ID_TEST_MAP_SUBDIR SLASH ID_TEST_MAP_FILE, cmd_file::SSTOOLS_INI);

    EXPECT_EQ(CMDARG_NONE, result);
    EXPECT_EQ(ID_TEST_MAP_SUBDIR SLASH ID_TEST_MAP_FILE, g_map_name);
}
