// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "geometry/plot3d.h"

namespace id::geometry
{

class StereoView
{
public:
    StereoView() = default;
    StereoView(const StereoView &) = delete;
    StereoView(StereoView &&) = delete;
    ~StereoView();
    StereoView &operator=(const StereoView &) = delete;
    StereoView &operator=(StereoView &&) = delete;

    void start();
    bool both_eyes() const;
    bool next_view_required() const;
    void next_view();
    void finish();

private:
    void setup_plot();

    GlassesType m_glasses{GlassesType::NONE};
    StereoImage m_eye{StereoImage::NONE};
    bool m_started{};
};

} // namespace id::geometry
