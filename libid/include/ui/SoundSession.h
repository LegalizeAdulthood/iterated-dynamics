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
    enum class InputPolicy
    {
        INTERRUPTIBLE,
        ATOMIC,
    };

    enum class Ownership
    {
        OWNED,
        BORROWED,
    };

    SoundSession();
    explicit SoundSession(DriverSoundOutput::Source source);
    explicit SoundSession(engine::SoundOutputPtr sound, Ownership ownership = Ownership::OWNED);
    ~SoundSession();

    SoundSession(const SoundSession &) = delete;
    SoundSession(SoundSession &&) = delete;
    SoundSession &operator=(const SoundSession &) = delete;
    SoundSession &operator=(SoundSession &&) = delete;

    bool wait_until_ready(InputPolicy input = InputPolicy::INTERRUPTIBLE);
    // Drain playback normally; return false if input interrupts completion.
    bool finish(InputPolicy input = InputPolicy::INTERRUPTIBLE);
    void cancel();

private:
    enum class State
    {
        ACTIVE,
        FINISHED,
        CANCELLED,
    };

    bool wait(InputPolicy input);

    engine::SoundOutputPtr m_sound;
    misc::ValueSaver<engine::SoundOutputPtr> m_saved_sound;
    Ownership m_ownership{Ownership::OWNED};
    State m_state{State::ACTIVE};
};

} // namespace id::ui
