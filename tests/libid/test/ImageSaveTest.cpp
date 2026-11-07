// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ImageSaveTest.h>

#include <io/ImageSave.h>
#include <io/library.h>
#include <misc/memory.h>

#include <algorithm>
#include <string>

namespace id::test::image_save_test
{
namespace
{

void ignore_pixel(int, int, int)
{
}

int blank_pixel(int, int)
{
    return 0;
}

} // namespace

void ImageSaveTest::SetUp()
{
    std::string name{UnitTest::GetInstance()->current_test_info()->name()};
    std::replace(name.begin(), name.end(), '/', '_');
    m_directory.create("id-encoder-" + name + "-");
    m_path = m_directory.path() / "image.gif";
    g_save_dir = m_directory.path();
    g_temp_dir = m_directory.path();
    set_save_library(m_directory.path());
    m_saved_palette.emplace();
    for (int color = 0; color < 256; ++color)
    {
        for (int channel = 0; channel < 3; ++channel)
        {
            g_dac_box[color][channel] = static_cast<Byte>((color * 29 + channel * 83) % 256);
        }
    }
    set_pixel_functions(driver_write_pixel, driver_read_pixel);
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    EXPECT_CALL(m_driver, flush()).Times(0);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, wait_key_pressed(_)).Times(0);
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(
            [this](const int x, const int y) { return m_frame.at(static_cast<std::size_t>(y) * m_frame_width + x); });
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).Times(0);
    configure_image(8, 6, 256);
}

void ImageSaveTest::TearDown()
{
    if (m_memory_initialized)
    {
        end_disk();
        exit_check();
    }
    set_pixel_functions(ignore_pixel, blank_pixel);
    clear_save_library();
    m_saved_palette.reset();
    m_directory.remove();
}

void ImageSaveTest::configure_image(
    const int width, const int height, const int colors, const int x_offset, const int y_offset)
{
    g_logical_screen = {
        width, height, x_offset, y_offset, static_cast<double>(width - 1), static_cast<double>(height - 1)};
    g_screen_x_dots = width + x_offset;
    g_screen_y_dots = height + y_offset;
    g_colors = colors;
    g_and_color = colors - 1;
    g_viewport.final_aspect_ratio = 0.75f;
    m_frame_width = g_screen_x_dots;
    m_frame.resize(static_cast<std::size_t>(g_screen_x_dots) * g_screen_y_dots);
    for (int row = 0; row < g_screen_y_dots; ++row)
    {
        for (int col = 0; col < g_screen_x_dots; ++col)
        {
            m_frame[static_cast<std::size_t>(row) * m_frame_width + col] =
                static_cast<Byte>((col * 17 + row * 31 + col * row) % colors);
        }
    }
}

int ImageSaveTest::save()
{
    std::filesystem::remove(m_path);
    ImageSave image{m_path, std::nullopt};
    while (!image.done())
    {
        image.iterate();
    }
    return image.succeeded() ? 0 : -1;
}

std::vector<Byte> ImageSaveTest::image_pixels() const
{
    std::vector<Byte> pixels;
    for (int row = 0; row < g_logical_screen.y_dots; ++row)
    {
        const std::size_t offset{
            static_cast<std::size_t>(row + g_logical_screen.y_offset) * m_frame_width + g_logical_screen.x_offset};
        pixels.insert(pixels.end(), m_frame.begin() + offset, m_frame.begin() + offset + g_logical_screen.x_dots);
    }
    return pixels;
}

} // namespace id::test::image_save_test
