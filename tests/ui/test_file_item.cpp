#include "engine/cmdfiles.h"
#include "misc/ValueSaver.h"

#include <ui/file_item.h>

#include "test_data.h"

#include <gtest/gtest.h>

using namespace id::test::data;

namespace
{

class TestFindFileItem : public testing::Test
{
public:
    ~TestFindFileItem() override = default;

protected:
    void TearDown() override;

    std::FILE *m_file{};
    std::filesystem::path m_path;
};

void TestFindFileItem::TearDown()
{
    if (m_file != nullptr)
    {
        std::fclose(m_file);
        m_file = nullptr;
    }
    Test::TearDown();
}

} // namespace

TEST_F(TestFindFileItem, formula)
{
    m_path = ID_TEST_FRM_DIR;
    m_path /= ID_TEST_FRM_FILE;

    const bool result{find_file_item(m_path, "Fractint", &m_file, ItemType::FORMULA)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
}

TEST_F(TestFindFileItem, ifs)
{
    m_path = ID_TEST_HOME_DIR;
    m_path /= "ifs";
    m_path /= ID_TEST_IFS_FILE;

    const bool result{find_file_item(m_path, ID_TEST_FIRST_IFS_NAME, &m_file, ItemType::IFS)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
}

TEST_F(TestFindFileItem, lindenmayerSystem)
{
    m_path = ID_TEST_HOME_DIR;
    m_path/= "lsystem";
    m_path /= ID_TEST_LSYSTEM_FILE;

    const bool result{find_file_item(m_path, "Koch1", &m_file, ItemType::L_SYSTEM)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
}

namespace
{

class TestFindParFileItem : public TestFindFileItem
{
public:
    ~TestFindParFileItem() override = default;

protected:
    std::filesystem::path m_par{std::filesystem::path{ID_TEST_PAR_DIR} / ID_TEST_PAR_FILE};
    ValueSaver<std::filesystem::path> m_saved_parameter_file{g_parameter_file, m_par};
};

} // namespace

TEST_F(TestFindParFileItem, formula)
{
    m_path = "missing.frm";

    const bool result{find_file_item(m_path, "Fractint", &m_file, ItemType::FORMULA)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
}

TEST_F(TestFindParFileItem, ifs)
{
    m_path = "missing.ifs";

    const bool result{find_file_item(m_path, ID_TEST_FIRST_IFS_NAME, &m_file, ItemType::IFS)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
}

TEST_F(TestFindParFileItem, lindenmayerSystem)
{
    m_path = "missing.l";

    const bool result{find_file_item(m_path, "Koch1", &m_file, ItemType::L_SYSTEM)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, m_file);
}
