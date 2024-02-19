/*
 * hc.c
 *
 * Stand-alone FRACTINT help compiler.  Compile in the COMPACT memory model.
 *
 * See help-compiler.txt for source file syntax.
 *
 */
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <fcntl.h>

#include "id_io.h"
#include "port.h"
#include "helpcom.h"

#define MAXFILE _MAX_FNAME
#define MAXEXT  _MAX_EXT
#define FNSPLIT _splitpath

 /*
 * When defined, SHOW_ERROR_LINE will cause the line number in HC.C where
 * errors/warnings/messages are generated to be displayed at the start of
 * the line.
 *
 * Used when debugging HC.  Also useful for finding the line (in HC.C) that
 * generated a error or warning.
 */
#define SHOW_ERROR_LINE

#ifdef XFRACT

#ifndef HAVESTRI
extern int stricmp(char const *, char const *);
extern int strnicmp(char const *, char const *, int);
#endif
extern int filelength(int);
extern int _splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext);

#else

#if defined(_WIN32)
// disable unsafe CRT warnings
#pragma warning(disable: 4996)
#endif

#endif

namespace
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

char const *const DEFAULT_SRC_FNAME = "help.src";
char const *const DEFAULT_HLP_FNAME = "fractint.hlp";
char const *const DEFAULT_EXE_FNAME = "fractint.exe";
char const *const DEFAULT_DOC_FNAME = "fractint.doc";
std::string const DEFAULT_HTML_FNAME = "index.rst";

char const *const TEMP_FNAME = "hc.tmp";
char const *const SWAP_FNAME = "hcswap.tmp";

int const MAX_ERRORS = (25);            // stop after this many errors
int const MAX_WARNINGS = (25);          // stop after this many warnings
                                        // 0 = never stop

char const *const INDEX_LABEL       = "IDHELP_INDEX";
char const *const DOCCONTENTS_TITLE = "DocContent";

int const BUFFER_SIZE = (1024*1024);    // 1 MB

enum class link_types
{
    LT_TOPIC,
    LT_LABEL,
    LT_SPECIAL
};

struct LINK
{
    link_types type;        // 0 = name is topic title, 1 = name is label,
                            //   2 = "special topic"; name is nullptr and
                            //   topic_num/topic_off is valid
    int      topic_num;     // topic number to link to
    unsigned topic_off;     // offset into topic to link to
    int      doc_page;      // document page # to link to
    std::string name;       // name of label or title of topic to link to
    std::string srcfile;    // .SRC file link appears in
    int      srcline;       // .SRC file line # link appears in
};


struct PAGE
{
    unsigned offset;    // offset from start of topic text
    unsigned length;    // length of page (in chars)
    int      margin;    // if > 0 then page starts in_para and text
                        // should be indented by this much
};


// values for TOPIC.flags
enum
{
    TF_IN_DOC = 1,          // set if topic is part of the printed document
    TF_DATA = 2             // set if it is a "data" topic
};

struct TOPIC
{
    unsigned  flags;          // see #defines for TF_???
    int       doc_page;       // page number in document where topic starts
    unsigned  title_len;      // length of title
    std::string title;        // title for this topic
    int       num_page;       // number of pages
    std::vector<PAGE> page;   // list of pages
    unsigned  text_len;       // length of topic text
    long      text;           // topic text (all pages)
    long      offset;         // offset from start of file to topic
};


struct LABEL
{
    std::string name;         // its name
    int      topic_num;       // topic number
    unsigned topic_off;       // offset of label in the topic's text
    int      doc_page;
};


// values for CONTENT.flags
enum
{
    CF_NEW_PAGE = 1         // true if section starts on a new page
};

int const MAX_CONTENT_TOPIC = 10;


struct CONTENT
{
    unsigned  flags;
    std::string id;
    std::string name;
    int       doc_page;
    unsigned  page_num_pos;
    int       num_topic;
    bool      is_label[MAX_CONTENT_TOPIC];
    std::string topic_name[MAX_CONTENT_TOPIC];
    int       topic_num[MAX_CONTENT_TOPIC];
    std::string srcfile;
    int       srcline;
};


struct help_sig_info
{
    unsigned long sig;
    int           version;
    unsigned long base;
};


std::vector<TOPIC> g_topics;
std::vector<LABEL> g_labels;
std::vector<LABEL> g_private_labels;
std::vector<LINK> g_all_links;
std::vector<CONTENT> g_contents;    // the table-of-contents

bool quiet_mode = false;          // true if "/Q" option used

int      max_pages        = 0;    // max. pages in any topic
int      max_links        = 0;    // max. links on any page
int      num_doc_pages    = 0;    // total number of pages in document

std::FILE    *srcfile;                 // .SRC file
int      srcline          = 0;    // .SRC line number (used for errors)
int      srccol           = 0;    // .SRC column.

int      version          = -1;   // help file version

int      errors           = 0,    // number of errors reported
         warnings         = 0;    // number of warnings reported

std::string src_fname;            // command-line .SRC filename
std::string hdr_fname;            // .H filename
std::string hlp_fname;            // .HLP filename
std::string src_cfname;           // current .SRC filename

int      format_exclude   = 0;    // disable formatting at this col, 0 to
//    never disable formatting
std::FILE    *swapfile;
long     swappos;

std::vector<char> buffer;            // alloc'ed as BUFFER_SIZE bytes
char             *g_curr;            // current position in the buffer
char              cmd[128];          // holds the current command
bool              compress_spaces{}; //
bool              xonline{};         //
bool              xdoc{};            //

int const MAX_INCLUDE_STACK = 5;    // allow 5 nested includes

struct include_stack_entry
{
    std::string fname;
    std::FILE *file;
    int   line;
    int   col;
};
include_stack_entry include_stack[MAX_INCLUDE_STACK];
int include_stack_top = -1;

std::vector<std::string> g_include_paths;

std::string g_html_output_dir = ".";

char *get_topic_text(TOPIC const *t);

void check_buffer(char const *curr, unsigned off, char const *buffer);

inline void check_buffer(unsigned off)
{
    check_buffer(g_curr, off, &buffer[0]);
}

std::ostream &operator<<(std::ostream &str, CONTENT const &content)
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

std::ostream &operator<<(std::ostream &str, PAGE const &page)
{
    return str << "Offset: " << page.offset << ", Length: " << page.length << ", Margin: " << page.margin;
}

