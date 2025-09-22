// SPDX-License-Identifier: GPL-3.0-only
//
#include "HelpSource.h"

#include "messages.h"
#include "modes.h"

#include <config/fdio.h>
#include <config/filelength.h>
#include <config/port.h>
#include <config/string_case_compare.h>
#include <helpcom.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <stdexcept>
#include <system_error>

#if defined(_WIN32)
// disable unsafe CRT warnings
#pragma warning(disable: 4996)
#endif

#if !defined(O_BINARY)
#define O_BINARY 0
#endif

using namespace id::config;
using namespace id::help;

namespace hc
{

constexpr int MAX_TABLE_SIZE{100};
constexpr int READ_CHAR_BUFF_SIZE{32};

struct Include
{
    std::string fname;
    std::FILE *file;
    int   line;
    int   col;
};

HelpSource g_src;

static std::FILE *s_src_file{};              // .SRC file
static int s_src_col{};                      // .SRC column.
static bool s_compress_spaces{};             //
static char s_cmd[128]{};                    // holds the current command
static int s_format_exclude{};               // disable formatting at this col, 0 to never disable formatting
static bool s_xonline{};                     //
static bool s_xdoc{};                        //
static bool s_xadoc{};                       //
static std::vector<Include> s_include_stack; //
static int s_read_char_buff[READ_CHAR_BUFF_SIZE];
static int s_read_char_buff_pos{-1};
static int s_read_char_sp{};

void Content::label_topic(const int ctr)
{
    if (const Label *const lbl = g_src.find_label(topic_name[ctr].c_str()))
    {
        if (bit_set(g_src.topics[lbl->topic_num].flags, TopicFlags::DATA))
        {
            g_current_src_filename = src_file;
            g_src_line = src_line;
            MSG_ERROR(0, "Label \"%s\" is a data-only topic.", topic_name[ctr].c_str());
            g_src_line = -1;
        }
        else
        {
            topic_num[ctr] = lbl->topic_num;
            if (bit_set(g_src.topics[lbl->topic_num].flags, TopicFlags::IN_DOC))
            {
                MSG_WARN(0, "Topic \"%s\" appears in document more than once.",
                    g_src.topics[lbl->topic_num].title.c_str());
            }
            else
            {
                g_src.topics[lbl->topic_num].flags |= TopicFlags::IN_DOC;
            }
        }
    }
    else
    {
        g_current_src_filename = src_file;
        g_src_line = src_line;
        MSG_ERROR(0, "Cannot find DocContent label \"%s\".", topic_name[ctr].c_str());
        g_src_line = -1;
    }
}

void Content::content_topic(const int ctr)
{
    const int t = find_topic_title(topic_name[ctr].c_str());
    if (t == -1)
    {
        g_current_src_filename = src_file;
        g_src_line = src_line;
        MSG_ERROR(0, "Cannot find DocContent topic \"%s\".", topic_name[ctr].c_str());
        g_src_line = -1;  // back to reality
    }
    else
    {
        topic_num[ctr] = t;
        if (bit_set(g_src.topics[t].flags, TopicFlags::IN_DOC))
        {
            MSG_WARN(0, "Topic \"%s\" appears in document more than once.",
                g_src.topics[t].title.c_str());
        }
        else
        {
            g_src.topics[t].flags |= TopicFlags::IN_DOC;
        }
    }
}

void Link::link_topic()
{
    const int t = find_topic_title(name.c_str());
    if (t == -1)
    {
        g_current_src_filename = src_file;
        g_src_line = src_line; // pretend we are still in the source...
        MSG_ERROR(0, "Cannot find implicit hot-link \"%s\".", name.c_str());
        g_src_line = -1;  // back to reality
    }
    else
    {
        topic_num = t;
        topic_off = 0;
        doc_page = bit_set(g_src.topics[t].flags, TopicFlags::IN_DOC) ? 0 : -1;
    }
}

void Link::link_label()
{
    if (const Label *const lbl = g_src.find_label(name.c_str()))
    {
        if (bit_set(g_src.topics[lbl->topic_num].flags, TopicFlags::DATA))
        {
            g_current_src_filename = src_file;
            g_src_line = src_line;
            MSG_ERROR(0, "Label \"%s\" is a data-only topic.", name.c_str());
            g_src_line = -1;
        }
        else
        {
            topic_num = lbl->topic_num;
            topic_off = lbl->topic_off;
            doc_page  = bit_set(g_src.topics[lbl->topic_num].flags, TopicFlags::IN_DOC) ? 0 : -1;
        }
    }
    else
    {
        g_current_src_filename = src_file;
        g_src_line = src_line; // pretend again
        MSG_ERROR(0, "Cannot find explicit hot-link \"%s\".", name.c_str());
        g_src_line = -1;
    }
}

void Topic::alloc_topic_text(const unsigned size)
{
    text_len = size;
    text = g_src.swap_pos;
    g_src.swap_pos += size;
    std::fseek(g_src.swap_file, text, SEEK_SET);
    std::fwrite(g_src.buffer.data(), 1, text_len, g_src.swap_file);
}

int Topic::add_page(const Page &p)
{
    page.push_back(p);
    return num_page++;
}

void Topic::add_page_break(
    const int margin, const char *str, const char *start, const char *curr, const int num_links)
{
    Page p;
    p.offset = static_cast<unsigned>(start - str);
    p.length = static_cast<unsigned>(curr - start);
    p.margin = margin;
    add_page(p);

    g_src.max_links = std::max(g_src.max_links, num_links);
}

char *Topic::get_topic_text()
{
    read_topic_text();
    return g_src.buffer.data();
}

const char *Topic::get_topic_text() const
{
    read_topic_text();
    return g_src.buffer.data();
}

void Topic::release_topic_text(const bool save) const
{
    if (save)
    {
        std::fseek(g_src.swap_file, text, SEEK_SET);
        std::fwrite(g_src.buffer.data(), 1, text_len, g_src.swap_file);
    }
}

void Topic::start(const char *str, const int len)
{
    flags = TopicFlags::NONE;
    title_len = len;
    title.assign(str, len);
    doc_page = -1;
    num_page = 0;
    g_src.curr = g_src.buffer.data();
}

void Topic::read_topic_text() const
{
    std::fseek(g_src.swap_file, text, SEEK_SET);
    if (std::fread(g_src.buffer.data(), 1, text_len, g_src.swap_file) != text_len)
    {
        throw std::system_error(errno, std::system_category(), "get_topic_text failed fread");
    }
}

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4311)
#endif
static void check_buffer(const char *curr, const unsigned int off, const char *buffer)
{
    if (static_cast<unsigned>(curr + off - buffer) >= BUFFER_SIZE - 1024)
    {
        throw std::runtime_error("Buffer overflowerd -- Help topic too large.");
    }
}
#if defined(_WIN32)
#pragma warning(pop)
#endif

