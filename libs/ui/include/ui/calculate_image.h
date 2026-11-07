// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <string>
#include <vector>

namespace id::engine
{
class ImageCalculation;
}

namespace id::ui
{

void complete_image_setup(engine::ImageCalculation &image, const std::vector<std::string> &warnings);

} // namespace id::ui
