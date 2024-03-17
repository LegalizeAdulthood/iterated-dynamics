#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "find_path.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_io.h"
#include "os.h"
#include "realdos.h"

#include <fcntl.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

#ifndef TEST // kills all those assert macros in production version
#define NDEBUG
#endif

#define MAX_HIST           16        // number of pages we'll remember
#define ACTION_CALL         0        // values returned by help_topic()
#define ACTION_PREV         1
#define ACTION_PREV2        2        // special - go back two topics
#define ACTION_INDEX        3
#define ACTION_QUIT         4
#define F_HIST              (1 << 0)   // flags for help_topic()
#define F_INDEX             (1 << 1)
#define MAX_PAGE_SIZE       (80*25)  // no page of text may be larger
#define TEXT_START_ROW      2        // start print the help text here

struct LINK
{
    BYTE r, c;
    int           width;
    unsigned      offset;
    int           topic_num;
    unsigned      topic_off;
};

struct LABEL
{
    int      topic_num;
    unsigned topic_off;
};

struct PAGE
{
    unsigned      offset;
    unsigned      len;
    int           margin;
};

struct HIST
{
    int      topic_num;
    unsigned topic_off;
    int      link;
};

struct help_sig_info
{
    unsigned long sig;
    int           version;
    // cppcheck-suppress unusedStructMember
    unsigned long base;     // only if added to id.exe
};

void print_document(char const *outfname, bool (*msg_func)(int, int), int save_extraseg);
static bool print_doc_msg_func(int pnum, int num_pages);

// stuff from fractint

static std::FILE *help_file = nullptr;       // help file handle
static long base_off;                   // offset to help info in help file
static int max_links;                   // max # of links in any page
static int max_pages;                   // max # of pages in any topic
static int num_label;                   // number of labels
static int num_topic;                   // number of topics
static int curr_hist = 0;               // current pos in history

// these items alloc'ed in init_help...

static std::vector<long> topic_offset;  // 4*num_topic
static std::vector<LABEL> label;        // 4*num_label
static std::vector<HIST> hist;          // 6*MAX_HIST (96 bytes)

// these items alloc'ed only while help is active...

static std::vector<char> g_buffer;   // MAX_PAGE_SIZE (2048 bytes)
static std::vector<LINK> link_table; // 10*max_links
static std::vector<PAGE> page_table; // 4*max_pages

static void help_seek(long pos)
{
    std::fseek(help_file, base_off + pos, SEEK_SET);
}

static void displaycc(int row, int col, int color, int ch)
{
    char s[] = { (char) ch, 0 };
    driver_put_string(row, col, color, s);
}

static void display_text(int row, int col, int color, char const *text, unsigned len)
{
    while (len-- != 0)
    {
        if (*text == CMD_LITERAL)
        {
            ++text;
            --len;
        }
        displaycc(row, col++, color, *text++);
    }
}

