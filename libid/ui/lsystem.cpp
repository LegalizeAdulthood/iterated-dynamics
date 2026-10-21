// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/lsystem.h"

#include "fractals/lsystem.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"
#include "ui/thinking.h"

using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

namespace
{

class LSystemThinkingScreen
{
public:
    LSystemThinkingScreen() = default;
    LSystemThinkingScreen(const LSystemThinkingScreen &) = delete;
    LSystemThinkingScreen(LSystemThinkingScreen &&) = delete;
    LSystemThinkingScreen &operator=(const LSystemThinkingScreen &) = delete;
    LSystemThinkingScreen &operator=(LSystemThinkingScreen &&) = delete;
    ~LSystemThinkingScreen();

    bool update(bool sizing);

private:
    void clear();

    bool m_active{};
};

LSystemThinkingScreen::~LSystemThinkingScreen()
{
    clear();
}

bool LSystemThinkingScreen::update(const bool sizing)
{
    if (!sizing)
    {
        clear();
        return false;
    }
    m_active = true;
    return thinking("L-System thinking (higher orders take longer)");
}

void LSystemThinkingScreen::clear()
{
    if (m_active)
    {
        m_active = false;
        thinking_end();
    }
}

} // namespace

int lsystem_type()
{
    if (!lsystem_loaded())
    {
        lsystem_load();
    }

    LSystem l_system;
    l_system.start();
    LSystemThinkingScreen thinking_screen;
    unsigned char poll_counter{};
    while (!l_system.done() && !l_system.interrupted())
    {
        const bool sizing{l_system.sizing()};
        if (thinking_screen.update(sizing))
        {
            l_system.suspend();
            return 0;
        }
        if (!sizing && poll_counter++ == 0)
        {
            display_disk_storage_progress();
            if (driver_key_pressed())
            {
                l_system.suspend();
                return 0;
            }
        }
        l_system.iterate();
    }
    return 0;
}

} // namespace id::ui