static void check_buffer(const unsigned int off)
{
    check_buffer(g_src.curr, off, g_src.buffer.data());
}

Label *HelpSource::find_label(const char *name)
{
    const auto finder = [=](std::vector<Label> &collection) -> Label *
    {
        for (Label &label : collection)
        {
            if (name == label.name)
            {
                return &label;
            }
        }
        return nullptr;
    };

    return finder(name[0] == '@' ? private_labels : labels);
}

void HelpSource::sort_labels()
{
    std::sort(labels.begin(), labels.end());
    std::sort(private_labels.begin(), private_labels.end());
}

int find_topic_title(const char *title)
{
    while (*title == ' ')
    {
        ++title;
    }

    int len = static_cast<int>(std::strlen(title)) - 1;
    while (title[len] == ' ' && len > 0)
    {
        --len;
    }

    ++len;

    if (len > 2 && title[0] == '\"' && title[len-1] == '\"')
    {
        ++title;
        len -= 2;
    }

    for (int t = 0; t < static_cast<int>(g_src.topics.size()); t++)
    {
        if (static_cast<int>(g_src.topics[t].title.length()) == len
            && string_case_equal(title, g_src.topics[t].title.c_str(), len))
        {
            return t;
        }
    }

    return -1;   // not found
}

/*
 * memory-allocation functions.
 */
int HelpSource::add_link(Link &l)
{
    all_links.push_back(l);
    return static_cast<int>(all_links.size() - 1);
}

int HelpSource::add_topic(const Topic &t)
{
    topics.push_back(t);
    return static_cast<int>(topics.size() - 1);
}

int HelpSource::add_label(const Label &l)
{
    if (l.name[0] == '@')    // if it's a private label...
    {
        private_labels.push_back(l);
        return static_cast<int>(private_labels.size() - 1);
    }

    labels.push_back(l);
    return static_cast<int>(labels.size() - 1);
}

int HelpSource::add_content(const Content &c)
{
    contents.push_back(c);
    return static_cast<int>(contents.size() - 1);
}

/*
 * read_char() stuff
 */

/*
 * Will not handle new-lines or tabs correctly!
 */
static void unread_char(const int ch)
{
    if (s_read_char_buff_pos+1 >= READ_CHAR_BUFF_SIZE)
    {
        throw std::runtime_error("Compiler Error -- Read char buffer overflow!");
    }

    s_read_char_buff[++s_read_char_buff_pos] = ch;

    --s_src_col;
}

static int read_char_aux()
{
    if (g_src_line <= 0)
    {
        g_src_line = 1;
        s_src_col = 0;
    }

    if (s_read_char_buff_pos >= 0)
    {
        ++s_src_col;
        return s_read_char_buff[s_read_char_buff_pos--];
    }

    if (s_read_char_sp > 0)
    {
        --s_read_char_sp;
        return ' ';
    }

    if (std::feof(s_src_file))
    {
        return -1;
    }

    while (true)
    {
        switch (const int ch = std::getc(s_src_file); ch)
        {
        case '\t':    // expand a tab
        {
            const int diff = (s_src_col / 8 + 1) * 8 - s_src_col;

            s_src_col += diff;
            s_read_char_sp += diff;
            break;
        }

        case ' ':
            ++s_src_col;
            ++s_read_char_sp;
            break;

        case '\n':
            s_read_char_sp = 0;   // delete spaces before a \n
            s_src_col = 0;
            ++g_src_line;
            return '\n';

        case -1:               // EOF
            if (s_read_char_sp > 0)
            {
                --s_read_char_sp;
                return ' ';
            }
            return -1;

        default:
            if (s_read_char_sp > 0)
            {
                std::ungetc(ch, s_src_file);
                --s_read_char_sp;
                return ' ';
            }

            ++s_src_col;
            return ch;
        } // switch
    }
}

static int read_char()
{
    int ch = read_char_aux();

    while (ch == ';' && s_src_col == 1)    // skip over comments
    {
        ch = read_char_aux();

        while (ch != '\n' && ch != -1)
        {
            ch = read_char_aux();
        }

        ch = read_char_aux();
    }

    if (ch == '\\')   // process an escape code
    {
        ch = read_char_aux();

        if (ch >= '0' && ch <= '9')
        {
            char buff[4];
            int  ctr;

            for (ctr = 0; ctr < 3; ctr++)
            {
                if (ch < '0' || ch > '9' || ch == -1 || ctr >= 3)
                {
                    unread_char(ch);
                    break;
                }
                buff[ctr] = ch;
                ch = read_char_aux();
            }
            buff[ctr] = '\0';
            ch = std::atoi(buff);
        }
        ch |= 0x100;
    }

    if ((ch & 0xFF) == 0)
    {
        MSG_ERROR(0, "Null character (\'\\0\') not allowed!");
        ch = 0x1FF; // since we've had an error the file will not be written;
        //   the value we return doesn't really matter
    }

    return ch;
}

/*
 * .SRC file parser stuff
 */
static bool validate_label_name(const char *name)
{
    if (!std::isalpha(*name) && *name != '@' && *name != '_')
    {
        return false;    // invalid
    }

    while (*++name != '\0')
    {
        if (!std::isalpha(*name) && !std::isdigit(*name) && *name != '_')
        {
            return false;    // invalid
        }
    }

    return true;  // valid
}

static char *read_until(char *buff, int len, const char *stop_chars)
{
    while (--len > 0)
    {
        const int ch = read_char();

        if (ch == -1)
        {
            *buff++ = '\0';
            break;
        }

        if ((ch&0xFF) <= MAX_CMD)
        {
            *buff++ = CMD_LITERAL;
        }

        // skip '\r' characters
        if (ch != '\r')
        {
            *buff++ = ch;
        }

        if ((ch&0x100) == 0 && std::strchr(stop_chars, ch) != nullptr)
        {
            break;
        }
    }

    return buff-1;
}

static void skip_over(const char *skip)
{
    while (true)
    {
        const int ch = read_char();

        if (ch == -1)
        {
            break;
        }
        if ((ch&0x100) == 0 && std::strchr(skip, ch) == nullptr)
        {
            unread_char(ch);
            break;
        }
    }
}

static char *char_lit(const int ch)
{
    static char buff[16];

    if (ch >= 0x20 && ch <= 0x7E)
    {
        std::sprintf(buff, "'%c'", ch);
    }
    else
    {
        // ReSharper disable once CppPrintfExtraArg
        std::sprintf(buff, R"('\x%02X')", static_cast<unsigned int>(ch & 0xFF));
    }

    return buff;
}

