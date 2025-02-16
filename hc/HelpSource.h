// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "modes.h"

#include <string>
#include <vector>

namespace hc
{

constexpr int MAX_CONTENT_TOPIC{10};
constexpr const char *DOC_CONTENTS_TITLE{"DocContent"};
constexpr const char *INDEX_LABEL{"HELP_INDEX"};
constexpr int BUFFER_SIZE{1024 * 1024}; // 1 MB

// values for Content.flags
enum
{
    CF_NEW_PAGE = 1         // true if section starts on a new page
};

struct Content
{
    void label_topic(int ctr);
    void content_topic(int ctr);

    unsigned flags;
    std::string id;
    int indent;
    std::string name;
    int doc_page;
    unsigned page_num_pos;
    int num_topic;
    bool is_label[MAX_CONTENT_TOPIC];
    std::string topic_name[MAX_CONTENT_TOPIC];
    int topic_num[MAX_CONTENT_TOPIC];
    std::string src_file;
    int src_line;
};

struct Label
{
    std::string name;         // its name
    int      topic_num;       // topic number
    unsigned topic_off;       // offset of label in the topic's text
    int      doc_page;
};

inline bool operator<(const Label &a, const Label &b)
{
    if (a.name == INDEX_LABEL)
    {
        return true;
    }
    if (b.name == INDEX_LABEL)
    {
        return false;
    }

    return a.name < b.name;
}

enum class LinkTypes
{
    LT_TOPIC,
    LT_LABEL,
    LT_SPECIAL
};

struct Link
{
    void link_topic();
    void link_label();

    LinkTypes type;       // 0 = name is topic title, 1 = name is label,
                          //   2 = "special topic"; name is nullptr and
                          //   topic_num/topic_off is valid
    int topic_num;        // topic number to link to
    unsigned topic_off;   // offset into topic to link to
    int doc_page;         // document page # to link to
    std::string name;     // name of label or title of topic to link to
    std::string src_file; // .SRC file link appears in
    int src_line;         // .SRC file line # link appears in
};

struct Page
{
    unsigned offset;    // offset from start of topic text
    unsigned length;    // length of page (in chars)
    int      margin;    // if > 0 then page starts in_para and text
                        // should be indented by this much
};

// values for Topic.flags
enum class TopicFlags
{
    NONE = 0,   // nothing special
    IN_DOC = 1, // set if topic is part of the printed document
    DATA = 2    // set if it is a "data" topic
};

inline int operator+(TopicFlags val)
{
    return static_cast<int>(val);
}

inline TopicFlags operator|(TopicFlags lhs, TopicFlags rhs)
{
    return static_cast<TopicFlags>(+lhs | +rhs);
}

inline TopicFlags &operator|=(TopicFlags &lhs, TopicFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline TopicFlags operator&(TopicFlags lhs, TopicFlags rhs)
{
    return static_cast<TopicFlags>(+lhs & +rhs);
}

inline bool bit_set(TopicFlags flags, TopicFlags bit)
{
    return (flags & bit) == bit;
}

struct Topic
{
    void alloc_topic_text(unsigned size);
    int add_page(const Page &p);
    void add_page_break(int margin, const char *str, const char *start, const char *curr, int num_links);
    char *get_topic_text();
    const char *get_topic_text() const;
    void release_topic_text(bool save) const;
    void start(const char *str, int len);

    TopicFlags flags;        // see #defines for TF_???
    int       doc_page;       // page number in document where topic starts
    unsigned  title_len;      // length of title
    std::string title;        // title for this topic
    int       num_page;       // number of pages
    std::vector<Page> page;   // list of pages
    unsigned  text_len;       // length of topic text
    long      text;           // topic text (all pages)
    long      offset;         // offset from start of file to topic

private:
    void read_topic_text() const;
};

struct HelpSource
{
    int add_content(const Content &c);
    int add_link(Link &l);
    int add_topic(const Topic &t);
    int add_label(const Label &l);
    Label *find_label(const char *name);
    void sort_labels();

    std::vector<Content> contents;
    std::vector<Link> all_links;
    std::vector<Topic> topics;
    std::vector<Label> labels;
    std::vector<Label> private_labels;
    std::FILE *swap_file{};
    long swap_pos{};
    std::vector<char> buffer;               // buffer to/from swap file
    char *curr{};                           // current position in the buffer
    int max_links{};                        // max. links on any page
    std::string hdr_filename;               // .H filename
    std::string hlp_filename;               // .HLP filename
    int version{-1};                        // help file version
    std::vector<std::string> include_paths; //
};

extern HelpSource g_src;

int find_topic_title(const char *title);
void read_src(const std::string &fname, Mode mode);

} // namespace hc