std::ostream &operator<<(std::ostream &str, TOPIC const &topic)
{
    str << "Flags: " << std::hex << topic.flags << std::dec << '\n'
        << "Doc Page: " << topic.doc_page << '\n'
        << "Title Len: " << topic.title_len << '\n'
        << "Title: <" << topic.title << ">\n"
        << "Num Page: " << topic.num_page << '\n';
    for (PAGE const &page : topic.page)
    {
        str << "    " << page << '\n';
    }
    str << "Text Len: " << topic.text_len << '\n'
        << "Text: " << topic.text << '\n'
        << "Offset: " << topic.offset << '\n'
        << "Tokens:\n";

    char const *text = get_topic_text(&topic);
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
 * error/warning/message reporting functions.
 */


void report_errors()
{
    std::printf("\n");
    std::printf("Compiler Status:\n");
    std::printf("%8d Error%c\n",       errors, (errors == 1)   ? ' ' : 's');
    std::printf("%8d Warning%c\n",     warnings, (warnings == 1) ? ' ' : 's');
}


void print_msg(char const *type, int lnum, char const *format, std::va_list arg)
{
    if (type != nullptr)
    {
        std::printf("   %s", type);
        if (lnum > 0)
        {
            std::printf(" %s %d", src_cfname.c_str(), lnum);
        }
        std::printf(": ");
    }
    vprintf(format, arg);
    std::printf("\n");
    std::fflush(stdout);
}

void fatal_msg(int diff, char const *format, ...)
{
    std::va_list arg;
    va_start(arg, format);

    print_msg("Fatal", srcline-diff, format, arg);
    va_end(arg);

    if (errors || warnings)
    {
        report_errors();
    }

    exit(errors + 1);
}


void error_msg(int diff, char const *format, ...)
{
    std::va_list arg;
    va_start(arg, format);

    print_msg("Error", srcline-diff, format, arg);
    va_end(arg);

    if (++errors >= MAX_ERRORS)
    {
        fatal_msg(0, "Too many errors!");
    }
}


void warn_msg(int diff, char const *format, ...)
{
    std::va_list arg;
    va_start(arg, format);

    print_msg("Warning", srcline-diff, format, arg);
    va_end(arg);

    if (++warnings >= MAX_WARNINGS)
    {
        fatal_msg(0, "Too many warnings!");
    }
}


void notice_msg(char const *format, ...)
{
    std::va_list arg;
    va_start(arg, format);
    print_msg("Note", srcline, format, arg);
    va_end(arg);
}


void msg_msg(char const *format, ...)
{
    std::va_list arg;

    if (quiet_mode)
    {
        return;
    }
    va_start(arg, format);
    print_msg(nullptr, 0, format, arg);
    va_end(arg);
}

void show_line(unsigned int line)
{
    std::printf("[%04d] ", line);
}

#ifdef SHOW_ERROR_LINE
#   define fatal(...)  (show_line(__LINE__), fatal_msg(__VA_ARGS__))
#   define error(...)  (show_line(__LINE__), error_msg(__VA_ARGS__))
#   define warn(...)   (show_line(__LINE__), warn_msg(__VA_ARGS__))
#   define notice(...) (show_line(__LINE__), notice_msg(__VA_ARGS__))
#   define msg(...)    (quiet_mode ? static_cast<void>(0) : (show_line(__LINE__), msg_msg(__VA_ARGS__)))
#else
#define fatal(...)  fatal_msg(__VA_ARGS__)
#define error(...)  error_msg(__VA_ARGS__)
#define warn(...)   warn_msg(__VA_ARGS__)
#define notice(...) notice_msg(__VA_ARGS__)
#define msg(...)    msg_msg(__VA_ARGS__)
#endif


/*
 * store-topic-text-to-disk stuff.
 */


void alloc_topic_text(TOPIC *t, unsigned size)
{
    t->text_len = size;
    t->text = swappos;
    swappos += size;
    fseek(swapfile, t->text, SEEK_SET);
    fwrite(&buffer[0], 1, t->text_len, swapfile);
}


char *get_topic_text(TOPIC const *t)
{
    fseek(swapfile, t->text, SEEK_SET);
    if (fread(&buffer[0], 1, t->text_len, swapfile) != t->text_len)
    {
        throw std::system_error(errno, std::system_category(), "get_topic_text failed fread");
    }
    return &buffer[0];
}


void release_topic_text(TOPIC const *t, int save)
{
    if (save)
    {
        fseek(swapfile, t->text, SEEK_SET);
        fwrite(&buffer[0], 1, t->text_len, swapfile);
    }
}


/*
 * memory-allocation functions.
 */


int add_link(LINK *l)
{
    g_all_links.push_back(*l);
    return static_cast<int>(g_all_links.size() - 1);
}


int add_page(TOPIC *t, PAGE const *p)
{
    t->page.push_back(*p);
    return t->num_page++;
}


int add_topic(TOPIC const *t)
{
    g_topics.push_back(*t);
    return static_cast<int>(g_topics.size() - 1);
}


int add_label(LABEL const *l)
{
    if (l->name[0] == '@')    // if it's a private label...
    {
        g_private_labels.push_back(*l);
        return static_cast<int>(g_private_labels.size() - 1);
    }

    g_labels.push_back(*l);
    return static_cast<int>(g_labels.size() - 1);
}


int add_content(CONTENT const *c)
{
    g_contents.push_back(*c);
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
        fatal(0, "Compiler Error -- Read char buffer overflow!");
    }

    read_char_buff[++read_char_buff_pos] = ch;

    --srccol;
}


int read_char_aux()
{
    int ch;

    if (srcline <= 0)
    {
        srcline = 1;
        srccol = 0;
    }

    if (read_char_buff_pos >= 0)
    {
        ++srccol;
        return read_char_buff[read_char_buff_pos--];
    }

    if (read_char_sp > 0)
    {
        --read_char_sp;
        return ' ';
    }

    if (feof(srcfile))
    {
        return -1;
    }

    while (true)
    {
        ch = getc(srcfile);

        switch (ch)
        {
        case '\t':    // expand a tab
        {
            int diff = (((srccol/8) + 1) * 8) - srccol;

            srccol += diff;
            read_char_sp += diff;
            break;
        }

        case ' ':
            ++srccol;
            ++read_char_sp;
            break;

        case '\n':
            read_char_sp = 0;   // delete spaces before a \n
            srccol = 0;
            ++srcline;
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
                ungetc(ch, srcfile);
                --read_char_sp;
                return ' ';
            }

            ++srccol;
            return ch;

        } // switch
    }
}


