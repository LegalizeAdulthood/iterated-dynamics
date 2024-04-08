#include <load_config.h>
#include <drivers.h>

#include "test_config_data.h"

#include <video_mode.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class MockDriver : public Driver
{
public:
    MockDriver(const std::string &name) :
        Driver(),
        m_name(name)
    {
        Driver::name = m_name.c_str();
        init = &s_init;
        validate_mode = &s_validate_mode;
        terminate = &s_terminate;
    }

    MOCK_METHOD(bool, m_init, (Driver * drv, int *argc, char **argv), ());
    MOCK_METHOD(bool, m_validate_mode, (Driver *drv, VIDEOINFO *mode), ());
    MOCK_METHOD(void, m_terminate, (Driver *rv), ());
    
    // delegating shims
    static bool s_init(Driver *drv, int *argc, char **argv)
    {
        return self(drv)->m_init(drv, argc, argv);
    }
    static bool s_validate_mode(Driver *drv, VIDEOINFO *mode)
    {
        return self(drv)->m_validate_mode(drv, mode);
    }
    static void s_terminate(Driver *drv)
    {
        self(drv)->m_terminate(drv);
    }
    static MockDriver *self(Driver *drv)
    {
        return static_cast<MockDriver *>(drv);
    }

    std::string m_name;
};

TEST(TestDriver, loadClose)
{
    MockDriver gdi{"gdi"};
    EXPECT_CALL(gdi, m_init(NotNull(), nullptr, nullptr)).WillOnce(Return(true));
    EXPECT_CALL(gdi, m_terminate(NotNull()));

    load_driver(&gdi, nullptr, nullptr);

    EXPECT_EQ(&gdi, g_driver);

    close_drivers();
    EXPECT_EQ(nullptr, g_driver);
}

TEST(TestLoadConfig, gdiDisk)
{
    MockDriver gdi{"gdi"};
    MockDriver disk{"disk"};
    ExpectationSet init_gdi = EXPECT_CALL(gdi, m_init(NotNull(), nullptr, nullptr)).WillOnce(Return(true));
    ExpectationSet init_disk = EXPECT_CALL(disk, m_init(NotNull(), nullptr, nullptr)).WillOnce(Return(true));
    EXPECT_CALL(gdi, m_validate_mode(NotNull(), NotNull())).After(init_gdi).WillRepeatedly(Return(true));
    EXPECT_CALL(disk, m_validate_mode(NotNull(), NotNull())).After(init_disk).WillRepeatedly(Return(true));
    EXPECT_CALL(gdi, m_terminate(NotNull()));
    EXPECT_CALL(disk, m_terminate(NotNull()));
    load_driver(&gdi, nullptr, nullptr);
    load_driver(&disk, nullptr, nullptr);

    load_config(ID_TEST_CONFIG_FILE);

    ASSERT_EQ(2, g_video_table_len);
    const VIDEOINFO &gdi_mode{g_video_table[0]};
    EXPECT_STREQ(ID_TEST_GDI_NAME, gdi_mode.name);
    EXPECT_STREQ(ID_TEST_GDI_COMMENT, gdi_mode.comment);
    EXPECT_EQ(ID_TEST_GDI_FN_KEY, gdi_mode.keynum);
    EXPECT_EQ(0, gdi_mode.videomodeax);
    EXPECT_EQ(0, gdi_mode.videomodebx);
    EXPECT_EQ(0, gdi_mode.videomodecx);
    EXPECT_EQ(0, gdi_mode.videomodedx);
    EXPECT_EQ(ID_TEST_GDI_DOTMODE, gdi_mode.dotmode);
    EXPECT_EQ(ID_TEST_GDI_WIDTH, gdi_mode.xdots);
    EXPECT_EQ(ID_TEST_GDI_HEIGHT, gdi_mode.ydots);
    EXPECT_EQ(ID_TEST_GDI_COLORS, gdi_mode.colors);
    EXPECT_EQ(&gdi, gdi_mode.driver);
    const VIDEOINFO &disk_mode{g_video_table[1]};
    EXPECT_STREQ(ID_TEST_DISK_NAME, disk_mode.name);
    EXPECT_STREQ(ID_TEST_DISK_COMMENT, disk_mode.comment);
    EXPECT_EQ(ID_TEST_DISK_FN_KEY, disk_mode.keynum);
    EXPECT_EQ(0, disk_mode.videomodeax);
    EXPECT_EQ(0, disk_mode.videomodebx);
    EXPECT_EQ(0, disk_mode.videomodecx);
    EXPECT_EQ(0, disk_mode.videomodedx);
    EXPECT_EQ(ID_TEST_DISK_DOTMODE, disk_mode.dotmode);
    EXPECT_EQ(ID_TEST_DISK_WIDTH, disk_mode.xdots);
    EXPECT_EQ(ID_TEST_DISK_HEIGHT, disk_mode.ydots);
    EXPECT_EQ(ID_TEST_DISK_COLORS, disk_mode.colors);
    EXPECT_EQ(&disk, disk_mode.driver);

    close_drivers();
}
