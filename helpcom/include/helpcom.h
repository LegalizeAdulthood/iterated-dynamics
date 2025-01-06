// SPDX-License-Identifier: GPL-3.0-only
//
/*
 * Common #defines, structures and code for help functions
 *
 */
#pragma once

#include <cstdint>
#include <cstring>
#include <string>

 /*
 * help file signature
 */
enum : unsigned long
{
    HELP_SIG = 0xAFBC1823UL
};

struct HelpSignature
{
    std::uint32_t sig;
    std::int16_t version;
    std::uint32_t base;
};

enum class HelpLabels;

/*
 * commands embedded in the help text
 */
enum HelpCommand
{
    CMD_LITERAL = 1,    // next char taken literally
    CMD_PARA = 2,       // paragraph start code
    CMD_LINK = 3,       // hot-link start/end code
    CMD_FF = 4,         // force a form-feed
    CMD_XONLINE = 5,    // exclude from online help on/off
    CMD_XDOC = 6,       // exclude from printed document on/off
    CMD_CENTER = 7,     // center this line
    CMD_SPACE = 8,      // next byte is count of spaces
    CMD_XADOC = 9,      // exclude from AsciiDoc document on/off
    MAX_CMD = 9
};

enum
{
    // on-line help dimensions
    SCREEN_WIDTH = 78,
    SCREEN_DEPTH = 22,
    SCREEN_INDENT = 1,

    // printed document dimensions
    PAGE_WIDTH = 72,                                       // width of printed text
    PAGE_INDENT = 2,                                       // indent all text by this much
    TITLE_INDENT = 1,                                      // indent titles by this much
    PAGE_TOTAL_HEIGHT = 59,                                // the total height (inc. heading)
    PAGE_HEADING_HEIGHT = 3,                               // height of the heading
    PAGE_HEIGHT = PAGE_TOTAL_HEIGHT - PAGE_HEADING_HEIGHT, // height of text

    // Document page-break values.  Goto to next page if this close (or closer)
    // to end of page when starting a Content, Topic, or at a BLANK line.
    CONTENT_BREAK = 7, // start of a "DocContent" entry
    TOPIC_BREAK = 4,   // start of each topic under a DocContent entry
    BLANK_BREAK = 2    // a blank line
};

/*
 * tokens returned by find_token_length
 */
enum class TokenType
{
    TOK_DONE = 0,       // len == 0
    TOK_SPACE = 1,      // a run of spaces
    TOK_LINK = 2,       // an entire link
    TOK_PARA = 3,       // a CMD_PARA
    TOK_NL = 4,         // a new-line ('\n')
    TOK_FF = 5,         // a form-feed (CMD_FF)
    TOK_WORD = 6,       // a word
    TOK_XONLINE = 7,    // a CMD_XONLINE
    TOK_XDOC = 8,       // a CMD_XDOC
    TOK_CENTER = 9,     // a CMD_CENTER
    TOK_XADOC = 10,     // a CMD_XADOC
};

/*
 * modes for find_token_length() and find_line_width()
 */
enum class TokenMode
{
    NONE = 0,
    ONLINE = 1,
    DOC = 2,
    ADOC = 3,
};

/*
 * struct ProcessDocumentInfo used by process_document()
 */
struct ProcessDocumentInfo
{
    // used by process_document -- look but don't touch!
    int page_num;
    int line_num;
    // PD_GET_TOPIC is allowed to change these
    char const *curr;
    unsigned  len;
    // PD_GET_CONTENT is allowed to change these
    char const *id;
    char const *title;
    bool new_page;
    // general parameters
    char const *s;
    int i;
    // PD_GET_LINK_PAGE
    std::string link_page;
};

/*
 * Commands passed to (*get_info)() and (*output)() by process_document()
 */
enum class PrintDocCommand
{
    // commands sent to pd_output
    PD_HEADING,          // call at the top of each page
    PD_FOOTING,          // called at the end of each page
    PD_PRINT,            // called to send text to the printer
    PD_PRINT_N,          // called to print a char n times
    PD_PRINT_SEC,        // called to print the section title line
    PD_START_SECTION,    // called at the start of each section
    PD_START_TOPIC,      // called at the start of each topic
    PD_SET_SECTION_PAGE, // set the current sections page number
    PD_SET_TOPIC_PAGE,   // set the current topics page number
    PD_PERIODIC,         // called just before curr is incremented to next token
    // commands sent to pd_get_info
    PD_GET_CONTENT,
    PD_GET_TOPIC,
    PD_RELEASE_TOPIC,
    PD_GET_LINK_PAGE,
};

using PD_FUNC = bool(PrintDocCommand cmd, ProcessDocumentInfo *pd, void *info);
TokenType find_token_length(
    TokenMode mode, char const *curr, unsigned len, int *ret_size, int *ret_width);
int find_line_width(TokenMode mode, char const *curr, unsigned len);
bool process_document(TokenMode mode, PD_FUNC *get_info, PD_FUNC *output, void *info);
int help();
int read_help_topic(HelpLabels label, int , int , void *);
bool make_doc_msg_func(int page_num, int num_pages);
void print_document(char const *filename, bool (*msg_func)(int, int));
int init_help();
void end_help();
bool is_hyphen(char const *ptr);

/* Get an int from an unaligned pointer
 * This routine is needed because this program uses unaligned 2 byte
 * pointers all over the place.
 */
inline int get_int(char const *ptr)
{
    int s;
    std::memcpy(&s, ptr, sizeof(int));
    return s;
}

/* Set an int to an unaligned pointer */
inline void set_int(char *ptr, int n)
{
    std::memcpy(ptr, &n, sizeof(int));
}
