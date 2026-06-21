// SPDX-License-Identifier: GPL-3.0-only
//
#include "AsciiDocCompiler.h"

#include "HelpSource.h"
#include "messages.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <helpcom.h>
#include <map>
#include <stdexcept>

using namespace id::help;

namespace hc
{
const std::string DEFAULT_ADOC_FNAME = "id.adoc";

int AsciiDocCompiler::process()
{
    if (!m_options.fname2.empty())
    {
        throw std::runtime_error(R"msg(Unexpected command-line argument ")msg" + m_options.fname2 + '"');
    }

    g_src.buffer.resize(BUFFER_SIZE);
    read_source_file();

    make_hot_links();

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

    return g_errors;
}

static bool is_link_in_ascii_doc(const Link &link)
{
    return link.doc_page != -1;
}

static void report_link_not_in_ascii_doc(const Link &link)
{
    g_current_src_filename = link.src_file;
    g_src_line = link.src_line;
    MSG_ERROR(0, "Hot-link destination is not in the AsciiDoc output.");
    g_src_line = -1;
}

static std::string make_content_anchor(const int content_num)
{
    return "id-content-" + std::to_string(content_num);
}

static std::string make_readable_anchor(const std::string &title)
{
    std::string anchor;
    bool last_was_underscore{};
    for (const char c : title)
    {
        const unsigned char ch{static_cast<unsigned char>(c)};
        if (std::isalnum(ch) != 0)
        {
            anchor += static_cast<char>(std::tolower(ch));
            last_was_underscore = false;
        }
        else if (!last_was_underscore)
        {
            anchor += '_';
            last_was_underscore = true;
        }
    }
    while (!anchor.empty() && anchor.front() == '_')
    {
        anchor.erase(0, 1);
    }
    while (!anchor.empty() && anchor.back() == '_')
    {
        anchor.pop_back();
    }
    return anchor;
}

static bool is_valid_asciidoc_anchor(const std::string &anchor)
{
    if (anchor.empty() || (std::isalpha(static_cast<unsigned char>(anchor[0])) == 0 && anchor[0] != '_'))
    {
        return false;
    }

    return std::all_of(anchor.begin(), anchor.end(),
        [](const unsigned char ch) { return std::isalnum(ch) != 0 || ch == '_' || ch == '-'; });
}

static std::vector<std::string> make_topic_anchors()
{
    std::vector<std::string> anchors(g_src.topics.size());
    std::map<std::string, int> anchor_topics;
    for (int topic_num = 0; topic_num < static_cast<int>(g_src.topics.size()); ++topic_num)
    {
        const Topic &topic{g_src.topics[topic_num]};
        if (!bit_set(topic.flags, TopicFlags::IN_DOC))
        {
            continue;
        }

        const std::string anchor{topic.adoc_anchor.empty() ? make_readable_anchor(topic.title) : topic.adoc_anchor};
        if (anchor.empty())
        {
            MSG_ERROR(0, "AsciiDoc anchor for topic title \"%s\" is empty.", topic.title.c_str());
            continue;
        }
        if (!is_valid_asciidoc_anchor(anchor))
        {
            MSG_ERROR(
                0, "AsciiDoc anchor \"%s\" for topic title \"%s\" is invalid.", anchor.c_str(), topic.title.c_str());
            continue;
        }

        const auto [it, inserted]{anchor_topics.emplace(anchor, topic_num)};
        if (!inserted)
        {
            MSG_ERROR(0, "AsciiDoc anchor collision for topic titles \"%s\" and \"%s\".",
                g_src.topics[it->second].title.c_str(), topic.title.c_str());
            continue;
        }

        anchors[topic_num] = anchor;
    }
    return anchors;
}

class AsciiDocProcessor
{
public:
    explicit AsciiDocProcessor(std::ostream &str, const std::vector<std::string> &topic_anchors) :
        m_str(str),
        m_topic_anchors(topic_anchors)
    {
    }

    AsciiDocProcessor(const AsciiDocProcessor &rhs) = delete;
    AsciiDocProcessor(AsciiDocProcessor &&rhs) = delete;
    ~AsciiDocProcessor() = default;
    AsciiDocProcessor &operator=(const AsciiDocProcessor &rhs) = delete;
    AsciiDocProcessor &operator=(AsciiDocProcessor &&rhs) = delete;

    void process();

private:
    void set_link_text(const Link &link, const ProcessDocumentInfo *pd);
    bool info(PrintDocCommand cmd, ProcessDocumentInfo *pd);
    bool output(PrintDocCommand cmd, ProcessDocumentInfo *pd);
    void emit_char(char c);
    void emit_key_name();
    void flush_bullet_candidate(bool bullet);
    void print_inside_key(char c);
    void print_char(char c, int n);
    void print_string(const char *s, int n);
    void print_anchor(const std::string &anchor);
    void print_raw_char(char c);
    void update_raw_block_state();