static void put_spaces(int how_many)
{
    if (how_many > 2 && s_compress_spaces)
    {
        if (how_many > 255)
        {
            MSG_ERROR(0, "Too many spaces (over 255).");
            how_many = 255;
        }

        *g_src.curr++ = CMD_SPACE;
        *g_src.curr++ = static_cast<Byte>(how_many);
    }
    else
    {
        while (how_many-- > 0)
        {
            *g_src.curr++ = ' ';
        }
    }
}

// used by parse_contents()
static bool get_next_item()
{
    skip_over(" \t\r\n");
    char *ptr = read_until(s_cmd, 128, ",}");
    const bool last = *ptr == '}';
    --ptr;
    while (ptr >= s_cmd && std::strchr(" \t\r\n", *ptr))     // strip trailing spaces
    {
        --ptr;
    }
    *++ptr = '\0';

    return last;
}

static void process_doc_contents(char *(*format_toc)(char *buffer, Content &c))
{
    Topic t;
    t.flags     = TopicFlags::NONE;
    t.title_len = static_cast<unsigned>(std::strlen(DOC_CONTENTS_TITLE)) +1;
    t.title     = DOC_CONTENTS_TITLE;
    t.doc_page  = -1;
    t.num_page  = 0;

    g_src.curr = g_src.buffer.data();

    Content c{};
    c.flags = 0;
    c.id.clear();
    c.name.clear();
    c.doc_page = -1;
    c.page_num_pos = 0;
    c.num_topic = 1;
    c.is_label[0] = false;
    c.topic_name[0] = DOC_CONTENTS_TITLE;
    c.src_line = -1;
    g_src.add_content(c);

    while (true)
    {
        const int ch = read_char();
        if (ch == '{')   // process a Content entry
        {
            c.flags = 0;
            c.num_topic = 0;
            c.doc_page = -1;
            c.src_file = g_current_src_filename;
            c.src_line = g_src_line;

            if (get_next_item())
            {
                MSG_ERROR(0, "Unexpected end of DocContent entry.");
                continue;
            }
            c.id = s_cmd;

            if (get_next_item())
            {
                MSG_ERROR(0, "Unexpected end of DocContent entry.");
                continue;
            }
            c.indent = std::stoi(s_cmd);

            bool last = get_next_item();

            if (s_cmd[0] == '\"')
            {
                char *ptr = &s_cmd[1];
                if (ptr[static_cast<int>(std::strlen(ptr)) -1] == '\"')
                {
                    ptr[static_cast<int>(std::strlen(ptr)) -1] = '\0';
                }
                else
                {
                    MSG_WARN(0, "Missing ending quote.");
                }

                c.is_label[c.num_topic] = false;
                c.topic_name[c.num_topic] = ptr;
                ++c.num_topic;
                c.name = ptr;
            }
            else
            {
                c.name = s_cmd;
            }

            // now, make the entry in the buffer
            g_src.curr = format_toc(g_src.curr, c);

            while (!last)
            {
                last = get_next_item();
                if (string_case_equal(s_cmd, "FF"))
                {
                    if (c.flags & CF_NEW_PAGE)
                    {
                        MSG_WARN(0, "FF already present in this entry.");
                    }
                    c.flags |= CF_NEW_PAGE;
                    continue;
                }

                if (s_cmd[0] == '\"')
                {
                    char *ptr = &s_cmd[1];
                    if (ptr[static_cast<int>(std::strlen(ptr)) -1] == '\"')
                    {
                        ptr[static_cast<int>(std::strlen(ptr)) -1] = '\0';
                    }
                    else
                    {
                        MSG_WARN(0, "Missing ending quote.");
                    }

                    c.is_label[c.num_topic] = false;
                    c.topic_name[c.num_topic] = ptr;
                }
                else
                {
                    c.is_label[c.num_topic] = true;
                    c.topic_name[c.num_topic] = s_cmd;
                }

                if (++c.num_topic >= MAX_CONTENT_TOPIC)
                {
                    MSG_ERROR(0, "Too many topics in DocContent entry.");
                    break;
                }
            }

            g_src.add_content(c);
        }
        else if (ch == '~')   // end at any command
        {
            unread_char(ch);
            break;
        }
        else
        {
            *g_src.curr++ = ch;
        }

        check_buffer(0);
    }

    t.alloc_topic_text(static_cast<unsigned>(g_src.curr - g_src.buffer.data()));
    g_src.add_topic(t);
}

static void process_doc_contents(const Mode mode)
{
    if (mode == Mode::ASCII_DOC)
    {
        process_doc_contents(
            [](char *buffer, Content &c)
            {
                c.page_num_pos = 0U;
                return buffer;
            });
    }
    else
    {
        process_doc_contents(
            [](char *buffer, Content &c)
            {
                std::sprintf(buffer, "%-5s %*.0s%s", c.id.c_str(), c.indent * 2, "", c.name.c_str());
                char *ptr = buffer + static_cast<int>(std::strlen(buffer));
                while (ptr - buffer < PAGE_WIDTH - 10)
                {
                    *ptr++ = '.';
                }
                c.page_num_pos = static_cast<unsigned>(ptr - 3 - g_src.buffer.data());
                return ptr;
            });
    }
}

static int parse_link()   // returns length of link or 0 on error
{
    char *ptr;
    bool bad = false;
    int   len;
    int   err_offset;

    Link l;
    l.src_file  = g_current_src_filename;
    l.src_line  = g_src_line;
    l.doc_page = -1;

    char *end = read_until(s_cmd, 128, "}\n");   // get the entire hot-link

    if (*end == '\0')
    {
        MSG_ERROR(0, "Unexpected EOF in hot-link.");
        return 0;
    }

    if (*end == '\n')
    {
        err_offset = 1;
        MSG_WARN(1, "Hot-link has no closing curly-brace (\'}\').");
    }
    else
    {
        err_offset = 0;
    }

    *end = '\0';

    if (s_cmd[0] == '=')   // it's an "explicit" link to a label or "special"
    {
        ptr = std::strchr(s_cmd, ' ');

        if (ptr == nullptr)
        {
            ptr = end;
        }
        else
        {
            *ptr++ = '\0';
        }

        len = static_cast<int>(end - ptr);

        if (s_cmd[1] == '-')
        {
            l.type      = LinkTypes::LT_SPECIAL;
            l.topic_num = std::atoi(&s_cmd[1]);
            l.topic_off = 0;
            l.name.clear();
        }
        else
        {
            l.type = LinkTypes::LT_LABEL;
            if (static_cast<int>(std::strlen(s_cmd)) > 32)
            {
                MSG_WARN(err_offset, "Label is long.");
            }
            if (s_cmd[1] == '\0')
            {
                MSG_ERROR(err_offset, "Explicit hot-link has no Label.");
                bad = true;
            }
            else
            {
                l.name = &s_cmd[1];
            }
        }
        if (len == 0)
        {
            MSG_WARN(err_offset, "Explicit hot-link has no title.");
        }
    }
    else
    {
        ptr = s_cmd;
        l.type = LinkTypes::LT_TOPIC;
        len = static_cast<int>(end - ptr);
        if (len == 0)
        {
            MSG_ERROR(err_offset, "Implicit hot-link has no title.");
            bad = true;
        }
        while (*ptr == ' ')
        {
            ++ptr;
            --len;
        }
        while (len > 0 && ptr[len - 1] == ' ')
        {
            --len;
        }
        if (len > 2 && ptr[0] == '"' && ptr[len - 1] == '"')
        {
            ++ptr;
            len -= 2;
        }
        l.name.assign(ptr, len);
    }

    if (!bad)
    {
        check_buffer(1 + 3 * sizeof(int) + len + 1);
        const int link_num = g_src.add_link(l);
        *g_src.curr++ = CMD_LINK;
        set_int(g_src.curr, link_num);
        g_src.curr += 3*sizeof(int);
        std::memcpy(g_src.curr, ptr, len);
        g_src.curr += len;
        *g_src.curr++ = CMD_LINK;
        return len;
    }

    return 0;
}

