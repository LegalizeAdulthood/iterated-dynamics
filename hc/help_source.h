#pragma once

#include <string>
#include <vector>

/*
 * When defined, SHOW_ERROR_LINE will cause the line number in HC.C where
 * errors/warnings/messages are generated to be displayed at the start of
 * the line.
 *
 * Used when debugging HC.  Also useful for finding the line (in HC.C) that
 * generated a error or warning.
 */
#define SHOW_ERROR_LINE

#ifdef SHOW_ERROR_LINE
#   define error(...)  (show_line(__LINE__), error_msg(__VA_ARGS__))
#   define warn(...)   (show_line(__LINE__), warn_msg(__VA_ARGS__))
#   define notice(...) (show_line(__LINE__), notice_msg(__VA_ARGS__))
#   define msg(...)    (g_quiet_mode ? static_cast<void>(0) : (show_line(__LINE__), msg_msg(__VA_ARGS__)))
#else
#define error(...)  error_msg(__VA_ARGS__)
#define warn(...)   warn_msg(__VA_ARGS__)
#define notice(...) notice_msg(__VA_ARGS__)
#define msg(...)    msg_msg(__VA_ARGS__)
#endif

namespace hc
{

constexpr int MAX_CONTENT_TOPIC{10};
constexpr const char *DOCCONTENTS_TITLE{"DocContent"};

// values for CONTENT.flags
enum
{
    CF_NEW_PAGE = 1         // true if section starts on a new page
};

struct CONTENT
{
    unsigned flags;
    std::string id;
    std::string name;
    int doc_page;
    unsigned page_num_pos;
    int num_topic;
    bool is_label[MAX_CONTENT_TOPIC];
    std::string topic_name[MAX_CONTENT_TOPIC];
    int topic_num[MAX_CONTENT_TOPIC];
    std::string srcfile;
    int srcline;
};

struct LABEL
{
    std::string name;         // its name
    int      topic_num;       // topic number
    unsigned topic_off;       // offset of label in the topic's text
    int      doc_page;
};

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
    void alloc_topic_text(unsigned size);
    int add_page(const PAGE &p);
    void add_page_break(int margin, char const *text, char const *start, char const *curr, int num_links);

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

extern bool g_quiet_mode;
extern std::vector<CONTENT> g_contents;
extern std::vector<LINK> g_all_links;
extern std::vector<TOPIC> g_topics;
extern long g_swap_pos;
extern std::FILE *g_swap_file;
extern std::vector<char> g_buffer;
extern int g_max_links;

void show_line(unsigned int line);
void error_msg(int diff, char const *format, ...);
void warn_msg(int diff, char const *format, ...);
void notice_msg(char const *format, ...);
void msg_msg(char const *format, ...);

char *get_topic_text(const TOPIC &t);
std::string rst_name(std::string const &content_name);

}
