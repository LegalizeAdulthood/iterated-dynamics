// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson
//
#pragma once

namespace id::ui
{

class SoundSession;

int prepare_image_restore();

// Complete the whole GIF, including scanline feedback, without polling input.
int restore_image();
int restore_image(SoundSession &sound);

} // namespace id::ui
