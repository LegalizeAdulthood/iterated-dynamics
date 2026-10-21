// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <memory>
#include <string>

namespace id::io
{
struct ImageMetadata;
}

namespace id::ui
{

extern std::shared_ptr<const io::ImageMetadata> g_tab_metadata;
extern bool g_tab_enabled; // tab display enabled

std::string perturbation_status_text(int reference_count);
int tab_display();

} // namespace id::ui
