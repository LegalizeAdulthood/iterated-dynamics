// SPDX-License-Identifier: GPL-3.0-only
//
#include "X11BaseDriver.h"

#include "engine/VideoInfo.h"

using namespace id::engine;

namespace id::misc
{

namespace
{

class X11Driver : public X11BaseDriver
{
public:
    X11Driver() :
        X11BaseDriver("x11", "X11")
    {
    }

    bool init(int *argc, char **argv) override;
};

#define DRIVER_MODE(width_, height_) {0, width_, height_, 256, nullptr, "                        "}
VideoInfo s_modes[]{
    DRIVER_MODE(800, 600),
    DRIVER_MODE(1024, 768),
    DRIVER_MODE(1200, 900),
    DRIVER_MODE(1280, 960),
    DRIVER_MODE(1400, 1050),
    DRIVER_MODE(1500, 1125),
    DRIVER_MODE(1600, 1200),
};
#undef DRIVER_MODE

X11Driver s_x11_driver;

} // namespace

bool X11Driver::init(int *argc, char **argv)
{
    if (!X11BaseDriver::init(argc, argv))
    {
        return false;
    }

    int width{};
    int height{};
    get_max_screen(width, height);
    for (VideoInfo &mode : s_modes)
    {
        if (mode.x_dots <= width && mode.y_dots <= height)
        {
            add_video_mode(this, &mode);
        }
    }
    return true;
}

Driver *get_x11_driver()
{
    return &s_x11_driver;
}

} // namespace id::misc
