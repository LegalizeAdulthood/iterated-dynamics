// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/SoundOutput.h>

#include <engine/NullSoundOutput.h>

namespace id::engine
{

SoundOutputPtr g_sound{std::make_shared<NullSoundOutput>()};

} // namespace id::engine
