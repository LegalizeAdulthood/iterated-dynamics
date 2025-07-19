// SPDX-License-Identifier: GPL-3.0-only
//
#include "helpcom.h"

#include "engine/cmdfiles.h"
#include "engine/engine_timer.h"
#include "engine/id_data.h"
#include "helpdefs.h"
#include "io/find_path.h"
#include "io/save_file.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/help_title.h"
#include "ui/id_keys.h"
#include "ui/mouse.h"
#include "ui/put_string_center.h"
#include "ui/stop_msg.h"
#include "ui/text_screen.h"

#include <config/fdio.h>
#include <config/filelength.h>
#include <config/home_dir.h>
#include <config/port.h>

#include <fmt/format.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <filesystem>
#include <new>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

enum
{
    MAX_HIST = 16,             // number of pages we'll remember
    ACTION_CALL = 0,           // values returned by help_topic()
    ACTION_PREV = 1,           //
    ACTION_PREV2 = 2,          // special - go back two topics
    ACTION_INDEX = 3,          //
    ACTION_QUIT = 4,           //
    F_HIST = 1 << 0,           // flags for help_topic()
    F_INDEX = 1 << 1,          //
    MAX_PAGE_SIZE = 80 * 25,   // no page of text may be larger
    TEXT_START_ROW = 2,        // start print the help text here
    PRINT_BUFFER_SIZE = 32767, // max. size of help topic in doc.
    MAX_NUM_TOPIC_SEC = 10,    // max. number of topics under any single section (Content)
};

namespace
{

struct Link
{
    Byte r, c;
    int           width;
    unsigned      offset;
    int           topic_num;
    unsigned      topic_off;
};

struct Label
{
    int      topic_num;
    unsigned topic_off;
};

struct Page
{
    unsigned      offset;
    unsigned      len;
    int           margin;
};

struct History
{
    int      topic_num;
    unsigned topic_off;
    int      link;
};

using PrintDocMessageFunc = bool(int page_num, int num_pages);

struct PrintDocInfo
{
    int current_content;              // current Content num
    int current_topic;                // current topic num
    long content_pos;                 // current Content item offset in file
    int num_page;                     // total number of pages in document
    int num_contents;                 // total number of Content entries
    int num_topic;                    // number of topics in current Content
    int topic_num[MAX_NUM_TOPIC_SEC]; // topic_num[] for current Content entry
    char buffer[PRINT_BUFFER_SIZE];   // text buffer
    char id[81];                      // buffer to store id in
    char title[81];                   // buffer to store title in
    PrintDocMessageFunc *msg_func;    //
    std::FILE *file;                  // file to sent output to
    int margin;                       // indent text by this much
    bool start_of_line;               // are we at the beginning of a line?
    int spaces;                       // number of spaces in a row
};

} // namespace

static std::FILE *s_help_file{};         // help file handle
static long s_base_off{};                // offset to help info in help file
static int s_max_links{};                // max # of links in any page
static int s_max_pages{};                // max # of pages in any topic
static int s_num_label{};                // number of labels
static int s_num_topic{};                // number of topics
static int s_curr_hist{};                // current pos in history
                                         // these items setup in init_help...
static std::vector<long> s_topic_offset; // 4*num_topic
static std::vector<Label> s_label;       // 4*num_label
static std::vector<History> s_hist;      // 6*MAX_HIST
                                         // these items used only while help is active...
static std::vector<char> s_buffer;       // MAX_PAGE_SIZE
static std::vector<Link> s_link_table;   // 10*max_links
static std::vector<Page> s_page_table;   // 4*max_pages

// forward declarations
static bool print_doc_msg_func(int page_num, int num_pages);

static void help_seek(long pos)
{
    std::fseek(s_help_file, s_base_off + pos, SEEK_SET);
}

static void display_cc(int row, int col, int color, int ch)
{
    char s[] = { (char) ch, 0 };
    driver_put_string(row, col, color, s);
}