static void display_parse_text(char const *text, unsigned len, int start_margin, int *num_link, LINK *link)
{
    char const *curr;
    int row, col;
    token_types tok;
    int size, width;

    g_text_cbase = SCREEN_INDENT;
    g_text_rbase = TEXT_START_ROW;

    curr = text;
    row = 0;
    col = 0;

    width = 0;
    size = width;

    if (start_margin >= 0)
    {
        tok = token_types::TOK_PARA;
    }
    else
    {
        tok = static_cast<token_types>(-1);
    }

    while (true)
    {
        switch (tok)
        {
        case token_types::TOK_PARA:
        {
            int indent, margin;

            if (size > 0)
            {
                ++curr;
                indent = *curr++;
                margin = *curr++;
                len  -= 3;
            }
            else
            {
                indent = start_margin;
                margin = start_margin;
            }

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
                    col = 0;   // fake a new-line
                    row++;
                    break;
                }

                if (tok == token_types::TOK_XONLINE || tok == token_types::TOK_XDOC)
                {
                    curr += size;
                    len  -= size;
                    continue;
                }

                // now tok is SPACE or LINK or WORD

                if (col+width > SCREEN_WIDTH)
                {
                    // go to next line...
                    col = margin;
                    ++row;

                    if (tok == token_types::TOK_SPACE)
                    {
                        width = 0;   // skip spaces at start of a line
                    }
                }

                if (tok == token_types::TOK_LINK)
                {
                    display_text(row, col, C_HELP_LINK, curr+1+3*sizeof(int), width);
                    if (num_link != nullptr)
                    {
                        link[*num_link].r         = (BYTE)row;
                        link[*num_link].c         = (BYTE)col;
                        link[*num_link].topic_num = getint(curr+1);
                        link[*num_link].topic_off = getint(curr+1+sizeof(int));
                        link[*num_link].offset    = (unsigned)((curr+1+3*sizeof(int)) - text);
                        link[*num_link].width     = width;
                        ++(*num_link);
                    }
                }
                else if (tok == token_types::TOK_WORD)
                {
                    display_text(row, col, C_HELP_BODY, curr, width);
                }

                col += width;
                curr += size;
                len -= size;
            }

            size = 0;
            width = size;
            break;
        }

        case token_types::TOK_CENTER:
            col = find_line_width(token_modes::ONLINE, curr, len);
            col = (SCREEN_WIDTH - col)/2;
            if (col < 0)
            {
                col = 0;
            }
            break;

        case token_types::TOK_NL:
            col = 0;
            ++row;
            break;

        case token_types::TOK_LINK:
            display_text(row, col, C_HELP_LINK, curr+1+3*sizeof(int), width);
            if (num_link != nullptr)
            {
                link[*num_link].r         = (BYTE)row;
                link[*num_link].c         = (BYTE)col;
                link[*num_link].topic_num = getint(curr+1);
                link[*num_link].topic_off = getint(curr+1+sizeof(int));
                link[*num_link].offset    = (unsigned)((curr+1+3*sizeof(int)) - text);
                link[*num_link].width     = width;
                ++(*num_link);
            }
            break;

        case token_types::TOK_XONLINE:  // skip
        case token_types::TOK_FF:       // ignore
        case token_types::TOK_XDOC:     // ignore
        case token_types::TOK_DONE:
        case token_types::TOK_SPACE:
            break;

        case token_types::TOK_WORD:
            display_text(row, col, C_HELP_BODY, curr, width);
            break;
        } // switch

        curr += size;
        len  -= size;
        col  += width;

        if (len == 0)
        {
            break;
        }

        tok = find_token_length(token_modes::ONLINE, curr, len, &size, &width);
    } // while (true)

    g_text_cbase = 0;
    g_text_rbase = 0;
}

static void color_link(LINK *link, int color)
{
    g_text_cbase = SCREEN_INDENT;
    g_text_rbase = TEXT_START_ROW;

    driver_set_attr(link->r, link->c, color, link->width);

    g_text_cbase = 0;
    g_text_rbase = 0;
}

#if defined(_WIN32)
#define PUT_KEY(name_, desc_) put_key(name_, desc_)
#else
#if !defined(XFRACT)
#define PUT_KEY(name, descrip)                              \
    driver_put_string(-1, -1, C_HELP_INSTR, name),             \
    driver_put_string(-1, -1, C_HELP_INSTR, ":" descrip "  ")
#else
#define PUT_KEY(name, descrip)                      \
    driver_put_string(-1, -1, C_HELP_INSTR, name);     \
    driver_put_string(-1, -1, C_HELP_INSTR, ":");      \
    driver_put_string(-1, -1, C_HELP_INSTR, descrip);  \
    driver_put_string(-1, -1, C_HELP_INSTR, "  ")
#endif
#endif

static void put_key(char const *name, char const *descrip)
{
    driver_put_string(-1, -1, C_HELP_INSTR, name);
    driver_put_string(-1, -1, C_HELP_INSTR, ":");
    driver_put_string(-1, -1, C_HELP_INSTR, descrip);
    driver_put_string(-1, -1, C_HELP_INSTR, "  ");
}

static void helpinstr()
{
    for (int ctr = 0; ctr < 80; ctr++)
    {
        driver_put_string(24, ctr, C_HELP_INSTR, " ");
    }

    driver_move_cursor(24, 1);
    PUT_KEY("F1",               "Index");
#if !defined(XFRACT) && !defined(_WIN32)
    PUT_KEY("\030\031\033\032", "Select");
#else
    PUT_KEY("K J H L", "Select");
#endif
    PUT_KEY("Enter",            "Go to");
    PUT_KEY("Backspace",        "Last topic");
    PUT_KEY("Escape",           "Exit help");
}

static void printinstr()
{
    for (int ctr = 0; ctr < 80; ctr++)
    {
        driver_put_string(24, ctr, C_HELP_INSTR, " ");
    }

    driver_move_cursor(24, 1);
    PUT_KEY("Escape", "Abort");
}

#undef PUT_KEY

static void display_page(char const *title, char const *text, unsigned text_len,
                         int page, int num_pages, int start_margin,
                         int *num_link, LINK *link)
{
    char temp[9];

    helptitle();
    helpinstr();
    driver_set_attr(2, 0, C_HELP_BODY, 80*22);
    putstringcenter(1, 0, 80, C_HELP_HDG, title);
    std::snprintf(temp, NUM_OF(temp), "%2d of %d", page+1, num_pages);
    driver_put_string(1, 79-(6 + ((num_pages >= 10)?2:1)), C_HELP_INSTR, temp);

    if (text != nullptr)
    {
        display_parse_text(text, text_len, start_margin, num_link, link);
    }

    driver_hide_text_cursor();
}

