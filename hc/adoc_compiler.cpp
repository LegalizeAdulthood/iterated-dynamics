#include "adoc_compiler.h"

#include "help_source.h"
#include "messages.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <helpcom.h>
#include <stdexcept>

namespace hc
{
std::string const DEFAULT_ADOC_FNAME = "index.adoc";

int AsciiDocCompiler::process()
{
    if (!m_options.fname2.empty())
    {
        throw std::runtime_error("Unexpected command-line argument \"" + m_options.fname2 + "\"");
    }

    g_src.buffer.resize(BUFFER_SIZE);
    read_source_file();

    make_hot_links();

    if (g_errors == 0)
    {
        paginate_html_document();
    }
    if (!g_errors)
    {
        calc_offsets();
    }
    if (!g_errors)
    {
        g_src.sort_labels();
    }
    if (g_errors == 0)
    {
        print_ascii_doc();
    }
    if (g_errors > 0 || g_warnings > 0)
    {
        report_errors();
    }

    return 0;
}

class AsciiDocProcessor
{
public:
    explicit AsciiDocProcessor(std::ostream &str) :
        m_str(str)
    {
    }
    ~AsciiDocProcessor() = default;

    void process();

private:
    bool info(PD_COMMANDS cmd, PD_INFO *pd);
    bool output(PD_COMMANDS cmd, PD_INFO *pd);
    void emit_char(char c);
    void emit_key_name();
    void print_inside_key(char c);
    void print_char(char c, int n);
    void print_string(char const *s, int n);