static void display_text(int row, int col, int color, const char *text, unsigned len)
{
    while (len-- != 0)
    {
        if (*text == CMD_LITERAL)
        {
            ++text;
            --len;
        }
        display_cc(row, col++, color, *text++);
    }
}

static void display_parse_text(const char *text, unsigned len, int start_margin, int *num_link, Link *link)
{
    TokenType tok;
    int size;
    int width;

    g_text_col_base = SCREEN_INDENT;
    g_text_row_base = TEXT_START_ROW;

    const char *curr = text;
    int row = 0;
    int col = 0;

    width = 0;
    size = 0;

    if (start_margin >= 0)
    {
        tok = TokenType::TOK_PARA;
    }
    else
    {
        tok = static_cast<TokenType>(-1);
    }

    while (true)
    {
        switch (tok)
        {
        case TokenType::TOK_PARA:
        {
            int indent;
            int margin;

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
                tok = find_token_length(TokenMode::ONLINE, curr, len, &size, &width);

                if (tok == TokenType::TOK_DONE || tok == TokenType::TOK_NL || tok == TokenType::TOK_FF)
                {
                    break;
                }

                if (tok == TokenType::TOK_PARA)
                {
                    col = 0;   // fake a new-line
                    row++;
                    break;
                }

                if (tok == TokenType::TOK_XONLINE || tok == TokenType::TOK_XDOC)
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

                    if (tok == TokenType::TOK_SPACE)
                    {
                        width = 0;   // skip spaces at start of a line
                    }
                }

                if (tok == TokenType::TOK_LINK)
                {
                    display_text(row, col, C_HELP_LINK, curr+1+3*sizeof(int), width);
                    if (num_link != nullptr)
                    {
                        link[*num_link].r         = (Byte)row;
                        link[*num_link].c         = (Byte)col;
                        link[*num_link].topic_num = get_int(curr+1);
                        link[*num_link].topic_off = get_int(curr+1+sizeof(int));
                        link[*num_link].offset    = (unsigned)((curr+1+3*sizeof(int)) - text);
                        link[*num_link].width     = width;
                        ++(*num_link);
                    }
                }
                else if (tok == TokenType::TOK_WORD)
                {
                    display_text(row, col, C_HELP_BODY, curr, width);
                }

                col += width;
                curr += size;
                len -= size;
            }

            size = 0;
            width = 0;
            break;
        }

        case TokenType::TOK_CENTER:
            col = find_line_width(TokenMode::ONLINE, curr, len);
            col = (SCREEN_WIDTH - col)/2;
            col = std::max(col, 0);
            break;

        case TokenType::TOK_NL:
            col = 0;
            ++row;
            break;

        case TokenType::TOK_LINK:
            display_text(row, col, C_HELP_LINK, curr+1+3*sizeof(int), width);
            if (num_link != nullptr)
            {
                link[*num_link].r         = (Byte)row;
                link[*num_link].c         = (Byte)col;
                link[*num_link].topic_num = get_int(curr+1);
                link[*num_link].topic_off = get_int(curr+1+sizeof(int));
                link[*num_link].offset    = (unsigned)((curr+1+3*sizeof(int)) - text);
                link[*num_link].width     = width;
                ++(*num_link);
            }
            break;

        case TokenType::TOK_XONLINE:  // skip
        case TokenType::TOK_FF:       // ignore
        case TokenType::TOK_XDOC:     // ignore
        case TokenType::TOK_DONE:
        case TokenType::TOK_SPACE:
            break;

        case TokenType::TOK_WORD:
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

        tok = find_token_length(TokenMode::ONLINE, curr, len, &size, &width);
    } // while (true)

    g_text_col_base = 0;
    g_text_row_base = 0;
}

static void color_link(Link *link, int color)
{
    g_text_col_base = SCREEN_INDENT;
    g_text_row_base = TEXT_START_ROW;

    driver_set_attr(link->r, link->c, color, link->width);

    g_text_col_base = 0;
    g_text_row_base = 0;
}

