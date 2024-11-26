// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

namespace hc
{

struct Content;
struct Topic;

extern std::string g_html_output_dir;

class HTMLProcessor
{
public:
    explicit HTMLProcessor(std::string const &fname)
        : m_fname(fname)
    {
    }

    void process();

private:
    void write_index_html();
    void write_contents();
    void write_content(const Content &c);
    void write_topic(const Topic &t);

    std::string m_fname;
};

std::string rst_name(std::string const &content_name);

} // namespace hc
