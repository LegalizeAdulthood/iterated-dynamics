// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <engine/SoundOutput.h>

namespace id::engine
{

class NullSoundOutput : public SoundOutput
{
public:
    bool play(const SoundSample &sample) override;
    void update() override;
    bool ready() const override;
    void begin_orbit() override;
    void mute() override;
    bool finish() override;
};

} // namespace id::engine
