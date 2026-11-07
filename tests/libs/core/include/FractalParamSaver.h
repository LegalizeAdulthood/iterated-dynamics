// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <engine/calcfrac.h>

#include <array>
#include <string>

namespace id::test
{

class FractalParamSaver
{
public:
    FractalParamSaver();
    ~FractalParamSaver();

    FractalParamSaver(const FractalParamSaver &) = delete;
    FractalParamSaver(FractalParamSaver &&) = delete;
    FractalParamSaver &operator=(const FractalParamSaver &) = delete;
    FractalParamSaver &operator=(FractalParamSaver &&) = delete;

private:
    std::array<double, engine::MAX_PARAMS> m_params{};
    std::array<std::string, engine::MAX_PARAMS> m_param_text;
};

} // namespace id::test
