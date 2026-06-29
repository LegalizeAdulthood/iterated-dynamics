// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/sound.h"

#include "engine/sound.h"
#include "misc/Driver.h"

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

namespace
{

bool sound_input_pending()
{
    return driver_key_pressed() != 0;
}

} // namespace

void init_sound_input()
{
    set_sound_input_pending(sound_input_pending);
}

} // namespace id::ui
