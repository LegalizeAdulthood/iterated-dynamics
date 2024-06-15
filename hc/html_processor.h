#pragma once

#include <string>

namespace hc
{

struct CONTENT;
struct TOPIC;

extern std::string g_html_output_dir;

class html_processor
{
public:
    html_processor(std::string const &fname)
        : m_fname(fname)
    {
    }

    void process();

private:
    void write_index_html();
    void write_contents();
    void write_content(const CONTENT &c);
    void write_topic(const TOPIC &t);

    std::string m_fname;
};

} // namespace hc
