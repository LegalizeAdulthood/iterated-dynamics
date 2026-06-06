// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "X11Connection.h"

#include <string>

namespace id::misc
{

class X11Frame
{
public:
    bool init(const char *title);
    void terminate();
    void create_window(int width, int height);
    bool resize(int width, int height);
    void pause();
    void resume();
    void pump_messages();
    void get_max_screen(int &width, int &height) const;

private:
    void destroy_window();
    void handle_event(const XEvent &event);
    void set_fixed_size(int width, int height);

    X11Connection m_connection;
    Window m_window{};
    std::string m_title;
    int m_width{};
    int m_height{};
    bool m_mapped{};
};

} // namespace id::misc
