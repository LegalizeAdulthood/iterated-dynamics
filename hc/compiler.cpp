/*
 * Stand-alone help compiler.
 *
 * See help-compiler.txt for source file syntax.
 *
 */
#include "compiler.h"

#include "help_source.h"
#include "html_processor.h"
#include "messages.h"

#include <port.h>
#include <id_io.h>
#include <helpcom.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

#if !defined(O_BINARY)
#define O_BINARY 0
#endif

#define MAXFILE _MAX_FNAME
#define MAXEXT  _MAX_EXT


#ifdef XFRACT

#ifndef HAVESTRI
extern int stricmp(char const *, char const *);
extern int strnicmp(char const *, char const *, int);
#endif
extern int filelength(int);

#else

#if defined(_WIN32)
// disable unsafe CRT warnings
#pragma warning(disable: 4996)
#endif

#endif

namespace hc
{

char const *const DEFAULT_SRC_FNAME = "help.src";
char const *const DEFAULT_HLP_FNAME = "id.hlp";
char const *const DEFAULT_EXE_FNAME = "id.exe";
char const *const DEFAULT_DOC_FNAME = "id.txt";
std::string const DEFAULT_HTML_FNAME = "index.rst";

char const *const TEMP_FNAME = "hc.tmp";
char const *const SWAP_FNAME = "hcswap.tmp";

char const *const INDEX_LABEL       = "HELP_INDEX";

int const BUFFER_SIZE = (1024*1024);    // 1 MB

struct help_sig_info
{
    unsigned long sig;
    int           version;
    unsigned long base;
};

struct Include
{
    std::string fname;
    std::FILE *file;
    int   line;
    int   col;
};

std::vector<TOPIC> g_topics;         //
std::vector<LABEL> g_labels;         //
std::vector<LABEL> g_private_labels; //
std::vector<LINK> g_all_links;       //
std::vector<CONTENT> g_contents;     // the table-of-contents
int g_max_pages{};                   // max. pages in any topic
int g_max_links{};                   // max. links on any page
int g_num_doc_pages{};               // total number of pages in document
std::FILE *g_src_file{};             // .SRC file
int g_src_line{};                    // .SRC line number (used for errors)
int g_src_col{};                     // .SRC column.
int g_version{-1};                   // help file version
std::string g_src_filename;          // command-line .SRC filename
std::string g_hdr_filename;          // .H filename
std::string g_hlp_filename;          // .HLP filename
std::string g_current_src_filename;  // current .SRC filename
int g_format_exclude{};              // disable formatting at this col, 0 to never disable formatting
std::FILE *g_swap_file{};            //
long g_swap_pos{};                   //
std::vector<char> g_buffer;          // alloc'ed as BUFFER_SIZE bytes
char *g_curr{};                      // current position in the buffer
char g_cmd[128]{};                   // holds the current command
bool g_compress_spaces{};            //
bool g_xonline{};                    //
bool g_xdoc{};                       //
std::vector<Include> g_include_stack;     //
std::vector<std::string> g_include_paths; //

void check_buffer(char const *curr, unsigned off, char const *buffer);

inline void check_buffer(unsigned off)
{
    check_buffer(g_curr, off, &g_buffer[0]);
}

std::ostream &operator<<(std::ostream &str, const CONTENT &content)
{
    str << "Flags: " << std::hex << content.flags << std::dec << '\n'
        << "Id: <" << content.id << ">\n"
        << "Name: <" << content.name << ">\n"
        << "Doc Page: " << content.doc_page << '\n'
        << "Page Num Pos: " << content.page_num_pos << '\n'
        << "Num Topic: " << content.num_topic << '\n';
    for (int i = 0; i < content.num_topic; ++i)
    {
        str << "    Label? " << std::boolalpha << content.is_label[i] << '\n'
            << "    Name: <" << content.topic_name[i] << ">\n"
            << "    Topic Num: " << content.topic_num[i] << '\n';
    }
    return str << "Source File: <" << content.srcfile << ">\n"
        << "Source Line: " << content.srcline << '\n';
}

std::ostream &operator<<(std::ostream &str, const PAGE &page)
{
    return str << "Offset: " << page.offset << ", Length: " << page.length << ", Margin: " << page.margin;
}

std::ostream &operator<<(std::ostream &str, const TOPIC &topic)
{
    str << "Flags: " << std::hex << topic.flags << std::dec << '\n'
        << "Doc Page: " << topic.doc_page << '\n'
        << "Title Len: " << topic.title_len << '\n'
        << "Title: <" << topic.title << ">\n"
        << "Num Page: " << topic.num_page << '\n';
    for (const PAGE &page : topic.page)
    {
        str << "    " << page << '\n';
    }
    str << "Text Len: " << topic.text_len << '\n'
        << "Text: " << topic.text << '\n'
        << "Offset: " << topic.offset << '\n'
        << "Tokens:\n";

    char const *text = topic.get_topic_text();
    char const *curr = text;
    unsigned int len = topic.text_len;

    while (len > 0)
    {
        int size = 0;
        int width = 0;
        token_types const tok = find_token_length(token_modes::ONLINE, curr, len, &size, &width);

        switch (tok)
        {
        case token_types::TOK_DONE:
            str << "  done\n";
            break;

        case token_types::TOK_SPACE:
            str << std::string(width, ' ');
            break;

        case token_types::TOK_LINK:
            str << "  link\n";
            break;

        case token_types::TOK_PARA:
            str << "  para\n";
            break;

        case token_types::TOK_NL:
            str << '\n';
            break;

        case token_types::TOK_FF:
            str << "  ff\n";
            break;

        case token_types::TOK_WORD:
            str << std::string(curr, width);
            break;

        case token_types::TOK_XONLINE:
            str << "  xonline\n";
            break;

        case token_types::TOK_XDOC:
            str << "  xdoc\n";
            break;

        case token_types::TOK_CENTER:
            str << "  center\n";
            break;
        }
        len -= size;
        curr += size;
    }

    return str;
}

/*
 * store-topic-text-to-disk stuff.
 */


void release_topic_text(const TOPIC &t, bool save)
{
    if (save)
    {
        std::fseek(g_swap_file, t.text, SEEK_SET);
        std::fwrite(&g_buffer[0], 1, t.text_len, g_swap_file);
    }
}


/*
 * memory-allocation functions.
 */


int add_link(LINK &l)
{
    g_all_links.push_back(l);
    return static_cast<int>(g_all_links.size() - 1);
}


int add_topic(const TOPIC &t)
{
    g_topics.push_back(t);
    return static_cast<int>(g_topics.size() - 1);
}


int add_label(const LABEL &l)
{
    if (l.name[0] == '@')    // if it's a private label...
    {
        g_private_labels.push_back(l);
        return static_cast<int>(g_private_labels.size() - 1);
    }

    g_labels.push_back(l);
    return static_cast<int>(g_labels.size() - 1);
}


int add_content(const CONTENT &c)
{
    g_contents.push_back(c);
    return static_cast<int>(g_contents.size() - 1);
}


/*
 * read_char() stuff
 */


int const READ_CHAR_BUFF_SIZE = 32;


int  read_char_buff[READ_CHAR_BUFF_SIZE];
int  read_char_buff_pos = -1;
int  read_char_sp       = 0;


/*
 * Will not handle new-lines or tabs correctly!
 */
void unread_char(int ch)
{
    if (read_char_buff_pos+1 >= READ_CHAR_BUFF_SIZE)
    {
        throw std::runtime_error("Compiler Error -- Read char buffer overflow!");
    }

    read_char_buff[++read_char_buff_pos] = ch;

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

    if (read_char_buff_pos >= 0)
    {
        ++g_src_col;
        return read_char_buff[read_char_buff_pos--];
    }

    if (read_char_sp > 0)
    {
        --read_char_sp;
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
            read_char_sp += diff;
            break;
        }

        case ' ':
            ++g_src_col;
            ++read_char_sp;
            break;

        case '\n':
            read_char_sp = 0;   // delete spaces before a \n
            g_src_col = 0;
            ++g_src_line;
            return '\n';

        case -1:               // EOF
            if (read_char_sp > 0)
            {
                --read_char_sp;
                return ' ';
            }
            return -1;

        default:
            if (read_char_sp > 0)
            {
                ungetc(ch, g_src_file);
                --read_char_sp;
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
            int  ctr = 0;

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
 * misc. search functions.
 */


LABEL *find_label(char const *name)
{
    if (*name == '@')
    {
        for (LABEL &pl : g_private_labels)
        {
            if (name == pl.name)
            {
                return &pl;
            }
        }
    }
    else
    {
        for (LABEL &l : g_labels)
        {
            if (name == l.name)
            {
                return &l;
            }
        }
    }

    return nullptr;
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

    for (int t = 0; t < static_cast<int>(g_topics.size()); t++)
    {
        if ((int) g_topics[t].title.length() == len
            && strnicmp(title, g_topics[t].title.c_str(), len) == 0)
        {
            return t;
        }
    }

    return -1;   // not found
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

        *g_curr++ = CMD_SPACE;
        *g_curr++ = (BYTE)how_many;
    }
    else
    {
        while (how_many-- > 0)
        {
            *g_curr++ = ' ';
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

std::string rst_name(std::string const &content_name)
{
    std::string name;
    name.reserve(content_name.length());
    bool underscore = true;     // can't start with an underscore
    for (unsigned char c : content_name)
    {
        if (std::isalnum(c) != 0)
        {
            name += static_cast<char>(std::tolower(c));
            underscore = false;
        }
        else if (!underscore)
        {
            name += '_';
            underscore = true;
        }
    }
    auto pos = name.find_last_not_of('_');
    if (pos != std::string::npos)
    {
        name.erase(pos + 1);
    }
    return name;
}

void process_doc_contents(hc::modes mode)
{
    TOPIC t;
    t.flags     = 0;
    t.title_len = (unsigned) std::strlen(DOCCONTENTS_TITLE)+1;
    t.title     = DOCCONTENTS_TITLE;
    t.doc_page  = -1;
    t.num_page  = 0;

    g_curr = &g_buffer[0];

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
    add_content(c);

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
            int const indent = std::atoi(g_cmd);

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
            if (mode == hc::modes::HTML)
            {
                std::sprintf(g_curr, "%s", rst_name(c.name).c_str());
                char *ptr = g_curr + (int) std::strlen(g_curr);
                c.page_num_pos = 0U;
                g_curr = ptr;
            }
            else
            {
                std::sprintf(g_curr, "%-5s %*.0s%s", c.id.c_str(), indent*2, "", c.name.c_str());
                char *ptr = g_curr + (int) std::strlen(g_curr);
                while ((ptr-g_curr) < PAGE_WIDTH-10)
                {
                    *ptr++ = '.';
                }
                c.page_num_pos = (unsigned)((ptr-3) - &g_buffer[0]);
                g_curr = ptr;
            }

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

            add_content(c);
        }
        else if (ch == '~')   // end at any command
        {
            unread_char(ch);
            break;
        }
        else
        {
            *g_curr++ = ch;
        }

        check_buffer(0);
    }

    t.alloc_topic_text((unsigned)(g_curr - &g_buffer[0]));
    add_topic(t);
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
        int const lnum = add_link(l);
        *g_curr++ = CMD_LINK;
        setint(g_curr, lnum);
        g_curr += 3*sizeof(int);
        std::memcpy(g_curr, ptr, len);
        g_curr += len;
        *g_curr++ = CMD_LINK;
        return len;
    }

    return 0;
}


int const MAX_TABLE_SIZE = 100;


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

    first_link = static_cast<int>(g_all_links.size());
    table_start = g_curr;
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
            g_curr = table_start;   // reset to the start...
            title.push_back(std::string(g_curr+3*sizeof(int)+1, len+1));
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

            if (first_link+lnum >= static_cast<int>(g_all_links.size()))
            {
                break;
            }

            len = static_cast<int>(title[lnum].length());
            *g_curr++ = CMD_LINK;
            setint(g_curr, first_link+lnum);
            g_curr += 3*sizeof(int);
            std::memcpy(g_curr, title[lnum].c_str(), len);
            g_curr += len;
            *g_curr++ = CMD_LINK;

            if (c < cols-1)
            {
                put_spaces(width-len);
            }
        }
        *g_curr++ = '\n';
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

