// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/GifRestore.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::io;
using namespace testing;

namespace id::test
{

namespace
{

std::vector<Byte> gif_header()
{
    return {'G', 'I', 'F', '8', '7', 'a', 2, 0, 2, 0, 0x80, 0, 0, 255, 0, 0, 0, 0, 0};
}

void append_image(std::vector<Byte> &data, const Byte left = 0, const Byte top = 0, const bool local_palette = false)
{
    data.insert(data.end(), {',', left, 0, top, 0, 2, 0, 2, 0, static_cast<Byte>(local_palette ? 0x80 : 0)});
    if (local_palette)
    {
        data.insert(data.end(), {0, 0, 255, 255, 255, 0});
    }
    // Three-bit codes: clear, 0, 1, clear, 1, 0, end.
    data.insert(data.end(), {2, 3, 0x44, 0x18, 0x14, 0});
}

std::vector<Byte> gif_data()
{
    std::vector<Byte> data{gif_header()};
    append_image(data);
    data.push_back(';');
    return data;
}

std::vector<std::vector<Byte>> drain(GifRestore &restore)
{
    std::vector<std::vector<Byte>> lines;
    for (int steps = 0; !restore.done() && steps < 100; ++steps)
    {
        restore.iterate();
        if (restore.state() == GifRestore::State::LINE)
        {
            lines.push_back(restore.line());
        }
    }
    EXPECT_TRUE(restore.done());
    return lines;
}

} // namespace

class TestGifRestore : public Test
{
protected:
    std::filesystem::path write_file(const std::vector<Byte> &data);
    void TearDown() override;

