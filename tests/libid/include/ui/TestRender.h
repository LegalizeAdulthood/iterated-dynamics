// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

namespace id::ui
{
class SoundSession;
}

namespace id::test
{

int render_image();
int render_standard_image();
int render_standard_image(ui::SoundSession &sound);

} // namespace id::test
