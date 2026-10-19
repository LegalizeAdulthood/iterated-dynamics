// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/SoundSession.h>

#include <engine/NullSoundOutput.h>
#include <engine/sound.h>
#include <fractals/lorenz.h>
#include <misc/Driver.h>
#include <ui/KeyboardHandler.h>

#include <memory>
#include <stdexcept>
#include <utility>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

SoundSession::SoundSession(const DriverSoundOutput::Source source) :
    m_saved_sound{g_sound}
{
    if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP || (g_orbit_save_flags & OSF_MIDI) != 0)
    {
        m_sound = std::make_shared<DriverSoundOutput>(source);
    }
    else
    {
        m_sound = std::make_shared<NullSoundOutput>();
    }
    g_sound = m_sound;
}

SoundSession::SoundSession(SoundOutputPtr sound) :
    m_sound{std::move(sound)},
    m_saved_sound{g_sound}
{
    if (m_sound == nullptr)
    {
        throw std::logic_error("Sound session requires an output");
    }
    g_sound = m_sound;
}

SoundSession::~SoundSession()
{
    // Explicit finish/cancel reports file errors; abandoned playback only mutes.
    if (m_state == State::ACTIVE)
    {
        m_sound->mute();
    }
}

bool SoundSession::wait_until_ready()
{
    if (m_state == State::CANCELLED)
    {
        return false;
    }
    if (m_state == State::FINISHED)
    {
        throw std::logic_error("Sound session is finished");
    }
    if (calc_interrupt_requested())
    {
        cancel();
        return false;
    }

    m_sound->update();
    while (!m_sound->ready())
    {
        if (!wait())
        {
            return false;
        }
        m_sound->update();
    }
    return true;
}

bool SoundSession::finish()
{
    if (m_state != State::ACTIVE)
    {
        return m_state == State::FINISHED;
    }
    if (calc_interrupt_requested())
    {
        cancel();
        return false;
    }
    while (true)
    {
        m_sound->update();
        try
        {
            if (m_sound->finish())
            {
                m_state = State::FINISHED;
                return true;
            }
        }
        catch (...)
        {
            // A finalization error may leave the port closed already.
            m_state = State::CANCELLED;
            throw;
        }
        if (!wait())
        {
            return false;
        }
    }
}

void SoundSession::cancel()
{
    if (m_state == State::ACTIVE)
    {
        m_sound->mute();
        m_state = State::CANCELLED;
        if (!m_sound->finish())
        {
            throw std::logic_error("Muted sound output still has pending playback");
        }
    }
}

bool SoundSession::wait()
{
    if (calc_interrupted())
    {
        cancel();
        return false;
    }
    driver_delay(1);
    if (calc_interrupt_requested())
    {
        cancel();
        return false;
    }
    return true;
}

} // namespace id::ui