    if (read(handle, g_curr, (unsigned)len) != len)
    {
        throw std::system_error(errno, std::system_category(), "process_bininc failed read");
    }

    g_curr += (unsigned)len;

    close(handle);
}


void start_topic(TOPIC &t, char const *title, int title_len)
{
    t.flags = 0;
    t.title_len = title_len;
    t.title.assign(title, title_len);
    t.doc_page = -1;
    t.num_page = 0;
    g_curr = &g_buffer[0];
}


void end_topic(TOPIC &t)
{
    t.alloc_topic_text((unsigned)(g_curr - &g_buffer[0]));
    add_topic(t);
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


void add_blank_for_split()   // add space at g_curr for merging two lines
{
    if (!is_hyphen(g_curr-1))     // no spaces if it's a hyphen
    {
        if (end_of_sentence(g_curr-1))
        {
            *g_curr++ = ' ';    // two spaces at end of a sentence
        }
        *g_curr++ = ' ';
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
            *g_curr++ = CMD_LITERAL;
        }
        *g_curr++ = ch;
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

void read_src(std::string const &fname, hc::modes mode)
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

    g_curr = &g_buffer[0];

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
            if (g_topics.empty())
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

                    start_topic(t, topic_title, static_cast<int>(title_len));
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

                    if (find_label(data) != nullptr)
                    {
                        error(eoff, "Label \"%s\" already exists", data);
                        continue;
                    }

                    if (g_cmd[5] == '@')
                    {
                        warn(eoff, "Data topic has a local label.");
                    }

                    start_topic(t, "", 0);
                    t.flags |= TF_DATA;

                    if ((int)std::strlen(data) > 32)
                    {
                        warn(eoff, "Label name is long.");
                    }

                    lbl.name      = data;
                    lbl.topic_num = static_cast<int>(g_topics.size());
                    lbl.topic_off = 0;
                    lbl.doc_page  = -1;
                    add_label(lbl);

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
                        if (!g_hdr_filename.empty())
                        {
                            warn(eoff, "Header Filename has already been defined.");
                        }
                        g_hdr_filename = &g_cmd[8];
                    }
                    else if (strnicmp(g_cmd, "HlpFile=", 8) == 0)
                    {
                        if (!g_hlp_filename.empty())
                        {
                            warn(eoff, "Help Filename has already been defined.");
                        }
                        g_hlp_filename = &g_cmd[8];
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
                        *g_curr++ = '\n';    // finish off current paragraph
                    }
                    *g_curr++ = CMD_FF;
                    state = STATES::S_Start;
                    in_para = false;
                    num_spaces = 0;
                }
                else if (strnicmp(g_cmd, "DocFF", 5) == 0)
                {
                    check_command_length(eoff, 5);
                    if (in_para)
                    {
                        *g_curr++ = '\n';    // finish off current paragraph
                    }
                    if (!g_xonline)
                    {
                        *g_curr++ = CMD_XONLINE;
                    }
                    *g_curr++ = CMD_FF;
                    if (!g_xonline)
                    {
                        *g_curr++ = CMD_XONLINE;
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
                        *g_curr++ = '\n';    // finish off current paragraph
                    }
                    if (!g_xdoc)
                    {
                        *g_curr++ = CMD_XDOC;
                    }
                    *g_curr++ = CMD_FF;
                    if (!g_xdoc)
                    {
                        *g_curr++ = CMD_XDOC;
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
                    else if (find_label(label_name) != nullptr)
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
                        lbl.topic_num = static_cast<int>(g_topics.size());
                        lbl.topic_off = (unsigned)(g_curr - &g_buffer[0]);
                        lbl.doc_page  = -1;
                        add_label(lbl);
                    }
                }
                else if (strnicmp(g_cmd, "Table=", 6) == 0)
                {
                    if (in_para)
                    {
                        *g_curr++ = '\n';  // finish off current paragraph
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
                                *g_curr++ = '\n';    // finish off current paragraph
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
                            *g_curr++ = CMD_XONLINE;
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
                            *g_curr++ = CMD_XONLINE;
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
                            *g_curr++ = CMD_XDOC;
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
                            *g_curr++ = CMD_XDOC;
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
                                *g_curr++ = '\n';
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
                        *g_curr++ = ch;    // no need to center blank lines.
                    }
                    else
                    {
                        *g_curr++ = CMD_CENTER;
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
                        *g_curr++ = ch;
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
                            *g_curr++ = CMD_PARA;
                            *g_curr++ = (char)num_spaces;
                            *g_curr++ = (char)num_spaces;
                            margin_pos = g_curr - 1;
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
                        *g_curr++ = '\n';
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
                        *g_curr++ = '\n';   // end the para
                        *g_curr++ = '\n';   // for the blank line
                        in_para = false;
                        state = STATES::S_Start;
                    }
                    else
                    {
                        if (lformat_exclude > 0 && num_spaces >= lformat_exclude)
                        {
                            *g_curr++ = '\n';
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
                        *g_curr++ = '\n';
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
                        *g_curr++ = '\n';   // end the para
                        *g_curr++ = '\n';   // for the blank line
                        in_para = false;
                        state = STATES::S_Start;
                    }
                    else
                    {
                        if (num_spaces != margin)
                        {
                            *g_curr++ = '\n';
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


/*
 * stuff to resolve hot-link references.
 */
void link_topic(LINK &l)
{
    int const t = find_topic_title(l.name.c_str());
    if (t == -1)
    {
        g_current_src_filename = l.srcfile;
        g_src_line = l.srcline; // pretend we are still in the source...
        error(0, "Cannot find implicit hot-link \"%s\".", l.name.c_str());
        g_src_line = -1;  // back to reality
    }
    else
    {
        l.topic_num = t;
        l.topic_off = 0;
        l.doc_page = (g_topics[t].flags & TF_IN_DOC) ? 0 : -1;
    }
}

void link_label(LINK &l)
{
    if (LABEL *lbl = find_label(l.name.c_str()))
    {
        if (g_topics[lbl->topic_num].flags & TF_DATA)
        {
            g_current_src_filename = l.srcfile;
            g_src_line = l.srcline;
            error(0, "Label \"%s\" is a data-only topic.", l.name.c_str());
            g_src_line = -1;
        }
        else
        {
            l.topic_num = lbl->topic_num;
            l.topic_off = lbl->topic_off;
            l.doc_page  = (g_topics[lbl->topic_num].flags & TF_IN_DOC) ? 0 : -1;
        }
    }
    else
    {
        g_current_src_filename = l.srcfile;
        g_src_line = l.srcline; // pretend again
        error(0, "Cannot find explicit hot-link \"%s\".", l.name.c_str());
        g_src_line = -1;
    }
}

void label_topic(CONTENT &c, int ctr)
{
    if (LABEL *lbl = find_label(c.topic_name[ctr].c_str()))
    {
        if (g_topics[lbl->topic_num].flags & TF_DATA)
        {
            g_current_src_filename = c.srcfile;
            g_src_line = c.srcline;
            error(0, "Label \"%s\" is a data-only topic.", c.topic_name[ctr].c_str());
            g_src_line = -1;
        }
        else
        {
            c.topic_num[ctr] = lbl->topic_num;
            if (g_topics[lbl->topic_num].flags & TF_IN_DOC)
            {
                warn(0, "Topic \"%s\" appears in document more than once.",
                    g_topics[lbl->topic_num].title.c_str());
            }
            else
            {
                g_topics[lbl->topic_num].flags |= TF_IN_DOC;
            }
        }
    }
    else
    {
        g_current_src_filename = c.srcfile;
        g_src_line = c.srcline;
        error(0, "Cannot find DocContent label \"%s\".", c.topic_name[ctr].c_str());
        g_src_line = -1;
    }
}

void content_topic(CONTENT &c, int ctr)
{
    int const t = find_topic_title(c.topic_name[ctr].c_str());
    if (t == -1)
    {
        g_current_src_filename = c.srcfile;
        g_src_line = c.srcline;
        error(0, "Cannot find DocContent topic \"%s\".", c.topic_name[ctr].c_str());
        g_src_line = -1;  // back to reality
    }
    else
    {
        c.topic_num[ctr] = t;
        if (g_topics[t].flags & TF_IN_DOC)
        {
            warn(0, "Topic \"%s\" appears in document more than once.",
                g_topics[t].title.c_str());
        }
        else
        {
            g_topics[t].flags |= TF_IN_DOC;
        }
    }
}

/*
 * calculate topic_num/topic_off for each link.
 */
void make_hot_links()
{
    msg("Making hot-links.");

    /*
     * Calculate topic_num for all entries in DocContents.  Also set
     * "TF_IN_DOC" flag for all topics included in the document.
     */
    for (CONTENT &c : g_contents)
    {
        for (int ctr = 0; ctr < c.num_topic; ctr++)
        {
            if (c.is_label[ctr])
            {
                label_topic(c, ctr);
            }
            else
            {
                content_topic(c, ctr);
            }
        }
    }

    /*
     * Find topic_num and topic_off for all hot-links.  Also flag all hot-
     * links which will (probably) appear in the document.
     */
    for (LINK &l : g_all_links)
    {
        // name is the title of the topic
        if (l.type == link_types::LT_TOPIC)
        {
            link_topic(l);
        }
        // name is the name of a label
        else if (l.type == link_types::LT_LABEL)
        {
            link_label(l);
        }
        // it's a "special" link; topic_off already has the value
        else if (l.type == link_types::LT_SPECIAL)
        {
        }
    }
}


/*
 * online help pagination stuff
 */


void paginate_online()    // paginate the text for on-line help
{
    int size;
    int width;

    msg("Paginating online help.");

    for (TOPIC &t : g_topics)
    {
        if (t.flags & TF_DATA)
        {
            continue;    // don't paginate data topics
        }

        const char *text = t.get_topic_text();
        const char *curr = text;
        unsigned int len = t.text_len;

        const char *start = curr;
        bool skip_blanks = false;
        int lnum = 0;
        int num_links = 0;
        int col = 0;
        int start_margin = -1;

        while (len > 0)
        {
            token_types tok = find_token_length(token_modes::ONLINE, curr, len, &size, &width);

            switch (tok)
            {
            case token_types::TOK_PARA:
            {
                ++curr;
                int const indent = *curr++;
                int const margin = *curr++;
                len -= 3;
                col = indent;
                while (true)
                {
                    tok = find_token_length(token_modes::ONLINE, curr, len, &size, &width);

                    if (tok == token_types::TOK_DONE || tok == token_types::TOK_NL || tok == token_types::TOK_FF)
                    {
                        break;
                    }

                    if (tok == token_types::TOK_PARA)
                    {
                        col = 0;   // fake a nl
                        ++lnum;
                        break;
                    }

                    if (tok == token_types::TOK_XONLINE || tok == token_types::TOK_XDOC)
                    {
                        curr += size;
                        len -= size;
                        continue;
                    }

                    // now tok is SPACE or LINK or WORD
                    if (col+width > SCREEN_WIDTH)
                    {
                        // go to next line...
                        if (++lnum >= SCREEN_DEPTH)
                        {
                            // go to next page...
                            t.add_page_break(start_margin, text, start, curr, num_links);
                            start = curr + ((tok == token_types::TOK_SPACE) ? size : 0);
                            start_margin = margin;
                            lnum = 0;
                            num_links = 0;
                        }
                        if (tok == token_types::TOK_SPACE)
                        {
                            width = 0;    // skip spaces at start of a line
                        }

                        col = margin;
                    }

                    col += width;
                    curr += size;
                    len -= size;
                }

                skip_blanks = false;
                size = 0;
                width = size;
                break;
            }

            case token_types::TOK_NL:
                if (skip_blanks && col == 0)
                {
                    start += size;
                    break;
                }
                ++lnum;
                if (lnum >= SCREEN_DEPTH || (col == 0 && lnum == SCREEN_DEPTH-1))
                {
                    t.add_page_break(start_margin, text, start, curr, num_links);
                    start = curr + size;
                    start_margin = -1;
                    lnum = 0;
                    num_links = 0;
                    skip_blanks = true;
                }
                col = 0;
                break;

            case token_types::TOK_FF:
                col = 0;
                if (skip_blanks)
                {
                    start += size;
                    break;
                }
                t.add_page_break(start_margin, text, start, curr, num_links);
                start_margin = -1;
                start = curr + size;
                lnum = 0;
                num_links = 0;
                break;

            case token_types::TOK_DONE:
            case token_types::TOK_XONLINE:   // skip
            case token_types::TOK_XDOC:      // ignore
            case token_types::TOK_CENTER:    // ignore
                break;

            case token_types::TOK_LINK:
                ++num_links;
                // fall-through

            default:    // SPACE, LINK, WORD
                skip_blanks = false;
                break;
            } // switch

            curr += size;
            len  -= size;
            col  += width;
        } // while

        if (!skip_blanks)
        {
            t.add_page_break(start_margin, text, start, curr, num_links);
        }

        if (g_max_pages < t.num_page)
        {
            g_max_pages = t.num_page;
        }

        release_topic_text(t, false);
    } // for
}


/*
 * paginate document stuff
 */


struct DOC_INFO
{
    int content_num;
    int topic_num;
    bool link_dest_warn;
};

struct PAGINATE_DOC_INFO : DOC_INFO
{
    char const *start;
    CONTENT  *c;
    LABEL    *lbl;
};


LABEL *find_next_label_by_topic(int t)
{
    LABEL *g = nullptr;
    for (LABEL &l : g_labels)
    {
        if (l.topic_num == t && l.doc_page == -1)
        {
            g = &l;
            break;
        }
        if (l.topic_num > t)
        {
            break;
        }
    }

    LABEL *p = nullptr;
    for (LABEL &pl : g_private_labels)
    {
        if (pl.topic_num == t && pl.doc_page == -1)
        {
            p = &pl;
            break;
        }
        if (pl.topic_num > t)
        {
            break;
        }
    }

    if (p == nullptr)
    {
        return g;
    }

    if (g == nullptr)
    {
        return p;
    }
    return (g->topic_off < p->topic_off) ? g : p;
}


/*
 * Find doc_page for all hot-links.
 */
void set_hot_link_doc_page()
{
    LABEL *lbl;
    int    t;

    for (LINK &l : g_all_links)
    {
        switch (l.type)
        {
        case link_types::LT_TOPIC:
            t = find_topic_title(l.name.c_str());
            if (t == -1)
            {
                g_current_src_filename = l.srcfile;
                g_src_line = l.srcline; // pretend we are still in the source...
                error(0, "Cannot find implicit hot-link \"%s\".", l.name.c_str());
                g_src_line = -1;  // back to reality
            }
            else
            {
                l.doc_page = g_topics[t].doc_page;
            }
            break;

        case link_types::LT_LABEL:
            lbl = find_label(l.name.c_str());
            if (lbl == nullptr)
            {
                g_current_src_filename = l.srcfile;
                g_src_line = l.srcline; // pretend again
                error(0, "Cannot find explicit hot-link \"%s\".", l.name.c_str());
                g_src_line = -1;
            }
            else
            {
                l.doc_page = lbl->doc_page;
            }
            break;

        case link_types::LT_SPECIAL:
            // special topics don't appear in the document
            break;
        }
    }
}


/*
 * insert page #'s in the DocContents
 */
void set_content_doc_page()
{
    char     buf[4];
    int      len;

    int tnum = find_topic_title(DOCCONTENTS_TITLE);
    assert(tnum >= 0);
    TOPIC &t = g_topics[tnum];

    char *base = t.get_topic_text();

    for (const CONTENT &c : g_contents)
    {
        assert(c.doc_page >= 1);
        std::sprintf(buf, "%d", c.doc_page);
        len = (int) std::strlen(buf);
        assert(len <= 3);
        std::memcpy(base + c.page_num_pos + (3 - len), buf, len);
    }

    release_topic_text(t, true);
}


// this function also used by print_document()
bool pd_get_info(int cmd, PD_INFO *pd, void *context)
{
    DOC_INFO &info = *static_cast<DOC_INFO *>(context);
    const CONTENT *c;

    switch (cmd)
    {
    case PD_GET_CONTENT:
        if (++info.content_num >= static_cast<int>(g_contents.size()))
        {
            return false;
        }
        c = &g_contents[info.content_num];
        info.topic_num = -1;
        pd->id       = c->id.c_str();
        pd->title    = c->name.c_str();
        pd->new_page = (c->flags & CF_NEW_PAGE) != 0;
        return true;

    case PD_GET_TOPIC:
        c = &g_contents[info.content_num];
        if (++info.topic_num >= c->num_topic)
        {
            return false;
        }
        pd->curr = g_topics[c->topic_num[info.topic_num]].get_topic_text();
        pd->len = g_topics[c->topic_num[info.topic_num]].text_len;
        return true;

    case PD_GET_LINK_PAGE:
    {
        const LINK &link = g_all_links[getint(pd->s)];
        if (link.doc_page == -1)
        {
            if (info.link_dest_warn)
            {
                g_current_src_filename = link.srcfile;
                g_src_line    = link.srcline;
                warn(0, "Hot-link destination is not in the document.");
                g_src_line = -1;
            }
            return false;
        }
        pd->i = g_all_links[getint(pd->s)].doc_page;
        return true;
    }

    case PD_RELEASE_TOPIC:
        c = &g_contents[info.content_num];
        release_topic_text(g_topics[c->topic_num[info.topic_num]], false);
        return true;

    default:
        return false;
    }
}


bool paginate_doc_output(int cmd, PD_INFO *pd, void *context)
{
    PAGINATE_DOC_INFO *info = static_cast<PAGINATE_DOC_INFO *>(context);
    switch (cmd)
    {
    case PD_FOOTING:
    case PD_PRINT:
    case PD_PRINTN:
    case PD_PRINT_SEC:
        return true;

    case PD_HEADING:
        ++g_num_doc_pages;
        return true;

    case PD_START_SECTION:
        info->c = &g_contents[info->content_num];
        return true;

    case PD_START_TOPIC:
        info->start = pd->curr;
        info->lbl = find_next_label_by_topic(info->c->topic_num[info->topic_num]);
        return true;

    case PD_SET_SECTION_PAGE:
        info->c->doc_page = pd->page_num;
        return true;

    case PD_SET_TOPIC_PAGE:
        g_topics[info->c->topic_num[info->topic_num]].doc_page = pd->page_num;
        return true;

    case PD_PERIODIC:
        while (info->lbl != nullptr && (unsigned)(pd->curr - info->start) >= info->lbl->topic_off)
        {
            info->lbl->doc_page = pd->page_num;
            info->lbl = find_next_label_by_topic(info->c->topic_num[info->topic_num]);
        }
        return true;

    default:
        return false;
    }
}


void paginate_document()
{
    PAGINATE_DOC_INFO info;

    if (g_contents.empty())
    {
        return;
    }

    msg("Paginating document.");

    info.topic_num = -1;
    info.content_num = info.topic_num;
    info.link_dest_warn = true;

    process_document(pd_get_info, paginate_doc_output, &info);

    set_hot_link_doc_page();
    set_content_doc_page();
}


/*
 * label sorting stuff
 */

bool cmp_LABEL(const LABEL &a, const LABEL &b)
{
    if (a.name == INDEX_LABEL)
        return true;
    
    if (b.name == INDEX_LABEL)
        return false;

    return a.name < b.name;
}


void sort_labels()
{
    std::sort(g_labels.begin(), g_labels.end(), cmp_LABEL);
    std::sort(g_private_labels.begin(), g_private_labels.end(), cmp_LABEL);
}


/*
 * file write stuff.
 */


// returns true if different
bool compare_files(std::FILE *f1, std::FILE *f2)
{
    if (filelength(fileno(f1)) != filelength(fileno(f2)))
    {
        return true;    // different if sizes are not the same
    }

    while (!std::feof(f1) && !std::feof(f2))
    {
        if (getc(f1) != getc(f2))
        {
            return true;
        }
    }

    return !(std::feof(f1) && std::feof(f2));
}


void _write_hdr(char const *fname, std::FILE *file)
{
    std::fprintf(file, "#if !defined(HELP_DEFS_H)\n"
        "#define HELP_DEFS_H\n"
        "\n/*\n * %s\n", fs::path{fname}.filename().string().c_str());
    std::fprintf(file, " *\n * Contains #defines for help.\n *\n");
    std::fprintf(file, " * Generated by HC from: %s\n *\n */\n\n\n", fs::path{g_src_filename}.filename().string().c_str());

    std::fprintf(file, "/* current help file version */\n");
    std::fprintf(file, "\n");
    std::fprintf(file, "#define %-32s %3d\n", "IDHELP_VERSION", g_version);
    std::fprintf(file, "\n\n");

    std::fprintf(file, "/* labels */\n"
        "\n"
        "enum class help_labels\n"
        "{\n"
        "    SPECIAL_IFS                      =  -4,\n"
        "    SPECIAL_L_SYSTEM                 =  -3,\n"
        "    SPECIAL_FORMULA                  =  -2,\n"
        "    NONE                             =  -1,\n");

    for (int ctr = 0; ctr < static_cast<int>(g_labels.size()); ctr++)
    {
        if (g_labels[ctr].name[0] != '@')  // if it's not a local label...
        {
            std::fprintf(file, "    %-32s = %3d%s", g_labels[ctr].name.c_str(), ctr, ctr != static_cast<int>(g_labels.size())-1 ? "," : "");
            if (g_labels[ctr].name == INDEX_LABEL)
            {
                std::fprintf(file, "        /* index */");
            }
            std::fprintf(file, "\n");
        }
    }
    std::fprintf(file, "};\n"
        "\n"
        "\n"
        "#endif\n");
}


void write_hdr(char const *fname)
{
    std::FILE *temp;
    std::FILE *hdr;

    hdr = std::fopen(fname, "rt");

    if (hdr == nullptr)
    {
        // if no prev. hdr file generate a new one
        hdr = std::fopen(fname, "wt");
        if (hdr == nullptr)
        {
            throw std::runtime_error("Cannot create \"" + std::string{fname} + "\".");
        }
        msg("Writing: %s", fname);
        _write_hdr(fname, hdr);
        std::fclose(hdr);
        notice("Id must be re-compiled.");
        return ;
    }

    msg("Comparing: %s", fname);

    temp = std::fopen(TEMP_FNAME, "wt");

    if (temp == nullptr)
    {
        throw std::runtime_error("Cannot create temporary file: \"" + std::string{TEMP_FNAME} + "\".");
    }

    _write_hdr(fname, temp);

    std::fclose(temp);
    temp = std::fopen(TEMP_FNAME, "rt");

    if (temp == nullptr)
    {
        throw std::runtime_error("Cannot open temporary file: \"" + std::string{TEMP_FNAME} + "\".");
    }

    if (compare_files(temp, hdr))     // if they are different...
    {
        msg("Updating: %s", fname);
        std::fclose(temp);
        std::fclose(hdr);
        std::remove(fname);               // delete the old hdr file
        std::rename(TEMP_FNAME, fname);   // rename the temp to the hdr file
        notice("Id must be re-compiled.");
    }
    else
    {
        // if they are the same leave the original alone.
        std::fclose(temp);
        std::fclose(hdr);
        std::remove(TEMP_FNAME);      // delete the temp
    }
}


void calc_offsets()    // calc file offset to each topic
{
    // NOTE: offsets do NOT include 6 bytes for signature & version!
    long offset = static_cast<long>(sizeof(int) +                       // max_pages
                                    sizeof(int) +                       // max_links
                                    sizeof(int) +                       // num_topic
                                    sizeof(int) +                       // num_label
                                    sizeof(int) +                       // num_contents
                                    sizeof(int) +                       // num_doc_pages
                                    g_topics.size() * sizeof(long) +    // offsets to each topic
                                    g_labels.size() * 2 * sizeof(int)); // topic_num/topic_off for all public labels

    offset = std::accumulate(g_contents.begin(), g_contents.end(), offset, [](long offset, const CONTENT &cp) {
        return offset += sizeof(int) +  // flags
            1 +                         // id length
            (int) cp.id.length() +      // id text
            1 +                         // name length
            (int) cp.name.length() +    // name text
            1 +                         // number of topics
            cp.num_topic*sizeof(int);   // topic numbers
    });

    for (TOPIC &tp : g_topics)
    {
        tp.offset = offset;
        offset += (long)sizeof(int) +       // topic flags
            sizeof(int) +                   // number of pages
            tp.num_page*3*sizeof(int) +     // page offset, length & starting margin
            1 +                             // length of title
            tp.title_len +                  // title
            sizeof(int) +                   // length of text
            tp.text_len;                    // text
    }
}


/*
 * Replaces link indexes in the help text with topic_num, topic_off and
 * doc_page info.
 */
void insert_real_link_info(char *curr, unsigned int len)
{
    while (len > 0)
    {
        int size = 0;
        token_types tok = find_token_length(token_modes::NONE, curr, len, &size, nullptr);

        if (tok == token_types::TOK_LINK)
        {
            const LINK &l = g_all_links[ getint(curr+1) ];
            setint(curr+1, l.topic_num);
            setint(curr+1+sizeof(int), l.topic_off);
            setint(curr+1+2*sizeof(int), l.doc_page);
        }

        len -= size;
        curr += size;
    }
}


void _write_help(std::FILE *file)
{
    char                 *text;
    help_sig_info  hs;

    // write the signature and version

    hs.sig = HELP_SIG; // Edit line 17 of helpcom.h if this is a syntax error
    hs.version = g_version;

    std::fwrite(&hs, sizeof(long)+sizeof(int), 1, file);

    // write max_pages & max_links

    putw(g_max_pages, file);
    putw(g_max_links, file);

    // write num_topic, num_label and num_contents

    putw(static_cast<int>(g_topics.size()), file);
    putw(static_cast<int>(g_labels.size()), file);
    putw(static_cast<int>(g_contents.size()), file);

    // write num_doc_page

    putw(g_num_doc_pages, file);

    // write the offsets to each topic
    for (const TOPIC &t : g_topics)
    {
        std::fwrite(&t.offset, sizeof(long), 1, file);
    }

    // write all public labels
    for (const LABEL &l : g_labels)
    {
        putw(l.topic_num, file);
        putw(l.topic_off, file);
    }

    // write contents
    for (const CONTENT &cp : g_contents)
    {
        putw(cp.flags, file);

        int t = (int) cp.id.length();
        putc((BYTE)t, file);
        std::fwrite(cp.id.c_str(), 1, t, file);

        t = (int) cp.name.length();
        putc((BYTE)t, file);
        std::fwrite(cp.name.c_str(), 1, t, file);

        putc((BYTE)cp.num_topic, file);
        std::fwrite(cp.topic_num, sizeof(int), cp.num_topic, file);
    }

    // write topics
    for (TOPIC &tp : g_topics)
    {
        // write the topics flags
        putw(tp.flags, file);

        // write offset, length and starting margin for each page

        putw(tp.num_page, file);
        for (const PAGE &p : tp.page)
        {
            putw(p.offset, file);
            putw(p.length, file);
            putw(p.margin, file);
        }

        // write the help title

        putc((BYTE)tp.title_len, file);
        std::fwrite(tp.title.c_str(), 1, tp.title_len, file);

        // insert hot-link info & write the help text

        text = tp.get_topic_text();

        if (!(tp.flags & TF_DATA))     // don't process data topics...
        {
            insert_real_link_info(text, tp.text_len);
        }

        putw(tp.text_len, file);
        std::fwrite(text, 1, tp.text_len, file);

        release_topic_text(tp, false);  // don't save the text even though
        // insert_real_link_info() modified it
        // because we don't access the info after
        // this.
    }
}


void write_help(char const *fname)
{
    std::FILE *hlp;

    hlp = std::fopen(fname, "wb");

    if (hlp == nullptr)
    {
        throw std::runtime_error("Cannot create .HLP file: \"" + std::string{fname} + "\".");
    }

    msg("Writing: %s", fname);

    _write_help(hlp);

    std::fclose(hlp);
}


/*
 * print document stuff.
 */


struct PRINT_DOC_INFO : public DOC_INFO
{
    std::FILE    *file;
    int      margin;
    bool     start_of_line;
    int      spaces;
};

void printerc(PRINT_DOC_INFO *info, int c, int n)
{
    while (n-- > 0)
    {
        if (c == ' ')
        {
            ++info->spaces;
        }
        else if (c == '\n' || c == '\f')
        {
            info->start_of_line = true;
            info->spaces = 0;   // strip spaces before a new-line
            putc(c, info->file);
        }
        else
        {
            if (info->start_of_line)
            {
                info->spaces += info->margin;
                info->start_of_line = false;
            }

            while (info->spaces > 0)
            {
                std::fputc(' ', info->file);
                --info->spaces;
            }

            std::fputc(c, info->file);
        }
    }
}


void printers(PRINT_DOC_INFO *info, char const *s, int n)
{
    if (n > 0)
    {
        while (n-- > 0)
        {
            printerc(info, *s++, 1);
        }
    }
    else
    {
        while (*s != '\0')
        {
            printerc(info, *s++, 1);
        }
    }
}


bool print_doc_output(int cmd, PD_INFO *pd, void *context)
{
    PRINT_DOC_INFO *info = static_cast<PRINT_DOC_INFO *>(context);
    switch (cmd)
    {
    case PD_HEADING:
    {
        std::ostringstream buff;
        info->margin = 0;
        // TODO: replace this fixed string with ID_PROGRAM_NAME and ID_VERSION
        buff << "\n"
            "                  Iterated Dynamics Version 1.0                 Page "
            << pd->page_num << "\n\n";
        printers(info, buff.str().c_str(), 0);
        info->margin = PAGE_INDENT;
        return true;
    }

    case PD_FOOTING:
        info->margin = 0;
        printerc(info, '\f', 1);
        info->margin = PAGE_INDENT;
        return true;

    case PD_PRINT:
        printers(info, pd->s, pd->i);
        return true;

    case PD_PRINTN:
        printerc(info, *pd->s, pd->i);
        return true;

    case PD_PRINT_SEC:
        info->margin = TITLE_INDENT;
        if (pd->id[0] != '\0')
        {
            printers(info, pd->id, 0);
            printerc(info, ' ', 1);
        }
        printers(info, pd->title, 0);
        printerc(info, '\n', 1);
        info->margin = PAGE_INDENT;
        return true;

    case PD_START_SECTION:
    case PD_START_TOPIC:
    case PD_SET_SECTION_PAGE:
    case PD_SET_TOPIC_PAGE:
    case PD_PERIODIC:
        return true;

    default:
        return false;
    }
}


void print_document(char const *fname)
{
    PRINT_DOC_INFO info;

    if (g_contents.empty())
    {
        throw std::runtime_error(".SRC has no DocContents.");
    }

    msg("Printing to: %s", fname);

    info.topic_num = -1;
    info.content_num = info.topic_num;
    info.link_dest_warn = false;

    info.file = std::fopen(fname, "wt");
    if (info.file == nullptr)
    {
        throw std::runtime_error("Couldn't create \"" + std::string{fname} + "\"");
    }

    info.margin = PAGE_INDENT;
    info.start_of_line = true;
    info.spaces = 0;

    process_document(pd_get_info, print_doc_output, &info);

    std::fclose(info.file);
}


/*
 * compiler status and memory usage report stuff.
 */


void report_memory()
{
    long bytes_in_strings = 0;
    long // bytes in strings
        text = 0;
    long // bytes in topic text (stored on disk)
        data = 0;
    long          // bytes in active data structure
        dead = 0; // bytes in unused data structure

    for (const TOPIC &t : g_topics)
    {
        data   += sizeof(TOPIC);
        bytes_in_strings += t.title_len;
        text   += t.text_len;
        data   += t.num_page * sizeof(PAGE);

        std::vector<PAGE> const &pages = t.page;
        dead += static_cast<long>((pages.capacity() - pages.size()) * sizeof(PAGE));
    }

    for (const LINK &l : g_all_links)
    {
        data += sizeof(LINK);
        bytes_in_strings += (long) l.name.length();
    }

    dead += static_cast<long>((g_all_links.capacity() - g_all_links.size()) * sizeof(LINK));

    for (const LABEL &l : g_labels)
    {
        data   += sizeof(LABEL);
        bytes_in_strings += (long) l.name.length() + 1;
    }

    dead += static_cast<long>((g_labels.capacity() - g_labels.size()) * sizeof(LABEL));

    for (const LABEL &l : g_private_labels)
    {
        data   += sizeof(LABEL);
        bytes_in_strings += (long) l.name.length() + 1;
    }

    dead += static_cast<long>((g_private_labels.capacity() - g_private_labels.size()) * sizeof(LABEL));

    for (const CONTENT &c : g_contents)
    {
        int t = (MAX_CONTENT_TOPIC - c.num_topic) *
            (sizeof(g_contents[0].is_label[0]) + sizeof(g_contents[0].topic_name[0]) + sizeof(g_contents[0].topic_num[0]));
        data += sizeof(CONTENT) - t;
        dead += t;
        bytes_in_strings += (long) c.id.length() + 1;
        bytes_in_strings += (long) c.name.length() + 1;
        for (int ctr2 = 0; ctr2 < c.num_topic; ctr2++)
        {
            bytes_in_strings += (long) c.topic_name[ctr2].length() + 1;
        }
    }

    dead += static_cast<long>((g_contents.capacity() - g_contents.size()) * sizeof(CONTENT));

    std::printf("\n");
    std::printf("Memory Usage:\n");
    std::printf("%8ld Bytes in buffers.\n", (long)BUFFER_SIZE);
    std::printf("%8ld Bytes in strings.\n", bytes_in_strings);
    std::printf("%8ld Bytes in data.\n", data);
    std::printf("%8ld Bytes in dead space.\n", dead);
    std::printf("--------\n");
    std::printf("%8ld Bytes total.\n", (long)BUFFER_SIZE+bytes_in_strings+data+dead);
    std::printf("\n");
    std::printf("Disk Usage:\n");
    std::printf("%8ld Bytes in topic text.\n", text);
}


void report_stats()
{
    int  pages = 0;
    for (const TOPIC &t : g_topics)
    {
        pages += t.num_page;
    }

    std::printf("\n");
    std::printf("Statistics:\n");
    std::printf("%8d Topics\n", static_cast<int>(g_topics.size()));
    std::printf("%8d Links\n", static_cast<int>(g_all_links.size()));
    std::printf("%8d Labels\n", static_cast<int>(g_labels.size()));
    std::printf("%8d Private labels\n", static_cast<int>(g_private_labels.size()));
    std::printf("%8d Table of contents (DocContent) entries\n", static_cast<int>(g_contents.size()));
    std::printf("%8d Online help pages\n", pages);
    std::printf("%8d Document pages\n", g_num_doc_pages);
}


/*
 * add/delete help from .EXE functions.
 */


void add_hlp_to_exe(char const *hlp_fname, char const *exe_fname)
{
    int exe;
    int // handles
        hlp;
    long                 len;
    int                  size;
    help_sig_info hs;

    exe = open(exe_fname, O_RDWR|O_BINARY);
    if (exe == -1)
    {
        throw std::runtime_error("Unable to open \"" + std::string{exe_fname} + "\"");
    }

    hlp = open(hlp_fname, O_RDONLY|O_BINARY);
    if (hlp == -1)
    {
        throw std::runtime_error("Unable to open \"" + std::string{hlp_fname} + "\"");
    }

    msg("Appending %s to %s", hlp_fname, exe_fname);

    // first, check and see if any help is currently installed

    lseek(exe, filelength(exe) - sizeof(help_sig_info), SEEK_SET);

    if (read(exe, (char *)&hs, 10) != 10)
    {
        close(hlp);
        close(exe);
        throw std::system_error(errno, std::system_category(), "add_hlp_to_exe failed read");
    }

    if (hs.sig == HELP_SIG)
    {
        warn(0, "Overwriting previous help. (Version=%d)", hs.version);
    }
    else
    {
        hs.base = filelength(exe);
    }

    // now, let's see if their help file is for real (and get the version)

    auto const sig_len = sizeof(long) + sizeof(int);
    if (read(hlp, (char *)&hs, sig_len) != sig_len)
    {
        close(hlp);
        close(exe);
        throw std::system_error(errno, std::system_category(), "add_hlp_to_exe failed read2");
    }

    if (hs.sig != HELP_SIG)
    {
        throw std::runtime_error("Help signature not found in " + std::string{hlp_fname});
    }

    msg("Help file %s Version=%d", hlp_fname, hs.version);

    // append the help stuff, overwriting old help (if any)

    lseek(exe, hs.base, SEEK_SET);

    len = filelength(hlp) - sizeof(long) - sizeof(int); // adjust for the file signature & version

    for (int count = 0; count < len;)
    {
        size = (int) std::min((long)BUFFER_SIZE, len-count);
        if (read(hlp, &g_buffer[0], size) != size)
        {
            throw std::system_error(errno, std::system_category(), "add_hlp_to_exe failed read3");
        }
        if (write(exe, &g_buffer[0], size) != size)
        {
            throw std::system_error(errno, std::system_category(), "add_hlp_to_exe failed write");
        }
        count += size;
    }

    // add on the signature, version and offset

    if (write(exe, (char *)&hs, 10) != 10)
    {
        close(hlp);
        close(exe);
        throw std::system_error(errno, std::system_category(), "add_hlp_to_exe failed write2");
    }

    off_t offset = lseek(exe, 0L, SEEK_CUR);
    if (chsize(exe, offset) != offset) // truncate if old help was longer
    {
        close(hlp);
        close(exe);
        throw std::system_error(errno, std::system_category(), "add_hlp_to_exe failed chsize");
    }

    close(exe);
    close(hlp);
}


void delete_hlp_from_exe(char const *exe_fname)
{
    int   exe;   // file handle
    help_sig_info hs;

    exe = open(exe_fname, O_RDWR|O_BINARY);
    if (exe == -1)
    {
        throw std::runtime_error("Unable to open \"" + std::string{exe_fname} + "\"");
    }

    msg("Deleting help from %s", exe_fname);

    // see if any help is currently installed

#ifndef XFRACT
    lseek(exe, filelength(exe) - 10, SEEK_SET);
    read(exe, (char *)&hs, 10);
#else
    lseek(exe, filelength(exe) - 12, SEEK_SET);
    if (read(exe, (char *)&hs, 12) != 12)
    {
        throw std::system_error(errno, std::system_category(), "add_hlp_to_exe failed read4");
    }
#endif

    if (hs.sig == HELP_SIG)
    {
        if (chsize(exe, hs.base) != hs.base) // truncate at the start of the help
        {
            throw std::system_error(errno, std::system_category(), "add_hlp_to_exe failed chsize2");
        }
        close(exe);
    }
    else
    {
        close(exe);
        throw std::runtime_error("No help found in " + std::string{exe_fname});
    }
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

compiler::compiler(const compiler_options &options) :
    m_options(options)
{
    g_quiet_mode = m_options.quiet_mode;
    g_include_paths = m_options.include_paths;
}

compiler::~compiler()
{
    if (g_swap_file != nullptr)
    {
        std::fclose(g_swap_file);
        std::remove(m_options.swappath.c_str());
    }
}

int compiler::process()
{
    std::printf("HC - " ID_PROGRAM_NAME " Help Compiler.\n\n");

    g_buffer.resize(BUFFER_SIZE);

    switch (m_options.mode)
    {
    case modes::NONE:
        usage();
        break;

    case modes::COMPILE:
        compile();
        break;

    case modes::PRINT:
        print();
        break;

    case modes::APPEND:
        add_hlp_to_exe(m_options.fname1.empty() ? DEFAULT_HLP_FNAME : m_options.fname1.c_str(),
                       m_options.fname2.empty() ? DEFAULT_EXE_FNAME : m_options.fname2.c_str());
        break;

    case modes::DELETE:
        if (!m_options.fname2.empty())
        {
            throw std::runtime_error("Unexpected argument \"" + m_options.fname2 + "\"");
        }
        delete_hlp_from_exe(m_options.fname1.empty() ? DEFAULT_EXE_FNAME : m_options.fname1.c_str());
        break;

    case modes::HTML:
        render_html();
        break;
    }

    return g_errors;     // return the number of errors
}

void compiler::usage()
{
    std::printf("To compile a .SRC file:\n"
                "      HC /c [/s] [/m] [/r[path]] [src_file]\n"
                "         /s       = report statistics.\n"
                "         /m       = report memory usage.\n"
                "         /r[path] = set swap file path.\n"
                "         src_file = .SRC file.  Default is \"%s\"\n",
        DEFAULT_SRC_FNAME);
    std::printf("To print a .SRC file:\n"
                "      HC /p [/r[path]] [src_file] [out_file]\n"
                "         /r[path] = set swap file path.\n"
                "         src_file = .SRC file.  Default is \"%s\"\n",
        DEFAULT_SRC_FNAME);
    std::printf("         out_file = Filename to print to. Default is \"%s\"\n",
        DEFAULT_DOC_FNAME);
    std::printf("To append a .HLP file to an .EXE file:\n"
                "      HC /a [hlp_file] [exe_file]\n"
                "         hlp_file = .HLP file.  Default is \"%s\"\n",
        DEFAULT_HLP_FNAME);
    std::printf("         exe_file = .EXE file.  Default is \"%s\"\n", DEFAULT_EXE_FNAME);
    std::printf("To delete help info from an .EXE file:\n"
                "      HC /d [exe_file]\n"
                "         exe_file = .EXE file.  Default is \"%s\"\n",
        DEFAULT_EXE_FNAME);
    std::printf("\n"
                "Use \"/q\" for quiet mode. (No status messages.)\n");
}

void compiler::read_source_file()
{
    g_src_filename = m_options.fname1.empty() ? DEFAULT_SRC_FNAME : m_options.fname1;

    m_options.swappath += SWAP_FNAME;

    g_swap_file = std::fopen(m_options.swappath.c_str(), "w+b");
    if (g_swap_file == nullptr)
    {
        throw std::runtime_error("Cannot create swap file \"" + m_options.swappath + "\"");
    }
    g_swap_pos = 0;

    read_src(g_src_filename, m_options.mode);
}

void compiler::compile()
{
    if (!m_options.fname2.empty())
    {
        throw std::runtime_error("Unexpected command-line argument \"" + m_options.fname2 + "\"");
    }

    read_source_file();

    if (g_hdr_filename.empty())
    {
        error(0, "No .H file defined.  (Use \"~HdrFile=\")");
    }
    if (g_hlp_filename.empty())
    {
        error(0, "No .HLP file defined.  (Use \"~HlpFile=\")");
    }
    if (g_version == -1)
    {
        warn(0, "No help version has been defined.  (Use \"~Version=\")");
    }

    // order of these is very important...

    make_hot_links();  // do even if errors since it may report more...

    if (!g_errors)
    {
        paginate_online();
    }
    if (!g_errors)
    {
        paginate_document();
    }
    if (!g_errors)
    {
        calc_offsets();
    }
    if (!g_errors)
    {
        sort_labels();
    }
    if (!g_errors)
    {
        write_hdr(g_hdr_filename.c_str());
    }
    if (!g_errors)
    {
        write_help(g_hlp_filename.c_str());
    }

    if (m_options.show_stats)
    {
        report_stats();
    }

    if (m_options.show_mem)
    {
        report_memory();
    }

    if (g_errors || g_warnings)
    {
        report_errors();
    }
}

void compiler::print()
{
    read_source_file();
    make_hot_links();

    if (!g_errors)
    {
        paginate_document();
    }
    if (!g_errors)
    {
        print_document(m_options.fname2.empty() ? DEFAULT_DOC_FNAME : m_options.fname2.c_str());
    }

    if (g_errors || g_warnings)
    {
        report_errors();
    }
}

void compiler::render_html()
{
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
        sort_labels();
    }
    if (g_errors == 0)
    {
        print_html_document(m_options.fname2.empty() ? DEFAULT_HTML_FNAME : m_options.fname2);
    }
    if (g_errors > 0 || g_warnings > 0)
    {
        report_errors();
    }
}

void compiler::paginate_html_document()
{
    if (g_contents.empty())
    {
        return;
    }

    int size;
    int width;

    msg("Paginating HTML.");

    for (TOPIC &t : g_topics)
    {
        if (t.flags & TF_DATA)
        {
            continue;    // don't paginate data topics
        }

        const char *text = t.get_topic_text();
        const char *curr = text;
        unsigned int len = t.text_len;

        const char *start = curr;
        bool skip_blanks = false;
        int lnum = 0;
        int num_links = 0;
        int col = 0;

        while (len > 0)
        {
            token_types tok = find_token_length(token_modes::ONLINE, curr, len, &size, &width);

            switch (tok)
            {
            case token_types::TOK_PARA:
            {
                ++curr;
                int const indent = *curr++;
                int const margin = *curr++;
                len -= 3;
                col = indent;
                while (true)
                {
                    tok = find_token_length(token_modes::ONLINE, curr, len, &size, &width);

                    if (tok == token_types::TOK_DONE || tok == token_types::TOK_NL || tok == token_types::TOK_FF)
                    {
                        break;
                    }

                    if (tok == token_types::TOK_PARA)
                    {
                        col = 0;   // fake a nl
                        ++lnum;
                        break;
                    }

                    if (tok == token_types::TOK_XONLINE || tok == token_types::TOK_XDOC)
                    {
                        curr += size;
                        len -= size;
                        continue;
                    }

                    // now tok is SPACE or LINK or WORD
                    if (col+width > SCREEN_WIDTH)
                    {
                        // go to next line...
                        if (tok == token_types::TOK_SPACE)
                        {
                            width = 0;    // skip spaces at start of a line
                        }

                        col = margin;
                    }

                    col += width;
                    curr += size;
                    len -= size;
                }

                skip_blanks = false;
                size = 0;
                width = size;
                break;
            }

            case token_types::TOK_NL:
                if (skip_blanks && col == 0)
                {
                    start += size;
                    break;
                }
                ++lnum;
                col = 0;
                break;

            case token_types::TOK_FF:
                col = 0;
                if (skip_blanks)
                {
                    start += size;
                    break;
                }
                start = curr + size;
                num_links = 0;
                break;

            case token_types::TOK_DONE:
            case token_types::TOK_XONLINE:   // skip
            case token_types::TOK_XDOC:      // ignore
            case token_types::TOK_CENTER:    // ignore
                break;

            case token_types::TOK_LINK:
                ++num_links;

                // fall-through

            default:    // SPACE, LINK, WORD
                skip_blanks = false;
                break;

            } // switch

            curr += size;
            len  -= size;
            col  += width;
        } // while

        if (g_max_pages < t.num_page)
        {
            g_max_pages = t.num_page;
        }

        release_topic_text(t, false);
    } // for
}

void compiler::print_html_document(std::string const &fname)
{
    html_processor(fname).process();
}

} // namespace hc
