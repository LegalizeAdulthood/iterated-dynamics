// SPDX-License-Identifier: GPL-3.0-only
//
#include "X11BaseDriver.h"

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
};

X11Driver s_x11_driver;

} // namespace

Driver *get_x11_driver()
{
    return &s_x11_driver;
}

} // namespace id::misc
