// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

namespace id::ui
{

class SoundSession;

// TODO: removed later
// Slice 52.12 removes the synchronous render entry points.
int standard_fractal();
int standard_fractal(SoundSession &sound);

} // namespace id::ui
