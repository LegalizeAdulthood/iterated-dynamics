// SPDX-License-Identifier: GPL-3.0-only
//
#include "AsciiDocCompiler.h"

#include "HelpSource.h"
#include "messages.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <helpcom.h>
#include <stdexcept>

namespace hc
{
std::string const DEFAULT_ADOC_FNAME = "id.adoc";

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
    void set_link_text(const LINK &link, const ProcessDocumentInfo *pd);
    bool info(PD_COMMANDS cmd, ProcessDocumentInfo *pd);
    bool output(PD_COMMANDS cmd, ProcessDocumentInfo *pd);
    void emit_char(char c);
    void emit_key_name();
    void print_inside_key(char c);
    void print_char(char c, int n);
    void print_string(char const *s, int n);
    void print_string(const std::string &text)
    {
        print_string(text.c_str(), static_cast<int>(text.size()));
    }

    std::ostream &m_str;
    int m_content_num{};
    int m_topic_num{};
    bool m_link_dest_warn{};
    int m_spaces{};
    int m_newlines{};
    bool m_start_of_line{};
    std::string m_content;
    std::string m_topic;
    bool m_inside_key{};
    std::string m_key_name;
    bool m_bullet_started{};
    bool m_indented_line{};
    bool m_inside_bullet{};
    std::string m_link_text;
    std::string m_link_markup;
};

bool AsciiDocProcessor::info(PD_COMMANDS cmd, ProcessDocumentInfo *pd)
{
    switch (cmd)
    {
    case PD_COMMANDS::PD_GET_CONTENT:
    {
        if (++m_content_num >= static_cast<int>(g_src.contents.size()))
        {
            return false;
        }
        const CONTENT &content{g_src.contents[m_content_num]};
        m_topic_num = -1;
        pd->id = content.id.c_str();
        m_content = std::string(content.indent + 2, '=') + ' ' + content.name;
        pd->title = m_content.c_str();
        pd->new_page = (content.flags & CF_NEW_PAGE) != 0;
        return true;
    }

    case PD_COMMANDS::PD_GET_TOPIC:
    {
        const CONTENT &content{g_src.contents[m_content_num]};
        if (++m_topic_num >= content.num_topic)
        {
            return false;
        }
        const TOPIC &topic{g_src.topics[content.topic_num[m_topic_num]]};
        if (topic.title != content.name)
        {
            m_topic = std::string(content.indent + 3, '=') + ' ' + topic.title;
        }
        else
        {
            m_topic.clear();
        }
        pd->curr = topic.get_topic_text();
        pd->len = topic.text_len;
        return true;
    }

    case PD_COMMANDS::PD_GET_LINK_PAGE:
    {
        const LINK &link{g_src.all_links[getint(pd->s)]};
        if (link.doc_page == -1)
        {
            if (m_link_dest_warn)
            {
                g_current_src_filename = link.srcfile;
                g_src_line = link.srcline;
                warn(0, "Hot-link destination is not in the document.");
                g_src_line = -1;
            }
            return false;
        }

        set_link_text(link, pd);
        pd->i = link.doc_page;
        pd->link_page.clear();
        return true;
    }

    case PD_COMMANDS::PD_RELEASE_TOPIC:
    {
        const CONTENT &content{g_src.contents[m_content_num]};
        const TOPIC &topic{g_src.topics[content.topic_num[m_topic_num]]};
        topic.release_topic_text(false);
        return true;
    }

    default:
        return false;
    }
}

bool AsciiDocProcessor::output(PD_COMMANDS cmd, ProcessDocumentInfo *pd)
{
    switch (cmd)
    {
    case PD_COMMANDS::PD_PRINT:
        print_string(pd->s, pd->i);
        return true;

    case PD_COMMANDS::PD_PRINTN:
        print_char(*pd->s, pd->i);
        return true;

    case PD_COMMANDS::PD_START_SECTION:
        print_char('\n', 1);
        print_string(pd->title, std::strlen(pd->title));
        print_char('\n', 2);
        return true;

    case PD_COMMANDS::PD_START_TOPIC:
        if (!m_topic.empty())
        {
            print_char('\n', 1);
            print_string(m_topic.data(), static_cast<int>(m_topic.length()));
            print_char('\n', 2);
        }
        return true;

    case PD_COMMANDS::PD_HEADING:
    case PD_COMMANDS::PD_FOOTING:
    case PD_COMMANDS::PD_SET_SECTION_PAGE:
    case PD_COMMANDS::PD_SET_TOPIC_PAGE:
    case PD_COMMANDS::PD_PERIODIC:
    case PD_COMMANDS::PD_PRINT_SEC:
        return true;

    default:
        return false;
    }
}

