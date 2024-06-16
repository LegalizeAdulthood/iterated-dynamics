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
extern int filelength(int);
#endif

#if defined(_WIN32)
// disable unsafe CRT warnings
#pragma warning(disable: 4996)
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

struct help_sig_info
{
    unsigned long sig;
    int           version;
    unsigned long base;
};

int g_max_pages{};                   // max. pages in any topic
int g_num_doc_pages{};               // total number of pages in document

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
 * stuff to resolve hot-link references.
 */

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
    for (CONTENT &c : g_src.contents)
    {
        for (int ctr = 0; ctr < c.num_topic; ctr++)
        {
            if (c.is_label[ctr])
            {
                c.label_topic(ctr);
            }
            else
            {
                c.content_topic(ctr);
            }
        }
    }

    /*
     * Find topic_num and topic_off for all hot-links.  Also flag all hot-
     * links which will (probably) appear in the document.
     */
    for (LINK &l : g_src.all_links)
    {
        // name is the title of the topic
        if (l.type == link_types::LT_TOPIC)
        {
            l.link_topic();
        }
        // name is the name of a label
        else if (l.type == link_types::LT_LABEL)
        {
            l.link_label();
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

        t.release_topic_text(false);
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

    for (LINK &l : g_src.all_links)
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

    for (const CONTENT &c : g_src.contents)
    {
        assert(c.doc_page >= 1);
        std::sprintf(buf, "%d", c.doc_page);
        len = (int) std::strlen(buf);
        assert(len <= 3);
        std::memcpy(base + c.page_num_pos + (3 - len), buf, len);
    }

    t.release_topic_text(true);
}


// this function also used by print_document()
bool pd_get_info(int cmd, PD_INFO *pd, void *context)
{
    DOC_INFO &info = *static_cast<DOC_INFO *>(context);
    const CONTENT *c;

    switch (cmd)
    {
    case PD_GET_CONTENT:
        if (++info.content_num >= static_cast<int>(g_src.contents.size()))
        {
            return false;
        }
        c = &g_src.contents[info.content_num];
        info.topic_num = -1;
        pd->id       = c->id.c_str();
        pd->title    = c->name.c_str();
        pd->new_page = (c->flags & CF_NEW_PAGE) != 0;
        return true;

    case PD_GET_TOPIC:
        c = &g_src.contents[info.content_num];
        if (++info.topic_num >= c->num_topic)
        {
            return false;
        }
        pd->curr = g_topics[c->topic_num[info.topic_num]].get_topic_text();
        pd->len = g_topics[c->topic_num[info.topic_num]].text_len;
        return true;

    case PD_GET_LINK_PAGE:
    {
        const LINK &link = g_src.all_links[getint(pd->s)];
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
        pd->i = g_src.all_links[getint(pd->s)].doc_page;
        return true;
    }

    case PD_RELEASE_TOPIC:
        c = &g_src.contents[info.content_num];
        g_topics[c->topic_num[info.topic_num]].release_topic_text(false);
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
        info->c = &g_src.contents[info->content_num];
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

    if (g_src.contents.empty())
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

void sort_labels()
{
    std::sort(g_labels.begin(), g_labels.end());
    std::sort(g_private_labels.begin(), g_private_labels.end());
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

    offset = std::accumulate(g_src.contents.begin(), g_src.contents.end(), offset, [](long offset, const CONTENT &cp) {
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
            const LINK &l = g_src.all_links[ getint(curr+1) ];
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
    putw(static_cast<int>(g_src.contents.size()), file);

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
    for (const CONTENT &cp : g_src.contents)
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

        tp.release_topic_text(false);  // don't save the text even though
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

    if (g_src.contents.empty())
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

    for (const LINK &l : g_src.all_links)
    {
        data += sizeof(LINK);
        bytes_in_strings += (long) l.name.length();
    }

    dead += static_cast<long>((g_src.all_links.capacity() - g_src.all_links.size()) * sizeof(LINK));

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

    for (const CONTENT &c : g_src.contents)
    {
        int t = (MAX_CONTENT_TOPIC - c.num_topic) *
            (sizeof(g_src.contents[0].is_label[0]) + sizeof(g_src.contents[0].topic_name[0]) + sizeof(g_src.contents[0].topic_num[0]));
        data += sizeof(CONTENT) - t;
        dead += t;
        bytes_in_strings += (long) c.id.length() + 1;
        bytes_in_strings += (long) c.name.length() + 1;
        for (int ctr2 = 0; ctr2 < c.num_topic; ctr2++)
        {
            bytes_in_strings += (long) c.topic_name[ctr2].length() + 1;
        }
    }

    dead += static_cast<long>((g_src.contents.capacity() - g_src.contents.size()) * sizeof(CONTENT));

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
    std::printf("%8d Links\n", static_cast<int>(g_src.all_links.size()));
    std::printf("%8d Labels\n", static_cast<int>(g_labels.size()));
    std::printf("%8d Private labels\n", static_cast<int>(g_private_labels.size()));
    std::printf("%8d Table of contents (DocContent) entries\n", static_cast<int>(g_src.contents.size()));
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

class compiler : public Compiler
{
public:
    compiler(const Options &options);
    ~compiler();

    int process();

private:
    void read_source_file();
    void usage();
    void compile();
    void print();
    void render_html();
    void paginate_html_document();
    void print_html_document(const std::string &output_filename);

    Options m_options;
    HelpSource m_src;
};

compiler::compiler(const Options &options) :
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

    m_src = read_src(g_src_filename, m_options.mode);
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
    if (m_src.contents.empty())
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

        t.release_topic_text(false);
    } // for
}

void compiler::print_html_document(std::string const &fname)
{
    html_processor(fname).process();
}

std::shared_ptr<Compiler> create_compiler(const Options &options)
{
    return std::make_shared<compiler>(options);
}

} // namespace hc
