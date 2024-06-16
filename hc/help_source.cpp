#include "help_source.h"

#include "html_processor.h"
#include "messages.h"
#include "modes.h"

#include <helpcom.h>
#include <id_io.h>
#include <port.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <stdexcept>
#include <system_error>

#ifdef XFRACT
#ifndef HAVESTRI
extern int stricmp(char const *, char const *);
extern int strnicmp(char const *, char const *, int);
#endif
extern int filelength(int);
#endif

#if defined(_WIN32)
// disable unsafe CRT warnings
#pragma warning(disable: 4996)
#endif

#if !defined(O_BINARY)
#define O_BINARY 0
#endif

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

std::FILE *g_src_file{};                  // .SRC file
int g_src_col{};                          // .SRC column.
bool g_compress_spaces{};                 //
char g_cmd[128]{};                        // holds the current command
int g_format_exclude{};                   // disable formatting at this col, 0 to never disable formatting
std::vector<std::string> g_include_paths; //
bool g_xonline{};                         //
bool g_xdoc{};                            //
std::vector<Include> g_include_stack;     //
int g_version{-1};                        // help file version

static int s_read_char_buff[READ_CHAR_BUFF_SIZE];
static int s_read_char_buff_pos{-1};
static int s_read_char_sp{};

void CONTENT::label_topic(int ctr)
{
    if (LABEL *lbl = g_src.find_label(topic_name[ctr].c_str()))
    {
        if (g_src.topics[lbl->topic_num].flags & TF_DATA)
        {
            g_current_src_filename = srcfile;
            g_src_line = srcline;
            error(0, "Label \"%s\" is a data-only topic.", topic_name[ctr].c_str());
            g_src_line = -1;
        }
        else
        {
            topic_num[ctr] = lbl->topic_num;
            if (g_src.topics[lbl->topic_num].flags & TF_IN_DOC)
            {
                warn(0, "Topic \"%s\" appears in document more than once.",
                    g_src.topics[lbl->topic_num].title.c_str());
            }
            else
            {
                g_src.topics[lbl->topic_num].flags |= TF_IN_DOC;
            }
        }
    }
    else
    {
        g_current_src_filename = srcfile;
        g_src_line = srcline;
        error(0, "Cannot find DocContent label \"%s\".", topic_name[ctr].c_str());
        g_src_line = -1;
    }
}

void CONTENT::content_topic(int ctr)
{
    int const t = find_topic_title(topic_name[ctr].c_str());
    if (t == -1)
    {
        g_current_src_filename = srcfile;
        g_src_line = srcline;
        error(0, "Cannot find DocContent topic \"%s\".", topic_name[ctr].c_str());
        g_src_line = -1;  // back to reality
    }
    else
    {
        topic_num[ctr] = t;
        if (g_src.topics[t].flags & TF_IN_DOC)
        {
            warn(0, "Topic \"%s\" appears in document more than once.",
                g_src.topics[t].title.c_str());
        }
        else
        {
            g_src.topics[t].flags |= TF_IN_DOC;
        }
    }
}


void LINK::link_topic()
{
    int const t = find_topic_title(name.c_str());
    if (t == -1)
    {
        g_current_src_filename = srcfile;
        g_src_line = srcline; // pretend we are still in the source...
        error(0, "Cannot find implicit hot-link \"%s\".", name.c_str());
        g_src_line = -1;  // back to reality
    }
    else
    {
        topic_num = t;
        topic_off = 0;
        doc_page = (g_src.topics[t].flags & TF_IN_DOC) ? 0 : -1;
    }
}

void LINK::link_label()
{
    if (LABEL *lbl = g_src.find_label(name.c_str()))
    {
        if (g_src.topics[lbl->topic_num].flags & TF_DATA)
        {
            g_current_src_filename = srcfile;
            g_src_line = srcline;
            error(0, "Label \"%s\" is a data-only topic.", name.c_str());
            g_src_line = -1;
        }
        else
        {
            topic_num = lbl->topic_num;
            topic_off = lbl->topic_off;
            doc_page  = (g_src.topics[lbl->topic_num].flags & TF_IN_DOC) ? 0 : -1;
        }
    }
    else
    {
        g_current_src_filename = srcfile;
        g_src_line = srcline; // pretend again
        error(0, "Cannot find explicit hot-link \"%s\".", name.c_str());
        g_src_line = -1;
    }
}


void TOPIC::alloc_topic_text(unsigned size)
{
    text_len = size;
    text = g_src.swap_pos;
    g_src.swap_pos += size;
    std::fseek(g_src.swap_file, text, SEEK_SET);
    std::fwrite(g_src.buffer.data(), 1, text_len, g_src.swap_file);
}

int TOPIC::add_page(const PAGE &p)
{
    page.push_back(p);
    return num_page++;
}

void TOPIC::add_page_break(int margin, char const *text, char const *start, char const *curr, int num_links)
{
    PAGE p;
    p.offset = (unsigned)(start - text);
    p.length = (unsigned)(curr - start);
    p.margin = margin;
    add_page(p);

    if (g_src.max_links < num_links)
    {
        g_src.max_links = num_links;
    }
}

char *TOPIC::get_topic_text()
{
    read_topic_text();
    return g_src.buffer.data();
}

const char *TOPIC::get_topic_text() const
{
    read_topic_text();
    return g_src.buffer.data();
}

void TOPIC::release_topic_text(bool save) const
{
    if (save)
    {
        std::fseek(g_src.swap_file, text, SEEK_SET);
        std::fwrite(g_src.buffer.data(), 1, text_len, g_src.swap_file);
    }
}

void TOPIC::start(char const *text, int len)
{
    flags = 0;
    title_len = len;
    title.assign(text, len);
    doc_page = -1;
    num_page = 0;
    g_src.curr = g_src.buffer.data();
}

void TOPIC::read_topic_text() const
{
    std::fseek(g_src.swap_file, text, SEEK_SET);
    if (std::fread(g_src.buffer.data(), 1, text_len, g_src.swap_file) != text_len)
    {
        throw std::system_error(errno, std::system_category(), "get_topic_text failed fread");
    }
}