/*
 * int overlap(int a, int a2, int b, int b2);
 *
 * If a, a2, b, and b2 are points on a line, this function returns the
 * distance of intersection between a-->a2 and b-->b2.  If there is no
 * intersection between the lines this function will return a negative number
 * representing the distance between the two lines.
 *
 * There are six possible cases of intersection between the lines:
 *
 *                      a                     a2
 *                      |                     |
 *     b         b2     |                     |       b         b2
 *     |---(1)---|      |                     |       |---(2)---|
 *                      |                     |
 *              b       |     b2      b       |      b2
 *              |------(3)----|       |------(4)-----|
 *                      |                     |
 *               b      |                     |   b2
 *               |------+--------(5)----------+---|
 *                      |                     |
 *                      |     b       b2      |
 *                      |     |--(6)--|       |
 *                      |                     |
 *                      |                     |
 *
 */

static int overlap(int a, int a2, int b, int b2)
{
    if (b < a)
    {
        if (b2 >= a2)
        {
            return a2 - a;            // case (5)
        }

        return b2 - a;               // case (1), case (3)
    }

    if (b2 <= a2)
    {
        return b2 - b;               // case (6)
    }

    return a2 - b;                  // case (2), case (4)
}

static int dist1(int a, int b)
{
    int t = a - b;

    return std::abs(t);
}

static int find_link_updown(LINK *link, int num_link, int curr_link, int up)
{
    int curr_c2, best_overlap = 0, temp_overlap;
    LINK *curr, *best;
    int temp_dist;

    curr    = &link[curr_link];
    best    = nullptr;
    curr_c2 = curr->c + curr->width - 1;

    LINK *temp = link;
    for (int ctr = 0; ctr < num_link; ctr++, temp++)
    {
        if (ctr != curr_link
            && ((up && temp->r < curr->r) || (!up && temp->r > curr->r)))
        {
            temp_overlap = overlap(curr->c, curr_c2, temp->c, temp->c+temp->width-1);
            // if >= 3 lines between, prioritize on vertical distance:
            temp_dist = dist1(temp->r, curr->r);
            if (temp_dist >= 4)
            {
                temp_overlap -= temp_dist*100;
            }

            if (best != nullptr)
            {
                if (best_overlap >= 0 && temp_overlap >= 0)
                {
                    // if they're both under curr set to closest in y dir
                    if (dist1(best->r, curr->r) > temp_dist)
                    {
                        best = nullptr;
                    }
                }
                else
                {
                    if (best_overlap < temp_overlap)
                    {
                        best = nullptr;
                    }
                }
            }

            if (best == nullptr)
            {
                best = temp;
                best_overlap = temp_overlap;
            }
        }
    }

    return (best == nullptr) ? -1 : (int)(best-link);
}

static int find_link_leftright(LINK *link, int num_link, int curr_link, int left)
{
    int curr_c2, best_c2 = 0, temp_c2, best_dist = 0, temp_dist;
    LINK *curr, *best;

    curr    = &link[curr_link];
    best    = nullptr;
    curr_c2 = curr->c + curr->width - 1;

    LINK *temp = link;
    for (int ctr = 0; ctr < num_link; ctr++, temp++)
    {
        temp_c2 = temp->c + temp->width - 1;

        if (ctr != curr_link
            && ((left && temp_c2 < (int) curr->c) || (!left && (int) temp->c > curr_c2)))
        {
            temp_dist = dist1(curr->r, temp->r);

            if (best != nullptr)
            {
                if (best_dist == 0 && temp_dist == 0)  // if both on curr's line...
                {
                    if ((left && dist1(curr->c, best_c2) > dist1(curr->c, temp_c2))
                        || (!left && dist1(curr_c2, best->c) > dist1(curr_c2, temp->c)))
                    {
                        best = nullptr;
                    }
                }
                else if (best_dist >= temp_dist)   // if temp is closer...
                {
                    best = nullptr;
                }
            }
            else
            {
                best      = temp;
                best_dist = temp_dist;
                best_c2   = temp_c2;
            }
        }
    } // for

    return (best == nullptr) ? -1 : (int)(best-link);
}

static int find_link_key(LINK * /*link*/, int num_link, int curr_link, int key)
{
    switch (key)
    {
    case FIK_TAB:
        return (curr_link >= num_link-1) ? -1 : curr_link+1;
    case FIK_SHF_TAB:
        return (curr_link <= 0)          ? -1 : curr_link-1;
    default:
        assert(0);
        return -1;
    }
}

