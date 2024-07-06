#pragma once

#include "compiler.h"

namespace hc
{

class HelpCompiler : public Compiler
{
public:
    HelpCompiler(const Options &options);
    ~HelpCompiler() override;

    int process() override;

protected:
    void read_source_file();
    void make_hot_links();
    void paginate_html_document();
    void calc_offsets();

    Options m_options;

private:
    void usage();
    void compile();
    void print();
    void render_html();
    void paginate_online();
    void set_hot_link_doc_page();
    void set_content_doc_page();
    void paginate_document();
    void write_header();
    void write_link_source();
    void write_help();
    void report_stats();
    void add_hlp_to_exe();
    void delete_hlp_from_exe();
    void print_document();
    void report_memory();
    void print_html_document(const std::string &output_filename);
};

} // namespace hc
