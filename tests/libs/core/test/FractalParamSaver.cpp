// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <FractalParamSaver.h>

#include <algorithm>

namespace id::test
{

FractalParamSaver::FractalParamSaver()
{
    std::copy(&engine::g_params[0], &engine::g_params[engine::MAX_PARAMS], m_params.begin());
    m_param_text = engine::g_param_text;
}

FractalParamSaver::~FractalParamSaver()
{
    std::copy(m_params.begin(), m_params.end(), &engine::g_params[0]);
    engine::g_param_text = m_param_text;
}

} // namespace id::test