static int create_table()
{
    int    width;
    int    cols;
    int    start_off;
    int    ch;
    std::vector<std::string> title;

    char *ptr = std::strchr(s_cmd, '=');

    if (ptr == nullptr)
    {
        return 0;    // should never happen!
    }

    ptr++;

    int len = std::sscanf(ptr, " %d %d %d", &width, &cols, &start_off);

    if (len < 3)
    {
        MSG_ERROR(1, "Too few arguments to Table.");
        return 0;
    }

    if (width <= 0 || width > 78 || cols <= 0 || start_off < 0 || start_off > 78)
    {
        MSG_ERROR(1, "Argument out of range.");
        return 0;
    }

    bool done = false;

    const int first_link = static_cast<int>(g_src.all_links.size());
    char *table_start = g_src.curr;
    int count = 0;

    // first, read all the links in the table

    do
    {
        do
        {
            ch = read_char();
        }
        while (ch == '\r' || ch == '\n' || ch == ' ');

        if (done)
        {
            break;
        }

        switch (ch)
        {
        case -1:
            MSG_ERROR(0, "Unexpected EOF in a Table.");
            return 0;

        case '{':
            if (count >= MAX_TABLE_SIZE)
            {
                throw std::runtime_error("Table is too large.");
            }
            len = parse_link();
            g_src.curr = table_start;   // reset to the start...
            title.emplace_back(g_src.curr+3*sizeof(int)+1, len+ 1);
            if (len >= width)
            {
                MSG_WARN(1, "Link is too long; truncating.");
                len = width-1;
            }
            title[count][len] = '\0';
            ++count;
            break;

        case '~':
        {
            bool embedded;

            ch = read_char();

            if (ch == '(')
            {
                embedded = true;
            }
            else
            {
                embedded = false;
                unread_char(ch);
            }

            ptr = read_until(s_cmd, 128, ")\n,");

            ch = *ptr;
            *ptr = '\0';

            if (string_case_equal(s_cmd, "EndTable"))
            {
                done = true;
            }
            else
            {
                MSG_ERROR(1, "Unexpected command in table \"%s\"", s_cmd);
                MSG_WARN(1, "Command will be ignored.");
            }

            if (ch == ',')
            {
                if (embedded)
                {
                    unread_char('(');
                }
                unread_char('~');
            }
        }
        break;

        default:
            MSG_ERROR(0, "Unexpected character %s.", char_lit(ch));
            break;
        }
    }
    while (!done);

    // now, put all the links into the buffer...

    const int rows = 1 + count / cols;

    for (int r = 0; r < rows; r++)
    {
        put_spaces(start_off);
        for (int c = 0; c < cols; c++)
        {
            const int link_num = c * rows + r;

            if (first_link+link_num >= static_cast<int>(g_src.all_links.size()))
            {
                break;
            }

            len = static_cast<int>(title[link_num].length());
            *g_src.curr++ = CMD_LINK;
            set_int(g_src.curr, first_link+link_num);
            g_src.curr += 3*sizeof(int);
            std::memcpy(g_src.curr, title[link_num].c_str(), len);
            g_src.curr += len;
            *g_src.curr++ = CMD_LINK;

            if (c < cols-1)
            {
                put_spaces(width-len);
            }
        }
        *g_src.curr++ = '\n';
    }

    return 1;
}

static void process_comment()
{
    while (true)
    {
        int ch = read_char();

        if (ch == '~')
        {
            bool embedded;

            ch = read_char();

            if (ch == '(')
            {
                embedded = true;
            }
            else
            {
                embedded = false;
                unread_char(ch);
            }

            char *ptr = read_until(s_cmd, 128, ")\n,");

            ch = *ptr;
            *ptr = '\0';

            if (string_case_equal(s_cmd, "EndComment"))
            {
                if (ch == ',')
                {
                    if (embedded)
                    {
                        unread_char('(');
                    }
                    unread_char('~');
                }
                break;
            }
        }
        else if (ch == -1)
        {
            MSG_ERROR(0, "Unexpected EOF in Comment");
            break;
        }
    }
}

static void process_bin_inc()
{
    const int handle = open(&s_cmd[7], O_RDONLY | O_BINARY);
    if (handle == -1)
    {
        MSG_ERROR(0, "Unable to open \"%s\"", &s_cmd[7]);
        return;
    }

    const long len = filelength(handle);

    if (len >= BUFFER_SIZE)
    {
        MSG_ERROR(0, "File \"%s\" is too large to BinInc (%dK).", &s_cmd[7], static_cast<int>(len >> 10));
        close(handle);
        return ;
    }

    /*
     * Since we know len is less than BUFFER_SIZE (and therefore less than
     * 64K) we can treat it as an unsigned.
     */

    check_buffer(static_cast<unsigned>(len));

    if (read(handle, g_src.curr, static_cast<unsigned>(len)) != len)
    {
        throw std::system_error(errno, std::system_category(), "process_bininc failed read");
    }

    g_src.curr += static_cast<unsigned>(len);

    close(handle);
}

static void end_topic(Topic &t)
{
    t.alloc_topic_text(static_cast<unsigned>(g_src.curr - g_src.buffer.data()));
    g_src.add_topic(t);
}

static bool end_of_sentence(const char *ptr)  // true if ptr is at the end of a sentence
{
    if (*ptr == ')')
    {
        --ptr;
    }

    if (*ptr == '\"')
    {
        --ptr;
    }

    return *ptr == '.' || *ptr == '?' || *ptr == '!';
}

static void add_blank_for_split()   // add space at g_src.curr for merging two lines
{
    if (!is_hyphen(g_src.curr-1))     // no spaces if it's a hyphen
    {
        if (end_of_sentence(g_src.curr-1))
        {
            *g_src.curr++ = ' ';    // two spaces at end of a sentence
        }
        *g_src.curr++ = ' ';
    }
}

