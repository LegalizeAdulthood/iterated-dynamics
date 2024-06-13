#pragma once

#include <string>

namespace hc
{

enum class modes
{
    NONE = 0,
    COMPILE,
    PRINT,
    APPEND,
    DELETE,
    HTML
};

/*
 * command-line parser, etc.
 */
struct compiler_options
{
    modes mode{modes::NONE};
    std::string fname1;
    std::string fname2;
    std::string swappath;
    bool show_mem{};
    bool show_stats{};
};

class compiler
{
public:
    compiler(int argc_, char *argv_[]);
    ~compiler();

    int process();

private:
    void parse_arguments();
    void read_source_file(modes mode);
    void usage();
    void compile();
    void print();
    void render_html();
    void paginate_html_document();
    void print_html_document(std::string const &output_filename);

    int argc;
    char **argv;
    compiler_options m_options;
};

} // namespace hc
