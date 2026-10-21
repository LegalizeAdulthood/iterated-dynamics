// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

namespace id::engine
{
class NonStandardFractal;
}

namespace id::ui
{

class SoundSession;

class StereoController
{
public:
    StereoController(engine::NonStandardFractal &calculation, SoundSession &sound);

    int run();

private:
    engine::NonStandardFractal &m_calculation;
    SoundSession &m_sound;
};

} // namespace id::ui