    std::ostream &m_str;
    int m_content_num{};
    int m_topic_num{};
    bool m_link_dest_warn{};
    int m_spaces{};
    int m_newlines{};
    bool m_start_of_line{};
    std::string m_content;
    bool m_inside_key{};
    std::string m_key_name;
    bool m_bullet_started{};
    bool m_indented_line{};
    bool m_inside_bullet{};
};

bool AsciiDocProcessor::info(PD_COMMANDS cmd, PD_INFO *pd)
{
    switch (cmd)
    {
    case PD_COMMANDS::PD_GET_CONTENT:
    {
        if (++m_content_num >= static_cast<int>(g_src.contents.size()))
        {
            return false;
        }
        const CONTENT &c{g_src.contents[m_content_num]};
        m_topic_num = -1;
        pd->id = c.id.c_str();
        m_content = std::string(c.indent + 2, '=') + ' ' + c.name;
        pd->title = m_content.c_str();
        pd->new_page = (c.flags & CF_NEW_PAGE) != 0;
        return true;
    }

    case PD_COMMANDS::PD_GET_TOPIC:
    {
        const CONTENT &c{g_src.contents[m_content_num]};
        if (++m_topic_num >= c.num_topic)
        {
            return false;
        }
        pd->curr = g_src.topics[c.topic_num[m_topic_num]].get_topic_text();
        pd->len = g_src.topics[c.topic_num[m_topic_num]].text_len;
        return true;
    }

    case PD_COMMANDS::PD_GET_LINK_PAGE:
    {
        const LINK &link = g_src.all_links[getint(pd->s)];
        if (link.doc_page == -1)
        {
            if (m_link_dest_warn)
            {
                g_current_src_filename = link.srcfile;
                g_src_line    = link.srcline;
                warn(0, "Hot-link destination is not in the document.");
                g_src_line = -1;
            }
            return false;
        }
        pd->i = g_src.all_links[getint(pd->s)].doc_page;
        return true;
    }

    case PD_COMMANDS::PD_RELEASE_TOPIC:
    {
        const CONTENT &c{g_src.contents[m_content_num]};
        g_src.topics[c.topic_num[m_topic_num]].release_topic_text(false);
        return true;
    }

    default:
        return false;
    }
}

bool AsciiDocProcessor::output(PD_COMMANDS cmd, PD_INFO *pd)
{
    switch (cmd)
    {
    case PD_COMMANDS::PD_HEADING:
    case PD_COMMANDS::PD_FOOTING:
        return true;

    case PD_COMMANDS::PD_PRINT:
        print_string(pd->s, pd->i);
        return true;

    case PD_COMMANDS::PD_PRINTN:
        print_char(*pd->s, pd->i);
        return true;

    case PD_COMMANDS::PD_PRINT_SEC:
        return true;

    case PD_COMMANDS::PD_START_SECTION:
        print_char('\n', 1);
        print_string(pd->title, std::strlen(pd->title));
        print_char('\n', 2);
        return true;

    case PD_COMMANDS::PD_START_TOPIC:
        return true;

    case PD_COMMANDS::PD_SET_SECTION_PAGE:
    case PD_COMMANDS::PD_SET_TOPIC_PAGE:
    case PD_COMMANDS::PD_PERIODIC:
        return true;

    default:
        return false;
    }
}

void AsciiDocProcessor::process()
{
    auto info_cb = [](PD_COMMANDS cmd, PD_INFO *pd, void *info)
    { return static_cast<AsciiDocProcessor *>(info)->info(cmd, pd); };
    auto output_cb = [](PD_COMMANDS cmd, PD_INFO *pd, void *info)
    { return static_cast<AsciiDocProcessor *>(info)->output(cmd, pd); };
    process_document(info_cb, output_cb, this);
}

static bool is_key_name(const std::string &name)
{
    auto is_function_key_name = [](const std::string &name)
    {
        return name[0] == 'F' && name.size() < 4 &&
            std::all_of(name.begin() + 1, name.end(), [](char c) { return std::isdigit(c) != 0; });
    };
    auto is_modified_key_name = [](const std::string &prefix, const std::string &name)
    {
        return name.substr(0, prefix.size()) == prefix && is_key_name(name.substr(prefix.size()));
    };

    return name.size() == 1                                                          //
        || name == "Enter" || name == "Esc" || name == "Delete" || name == "Insert"  //
        || name == "Tab" || name == "Space"                                          //
        || name == "Shift" || name == "Ctrl" || name == "Alt"                        //
        || name == "Keypad+" || name == "Keypad-"                                    //
        || name == "PageDown" || name == "PageUp" || name == "Home" || name == "End" //
        || name == "Left" || name == "Right" || name == "Up" || name == "Down"       //
        || name == "Fn" || name == "Arrow"                                           //
        || is_function_key_name(name)                                                //
        || is_modified_key_name("Shift+", name)                                      //
        || is_modified_key_name("Ctrl+", name)                                       //
        || is_modified_key_name("Alt+", name);
}

void AsciiDocProcessor::emit_char(char c)
{
    if (m_start_of_line)
    {
        m_indented_line = m_spaces > 0;
    }
    m_start_of_line = false;

    while (m_spaces > 0)
    {
        m_str << ' ';
        --m_spaces;
    }

    m_str << c;
    m_newlines = 0;
}

void AsciiDocProcessor::emit_key_name()
{
    for (char c : m_key_name)
    {
        emit_char(c);
    }
}

void AsciiDocProcessor::print_inside_key(char c)
{
    if (c == '>')
    {
        if (m_key_name.empty())
        {
            m_key_name += c;
        }
        else
        {
            m_inside_key = false;
            if (is_key_name(m_key_name))
            {
                if (m_key_name.back() == '\\' || m_key_name.back() == '+' || m_key_name.back() == '#')
                {
                    // some trailing special characters inside kbd:[] must be followed by a space
                    m_key_name += ' ';
                }
                else if (m_key_name.back() == ']')
                {
                    const char last = m_key_name.back();
                    m_key_name.pop_back();
                    m_key_name += R"(\])";
                }
                m_key_name = "kbd:[" + m_key_name + ']';
                emit_key_name();
            }
            else if (m_key_name.substr(0,4) == "http")
            {
                print_string(m_key_name.c_str(), static_cast<int>(m_key_name.size()));
            }
            else
            {
                emit_char('<');
                emit_key_name();
                emit_char('>');
            }
        }
    }
    else if (!std::isprint(c) || c == ' ')
    {
        m_inside_key = false;
        emit_char('<');
        emit_key_name();
        emit_char(c);
    }
    else
    {
        m_key_name += c;
    }
}

void AsciiDocProcessor::print_char(char c, int n)
{
    while (n-- > 0)
    {
        if (m_inside_key)
        {
            print_inside_key(c);
        }
        else if (m_start_of_line && c == 'o')
        {
            m_bullet_started = true;
            m_inside_bullet = true;
        }
        else if (c == '<' && (!m_indented_line || m_inside_bullet))
        {
            m_bullet_started = false;
            m_inside_key = true;
            m_key_name.clear();
        }
        else if (c == ' ')
        {
            if (m_bullet_started)
            {
                emit_char('*');
            }
            m_bullet_started = false;
            ++m_spaces;
        }
        else if (c == '\n' || c == '\f')
        {
            if (m_bullet_started)
            {
                emit_char('o');
            }
            m_bullet_started = false;
            ++m_newlines;
            m_start_of_line = true;
            m_indented_line = false;
            m_spaces = 0;   // strip spaces before a new-line
            if (m_newlines <= 2)
            {
                m_str << c;
            }
        }
        else 
        {
            if (m_bullet_started)
            {
                emit_char('o');
            }
            m_bullet_started = false;
            emit_char(c);
        }
    }
}

void AsciiDocProcessor::print_string(char const *s, int n)
{
    if (n > 0)
    {
        while (n-- > 0)
        {
            print_char(*s++, 1);
        }
    }
    else
    {
        while (*s != '\0')
        {
            print_char(*s++, 1);
        }
    }
}

void AsciiDocCompiler::print_ascii_doc()
{
    if (g_src.contents.empty())
    {
        throw std::runtime_error(".SRC has no DocContents.");
    }

    const std::string fname{m_options.fname2.empty() ? DEFAULT_ADOC_FNAME : m_options.fname2};

    msg(("Writing " + fname).c_str());

    const CONTENT &toc = g_src.contents[0];
    if (toc.num_topic != 1)
    {
        throw std::runtime_error("First content block contains multiple topics.");
    }
    if (toc.topic_name[0] != DOCCONTENTS_TITLE)
    {
        throw std::runtime_error("First content block doesn't contain DocContent.");
    }

    const TOPIC &toc_topic = g_src.topics[toc.topic_num[0]];
    const std::filesystem::path out_file{std::filesystem::path{m_options.output_dir} / fname};
    std::ofstream str{out_file};
    if (!str)
    {
        throw std::runtime_error("Couldn't open output file " + out_file.string());
    }
    str << "= Iterated Dynamics\n"
           ":toc: left\n"
           ":experimental:\n";

    AsciiDocProcessor(str).process();
}

} // namespace hc
