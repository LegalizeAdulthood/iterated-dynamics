// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <array>
#include <vector>

namespace id::engine
{

class Tesseral
{
public:
    Tesseral();

    bool done() const;
    bool iterate();
    void suspend();

private:
    enum class InitPhase
    {
        TOP,
        BOTTOM,
        LEFT,
        RIGHT,
        COMPLETE,
    };

    enum class EdgeProbeKind
    {
        NONE,
        ROW,
        COLUMN,
    };

    enum class SplitCheckPhase
    {
        START,
        TOP,
        BOTTOM,
        LEFT,
        RIGHT,
        MIDDLE,
    };

    enum class FillMode
    {
        NONE,
        PIXEL,
        SPAN,
    };

    struct Tess
    {
        int x1{};
        int x2{};
        int y1{};
        int y2{};
        int top{};
        int bot{};
        int lft{};
        int rgt{};
    };

    struct EdgeProbe
    {
        EdgeProbeKind kind{EdgeProbeKind::NONE};
        int fixed{};
        int current{};
        int final{};
        int color{};
        bool started{};
    };

    struct FillState
    {
        FillMode mode{FillMode::NONE};
        int col{};
        int row{};
        std::vector<unsigned char> pixels{};
    };

    bool advance_edge_probe();
    bool fill_box();
    void finish_fill_box();
    bool finish_edge_probe(int &color);
    bool initialize_edges();
    void next_box();
    bool probe_active() const;
    void reset_edge_probe();
    void reset_split_check();
    void start_fill_box();
    void start_column_probe(int x, int y1, int y2);
    void start_row_probe(int x1, int x2, int y);
    bool split_box();
    bool split_needed(bool &needed);

    bool m_guess_plot{};
    bool m_interrupted{};
    bool m_split_box_active{};
    bool m_split_box_vertical{};
    int m_split_box_mid{};
    InitPhase m_init_phase{InitPhase::COMPLETE};
    SplitCheckPhase m_split_check_phase{SplitCheckPhase::START};
    EdgeProbe m_edge_probe{};
    FillState m_fill{};
    std::array<Tess, 4096 / sizeof(Tess)> m_stack{};
    Tess *m_tp{};
};

int tesseral();

} // namespace id::engine
