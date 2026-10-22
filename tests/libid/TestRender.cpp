// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "TestRender.h"

#include <engine/SoundOutput.h>
#include <engine/StandardFractal.h>
#include <ui/ImageRenderContext.h>
#include <ui/SoundSession.h>
#include <ui/StandardFractalController.h>

using namespace id::engine;
using namespace id::ui;

namespace id::test
{

int render_image()
{
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    ImageRenderContext rendering{sound};
    rendering.start();
    while (!rendering.done())
    {
        rendering.iterate();
    }
    return rendering.result();
}

int render_standard_image()
{
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    return render_standard_image(sound);
}

int render_standard_image(SoundSession &sound)
{
    StandardFractal calculation;
    StandardFractalController controller{calculation, sound};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    return controller.result();
}

} // namespace id::test
