// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <iosfwd>
#include <string>

namespace id::io
{

struct VideoConfigEntry
{
    std::string key_name;
    int x_dots{};
    int y_dots{};
    int colors{};
    std::string driver_name;
    std::string comment;
    int line_num{};
};

class VideoConfigReader
{
public:
    explicit VideoConfigReader(std::istream &input);
    // Read one record; false means EOF or malformed input. Check failed().
    bool next(VideoConfigEntry &entry);
    bool failed() const;

private:
    std::istream &m_input;
    int m_line_num{};
    bool m_failed{};
};

} // namespace id::io
