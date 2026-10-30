// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <engine/work_list.h>
#include <misc/ValueSaver.h>

#include <array>

namespace id::test
{

class WorkListSnapshot
{
public:
    WorkListSnapshot();
    ~WorkListSnapshot();

private:
    misc::ValueSaver<int> m_saved_count{engine::g_num_work_list, 0};
    std::array<engine::WorkList, engine::MAX_CALC_WORK> m_saved_work{};
};

} // namespace id::test
