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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>
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
    void SetUp() override;

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

void TestIFSLoad::SetUp()
{
    std::string name{UnitTest::GetInstance()->current_test_info()->name()};
    std::replace(name.begin(), name.end(), '/', '-');
    m_path = std::filesystem::path{ID_TEST_DATA_DIR} / ("ifs-" + name + ".ifs");
    g_ifs_filename = m_path;
    g_ifs_name = "entry";
    g_ifs_definition = {99.0f};
    g_num_affine_transforms = 9;
    g_ifs_dim = IFSDimension::THREE;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
}

struct ValidIFSInput
{
    const char *name;
    const char *definition;
    IFSDimension dimension;
    std::size_t size;
    std::optional<std::vector<float>> values;
};

void PrintTo(const ValidIFSInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestValidIFSLoad : public TestIFSLoad, public WithParamInterface<ValidIFSInput>
{
protected:
    void SetUp() override;
};

void TestValidIFSLoad::SetUp()
{
    TestIFSLoad::SetUp();
    write_definition(GetParam().definition);
}

TEST_P(TestValidIFSLoad, publishesDefinition)
{
    ifs_load();

    EXPECT_EQ(GetParam().size, g_ifs_definition.size());
    if (GetParam().values)
    {
        EXPECT_THAT(g_ifs_definition, ElementsAreArray(*GetParam().values));
    }
    EXPECT_EQ(1, g_num_affine_transforms);
    EXPECT_EQ(GetParam().dimension, g_ifs_dim);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestValidIFSLoad,
    Values(ValidIFSInput{"TwoDimensionalComments", "entry { ; header\n; body\n0.5 0 0 0.5 -2 3 1 ; transform\n}\n",
               IFSDimension::TWO, 7, std::vector<float>{0.5f, 0.0f, 0.0f, 0.5f, -2.0f, 3.0f, 1.0f}},
        ValidIFSInput{
            "ThreeDimensional", "entry (3D) {\n1 0 0 0 1 0 0 0 1 0 0 0 1\n}\n", IFSDimension::THREE, 13, std::nullopt}),
    [](const TestParamInfo<ValidIFSInput> &info) { return info.param.name; });

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

struct InvalidIFSInput
{
    const char *name;
    std::optional<std::string> definition;
    const char *entry;
    const char *message;
};

void PrintTo(const InvalidIFSInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<InvalidIFSInput> invalid_ifs_inputs()
{
    std::vector<InvalidIFSInput> inputs{{"MissingFile", std::nullopt, "entry", "'entry' file entry item not found"},
        {"MissingEntry", "other {\n0.5 0 0 0.5 0 0 1\n}\n", "entry", "'entry' file entry item not found"},
        {"EmptyEntryName", std::nullopt, "", "File entry item name is empty"},
        {"UnterminatedDefinition", "entry {\n0.5 0 0 0.5 0 0 1\n", "entry", "'entry' file entry item not found"},
        {"EmptyDefinition", "entry {\n; no transforms\n}\n", "entry", "Empty IFS definition"},
        {"IncompleteTransform", "entry {\n0.5 0 0 0.5 0 0 1\n0.5 0\n}\n", "entry", "invalid IFS definition"},
        {"IncompleteThreeDimensional", "entry (3d) {\n1 0 0 0 1 0 0 0 1 0 0 0\n}\n", "entry",
            "invalid IFS definition"}};

    struct Coefficient
    {
        const char *name;
        const char *value;
    };

    for (const Coefficient &coefficient : {Coefficient{"Malformed", "bad"}, Coefficient{"TrailingJunk", "1junk"},
             Coefficient{"NotANumber", "nan"}, Coefficient{"Infinity", "inf"}, Coefficient{"Overflow", "1e999"}})
    {
        inputs.push_back({coefficient.name, "entry {\n0.5 0 0 0.5 0 0 " + std::string{coefficient.value} + "\n}\n",
            "entry", "invalid IFS definition"});
    }
    return inputs;
}

class TestInvalidIFSLoad : public TestIFSLoad, public WithParamInterface<InvalidIFSInput>
{
protected:
    void SetUp() override;
};

void TestInvalidIFSLoad::SetUp()
{
    TestIFSLoad::SetUp();
    g_ifs_name = GetParam().entry;
    if (GetParam().definition)
    {
        write_definition(*GetParam().definition);
    }
}

TEST_P(TestInvalidIFSLoad, reportsDiagnosticWithoutPublishingPartialData)
{
    expect_failure(GetParam().message);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInvalidIFSLoad, ValuesIn(invalid_ifs_inputs()),
    [](const TestParamInfo<InvalidIFSInput> &info) { return info.param.name; });

class TestIFSAtLimit : public TestIFSLoad
{
protected:
    void SetUp() override;
    std::string m_text{"entry {\n"};
};

void TestIFSAtLimit::SetUp()
{
    TestIFSLoad::SetUp();
    for (int row = 0; row < 63; ++row)
    {
        m_text += "0.5 0 0 0.5 0 0 1\n";
    }
    write_definition(m_text + "}\n");
}

class TestIFSOverLimit : public TestIFSAtLimit
{
protected:
    void SetUp() override;
};

void TestIFSOverLimit::SetUp()
{
    ASSERT_NO_FATAL_FAILURE(TestIFSAtLimit::SetUp());
    ifs_load();
    ASSERT_EQ(63, g_num_affine_transforms);
    write_definition(m_text + "0.5 0 0 0.5 0 0 1\n}\n");
}

TEST_F(TestIFSOverLimit, rejectsSixtyFourthTransformAfterValidLoad)
{
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
