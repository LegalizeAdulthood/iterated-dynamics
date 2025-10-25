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
    void set_video_mode(const engine::VideoInfo &mode) override;
    void discard_screen() override;
    bool is_disk() const override;
    bool validate_mode(const engine::VideoInfo &mode) override;
    void create_window() override;
    bool resize() override;
    int read_pixel(int x, int y) override;
    void write_pixel(int x, int y, int color) override;
    void draw_line(int x1, int y1, int x2, int y2, int color) override;
    void display_string(int x, int y, int fg, int bg, const char *text) override;
    void save_graphics() override;
    void restore_graphics() override;

private:
    int m_width{};
    int m_height{};
    Byte m_clut[256][3]{};
};

Driver *get_wx_disk_driver();

} // namespace id::misc
