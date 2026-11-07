// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/load_config.h>

#include <engine/pixel_limits.h>

#include <array>
#include <cstdlib>
#include <istream>
#include <string>
#include <string_view>

using namespace id::engine;

namespace id::io
{

VideoConfigReader::VideoConfigReader(std::istream &input) :
    m_input(input)
{
}

bool VideoConfigReader::next(VideoConfigEntry &entry)
{
    if (m_failed)
    {
        return false;
    }

    std::string line;
    while (std::getline(m_input, line))
    {
        ++m_line_num;
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (!line.empty() && line[0] == ';')
        {
            continue; // comment line
        }
        if (line.size() > 120)
        {
            line.resize(120);
        }
        for (char &ch : line)
        {
            if (static_cast<unsigned char>(ch) < ' ')
            {
                ch = ' '; // convert tab (or whatever) to blank
            }
        }
        // key, 0: x, 1: y, 2: colors, 3: driver, 4: comments
        std::string_view line_view{line};
        const std::size_t key_end{line_view.find(',')};
        if (key_end == std::string_view::npos)
        {
            m_failed = true;
            return false;
        }
        const std::string_view key_name{line_view.substr(0, key_end)};
        std::array<std::string_view, 5> fields{};
        std::size_t field_start{key_end + 1};
        for (int field = 0; field < 4; ++field)
        {
            const std::size_t field_end{line_view.find(',', field_start)};
            if (field_end == std::string_view::npos)
            {
                m_failed = true;
                return false;
            }
            fields[field] = line_view.substr(field_start, field_end - field_start);
            field_start = field_end + 1;
        }
        fields[4] = line_view.substr(field_start);

        const std::string x_field{fields[0]};
        const long x_dots = std::atol(x_field.c_str());
        const std::string y_field{fields[1]};
        const long y_dots = std::atol(y_field.c_str());
        const std::string colors_field{fields[2]};
        const int colors = std::atoi(colors_field.c_str());

        if (x_dots < MIN_PIXELS || x_dots > GIF_MAX_PIXELS || y_dots < MIN_PIXELS || y_dots > GIF_MAX_PIXELS ||
            (colors != 0 && colors != 2 && colors != 4 && colors != 16 && colors != 256))
        {
            m_failed = true;
            return false;
        }
        entry = {std::string{key_name}, static_cast<int>(x_dots), static_cast<int>(y_dots), colors,
            std::string{fields[3]}, std::string{fields[4]}, m_line_num};
        return true;
    }
    return false;
}

bool VideoConfigReader::failed() const
{
    return m_failed;
}

} // namespace id::io
