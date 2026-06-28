// SPDX-License-Identifier: GPL-3.0-only
//
//      Header file for L-system code.
//
#pragma once

#include <config/port.h>

#include <filesystem>
#include <memory>
#include <string>

namespace id::fractals
{

extern std::filesystem::path g_l_system_filename;
extern std::string           g_l_system_name;
extern char                  g_max_angle;

class LSystem
{
public:
    LSystem();
    ~LSystem();

    LSystem(const LSystem &) = delete;
    LSystem(LSystem &&) = delete;
    LSystem &operator=(const LSystem &) = delete;
    LSystem &operator=(LSystem &&) = delete;

    void start();
    bool done() const;
    bool interrupted() const;
    void iterate();

private:
    class Impl;

    std::unique_ptr<Impl> m_impl;
};

bool lsystem_loaded();
bool lsystem_load();

} // namespace id::fractals