    std::vector<std::filesystem::path> m_paths;
};

std::filesystem::path TestGifRestore::write_file(const std::vector<Byte> &data)
{
    const std::string name{UnitTest::GetInstance()->current_test_info()->name()};
    const std::filesystem::path path{std::filesystem::temp_directory_path() /
        ("id-GifRestore-" + name + "-" + std::to_string(m_paths.size()) + ".gif")};
    m_paths.push_back(path);
    std::ofstream output{path, std::ios::binary};
    if (!data.empty())
    {
        output.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    output.close();
    if (!output)
    {
        throw std::runtime_error("Can't create GIF restore test input");
    }
    return path;
}

void TestGifRestore::TearDown()
{
    for (const std::filesystem::path &path : m_paths)
    {
        std::filesystem::remove(path);
    }
}

TEST_F(TestGifRestore, separatesHeaderImageLinesAndCompletion)
{
    const std::filesystem::path path{write_file(gif_data())};
    GifRestore restore{path};
    EXPECT_FALSE(restore.done());
    EXPECT_EQ(GifRestore::State::BLOCK, restore.state());
    EXPECT_EQ(2, restore.width());
    EXPECT_EQ(2, restore.height());
    const std::vector<std::array<Byte, 3>> expected_palette{{255, 0, 0}, {0, 0, 0}};
    EXPECT_EQ(expected_palette, restore.palette());

    restore.iterate();
    EXPECT_EQ(GifRestore::State::IMAGE, restore.state());
    EXPECT_EQ(0, restore.image().left);
    EXPECT_EQ(0, restore.image().top);
    EXPECT_EQ(2, restore.image().width);
    EXPECT_EQ(2, restore.image().height);
    restore.iterate();
    EXPECT_EQ(GifRestore::State::LINE, restore.state());
    EXPECT_EQ((std::vector<Byte>{0, 1}), restore.line());
    restore.iterate();
    EXPECT_EQ(GifRestore::State::LINE, restore.state());
    EXPECT_EQ((std::vector<Byte>{1, 0}), restore.line());
    restore.iterate();
    EXPECT_EQ(GifRestore::State::IMAGE_END, restore.state());
    EXPECT_FALSE(restore.done());
    restore.iterate();
    EXPECT_TRUE(restore.done());
    EXPECT_EQ(0, restore.result());
    restore.iterate();
    EXPECT_EQ(0, restore.result());
    EXPECT_TRUE(std::filesystem::remove(path));
}

TEST_F(TestGifRestore, extensionAdvancesOneSubBlockPerStep)
{
    std::vector<Byte> data{gif_header()};
    data.insert(data.end(), {'!', 0xFE, 3, 'a', 'b', 'c', 2, 'd', 'e', 0});
    append_image(data);
    data.push_back(';');
    GifRestore restore{write_file(data)};

    for (int step = 0; step < 3; ++step)
    {
        restore.iterate();
        EXPECT_EQ(GifRestore::State::EXTENSION, restore.state());
        EXPECT_TRUE(restore.line().empty());
    }
    restore.iterate();
    EXPECT_EQ(GifRestore::State::BLOCK, restore.state());
    restore.iterate();
    EXPECT_EQ(GifRestore::State::IMAGE, restore.state());
    const std::vector<std::vector<Byte>> expected{{0, 1}, {1, 0}};
    EXPECT_EQ(expected, drain(restore));
    EXPECT_EQ(0, restore.result());
}

TEST_F(TestGifRestore, retainsImageBoundariesPositionsAndGlobalPalette)
{
    std::vector<Byte> data{gif_header()};
    append_image(data);
    data.insert(data.end(), {'!', 0xFE, 1, 'x', 0});
    append_image(data, 3, 4, true);
    data.push_back(';');
    GifRestore restore{write_file(data)};
    std::vector<GifRestore::Image> images;
    std::vector<std::vector<Byte>> lines;
    int endings{};

    for (int steps = 0; !restore.done() && steps < 100; ++steps)
    {
        restore.iterate();
        if (restore.state() == GifRestore::State::IMAGE)
        {
            images.push_back(restore.image());
        }
        else if (restore.state() == GifRestore::State::LINE)
        {
            lines.push_back(restore.line());
        }
        else if (restore.state() == GifRestore::State::IMAGE_END)
        {
            ++endings;
        }
    }
    EXPECT_TRUE(restore.done());
    EXPECT_EQ(0, restore.result());
    EXPECT_EQ(2, endings);
    ASSERT_EQ(2, images.size());
    EXPECT_EQ(0, images[0].left);
    EXPECT_EQ(0, images[0].top);
    EXPECT_EQ(3, images[1].left);
    EXPECT_EQ(4, images[1].top);
    EXPECT_EQ((std::array<Byte, 3>{255, 0, 0}), restore.palette()[0]);
    const std::vector<std::vector<Byte>> expected{{0, 1}, {1, 0}, {0, 1}, {1, 0}};
    EXPECT_EQ(expected, lines);
}

TEST_F(TestGifRestore, independentRestoresRetainInputAndLineBuffers)
{
    GifRestore first{write_file(gif_data())};
    GifRestore second{write_file(gif_data())};
    first.iterate();
    first.iterate();
    const std::vector<Byte> &line{first.line()};
    const Byte *pixels{line.data()};

    const std::vector<std::vector<Byte>> expected{{0, 1}, {1, 0}};
    EXPECT_EQ(expected, drain(second));
    EXPECT_EQ((std::vector<Byte>{0, 1}), line);
    EXPECT_EQ(pixels, line.data());
    first.iterate();
    EXPECT_EQ((std::vector<Byte>{1, 0}), first.line());
    EXPECT_TRUE(drain(first).empty());
    EXPECT_EQ(0, first.result());
}

TEST_F(TestGifRestore, preservesSkipFactorsAndPotentialRowWidth)
{
    const std::filesystem::path path{write_file(gif_data())};
    GifRestore skipped{path, 1, 1};
    const std::vector<std::vector<Byte>> expected_skipped{{0}};
    EXPECT_EQ(expected_skipped, drain(skipped));
    EXPECT_EQ(0, skipped.result());

    GifRestore potential{path, 0, 0, true};
    const std::vector<std::vector<Byte>> expected_potential{{0}, {1}, {1}, {0}};
    EXPECT_EQ(expected_potential, drain(potential));
    EXPECT_EQ(2, potential.image().width);
    EXPECT_EQ(0, potential.result());
}

TEST_F(TestGifRestore, closesFileWhenAbandonedBetweenLines)
{
    const std::filesystem::path path{write_file(gif_data())};
    {
        GifRestore restore{path};
        restore.iterate();
        restore.iterate();
        EXPECT_EQ(GifRestore::State::LINE, restore.state());
    }
    EXPECT_TRUE(std::filesystem::remove(path));
}

TEST_F(TestGifRestore, truncatedFileFailsAndClosesAtEveryByteBoundary)
{
    const std::vector<Byte> data{gif_data()};
    for (std::size_t size = 0; size < data.size(); ++size)
    {
        SCOPED_TRACE(Message() << "size=" << size);
        const std::filesystem::path path{write_file(std::vector<Byte>{data.begin(), data.begin() + size})};
        GifRestore restore{path};
        drain(restore);
        EXPECT_LT(restore.result(), 0);
        EXPECT_TRUE(std::filesystem::remove(path));
    }
}

TEST_F(TestGifRestore, rejectsInvalidHeadersDescriptorsAndDataTerminators)
{
    for (const int offset : {0, 3, 4, 5, 10, 19, 24, 29, 34})
    {
        SCOPED_TRACE(Message() << "offset=" << offset);
        std::vector<Byte> data{gif_data()};
        data[offset] = offset == 34 ? 1 : 0;
        const std::filesystem::path path{write_file(data)};
        GifRestore restore{path};
        drain(restore);
        EXPECT_LT(restore.result(), 0);
        EXPECT_TRUE(std::filesystem::remove(path));
    }
}

TEST_F(TestGifRestore, truncatedExtensionAndLocalPaletteFailAndClose)
{
    std::vector<Byte> extension{gif_header()};
    extension.insert(extension.end(), {'!', 0xFE, 3, 'x'});
    std::vector<Byte> palette{gif_header()};
    palette.insert(palette.end(), {',', 0, 0, 0, 0, 2, 0, 2, 0, 0x80, 255});
    for (const std::vector<Byte> &data : {extension, palette})
    {
        const std::filesystem::path path{write_file(data)};
        GifRestore restore{path};
        EXPECT_TRUE(drain(restore).empty());
        EXPECT_EQ(-1, restore.result());
        EXPECT_TRUE(std::filesystem::remove(path));
    }
}

TEST_F(TestGifRestore, missingFileReportsFailureAndNegativeSkipFactorsThrow)
{
    const std::filesystem::path path{write_file(gif_data())};
    EXPECT_TRUE(std::filesystem::remove(path));
    GifRestore missing{path};
    EXPECT_TRUE(missing.done());
    EXPECT_EQ(-1, missing.result());
    EXPECT_THROW((GifRestore{path, -1, 0}), std::logic_error);
    EXPECT_THROW((GifRestore{path, 0, -1}), std::logic_error);
}

} // namespace id::test
