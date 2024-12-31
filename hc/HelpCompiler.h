// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "Compiler.h"
#include "Options.h"

#include <cstdio>

namespace hc
{

class HelpCompiler : public Compiler
{
public:
    explicit HelpCompiler(const Options &options);
    ~HelpCompiler() override;

    int process() override;

protected:
    void read_source_file();
    void make_hot_links();
    void calc_offsets();

    Options m_options;
    int m_max_pages{};                   // max. pages in any topic

private:
    void usage();
    void compile();
    void print();
    void paginate_online();
    void set_hot_link_doc_page();
    void set_content_doc_page();
    void paginate_document();
    void write_header();
    void write_link_source();
    void write_help(std::FILE *file);
    void write_help();
    void report_stats();
    void add_hlp_to_exe();
    void delete_hlp_from_exe();
    void print_document();
    void report_memory();
};

} // namespace hc