static int do_move_link(LINK *link, int num_link, int *curr, int (*f)(LINK *, int, int, int), int val)
{
    if (num_link > 1)
    {
        int t;
        if (f == nullptr)
        {
            t = val;
        }
        else
        {
            t = (*f)(link, num_link, *curr, val);
        }

        if (t >= 0 && t != *curr)
        {
            color_link(&link[*curr], C_HELP_LINK);
            *curr = t;
            color_link(&link[*curr], C_HELP_CURLINK);
            return 1;
        }
    }

    return 0;
}

inline void freader(void *ptr, size_t size, size_t nmemb, std::FILE *stream)
{
    if (std::fread(ptr, size, nmemb, stream) != nmemb)
    {
        throw std::system_error(errno, std::system_category(), "failed fread");
    }
}


static int help_topic(HIST *curr, HIST *next, int flags)
{
    int       len;
    int       key;
    int       num_pages;
    int       num_link;
    int       curr_link;
    char      title[81];
    long      where;
    int       draw_page;
    int       action;
    BYTE ch;

    where     = topic_offset[curr->topic_num]+sizeof(int); // to skip flags
    curr_link = curr->link;

    help_seek(where);

    freader(&num_pages, sizeof(int), 1, help_file);
    assert(num_pages > 0 && num_pages <= max_pages);

    freader(&page_table[0], 3*sizeof(int), num_pages, help_file);

    freader(&ch, sizeof(char), 1, help_file);
    len = ch;
    assert(len < 81);
    freader(title, sizeof(char), len, help_file);
    title[len] = '\0';

    where += sizeof(int) + num_pages*3*sizeof(int) + 1 + len + sizeof(int);

    int page;
    for (page = 0; page < num_pages; page++)
    {
        if (curr->topic_off >= page_table[page].offset
            && curr->topic_off <  page_table[page].offset+page_table[page].len)
        {
            break;
        }
    }

    assert(page < num_pages);

    action = -1;
    draw_page = 2;

    do
    {
        if (draw_page)
        {
            help_seek(where+page_table[page].offset);
            freader(&g_buffer[0], sizeof(char), page_table[page].len, help_file);

            num_link = 0;
            display_page(title, &g_buffer[0], page_table[page].len, page, num_pages,
                         page_table[page].margin, &num_link, &link_table[0]);

            if (draw_page == 2)
            {
                assert(num_link <= 0 || (curr_link >= 0 && curr_link < num_link));
            }
            else if (draw_page == 3)
            {
                curr_link = num_link - 1;
            }
            else
            {
                curr_link = 0;
            }

            if (num_link > 0)
            {
                color_link(&link_table[curr_link], C_HELP_CURLINK);
            }

            draw_page = 0;
        }

        key = driver_get_key();

        switch (key)
        {
        case FIK_PAGE_DOWN:
            if (page < num_pages-1)
            {
                page++;
                draw_page = 1;
            }
            break;

        case FIK_PAGE_UP:
            if (page > 0)
            {
                page--;
                draw_page = 1;
            }
            break;

        case FIK_HOME:
            if (page != 0)
            {
                page = 0;
                draw_page = 1;
            }
            else
            {
                do_move_link(&link_table[0], num_link, &curr_link, nullptr, 0);
            }
            break;

        case FIK_END:
            if (page != num_pages-1)
            {
                page = num_pages-1;
                draw_page = 3;
            }
            else
            {
                do_move_link(&link_table[0], num_link, &curr_link, nullptr, num_link-1);
            }
            break;

        case FIK_TAB:
            if (!do_move_link(&link_table[0], num_link, &curr_link, find_link_key, key)
                && page < num_pages-1)
            {
                ++page;
                draw_page = 1;
            }
            break;

        case FIK_SHF_TAB:
            if (!do_move_link(&link_table[0], num_link, &curr_link, find_link_key, key)
                && page > 0)
            {
                --page;
                draw_page = 3;
            }
            break;

        case FIK_DOWN_ARROW:
            if (!do_move_link(&link_table[0], num_link, &curr_link, find_link_updown, 0)
                && page < num_pages-1)
            {
                ++page;
                draw_page = 1;
            }
            break;

        case FIK_UP_ARROW:
            if (!do_move_link(&link_table[0], num_link, &curr_link, find_link_updown, 1)
                && page > 0)
            {
                --page;
                draw_page = 3;
            }
            break;

        case FIK_LEFT_ARROW:
            do_move_link(&link_table[0], num_link, &curr_link, find_link_leftright, 1);
            break;

        case FIK_RIGHT_ARROW:
            do_move_link(&link_table[0], num_link, &curr_link, find_link_leftright, 0);
            break;

        case FIK_ESC:         // exit help
            action = ACTION_QUIT;
            break;

        case FIK_BACKSPACE:   // prev topic
        case FIK_ALT_F1:
            if (flags & F_HIST)
            {
                action = ACTION_PREV;
            }
            break;

        case FIK_F1:    // help index
            if (!(flags & F_INDEX))
            {
                action = ACTION_INDEX;
            }
            break;

        case FIK_ENTER:
        case FIK_ENTER_2:
            if (num_link > 0)
            {
                next->topic_num = link_table[curr_link].topic_num;
                next->topic_off = link_table[curr_link].topic_off;
                action = ACTION_CALL;
            }
            break;
        } // switch
    }
    while (action == -1);

    curr->topic_off = page_table[page].offset;
    curr->link      = curr_link;

    return action;
}

