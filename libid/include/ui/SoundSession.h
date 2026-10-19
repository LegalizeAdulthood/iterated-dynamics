// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <engine/SoundOutput.h>
#include <misc/ValueSaver.h>
#include <ui/DriverSoundOutput.h>

namespace id::ui
{

// One owning scope selects output; nested controllers use the same g_sound.
class SoundSession
{
public:
    explicit SoundSession(DriverSoundOutput::Source source);
    explicit SoundSession(engine::SoundOutputPtr sound);
    ~SoundSession();

    SoundSession(const SoundSession &) = delete;
    SoundSession(SoundSession &&) = delete;
    SoundSession &operator=(const SoundSession &) = delete;
    SoundSession &operator=(SoundSession &&) = delete;

    bool wait_until_ready();
    // Drain playback normally; return false if input interrupts completion.
    bool finish();
    void cancel();

private:
    enum class State
    {
        ACTIVE,
        FINISHED,
        CANCELLED,
    };

    bool wait();

    engine::SoundOutputPtr m_sound;
    misc::ValueSaver<engine::SoundOutputPtr> m_saved_sound;
    State m_state{State::ACTIVE};
};

} // namespace id::ui
