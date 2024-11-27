// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "modes.h"

#include <string>
#include <vector>

namespace hc
{

constexpr int MAX_CONTENT_TOPIC{10};
constexpr const char *DOCCONTENTS_TITLE{"DocContent"};
constexpr char const *INDEX_LABEL{"HELP_INDEX"};
constexpr int BUFFER_SIZE{1024 * 1024}; // 1 MB

// values for CONTENT.flags
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
    std::string srcfile;
    int srcline;
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

enum class link_types
{
    LT_TOPIC,
    LT_LABEL,
    LT_SPECIAL
};

struct Link
{
    void link_topic();
    void link_label();

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

struct Page
{
    unsigned offset;    // offset from start of topic text
    unsigned length;    // length of page (in chars)
    int      margin;    // if > 0 then page starts in_para and text
                        // should be indented by this much
};

// values for TOPIC.flags
enum class topic_flags
{
    NONE = 0,   // nothing special
    IN_DOC = 1, // set if topic is part of the printed document
    DATA = 2    // set if it is a "data" topic
};

inline int operator+(topic_flags val)
{
    return static_cast<int>(val);
}

inline topic_flags operator|(topic_flags lhs, topic_flags rhs)
{
    return static_cast<topic_flags>(+lhs | +rhs);
}

inline topic_flags &operator|=(topic_flags &lhs, topic_flags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline topic_flags operator&(topic_flags lhs, topic_flags rhs)
{
    return static_cast<topic_flags>(+lhs & +rhs);
}

inline bool bit_set(topic_flags flags, topic_flags bit)
{
    return (flags & bit) == bit;
}

struct Topic
{
    void alloc_topic_text(unsigned size);
    int add_page(const Page &p);
    void add_page_break(int margin, char const *text, char const *start, char const *curr, int num_links);
    char *get_topic_text();
    const char *get_topic_text() const;
    void release_topic_text(bool save) const;
    void start(char const *text, int len);

    topic_flags flags;        // see #defines for TF_???
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
    Label *find_label(char const *name);
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

int find_topic_title(char const *title);
void read_src(std::string const &fname, modes mode);

} // namespace hc