static void put_a_char(const int ch, const Topic &t)
{
    if (ch == '{' && !bit_set(t.flags, TopicFlags::DATA)) // is it a hot-link?
    {
        parse_link();
    }
    else
    {
        if ((ch&0xFF) <= MAX_CMD)
        {
            *g_src.curr++ = CMD_LITERAL;
        }
        *g_src.curr++ = ch;
    }
}

enum class ParseStates // states for FSM's
{
    START,                  // initial state, between paragraphs
    START_FIRST_LINE,       // spaces at start of first line
    FIRST_LINE,             // text on the first line
    FIRST_LINE_SPACES,      // spaces on the first line
    START_SECOND_LINE,      // spaces at start of second line
    LINE,                   // text on lines after the first
    LINE_SPACES,            // spaces on lines after the first
    START_LINE,             // spaces at start of lines after second
    FORMAT_DISABLED,        // format automatically disabled for this line
    FORMAT_DISABLED_SPACES, // spaces in line which format is disabled
    SPACES
};

static void check_command_length(const int err_offset, const int len)
{
    if (static_cast<int>(std::strlen(s_cmd)) != len)
    {
        MSG_ERROR(err_offset, "Invalid text after a command \"%s\"", s_cmd+len);
    }
}

static std::FILE *open_include(const std::string &filename)
{
    std::FILE *result = std::fopen(filename.c_str(), "rt");
    if (result == nullptr)
    {
        for (const std::string &dir : g_src.include_paths)
        {
            const std::string path{dir + '/' + filename};
            result = std::fopen(path.c_str(), "rt");
            if (result != nullptr)
            {
                return result;
            }
        }
    }
    return result;
}