    void print_string(const std::string &text)
    {
        print_string(text.c_str(), static_cast<int>(text.size()));
    }

    std::ostream &m_str;
    const std::vector<std::string> &m_topic_anchors;
    int m_content_num{};
    int m_topic_num{};
    int m_spaces{};
    int m_newlines{};
    bool m_start_of_line{};
    std::string m_content_anchor;
    std::string m_section_topic_anchor;
    std::string m_topic_anchor;
    std::string m_content;
    std::string m_topic;
    bool m_inside_key{};
    std::string m_key_name;
    bool m_bullet_started{};
    bool m_bullet_indented{};
    int m_bullet_spaces{};
    std::string m_bullet_probe;
    bool m_indented_line{};
    bool m_inside_bullet{};
    std::string m_link_text;
    std::string m_link_markup;
    std::string m_line_text;
    bool m_inside_raw_block{};
    std::string m_raw_block_delimiter;
};

bool AsciiDocProcessor::info(const PrintDocCommand cmd, ProcessDocumentInfo *pd)
{
    switch (cmd)
    {
    case PrintDocCommand::PD_GET_CONTENT:
    {
        if (++m_content_num >= static_cast<int>(g_src.contents.size()))
        {
            return false;
        }
        const Content &content{g_src.contents[m_content_num]};
        m_topic_num = -1;
        m_content_anchor = make_content_anchor(m_content_num);
        m_section_topic_anchor.clear();
        if (content.num_topic > 0)
        {
            const Topic &topic{g_src.topics[content.topic_num[0]]};
            if (topic.title == content.name)
            {
                m_section_topic_anchor = m_topic_anchors[content.topic_num[0]];
            }
        }
        pd->id = content.id.c_str();
        m_content = std::string(content.indent + 2, '=') + ' ' + content.name;
        pd->title = m_content.c_str();
        pd->new_page = (content.flags & CF_NEW_PAGE) != 0;
        return true;
    }

    case PrintDocCommand::PD_GET_TOPIC:
    {
        const Content &content{g_src.contents[m_content_num]};
        if (++m_topic_num >= content.num_topic)
        {
            return false;
        }
        const Topic &topic{g_src.topics[content.topic_num[m_topic_num]]};
        m_topic_anchor = m_topic_anchors[content.topic_num[m_topic_num]];
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

    case PrintDocCommand::PD_GET_LINK_PAGE:
    {
        const Link &link{g_src.all_links[get_int(pd->s)]};
        if (!is_link_in_ascii_doc(link))
        {
            report_link_not_in_ascii_doc(link);
            return false;
        }

        set_link_text(link, pd);
        pd->link_page.clear();
        return true;
    }

    case PrintDocCommand::PD_RELEASE_TOPIC:
    {
        const Content &content{g_src.contents[m_content_num]};
        const Topic &topic{g_src.topics[content.topic_num[m_topic_num]]};
        topic.release_topic_text(false);
        return true;
    }

    default:
        return false;
    }
}

bool AsciiDocProcessor::output(const PrintDocCommand cmd, ProcessDocumentInfo *pd)
{
    switch (cmd)
    {
    case PrintDocCommand::PD_PRINT:
        print_string(pd->s, pd->i);
        return true;

    case PrintDocCommand::PD_PRINT_N:
        print_char(*pd->s, pd->i);
        return true;

    case PrintDocCommand::PD_START_SECTION:
        print_char('\n', 1);
        print_anchor(m_content_anchor);
        if (!m_section_topic_anchor.empty())
        {
            print_anchor(m_section_topic_anchor);
        }
        print_string(m_content);
        print_char('\n', 2);
        return true;

    case PrintDocCommand::PD_START_TOPIC:
        if (!m_topic.empty())
        {
            print_char('\n', 1);
            print_anchor(m_topic_anchor);
            print_string(m_topic.data(), static_cast<int>(m_topic.length()));
            print_char('\n', 2);
        }
        return true;

    case PrintDocCommand::PD_HEADING:
    case PrintDocCommand::PD_FOOTING:
    case PrintDocCommand::PD_SET_SECTION_PAGE:
    case PrintDocCommand::PD_SET_TOPIC_PAGE:
    case PrintDocCommand::PD_PERIODIC:
    case PrintDocCommand::PD_PRINT_SEC:
        return true;

    default:
        return false;
    }
}

void AsciiDocProcessor::process()
{
    const auto info_cb = [](const PrintDocCommand cmd, ProcessDocumentInfo *pd, void *info)
    { return static_cast<AsciiDocProcessor *>(info)->info(cmd, pd); };
    const auto output_cb = [](const PrintDocCommand cmd, ProcessDocumentInfo *pd, void *info)
    { return static_cast<AsciiDocProcessor *>(info)->output(cmd, pd); };
    process_document(TokenMode::ADOC, false, info_cb, output_cb, this);
}

static std::string to_string(LinkTypes type)
{
    switch (type)
    {
    case LinkTypes::LT_TOPIC:
        return "LT_TOPIC";
    case LinkTypes::LT_LABEL:
        return "LT_LABEL";
    case LinkTypes::LT_SPECIAL:
        return "LT_SPECIAL";
    }
    return "? (" + std::to_string(static_cast<int>(type)) + ")";
}

void AsciiDocProcessor::set_link_text(const Link &link, const ProcessDocumentInfo *pd)
{
    std::string anchor_title;
    switch (link.type)
    {
    case LinkTypes::LT_TOPIC:
        anchor_title = g_src.topics[link.topic_num].title;
        break;
    case LinkTypes::LT_LABEL:
        anchor_title = g_src.topics[link.topic_num].title;
        break;
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
    m_link_markup = "<<" + m_topic_anchors[link.topic_num];
    if (m_link_text != anchor_title)
    {
        m_link_markup += "," + m_link_text;
    }
    m_link_markup += ">>";
}

static bool is_single_character_key_name(const std::string &name)
{
    if (name.size() != 1)
    {
        return false;
    }

    const unsigned char c{static_cast<unsigned char>(name[0])};
    const std::string punctuation_keys{"+-#\\<>@!,=.[]"};
    return std::isupper(c) != 0 || std::isdigit(c) != 0 || punctuation_keys.find(name[0]) != std::string::npos;
}

static bool starts_legacy_bullet_item(const char c)
{
    const unsigned char ch{static_cast<unsigned char>(c)};
    return std::isupper(ch) != 0 || std::isdigit(ch) != 0 || c == '<' || c == '"' || c == '\'' || c == '@' || c == '#';
}

static bool is_bullet_probe_char(const char c)
{
    const unsigned char ch{static_cast<unsigned char>(c)};
    return std::isalnum(ch) != 0 || c == '-' || c == '_' || c == '=' || c == ':';
}

static bool is_legacy_bullet_word(const std::string &word)
{
    return !word.empty() &&
        (starts_legacy_bullet_item(word[0]) || word.find('-') != std::string::npos ||
            word.find('=') != std::string::npos || word.back() == ':');
}

static bool is_key_name(const std::string &name)
{
    const auto is_function_key_name = [&name]
    {
        return name[0] == 'F' && name.size() < 4 &&
            std::all_of(name.begin() + 1, name.end(), [](const char c) { return std::isdigit(c) != 0; });
    };
    const auto is_modified_key_name = [&name](const std::string &prefix)
    { return name.substr(0, prefix.size()) == prefix && is_key_name(name.substr(prefix.size())); };

    return is_single_character_key_name(name)                                        //
        || name == "Enter" || name == "Esc" || name == "Delete" || name == "Insert"  //
        || name == "Tab" || name == "Space"                                          //
        || name == "Shift" || name == "Ctrl" || name == "Alt"                        //
        || name == "Keypad+" || name == "Keypad-"                                    //
        || name == "PageDown" || name == "PageUp" || name == "Home" || name == "End" //
        || name == "Left" || name == "Right" || name == "Up" || name == "Down"       //
        || name == "Fn" || name == "Arrow"                                           //
        || is_function_key_name()                                                    //
        || is_modified_key_name("Shift+")                                            //
        || is_modified_key_name("Ctrl+")                                             //
        || is_modified_key_name("Alt+");
}

void AsciiDocProcessor::emit_char(const char c)
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
    for (const char c : m_key_name)
    {
        emit_char(c);
    }
}

void AsciiDocProcessor::flush_bullet_candidate(const bool bullet)
{
    if (bullet)
    {
        emit_char('*');
        m_inside_bullet = true;
    }
    else
    {
        emit_char('o');
        m_inside_bullet = false;
    }

    m_spaces += m_bullet_spaces;
    for (const char c : m_bullet_probe)
    {
        emit_char(c);
    }
    m_bullet_started = false;
    m_bullet_indented = false;
    m_bullet_spaces = 0;
    m_bullet_probe.clear();
}

void AsciiDocProcessor::print_inside_key(const char c)
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

static std::string trim_right_copy(const std::string &text)
{
    std::string trimmed{text};
    while (!trimmed.empty() && trimmed.back() == ' ')
    {
        trimmed.pop_back();
    }
    return trimmed;
}

static bool is_raw_block_delimiter(const std::string &line)
{
    return line == "++++" || line == "|===" || line == "----" || line == "...." || line == "====" || line == "____" ||
        line == "****" || line == "////" || line == "--";
}

static bool is_raw_adoc_line(const std::string &line)
{
    return line.rfind("image::", 0) == 0;
}

void AsciiDocProcessor::update_raw_block_state()
{
    const std::string line{trim_right_copy(m_line_text)};
    if (m_inside_raw_block)
    {
        if (line == m_raw_block_delimiter)
        {
            m_inside_raw_block = false;
            m_raw_block_delimiter.clear();
        }
        return;
    }

    if (is_raw_block_delimiter(line))
    {
        m_inside_raw_block = true;
        m_raw_block_delimiter = line;
    }
}

void AsciiDocProcessor::print_raw_char(const char c)
{
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
        return;
    }

    if (c == '\n' || c == '\f')
    {
        ++m_newlines;
        m_start_of_line = true;
        m_indented_line = false;
        m_spaces = 0;
        m_str << c;
        return;
    }

    emit_char(c);
}

void AsciiDocProcessor::print_char(const char c, int n)
{
    while (n-- > 0)
    {
        bool raw_line = m_inside_raw_block;
        if (c == '\n' || c == '\f')
        {
            raw_line = raw_line || is_raw_adoc_line(m_line_text);
            update_raw_block_state();
            m_line_text.clear();
        }
        else
        {
            m_line_text += c;
            raw_line = raw_line || is_raw_adoc_line(m_line_text);
        }

        if (raw_line)
        {
            print_raw_char(c);
            continue;
        }

        if (m_bullet_started && !m_link_text.empty())
        {
            flush_bullet_candidate(m_bullet_spaces > 0);
        }

        if (m_bullet_started)
        {
            if (c == ' ' && m_bullet_probe.empty())
            {
                ++m_bullet_spaces;
                continue;
            }
            if (c == '\n' || c == '\f')
            {
                flush_bullet_candidate(
                    (m_bullet_indented && !m_bullet_probe.empty()) || is_legacy_bullet_word(m_bullet_probe));
            }
            else if (m_bullet_spaces == 0)
            {
                flush_bullet_candidate(false);
            }
            else if (m_bullet_indented && m_bullet_probe.empty())
            {
                flush_bullet_candidate(true);
            }
            else if (m_bullet_probe.empty() && starts_legacy_bullet_item(c))
            {
                flush_bullet_candidate(true);
            }
            else if (is_bullet_probe_char(c))
            {
                m_bullet_probe += c;
                continue;
            }
            else
            {
                flush_bullet_candidate(m_bullet_indented || is_legacy_bullet_word(m_bullet_probe));
            }
        }

        if (m_inside_key)
        {
            print_inside_key(c);
        }
        else if (m_start_of_line && c == 'o' && m_link_text.empty())
        {
            m_bullet_started = true;
            m_bullet_indented = m_spaces > 0;
        }
        else if (c == '<' && m_link_text.empty() && (!m_indented_line || m_inside_bullet))
        {
            m_inside_key = true;
            m_key_name.clear();
        }
        else if (c == ' ')
        {
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
            ++m_newlines;
            if (m_newlines >= 2)
            {
                m_inside_bullet = false;
            }
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

void AsciiDocProcessor::print_string(const char *s, int n)
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

void AsciiDocProcessor::print_anchor(const std::string &anchor)
{
    print_string("[[" + anchor + "]]");
    print_char('\n', 1);
}

void AsciiDocCompiler::print_ascii_doc()
{
    if (g_src.contents.empty())
    {
        throw std::runtime_error(".SRC has no DocContents.");
    }

    const std::string fname{m_options.fname2.empty() ? DEFAULT_ADOC_FNAME : m_options.fname2};

    MSG_MSG(("Writing " + fname).c_str());

    const Content &toc = g_src.contents[0];
    if (toc.num_topic != 1)
    {
        throw std::runtime_error("First content block contains multiple topics.");
    }
    if (toc.topic_name[0] != DOC_CONTENTS_TITLE)
    {
        throw std::runtime_error("First content block doesn't contain DocContent.");
    }

    const std::vector<std::string> topic_anchors{make_topic_anchors()};
    if (g_errors != 0)
    {
        return;
    }

    const std::filesystem::path out_file{std::filesystem::path{m_options.output_dir} / fname};
    std::ofstream str{out_file};
    if (!str)
    {
        throw std::runtime_error("Couldn't open output file " + out_file.string());
    }
    str << "= Iterated Dynamics\n"
           ":stem:\n"
           ":toc: left\n"
           ":toclevels: 4\n"
           ":experimental:\n";

    AsciiDocProcessor(str, topic_anchors).process();
}

} // namespace hc