int read_char()
{
    int ch;

    ch = read_char_aux();

    while (ch == ';' && srccol == 1)    // skip over comments
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
            ch = atoi(buff);
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

        *buff++ = ch;

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
    if (how_many > 2 && compress_spaces)
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
    skip_over(" \t\n");
    char *ptr = read_until(cmd, 128, ",}");
    bool last = (*ptr == '}');
    --ptr;
    while (ptr >= cmd && std::strchr(" \t\n", *ptr))     // strip trailing spaces
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

void process_doc_contents(modes mode)
{
    TOPIC t;
    t.flags     = 0;
    t.title_len = (unsigned) std::strlen(DOCCONTENTS_TITLE)+1;
    t.title     = DOCCONTENTS_TITLE;
    t.doc_page  = -1;
    t.num_page  = 0;

    g_curr = &buffer[0];

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
    add_content(&c);

    while (true)
    {
        int const ch = read_char();
        if (ch == '{')   // process a CONTENT entry
        {
            c.flags = 0;
            c.num_topic = 0;
            c.doc_page = -1;
            c.srcfile = src_cfname;
            c.srcline = srcline;

            if (get_next_item())
            {
                error(0, "Unexpected end of DocContent entry.");
                continue;
            }
            c.id = cmd;

            if (get_next_item())
            {
                error(0, "Unexpected end of DocContent entry.");
                continue;
            }
            int const indent = atoi(cmd);

            bool last = get_next_item();

            if (cmd[0] == '\"')
            {
                char *ptr = &cmd[1];
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
                c.name = cmd;
            }

            // now, make the entry in the buffer
            if (mode == modes::HTML)
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
                c.page_num_pos = (unsigned)((ptr-3) - &buffer[0]);
                g_curr = ptr;
            }

            while (!last)
            {
                last = get_next_item();
                if (stricmp(cmd, "FF") == 0)
                {
                    if (c.flags & CF_NEW_PAGE)
                    {
                        warn(0, "FF already present in this entry.");
                    }
                    c.flags |= CF_NEW_PAGE;
                    continue;
                }

                if (cmd[0] == '\"')
                {
                    char *ptr = &cmd[1];
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
                    c.topic_name[c.num_topic] = cmd;
                }

                if (++c.num_topic >= MAX_CONTENT_TOPIC)
                {
                    error(0, "Too many topics in DocContent entry.");
                    break;
                }
            }

            add_content(&c);
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

    alloc_topic_text(&t, (unsigned)(g_curr - &buffer[0]));
    add_topic(&t);
}


int parse_link()   // returns length of link or 0 on error
{
    char *ptr;
    bool bad = false;
    int   len;
    int   err_off;

    LINK l;
    l.srcfile  = src_cfname;
    l.srcline  = srcline;
    l.doc_page = -1;

    char *end = read_until(cmd, 128, "}\n");   // get the entire hot-link

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

    if (cmd[0] == '=')   // it's an "explicit" link to a label or "special"
    {
        ptr = std::strchr(cmd, ' ');

        if (ptr == nullptr)
        {
            ptr = end;
        }
        else
        {
            *ptr++ = '\0';
        }

        len = (int)(end - ptr);

        if (cmd[1] == '-')
        {
            l.type      = link_types::LT_SPECIAL;
            l.topic_num = atoi(&cmd[1]);
            l.topic_off = 0;
            l.name.clear();
        }
        else
        {
            l.type = link_types::LT_LABEL;
            if ((int)std::strlen(cmd) > 32)
            {
                warn(err_off, "Label is long.");
            }
            if (cmd[1] == '\0')
            {
                error(err_off, "Explicit hot-link has no Label.");
                bad = true;
            }
            else
            {
                l.name = &cmd[1];
            }
        }
        if (len == 0)
        {
            warn(err_off, "Explicit hot-link has no title.");
        }
    }
    else
    {
        ptr = cmd;
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
        int const lnum = add_link(&l);
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

    ptr = std::strchr(cmd, '=');

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
        while (ch == '\n' || ch == ' ');

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
                fatal(0, "Table is too large.");
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

            ptr = read_until(cmd, 128, ")\n,");

            ch = *ptr;
            *ptr = '\0';

            if (stricmp(cmd, "EndTable") == 0)
            {
                done = true;
            }
            else
            {
                error(1, "Unexpected command in table \"%s\"", cmd);
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

            ptr = read_until(cmd, 128, ")\n,");

            ch = *ptr;
            *ptr = '\0';

            if (stricmp(cmd, "EndComment") == 0)
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

    handle = open(&cmd[7], O_RDONLY|O_BINARY);
    if (handle == -1)
    {
        error(0, "Unable to open \"%s\"", &cmd[7]);
        return ;
    }

    len = filelength(handle);

    if (len >= BUFFER_SIZE)
    {
        error(0, "File \"%s\" is too large to BinInc (%dK).", &cmd[7], (int)(len >> 10));
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


void start_topic(TOPIC *t, char const *title, int title_len)
{
    t->flags = 0;
    t->title_len = title_len;
    t->title.assign(title, title_len);
    t->doc_page = -1;
    t->num_page = 0;
    g_curr = &buffer[0];
}


void end_topic(TOPIC *t)
{
    alloc_topic_text(t, (unsigned)(g_curr - &buffer[0]));
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


void put_a_char(int ch, TOPIC const *t)
{
    if (ch == '{' && !(t->flags & TF_DATA))    // is it a hot-link?
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
    if ((int) std::strlen(cmd) != len)
    {
        error(eoff, "Invalid text after a command \"%s\"", cmd+len);
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
    int    lformat_exclude = format_exclude;

    xonline = false;
    xdoc = false;

    src_cfname = fname;

    srcfile = open_include(fname);
    if (srcfile == nullptr)
    {
        fatal(0, "Unable to open \"%s\"", fname.c_str());
    }

    msg("Compiling: %s", fname.c_str());

    in_topic = false;

    g_curr = &buffer[0];

    while (true)
    {
        ch = read_char();

        if (ch == -1)     // EOF?
        {
            if (include_stack_top >= 0)
            {
                std::fclose(srcfile);
                src_cfname = include_stack[include_stack_top].fname;
                srcfile = include_stack[include_stack_top].file;
                srcline = include_stack[include_stack_top].line;
                srccol  = include_stack[include_stack_top].col;
                --include_stack_top;
                continue;
            }
            if (in_topic)  // if we're in a topic, finish it
            {
                end_topic(&t);
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
            int eoff;

            ch = read_char();
            if (ch == '(')
            {
                imbedded = true;
                eoff = 0;
            }
            else
            {
                imbedded = false;
                eoff = 0;
                unread_char(ch);
            }

            bool done = false;

            while (!done)
            {
                do
                {
                    ch = read_char();
                }
                while (ch == ' ');
                unread_char(ch);

                ptr = read_until(cmd, 128, imbedded ? ")\n," : "\n,");

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
                if (strnicmp(cmd, "Topic=", 6) == 0)
                {
                    if (in_topic)  // if we're in a topic, finish it
                    {
                        end_topic(&t);
                    }
                    else
                    {
                        in_topic = true;
                    }

                    char const *topic_title = &cmd[6];
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

                    start_topic(&t, topic_title, static_cast<int>(title_len));
                    formatting = true;
                    centering = false;
                    state = STATES::S_Start;
                    in_para = false;
                    num_spaces = 0;
                    xonline = false;
                    xdoc = false;
                    lformat_exclude = format_exclude;
                    compress_spaces = true;
                    continue;
                }
                if (strnicmp(cmd, "Data=", 5) == 0)
                {
                    if (in_topic)  // if we're in a topic, finish it
                    {
                        end_topic(&t);
                    }
                    else
                    {
                        in_topic = true;
                    }

                    char const *data = &cmd[5];
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

                    if (cmd[5] == '@')
                    {
                        warn(eoff, "Data topic has a local label.");
                    }

                    start_topic(&t, "", 0);
                    t.flags |= TF_DATA;

                    if ((int)std::strlen(data) > 32)
                    {
                        warn(eoff, "Label name is long.");
                    }

                    lbl.name      = data;
                    lbl.topic_num = static_cast<int>(g_topics.size());
                    lbl.topic_off = 0;
                    lbl.doc_page  = -1;
                    add_label(&lbl);

                    formatting = false;
                    centering = false;
                    state = STATES::S_Start;
                    in_para = false;
                    num_spaces = 0;
                    xonline = false;
                    xdoc = false;
                    lformat_exclude = format_exclude;
                    compress_spaces = false;
                    continue;
                }
                if (strnicmp(cmd, "DocContents", 11) == 0)
                {
                    check_command_length(eoff, 11);
                    if (in_topic)  // if we're in a topic, finish it
                    {
                        end_topic(&t);
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
                    compress_spaces = true;
                    process_doc_contents(mode);
                    in_topic = false;
                    continue;
                }
                if (stricmp(cmd, "Comment") == 0)
                {
                    process_comment();
                    continue;
                }
                if (strnicmp(cmd, "FormatExclude", 13) == 0)
                {
                    if (cmd[13] == '-')
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
                            if (format_exclude > 0)
                            {
                                format_exclude = -format_exclude;
                            }
                            else
                            {
                                warn(eoff, "\"FormatExclude-\" is already in effect.");
                            }
                        }
                    }
                    else if (cmd[13] == '+')
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
                            if (format_exclude < 0)
                            {
                                format_exclude = -format_exclude;
                            }
                            else
                            {
                                warn(eoff, "\"FormatExclude+\" is already in effect.");
                            }
                        }
                    }
                    else if (cmd[13] == '=')
                    {
                        if (cmd[14] == 'n' || cmd[14] == 'N')
                        {
                            check_command_length(eoff, 15);
                            if (in_topic)
                            {
                                lformat_exclude = 0;
                            }
                            else
                            {
                                format_exclude = 0;
                            }
                        }
                        else if (cmd[14] == '\0')
                        {
                            lformat_exclude = format_exclude;
                        }
                        else
                        {
                            int n = ((in_topic ? lformat_exclude : format_exclude) < 0) ? -1 : 1;

                            lformat_exclude = atoi(&cmd[14]);

                            if (lformat_exclude <= 0)
                            {
                                error(eoff, "Invalid argument to FormatExclude=");
                                lformat_exclude = 0;
                            }

                            lformat_exclude *= n;

                            if (!in_topic)
                            {
                                format_exclude = lformat_exclude;
                            }
                        }
                    }
                    else
                    {
                        error(eoff, "Invalid format for FormatExclude");
                    }

                    continue;
                }
                if (strnicmp(cmd, "Include ", 8) == 0)
                {
                    if (include_stack_top >= MAX_INCLUDE_STACK-1)
                    {
                        error(eoff, "Too many nested Includes.");
                    }
                    else
                    {
                        ++include_stack_top;
                        include_stack[include_stack_top].fname = src_cfname;
                        include_stack[include_stack_top].file = srcfile;
                        include_stack[include_stack_top].line = srcline;
                        include_stack[include_stack_top].col  = srccol;
                        std::string const file_name = &cmd[8];
                        srcfile = open_include(file_name);
                        if (srcfile == nullptr)
                        {
                            error(eoff, "Unable to open \"%s\"", file_name.c_str());
                            srcfile = include_stack[include_stack_top--].file;
                        }
                        src_cfname = file_name;
                        srcline = 1;
                        srccol = 0;
                    }

                    continue;
                }


                // commands allowed only before all topics...

                if (!in_topic)
                {
                    if (strnicmp(cmd, "HdrFile=", 8) == 0)
                    {
                        if (!hdr_fname.empty())
                        {
                            warn(eoff, "Header Filename has already been defined.");
                        }
                        hdr_fname = &cmd[8];
                    }
                    else if (strnicmp(cmd, "HlpFile=", 8) == 0)
                    {
                        if (!hlp_fname.empty())
                        {
                            warn(eoff, "Help Filename has already been defined.");
                        }
                        hlp_fname = &cmd[8];
                    }
                    else if (strnicmp(cmd, "Version=", 8) == 0)
                    {
                        if (version != -1)   // an unlikely value
                        {
                            warn(eoff, "Help version has already been defined");
                        }
                        version = atoi(&cmd[8]);
                    }
                    else
                    {
                        error(eoff, "Bad or unexpected command \"%s\"", cmd);
                    }

                    continue;
                }
                // commands allowed only in a topic...
                if (strnicmp(cmd, "FF", 2) == 0)
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
                else if (strnicmp(cmd, "DocFF", 5) == 0)
                {
                    check_command_length(eoff, 5);
                    if (in_para)
                    {
                        *g_curr++ = '\n';    // finish off current paragraph
                    }
                    if (!xonline)
                    {
                        *g_curr++ = CMD_XONLINE;
                    }
                    *g_curr++ = CMD_FF;
                    if (!xonline)
                    {
                        *g_curr++ = CMD_XONLINE;
                    }
                    state = STATES::S_Start;
                    in_para = false;
                    num_spaces = 0;
                }
                else if (strnicmp(cmd, "OnlineFF", 8) == 0)
                {
                    check_command_length(eoff, 8);
                    if (in_para)
                    {
                        *g_curr++ = '\n';    // finish off current paragraph
                    }
                    if (!xdoc)
                    {
                        *g_curr++ = CMD_XDOC;
                    }
                    *g_curr++ = CMD_FF;
                    if (!xdoc)
                    {
                        *g_curr++ = CMD_XDOC;
                    }
                    state = STATES::S_Start;
                    in_para = false;
                    num_spaces = 0;
                }
                else if (strnicmp(cmd, "Label=", 6) == 0)
                {
                    char const *label_name = &cmd[6];
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

                        if ((t.flags & TF_DATA) && cmd[6] == '@')
                        {
                            warn(eoff, "Data topic has a local label.");
                        }

                        lbl.name      = label_name;
                        lbl.topic_num = static_cast<int>(g_topics.size());
                        lbl.topic_off = (unsigned)(g_curr - &buffer[0]);
                        lbl.doc_page  = -1;
                        add_label(&lbl);
                    }
                }
                else if (strnicmp(cmd, "Table=", 6) == 0)
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
                else if (strnicmp(cmd, "FormatExclude", 12) == 0)
                {
                    if (cmd[13] == '-')
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
                    else if (cmd[13] == '+')
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
                else if (strnicmp(cmd, "Format", 6) == 0)
                {
                    if (cmd[6] == '+')
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
                    else if (cmd[6] == '-')
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
                else if (strnicmp(cmd, "Online", 6) == 0)
                {
                    if (cmd[6] == '+')
                    {
                        check_command_length(eoff, 7);

                        if (xonline)
                        {
                            *g_curr++ = CMD_XONLINE;
                            xonline = false;
                        }
                        else
                        {
                            warn(eoff, "\"Online+\" already in effect.");
                        }
                    }
                    else if (cmd[6] == '-')
                    {
                        check_command_length(eoff, 7);
                        if (!xonline)
                        {
                            *g_curr++ = CMD_XONLINE;
                            xonline = true;
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
                else if (strnicmp(cmd, "Doc", 3) == 0)
                {
                    if (cmd[3] == '+')
                    {
                        check_command_length(eoff, 4);
                        if (xdoc)
                        {
                            *g_curr++ = CMD_XDOC;
                            xdoc = false;
                        }
                        else
                        {
                            warn(eoff, "\"Doc+\" already in effect.");
                        }
                    }
                    else if (cmd[3] == '-')
                    {
                        check_command_length(eoff, 4);
                        if (!xdoc)
                        {
                            *g_curr++ = CMD_XDOC;
                            xdoc = true;
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
                else if (strnicmp(cmd, "Center", 6) == 0)
                {
                    if (cmd[6] == '+')
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
                    else if (cmd[6] == '-')
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
                else if (strnicmp(cmd, "CompressSpaces", 14) == 0)
                {
                    check_command_length(eoff, 15);

                    if (cmd[14] == '+')
                    {
                        if (compress_spaces)
                        {
                            warn(eoff, "\"CompressSpaces+\" is already in effect.");
                        }
                        else
                        {
                            compress_spaces = true;
                        }
                    }
                    else if (cmd[14] == '-')
                    {
                        if (!compress_spaces)
                        {
                            warn(eoff, "\"CompressSpaces-\" is already in effect.");
                        }
                        else
                        {
                            compress_spaces = false;
                        }
                    }
                    else
                    {
                        error(eoff, "Invalid argument to CompressSpaces.");
                    }
                }
                else if (strnicmp("BinInc ", cmd, 7) == 0)
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
                    error(eoff, "Bad or unexpected command \"%s\".", cmd);
                }
                // else

            } // while (!done)

            continue;
        }

        if (!in_topic)
        {
            cmd[0] = ch;
            ptr = read_until(&cmd[1], 127, "\n~");
            if (*ptr == '~')
            {
                unread_char('~');
            }
            *ptr = '\0';
            error(0, "Text outside of any topic \"%s\".", cmd);
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
                    put_a_char(ch, &t);
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
                        put_a_char(ch, &t);
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
                        put_a_char(ch, &t);
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
                        put_a_char(ch, &t);
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
                        put_a_char(ch, &t);
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

    std::fclose(srcfile);

    srcline = -1;
}


/*
 * stuff to resolve hot-link references.
 */
void link_topic(LINK &l)
{
    int const t = find_topic_title(l.name.c_str());
    if (t == -1)
    {
        src_cfname = l.srcfile;
        srcline = l.srcline; // pretend we are still in the source...
        error(0, "Cannot find implicit hot-link \"%s\".", l.name.c_str());
        srcline = -1;  // back to reality
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
            src_cfname = l.srcfile;
            srcline = l.srcline;
            error(0, "Label \"%s\" is a data-only topic.", l.name.c_str());
            srcline = -1;
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
        src_cfname = l.srcfile;
        srcline = l.srcline; // pretend again
        error(0, "Cannot find explicit hot-link \"%s\".", l.name.c_str());
        srcline = -1;
    }
}

void label_topic(CONTENT &c, int ctr)
{
    if (LABEL *lbl = find_label(c.topic_name[ctr].c_str()))
    {
        if (g_topics[lbl->topic_num].flags & TF_DATA)
        {
            src_cfname = c.srcfile;
            srcline = c.srcline;
            error(0, "Label \"%s\" is a data-only topic.", c.topic_name[ctr].c_str());
            srcline = -1;
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
        src_cfname = c.srcfile;
        srcline = c.srcline;
        error(0, "Cannot find DocContent label \"%s\".", c.topic_name[ctr].c_str());
        srcline = -1;
    }
}

void content_topic(CONTENT &c, int ctr)
{
    int const t = find_topic_title(c.topic_name[ctr].c_str());
    if (t == -1)
    {
        src_cfname = c.srcfile;
        srcline = c.srcline;
        error(0, "Cannot find DocContent topic \"%s\".", c.topic_name[ctr].c_str());
        srcline = -1;  // back to reality
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


void add_page_break(TOPIC *t, int margin, char const *text, char const *start, char const *curr, int num_links)
{
    PAGE p;
    p.offset = (unsigned)(start - text);
    p.length = (unsigned)(curr - start);
    p.margin = margin;
    add_page(t, &p);

    if (max_links < num_links)
    {
        max_links = num_links;
    }
}


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

        char *text = get_topic_text(&t);
        char *curr = text;
        unsigned int len = t.text_len;

        char *start = curr;
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
                            add_page_break(&t, start_margin, text, start, curr, num_links);
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
                    add_page_break(&t, start_margin, text, start, curr, num_links);
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
                add_page_break(&t, start_margin, text, start, curr, num_links);
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
            add_page_break(&t, start_margin, text, start, curr, num_links);
        }

        if (max_pages < t.num_page)
        {
            max_pages = t.num_page;
        }

        release_topic_text(&t, 0);
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

struct PAGINATE_DOC_INFO : public DOC_INFO
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
                src_cfname = l.srcfile;
                srcline = l.srcline; // pretend we are still in the source...
                error(0, "Cannot find implicit hot-link \"%s\".", l.name.c_str());
                srcline = -1;  // back to reality
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
                src_cfname = l.srcfile;
                srcline = l.srcline; // pretend again
                error(0, "Cannot find explicit hot-link \"%s\".", l.name.c_str());
                srcline = -1;
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
    TOPIC *t = &g_topics[tnum];

    char *base = get_topic_text(t);

    for (CONTENT const &c : g_contents)
    {
        assert(c.doc_page >= 1);
        std::sprintf(buf, "%d", c.doc_page);
        len = (int) std::strlen(buf);
        assert(len <= 3);
        std::memcpy(base + c.page_num_pos + (3 - len), buf, len);
    }

    release_topic_text(t, 1);
}


// this funtion also used by print_document()
bool pd_get_info(int cmd, PD_INFO *pd, void *context)
{
    DOC_INFO &info = *static_cast<DOC_INFO *>(context);
    CONTENT const *c;

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
        pd->curr = get_topic_text(&g_topics[c->topic_num[info.topic_num]]);
        pd->len = g_topics[c->topic_num[info.topic_num]].text_len;
        return true;

    case PD_GET_LINK_PAGE:
    {
        LINK const &link = g_all_links[getint(pd->s)];
        if (link.doc_page == -1)
        {
            if (info.link_dest_warn)
            {
                src_cfname = link.srcfile;
                srcline    = link.srcline;
                warn(0, "Hot-link destination is not in the document.");
                srcline = -1;
            }
            return false;
        }
        pd->i = g_all_links[getint(pd->s)].doc_page;
        return true;
    }

    case PD_RELEASE_TOPIC:
        c = &g_contents[info.content_num];
        release_topic_text(&g_topics[c->topic_num[info.topic_num]], 0);
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
        ++num_doc_pages;
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

int fcmp_LABEL(void const *a, void const *b)
{
    char const *an = static_cast<LABEL const *>(a)->name.c_str();
    char const *bn = static_cast<LABEL const *>(b)->name.c_str();

    // compare the names, making sure that the index goes first
    int const diff = std::strcmp(an, bn);
    if (diff == 0)
    {
        return 0;
    }

    if (std::strcmp(an, INDEX_LABEL) == 0)
    {
        return -1;
    }

    if (std::strcmp(bn, INDEX_LABEL) == 0)
    {
        return 1;
    }

    return diff;
}


void sort_labels()
{
    qsort(&g_labels[0], static_cast<int>(g_labels.size()),  sizeof(LABEL), fcmp_LABEL);
    qsort(&g_private_labels[0], static_cast<int>(g_private_labels.size()), sizeof(LABEL), fcmp_LABEL);
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

    while (!feof(f1) && !feof(f2))
    {
        if (getc(f1) != getc(f2))
        {
            return true;
        }
    }

    return !(feof(f1) && feof(f2));
}


void _write_hdr(char const *fname, std::FILE *file)
{
    char nfile[MAXFILE],
         next[MAXEXT];

    FNSPLIT(fname, nullptr, nullptr, nfile, next);
    std::fprintf(file, "#if !defined(HELP_DEFS_H)\n"
        "#define HELP_DEFS_H\n"
        "\n/*\n * %s%s\n", nfile, next);
    FNSPLIT(src_fname.c_str(), nullptr, nullptr, nfile, next);
    std::fprintf(file, " *\n * Contains #defines for help.\n *\n");
    std::fprintf(file, " * Generated by HC from: %s%s\n *\n */\n\n\n", nfile, next);

    std::fprintf(file, "/* current help file version */\n");
    std::fprintf(file, "\n");
    std::fprintf(file, "#define %-32s %3d\n", "IDHELP_VERSION", version);
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
    std::FILE *temp,
         *hdr;

    hdr = std::fopen(fname, "rt");

    if (hdr == nullptr)
    {
        // if no prev. hdr file generate a new one
        hdr = std::fopen(fname, "wt");
        if (hdr == nullptr)
        {
            fatal(0, "Cannot create \"%s\".", fname);
        }
        msg("Writing: %s", fname);
        _write_hdr(fname, hdr);
        std::fclose(hdr);
        notice("FRACTINT must be re-compiled.");
        return ;
    }

    msg("Comparing: %s", fname);

    temp = std::fopen(TEMP_FNAME, "wt");

    if (temp == nullptr)
    {
        fatal(0, "Cannot create temporary file: \"%s\".", TEMP_FNAME);
    }

    _write_hdr(fname, temp);

    std::fclose(temp);
    temp = std::fopen(TEMP_FNAME, "rt");

    if (temp == nullptr)
    {
        fatal(0, "Cannot open temporary file: \"%s\".", TEMP_FNAME);
    }

    if (compare_files(temp, hdr))     // if they are different...
    {
        msg("Updating: %s", fname);
        std::fclose(temp);
        std::fclose(hdr);
        std::remove(fname);               // delete the old hdr file
        std::rename(TEMP_FNAME, fname);   // rename the temp to the hdr file
        notice("FRACTINT must be re-compiled.");
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

    offset = std::accumulate(g_contents.begin(), g_contents.end(), offset, [](long offset, CONTENT const &cp) {
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
            LINK *l = &g_all_links[ getint(curr+1) ];
            setint(curr+1, l->topic_num);
            setint(curr+1+sizeof(int), l->topic_off);
            setint(curr+1+2*sizeof(int), l->doc_page);
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
    hs.version = version;

    fwrite(&hs, sizeof(long)+sizeof(int), 1, file);
    notice("Wrote signature and version");

    // write max_pages & max_links

    putw(max_pages, file);
    putw(max_links, file);
    notice("Wrote max pages, max links");

    // write num_topic, num_label and num_contents

    putw(static_cast<int>(g_topics.size()), file);
    putw(static_cast<int>(g_labels.size()), file);
    putw(static_cast<int>(g_contents.size()), file);
    notice("Wrote num topics, num labels, num contents");

    // write num_doc_page

    putw(num_doc_pages, file);
    notice("Wrote num doc pages");

    // write the offsets to each topic
    for (TOPIC const &t : g_topics)
    {
        fwrite(&t.offset, sizeof(long), 1, file);
    }
    notice("Wrote topic offsets");

    // write all public labels
    for (LABEL const &l : g_labels)
    {
        putw(l.topic_num, file);
        putw(l.topic_off, file);
    }
    notice("Wrote public labels");

    // write contents
    for (CONTENT const &cp : g_contents)
    {
        putw(cp.flags, file);

        int t = (int) cp.id.length();
        putc((BYTE)t, file);
        fwrite(cp.id.c_str(), 1, t, file);

        t = (int) cp.name.length();
        putc((BYTE)t, file);
        fwrite(cp.name.c_str(), 1, t, file);

        putc((BYTE)cp.num_topic, file);
        fwrite(cp.topic_num, sizeof(int), cp.num_topic, file);
    }
    notice("Wrote contents");

    // write topics
    int i{};
    for (TOPIC const &tp : g_topics)
    {
        // write the topics flags
        putw(tp.flags, file);
        notice("Wrote topic %d flags", i);

        // write offset, length and starting margin for each page

        putw(tp.num_page, file);
        for (PAGE const &p : tp.page)
        {
            putw(p.offset, file);
            putw(p.length, file);
            putw(p.margin, file);
        }
        notice("Wrote topic %d offset, length, starting margins", i);

        // write the help title

        putc((BYTE)tp.title_len, file);
        fwrite(tp.title.c_str(), 1, tp.title_len, file);
        notice("Wrote topic %d title", i);

        // insert hot-link info & write the help text

        text = get_topic_text(&tp);

        if (!(tp.flags & TF_DATA))     // don't process data topics...
        {
            insert_real_link_info(text, tp.text_len);
        }

        putw(tp.text_len, file);
        fwrite(text, 1, tp.text_len, file);
        notice("Wrote topic %d hot link info and help text", i);

        release_topic_text(&tp, 0);  // don't save the text even though
        // insert_real_link_info() modified it
        // because we don't access the info after
        // this.
        notice("Release topic %d", i);

        ++i;
    }
    notice("Done writing help");
}


void write_help(char const *fname)
{
    std::FILE *hlp;

    hlp = std::fopen(fname, "wb");

    if (hlp == nullptr)
    {
        fatal(0, "Cannot create .HLP file: \"%s\".", fname);
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
                fputc(' ', info->file);
                --info->spaces;
            }

            fputc(c, info->file);
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
        fatal(0, ".SRC has no DocContents.");
    }

    msg("Printing to: %s", fname);

    info.topic_num = -1;
    info.content_num = info.topic_num;
    info.link_dest_warn = false;

    info.file = std::fopen(fname, "wt");
    if (info.file == nullptr)
    {
        fatal(0, "Couldn't create \"%s\"", fname);
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
    long bytes_in_strings = 0,  // bytes in strings
         text   = 0,   // bytes in topic text (stored on disk)
         data   = 0,   // bytes in active data structure
         dead   = 0;   // bytes in unused data structure

    for (TOPIC const &t : g_topics)
    {
        data   += sizeof(TOPIC);
        bytes_in_strings += t.title_len;
        text   += t.text_len;
        data   += t.num_page * sizeof(PAGE);

        std::vector<PAGE> const &pages = t.page;
        dead += static_cast<long>((pages.capacity() - pages.size()) * sizeof(PAGE));
    }

    for (LINK const &l : g_all_links)
    {
        data += sizeof(LINK);
        bytes_in_strings += (long) l.name.length();
    }

    dead += static_cast<long>((g_all_links.capacity() - g_all_links.size()) * sizeof(LINK));

    for (LABEL const &l : g_labels)
    {
        data   += sizeof(LABEL);
        bytes_in_strings += (long) l.name.length() + 1;
    }

    dead += static_cast<long>((g_labels.capacity() - g_labels.size()) * sizeof(LABEL));

    for (LABEL const &l : g_private_labels)
    {
        data   += sizeof(LABEL);
        bytes_in_strings += (long) l.name.length() + 1;
    }

    dead += static_cast<long>((g_private_labels.capacity() - g_private_labels.size()) * sizeof(LABEL));

    for (CONTENT const &c : g_contents)
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
    for (TOPIC const &t : g_topics)
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
    std::printf("%8d Document pages\n", num_doc_pages);
}


/*
 * add/delete help from .EXE functions.
 */


void add_hlp_to_exe(char const *hlp_fname, char const *exe_fname)
{
    int                  exe,   // handles
                         hlp;
    long                 len;
    int                  size;
    help_sig_info hs;

    exe = open(exe_fname, O_RDWR|O_BINARY);
    if (exe == -1)
    {
        fatal(0, "Unable to open \"%s\"", exe_fname);
    }

    hlp = open(hlp_fname, O_RDONLY|O_BINARY);
    if (hlp == -1)
    {
        fatal(0, "Unable to open \"%s\"", hlp_fname);
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
        fatal(0, "Help signature not found in %s", hlp_fname);
    }

    msg("Help file %s Version=%d", hlp_fname, hs.version);

    // append the help stuff, overwriting old help (if any)

    lseek(exe, hs.base, SEEK_SET);

    len = filelength(hlp) - sizeof(long) - sizeof(int); // adjust for the file signature & version

    for (int count = 0; count < len;)
    {
        size = (int) std::min((long)BUFFER_SIZE, len-count);
        if (read(hlp, &buffer[0], size) != size)
        {
            throw std::system_error(errno, std::system_category(), "add_hlp_to_exe failed read3");
        }
        if (write(exe, &buffer[0], size) != size)
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
        fatal(0, "Unable to open \"%s\"", exe_fname);
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
        fatal(0, "No help found in %s", exe_fname);
    }
}


/*
 * command-line parser, etc.
 */
struct compiler_options
{
    modes       mode{modes::NONE};
    std::string fname1;
    std::string fname2;
    std::string swappath;
    bool        show_mem{};
    bool        show_stats{};
};

class compiler
{
public:
    compiler(int argc_, char *argv_[])
        : argc(argc_),
        argv(argv_)
    {
    }

    ~compiler()
    {
        if (swapfile != nullptr)
        {
            std::fclose(swapfile);
            std::remove(m_options.swappath.c_str());
        }
    }

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

    int              argc;
    char           **argv;
    compiler_options m_options;
};

compiler_options parse_compiler_options(int argc, char **argv)
{
    compiler_options result{};
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg{argv[i]};
        if (arg[0] == '/' || arg[0] == '-')
        {
            switch (arg[1])
            {
            case 'a':
                if (result.mode == modes::NONE)
                {
                    result.mode = modes::APPEND;
                }
                else
                {
                    fatal(0, "Cannot have /a with /c, /d, /h or /p");
                }
                break;

            case 'c':
                if (result.mode == modes::NONE)
                {
                    result.mode = modes::COMPILE;
                }
                else
                {
                    fatal(0, "Cannot have /c with /a, /d, /h or /p");
                }
                break;

            case 'd':
                if (result.mode == modes::NONE)
                {
                    result.mode = modes::DELETE;
                }
                else
                {
                    fatal(0, "Cannot have /d with /a, /c, /h or /p");
                }
                break;

            case 'h':
                if (result.mode == modes::NONE)
                {
                    result.mode = modes::HTML;
                }
                else
                {
                    fatal(0, "Cannot have /h with /a, /c, /d or /p");
                }
                break;

            case 'i':
                if (i < argc - 1)
                {
                    g_include_paths.emplace_back(argv[i + 1]);
                    ++i;
                }
                else
                {
                    fatal(0, "Missing argument for /i");
                }
                break;

            case 'm':
                if (result.mode == modes::COMPILE)
                {
                    result.show_mem = true;
                }
                else
                {
                    fatal(0, "/m switch allowed only when compiling (/c)");
                }
                break;

            case 'o':
                if (result.mode == modes::HTML)
                {
                    if (i < argc - 1)
                    {
                        g_html_output_dir = argv[i + 1];
                        ++i;
                    }
                    else
                    {
                        fatal(0, "Missing argument for /o");
                    }
                }
                else
                {
                    fatal(0, "/o switch allowed only when writing HTML (/h)");
                }
                break;

            case 'p':
                if (result.mode == modes::NONE)
                {
                    result.mode = modes::PRINT;
                }
                else
                {
                    fatal(0, "Cannot have /p with /a, /c, /h or /d");
                }
                break;

            case 's':
                if (result.mode == modes::COMPILE)
                {
                    result.show_stats = true;
                }
                else
                {
                    fatal(0, "/s switch allowed only when compiling (/c)");
                }
                break;

            case 'r':
                if (result.mode == modes::COMPILE || result.mode == modes::PRINT)
                {
                    if (i < argc - 1)
                    {
                        result.swappath = argv[i + 1];
                        ++i;
                    }
                    else
                    {
                        fatal(0, "Missing argument for /r");
                    }
                }
                else
                {
                    fatal(0, "/r switch allowed when compiling (/c) or printing (/p)");
                }
                break;

            case 'q':
                quiet_mode = true;
                break;

            default:
                fatal(0, "Bad command-line switch /%c", arg[1]);
                break;
            }
        }
        else
        {
            // assume it is a filename
            if (result.fname1.empty())
            {
                result.fname1 = arg;
            }
            else if (result.fname2.empty())
            {
                result.fname2 = arg;
                std::cout << "Assigning to fname2" << std::endl;
            }
            else
            {
                fatal(0, "Unexpected command-line argument \"%s\"", arg.c_str());
            }
        }
    }
    return result;
}

void compiler::parse_arguments()
{
    m_options = parse_compiler_options(argc, argv);
}

int compiler::process()
{
    std::printf("HC - FRACTINT Help Compiler.\n\n");

    buffer.resize(BUFFER_SIZE);

    parse_arguments();

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
            fatal(0, "Unexpected argument \"%s\"", m_options.fname2.c_str());
        }
        delete_hlp_from_exe(m_options.fname1.empty() ? DEFAULT_EXE_FNAME : m_options.fname1.c_str());
        break;

    case modes::HTML:
        render_html();
        break;
    }

    return errors;     // return the number of errors
}

void compiler::usage()
{
    std::printf("To compile a .SRC file:\n");
    std::printf("      HC /c [/s] [/m] [/r[path]] [src_file]\n");
    std::printf("         /s       = report statistics.\n");
    std::printf("         /m       = report memory usage.\n");
    std::printf("         /r[path] = set swap file path.\n");
    std::printf("         src_file = .SRC file.  Default is \"%s\"\n", DEFAULT_SRC_FNAME);
    std::printf("To print a .SRC file:\n");
    std::printf("      HC /p [/r[path]] [src_file] [out_file]\n");
    std::printf("         /r[path] = set swap file path.\n");
    std::printf("         src_file = .SRC file.  Default is \"%s\"\n", DEFAULT_SRC_FNAME);
    std::printf("         out_file = Filename to print to. Default is \"%s\"\n",
        DEFAULT_DOC_FNAME);
    std::printf("To append a .HLP file to an .EXE file:\n");
    std::printf("      HC /a [hlp_file] [exe_file]\n");
    std::printf("         hlp_file = .HLP file.  Default is \"%s\"\n", DEFAULT_HLP_FNAME);
    std::printf("         exe_file = .EXE file.  Default is \"%s\"\n", DEFAULT_EXE_FNAME);
    std::printf("To delete help info from an .EXE file:\n");
    std::printf("      HC /d [exe_file]\n");
    std::printf("         exe_file = .EXE file.  Default is \"%s\"\n", DEFAULT_EXE_FNAME);
    std::printf("\n");
    std::printf("Use \"/q\" for quiet mode. (No status messages.)\n");
}

void compiler::read_source_file(modes mode)
{
    src_fname = m_options.fname1.empty() ? DEFAULT_SRC_FNAME : m_options.fname1;

    m_options.swappath += SWAP_FNAME;

    swapfile = std::fopen(m_options.swappath.c_str(), "w+b");
    if (swapfile == nullptr)
    {
        fatal(0, "Cannot create swap file \"%s\"", m_options.swappath.c_str());
    }
    swappos = 0;

    read_src(src_fname, mode);
}

void compiler::compile()
{
    if (!m_options.fname2.empty())
    {
        fatal(0, "Unexpected command-line argument \"%s\"", m_options.fname2.c_str());
    }

    read_source_file(m_options.mode);

    if (hdr_fname.empty())
    {
        error(0, "No .H file defined.  (Use \"~HdrFile=\")");
    }
    if (hlp_fname.empty())
    {
        error(0, "No .HLP file defined.  (Use \"~HlpFile=\")");
    }
    if (version == -1)
    {
        warn(0, "No help version has been defined.  (Use \"~Version=\")");
    }

    // order of these is very important...

    make_hot_links();  // do even if errors since it may report more...

    if (!errors)
    {
        paginate_online();
    }
    if (!errors)
    {
        paginate_document();
    }
    if (!errors)
    {
        calc_offsets();
    }
    if (!errors)
    {
        sort_labels();
    }
    if (!errors)
    {
        write_hdr(hdr_fname.c_str());
    }
    if (!errors)
    {
        write_help(hlp_fname.c_str());
    }

    if (m_options.show_stats)
    {
        report_stats();
    }

    if (m_options.show_mem)
    {
        report_memory();
    }

    if (errors || warnings)
    {
        report_errors();
    }
}

void compiler::print()
{
    read_source_file(m_options.mode);
    make_hot_links();

    if (!errors)
    {
        paginate_document();
    }
    if (!errors)
    {
        print_document(m_options.fname2.empty() ? DEFAULT_DOC_FNAME : m_options.fname2.c_str());
    }

    if (errors || warnings)
    {
        report_errors();
    }
}

void compiler::render_html()
{
    read_source_file(m_options.mode);
    make_hot_links();

    if (errors == 0)
    {
        paginate_html_document();
    }
    if (!errors)
    {
        calc_offsets();
    }
    if (!errors)
    {
        sort_labels();
    }
    if (errors == 0)
    {
        print_html_document(m_options.fname2.empty() ? DEFAULT_HTML_FNAME : m_options.fname2);
    }
    if (errors > 0 || warnings > 0)
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

        char *text = get_topic_text(&t);
        char *curr = text;
        unsigned int len = t.text_len;

        char *start = curr;
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

        if (max_pages < t.num_page)
        {
            max_pages = t.num_page;
        }

        release_topic_text(&t, 0);
    } // for
}

class html_processor
{
public:
    html_processor(std::string const &fname)
        : m_fname(fname)
    {
    }

    void process();

private:
    void write_index_html();
    void write_contents();
    void write_content(CONTENT const &c);
    void write_topic(TOPIC const &t);

    std::string m_fname;
};

void compiler::print_html_document(std::string const &fname)
{
    html_processor(fname).process();
}

void html_processor::process()
{
    if (g_contents.empty())
    {
        fatal(0, ".SRC has no DocContents.");
    }

    write_index_html();
    write_contents();
}

void html_processor::write_index_html()
{
    msg("Writing index.rst");

    CONTENT const &toc = g_contents[0];
    if (toc.num_topic != 1)
    {
        throw std::runtime_error("First content block contains multiple topics.");
    }
    if (toc.topic_name[0] != DOCCONTENTS_TITLE)
    {
        throw std::runtime_error("First content block doesn't contain DocContent.");
    }

    TOPIC const &toc_topic = g_topics[toc.topic_num[0]];
    std::ofstream str(g_html_output_dir + "/index.rst");
    str << ".. toctree::\n";
    char const *text = get_topic_text(&toc_topic);
    char const *curr = text;
    unsigned int len = toc_topic.text_len;
    while (len > 0)
    {
        int size = 0;
        int width = 0;
        token_types const tok = find_token_length(token_modes::ONLINE, curr, len, &size, &width);

        switch (tok)
        {
        case token_types::TOK_SPACE:
            break;

        case token_types::TOK_NL:
            str << '\n';
            break;

        case token_types::TOK_WORD:
            str << "   " << std::string(curr, width);
            break;

        default:
            throw std::runtime_error("Unexpected token in table of contents.");
        }
        len -= size;
        curr += size;
    }
}

void html_processor::write_contents()
{
    for (CONTENT const &c : g_contents)
    {
        write_content(c);
    }
}

void html_processor::write_content(CONTENT const &c)
{
    for (int i = 0; i < c.num_topic; ++i)
    {
        TOPIC const &t = g_topics[c.topic_num[i]];
        if (t.title == DOCCONTENTS_TITLE)
        {
            continue;
        }

        write_topic(t);
    }
}

void html_processor::write_topic(TOPIC const &t)
{
    std::string const filename = rst_name(t.title) + ".rst";
    msg("Writing %s", filename.c_str());
    std::ofstream str(g_html_output_dir + '/' + filename);
    char const *text = get_topic_text(&t);
    char const *curr = text;
    unsigned int len = t.text_len;
    unsigned int column = 0;
    std::string spaces;
    auto const nl = [&str, &column](int width)
    {
        if (column + width > 70)
        {
            str << '\n';
            column = 0;
            return true;
        }
        return false;
    };
    while (len > 0)
    {
        int size = 0;
        int width = 0;
        token_types const tok = find_token_length(token_modes::ONLINE, curr, len, &size, &width);

        switch (tok)
        {
        case token_types::TOK_SPACE:
            if (!nl(width))
            {
                spaces = std::string(width, ' ');
                column += width;
            }
            else
            {
                spaces.clear();
            }
            break;

        case token_types::TOK_NL:
            str << '\n';
            spaces.clear();
            column = 0;
            break;

        case token_types::TOK_WORD:
            if (!nl(width) && !spaces.empty())
            {
                str << spaces;
                spaces.clear();
            }
            str << std::string(curr, width);
            column += width;
            break;

        case token_types::TOK_PARA:
            if (column > 0)
            {
                str << '\n';
            }
            column = 0;
            spaces.clear();
            break;

        case token_types::TOK_LINK:
            {
                char const *data = &curr[1];
                int const link_num = getint(data);
                int const link_topic_num = g_all_links[link_num].topic_num;
                data += 3*sizeof(int);
                std::string const link_text(":doc:`" + std::string(data, width) +
                    " <" + rst_name(g_topics[link_topic_num].title) + ">`");
                if (!nl(static_cast<int>(link_text.length())) && !spaces.empty())
                {
                    str << spaces;
                    spaces.clear();
                }
                str << link_text;
                column += static_cast<unsigned int>(link_text.length());
            }
            break;

        case token_types::TOK_FF:
        case token_types::TOK_XONLINE:
        case token_types::TOK_XDOC:
            break;

        default:
            throw std::runtime_error("Unexpected token in topic.");
        }
        len -= size;
        curr += size;
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
        fatal(0, "Buffer overflowerd -- Help topic too large.");
    }
}
#if defined(_WIN32)
#pragma warning(pop)
#endif

} // namespace

int main(int argc, char *argv[])
{
    std::cerr << "Running compiler\n";
    return compiler(argc, argv).process();
}
