// SPDX-License-Identifier: GPL-3.0-only
//
#include "misc/Driver.h"

#include "ui/video_mode.h"

#include <config/driver_types.h>

#include <algorithm>
#include <cstring>
#include <vector>

using namespace id::ui;

namespace id::misc
{

// Array of drivers that are supported by source code in Id.
// The default driver is first one in the list that initializes.
static std::vector<Driver *> s_available;

Driver *g_driver{};

void load_driver(Driver *drv, int *argc, char **argv)
{
    if (drv != nullptr)
    {
        const int num = drv->init(argc, argv);
        if (num > 0)
        {
            if (! g_driver)
            {
                g_driver = drv;
            }
            s_available.push_back(drv);
        }
    }
}

//------------------------------------------------------------
// init_drivers
//
// Go through the static list of drivers defined and try to initialize
// them one at a time.  Returns the number of drivers initialized.
//
int init_drivers(int *argc, char **argv)
{
#if ID_HAVE_X11_DRIVER
    load_driver(get_x11_driver(), argc, argv);
#endif

#if ID_HAVE_WIN32_DISK_DRIVER
    load_driver(get_disk_driver(), argc, argv);
#endif

#if ID_HAVE_GDI_DRIVER
    load_driver(get_gdi_driver(), argc, argv);
#endif

#if ID_HAVE_WX_DRIVER
    load_driver(get_wx_driver(), argc, argv);
#endif

    return static_cast<int>(s_available.size()); // number of drivers supported at runtime
}

// add_video_mode
//
// a driver uses this to inform the system of an available video mode
//
void add_video_mode(Driver *drv, VideoInfo *mode)
{
#if defined(_WIN32)
    _ASSERTE(g_video_table_len < MAX_VIDEO_MODES);
#endif
    // stash away driver pointer so we can init driver for selected mode
    mode->driver = drv;
    std::memcpy(&g_video_table[g_video_table_len], mode, sizeof(g_video_table[0]));
    g_video_table_len++;
}

void close_drivers()
{
    // terminate drivers in reverse order of initialization
    while (!s_available.empty())
    {
        Driver *drv = s_available.back();
        s_available.pop_back();
        drv->terminate();
    }
    g_driver = nullptr;
}

Driver *driver_find_by_name(const char *name)
{
    if (const auto it = std::find_if(
            s_available.begin(), s_available.end(), [name](Driver *drv) { return drv->get_name() == name; });
        it != s_available.end())
    {
        return *it;
    }
    return nullptr;
}

void driver_set_video_mode(VideoInfo *mode)
{
    if (g_driver != mode->driver)
    {
        g_driver->pause();
        g_driver = mode->driver;
        g_driver->resume();
    }
    g_driver->set_video_mode(*mode);
}

} // namespace id::misc
