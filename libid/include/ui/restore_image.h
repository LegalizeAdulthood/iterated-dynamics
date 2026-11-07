// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson
//
#pragma once

#include <io/EvolutionInfo.h>

#include <optional>

namespace id::engine
{
struct Viewport;
}

namespace id::ui
{

class SoundSession;

// Apply normalized evolution controls and retain resumable progress.
void apply_evolution_info(const std::optional<io::EvolutionInfo> &info);

int prepare_image_restore(const engine::Viewport &viewport);

int restore_image_views();

// Complete the whole GIF, including scanline feedback, without polling input.
int restore_image();
int restore_image(SoundSession &sound);

} // namespace id::ui
