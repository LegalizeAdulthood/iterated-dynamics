// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

namespace id::engine
{

extern int g_max_rhombus_depth;
extern int g_rhombus_stack[];
extern int g_soi_min_stack;
extern int g_soi_min_stack_available;

class SOI
{
public:
    void calculate();
    bool iterate();

private:
    bool rhombus(double c_re1, double c_re2, double c_im1, double c_im2,     //
        int x1, int x2, int y1, int y2, long iter);
    bool rhombus_aux(double c_re1, double c_re2, double c_im1, double c_im2, //
        int x1, int x2, int y1, int y2, long iter);
    bool rhombus2(double cre1, double cre2, double cim1, double cim2,        //
        int x1, int x2, int y1, int y2,                                      //
        double zre1, double zim1, double zre2, double zim2,                  //
        double zre3, double zim3, double zre4, double zim4,                  //
        double zre5, double zim5, double zre6, double zim6,                  //
        double zre7, double zim7, double zre8, double zim8,                  //
        double zre9, double zim9, long iter);
    bool rhombus2(double cre1, double cre2, double cim1, double cim2,        //
        int x1, int x2, int y1, int y2,                                      //
        math::DComplex z1, math::DComplex z2,                                //
        math::DComplex z3, math::DComplex z4,                                //
        double zre5, double zim5, double zre6, double zim6,                  //
        double zre7, double zim7, double zre8, double zim8,                  //
        double zre9, double zim9, long iter);
    void soi_orbit(math::DComplex &z, math::DComplex &rq, double cr, double ci, bool &esc);

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

void soi();

} // namespace id::engine
