// SPDX-License-Identifier: GPL-3.0-only
//
#include <config/driver_types.h>

#include "MockDriver.h"

using namespace id::misc::test;

namespace id::misc
{

#if ID_HAVE_X11_DRIVER
static MockDriver s_x11_driver;

Driver *get_x11_driver()
{
    return &s_x11_driver;
}
#endif

#if ID_HAVE_GDI_DRIVER
static MockDriver s_gdi_driver;

Driver *get_gdi_driver()
{
    return &s_gdi_driver;
}
#endif

#if ID_HAVE_WIN32_DISK_DRIVER
static MockDriver s_disk_driver;

Driver *get_disk_driver()
{
    return &s_disk_driver;
}
#endif

#if ID_HAVE_WX_DRIVER
static MockDriver s_wx_driver;

Driver *get_wx_driver()
{
    return &s_wx_driver;
}
#endif

} // namespace id::misc