void AsciiDocProcessor::process()
{
    const auto info_cb = [](PD_COMMANDS cmd, ProcessDocumentInfo *pd, void *info)
    { return static_cast<AsciiDocProcessor *>(info)->info(cmd, pd); };
    const auto output_cb = [](PD_COMMANDS cmd, ProcessDocumentInfo *pd, void *info)
    { return static_cast<AsciiDocProcessor *>(info)->output(cmd, pd); };
    process_document(token_modes::ADOC, info_cb, output_cb, this);
}

static std::string to_string(link_types type)
{
    switch (type)
    {
    case link_types::LT_TOPIC:
        return "LT_TOPIC";
    case link_types::LT_LABEL:
        return "LT_LABEL";
    case link_types::LT_SPECIAL:
        return "LT_SPECIAL";
    }
    return "? (" + std::to_string(static_cast<int>(type)) + ")";
}

void AsciiDocProcessor::set_link_text(const LINK &link, const ProcessDocumentInfo *pd)
{
    std::string anchor_name;
    switch (link.type)
    {
    case link_types::LT_TOPIC:
        anchor_name = link.name;
        break;
    case link_types::LT_LABEL:
    {
        const LABEL *label = g_src.find_label(link.name.c_str());
        const TOPIC &topic = g_src.topics[label->topic_num];
        anchor_name = topic.title;
        break;
    }
    default:
        throw std::runtime_error("Unknown link type " + to_string(link.type));
    }

    const char *begin{pd->s + sizeof(int) * 3};
    const size_t len{pd->i - sizeof(int) * 3 - 2};
    m_link_text.assign(begin, len);
    if (const auto first_non_space{m_link_text.find_first_not_of(' ')}; first_non_space != 0)
    {
        m_link_text.erase(0, first_non_space);
    }
    if (const auto last_non_space{m_link_text.find_last_not_of(' ')}; last_non_space != m_link_text.length() - 1)
    {
        m_link_text.erase(last_non_space + 1);
    }
    m_link_markup = boost::algorithm::to_lower_copy(anchor_name);
    for (const char c : " .-")
    {
        std::replace(m_link_markup.begin(), m_link_markup.end(), c, '_');
    }
    constexpr const char *BAD_CHARS{R"bad_chars(=|/()<>@")bad_chars"};
    for (auto pos = m_link_markup.find_first_of(BAD_CHARS); pos != std::string::npos;
         pos = m_link_markup.find_first_of(BAD_CHARS, pos))
    {
        m_link_markup.erase(pos, 1);
    }
    boost::algorithm::replace_all(m_link_markup, "__", "_");
    m_link_markup = "<<_" + m_link_markup;
    if (m_link_text != anchor_name)
    {
        m_link_markup += "," + m_link_text;
    }
    m_link_markup += ">>";
}

static bool is_key_name(const std::string &name)
{
    auto is_function_key_name = [](const std::string &name)
    {
        return name[0] == 'F' && name.size() < 4 &&
            std::all_of(name.begin() + 1, name.end(), [](char c) { return std::isdigit(c) != 0; });
    };
    auto is_modified_key_name = [](const std::string &prefix, const std::string &name)
    { return name.substr(0, prefix.size()) == prefix && is_key_name(name.substr(prefix.size())); };

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
            else if (m_key_name.substr(0, 4) == "http")
            {
                print_string(m_key_name);
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
        else if (c == '<' && m_link_text.empty() && (!m_indented_line || m_inside_bullet))
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
                m_bullet_started = false;
            }
            if (!m_link_text.empty() && m_link_text.front() == ' ')
            {
                m_link_text.erase(0, 1);
            }
            else
            {
                ++m_spaces;
            }
        }
        else if (c == '\n' || c == '\f')
        {
            if (m_bullet_started)
            {
                emit_char('o');
                m_bullet_started = false;
            }
            ++m_newlines;
            m_start_of_line = true;
            m_indented_line = false;
            m_spaces = 0; // strip spaces before a new-line
            if (m_newlines <= 2)
            {
                m_str << c;
            }
            while (!m_link_text.empty() && m_link_text.front() == ' ')
            {
                m_link_text.erase(0, 1);
            }
        }
        else
        {
            if (m_bullet_started)
            {
                emit_char('o');
                m_bullet_started = false;
            }

            if (!m_link_text.empty())
            {
                if (c == m_link_text.front())
                {
                    m_link_text.erase(0, 1);
                }
                else
                {
                     throw std::runtime_error("Unexpected character '" + std::string{c} + "'");
                }
                if (m_link_text.empty())
                {
                    print_string(m_link_markup);
                }
                m_start_of_line = false;
            }
            else
            {
                emit_char(c);
            }
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

    const std::filesystem::path out_file{std::filesystem::path{m_options.output_dir} / fname};
    std::ofstream str{out_file};
    if (!str)
    {
        throw std::runtime_error("Couldn't open output file " + out_file.string());
    }
    str << "= Iterated Dynamics\n"
           ":toc: left\n"
           ":toclevels: 4\n"
           ":experimental:\n";

    AsciiDocProcessor(str).process();
}

} // namespace hc
