// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/calcfrac.h"
#include "engine/VideoInfo.h"
#include "engine/Viewport.h"

#include <optional>

namespace id::io
{
struct ImageMetadata;
}

namespace id::ui
{

struct ImageVideoMode
{
    int mode{-1};
    engine::VideoInfo video{};
    engine::Viewport viewport{};
    int width{};
    int height{};
    int skip_x{};
    int skip_y{};
    float aspect_ratio{};
    engine::CalcStatus calc_status{};
};

std::optional<ImageVideoMode> get_video_mode(const io::ImageMetadata &metadata, const engine::Viewport &viewport);

} // namespace id::ui
