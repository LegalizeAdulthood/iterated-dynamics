// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "WorkListSnapshot.h"

#include <algorithm>

namespace id::test
{

WorkListSnapshot::WorkListSnapshot()
{
    std::copy_n(engine::g_work_list, engine::MAX_CALC_WORK, m_saved_work.begin());
}

WorkListSnapshot::~WorkListSnapshot()
{
    std::copy(m_saved_work.begin(), m_saved_work.end(), engine::g_work_list);
}

} // namespace id::test
