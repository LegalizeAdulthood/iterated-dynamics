// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/zoom.h>

#include <engine/calcfrac.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <ostream>

using namespace id::engine;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

namespace id::test
{

struct ResumeCase
{
    bool translated;
    double zoom_width;
    CalcStatus status;
    CalcStatus expected;
};

void PrintTo(const ResumeCase &value, std::ostream *out)
{
    *out << "translated=" << value.translated << ", width=" << value.zoom_width
         << ", status=" << static_cast<int>(value.status);
}

class TestZoomResume : public TestWithParam<ResumeCase>
{
};

TEST_P(TestZoomResume, invalidatesOnlyTranslatedResumableWorkWithFullSizeBox)
{
    const ResumeCase &value{GetParam()};
    ValueSaver saved_status{g_calc_status, value.status};
    ValueSaver saved_width{g_zoom_box_width, value.zoom_width};

    invalidate_translated_resume(value.translated);

    EXPECT_EQ(value.expected, g_calc_status);
}

INSTANTIATE_TEST_SUITE_P(BoundsAdjustment, TestZoomResume,
    Values(ResumeCase{true, 1.0, CalcStatus::RESUMABLE, CalcStatus::PARAMS_CHANGED},
        ResumeCase{false, 1.0, CalcStatus::RESUMABLE, CalcStatus::RESUMABLE},
        ResumeCase{true, 0.0, CalcStatus::RESUMABLE, CalcStatus::RESUMABLE},
        ResumeCase{true, 0.5, CalcStatus::RESUMABLE, CalcStatus::RESUMABLE},
        ResumeCase{true, 2.0, CalcStatus::RESUMABLE, CalcStatus::RESUMABLE},
        ResumeCase{true, 1.0, CalcStatus::NO_FRACTAL, CalcStatus::NO_FRACTAL},
        ResumeCase{true, 1.0, CalcStatus::PARAMS_CHANGED, CalcStatus::PARAMS_CHANGED},
        ResumeCase{true, 1.0, CalcStatus::IN_PROGRESS, CalcStatus::IN_PROGRESS},
        ResumeCase{true, 1.0, CalcStatus::NON_RESUMABLE, CalcStatus::NON_RESUMABLE},
        ResumeCase{true, 1.0, CalcStatus::COMPLETED, CalcStatus::COMPLETED}));

} // namespace id::test
