// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/ifs.h>

#include "MockDriver.h"
#include "test_data.h"

#include <engine/cmdfiles.h>
#include <io/library.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::test::data;
using namespace testing;

namespace id::test
{

class TestIFSLoad : public Test
{
protected:
    void SetUp() override
    {
        m_path = std::filesystem::path{ID_TEST_DATA_DIR} /
            ("ifs-" + std::string{UnitTest::GetInstance()->current_test_info()->name()} + ".ifs");
        g_ifs_filename = m_path;
        g_ifs_name = "entry";
        g_ifs_definition = {99.0f};
        g_num_affine_transforms = 9;
        g_ifs_dim = IFSDimension::THREE;
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
    }

    void TearDown() override
    {
        std::error_code ignored;
        std::filesystem::remove(m_path, ignored);
    }

    void write_definition(const std::string &text)
    {
        std::ofstream out{m_path};
        ASSERT_TRUE(out);
        out << text;
    }

    void expect_failure(const char *message)
    {
        try
        {
            ifs_load();
            FAIL() << "Expected a loader diagnostic";
        }
        catch (const std::runtime_error &error)
        {
            EXPECT_STREQ(message, error.what());
        }
        EXPECT_TRUE(g_ifs_definition.empty());
        EXPECT_EQ(0, g_num_affine_transforms);
        EXPECT_EQ(IFSDimension::TWO, g_ifs_dim);
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<std::filesystem::path> m_saved_filename{g_ifs_filename};
    ValueSaver<std::string> m_saved_name{g_ifs_name};
    ValueSaver<std::vector<float>> m_saved_definition{g_ifs_definition};
    ValueSaver<int> m_saved_count{g_num_affine_transforms};
    ValueSaver<IFSDimension> m_saved_dimension{g_ifs_dim};
    ValueSaver<std::filesystem::path> m_saved_parameter_file{g_parameter_file, std::filesystem::path{}};
    ValueSaver<bool> m_saved_search_current{g_check_cur_dir, false};
    std::filesystem::path m_path;
};

TEST_F(TestIFSLoad, loadsTwoDimensionalTransformsAndComments)
{
    write_definition("entry { ; header\n; body\n0.5 0 0 0.5 -2 3 1 ; transform\n}\n");

    ifs_load();

    EXPECT_THAT(g_ifs_definition, ElementsAre(0.5f, 0.0f, 0.0f, 0.5f, -2.0f, 3.0f, 1.0f));
    EXPECT_EQ(1, g_num_affine_transforms);
    EXPECT_EQ(IFSDimension::TWO, g_ifs_dim);
}

TEST_F(TestIFSLoad, loadsThreeDimensionalTransforms)
{
    write_definition("entry (3D) {\n1 0 0 0 1 0 0 0 1 0 0 0 1\n}\n");

    ifs_load();

    EXPECT_EQ(13U, g_ifs_definition.size());
    EXPECT_EQ(1, g_num_affine_transforms);
    EXPECT_EQ(IFSDimension::THREE, g_ifs_dim);
}

TEST_F(TestIFSLoad, loadsEmbeddedParameterFileDefinition)
{
    write_definition("ifs:entry {\n0.5 0 0 0.5 0 0 1\n}\n");
    g_parameter_file = m_path;
    g_ifs_filename = m_path.parent_path() / "missing-ifs-file.ifs";

    ifs_load();

    EXPECT_EQ(m_path, g_ifs_filename);
    EXPECT_EQ(1, g_num_affine_transforms);
    EXPECT_EQ(7U, g_ifs_definition.size());
}

TEST_F(TestIFSLoad, reportsMissingFileWithoutPolling)
{
    expect_failure("'entry' file entry item not found");
}

TEST_F(TestIFSLoad, reportsMissingEntryWithoutPolling)
{
    write_definition("other {\n0.5 0 0 0.5 0 0 1\n}\n");

    expect_failure("'entry' file entry item not found");
}

TEST_F(TestIFSLoad, reportsEmptyEntryNameWithoutPolling)
{
    g_ifs_name.clear();

    expect_failure("File entry item name is empty");
}

TEST_F(TestIFSLoad, reportsUnterminatedDefinitionWithoutPolling)
{
    write_definition("entry {\n0.5 0 0 0.5 0 0 1\n");

    expect_failure("'entry' file entry item not found");
}

TEST_F(TestIFSLoad, reportsEmptyDefinitionWithoutPolling)
{
    write_definition("entry {\n; no transforms\n}\n");

    expect_failure("Empty IFS definition");
}

TEST_F(TestIFSLoad, rejectsMalformedAndNonFiniteCoefficients)
{
    for (const char *value : {"bad", "1junk", "nan", "inf", "1e999"})
    {
        SCOPED_TRACE(value);
        write_definition("entry {\n0.5 0 0 0.5 0 0 " + std::string{value} + "\n}\n");

        expect_failure("invalid IFS definition");
    }
}

TEST_F(TestIFSLoad, rejectsIncompleteTransformWithoutPublishingPartialData)
{
    write_definition("entry {\n0.5 0 0 0.5 0 0 1\n0.5 0\n}\n");

    expect_failure("invalid IFS definition");
}

TEST_F(TestIFSLoad, rejectsIncompleteThreeDimensionalTransform)
{
    write_definition("entry (3d) {\n1 0 0 0 1 0 0 0 1 0 0 0\n}\n");

    expect_failure("invalid IFS definition");
}

TEST_F(TestIFSLoad, preservesExistingTransformLimit)
{
    std::string text{"entry {\n"};
    for (int row = 0; row < 63; ++row)
    {
        text += "0.5 0 0 0.5 0 0 1\n";
    }
    write_definition(text + "}\n");
    ifs_load();
    EXPECT_EQ(63, g_num_affine_transforms);

    write_definition(text + "0.5 0 0 0.5 0 0 1\n}\n");

    expect_failure("IFS definition has too many lines");
}

TEST_F(TestIFSLoad, closesRejectedDefinitionAndCanLoadAReplacement)
{
    write_definition("entry {\n0.5 0\n}\n");
    expect_failure("invalid IFS definition");
    const std::filesystem::path closed{m_path.string() + ".closed"};
    std::error_code error;

    std::filesystem::rename(m_path, closed, error);

    ASSERT_FALSE(error) << error.message();
    std::filesystem::remove(closed);
    write_definition("entry {\n0.5 0 0 0.5 0 0 1\n}\n");
    ifs_load();
    EXPECT_EQ(1, g_num_affine_transforms);
    EXPECT_EQ(7U, g_ifs_definition.size());
}

} // namespace id::test
