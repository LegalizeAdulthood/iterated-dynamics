// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <misc/Driver.h>

namespace id::misc
{

// Metadata checkers do not initialize graphical drivers.
Driver *get_wx_driver()
{
    return nullptr;
}

Driver *get_wx_disk_driver()
{
    return nullptr;
}

} // namespace id::misc
