#pragma once

#include "options.h"

#include <string>

namespace hc
{

class compiler
{
public:
    compiler(const Options &options);
    ~compiler();

    int process();

private:
    void read_source_file();
    void usage();
    void compile();
    void print();
    void render_html();
    void paginate_html_document();
    void print_html_document(const std::string &output_filename);

    Options m_options;
};

} // namespace hc