static void put_key(const char *name, const char *description)
{
    driver_put_string(-1, -1, C_HELP_INSTR, name);
    driver_put_string(-1, -1, C_HELP_INSTR, ":");
    driver_put_string(-1, -1, C_HELP_INSTR, description);
    driver_put_string(-1, -1, C_HELP_INSTR, "  ");
}

static void help_instr()
{
    for (int ctr = 0; ctr < 80; ctr++)
    {
        driver_put_string(24, ctr, C_HELP_INSTR, " ");
    }

    driver_move_cursor(24, 1);
    put_key("F1", "Index");
    put_key("Arrow Keys", "Select");
    put_key("Enter", "Go to");
    put_key("Backspace", "Last topic");
    put_key("Esc", "Exit help");
}

static void print_instr()
{
    for (int ctr = 0; ctr < 80; ctr++)
    {
        driver_put_string(24, ctr, C_HELP_INSTR, " ");
    }

    driver_move_cursor(24, 1);
    put_key("Escape", "Abort");
}

static void display_page(const char *title, const char *text, unsigned text_len,
                         int page, int num_pages, int start_margin,
                         int *num_link, Link *link)
{
    help_title();
    help_instr();
    driver_set_attr(2, 0, C_HELP_BODY, 80*22);
    put_string_center(1, 0, 80, C_HELP_HDG, title);
    driver_put_string(1, 79 - (6 + ((num_pages >= 10) ? 2 : 1)), C_HELP_INSTR,
        fmt::format("{:2d} of {:d}", page + 1, num_pages));

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

static int find_link_up_down(Link *link, int num_link, int curr_link, int up)
{
    int best_overlap = 0;

    Link *curr = &link[curr_link];
    Link *best = nullptr;
    int curr_c2 = curr->c + curr->width - 1;

    Link *temp = link;
    for (int ctr = 0; ctr < num_link; ctr++, temp++)
    {
        if (ctr != curr_link
            && ((up && temp->r < curr->r) || (!up && temp->r > curr->r)))
        {
            int temp_overlap = overlap(curr->c, curr_c2, temp->c, temp->c + temp->width - 1);
            // if >= 3 lines between, prioritize on vertical distance:
            int temp_dist = dist1(temp->r, curr->r);
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

static int find_link_left_right(Link *link, int num_link, int curr_link, int left)
{
    int best_c2 = 0;
    int best_dist = 0;

    Link *curr = &link[curr_link];
    Link *best = nullptr;
    int curr_c2 = curr->c + curr->width - 1;

    Link *temp = link;
    for (int ctr = 0; ctr < num_link; ctr++, temp++)
    {
        int temp_c2 = temp->c + temp->width - 1;

        if (ctr != curr_link
            && ((left && temp_c2 < (int) curr->c) || (!left && (int) temp->c > curr_c2)))
        {
            int temp_dist = dist1(curr->r, temp->r);

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

static int find_link_key(Link * /*link*/, int num_link, int curr_link, int key)
{
    switch (key)
    {
    case ID_KEY_TAB:
        return (curr_link >= num_link-1) ? -1 : curr_link+1;
    case ID_KEY_SHF_TAB:
        return (curr_link <= 0)          ? -1 : curr_link-1;
    default:
        assert(0);
        return -1;
    }
}

static int do_move_link(Link *link, int num_link, int *curr, int (*f)(Link *, int, int, int), int val)
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

static void freader(void *ptr, size_t size, size_t num, std::FILE *stream)
{
    if (std::fread(ptr, size, num, stream) != num)
    {
        throw std::system_error(errno, std::system_category(), "failed fread");
    }
}

static int help_topic(History *curr, History *next, int flags)
{
    int       key;
    int       num_pages;
    int       num_link;
    int       curr_link;
    char      title[81];
    Byte ch;

    long where = s_topic_offset[curr->topic_num] + sizeof(int); // to skip flags
    curr_link = curr->link;

    help_seek(where);

    freader(&num_pages, sizeof(int), 1, s_help_file);
    assert(num_pages > 0 && num_pages <= s_max_pages);

    freader(s_page_table.data(), 3*sizeof(int), num_pages, s_help_file);

    freader(&ch, sizeof(char), 1, s_help_file);
    int len = ch;
    assert(len < 81);
    freader(title, sizeof(char), len, s_help_file);
    title[len] = '\0';

    where += sizeof(int) + num_pages*3*sizeof(int) + 1 + len + sizeof(int);

    int page;
    for (page = 0; page < num_pages; page++)
    {
        if (curr->topic_off >= s_page_table[page].offset
            && curr->topic_off <  s_page_table[page].offset+s_page_table[page].len)
        {
            break;
        }
    }

    assert(page < num_pages);

    int action = -1;
    int draw_page = 2;

    do
    {
        if (draw_page)
        {
            help_seek(where+s_page_table[page].offset);
            s_buffer.resize(s_page_table[page].len);
            freader(s_buffer.data(), sizeof(char), s_page_table[page].len, s_help_file);

            num_link = 0;
            display_page(title, s_buffer.data(), s_page_table[page].len, page, num_pages,
                         s_page_table[page].margin, &num_link, s_link_table.data());

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
                color_link(&s_link_table[curr_link], C_HELP_CURLINK);
            }

            draw_page = 0;
        }

        {
            ValueSaver saved_tab_mode(g_tab_mode, false);
            key = driver_get_key();
        }

        switch (key)
        {
        case ID_KEY_PAGE_DOWN:
            if (page < num_pages-1)
            {
                page++;
                draw_page = 1;
            }
            break;

        case ID_KEY_PAGE_UP:
            if (page > 0)
            {
                page--;
                draw_page = 1;
            }
            break;

        case ID_KEY_HOME:
            if (page != 0)
            {
                page = 0;
                draw_page = 1;
            }
            else
            {
                do_move_link(s_link_table.data(), num_link, &curr_link, nullptr, 0);
            }
            break;

        case ID_KEY_END:
            if (page != num_pages-1)
            {
                page = num_pages-1;
                draw_page = 3;
            }
            else
            {
                do_move_link(s_link_table.data(), num_link, &curr_link, nullptr, num_link-1);
            }
            break;

        case ID_KEY_TAB:
            if (!do_move_link(s_link_table.data(), num_link, &curr_link, find_link_key, key)
                && page < num_pages-1)
            {
                ++page;
                draw_page = 1;
            }
            break;

        case ID_KEY_SHF_TAB:
            if (!do_move_link(s_link_table.data(), num_link, &curr_link, find_link_key, key)
                && page > 0)
            {
                --page;
                draw_page = 3;
            }
            break;

        case ID_KEY_DOWN_ARROW:
            if (!do_move_link(s_link_table.data(), num_link, &curr_link, find_link_up_down, 0)
                && page < num_pages-1)
            {
                ++page;
                draw_page = 1;
            }
            break;

        case ID_KEY_UP_ARROW:
            if (!do_move_link(s_link_table.data(), num_link, &curr_link, find_link_up_down, 1)
                && page > 0)
            {
                --page;
                draw_page = 3;
            }
            break;

        case ID_KEY_LEFT_ARROW:
            do_move_link(s_link_table.data(), num_link, &curr_link, find_link_left_right, 1);
            break;

        case ID_KEY_RIGHT_ARROW:
            do_move_link(s_link_table.data(), num_link, &curr_link, find_link_left_right, 0);
            break;

        case ID_KEY_ESC:         // exit help
            action = ACTION_QUIT;
            break;

        case ID_KEY_BACKSPACE:   // prev topic
        case ID_KEY_ALT_F1:
            if (flags & F_HIST)
            {
                action = ACTION_PREV;
            }
            break;

        case ID_KEY_F1:    // help index
            if (!(flags & F_INDEX))
            {
                action = ACTION_INDEX;
            }
            break;

        case ID_KEY_ENTER:
        case ID_KEY_ENTER_2:
            if (num_link > 0)
            {
                next->topic_num = s_link_table[curr_link].topic_num;
                next->topic_off = s_link_table[curr_link].topic_off;
                action = ACTION_CALL;
            }
            break;
        } // switch
    }
    while (action == -1);

    curr->topic_off = s_page_table[page].offset;
    curr->link      = curr_link;

    return action;
}

int help()
{
    int action{};
    History curr{-1, 0, 0};
    History next;

    if (g_help_mode == HelpLabels::NONE)   // is help disabled?
    {
        return 0;
    }

    if (s_help_file == nullptr)
    {
        driver_buzzer(Buzzer::PROBLEM);
        return 0;
    }

    try
    {
        s_buffer.resize(MAX_PAGE_SIZE);
        s_link_table.resize(s_max_links);
        s_page_table.resize(s_max_pages);
    }
    catch (const std::bad_alloc &)
    {
        driver_buzzer(Buzzer::PROBLEM);
        return 0;
    }

    ValueSaver saved_look_at_mouse{g_look_at_mouse, MouseLook::IGNORE_MOUSE};
    g_timer_start -= std::clock();
    driver_stack_screen();

    if (g_help_mode >= HelpLabels::HELP_INDEX)
    {
        next.topic_num = s_label[static_cast<int>(g_help_mode)].topic_num;
        next.topic_off = s_label[static_cast<int>(g_help_mode)].topic_off;
    }
    else
    {
        next.topic_num = static_cast<int>(g_help_mode);
        next.topic_off = 0;
    }

    const HelpLabels old_help_mode = g_help_mode;

    if (s_curr_hist <= 0)
    {
        action = ACTION_CALL;  // make sure it isn't ACTION_PREV!
    }

    do
    {
        switch (action)
        {
        case ACTION_PREV2:
            if (s_curr_hist > 0)
            {
                curr = s_hist[--s_curr_hist];
            }
            // fall-through

        case ACTION_PREV:
            if (s_curr_hist > 0)
            {
                curr = s_hist[--s_curr_hist];
            }
            break;

        case ACTION_QUIT:
            break;

        case ACTION_INDEX:
            next.topic_num = s_label[static_cast<int>(HelpLabels::HELP_INDEX)].topic_num;
            next.topic_off = s_label[static_cast<int>(HelpLabels::HELP_INDEX)].topic_off;
            // fall-through

        case ACTION_CALL:
            curr = next;
            curr.link = 0;
            break;
        } // switch

        int flags = 0;
        if (curr.topic_num == s_label[static_cast<int>(HelpLabels::HELP_INDEX)].topic_num)
        {
            flags |= F_INDEX;
        }
        if (s_curr_hist > 0)
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
                print_document("id.txt", print_doc_msg_func);
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
                    case ID_KEY_ESC:
                        action = ACTION_QUIT;
                        break;
                    case ID_KEY_ALT_F1:
                        action = ACTION_PREV;
                        break;
                    case ID_KEY_F1:
                        action = ACTION_INDEX;
                        break;
                    } // switch
                } // while
            }
        } // else

        if (action != ACTION_PREV && action != ACTION_PREV2)
        {
            if (s_curr_hist >= MAX_HIST)
            {
                for (int ctr = 0; ctr < MAX_HIST-1; ctr++)
                {
                    s_hist[ctr] = s_hist[ctr+1];
                }

                s_curr_hist = MAX_HIST-1;
            }
            s_hist[s_curr_hist++] = curr;
        }
    }
    while (action != ACTION_QUIT);

    driver_unstack_screen();
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

static std::string find_file(const char *filename)
{
    std::string path{(fs::path(id::config::HOME_DIR) / filename).string()};
    if (can_read_file(path))
    {
        return path;
    }
    return find_path(filename);
}

static int read_help_topic(int topic, int off, int len, void *buf)
{
    static int  curr_topic = -1;
    static long curr_base;
    static int  curr_len;

    if (topic != curr_topic)
    {
        int t;
        char ch;

        curr_topic = topic;

        curr_base = s_topic_offset[topic];

        curr_base += sizeof(int);                 // skip flags

        help_seek(curr_base);
        freader(&t, sizeof(int), 1, s_help_file); // read num_pages
        curr_base += sizeof(int) + t*3*sizeof(int); // skip page info

        if (t > 0)
        {
            help_seek(curr_base);
        }
        freader(&ch, sizeof(char), 1, s_help_file);   // read title_len
        t = ch;
        curr_base += 1 + t;                       // skip title

        if (t > 0)
        {
            help_seek(curr_base);
        }
        freader(&curr_len, sizeof(int), 1, s_help_file); // read topic len
        curr_base += sizeof(int);
    }

    int read_len = (off + len > curr_len) ? curr_len - off : len;

    if (read_len > 0)
    {
        help_seek(curr_base + off);
        freader(buf, sizeof(char), read_len, s_help_file);
    }

    return curr_len - (off+len);
}

/*
 * reads text from a help topic.  Returns number of bytes from (off+len)
 * to end of topic.  On "EOF" returns a negative number representing
 * number of bytes not read.
 */
int read_help_topic(HelpLabels label, int off, int len, void *buf)
{
    return read_help_topic(s_label[static_cast<int>(label)].topic_num,
        s_label[static_cast<int>(label)].topic_off + off, len, buf);
}

static void printer_ch(PrintDocInfo *info, int c, int n)
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

static void printer_str(PrintDocInfo *info, const char *s, int n)
{
    if (n > 0)
    {
        while (n-- > 0)
        {
            printer_ch(info, *s++, 1);
        }
    }
    else
    {
        while (*s != '\0')
        {
            printer_ch(info, *s++, 1);
        }
    }
}

static bool print_doc_get_info(PrintDocCommand cmd, ProcessDocumentInfo *pd, void *context)
{
    PrintDocInfo *info = static_cast<PrintDocInfo *>(context);
    int t;
    Byte ch;

    switch (cmd)
    {
    case PrintDocCommand::PD_GET_CONTENT:
        if (++info->current_content >= info->num_contents)
        {
            return false;
        }

        help_seek(info->content_pos);

        freader(&t, sizeof(int), 1, s_help_file);      // read flags
        info->content_pos += sizeof(int);
        pd->new_page = (t & 1) != 0;

        freader(&ch, sizeof(char), 1, s_help_file);       // read id len

        t = ch;
        assert(t < 80);
        freader(info->id, sizeof(char), t, s_help_file);  // read the id
        info->content_pos += 1 + t;
        info->id[t] = '\0';

        freader(&ch, sizeof(char), 1, s_help_file);       // read title len
        t = ch;
        assert(t < 80);
        freader(info->title, sizeof(char), t, s_help_file); // read the title
        info->content_pos += 1 + t;
        info->title[t] = '\0';

        freader(&ch, sizeof(char), 1, s_help_file);       // read num_topic
        t = ch;
        assert(t < MAX_NUM_TOPIC_SEC);
        freader(info->topic_num, sizeof(int), t, s_help_file);  // read topic_num[]
        info->num_topic = t;
        info->content_pos += 1 + t*sizeof(int);

        info->current_topic = -1;

        pd->id = info->id;
        pd->title = info->title;
        return true;

    case PrintDocCommand::PD_GET_TOPIC:
        if (++info->current_topic >= info->num_topic)
        {
            return false;
        }

        t = read_help_topic(info->topic_num[info->current_topic], 0, PRINT_BUFFER_SIZE, info->buffer);

        assert(t <= 0);

        pd->curr = info->buffer;
        pd->len  = PRINT_BUFFER_SIZE + t;   // same as ...SIZE - abs(t)
        return true;

    case PrintDocCommand::PD_GET_LINK_PAGE:
        pd->i = get_int(pd->s+2*sizeof(int));
        pd->link_page = "(p. " + std::to_string(pd->i) + ")";
        return pd->i != -1;

    case PrintDocCommand::PD_RELEASE_TOPIC:
        return true;

    default:
        return false;
    }
}

static bool print_doc_output(PrintDocCommand cmd, ProcessDocumentInfo *pd, void *context)
{
    PrintDocInfo *info = static_cast<PrintDocInfo *>(context);
    switch (cmd)
    {
    case PrintDocCommand::PD_HEADING:
    {
        char line[81];
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
        std::string buff = fmt::format(ID_PROGRAM_NAME " Version {:d}.{:d}.{:d}", //
            ID_VERSION_MAJOR, ID_VERSION_MINOR, ID_VERSION_PATCH);
        std::memmove(line + ((width - (int) (buff.length())) / 2) - 4, buff.c_str(), buff.length());

        buff = fmt::format("Page {:d}", pd->page_num);
        std::memmove(line + (width - (int)buff.length()), buff.c_str(), buff.length());

        printer_ch(info, '\n', 1);
        printer_str(info, line, width);
        printer_ch(info, '\n', 2);

        info->margin = PAGE_INDENT;

        return keep_going;
    }

    case PrintDocCommand::PD_FOOTING:
        info->margin = 0;
        printer_ch(info, '\f', 1);
        info->margin = PAGE_INDENT;
        return true;

    case PrintDocCommand::PD_PRINT:
        printer_str(info, pd->s, pd->i);
        return true;

    case PrintDocCommand::PD_PRINT_N:
        printer_ch(info, *pd->s, pd->i);
        return true;

    case PrintDocCommand::PD_PRINT_SEC:
        info->margin = TITLE_INDENT;
        if (pd->id[0] != '\0')
        {
            printer_str(info, pd->id, 0);
            printer_ch(info, ' ', 1);
        }
        printer_str(info, pd->title, 0);
        printer_ch(info, '\n', 1);
        info->margin = PAGE_INDENT;
        return true;

    case PrintDocCommand::PD_START_SECTION:
    case PrintDocCommand::PD_START_TOPIC:
    case PrintDocCommand::PD_SET_SECTION_PAGE:
    case PrintDocCommand::PD_SET_TOPIC_PAGE:
    case PrintDocCommand::PD_PERIODIC:
        return true;

    default:
        return false;
    }
}

static bool print_doc_msg_func(int page_num, int num_pages)
{
    if (page_num == -1)      // successful completion
    {
        driver_buzzer(Buzzer::COMPLETE);
        put_string_center(7, 0, 80, C_HELP_LINK, "Done -- Press any key");
        driver_get_key();
        return false;
    }

    if (page_num == -2)     // aborted
    {
        driver_buzzer(Buzzer::INTERRUPT);
        put_string_center(7, 0, 80, C_HELP_LINK, "Aborted -- Press any key");
        driver_get_key();
        return false;
    }

    if (page_num == 0)   // initialization
    {
        help_title();
        print_instr();
        driver_set_attr(2, 0, C_HELP_BODY, 80*22);
        put_string_center(1, 0, 80, C_HELP_HDG, "Generating id.txt");

        driver_put_string(7, 30, C_HELP_BODY, "Completed:");

        driver_hide_text_cursor();
    }

    driver_put_string(7, 41, C_HELP_LINK, fmt::format("{:d}%", (int) (100.0 / num_pages * page_num)));

    while (driver_key_pressed())
    {
        int key = driver_get_key();
        if (key == ID_KEY_ESC)
        {
            return false;    // user abort
        }
    }

    return true;   // AOK -- continue
}

bool make_doc_msg_func(int page_num, int num_pages)
{
    enum
    {
        BOX_ROW = 6,
        BOX_COL = 11,
    };
    if (page_num >= 0)
    {
        driver_put_string(BOX_ROW + 8, BOX_COL + 4, C_DVID_LO,
            fmt::format("completed {:d}%", (int) ((100.0 / num_pages) * page_num)));
        driver_key_pressed(); // pumps messages to force screen update
        return true;
    }
    if (page_num == -2)
    {
        stop_msg("*** aborted");
        return false;
    }
    return false;
}

void print_document(const char *filename, bool (*msg_func)(int, int))
{
    PrintDocInfo info;
    bool success = false;
    const char *msg = nullptr;

    help_seek(16L);
    freader(&info.num_contents, sizeof(int), 1, s_help_file);
    freader(&info.num_page, sizeof(int), 1, s_help_file);

    info.current_topic = -1;
    info.current_content = info.current_topic;
    info.content_pos = 6*sizeof(int) + s_num_topic*sizeof(long) + s_num_label*2*sizeof(int);
    info.msg_func = msg_func;

    if (msg_func != nullptr)
    {
        msg_func(0, info.num_page);   // initialize
    }

    info.file = open_save_file(filename, "wt");
    if (info.file == nullptr)
    {
        msg = "Unable to create output file.\n";
        goto error_abort;
    }

    info.margin = PAGE_INDENT;
    info.start_of_line = true;
    info.spaces = 0;

    success = process_document(TokenMode::DOC, true, print_doc_get_info, print_doc_output, &info);
    std::fclose(info.file);

error_abort:
    if (msg != nullptr)
    {
        help_title();
        stop_msg(StopMsgFlags::NO_STACK, msg);
    }

    else if (msg_func != nullptr)
    {
        msg_func(success ? -1 : -2, info.num_page);
    }
}

int init_help()
{
    HelpSignature hs{};

    s_help_file = nullptr;

    const std::string path{find_file("id.hlp")};
    if (!path.empty())
    {
        s_help_file = std::fopen(path.c_str(), "rb");
        if (s_help_file != nullptr)
        {
            freader(&hs, sizeof(HelpSignature), 1, s_help_file);

            if (hs.sig != HELP_SIG)
            {
                std::fclose(s_help_file);
                stop_msg(StopMsgFlags::NO_STACK, "Invalid help signature in id.hlp!\n");
            }
            else if (hs.version != IDHELP_VERSION)
            {
                std::fclose(s_help_file);
                stop_msg(StopMsgFlags::NO_STACK, "Wrong help version in id.hlp!\n");
            }
            else
            {
                s_base_off = sizeof(HelpSignature);
            }
        }
    }

    if (s_help_file == nullptr)         // Can't find the help files anywhere!
    {
        stop_msg(StopMsgFlags::NO_STACK, "Couldn't find id.hlp; set environment variable FRACTDIR to proper directory.\n");
    }

    help_seek(0L);

    freader(&s_max_pages, sizeof(int), 1, s_help_file);
    freader(&s_max_links, sizeof(int), 1, s_help_file);
    freader(&s_num_topic, sizeof(int), 1, s_help_file);
    freader(&s_num_label, sizeof(int), 1, s_help_file);
    help_seek(6L*sizeof(int));  // skip num_contents and num_doc_pages

    assert(s_max_pages > 0);
    assert(s_max_links >= 0);
    assert(s_num_topic > 0);
    assert(s_num_label > 0);

    // allocate all three arrays
    try
    {
        s_topic_offset.resize(s_num_topic);
        s_label.resize(s_num_label);
        s_hist.resize(MAX_HIST);
    }
    catch (const std::bad_alloc &)
    {
        std::fclose(s_help_file);
        s_help_file = nullptr;
        stop_msg(StopMsgFlags::NO_STACK, "Not enough memory for help system!\n");

        return -2;
    }

    // read in the tables...
    freader(s_topic_offset.data(), sizeof(long), s_num_topic, s_help_file);
    freader(s_label.data(), sizeof(Label), s_num_label, s_help_file);

    // finished!

    return 0;  // success
}

void end_help()
{
    if (s_help_file != nullptr)
    {
        std::fclose(s_help_file);
        s_help_file = nullptr;
    }
}
