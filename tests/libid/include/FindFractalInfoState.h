// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <io/loadfile.h>
#include <misc/ValueSaver.h>
#include <ui/make_batch_file.h>

namespace id::test
{

class FindFractalInfoState
{
    misc::ValueSaver<int> m_saved_width{io::g_file_x_dots};
    misc::ValueSaver<int> m_saved_height{io::g_file_y_dots};
    misc::ValueSaver<int> m_saved_colors{io::g_file_colors};
    misc::ValueSaver<float> m_saved_aspect{io::g_file_aspect_ratio};
    misc::ValueSaver<bool> m_saved_make_parameter_file{ui::g_make_parameter_file, false};
};

} // namespace id::test