int help(int action)
{
    HIST      curr = { -1 };
    int old_look_at_mouse;
    int       flags;
    HIST      next;

    if (g_help_mode == help_labels::NONE)   // is help disabled?
    {
        return 0;
    }

    if (help_file == nullptr)
    {
        driver_buzzer(buzzer_codes::PROBLEM);
        return 0;
    }

    bool resized = false;
    try
    {
        g_buffer.resize(MAX_PAGE_SIZE);
        link_table.resize(max_links);
        page_table.resize(max_pages);
        resized = true;
    }
    catch (std::bad_alloc const&)
    {
    }
    if (!resized)
    {
        driver_buzzer(buzzer_codes::PROBLEM);
        return 0;
    }

    old_look_at_mouse = g_look_at_mouse;
    g_look_at_mouse = 0;
    g_timer_start -= std::clock();
    driver_stack_screen();

    if (g_help_mode >= help_labels::IDHELP_INDEX)
    {
        next.topic_num = label[static_cast<int>(g_help_mode)].topic_num;
        next.topic_off = label[static_cast<int>(g_help_mode)].topic_off;
    }
    else
    {
        next.topic_num = static_cast<int>(g_help_mode);
        next.topic_off = 0;
    }

    help_labels const old_help_mode = g_help_mode;

    if (curr_hist <= 0)
    {
        action = ACTION_CALL;  // make sure it isn't ACTION_PREV!
    }

    do
    {
        switch (action)
        {
        case ACTION_PREV2:
            if (curr_hist > 0)
            {
                curr = hist[--curr_hist];
            }
            // fall-through

        case ACTION_PREV:
            if (curr_hist > 0)
            {
                curr = hist[--curr_hist];
            }
            break;

        case ACTION_QUIT:
            break;

        case ACTION_INDEX:
            next.topic_num = label[static_cast<int>(help_labels::IDHELP_INDEX)].topic_num;
            next.topic_off = label[static_cast<int>(help_labels::IDHELP_INDEX)].topic_off;
            // fall-through

        case ACTION_CALL:
            curr = next;
            curr.link = 0;
            break;
        } // switch

        flags = 0;
        if (curr.topic_num == label[static_cast<int>(help_labels::IDHELP_INDEX)].topic_num)
        {
            flags |= F_INDEX;
        }
        if (curr_hist > 0)
        {
            flags |= F_HIST;
        }

        if (curr.topic_num >= 0)
        {
            action = help_topic(&curr, &next, flags);
        }
        else
        {
            if (curr.topic_num == -100)
            {
                print_document("id.txt", print_doc_msg_func, 1);
                action = ACTION_PREV2;
            }
            else if (curr.topic_num == -101)
            {
                action = ACTION_PREV2;
            }
            else
            {
                display_page("Unknown Help Topic", nullptr, 0, 0, 1, 0, nullptr, nullptr);
                action = -1;
                while (action == -1)
                {
                    switch (driver_get_key())
                    {
                    case FIK_ESC:
                        action = ACTION_QUIT;
                        break;
                    case FIK_ALT_F1:
                        action = ACTION_PREV;
                        break;
                    case FIK_F1:
                        action = ACTION_INDEX;
                        break;
                    } // switch
                } // while
            }
        } // else

        if (action != ACTION_PREV && action != ACTION_PREV2)
        {
            if (curr_hist >= MAX_HIST)
            {
                for (int ctr = 0; ctr < MAX_HIST-1; ctr++)
                {
                    hist[ctr] = hist[ctr+1];
                }

                curr_hist = MAX_HIST-1;
            }
            hist[curr_hist++] = curr;
        }
    }
    while (action != ACTION_QUIT);

    driver_unstack_screen();
    g_look_at_mouse = old_look_at_mouse;
    g_help_mode = old_help_mode;
    g_timer_start += std::clock();

    return 0;
}

