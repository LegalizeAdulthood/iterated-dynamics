#include <make_path.h>

#include <gtest/gtest.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace
{

class TestMakePath : public ::testing::Test
{
public:
    ~TestMakePath() override = default;

protected:
    void SetUp() override;

    char buffer[100]{};
};

void TestMakePath::SetUp()
{
    strcpy(buffer, "foo.txt");
}

} // namespace

TEST_F(TestMakePath, empty)
{
    make_path(buffer, nullptr, nullptr, nullptr, nullptr);

    ASSERT_STREQ("", buffer);
}

TEST_F(TestMakePath, drive)
{
    make_path(buffer, "C:", nullptr, nullptr, nullptr);

#if WIN32
    ASSERT_STREQ("C:", buffer);
#else
    ASSERT_STREQ("", buffer);
#endif
}

TEST_F(TestMakePath, directory)
{
    make_path(buffer, nullptr, "foo", nullptr, nullptr);

    ASSERT_EQ(fs::path{"foo/"}.make_preferred().string(), std::string{buffer});
}

TEST_F(TestMakePath, filenameNullDirectory)
{
    make_path(buffer, nullptr, nullptr, "foo", nullptr);

    ASSERT_STREQ("foo", buffer);
}

TEST_F(TestMakePath, filenameEmptyDirectory)
{
    make_path(buffer, nullptr, "", "foo", nullptr);

    ASSERT_STREQ("foo", buffer);
}

TEST_F(TestMakePath, extension)
{
    make_path(buffer, nullptr, nullptr, nullptr, ".gif");

    ASSERT_STREQ(".gif", buffer);
}

TEST_F(TestMakePath, filenameExtension)
{
    make_path(buffer, nullptr, nullptr, "foo", ".gif");

    ASSERT_STREQ("foo.gif", buffer);
}

TEST_F(TestMakePath, directoryFilenameExtension)
{
    make_path(buffer, nullptr, "tmp", "foo", ".gif");

    ASSERT_STREQ(fs::path{"tmp/foo.gif"}.make_preferred().string().c_str(), buffer);
}

TEST_F(TestMakePath, driveDirectoryFilenameExtension)
{
    make_path(buffer, "C:", "tmp", "foo", ".gif");

#if WIN32
    ASSERT_STREQ(fs::path{"C:tmp/foo.gif"}.make_preferred().string().c_str(), buffer);
#else
    ASSERT_STREQ(fs::path{"tmp/foo.gif"}.make_preferred().string().c_str(), buffer);
#endif
}

TEST_F(TestMakePath, directoryWithTrailingSlash)
{
    make_path(buffer, nullptr, "tmp/", nullptr, nullptr);

    ASSERT_STREQ(fs::path{"tmp/"}.make_preferred().string().c_str(), buffer);
}

namespace
{

class TestMakeFNameExt : public TestMakePath
{
};

} // namespace

TEST_F(TestMakeFNameExt, nullFilename)
{
    make_fname_ext(buffer, nullptr, ".par");

    ASSERT_STREQ(".par", buffer);
}

TEST_F(TestMakeFNameExt, emptyFilename)
{
    make_fname_ext(buffer, "", ".par");

    ASSERT_STREQ(".par", buffer);
}

TEST_F(TestMakeFNameExt, nullExtension)
{
    make_fname_ext(buffer, "id", nullptr);

    ASSERT_STREQ("id", buffer);
}

TEST_F(TestMakeFNameExt, emptyExtension)
{
    make_fname_ext(buffer, "id", "");

    ASSERT_STREQ("id", buffer);
}

TEST_F(TestMakeFNameExt, basic)
{
    make_fname_ext(buffer, "id", ".par");

    ASSERT_STREQ("id.par", buffer);
}