static void toggle_mode(const std::string &tag, const HelpCommand cmd, bool &flag, const int err_offset)
{
    if (!string_case_equal(s_cmd, tag.data(), tag.length()))
    {
        return;
    }

    if (s_cmd[tag.length()] == '+')
    {
        check_command_length(err_offset, static_cast<int>(tag.length() + 1));

        if (flag)
        {
            *g_src.curr++ = cmd;
            flag = false;
        }
        else
        {
            MSG_WARN(err_offset, ('"' + tag + R"msg(+" already in effect.)msg").c_str());
        }
    }
    else if (s_cmd[tag.length()] == '-')
    {
        check_command_length(err_offset, static_cast<int>(tag.length() + 1));
        if (!flag)
        {
            *g_src.curr++ = cmd;
            flag = true;
        }
        else
        {
            MSG_WARN(err_offset, ('"' + tag + R"msg(-" already in effect.)msg").c_str());
        }
    }
    else
    {
        MSG_ERROR(err_offset, ("Invalid argument to " + tag + ".").c_str());
    }
}

void read_src(const std::string &fname, Mode mode)
{
    int    ch;
    char  *ptr;
    Topic  t;
    Label  lbl;
    char  *margin_pos = nullptr;
    bool in_topic = false;
    bool formatting = true;
    ParseStates state = ParseStates::START;
    int    num_spaces = 0;
    int    margin     = 0;
    bool in_para = false;
    bool centering = false;
    int    format_exclude = s_format_exclude;

    s_xonline = false;
    s_xdoc = false;

    g_current_src_filename = fname;

    s_src_file = open_include(fname);
    if (s_src_file == nullptr)
    {
        throw std::runtime_error(R"msg(Unable to open ")msg" + fname + '"');
    }

    MSG_MSG("Compiling: %s", fname.c_str());

    in_topic = false;

    g_src.curr = g_src.buffer.data();

    while (true)
    {
        ch = read_char();

        if (ch == -1)     // EOF?
        {
            if (!s_include_stack.empty())
            {
                std::fclose(s_src_file);
                const Include &top{s_include_stack.back()};
                g_current_src_filename = top.fname;
                s_src_file = top.file;
                g_src_line = top.line;
                s_src_col = top.col;
                s_include_stack.pop_back();
                continue;
            }
            if (in_topic)  // if we're in a topic, finish it
            {
                end_topic(t);
            }
            if (g_src.topics.empty())
            {
                MSG_WARN(0, ".SRC file has no topics.");
            }
            break;
        }

        if (ch == '~')   // is is a command?
        {
            bool embedded;

            ch = read_char();
            if (ch == '(')
            {
                embedded = true;
            }
            else
            {
                embedded = false;
                unread_char(ch);
            }

            bool done = false;

            int err_offset{};
            while (!done)
            {
                do
                {
                    ch = read_char();
                }
                while (ch == ' ');
                unread_char(ch);

                ptr = read_until(s_cmd, 128, embedded ? ")\n," : "\n,");

                if (*ptr == '\0')
                {
                    MSG_ERROR(0, "Unexpected EOF in command.");
                    break;
                }

                if (*ptr == '\n')
                {
                    ++err_offset;
                }

                if (embedded && *ptr == '\n')
                {
                    MSG_ERROR(err_offset, "Embedded command has no closing paren (\')\')");
                }

                done = *ptr != ',';   // we're done if it's not a comma

                if (*ptr != '\n' && *ptr != ')' && *ptr != ',')
                {
                    MSG_ERROR(0, "Command line too long.");
                    break;
                }

                *ptr = '\0';

                // commands allowed anytime...
                if (string_case_equal(s_cmd, "Topic=", 6))
                {
                    if (in_topic)  // if we're in a topic, finish it
                    {
                        end_topic(t);
                    }
                    else
                    {
                        in_topic = true;
                    }

                    const char *topic_title = &s_cmd[6];
                    const size_t title_len = std::strlen(topic_title);
                    if (title_len == 0)
                    {
                        MSG_WARN(err_offset, "Topic has no title.");
                    }
                    else if (title_len > 70)
                    {
                        MSG_ERROR(err_offset, "Topic title is too long.");
                    }
                    else if (title_len > 60)
                    {
                        MSG_WARN(err_offset, "Topic title is long.");
                    }

                    if (find_topic_title(topic_title) != -1)
                    {
                        MSG_ERROR(err_offset, "Topic title already exists.");
                    }

                    t.start(topic_title, static_cast<int>(title_len));
                    formatting = true;
                    centering = false;
                    state = ParseStates::START;
                    in_para = false;
                    num_spaces = 0;
                    s_xonline = false;
                    s_xdoc = false;
                    format_exclude = s_format_exclude;
                    s_compress_spaces = true;
                    continue;
                }
                if (string_case_equal(s_cmd, "Data=", 5))
                {
                    if (in_topic)  // if we're in a topic, finish it
                    {
                        end_topic(t);
                    }
                    else
                    {
                        in_topic = true;
                    }

                    const char *data = &s_cmd[5];
                    if (data[0] == '\0')
                    {
                        MSG_WARN(err_offset, "Data topic has no label.");
                    }

                    if (!validate_label_name(data))
                    {
                        MSG_ERROR(err_offset, "Label \"%s\" contains illegal characters.", data);
                        continue;
                    }

                    if (g_src.find_label(data) != nullptr)
                    {
                        MSG_ERROR(err_offset, "Label \"%s\" already exists", data);
                        continue;
                    }

                    if (s_cmd[5] == '@')
                    {
                        MSG_WARN(err_offset, "Data topic has a local label.");
                    }

                    t.start("", 0);
                    t.flags |= TopicFlags::DATA;

                    if (static_cast<int>(std::strlen(data)) > 32)
                    {
                        MSG_WARN(err_offset, "Label name is long.");
                    }

                    lbl.name      = data;
                    lbl.topic_num = static_cast<int>(g_src.topics.size());
                    lbl.topic_off = 0;
                    lbl.doc_page  = -1;
                    g_src.add_label(lbl);

                    formatting = false;
                    centering = false;
                    state = ParseStates::START;
                    in_para = false;
                    num_spaces = 0;
                    s_xonline = false;
                    s_xdoc = false;
                    format_exclude = s_format_exclude;
                    s_compress_spaces = false;
                    continue;
                }
                if (string_case_equal(s_cmd, "DocContents", 11))
                {
                    check_command_length(err_offset, 11);
                    if (in_topic)  // if we're in a topic, finish it
                    {
                        end_topic(t);
                    }
                    if (!done)
                    {
                        if (embedded)
                        {
                            unread_char('(');
                        }
                        unread_char('~');
                        done = true;
                    }
                    s_compress_spaces = true;
                    process_doc_contents(mode);
                    in_topic = false;
                    continue;
                }
                if (string_case_equal(s_cmd, "Comment"))
                {
                    process_comment();
                    continue;
                }
                if (string_case_equal(s_cmd, "FormatExclude", 13))
                {
                    if (s_cmd[13] == '-')
                    {
                        check_command_length(err_offset, 14);
                        if (in_topic)
                        {
                            if (format_exclude > 0)
                            {
                                format_exclude = -format_exclude;
                            }
                            else
                            {
                                MSG_WARN(err_offset, "\"FormatExclude-\" is already in effect.");
                            }
                        }
                        else
                        {
                            if (s_format_exclude > 0)
                            {
                                s_format_exclude = -s_format_exclude;
                            }
                            else
                            {
                                MSG_WARN(err_offset, "\"FormatExclude-\" is already in effect.");
                            }
                        }
                    }
                    else if (s_cmd[13] == '+')
                    {
                        check_command_length(err_offset, 14);
                        if (in_topic)
                        {
                            if (format_exclude < 0)
                            {
                                format_exclude = -format_exclude;
                            }
                            else
                            {
                                MSG_WARN(err_offset, "\"FormatExclude+\" is already in effect.");
                            }
                        }
                        else
                        {
                            if (s_format_exclude < 0)
                            {
                                s_format_exclude = -s_format_exclude;
                            }
                            else
                            {
                                MSG_WARN(err_offset, "\"FormatExclude+\" is already in effect.");
                            }
                        }
                    }
                    else if (s_cmd[13] == '=')
                    {
                        if (s_cmd[14] == 'n' || s_cmd[14] == 'N')
                        {
                            check_command_length(err_offset, 15);
                            if (in_topic)
                            {
                                format_exclude = 0;
                            }
                            else
                            {
                                s_format_exclude = 0;
                            }
                        }
                        else if (s_cmd[14] == '\0')
                        {
                            format_exclude = s_format_exclude;
                        }
                        else
                        {
                            int n = (in_topic ? format_exclude : s_format_exclude) < 0 ? -1 : 1;

                            format_exclude = std::atoi(&s_cmd[14]);

                            if (format_exclude <= 0)
                            {
                                MSG_ERROR(err_offset, "Invalid argument to FormatExclude=");
                                format_exclude = 0;
                            }

                            format_exclude *= n;

                            if (!in_topic)
                            {
                                s_format_exclude = format_exclude;
                            }
                        }
                    }
                    else
                    {
                        MSG_ERROR(err_offset, "Invalid format for FormatExclude");
                    }

                    continue;
                }
                if (string_case_equal(s_cmd, "Include ", 8))
                {
                    const std::string filename = &s_cmd[8];
                    if (std::FILE *new_file = open_include(filename))
                    {
                        Include top{};
                        top.fname = g_current_src_filename;
                        top.file = s_src_file;
                        top.line = g_src_line;
                        top.col  = s_src_col;
                        s_include_stack.push_back(top);
                        s_src_file = new_file;
                        g_current_src_filename = filename;
                        g_src_line = 1;
                        s_src_col = 0;
                    }
                    else
                    {
                        MSG_ERROR(err_offset, "Unable to open \"%s\"", filename.c_str());
                    }
                    continue;
                }

                // commands allowed only before all topics...

                if (!in_topic)
                {
                    if (string_case_equal(s_cmd, "HdrFile=", 8))
                    {
                        if (!g_src.hdr_filename.empty())
                        {
                            MSG_WARN(err_offset, "Header Filename has already been defined.");
                        }
                        g_src.hdr_filename = &s_cmd[8];
                    }
                    else if (string_case_equal(s_cmd, "HlpFile=", 8))
                    {
                        if (!g_src.hlp_filename.empty())
                        {
                            MSG_WARN(err_offset, "Help Filename has already been defined.");
                        }
                        g_src.hlp_filename = &s_cmd[8];
                    }
                    else if (string_case_equal(s_cmd, "Version=", 8))
                    {
                        if (g_src.version != -1)   // an unlikely value
                        {
                            MSG_WARN(err_offset, "Help version has already been defined");
                        }
                        g_src.version = std::atoi(&s_cmd[8]);
                    }
                    else
                    {
                        MSG_ERROR(err_offset, "Bad or unexpected command \"%s\"", s_cmd);
                    }

                    continue;
                }
                // commands allowed only in a topic...
                if (string_case_equal(s_cmd, "FF", 2))
                {
                    check_command_length(err_offset, 2);
                    if (in_para)
                    {
                        *g_src.curr++ = '\n';    // finish off current paragraph
                    }
                    *g_src.curr++ = CMD_FF;
                    state = ParseStates::START;
                    in_para = false;
                    num_spaces = 0;
                }
                else if (string_case_equal(s_cmd, "DocFF", 5))
                {
                    check_command_length(err_offset, 5);
                    if (in_para)
                    {
                        *g_src.curr++ = '\n';    // finish off current paragraph
                    }
                    if (!s_xonline)
                    {
                        *g_src.curr++ = CMD_XONLINE;
                    }
                    *g_src.curr++ = CMD_FF;
                    if (!s_xonline)
                    {
                        *g_src.curr++ = CMD_XONLINE;
                    }
                    state = ParseStates::START;
                    in_para = false;
                    num_spaces = 0;
                }
                else if (string_case_equal(s_cmd, "OnlineFF", 8))
                {
                    check_command_length(err_offset, 8);
                    if (in_para)
                    {
                        *g_src.curr++ = '\n';    // finish off current paragraph
                    }
                    if (!s_xdoc)
                    {
                        *g_src.curr++ = CMD_XDOC;
                    }
                    *g_src.curr++ = CMD_FF;
                    if (!s_xdoc)
                    {
                        *g_src.curr++ = CMD_XDOC;
                    }
                    state = ParseStates::START;
                    in_para = false;
                    num_spaces = 0;
                }
                else if (string_case_equal(s_cmd, "Label=", 6))
                {
                    const char *label_name = &s_cmd[6];
                    if (static_cast<int>(std::strlen(label_name)) <= 0)
                    {
                        MSG_ERROR(err_offset, "Label has no name.");
                    }
                    else if (!validate_label_name(label_name))
                    {
                        MSG_ERROR(err_offset, "Label \"%s\" contains illegal characters.", label_name);
                    }
                    else if (g_src.find_label(label_name) != nullptr)
                    {
                        MSG_ERROR(err_offset, "Label \"%s\" already exists", label_name);
                    }
                    else
                    {
                        if (static_cast<int>(std::strlen(label_name)) > 32)
                        {
                            MSG_WARN(err_offset, "Label name is long.");
                        }

                        if (bit_set(t.flags, TopicFlags::DATA) && s_cmd[6] == '@')
                        {
                            MSG_WARN(err_offset, "Data topic has a local label.");
                        }

                        lbl.name      = label_name;
                        lbl.topic_num = static_cast<int>(g_src.topics.size());
                        lbl.topic_off = static_cast<unsigned>(g_src.curr - g_src.buffer.data());
                        lbl.doc_page  = -1;
                        g_src.add_label(lbl);
                    }
                }
                else if (string_case_equal(s_cmd, "Table=", 6))
                {
                    if (in_para)
                    {
                        *g_src.curr++ = '\n';  // finish off current paragraph
                        in_para = false;
                        num_spaces = 0;
                        state = ParseStates::START;
                    }

                    if (!done)
                    {
                        if (embedded)
                        {
                            unread_char('(');
                        }
                        unread_char('~');
                        done = true;
                    }

                    create_table();
                }
                else if (string_case_equal(s_cmd, "FormatExclude", 12))
                {
                    if (s_cmd[13] == '-')
                    {
                        check_command_length(err_offset, 14);
                        if (format_exclude > 0)
                        {
                            format_exclude = -format_exclude;
                        }
                        else
                        {
                            MSG_WARN(0, "\"FormatExclude-\" is already in effect.");
                        }
                    }
                    else if (s_cmd[13] == '+')
                    {
                        check_command_length(err_offset, 14);
                        if (format_exclude < 0)
                        {
                            format_exclude = -format_exclude;
                        }
                        else
                        {
                            MSG_WARN(0, "\"FormatExclude+\" is already in effect.");
                        }
                    }
                    else
                    {
                        MSG_ERROR(err_offset, "Unexpected or invalid argument to FormatExclude.");
                    }
                }
                else if (string_case_equal(s_cmd, "Format", 6))
                {
                    if (s_cmd[6] == '+')
                    {
                        check_command_length(err_offset, 7);
                        if (!formatting)
                        {
                            formatting = true;
                            in_para = false;
                            num_spaces = 0;
                            state = ParseStates::START;
                        }
                        else
                        {
                            MSG_WARN(err_offset, "\"Format+\" is already in effect.");
                        }
                    }
                    else if (s_cmd[6] == '-')
                    {
                        check_command_length(err_offset, 7);
                        if (formatting)
                        {
                            if (in_para)
                            {
                                *g_src.curr++ = '\n';    // finish off current paragraph
                            }
                            in_para = false;
                            formatting = false;
                            num_spaces = 0;
                            state = ParseStates::START;
                        }
                        else
                        {
                            MSG_WARN(err_offset, "\"Format-\" is already in effect.");
                        }
                    }
                    else
                    {
                        MSG_ERROR(err_offset, "Invalid argument to Format.");
                    }
                }
                else if (string_case_equal(s_cmd, "Online", 6))
                {
                    toggle_mode("Online", CMD_XONLINE, s_xonline, err_offset);
                }
                else if (string_case_equal(s_cmd, "Doc", 3))
                {
                    toggle_mode("Doc", CMD_XDOC, s_xdoc, err_offset);
                }
                else if (string_case_equal(s_cmd, "ADoc", 4))
                {
                    toggle_mode("ADoc", CMD_XADOC, s_xadoc, err_offset);
                }
                else if (string_case_equal(s_cmd, "Center", 6))
                {
                    if (s_cmd[6] == '+')
                    {
                        check_command_length(err_offset, 7);
                        if (!centering)
                        {
                            centering = true;
                            if (in_para)
                            {
                                *g_src.curr++ = '\n';
                                in_para = false;
                            }
                            state = ParseStates::START;  // for centering FSM
                        }
                        else
                        {
                            MSG_WARN(err_offset, "\"Center+\" already in effect.");
                        }
                    }
                    else if (s_cmd[6] == '-')
                    {
                        check_command_length(err_offset, 7);
                        if (centering)
                        {
                            centering = false;
                            state = ParseStates::START;  // for centering FSM
                        }
                        else
                        {
                            MSG_WARN(err_offset, "\"Center-\" already in effect.");
                        }
                    }
                    else
                    {
                        MSG_ERROR(err_offset, "Invalid argument to Center.");
                    }
                }
                else if (string_case_equal(s_cmd, "CompressSpaces", 14))
                {
                    check_command_length(err_offset, 15);

                    if (s_cmd[14] == '+')
                    {
                        if (s_compress_spaces)
                        {
                            MSG_WARN(err_offset, "\"CompressSpaces+\" is already in effect.");
                        }
                        else
                        {
                            s_compress_spaces = true;
                        }
                    }
                    else if (s_cmd[14] == '-')
                    {
                        if (!s_compress_spaces)
                        {
                            MSG_WARN(err_offset, "\"CompressSpaces-\" is already in effect.");
                        }
                        else
                        {
                            s_compress_spaces = false;
                        }
                    }
                    else
                    {
                        MSG_ERROR(err_offset, "Invalid argument to CompressSpaces.");
                    }
                }
                else if (string_case_equal("BinInc ", s_cmd, 7))
                {
                    if (!bit_set(t.flags, TopicFlags::DATA))
                    {
                        MSG_ERROR(err_offset, "BinInc allowed only in Data topics.");
                    }
                    else
                    {
                        process_bin_inc();
                    }
                }
                else
                {
                    MSG_ERROR(err_offset, "Bad or unexpected command \"%s\".", s_cmd);
                }
                // else
            } // while (!done)

            continue;
        }

        if (!in_topic)
        {
            s_cmd[0] = ch;
            ptr = read_until(&s_cmd[1], 127, "\n~");
            if (*ptr == '~')
            {
                unread_char('~');
            }
            *ptr = '\0';
            MSG_ERROR(0, R"msg(Text outside any topic "%s".)msg", s_cmd);
            continue;
        }

        if (centering)
        {
            bool again;
            do
            {
                again = false;   // default

                if (state == ParseStates::START)
                {
                    if (ch == ' ')
                    {
                        // do nothing
                    }
                    else if ((ch&0xFF) == '\n')
                    {
                        *g_src.curr++ = ch;    // no need to center blank lines.
                    }
                    else
                    {
                        *g_src.curr++ = CMD_CENTER;
                        state = ParseStates::LINE;
                        again = true;
                    }
                }
                else if (state == ParseStates::LINE)
                {
                    put_a_char(ch, t);
                    if ((ch&0xFF) == '\n')
                    {
                        state = ParseStates::START;
                    }
                }
            }
            while (again);
        }
        else if (formatting)
        {
            bool again;

            do
            {
                again = false;   // default

                switch (state)
                {
                case ParseStates::START:
                    if ((ch&0xFF) == '\n')
                    {
                        *g_src.curr++ = ch;
                    }
                    else
                    {
                        state = ParseStates::START_FIRST_LINE;
                        num_spaces = 0;
                        again = true;
                    }
                    break;

                case ParseStates::START_FIRST_LINE:
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else
                    {
                        if (format_exclude > 0 && num_spaces >= format_exclude)
                        {
                            put_spaces(num_spaces);
                            num_spaces = 0;
                            state = ParseStates::FORMAT_DISABLED;
                            again = true;
                        }
                        else
                        {
                            *g_src.curr++ = CMD_PARA;
                            *g_src.curr++ = static_cast<char>(num_spaces);
                            *g_src.curr++ = static_cast<char>(num_spaces);
                            margin_pos = g_src.curr - 1;
                            state = ParseStates::FIRST_LINE;
                            again = true;
                            in_para = true;
                        }
                    }
                    break;

                case ParseStates::FIRST_LINE:
                    if (ch == '\n')
                    {
                        state = ParseStates::START_SECOND_LINE;
                        num_spaces = 0;
                    }
                    else if (ch == ('\n'|0x100))    // force end of para ?
                    {
                        *g_src.curr++ = '\n';
                        in_para = false;
                        state = ParseStates::START;
                    }
                    else if (ch == ' ')
                    {
                        state = ParseStates::FIRST_LINE_SPACES;
                        num_spaces = 1;
                    }
                    else
                    {
                        put_a_char(ch, t);
                    }
                    break;

                case ParseStates::FIRST_LINE_SPACES:
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else
                    {
                        put_spaces(num_spaces);
                        state = ParseStates::FIRST_LINE;
                        again = true;
                    }
                    break;

                case ParseStates::START_SECOND_LINE:
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else if ((ch&0xFF) == '\n') // a blank line means end of a para
                    {
                        *g_src.curr++ = '\n';   // end the para
                        *g_src.curr++ = '\n';   // for the blank line
                        in_para = false;
                        state = ParseStates::START;
                    }
                    else
                    {
                        if (format_exclude > 0 && num_spaces >= format_exclude)
                        {
                            *g_src.curr++ = '\n';
                            in_para = false;
                            put_spaces(num_spaces);
                            num_spaces = 0;
                            state = ParseStates::FORMAT_DISABLED;
                            again = true;
                        }
                        else
                        {
                            add_blank_for_split();
                            margin = num_spaces;
                            *margin_pos = static_cast<char>(num_spaces);
                            state = ParseStates::LINE;
                            again = true;
                        }
                    }
                    break;

                case ParseStates::LINE:   // all lines after the first
                    if (ch == '\n')
                    {
                        state = ParseStates::START_LINE;
                        num_spaces = 0;
                    }
                    else if (ch == ('\n' | 0x100))    // force end of para ?
                    {
                        *g_src.curr++ = '\n';
                        in_para = false;
                        state = ParseStates::START;
                    }
                    else if (ch == ' ')
                    {
                        state = ParseStates::LINE_SPACES;
                        num_spaces = 1;
                    }
                    else
                    {
                        put_a_char(ch, t);
                    }
                    break;

                case ParseStates::LINE_SPACES:
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else
                    {
                        put_spaces(num_spaces);
                        state = ParseStates::LINE;
                        again = true;
                    }
                    break;

                case ParseStates::START_LINE:   // for all lines after the second
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else if ((ch&0xFF) == '\n') // a blank line means end of a para
                    {
                        *g_src.curr++ = '\n';   // end the para
                        *g_src.curr++ = '\n';   // for the blank line
                        in_para = false;
                        state = ParseStates::START;
                    }
                    else
                    {
                        if (num_spaces != margin)
                        {
                            *g_src.curr++ = '\n';
                            in_para = false;
                            state = ParseStates::START_FIRST_LINE;  // with current num_spaces
                        }
                        else
                        {
                            add_blank_for_split();
                            state = ParseStates::LINE;
                        }
                        again = true;
                    }
                    break;

                case ParseStates::FORMAT_DISABLED:
                    if (ch == ' ')
                    {
                        state = ParseStates::FORMAT_DISABLED_SPACES;
                        num_spaces = 1;
                    }
                    else
                    {
                        if ((ch&0xFF) == '\n')
                        {
                            state = ParseStates::START;
                        }
                        put_a_char(ch, t);
                    }
                    break;

                case ParseStates::FORMAT_DISABLED_SPACES:
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else
                    {
                        put_spaces(num_spaces);
                        num_spaces = 0;    // is this needed?
                        state = ParseStates::FORMAT_DISABLED;
                        again = true;
                    }
                    break;

                case ParseStates::SPACES:
                    break;
                } // switch (state)
            }
            while (again);
        }
        else
        {
            bool again;
            do
            {
                again = false;   // default

                if (state == ParseStates::START)
                {
                    if (ch == ' ')
                    {
                        state = ParseStates::SPACES;
                        num_spaces = 1;
                    }
                    else
                    {
                        put_a_char(ch, t);
                    }
                }
                else if (state == ParseStates::SPACES)
                {
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else
                    {
                        put_spaces(num_spaces);
                        num_spaces = 0;     // is this needed?
                        state = ParseStates::START;
                        again = true;
                    }
                }
            }
            while (again);
        }

        check_buffer(0);
    } // while ( 1 )

    std::fclose(s_src_file);

    g_src_line = -1;
}

} // namespace hc
