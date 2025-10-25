// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/driver_types.h>
#include <config/port.h>
#include <misc/Driver.h>

#include "WxBaseDriver.h"

namespace id::misc
{

class WxDiskDriver : public WxBaseDriver
{
public:
    WxDiskDriver() :
        WxBaseDriver("disk")
    {
    }

    WxDiskDriver(const WxDiskDriver &) = delete;
    WxDiskDriver(WxDiskDriver &&) = delete;
    ~WxDiskDriver() override = default;
    WxDiskDriver &operator=(const WxDiskDriver &) = delete;
    WxDiskDriver &operator=(WxDiskDriver &&) = delete;

    bool init(int *argc, char **argv) override;
    bool validate_mode(const engine::VideoInfo &mode) override;
    void get_max_screen(int &width, int &height) override;
    void pause() override;
    void resume() override;
    void create_window() override;
    bool resize() override;
    void read_palette() override;
    void write_palette() override;
    int read_pixel(int x, int y) override;
    void write_pixel(int x, int y, int color) override;
    void display_string(int x, int y, int fg, int bg, const char *text) override;
    void save_graphics() override;
    void restore_graphics() override;
    void set_video_mode(const engine::VideoInfo &mode) override;
    bool is_text() override;
    void set_for_text() override;
    void set_for_graphics() override;
    bool is_disk() const override;
    void flush() override;

private:
    int m_width{};
    int m_height{};
    Byte m_clut[256][3]{};
};

} // namespace id::misc
