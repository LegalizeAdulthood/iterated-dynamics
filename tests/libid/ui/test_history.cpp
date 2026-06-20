// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/history.h>

#include "MockDriver.h"
#include "test_data.h"

#include <fractals/fractalp.h>
#include <io/CurrentPathSaver.h>
#include <io/library.h>
#include <io/loadfile.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::test::data;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestHistory : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    MockDriver m_driver;
    ValueSaver<int> m_saved_max_image_history{g_max_image_history, 1};
    ValueSaver<int> m_saved_history_ptr{g_history_ptr, -1};
    ValueSaver<bool> m_saved_history_flag{g_history_flag, false};
    ValueSaver<Version> m_saved_version{g_version};
    ValueSaver<Version> m_saved_file_version{g_file_version};
    ValueSaver<FractalType> m_saved_fractal_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<const FractalSpecific *> m_saved_fractal_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<DebugFlags> m_saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    CurrentPathSaver m_current_path{ID_TEST_DATA_DIR};
};

void TestHistory::SetUp()
{
    clear_save_library();
    set_save_library(ID_TEST_DATA_DIR);
    history_init();
}

void TestHistory::TearDown()
{
    clear_save_library();
}

static std::string read_file_contents(const std::filesystem::path &path)
{
    const std::ifstream file{path};
    std::ostringstream str;
    str << file.rdbuf();
    return str.str();
}

TEST_F(TestHistory, restoresVersionAndFileVersion)
{
    g_version = parse_legacy_version(1730);
    g_file_version = parse_legacy_version(2004);
    save_history_info();
    g_version = current_id_version();
    g_file_version = current_id_version();

    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, delay(255));

    restore_history_info(g_history_ptr);

    EXPECT_EQ(parse_legacy_version(1730), g_version);
    EXPECT_EQ(parse_legacy_version(2004), g_file_version);
}

TEST_F(TestHistory, dumpsVersionAndFileVersion)
{
    const std::filesystem::path history_file{std::filesystem::path{ID_TEST_DATA_DIR} / "debug/history.json"};
    std::filesystem::remove(history_file);
    g_debug_flag = DebugFlags::HISTORY_DUMP_JSON;
    g_version = Version{1, 2, 3, 4, false};
    g_file_version = parse_legacy_version(2004);

    save_history_info();

    const std::string text{read_file_contents(history_file)};
    EXPECT_THAT(text, HasSubstr(R"json("version":{"major":1,"minor":2,"patch":3,"tweak":4,"legacy":0})json"));
    EXPECT_THAT(text, HasSubstr(R"json("file_version":{"major":20,"minor":4,"patch":0,"tweak":0,"legacy":1})json"));
    EXPECT_THAT(text, Not(HasSubstr(R"json("release":)json")));
}

} // namespace id::test
