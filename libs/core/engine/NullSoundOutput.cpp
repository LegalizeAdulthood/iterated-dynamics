// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/NullSoundOutput.h>

namespace id::engine
{

bool NullSoundOutput::play(const SoundSample &)
{
    return true;
}

void NullSoundOutput::update()
{
}

bool NullSoundOutput::ready() const
{
    return true;
}

void NullSoundOutput::begin_orbit()
{
}

void NullSoundOutput::mute()
{
}

bool NullSoundOutput::finish()
{
    return true;
}

} // namespace id::engine
