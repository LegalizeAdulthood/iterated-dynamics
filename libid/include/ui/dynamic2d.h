// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

namespace id::engine
{
class NonStandardFractal;
}

namespace id::ui
{

class SoundSession;

// TODO: removed later
// Slice 52.12 removes the synchronous render entry point.
int dynamic2d_type(engine::NonStandardFractal &calculation, SoundSession &sound);

} // namespace id::ui