static bool can_read_file(const std::string &path)
{
    int handle = open(path.c_str(), O_RDONLY);
    if (handle != -1)
    {
        close(handle);
        return true;
    }
    return false;
}


static std::string find_file(char const *filename)
{
    const std::string path{(fs::path(SRCDIR) / filename).string()};
    if (can_read_file(path))
    {
        return path;
    }
    return find_path(filename);
}

static int _read_help_topic(int topic, int off, int len, void *buf)
{
    static int  curr_topic = -1;
    static long curr_base;
    static int  curr_len;
    int         read_len;

    if (topic != curr_topic)
    {
        int t;
        char ch;

        curr_topic = topic;

        curr_base = topic_offset[topic];

        curr_base += sizeof(int);                 // skip flags

        help_seek(curr_base);
        freader(&t, sizeof(int), 1, help_file); // read num_pages
        curr_base += sizeof(int) + t*3*sizeof(int); // skip page info

        if (t > 0)
        {
            help_seek(curr_base);
        }
        freader(&ch, sizeof(char), 1, help_file);   // read title_len
        t = ch;
        curr_base += 1 + t;                       // skip title

        if (t > 0)
        {
            help_seek(curr_base);
        }
        freader(&curr_len, sizeof(int), 1, help_file); // read topic len
        curr_base += sizeof(int);
    }

    read_len = (off+len > curr_len) ? curr_len - off : len;

    if (read_len > 0)
    {
        help_seek(curr_base + off);
        freader(buf, sizeof(char), read_len, help_file);
    }

    return curr_len - (off+len);
}

int read_help_topic(help_labels label_num, int off, int len, void *buf)
/*
 * reads text from a help topic.  Returns number of bytes from (off+len)
 * to end of topic.  On "EOF" returns a negative number representing
 * number of bytes not read.
 */
{
    int ret;
    ret = _read_help_topic(label[static_cast<int>(label_num)].topic_num,
                           label[static_cast<int>(label_num)].topic_off + off, len, buf);
    return ret;
}

#define PRINT_BUFFER_SIZE  (32767)       // max. size of help topic in doc.
#define TEMP_FILE_NAME     "HELP.$$$"    // temp file for storing extraseg
//    while printing document
#define MAX_NUM_TOPIC_SEC  (10)          // max. number of topics under any
//    single section (CONTENT)

struct PRINT_DOC_INFO
{
    int       cnum;          // current CONTENT num
    int       tnum;          // current topic num

    long      content_pos;   // current CONTENT item offset in file
    int       num_page;      // total number of pages in document

    int       num_contents,  // total number of CONTENT entries
              num_topic;     // number of topics in current CONTENT

    int       topic_num[MAX_NUM_TOPIC_SEC]; // topic_num[] for current CONTENT entry

    char buffer[PRINT_BUFFER_SIZE];        // text buffer

    char      id[81];        // buffer to store id in
    char      title[81];     // buffer to store title in

    bool (*msg_func)(int pnum, int num_page);

    std::FILE     *file;          // file to sent output to
    int       margin;        // indent text by this much
    bool      start_of_line; // are we at the beginning of a line?
    int       spaces;        // number of spaces in a row
};

static void printerc(PRINT_DOC_INFO *info, int c, int n)
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
            std::fputc(c, info->file);
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

static void printers(PRINT_DOC_INFO *info, char const *s, int n)
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

static bool print_doc_get_info(int cmd, PD_INFO *pd, void *context)
{
    PRINT_DOC_INFO *info = static_cast<PRINT_DOC_INFO *>(context);
    int t;
    BYTE ch;

    switch (cmd)
    {
    case PD_GET_CONTENT:
        if (++info->cnum >= info->num_contents)
        {
            return false;
        }

        help_seek(info->content_pos);

        freader(&t, sizeof(int), 1, help_file);      // read flags
        info->content_pos += sizeof(int);
        pd->new_page = (t & 1) != 0;

        freader(&ch, sizeof(char), 1, help_file);       // read id len

        t = ch;
        assert(t < 80);
        freader(info->id, sizeof(char), t, help_file);  // read the id
        info->content_pos += 1 + t;
        info->id[t] = '\0';

        freader(&ch, sizeof(char), 1, help_file);       // read title len
        t = ch;
        assert(t < 80);
        freader(info->title, sizeof(char), t, help_file); // read the title
        info->content_pos += 1 + t;
        info->title[t] = '\0';

        freader(&ch, sizeof(char), 1, help_file);       // read num_topic
        t = ch;
        assert(t < MAX_NUM_TOPIC_SEC);
        freader(info->topic_num, sizeof(int), t, help_file);  // read topic_num[]
        info->num_topic = t;
        info->content_pos += 1 + t*sizeof(int);

        info->tnum = -1;

        pd->id = info->id;
        pd->title = info->title;
        return true;

    case PD_GET_TOPIC:
        if (++info->tnum >= info->num_topic)
        {
            return false;
        }

        t = _read_help_topic(info->topic_num[info->tnum], 0, PRINT_BUFFER_SIZE, info->buffer);

        assert(t <= 0);

        pd->curr = info->buffer;
        pd->len  = PRINT_BUFFER_SIZE + t;   // same as ...SIZE - abs(t)
        return true;

    case PD_GET_LINK_PAGE:
        pd->i = getint(pd->s+sizeof(long));
        return pd->i != -1;

    case PD_RELEASE_TOPIC:
        return true;

    default:
        return false;
    }
}

