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

int froth_type(engine::NonStandardFractal &calculation);
int froth_type(engine::NonStandardFractal &calculation, SoundSession &sound);

} // namespace id::ui