void check_buffer(char const *curr, unsigned off, char const *buffer);

inline void check_buffer(unsigned off)
{
    check_buffer(g_src.curr, off, g_src.buffer.data());
}

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4311)
#endif
void check_buffer(char const *curr, unsigned int off, char const *buffer)
{
    if ((unsigned)(curr + off - buffer) >= (BUFFER_SIZE-1024))
    {
        throw std::runtime_error("Buffer overflowerd -- Help topic too large.");
    }
}
#if defined(_WIN32)
#pragma warning(pop)
#endif

LABEL *HelpSource::find_label(char const *name)
{
    auto finder = [=](std::vector<LABEL> &collection) -> LABEL *
    {
        for (LABEL &label : collection)
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

int find_topic_title(char const *title)
{
    while (*title == ' ')
    {
        ++title;
    }

    int len = (int) std::strlen(title) - 1;
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
        if ((int) g_src.topics[t].title.length() == len
            && strnicmp(title, g_src.topics[t].title.c_str(), len) == 0)
        {
            return t;
        }
    }

    return -1;   // not found
}

/*
 * memory-allocation functions.
 */
int HelpSource::add_link(LINK &l)
{
    all_links.push_back(l);
    return static_cast<int>(all_links.size() - 1);
}

int HelpSource::add_topic(const TOPIC &t)
{
    topics.push_back(t);
    return static_cast<int>(topics.size() - 1);
}

int HelpSource::add_label(const LABEL &l)
{
    if (l.name[0] == '@')    // if it's a private label...
    {
        private_labels.push_back(l);
        return static_cast<int>(private_labels.size() - 1);
    }

    labels.push_back(l);
    return static_cast<int>(labels.size() - 1);
}

int HelpSource::add_content(const CONTENT &c)
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
void unread_char(int ch)
{
    if (s_read_char_buff_pos+1 >= READ_CHAR_BUFF_SIZE)
    {
        throw std::runtime_error("Compiler Error -- Read char buffer overflow!");
    }

    s_read_char_buff[++s_read_char_buff_pos] = ch;

    --g_src_col;
}

int read_char_aux()
{
    int ch;

    if (g_src_line <= 0)
    {
        g_src_line = 1;
        g_src_col = 0;
    }

    if (s_read_char_buff_pos >= 0)
    {
        ++g_src_col;
        return s_read_char_buff[s_read_char_buff_pos--];
    }

    if (s_read_char_sp > 0)
    {
        --s_read_char_sp;
        return ' ';
    }

    if (std::feof(g_src_file))
    {
        return -1;
    }

    while (true)
    {
        ch = getc(g_src_file);

        switch (ch)
        {
        case '\t':    // expand a tab
        {
            int diff = (((g_src_col/8) + 1) * 8) - g_src_col;

            g_src_col += diff;
            s_read_char_sp += diff;
            break;
        }

        case ' ':
            ++g_src_col;
            ++s_read_char_sp;
            break;

        case '\n':
            s_read_char_sp = 0;   // delete spaces before a \n
            g_src_col = 0;
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
                ungetc(ch, g_src_file);
                --s_read_char_sp;
                return ' ';
            }

            ++g_src_col;
            return ch;

        } // switch
    }
}

int read_char()
{
    int ch;

    ch = read_char_aux();

    while (ch == ';' && g_src_col == 1)    // skip over comments
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

            for (ctr = 0; true; ctr++)
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

#ifdef XFRACT
        // Convert graphics arrows into keyboard chars
        if (ch >= 24 && ch <= 27)
        {
            ch = "KJHL"[ch-24];
        }
#endif
        ch |= 0x100;
    }

    if ((ch & 0xFF) == 0)
    {
        error(0, "Null character (\'\\0\') not allowed!");
        ch = 0x1FF; // since we've had an error the file will not be written;
        //   the value we return doesn't really matter
    }

    return ch;
}

/*
 * .SRC file parser stuff
 */
bool validate_label_name(char const *name)
{
    if (!std::isalpha(*name) && *name != '@' && *name != '_')
    {
        return false;    // invalid
    }

    while (*(++name) != '\0')
    {
        if (!std::isalpha(*name) && !std::isdigit(*name) && *name != '_')
        {
            return false;    // invalid
        }
    }

    return true;  // valid
}

