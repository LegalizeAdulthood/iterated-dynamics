/*
 * Common #defines, structures and code for help functions
 *
 */
#pragma once

#include <cstring>

 /*
 * help file signature
 */
#define HELP_SIG           (0xAFBC1823UL)

enum class help_labels;

/*
 * commands embedded in the help text
 */
enum help_commands
{
    CMD_LITERAL = 1,    // next char taken literally
    CMD_PARA = 2,       // paragraph start code
    CMD_LINK = 3,       // hot-link start/end code
    CMD_FF = 4,         // force a form-feed
    CMD_XONLINE = 5,    // exclude from online help on/off
    CMD_XDOC = 6,       // exclude from printed document on/off
    CMD_CENTER = 7,     // center this line
    CMD_SPACE = 8,      // next byte is count of spaces
    MAX_CMD = 8
};
/*
 * on-line help dimensions
 */
#define SCREEN_WIDTH      (78)
#define SCREEN_DEPTH      (22)
#define SCREEN_INDENT     (1)
/*
 * printed document dimensions
 */
#define PAGE_WIDTH         (72)  // width of printed text
#define PAGE_INDENT        (2)   // indent all text by this much
#define TITLE_INDENT       (1)   // indent titles by this much
#define PAGE_RDEPTH        (59)  // the total depth (inc. heading)
#define PAGE_HEADING_DEPTH (3)   // depth of the heading
#define PAGE_DEPTH         (PAGE_RDEPTH-PAGE_HEADING_DEPTH) // depth of text
/*
 * Document page-break macros.  Goto to next page if this close (or closer)
 * to end of page when starting a CONTENT, TOPIC, or at a BLANK line.
 */
#define CONTENT_BREAK (7)  // start of a "DocContent" entry
#define TOPIC_BREAK   (4)  // start of each topic under a DocContent entry
#define BLANK_BREAK   (2)  // a blank line
/*
 * tokens returned by find_token_length
 */
enum class token_types
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
    TOK_CENTER = 9      // a CMD_CENTER
};
/*
 * modes for find_token_length() and find_line_width()
 */
enum class token_modes
{
    NONE = 0,
    ONLINE = 1,
    DOC = 2
};
/*
 * struct PD_INFO used by process_document()
 */
struct PD_INFO
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
};
/*
 * Commands passed to (*get_info)() and (*output)() by process_document()
 */
enum class PD_COMMANDS
{
    // commands sent to pd_output
    PD_HEADING,         // call at the top of each page
    PD_FOOTING,          // called at the end of each page
    PD_PRINT,            // called to send text to the printer
    PD_PRINTN,           // called to print a char n times
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
    PD_GET_LINK_PAGE
};
typedef bool (*PD_FUNC)(PD_COMMANDS cmd, PD_INFO *pd, void *info);
token_types find_token_length(token_modes mode, char const *curr, unsigned len, int *size, int *width);
int find_line_width(token_modes mode, char const *curr, unsigned len);
bool process_document(PD_FUNC get_info, PD_FUNC output, void *info);
int help(int);
int read_help_topic(help_labels label, int , int , void *);
bool makedoc_msg_func(int pnum, int num_pages);
void print_document(char const *outfname, bool (*msg_func)(int, int));
int init_help();
void end_help();
bool is_hyphen(char const *ptr);

/* Get an int from an unaligned pointer
 * This routine is needed because this program uses unaligned 2 byte
 * pointers all over the place.
 */
inline int getint(char const *ptr)
{
    int s;
    std::memcpy(&s, ptr, sizeof(int));
    return s;
}

/* Set an int to an unaligned pointer */
inline void setint(char *ptr, int n)
{
    std::memcpy(ptr, &n, sizeof(int));
}
