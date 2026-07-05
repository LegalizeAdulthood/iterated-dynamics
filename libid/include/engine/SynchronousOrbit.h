// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

#include <array>
#include <vector>

namespace id::engine
{

extern int g_max_rhombus_depth;
extern int g_rhombus_stack[];
extern int g_soi_min_stack;
extern int g_soi_min_stack_available;

class SynchronousOrbit
{
public:
    void calculate();
    bool iterate();

private:
    struct RhombusInput
    {
        double c_re1{};
        double c_re2{};
        double c_im1{};
        double c_im2{};
        int x1{};
        int x2{};
        int y1{};
        int y2{};
        long iter{};
        int depth{};
        std::array<math::DComplex, 9> zi{};
    };

    enum class RhombusPhase
    {
        START,
        CHILDREN
    };

    enum class RhombusStep
    {
        COMPLETE,
        CHILDREN
    };

    struct RhombusFrame
    {
        RhombusInput input{};
        RhombusPhase phase{RhombusPhase::START};
        std::array<RhombusInput, 4> children{};
        int child_index{};
    };

    static RhombusInput make_rhombus_input(double c_re1, double c_re2, double c_im1, double c_im2, //
        int x1, int x2, int y1, int y2,                                                            //
        math::DComplex z1, math::DComplex z2, math::DComplex z3, math::DComplex z4,                //
        math::DComplex z5, math::DComplex z6, math::DComplex z7, math::DComplex z8,                //
        math::DComplex z9, long iter, int depth);
    void push_rhombus(const RhombusInput &input);
    bool process_next_frame();
    RhombusStep process_rhombus(RhombusFrame &frame);
    void orbit(math::DComplex &z, math::DComplex &rq, double cr, double ci, bool &esc);

    std::vector<RhombusFrame> m_frame_stack{};
    math::DComplex m_zi[9]{};
    bool m_esc[9]{};
    bool m_t_esc[4]{};
    math::DComplex m_z{};
    math::DComplex m_step{};
    double m_interleave_step{};
    double m_help_real{};
    math::DComplex m_scan_z{};
    math::DComplex m_b1[3]{};
    math::DComplex m_b2[3]{};
    math::DComplex m_b3[3]{};
    math::DComplex m_limit{};
    math::DComplex m_rq[9]{};
    math::DComplex m_corner[2]{};
    math::DComplex m_tz[4]{};
    math::DComplex m_tq[4]{};
    double m_t_width{};
    double m_equal{};
    int m_rhombus_depth{-1};
};

} // namespace id::engine
