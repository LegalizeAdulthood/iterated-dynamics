// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::fractals
{

class TestPoint
{
public:
    TestPoint();
    TestPoint(const TestPoint &) = delete;
    TestPoint(TestPoint &&) = delete;
    ~TestPoint() = default;
    TestPoint &operator=(const TestPoint &) = delete;
    TestPoint &operator=(TestPoint &&) = delete;

    void resume();
    bool start();
    void suspend();
    bool done();
    void iterate();
    void finish();
    int per_pixel(
        double init_real, double init_imag, double param1, double param2, long max_iter, int inside);

    int start_pass{};
    int start_row{};
    int num_passes{};
    int passes{};
};

} // namespace id::fractals