char *read_until(char *buff, int len, char const *stop_chars)
{
    int ch;

    while (--len > 0)
    {
        ch = read_char();

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

void skip_over(char const *skip)
{
    int ch;

    while (true)
    {
        ch = read_char();

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

char *pchar(int ch)
{
    static char buff[16];

    if (ch >= 0x20 && ch <= 0x7E)
    {
        std::sprintf(buff, "\'%c\'", ch);
    }
    else
    {
        std::sprintf(buff, "\'\\x%02X\'", ch&0xFF);
    }

    return buff;
}

void put_spaces(int how_many)
{
    if (how_many > 2 && g_compress_spaces)
    {
        if (how_many > 255)
        {
            error(0, "Too many spaces (over 255).");
            how_many = 255;
        }

        *g_src.curr++ = CMD_SPACE;
        *g_src.curr++ = (BYTE)how_many;
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
bool get_next_item()
{
    skip_over(" \t\r\n");
    char *ptr = read_until(g_cmd, 128, ",}");
    bool last = (*ptr == '}');
    --ptr;
    while (ptr >= g_cmd && std::strchr(" \t\r\n", *ptr))     // strip trailing spaces
    {
        --ptr;
    }
    *(++ptr) = '\0';

    return last;
}

void process_doc_contents(char * (*format_toc)(char *buffer, CONTENT &c))
{
    TOPIC t;
    t.flags     = 0;
    t.title_len = (unsigned) std::strlen(DOCCONTENTS_TITLE)+1;
    t.title     = DOCCONTENTS_TITLE;
    t.doc_page  = -1;
    t.num_page  = 0;

    g_src.curr = g_src.buffer.data();

    CONTENT c{};
    c.flags = 0;
    c.id.clear();
    c.name.clear();
    c.doc_page = -1;
    c.page_num_pos = 0;
    c.num_topic = 1;
    c.is_label[0] = false;
    c.topic_name[0] = DOCCONTENTS_TITLE;
    c.srcline = -1;
    g_src.add_content(c);

    while (true)
    {
        int const ch = read_char();
        if (ch == '{')   // process a CONTENT entry
        {
            c.flags = 0;
            c.num_topic = 0;
            c.doc_page = -1;
            c.srcfile = g_current_src_filename;
            c.srcline = g_src_line;

            if (get_next_item())
            {
                error(0, "Unexpected end of DocContent entry.");
                continue;
            }
            c.id = g_cmd;

            if (get_next_item())
            {
                error(0, "Unexpected end of DocContent entry.");
                continue;
            }

            bool last = get_next_item();

            if (g_cmd[0] == '\"')
            {
                char *ptr = &g_cmd[1];
                if (ptr[(int) std::strlen(ptr)-1] == '\"')
                {
                    ptr[(int) std::strlen(ptr)-1] = '\0';
                }
                else
                {
                    warn(0, "Missing ending quote.");
                }

                c.is_label[c.num_topic] = false;
                c.topic_name[c.num_topic] = ptr;
                ++c.num_topic;
                c.name = ptr;
            }
            else
            {
                c.name = g_cmd;
            }

            // now, make the entry in the buffer
            g_src.curr = format_toc(g_src.curr, c);

            while (!last)
            {
                last = get_next_item();
                if (stricmp(g_cmd, "FF") == 0)
                {
                    if (c.flags & CF_NEW_PAGE)
                    {
                        warn(0, "FF already present in this entry.");
                    }
                    c.flags |= CF_NEW_PAGE;
                    continue;
                }

                if (g_cmd[0] == '\"')
                {
                    char *ptr = &g_cmd[1];
                    if (ptr[(int) std::strlen(ptr)-1] == '\"')
                    {
                        ptr[(int) std::strlen(ptr)-1] = '\0';
                    }
                    else
                    {
                        warn(0, "Missing ending quote.");
                    }

                    c.is_label[c.num_topic] = false;
                    c.topic_name[c.num_topic] = ptr;
                }
                else
                {
                    c.is_label[c.num_topic] = true;
                    c.topic_name[c.num_topic] = g_cmd;
                }

                if (++c.num_topic >= MAX_CONTENT_TOPIC)
                {
                    error(0, "Too many topics in DocContent entry.");
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

    t.alloc_topic_text((unsigned)(g_src.curr - g_src.buffer.data()));
    g_src.add_topic(t);
}

void process_doc_contents(modes mode)
{

    if (mode == modes::HTML)
    {
        process_doc_contents(
            [](char *buffer, CONTENT &c)
            {
                std::sprintf(buffer, "%s", rst_name(c.name).c_str());
                c.page_num_pos = 0U;
                return buffer + (int) std::strlen(buffer);
            });
    }
    else
    {
        process_doc_contents(
            [](char *buffer, CONTENT &c)
            {
                const int indent = std::atoi(g_cmd);
                std::sprintf(buffer, "%-5s %*.0s%s", c.id.c_str(), indent * 2, "", c.name.c_str());
                char *ptr = buffer + (int) std::strlen(buffer);
                while ((ptr - buffer) < PAGE_WIDTH - 10)
                {
                    *ptr++ = '.';
                }
                c.page_num_pos = (unsigned) ((ptr - 3) - g_src.buffer.data());
                return ptr;
            });
    }
}

int parse_link()   // returns length of link or 0 on error
{
    char *ptr;
    bool bad = false;
    int   len;
    int   err_off;

    LINK l;
    l.srcfile  = g_current_src_filename;
    l.srcline  = g_src_line;
    l.doc_page = -1;

    char *end = read_until(g_cmd, 128, "}\n");   // get the entire hot-link

    if (*end == '\0')
    {
        error(0, "Unexpected EOF in hot-link.");
        return 0;
    }

    if (*end == '\n')
    {
        err_off = 1;
        warn(1, "Hot-link has no closing curly-brace (\'}\').");
    }
    else
    {
        err_off = 0;
    }

    *end = '\0';

    if (g_cmd[0] == '=')   // it's an "explicit" link to a label or "special"
    {
        ptr = std::strchr(g_cmd, ' ');

        if (ptr == nullptr)
        {
            ptr = end;
        }
        else
        {
            *ptr++ = '\0';
        }

        len = (int)(end - ptr);

        if (g_cmd[1] == '-')
        {
            l.type      = link_types::LT_SPECIAL;
            l.topic_num = std::atoi(&g_cmd[1]);
            l.topic_off = 0;
            l.name.clear();
        }
        else
        {
            l.type = link_types::LT_LABEL;
            if ((int)std::strlen(g_cmd) > 32)
            {
                warn(err_off, "Label is long.");
            }
            if (g_cmd[1] == '\0')
            {
                error(err_off, "Explicit hot-link has no Label.");
                bad = true;
            }
            else
            {
                l.name = &g_cmd[1];
            }
        }
        if (len == 0)
        {
            warn(err_off, "Explicit hot-link has no title.");
        }
    }
    else
    {
        ptr = g_cmd;
        l.type = link_types::LT_TOPIC;
        len = (int)(end - ptr);
        if (len == 0)
        {
            error(err_off, "Implicit hot-link has no title.");
            bad = true;
        }
        l.name.assign(ptr, len);
    }

    if (!bad)
    {
        check_buffer(1+3*sizeof(int)+len+1);
        int const lnum = g_src.add_link(l);
        *g_src.curr++ = CMD_LINK;
        setint(g_src.curr, lnum);
        g_src.curr += 3*sizeof(int);
        std::memcpy(g_src.curr, ptr, len);
        g_src.curr += len;
        *g_src.curr++ = CMD_LINK;
        return len;
    }

    return 0;
}

int create_table()
{
    char  *ptr;
    int    width;
    int    cols;
    int    start_off;
    int    first_link;
    int    rows;
    int    ch;
    int    len;
    int    lnum;
    int    count;
    std::vector<std::string> title;
    char  *table_start;

    ptr = std::strchr(g_cmd, '=');

    if (ptr == nullptr)
    {
        return 0;    // should never happen!
    }

    ptr++;

    len = std::sscanf(ptr, " %d %d %d", &width, &cols, &start_off);

    if (len < 3)
    {
        error(1, "Too few arguments to Table.");
        return 0;
    }

    if (width <= 0 || width > 78 || cols <= 0 || start_off < 0 || start_off > 78)
    {
        error(1, "Argument out of range.");
        return 0;
    }

    bool done = false;

    first_link = static_cast<int>(g_src.all_links.size());
    table_start = g_src.curr;
    count = 0;

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
            error(0, "Unexpected EOF in a Table.");
            return 0;

        case '{':
            if (count >= MAX_TABLE_SIZE)
            {
                throw std::runtime_error("Table is too large.");
            }
            len = parse_link();
            g_src.curr = table_start;   // reset to the start...
            title.push_back(std::string(g_src.curr+3*sizeof(int)+1, len+1));
            if (len >= width)
            {
                warn(1, "Link is too long; truncating.");
                len = width-1;
            }
            title[count][len] = '\0';
            ++count;
            break;

        case '~':
        {
            bool imbedded;

            ch = read_char();

            if (ch == '(')
            {
                imbedded = true;
            }
            else
            {
                imbedded = false;
                unread_char(ch);
            }

            ptr = read_until(g_cmd, 128, ")\n,");

            ch = *ptr;
            *ptr = '\0';

            if (stricmp(g_cmd, "EndTable") == 0)
            {
                done = true;
            }
            else
            {
                error(1, "Unexpected command in table \"%s\"", g_cmd);
                warn(1, "Command will be ignored.");
            }

            if (ch == ',')
            {
                if (imbedded)
                {
                    unread_char('(');
                }
                unread_char('~');
            }
        }
        break;

        default:
            error(0, "Unexpected character %s.", pchar(ch));
            break;
        }
    }
    while (!done);

    // now, put all the links into the buffer...

    rows = 1 + (count / cols);

    for (int r = 0; r < rows; r++)
    {
        put_spaces(start_off);
        for (int c = 0; c < cols; c++)
        {
            lnum = c*rows + r;

            if (first_link+lnum >= static_cast<int>(g_src.all_links.size()))
            {
                break;
            }

            len = static_cast<int>(title[lnum].length());
            *g_src.curr++ = CMD_LINK;
            setint(g_src.curr, first_link+lnum);
            g_src.curr += 3*sizeof(int);
            std::memcpy(g_src.curr, title[lnum].c_str(), len);
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

void process_comment()
{
    int ch;

    while (true)
    {
        ch = read_char();

        if (ch == '~')
        {
            bool imbedded;
            char *ptr;

            ch = read_char();

            if (ch == '(')
            {
                imbedded = true;
            }
            else
            {
                imbedded = false;
                unread_char(ch);
            }

            ptr = read_until(g_cmd, 128, ")\n,");

            ch = *ptr;
            *ptr = '\0';

            if (stricmp(g_cmd, "EndComment") == 0)
            {
                if (ch == ',')
                {
                    if (imbedded)
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
            error(0, "Unexpected EOF in Comment");
            break;
        }
    }
}

void process_bininc()
{
    int  handle;
    long len;

    handle = open(&g_cmd[7], O_RDONLY|O_BINARY);
    if (handle == -1)
    {
        error(0, "Unable to open \"%s\"", &g_cmd[7]);
        return ;
    }

    len = filelength(handle);

    if (len >= BUFFER_SIZE)
    {
        error(0, "File \"%s\" is too large to BinInc (%dK).", &g_cmd[7], (int)(len >> 10));
        close(handle);
        return ;
    }

    /*
     * Since we know len is less than BUFFER_SIZE (and therefore less then
     * 64K) we can treat it as an unsigned.
     */

    check_buffer((unsigned)len);

    if (read(handle, g_src.curr, (unsigned)len) != len)
    {
        throw std::system_error(errno, std::system_category(), "process_bininc failed read");
    }

    g_src.curr += (unsigned)len;

    close(handle);
}

void end_topic(TOPIC &t)
{
    t.alloc_topic_text((unsigned)(g_src.curr - g_src.buffer.data()));
    g_src.add_topic(t);
}

bool end_of_sentence(char const *ptr)  // true if ptr is at the end of a sentence
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

void add_blank_for_split()   // add space at g_src.curr for merging two lines
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

void put_a_char(int ch, const TOPIC &t)
{
    if (ch == '{' && !(t.flags & TF_DATA))    // is it a hot-link?
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

enum class STATES   // states for FSM's
{
    S_Start,                 // initial state, between paragraphs
    S_StartFirstLine,        // spaces at start of first line
    S_FirstLine,             // text on the first line
    S_FirstLineSpaces,       // spaces on the first line
    S_StartSecondLine,       // spaces at start of second line
    S_Line,                  // text on lines after the first
    S_LineSpaces,            // spaces on lines after the first
    S_StartLine,             // spaces at start of lines after second
    S_FormatDisabled,        // format automatically disabled for this line
    S_FormatDisabledSpaces,  // spaces in line which format is disabled
    S_Spaces
};

void check_command_length(int eoff, int len)
{
    if ((int) std::strlen(g_cmd) != len)
    {
        error(eoff, "Invalid text after a command \"%s\"", g_cmd+len);
    }
}

std::FILE *open_include(std::string const &file_name)
{
    std::FILE *result = std::fopen(file_name.c_str(), "rt");
    if (result == nullptr)
    {
        for (std::string const &dir : g_include_paths)
        {
            std::string const path{dir + '/' + file_name};
            result = std::fopen(path.c_str(), "rt");
            if (result != nullptr)
            {
                return result;
            }
        }
    }
    return result;
}

void read_src(std::string const &fname, modes mode)
{
    int    ch;
    char  *ptr;
    TOPIC  t;
    LABEL  lbl;
    char  *margin_pos = nullptr;
    bool in_topic = false;
    bool formatting = true;
    STATES state = STATES::S_Start;
    int    num_spaces = 0;
    int    margin     = 0;
    bool in_para = false;
    bool centering = false;
    int    lformat_exclude = g_format_exclude;

    g_xonline = false;
    g_xdoc = false;

    g_current_src_filename = fname;

    g_src_file = open_include(fname);
    if (g_src_file == nullptr)
    {
        throw std::runtime_error("Unable to open \"" + fname + "\"");
    }

    msg("Compiling: %s", fname.c_str());

    in_topic = false;

    g_src.curr = g_src.buffer.data();

    while (true)
    {
        ch = read_char();

        if (ch == -1)     // EOF?
        {
            if (!g_include_stack.empty())
            {
                std::fclose(g_src_file);
                const Include &top{g_include_stack.back()};
                g_current_src_filename = top.fname;
                g_src_file = top.file;
                g_src_line = top.line;
                g_src_col = top.col;
                g_include_stack.pop_back();
                continue;
            }
            if (in_topic)  // if we're in a topic, finish it
            {
                end_topic(t);
            }
            if (g_src.topics.empty())
            {
                warn(0, ".SRC file has no topics.");
            }
            break;
        }

        if (ch == '~')   // is is a command?
        {
            bool imbedded;

            ch = read_char();
            if (ch == '(')
            {
                imbedded = true;
            }
            else
            {
                imbedded = false;
                unread_char(ch);
            }

            bool done = false;

            int eoff{};
            while (!done)
            {
                do
                {
                    ch = read_char();
                }
                while (ch == ' ');
                unread_char(ch);

                ptr = read_until(g_cmd, 128, imbedded ? ")\n," : "\n,");

                if (*ptr == '\0')
                {
                    error(0, "Unexpected EOF in command.");
                    break;
                }

                if (*ptr == '\n')
                {
                    ++eoff;
                }

                if (imbedded && *ptr == '\n')
                {
                    error(eoff, "Embedded command has no closing paren (\')\')");
                }

                done = (*ptr != ',');   // we done if it's not a comma

                if (*ptr != '\n' && *ptr != ')' && *ptr != ',')
                {
                    error(0, "Command line too long.");
                    break;
                }

                *ptr = '\0';

                // commands allowed anytime...
                if (strnicmp(g_cmd, "Topic=", 6) == 0)
                {
                    if (in_topic)  // if we're in a topic, finish it
                    {
                        end_topic(t);
                    }
                    else
                    {
                        in_topic = true;
                    }

                    char const *topic_title = &g_cmd[6];
                    size_t const title_len = std::strlen(topic_title);
                    if (title_len == 0)
                    {
                        warn(eoff, "Topic has no title.");
                    }
                    else if (title_len > 70)
                    {
                        error(eoff, "Topic title is too long.");
                    }
                    else if (title_len > 60)
                    {
                        warn(eoff, "Topic title is long.");
                    }

                    if (find_topic_title(topic_title) != -1)
                    {
                        error(eoff, "Topic title already exists.");
                    }

                    t.start(topic_title, static_cast<int>(title_len));
                    formatting = true;
                    centering = false;
                    state = STATES::S_Start;
                    in_para = false;
                    num_spaces = 0;
                    g_xonline = false;
                    g_xdoc = false;
                    lformat_exclude = g_format_exclude;
                    g_compress_spaces = true;
                    continue;
                }
                if (strnicmp(g_cmd, "Data=", 5) == 0)
                {
                    if (in_topic)  // if we're in a topic, finish it
                    {
                        end_topic(t);
                    }
                    else
                    {
                        in_topic = true;
                    }

                    char const *data = &g_cmd[5];
                    if (data[0] == '\0')
                    {
                        warn(eoff, "Data topic has no label.");
                    }

                    if (!validate_label_name(data))
                    {
                        error(eoff, "Label \"%s\" contains illegal characters.", data);
                        continue;
                    }

                    if (g_src.find_label(data) != nullptr)
                    {
                        error(eoff, "Label \"%s\" already exists", data);
                        continue;
                    }

                    if (g_cmd[5] == '@')
                    {
                        warn(eoff, "Data topic has a local label.");
                    }

                    t.start("", 0);
                    t.flags |= TF_DATA;

                    if ((int)std::strlen(data) > 32)
                    {
                        warn(eoff, "Label name is long.");
                    }

                    lbl.name      = data;
                    lbl.topic_num = static_cast<int>(g_src.topics.size());
                    lbl.topic_off = 0;
                    lbl.doc_page  = -1;
                    g_src.add_label(lbl);

                    formatting = false;
                    centering = false;
                    state = STATES::S_Start;
                    in_para = false;
                    num_spaces = 0;
                    g_xonline = false;
                    g_xdoc = false;
                    lformat_exclude = g_format_exclude;
                    g_compress_spaces = false;
                    continue;
                }
                if (strnicmp(g_cmd, "DocContents", 11) == 0)
                {
                    check_command_length(eoff, 11);
                    if (in_topic)  // if we're in a topic, finish it
                    {
                        end_topic(t);
                    }
                    if (!done)
                    {
                        if (imbedded)
                        {
                            unread_char('(');
                        }
                        unread_char('~');
                        done = true;
                    }
                    g_compress_spaces = true;
                    process_doc_contents(mode);
                    in_topic = false;
                    continue;
                }
                if (stricmp(g_cmd, "Comment") == 0)
                {
                    process_comment();
                    continue;
                }
                if (strnicmp(g_cmd, "FormatExclude", 13) == 0)
                {
                    if (g_cmd[13] == '-')
                    {
                        check_command_length(eoff, 14);
                        if (in_topic)
                        {
                            if (lformat_exclude > 0)
                            {
                                lformat_exclude = -lformat_exclude;
                            }
                            else
                            {
                                warn(eoff, "\"FormatExclude-\" is already in effect.");
                            }
                        }
                        else
                        {
                            if (g_format_exclude > 0)
                            {
                                g_format_exclude = -g_format_exclude;
                            }
                            else
                            {
                                warn(eoff, "\"FormatExclude-\" is already in effect.");
                            }
                        }
                    }
                    else if (g_cmd[13] == '+')
                    {
                        check_command_length(eoff, 14);
                        if (in_topic)
                        {
                            if (lformat_exclude < 0)
                            {
                                lformat_exclude = -lformat_exclude;
                            }
                            else
                            {
                                warn(eoff, "\"FormatExclude+\" is already in effect.");
                            }
                        }
                        else
                        {
                            if (g_format_exclude < 0)
                            {
                                g_format_exclude = -g_format_exclude;
                            }
                            else
                            {
                                warn(eoff, "\"FormatExclude+\" is already in effect.");
                            }
                        }
                    }
                    else if (g_cmd[13] == '=')
                    {
                        if (g_cmd[14] == 'n' || g_cmd[14] == 'N')
                        {
                            check_command_length(eoff, 15);
                            if (in_topic)
                            {
                                lformat_exclude = 0;
                            }
                            else
                            {
                                g_format_exclude = 0;
                            }
                        }
                        else if (g_cmd[14] == '\0')
                        {
                            lformat_exclude = g_format_exclude;
                        }
                        else
                        {
                            int n = ((in_topic ? lformat_exclude : g_format_exclude) < 0) ? -1 : 1;

                            lformat_exclude = std::atoi(&g_cmd[14]);

                            if (lformat_exclude <= 0)
                            {
                                error(eoff, "Invalid argument to FormatExclude=");
                                lformat_exclude = 0;
                            }

                            lformat_exclude *= n;

                            if (!in_topic)
                            {
                                g_format_exclude = lformat_exclude;
                            }
                        }
                    }
                    else
                    {
                        error(eoff, "Invalid format for FormatExclude");
                    }

                    continue;
                }
                if (strnicmp(g_cmd, "Include ", 8) == 0)
                {
                    const std::string file_name = &g_cmd[8];
                    if (FILE *new_file = open_include(file_name))
                    {
                        Include top{};
                        top.fname = g_current_src_filename;
                        top.file = g_src_file;
                        top.line = g_src_line;
                        top.col  = g_src_col;
                        g_include_stack.push_back(top);
                        g_src_file = new_file;
                        g_current_src_filename = file_name;
                        g_src_line = 1;
                        g_src_col = 0;
                    }
                    else
                    {
                        error(eoff, "Unable to open \"%s\"", file_name.c_str());
                    }
                    continue;
                }


                // commands allowed only before all topics...

                if (!in_topic)
                {
                    if (strnicmp(g_cmd, "HdrFile=", 8) == 0)
                    {
                        if (!g_src.hdr_filename.empty())
                        {
                            warn(eoff, "Header Filename has already been defined.");
                        }
                        g_src.hdr_filename = &g_cmd[8];
                    }
                    else if (strnicmp(g_cmd, "HlpFile=", 8) == 0)
                    {
                        if (!g_src.hlp_filename.empty())
                        {
                            warn(eoff, "Help Filename has already been defined.");
                        }
                        g_src.hlp_filename = &g_cmd[8];
                    }
                    else if (strnicmp(g_cmd, "Version=", 8) == 0)
                    {
                        if (g_version != -1)   // an unlikely value
                        {
                            warn(eoff, "Help version has already been defined");
                        }
                        g_version = std::atoi(&g_cmd[8]);
                    }
                    else
                    {
                        error(eoff, "Bad or unexpected command \"%s\"", g_cmd);
                    }

                    continue;
                }
                // commands allowed only in a topic...
                if (strnicmp(g_cmd, "FF", 2) == 0)
                {
                    check_command_length(eoff, 2);
                    if (in_para)
                    {
                        *g_src.curr++ = '\n';    // finish off current paragraph
                    }
                    *g_src.curr++ = CMD_FF;
                    state = STATES::S_Start;
                    in_para = false;
                    num_spaces = 0;
                }
                else if (strnicmp(g_cmd, "DocFF", 5) == 0)
                {
                    check_command_length(eoff, 5);
                    if (in_para)
                    {
                        *g_src.curr++ = '\n';    // finish off current paragraph
                    }
                    if (!g_xonline)
                    {
                        *g_src.curr++ = CMD_XONLINE;
                    }
                    *g_src.curr++ = CMD_FF;
                    if (!g_xonline)
                    {
                        *g_src.curr++ = CMD_XONLINE;
                    }
                    state = STATES::S_Start;
                    in_para = false;
                    num_spaces = 0;
                }
                else if (strnicmp(g_cmd, "OnlineFF", 8) == 0)
                {
                    check_command_length(eoff, 8);
                    if (in_para)
                    {
                        *g_src.curr++ = '\n';    // finish off current paragraph
                    }
                    if (!g_xdoc)
                    {
                        *g_src.curr++ = CMD_XDOC;
                    }
                    *g_src.curr++ = CMD_FF;
                    if (!g_xdoc)
                    {
                        *g_src.curr++ = CMD_XDOC;
                    }
                    state = STATES::S_Start;
                    in_para = false;
                    num_spaces = 0;
                }
                else if (strnicmp(g_cmd, "Label=", 6) == 0)
                {
                    char const *label_name = &g_cmd[6];
                    if ((int)std::strlen(label_name) <= 0)
                    {
                        error(eoff, "Label has no name.");
                    }
                    else if (!validate_label_name(label_name))
                    {
                        error(eoff, "Label \"%s\" contains illegal characters.", label_name);
                    }
                    else if (g_src.find_label(label_name) != nullptr)
                    {
                        error(eoff, "Label \"%s\" already exists", label_name);
                    }
                    else
                    {
                        if ((int)std::strlen(label_name) > 32)
                        {
                            warn(eoff, "Label name is long.");
                        }

                        if ((t.flags & TF_DATA) && g_cmd[6] == '@')
                        {
                            warn(eoff, "Data topic has a local label.");
                        }

                        lbl.name      = label_name;
                        lbl.topic_num = static_cast<int>(g_src.topics.size());
                        lbl.topic_off = (unsigned)(g_src.curr - g_src.buffer.data());
                        lbl.doc_page  = -1;
                        g_src.add_label(lbl);
                    }
                }
                else if (strnicmp(g_cmd, "Table=", 6) == 0)
                {
                    if (in_para)
                    {
                        *g_src.curr++ = '\n';  // finish off current paragraph
                        in_para = false;
                        num_spaces = 0;
                        state = STATES::S_Start;
                    }

                    if (!done)
                    {
                        if (imbedded)
                        {
                            unread_char('(');
                        }
                        unread_char('~');
                        done = true;
                    }

                    create_table();
                }
                else if (strnicmp(g_cmd, "FormatExclude", 12) == 0)
                {
                    if (g_cmd[13] == '-')
                    {
                        check_command_length(eoff, 14);
                        if (lformat_exclude > 0)
                        {
                            lformat_exclude = -lformat_exclude;
                        }
                        else
                        {
                            warn(0, "\"FormatExclude-\" is already in effect.");
                        }
                    }
                    else if (g_cmd[13] == '+')
                    {
                        check_command_length(eoff, 14);
                        if (lformat_exclude < 0)
                        {
                            lformat_exclude = -lformat_exclude;
                        }
                        else
                        {
                            warn(0, "\"FormatExclude+\" is already in effect.");
                        }
                    }
                    else
                    {
                        error(eoff, "Unexpected or invalid argument to FormatExclude.");
                    }
                }
                else if (strnicmp(g_cmd, "Format", 6) == 0)
                {
                    if (g_cmd[6] == '+')
                    {
                        check_command_length(eoff, 7);
                        if (!formatting)
                        {
                            formatting = true;
                            in_para = false;
                            num_spaces = 0;
                            state = STATES::S_Start;
                        }
                        else
                        {
                            warn(eoff, "\"Format+\" is already in effect.");
                        }
                    }
                    else if (g_cmd[6] == '-')
                    {
                        check_command_length(eoff, 7);
                        if (formatting)
                        {
                            if (in_para)
                            {
                                *g_src.curr++ = '\n';    // finish off current paragraph
                            }
                            in_para = false;
                            formatting = false;
                            num_spaces = 0;
                            state = STATES::S_Start;
                        }
                        else
                        {
                            warn(eoff, "\"Format-\" is already in effect.");
                        }
                    }
                    else
                    {
                        error(eoff, "Invalid argument to Format.");
                    }
                }
                else if (strnicmp(g_cmd, "Online", 6) == 0)
                {
                    if (g_cmd[6] == '+')
                    {
                        check_command_length(eoff, 7);

                        if (g_xonline)
                        {
                            *g_src.curr++ = CMD_XONLINE;
                            g_xonline = false;
                        }
                        else
                        {
                            warn(eoff, "\"Online+\" already in effect.");
                        }
                    }
                    else if (g_cmd[6] == '-')
                    {
                        check_command_length(eoff, 7);
                        if (!g_xonline)
                        {
                            *g_src.curr++ = CMD_XONLINE;
                            g_xonline = true;
                        }
                        else
                        {
                            warn(eoff, "\"Online-\" already in effect.");
                        }
                    }
                    else
                    {
                        error(eoff, "Invalid argument to Online.");
                    }
                }
                else if (strnicmp(g_cmd, "Doc", 3) == 0)
                {
                    if (g_cmd[3] == '+')
                    {
                        check_command_length(eoff, 4);
                        if (g_xdoc)
                        {
                            *g_src.curr++ = CMD_XDOC;
                            g_xdoc = false;
                        }
                        else
                        {
                            warn(eoff, "\"Doc+\" already in effect.");
                        }
                    }
                    else if (g_cmd[3] == '-')
                    {
                        check_command_length(eoff, 4);
                        if (!g_xdoc)
                        {
                            *g_src.curr++ = CMD_XDOC;
                            g_xdoc = true;
                        }
                        else
                        {
                            warn(eoff, "\"Doc-\" already in effect.");
                        }
                    }
                    else
                    {
                        error(eoff, "Invalid argument to Doc.");
                    }
                }
                else if (strnicmp(g_cmd, "Center", 6) == 0)
                {
                    if (g_cmd[6] == '+')
                    {
                        check_command_length(eoff, 7);
                        if (!centering)
                        {
                            centering = true;
                            if (in_para)
                            {
                                *g_src.curr++ = '\n';
                                in_para = false;
                            }
                            state = STATES::S_Start;  // for centering FSM
                        }
                        else
                        {
                            warn(eoff, "\"Center+\" already in effect.");
                        }
                    }
                    else if (g_cmd[6] == '-')
                    {
                        check_command_length(eoff, 7);
                        if (centering)
                        {
                            centering = false;
                            state = STATES::S_Start;  // for centering FSM
                        }
                        else
                        {
                            warn(eoff, "\"Center-\" already in effect.");
                        }
                    }
                    else
                    {
                        error(eoff, "Invalid argument to Center.");
                    }
                }
                else if (strnicmp(g_cmd, "CompressSpaces", 14) == 0)
                {
                    check_command_length(eoff, 15);

                    if (g_cmd[14] == '+')
                    {
                        if (g_compress_spaces)
                        {
                            warn(eoff, "\"CompressSpaces+\" is already in effect.");
                        }
                        else
                        {
                            g_compress_spaces = true;
                        }
                    }
                    else if (g_cmd[14] == '-')
                    {
                        if (!g_compress_spaces)
                        {
                            warn(eoff, "\"CompressSpaces-\" is already in effect.");
                        }
                        else
                        {
                            g_compress_spaces = false;
                        }
                    }
                    else
                    {
                        error(eoff, "Invalid argument to CompressSpaces.");
                    }
                }
                else if (strnicmp("BinInc ", g_cmd, 7) == 0)
                {
                    if (!(t.flags & TF_DATA))
                    {
                        error(eoff, "BinInc allowed only in Data topics.");
                    }
                    else
                    {
                        process_bininc();
                    }
                }
                else
                {
                    error(eoff, "Bad or unexpected command \"%s\".", g_cmd);
                }
                // else

            } // while (!done)

            continue;
        }

        if (!in_topic)
        {
            g_cmd[0] = ch;
            ptr = read_until(&g_cmd[1], 127, "\n~");
            if (*ptr == '~')
            {
                unread_char('~');
            }
            *ptr = '\0';
            error(0, "Text outside of any topic \"%s\".", g_cmd);
            continue;
        }

        if (centering)
        {
            bool again;
            do
            {
                again = false;   // default

                if (state == STATES::S_Start)
                {
                    if (ch == ' ')
                    {
                        ; // do nothing
                    }
                    else if ((ch&0xFF) == '\n')
                    {
                        *g_src.curr++ = ch;    // no need to center blank lines.
                    }
                    else
                    {
                        *g_src.curr++ = CMD_CENTER;
                        state = STATES::S_Line;
                        again = true;
                    }
                }
                else if (state == STATES::S_Line)
                {
                    put_a_char(ch, t);
                    if ((ch&0xFF) == '\n')
                    {
                        state = STATES::S_Start;
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
                case STATES::S_Start:
                    if ((ch&0xFF) == '\n')
                    {
                        *g_src.curr++ = ch;
                    }
                    else
                    {
                        state = STATES::S_StartFirstLine;
                        num_spaces = 0;
                        again = true;
                    }
                    break;

                case STATES::S_StartFirstLine:
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else
                    {
                        if (lformat_exclude > 0 && num_spaces >= lformat_exclude)
                        {
                            put_spaces(num_spaces);
                            num_spaces = 0;
                            state = STATES::S_FormatDisabled;
                            again = true;
                        }
                        else
                        {
                            *g_src.curr++ = CMD_PARA;
                            *g_src.curr++ = (char)num_spaces;
                            *g_src.curr++ = (char)num_spaces;
                            margin_pos = g_src.curr - 1;
                            state = STATES::S_FirstLine;
                            again = true;
                            in_para = true;
                        }
                    }
                    break;

                case STATES::S_FirstLine:
                    if (ch == '\n')
                    {
                        state = STATES::S_StartSecondLine;
                        num_spaces = 0;
                    }
                    else if (ch == ('\n'|0x100))    // force end of para ?
                    {
                        *g_src.curr++ = '\n';
                        in_para = false;
                        state = STATES::S_Start;
                    }
                    else if (ch == ' ')
                    {
                        state = STATES::S_FirstLineSpaces;
                        num_spaces = 1;
                    }
                    else
                    {
                        put_a_char(ch, t);
                    }
                    break;

                case STATES::S_FirstLineSpaces:
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else
                    {
                        put_spaces(num_spaces);
                        state = STATES::S_FirstLine;
                        again = true;
                    }
                    break;

                case STATES::S_StartSecondLine:
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else if ((ch&0xFF) == '\n') // a blank line means end of a para
                    {
                        *g_src.curr++ = '\n';   // end the para
                        *g_src.curr++ = '\n';   // for the blank line
                        in_para = false;
                        state = STATES::S_Start;
                    }
                    else
                    {
                        if (lformat_exclude > 0 && num_spaces >= lformat_exclude)
                        {
                            *g_src.curr++ = '\n';
                            in_para = false;
                            put_spaces(num_spaces);
                            num_spaces = 0;
                            state = STATES::S_FormatDisabled;
                            again = true;
                        }
                        else
                        {
                            add_blank_for_split();
                            margin = num_spaces;
                            *margin_pos = (char)num_spaces;
                            state = STATES::S_Line;
                            again = true;
                        }
                    }
                    break;

                case STATES::S_Line:   // all lines after the first
                    if (ch == '\n')
                    {
                        state = STATES::S_StartLine;
                        num_spaces = 0;
                    }
                    else if (ch == ('\n' | 0x100))    // force end of para ?
                    {
                        *g_src.curr++ = '\n';
                        in_para = false;
                        state = STATES::S_Start;
                    }
                    else if (ch == ' ')
                    {
                        state = STATES::S_LineSpaces;
                        num_spaces = 1;
                    }
                    else
                    {
                        put_a_char(ch, t);
                    }
                    break;

                case STATES::S_LineSpaces:
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else
                    {
                        put_spaces(num_spaces);
                        state = STATES::S_Line;
                        again = true;
                    }
                    break;

                case STATES::S_StartLine:   // for all lines after the second
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else if ((ch&0xFF) == '\n') // a blank line means end of a para
                    {
                        *g_src.curr++ = '\n';   // end the para
                        *g_src.curr++ = '\n';   // for the blank line
                        in_para = false;
                        state = STATES::S_Start;
                    }
                    else
                    {
                        if (num_spaces != margin)
                        {
                            *g_src.curr++ = '\n';
                            in_para = false;
                            state = STATES::S_StartFirstLine;  // with current num_spaces
                        }
                        else
                        {
                            add_blank_for_split();
                            state = STATES::S_Line;
                        }
                        again = true;
                    }
                    break;

                case STATES::S_FormatDisabled:
                    if (ch == ' ')
                    {
                        state = STATES::S_FormatDisabledSpaces;
                        num_spaces = 1;
                    }
                    else
                    {
                        if ((ch&0xFF) == '\n')
                        {
                            state = STATES::S_Start;
                        }
                        put_a_char(ch, t);
                    }
                    break;

                case STATES::S_FormatDisabledSpaces:
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else
                    {
                        put_spaces(num_spaces);
                        num_spaces = 0;    // is this needed?
                        state = STATES::S_FormatDisabled;
                        again = true;
                    }
                    break;

                case STATES::S_Spaces:
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

                if (state == STATES::S_Start)
                {
                    if (ch == ' ')
                    {
                        state = STATES::S_Spaces;
                        num_spaces = 1;
                    }
                    else
                    {
                        put_a_char(ch, t);
                    }
                }
                else if (state == STATES::S_Spaces)
                {
                    if (ch == ' ')
                    {
                        ++num_spaces;
                    }
                    else
                    {
                        put_spaces(num_spaces);
                        num_spaces = 0;     // is this needed?
                        state = STATES::S_Start;
                        again = true;
                    }
                }
            }
            while (again);
        }

        check_buffer(0);
    } // while ( 1 )

    std::fclose(g_src_file);

    g_src_line = -1;
}

} // namespace hc
