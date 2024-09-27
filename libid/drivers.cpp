// SPDX-License-Identifier: GPL-3.0-only
//
#include "drivers.h"

#include "video_mode.h"

#include <cstring>

extern Driver *x11_driver;
extern Driver *g_gdi_driver;
extern Driver *g_disk_driver;

// list of drivers that are supported by source code in Id.
// default driver is first one in the list that initializes.
enum
{
    MAX_DRIVERS = 10
};
static int s_num_drivers{};
static Driver *s_available[MAX_DRIVERS]{};

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
            s_available[s_num_drivers++] = drv;
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
#if HAVE_X11_DRIVER
    load_driver(x11_driver, argc, argv);
#endif

#if HAVE_WIN32_DISK_DRIVER
    load_driver(g_disk_driver, argc, argv);
#endif

#if HAVE_GDI_DRIVER
    load_driver(g_gdi_driver, argc, argv);
#endif

    return s_num_drivers;     // number of drivers supported at runtime
}

// add_video_mode
//
// a driver uses this to inform the system of an available video mode
//
void add_video_mode(Driver *drv, VIDEOINFO *mode)
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
    for (int i = 0; i < s_num_drivers; i++)
    {
        if (s_available[i])
        {
            s_available[i]->terminate();
            s_available[i] = nullptr;
        }
    }

    g_driver = nullptr;
    s_num_drivers = 0;
}

Driver *driver_find_by_name(char const *name)
{
    for (int i = 0; i < s_num_drivers; i++)
    {
        if (name == s_available[i]->get_name())
        {
            return s_available[i];
        }
    }
    return nullptr;
}

void driver_set_video_mode(VIDEOINFO *mode)
{
    if (g_driver != mode->driver)
    {
        g_driver->pause();
        g_driver = mode->driver;
        g_driver->resume();
    }
    g_driver->set_video_mode(mode);
}