static bool print_doc_output(int cmd, PD_INFO *pd, void *context)
{
    PRINT_DOC_INFO *info = static_cast<PRINT_DOC_INFO *>(context);
    switch (cmd)
    {
    case PD_HEADING:
    {
        char line[81];
        char buff[40];
        int  width = PAGE_WIDTH + PAGE_INDENT;
        bool keep_going;

        if (info->msg_func != nullptr)
        {
            keep_going = (*info->msg_func)(pd->page_num, info->num_page) != 0;
        }
        else
        {
            keep_going = true;
        }

        info->margin = 0;

        std::memset(line, ' ', 81);
        std::snprintf(buff, NUM_OF(buff), "Iterated Dynamics Version %d.%01d%c", g_release/100, (g_release%100)/10,
                ((g_release%10) ? '0'+(g_release%10) : ' '));
        std::memmove(line + ((width-(int)(std::strlen(buff))) / 2)-4, buff, std::strlen(buff));

        std::snprintf(buff, NUM_OF(buff), "Page %d", pd->page_num);
        std::memmove(line + (width - (int)std::strlen(buff)), buff, std::strlen(buff));

        printerc(info, '\n', 1);
        printers(info, line, width);
        printerc(info, '\n', 2);

        info->margin = PAGE_INDENT;

        return keep_going;
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

static bool print_doc_msg_func(int pnum, int num_pages)
{
    char temp[10];
    int  key;

    if (pnum == -1)      // successful completion
    {
        driver_buzzer(buzzer_codes::COMPLETE);
        putstringcenter(7, 0, 80, C_HELP_LINK, "Done -- Press any key");
        driver_get_key();
        return false;
    }

    if (pnum == -2)     // aborted
    {
        driver_buzzer(buzzer_codes::INTERRUPT);
        putstringcenter(7, 0, 80, C_HELP_LINK, "Aborted -- Press any key");
        driver_get_key();
        return false;
    }

    if (pnum == 0)   // initialization
    {
        helptitle();
        printinstr();
        driver_set_attr(2, 0, C_HELP_BODY, 80*22);
        putstringcenter(1, 0, 80, C_HELP_HDG, "Generating id.txt");

        driver_put_string(7, 30, C_HELP_BODY, "Completed:");

        driver_hide_text_cursor();
    }

    std::snprintf(temp, NUM_OF(temp), "%d%%", (int)((100.0 / num_pages) * pnum));
    driver_put_string(7, 41, C_HELP_LINK, temp);

    while (driver_key_pressed())
    {
        key = driver_get_key();
        if (key == FIK_ESC)
        {
            return false;    // user abort
        }
    }

    return true;   // AOK -- continue
}

bool makedoc_msg_func(int pnum, int num_pages)
{
    char buffer[80] = "";
    bool result = false;

    if (pnum >= 0)
    {
        std::snprintf(buffer, NUM_OF(buffer), "\rcompleted %d%%", (int)((100.0 / num_pages) * pnum));
        result = true;
    }
    else if (pnum == -2)
    {
        std::snprintf(buffer, NUM_OF(buffer), "\n*** aborted\n");
    }
    stopmsg(STOPMSG_NONE, buffer);
    return result;
}

void print_document(char const *outfname, bool (*msg_func)(int, int), int save_extraseg)
{
    PRINT_DOC_INFO info;
    bool success = false;
    std::FILE *temp_file = nullptr;
    char const *msg = nullptr;

    help_seek(16L);
    freader(&info.num_contents, sizeof(int), 1, help_file);
    freader(&info.num_page, sizeof(int), 1, help_file);

    info.tnum = -1;
    info.cnum = info.tnum;
    info.content_pos = 6*sizeof(int) + num_topic*sizeof(long) + num_label*2*sizeof(int);
    info.msg_func = msg_func;

    if (msg_func != nullptr)
    {
        msg_func(0, info.num_page);   // initialize
    }

    if (save_extraseg)
    {
        temp_file = std::fopen(TEMP_FILE_NAME, "wb");
        if (temp_file == nullptr)
        {
            msg = "Unable to create temporary file.\n";
            goto ErrorAbort;
        }

        if (std::fwrite(info.buffer, sizeof(char), PRINT_BUFFER_SIZE, temp_file) != PRINT_BUFFER_SIZE)
        {
            msg = "Error writing temporary file.\n";
            goto ErrorAbort;
        }
    }

    info.file = std::fopen(outfname, "wt");
    if (info.file == nullptr)
    {
        msg = "Unable to create output file.\n";
        goto ErrorAbort;
    }

    info.margin = PAGE_INDENT;
    info.start_of_line = true;
    info.spaces = 0;

    success = process_document(print_doc_get_info,
                               print_doc_output,   &info);
    std::fclose(info.file);

    if (save_extraseg)
    {
        if (std::fseek(temp_file, 0L, SEEK_SET) != 0L)
        {
            msg = "Error reading temporary file.\nSystem may be corrupt!\nSave your image and re-start id!\n";
            goto ErrorAbort;
        }

        if (std::fread(info.buffer, sizeof(char), PRINT_BUFFER_SIZE, temp_file) != PRINT_BUFFER_SIZE)
        {
            msg = "Error reading temporary file.\nSystem may be corrupt!\nSave your image and re-start id!\n";
            goto ErrorAbort;
        }
    }

ErrorAbort:
    if (temp_file != nullptr)
    {
        std::fclose(temp_file);
        std::remove(TEMP_FILE_NAME);
        temp_file = nullptr;
    }

    if (msg != nullptr)
    {
        helptitle();
        stopmsg(STOPMSG_NO_STACK, msg);
    }

    else if (msg_func != nullptr)
    {
        msg_func(success ? -1 : -2, info.num_page);
    }
}

int init_help()
{
    help_sig_info hs = { 0 };

    help_file = nullptr;

    const std::string path{find_file("id.hlp")};
    if (!path.empty())
    {
        help_file = std::fopen(path.c_str(), "rb");
        if (help_file != nullptr)
        {
            freader(&hs, sizeof(long)+sizeof(int), 1, help_file);

            if (hs.sig != HELP_SIG)
            {
                std::fclose(help_file);
                stopmsg(STOPMSG_NO_STACK, "Invalid help signature in id.hlp!\n");
            }
            else if (hs.version != IDHELP_VERSION)
            {
                std::fclose(help_file);
                stopmsg(STOPMSG_NO_STACK, "Wrong help version in id.hlp!\n");
            }
            else
            {
                base_off = sizeof(long)+sizeof(int);
            }
        }
    }

    if (help_file == nullptr)         // Can't find the help files anywhere!
    {
        static char msg[] =
#if !defined(XFRACT) && !defined(_WIN32)
            {"Help Files aren't in id.exe, and couldn't find id.hlp!\n"};
#else
            {"Couldn't find id.hlp; set FRACTDIR to proper directory with setenv.\n"};
#endif
        stopmsg(STOPMSG_NO_STACK, msg);
    }

    help_seek(0L);

    freader(&max_pages, sizeof(int), 1, help_file);
    freader(&max_links, sizeof(int), 1, help_file);
    freader(&num_topic, sizeof(int), 1, help_file);
    freader(&num_label, sizeof(int), 1, help_file);
    help_seek(6L*sizeof(int));  // skip num_contents and num_doc_pages

    assert(max_pages > 0);
    assert(max_links >= 0);
    assert(num_topic > 0);
    assert(num_label > 0);

    // allocate all three arrays
    bool resized = false;
    try
    {
        topic_offset.resize(num_topic);
        label.resize(num_label);
        hist.resize(MAX_HIST);
        resized = true;
    }
    catch (std::bad_alloc const&)
    {
    }

    if (!resized)
    {
        std::fclose(help_file);
        help_file = nullptr;
        stopmsg(STOPMSG_NO_STACK, "Not enough memory for help system!\n");

        return -2;
    }

    // read in the tables...
    freader(&topic_offset[0], sizeof(long), num_topic, help_file);
    freader(&label[0], sizeof(LABEL), num_label, help_file);

    // finished!

    return 0;  // success
}

void end_help()
{
    if (help_file != nullptr)
    {
        std::fclose(help_file);
        help_file = nullptr;
    }
}
