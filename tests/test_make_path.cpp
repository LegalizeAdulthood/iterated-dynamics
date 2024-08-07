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
    std::string result{"foo.txt"};
};

} // namespace

TEST_F(TestMakePath, empty)
{
    result = make_path(nullptr, nullptr, nullptr, nullptr);

    EXPECT_TRUE(result.empty());
}

TEST_F(TestMakePath, drive)
{
    result = make_path("C:", nullptr, nullptr, nullptr);

#if WIN32
    ASSERT_EQ("C:", result);
#else
    ASSERT_EQ("", result);
#endif
}

TEST_F(TestMakePath, directory)
{
    result = make_path(nullptr, "foo", nullptr, nullptr);

    ASSERT_EQ(fs::path{"foo/"}.make_preferred().string(), result);
}

TEST_F(TestMakePath, filenameNullDirectory)
{
    result = make_path(nullptr, nullptr, "foo", nullptr);

    ASSERT_EQ("foo", result);
}

TEST_F(TestMakePath, filenameEmptyDirectory)
{
    result = make_path(nullptr, "", "foo", nullptr);

    ASSERT_EQ("foo", result);
}

TEST_F(TestMakePath, extension)
{
    result = make_path(nullptr, nullptr, nullptr, ".gif");

    ASSERT_EQ(".gif", result);
}

TEST_F(TestMakePath, filenameExtension)
{
    result = make_path(nullptr, nullptr, "foo", ".gif");

    ASSERT_EQ("foo.gif", result);
}

TEST_F(TestMakePath, directoryFilenameExtension)
{
    result = make_path(nullptr, "tmp", "foo", ".gif");

    ASSERT_EQ(fs::path{"tmp/foo.gif"}.make_preferred().string(), result);
}

TEST_F(TestMakePath, driveDirectoryFilenameExtension)
{
    result = make_path("C:", "tmp", "foo", ".gif");

#if WIN32
    ASSERT_EQ(fs::path{"C:tmp/foo.gif"}.make_preferred().string(), result);
#else
    ASSERT_EQ(fs::path{"tmp/foo.gif"}.make_preferred().string(), result);
#endif
}

TEST_F(TestMakePath, directoryWithTrailingSlash)
{
    result = make_path(nullptr, "tmp/", nullptr, nullptr);

    ASSERT_EQ(fs::path{"tmp/"}.make_preferred().string(), result);
}

TEST_F(TestMakePath, filenameWithDot)
{
    result = make_path(nullptr, nullptr, "1997.04.30-Ship_of_Indecision", ".par");

    ASSERT_EQ("1997.04.30-Ship_of_Indecision.par", result);
}

namespace
{

class TestMakeFNameExt : public TestMakePath
{
};

} // namespace

TEST_F(TestMakeFNameExt, nullFilename)
{
    result = make_fname_ext(nullptr, ".par");

    ASSERT_EQ(".par", result);
}

TEST_F(TestMakeFNameExt, emptyFilename)
{
    result = make_fname_ext("", ".par");

    ASSERT_EQ(".par", result);
}

TEST_F(TestMakeFNameExt, nullExtension)
{
    result = make_fname_ext("id", nullptr);

    ASSERT_EQ("id", result);
}

TEST_F(TestMakeFNameExt, emptyExtension)
{
    result = make_fname_ext("id", "");

    ASSERT_EQ("id", result);
}

TEST_F(TestMakeFNameExt, basic)
{
    result = make_fname_ext("id", ".par");

    ASSERT_EQ("id.par", result);
}
