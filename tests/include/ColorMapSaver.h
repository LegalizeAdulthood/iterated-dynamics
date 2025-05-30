// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>
#include <ui/rotate.h>

class ColorMapSaver
{
public:
    ColorMapSaver()
    {
        for (int i = 0; i < 256; ++i)
        {
            m_map[i][0] = g_dac_box[i][0];
            m_map[i][1] = g_dac_box[i][1];
            m_map[i][2] = g_dac_box[i][2];
        }
    }

    ~ColorMapSaver()
    {
        for (int i = 0; i < 256; ++i)
        {
            g_dac_box[i][0] = m_map[i][0];
            g_dac_box[i][1] = m_map[i][1];
            g_dac_box[i][2] = m_map[i][2];
        }
    }

    ColorMapSaver(const ColorMapSaver &rhs) = delete;
    ColorMapSaver(ColorMapSaver &&rhs) = delete;
    ColorMapSaver &operator=(const ColorMapSaver &rhs) = delete;
    ColorMapSaver &operator=(ColorMapSaver &&rhs) = delete;

private:
    Byte m_map[256][3];
};
