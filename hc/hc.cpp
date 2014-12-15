/*
 * hc.c
 *
 * Stand-alone FRACTINT help compiler.  Compile in the COMPACT memory model.
 *
 * See help-compiler.txt for source file syntax.
 *
 */
#include <algorithm>
#include <cstdarg>
#include <string>
#include <vector>

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "port.h"
#include "helpcom.h"

#define MAXFILE _MAX_FNAME
#define MAXEXT  _MAX_EXT
#define FNSPLIT _splitpath

#ifdef XFRACT
#include <unistd.h>

#ifndef HAVESTRI
extern int stricmp(const char *, const char *);
extern int strnicmp(const char *, const char *, int);
#endif
extern int filelength(int);
extern int _splitpath(const char *file_template, char *drive, char *dir, char *fname, char *ext);
#define strupr strlwr

#else

/*
 * When defined, SHOW_ERROR_LINE will cause the line number in HC.C where
 * errors/warnings/messages are generated to be displayed at the start of
 * the line.
 *
 * Used when debugging HC.  Also useful for finding the line (in HC.C) that
 * generated a error or warning.
 */
#define SHOW_ERROR_LINE

#if defined(_WIN32)
#include <io.h>
/* disable unsafe CRT warnings */
#pragma warning(disable: 4996)
#endif

#endif

#define DEFAULT_SRC_FNAME "help.src"
#define DEFAULT_HLP_FNAME "fractint.hlp"
#define DEFAULT_EXE_FNAME "fractint.exe"
#define DEFAULT_DOC_FNAME "fractint.doc"

#define TEMP_FNAME        "HC.$$$"
#define SWAP_FNAME        "HCSWAP.$$$"

#define MAX_ERRORS        (25)   /* stop after this many errors */
#define MAX_WARNINGS      (25)   /* stop after this many warnings */
                                 /* 0 = never stop */

#define INDEX_LABEL       "FIHELP_INDEX"
#define DOCCONTENTS_TITLE "DocContent"

#define BUFFER_SIZE   (30*1024)


struct LINK
{
    int      type;          /* 0 = name is topic title, 1 = name is label, */
                            /*   2 = "special topic"; name is nullptr and */
                            /*   topic_num/topic_off is valid */
    int      topic_num;     /* topic number to link to */
    unsigned topic_off;     /* offset into topic to link to */
    int      doc_page;      /* document page # to link to */
    char    *name;          /* name of label or title of topic to link to */
    char    *srcfile;       /* .SRC file link appears in */
    int      srcline;       /* .SRC file line # link appears in */
};


struct PAGE
{
    unsigned offset;     /* offset from start of topic text */
    unsigned length;     /* length of page (in chars) */
    int      margin;     /* if > 0 then page starts in_para and text */
    /* should be indented by this much */
};


/* values for TOPIC.flags */

#define TF_IN_DOC  (1)       /* 1 if topic is part of the printed document */
#define TF_DATA    (2)       /* 1 if it is a "data" topic */


struct TOPIC
{
    unsigned  flags;          /* see #defines for TF_??? */
    int       doc_page;       /* page number in document where topic starts */
    unsigned  title_len;      /* length of title */
    char     *title;          /* title for this topic */
    int       num_page;       /* number of pages */
    PAGE     *page;           /* list of pages */
    unsigned  text_len;       /* lenth of topic text */
    long      text;           /* topic text (all pages) */
    long      offset;         /* offset to topic from start of file */
};


struct LABEL
{
    char    *name;            /* its name */
    int      topic_num;       /* topic number */
    unsigned topic_off;       /* offset of label in the topic's text */
    int      doc_page;
};


/* values for CONTENT.flags */

#define CF_NEW_PAGE  (1)     /* true if section starts on a new page */


#define MAX_CONTENT_TOPIC (10)


struct CONTENT
{
    unsigned  flags;
    char     *id;
    char     *name;
    int       doc_page;
    unsigned  page_num_pos;
    int       num_topic;
    char      is_label[MAX_CONTENT_TOPIC];
    char     *topic_name[MAX_CONTENT_TOPIC];
    int       topic_num[MAX_CONTENT_TOPIC];
    char     *srcfile;
    int       srcline;
};


struct help_sig_info
{
    unsigned long sig;
    int           version;
    unsigned long base;
};


int      num_topic        = 0;    /* topics */
TOPIC   *topic;

int      num_label        = 0;    /* labels */
LABEL   *label;

int      num_plabel       = 0;    /* private labels */
LABEL   *plabel;

int      num_link         = 0;    /* all links */
LINK    *a_link           = 0;

int      num_contents     = 0;    /* the table-of-contents */
CONTENT *contents;

bool quiet_mode = false;          /* true if "/Q" option used */

int      max_pages        = 0;    /* max. pages in any topic */
int      max_links        = 0;    /* max. links on any page */
int      num_doc_pages    = 0;    /* total number of pages in document */

FILE    *srcfile;                 /* .SRC file */
int      srcline          = 0;    /* .SRC line number (used for errors) */
int      srccol           = 0;    /* .SRC column. */

int      version          = -1;   /* help file version */

int      errors           = 0,    /* number of errors reported */
         warnings         = 0;    /* number of warnings reported */

char     src_fname[81]    = "";   /* command-line .SRC filename */
char     hdr_fname[81]    = "";   /* .H filename */
char     hlp_fname[81]    = "";   /* .HLP filename */
char    *src_cfname       = nullptr; /* current .SRC filename */

int      format_exclude   = 0;    /* disable formatting at this col, 0 to */
/*    never disable formatting */
FILE    *swapfile;
long     swappos;

char    *buffer;                  /* alloc'ed as BUFFER_SIZE bytes */
char    *curr;                    /* current position in the buffer */
char     cmd[128];                /* holds the current command */
bool compress_spaces = false;
bool xonline = false;
bool xdoc = false;

#define  MAX_INCLUDE_STACK (5)    /* allow 5 nested includes */

struct include_stack_entry
{
    char *fname;
    FILE *file;
    int   line;
    int   col;
};
include_stack_entry include_stack[MAX_INCLUDE_STACK];
int include_stack_top = -1;

void check_buffer(char *current, unsigned off, char *buffer);

#define CHK_BUFFER(off) check_buffer(curr, off, buffer)

#ifdef XFRACT
#define putw( x1, x2 )  fwrite( &(x1), 1, sizeof(int), x2);
#endif

/*
 * error/warning/message reporting functions.
 */


void report_errors()
{
    printf("\n");
    printf("Compiler Status:\n");
    printf("%8d Error%c\n",       errors, (errors==1)   ? ' ' : 's');
    printf("%8d Warning%c\n",     warnings, (warnings==1) ? ' ' : 's');
}


void print_msg(const char *type, int lnum, const char *format, va_list arg)
{
    if (type != nullptr)
    {
        printf("   %s", type);
        if (lnum>0)
            printf(" %s %d", src_cfname, lnum);
        printf(": ");
    }
    vprintf(format, arg);
    printf("\n");
}

void fatal(int diff, const char *format, ...)
{
    va_list arg;
    va_start(arg, format);

    print_msg("Fatal", srcline-diff, format, arg);
    va_end(arg);

    if (errors || warnings)
        report_errors();

    exit(errors + 1);
}


void error(int diff, const char *format, ...)
{
    va_list arg;
    va_start(arg, format);

    print_msg("Error", srcline-diff, format, arg);
    va_end(arg);

    if (++errors >= MAX_ERRORS && MAX_ERRORS > 0)
        fatal(0,"Too many errors!");
}


void warn(int diff, const char *format, ...)
{
    va_list arg;
    va_start(arg, format);

    print_msg("Warning", srcline-diff, format, arg);
    va_end(arg);

    if (++warnings >= MAX_WARNINGS && MAX_WARNINGS > 0)
        fatal(0,"Too many warnings!");
}


void notice(const char *format, ...)
{
    va_list arg;
    va_start(arg, format);
    print_msg("Note", srcline, format, arg);
    va_end(arg);
}


void msg(const char *format, ...)
{
    va_list arg;

    if (quiet_mode)
        return;
    va_start(arg, format);
    print_msg(nullptr, 0, format, arg);
    va_end(arg);
}


#ifdef SHOW_ERROR_LINE
#   define fatal  (printf("[%04d] ", __LINE__), fatal)
#   define error  (printf("[%04d] ", __LINE__), error)
#   define warn   (printf("[%04d] ", __LINE__), warn)
#   define notice (printf("[%04d] ", __LINE__), notice)
#   define msg    (printf(quiet_mode ? "" : "[%04d] ", __LINE__), msg)
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
    fwrite(buffer, 1, t->text_len, swapfile);
}


char *get_topic_text(TOPIC *t)
{
    fseek(swapfile, t->text, SEEK_SET);
    fread(buffer, 1, t->text_len, swapfile);
    return (buffer);
}


void release_topic_text(TOPIC *t, int save)
{
    if (save)
    {
        fseek(swapfile, t->text, SEEK_SET);
        fwrite(buffer, 1, t->text_len, swapfile);
    }
}


/*
 * memory-allocation functions.
 */


VOIDPTR newx(unsigned size)
{
    VOIDPTR ptr;

    ptr = malloc(size);

    if (ptr == nullptr)
        fatal(0,"Out of memory!");

    return (ptr);
}


VOIDPTR renewx(VOIDPTR ptr, unsigned size)
{
    ptr = realloc(ptr, size);

    if (ptr == nullptr)
        fatal(0,"Out of memory!");

    return (ptr);
}


char *dupstr(const char *s, unsigned len)
{
    char *ptr;

    if (len == 0)
        len = (int) strlen(s) + 1;

    ptr = static_cast<char *>(newx(len));

    memcpy(ptr, s, len);

    return (ptr);
}


#define LINK_ALLOC_SIZE (16)


int add_link(LINK *l)
{
    if (num_link == 0)
        a_link = static_cast<LINK *>(newx(sizeof(LINK)*LINK_ALLOC_SIZE));

    else if (num_link%LINK_ALLOC_SIZE == 0)
        a_link = static_cast<LINK *>(renewx(a_link, sizeof(LINK) * (num_link+LINK_ALLOC_SIZE)));

    a_link[num_link] = *l;

    return (num_link++);
}


#define PAGE_ALLOC_SIZE (4)


int add_page(TOPIC *t, PAGE *p)
{
    if (t->num_page == 0)
        t->page = static_cast<PAGE *>(newx(sizeof(PAGE)*PAGE_ALLOC_SIZE));

    else if (t->num_page%PAGE_ALLOC_SIZE == 0)
        t->page = static_cast<PAGE *>(renewx(t->page, sizeof(PAGE) * (t->num_page+PAGE_ALLOC_SIZE)));

    t->page[t->num_page] = *p;

    return (t->num_page++);
}


#define TOPIC_ALLOC_SIZE (16)


int add_topic(TOPIC *t)
{
    if (num_topic == 0)
        topic = static_cast<TOPIC *>(newx(sizeof(TOPIC)*TOPIC_ALLOC_SIZE));

    else if (num_topic%TOPIC_ALLOC_SIZE == 0)
        topic = static_cast<TOPIC *>(renewx(topic, sizeof(TOPIC) * (num_topic+TOPIC_ALLOC_SIZE)));

    topic[num_topic] = *t;

    return (num_topic++);
}


#define LABEL_ALLOC_SIZE (16)


int add_label(LABEL *l)
{
    if (l->name[0] == '@')    /* if it's a private label... */
    {
        if (num_plabel == 0)
            plabel = static_cast<LABEL *>(newx(sizeof(LABEL)*LABEL_ALLOC_SIZE));

        else if (num_plabel%LABEL_ALLOC_SIZE == 0)
            plabel = static_cast<LABEL *>(renewx(plabel, sizeof(LABEL) * (num_plabel+LABEL_ALLOC_SIZE)));

        plabel[num_plabel] = *l;

        return (num_plabel++);
    }
    else
    {
        if (num_label == 0)
            label = static_cast<LABEL *>(newx(sizeof(LABEL)*LABEL_ALLOC_SIZE));

        else if (num_label%LABEL_ALLOC_SIZE == 0)
            label = static_cast<LABEL *>(renewx(label, sizeof(LABEL) * (num_label+LABEL_ALLOC_SIZE)));

        label[num_label] = *l;

        return (num_label++);
    }
}


#define CONTENTS_ALLOC_SIZE (16)


int add_content(CONTENT *c)
{
    if (num_contents == 0)
        contents = static_cast<CONTENT *>(newx(sizeof(CONTENT)*CONTENTS_ALLOC_SIZE));

    else if (num_contents%CONTENTS_ALLOC_SIZE == 0)
        contents = static_cast<CONTENT *>(renewx(contents, sizeof(CONTENT) * (num_contents+CONTENTS_ALLOC_SIZE)));

    contents[num_contents] = *c;

    return (num_contents++);
}


/*
 * read_char() stuff
 */


#define READ_CHAR_BUFF_SIZE (32)


int  read_char_buff[READ_CHAR_BUFF_SIZE];
int  read_char_buff_pos = -1;
int  read_char_sp       = 0;


void unread_char(int ch)
/*
 * Will not handle new-lines or tabs correctly!
 */
{
    if (read_char_buff_pos+1 >= READ_CHAR_BUFF_SIZE)
        fatal(0,"Compiler Error -- Read char buffer overflow!");

    read_char_buff[++read_char_buff_pos] = ch;

    --srccol;
}


void unread_string(char *s)
{
    int p = (int) strlen(s);

    while (p-- > 0)
        unread_char(s[p]);
}


int eos()    /* end-of-source ? */
{
    return (!((read_char_sp==0) && (read_char_buff_pos==0) && feof(srcfile)));
}


int _read_char()
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
        return (read_char_buff[read_char_buff_pos--]);
    }

    if (read_char_sp > 0)
    {
        --read_char_sp;
        return (' ');
    }

    if (feof(srcfile))
        return (-1);

    while (1)
    {
        ch = getc(srcfile);

        switch (ch)
        {
        case '\t':    /* expand a tab */
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
            read_char_sp = 0;   /* delete spaces before a \n */
            srccol = 0;
            ++srcline;
            return ('\n');

        case -1:               /* EOF */
            if (read_char_sp > 0)
            {
                --read_char_sp;
                return (' ');
            }
            return (-1);

        default:
            if (read_char_sp > 0)
            {
                ungetc(ch, srcfile);
                --read_char_sp;
                return (' ');
            }

            ++srccol;
            return (ch);

        } /* switch */
    }
}


int read_char()
{
    int ch;

    ch = _read_char();

    while (ch == ';' && srccol==1)    /* skip over comments */
    {
        ch = _read_char();

        while (ch!='\n' && ch!=-1)
            ch = _read_char();

        ch = _read_char();
    }

    if (ch == '\\')   /* process an escape code */
    {
        ch = _read_char();

        if (ch >= '0' && ch <= '9')
        {
            char buff[4];
            int  ctr = 0;

            for (ctr = 0; true; ctr++)
            {
                if (ch<'0' || ch>'9' || ch==-1 || ctr>=3)
                {
                    unread_char(ch);
                    break;
                }
                buff[ctr] = ch;
                ch = _read_char();
            }
            buff[ctr] = '\0';
            ch = atoi(buff);
        }

#ifdef XFRACT
        /* Convert graphics arrows into keyboard chars */
        if (ch>=24 && ch<=27) {
            ch = "KJHL"[ch-24];
        }
#endif
        ch |= 0x100;
    }

    if ((ch & 0xFF) == 0)
    {
        error(0,"Null character (\'\\0\') not allowed!");
        ch = 0x1FF; /* since we've had an error the file will not be written; */
        /*   the value we return doesn't really matter */
    }

    return (ch);
}


/*
 * misc. search functions.
 */


LABEL *find_label(char *name)
{
    if (*name == '@')
    {
        LABEL *lp = plabel;
        for (int l = 0; l < num_plabel; l++, lp++)
            if (strcmp(name, lp->name) == 0)
                return (lp);
    }
    else
    {
        LABEL *lp = label;
        for (int l = 0; l < num_label; l++, lp++)
            if (strcmp(name, lp->name) == 0)
                return (lp);
    }

    return (nullptr);
}


int find_topic_title(const char *title)
{
    while (*title == ' ')
        ++title;

    int len = (int) strlen(title) - 1;
    while (title[len] == ' ' && len > 0)
        --len;

    ++len;

    if (len > 2 && title[0] == '\"' && title[len-1] == '\"')
    {
        ++title;
        len -= 2;
    }

    for (int t = 0; t < num_topic; t++)
        if ((int) strlen(topic[t].title) == len &&
                strnicmp(title, topic[t].title, len) == 0)
            return (t);

    return (-1);   /* not found */
}


/*
 * .SRC file parser stuff
 */


int validate_label_name(char *name)
{
    if (!isalpha(*name) && *name!='@' && *name!='_')
        return (0);  /* invalid */

    while (*(++name) != '\0')
        if (!isalpha(*name) && !isdigit(*name) && *name!='_')
            return (0); /* invalid */

    return (1);  /* valid */
}


char *read_until(char *buff, int len, const char *stop_chars)
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
            *buff++ = CMD_LITERAL;

        *buff++ = ch;

        if ((ch&0x100)==0 && strchr(stop_chars, ch) != nullptr)
            break;
    }

    return (buff-1);
}


void skip_over(const char *skip)
{
    int ch;

    while (1)
    {
        ch = read_char();

        if (ch == -1)
            break;

        else if ((ch&0x100) == 0 && strchr(skip, ch) == nullptr)
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
        sprintf(buff, "\'%c\'", ch);
    else
        sprintf(buff, "\'\\x%02X\'", ch&0xFF);

    return (buff);
}


void put_spaces(int how_many)
{
    if (how_many > 2 && compress_spaces)
    {
        if (how_many > 255)
        {
            error(0,"Too many spaces (over 255).");
            how_many = 255;
        }

        *curr++ = CMD_SPACE;
        *curr++ = (BYTE)how_many;
    }
    else
    {
        while (how_many-- > 0)
            *curr++ = ' ';
    }
}


/* used by parse_contents() */
bool get_next_item()
{
    char *ptr;

    skip_over(" \t\n");
    ptr = read_until(cmd, 128, ",}");
    bool last = (*ptr == '}');
    --ptr;
    while (ptr >= cmd && strchr(" \t\n",*ptr))     /* strip trailing spaces */
        --ptr;
    *(++ptr) = '\0';

    return last;
}


void process_contents()
{
    CONTENT c;
    char   *ptr;
    int     indent;
    int     ch;
    TOPIC   t;

    t.flags     = 0;
    t.title_len = (unsigned) strlen(DOCCONTENTS_TITLE)+1;
    t.title     = dupstr(DOCCONTENTS_TITLE, t.title_len);
    t.doc_page  = -1;
    t.num_page  = 0;

    curr = buffer;

    c.flags = 0;
    c.id = dupstr("",1);
    c.name = dupstr("",1);
    c.doc_page = -1;
    c.page_num_pos = 0;
    c.num_topic = 1;
    c.is_label[0] = 0;
    c.topic_name[0] = dupstr(DOCCONTENTS_TITLE,0);
    c.srcline = -1;
    add_content(&c);

    while (1)
    {
        ch = read_char();

        if (ch == '{')   /* process a CONTENT entry */
        {
            c.flags = 0;
            c.num_topic = 0;
            c.doc_page = -1;
            c.srcfile = src_cfname;
            c.srcline = srcline;

            if (get_next_item())
            {
                error(0,"Unexpected end of DocContent entry.");
                continue;
            }
            c.id = dupstr(cmd,0);

            if (get_next_item())
            {
                error(0,"Unexpected end of DocContent entry.");
                continue;
            }
            indent = atoi(cmd);

            bool last = get_next_item();

            if (cmd[0] == '\"')
            {
                ptr = cmd+1;
                if (ptr[(int) strlen(ptr)-1] == '\"')
                    ptr[(int) strlen(ptr)-1] = '\0';
                else
                    warn(0,"Missing ending quote.");

                c.is_label[c.num_topic] = 0;
                c.topic_name[c.num_topic] = dupstr(ptr,0);
                ++c.num_topic;
                c.name = dupstr(ptr,0);
            }
            else
                c.name = dupstr(cmd,0);

            /* now, make the entry in the buffer */

            sprintf(curr, "%-5s %*.0s%s", c.id, indent*2, "", c.name);
            ptr = curr + (int) strlen(curr);
            while ((ptr-curr) < PAGE_WIDTH-10)
                *ptr++ = '.';
            c.page_num_pos = (unsigned)((ptr-3) - buffer);
            curr = ptr;

            while (!last)
            {
                last = get_next_item();

                if (stricmp(cmd, "FF") == 0)
                {
                    if (c.flags & CF_NEW_PAGE)
                        warn(0,"FF already present in this entry.");
                    c.flags |= CF_NEW_PAGE;
                    continue;
                }

                if (cmd[0] == '\"')
                {
                    ptr = cmd+1;
                    if (ptr[(int) strlen(ptr)-1] == '\"')
                        ptr[(int) strlen(ptr)-1] = '\0';
                    else
                        warn(0,"Missing ending quote.");

                    c.is_label[c.num_topic] = 0;
                    c.topic_name[c.num_topic] = dupstr(ptr,0);
                }
                else
                {
                    c.is_label[c.num_topic] = 1;
                    c.topic_name[c.num_topic] = dupstr(cmd,0);
                }

                if (++c.num_topic >= MAX_CONTENT_TOPIC)
                {
                    error(0,"Too many topics in DocContent entry.");
                    break;
                }
            }

            add_content(&c);
        }

        else if (ch == '~')   /* end at any command */
        {
            unread_char(ch);
            break;
        }

        else
            *curr++ = ch;

        CHK_BUFFER(0);
    }

    alloc_topic_text(&t, (unsigned)(curr - buffer));
    add_topic(&t);
}


int parse_link()   /* returns length of link or 0 on error */
{
    char *ptr;
    char *end;
    bool bad = false;
    int   len;
    LINK  l;
    int   err_off;

    l.srcfile  = src_cfname;
    l.srcline  = srcline;
    l.doc_page = -1;

    end = read_until(cmd, 128, "}\n");   /* get the entire hot-link */

    if (*end == '\0')
    {
        error(0,"Unexpected EOF in hot-link.");
        return (0);
    }

    if (*end == '\n')
    {
        err_off = 1;
        warn(1,"Hot-link has no closing curly-brace (\'}\').");
    }
    else
        err_off = 0;

    *end = '\0';

    if (cmd[0] == '=')   /* it's an "explicit" link to a label or "special" */
    {
        ptr = strchr(cmd, ' ');

        if (ptr == nullptr)
            ptr = end;
        else
            *ptr++ = '\0';

        len = (int)(end - ptr);

        if (cmd[1] == '-')
        {
            l.type      = 2;          /* type 2 = "special" */
            l.topic_num = atoi(cmd+1);
            l.topic_off = 0;
            l.name      = nullptr;
        }
        else
        {
            l.type = 1;           /* type 1 = to a label */
            if ((int)strlen(cmd) > 32)
                warn(err_off, "Label is long.");
            if (cmd[1] == '\0')
            {
                error(err_off, "Explicit hot-link has no Label.");
                bad = true;
            }
            else
                l.name = dupstr(cmd+1,0);
        }
        if (len == 0)
            warn(err_off, "Explicit hot-link has no title.");
    }
    else
    {
        ptr = cmd;
        l.type = 0;   /* type 0 = topic title */
        len = (int)(end - ptr);
        if (len == 0)
        {
            error(err_off, "Implicit hot-link has no title.");
            bad = true;
        }
        l.name = dupstr(ptr,len+1);
        l.name[len] = '\0';
    }

    if (!bad)
    {
        CHK_BUFFER(1+3*sizeof(int)+len+1);
        int const lnum = add_link(&l);
        *curr++ = CMD_LINK;
        setint(curr,lnum);
        curr += 3*sizeof(int);
        memcpy(curr, ptr, len);
        curr += len;
        *curr++ = CMD_LINK;
        return (len);
    }
    else
        return (0);
}


#define MAX_TABLE_SIZE (100)


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

    ptr = strchr(cmd, '=');

    if (ptr == nullptr)
        return (0);   /* should never happen! */

    ptr++;

    len = sscanf(ptr, " %d %d %d", &width, &cols, &start_off);

    if (len < 3)
    {
        error(1,"Too few arguments to Table.");
        return (0);
    }

    if (width<=0 || width > 78 || cols<=0 || start_off<0 || start_off > 78)
    {
        error(1,"Argument out of range.");
        return (0);
    }

    bool done = false;

    first_link = num_link;
    table_start = curr;
    count = 0;

    /* first, read all the links in the table */

    do
    {
        do
            ch = read_char();
        while (ch=='\n' || ch == ' ');

        if (done)
            break;

        switch (ch)
        {
        case -1:
            error(0,"Unexpected EOF in a Table.");
            return (0);

        case '{':
            if (count >= MAX_TABLE_SIZE)
                fatal(0,"Table is too large.");
            len = parse_link();
            curr = table_start;   /* reset to the start... */
            title.push_back(std::string(curr+3*sizeof(int)+1, len+1));
            if (len >= width)
            {
                warn(1,"Link is too long; truncating.");
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
                imbedded = true;
            else
            {
                imbedded = false;
                unread_char(ch);
            }

            ptr = read_until(cmd, 128, ")\n,");

            ch = *ptr;
            *ptr = '\0';

            if (stricmp(cmd, "EndTable") == 0)
                done = true;
            else
            {
                error(1,"Unexpected command in table \"%s\"", cmd);
                warn(1,"Command will be ignored.");
            }

            if (ch == ',')
            {
                if (imbedded)
                    unread_char('(');
                unread_char('~');
            }
        }
        break;

        default:
            error(0,"Unexpected character %s.", pchar(ch));
            break;
        }
    }
    while (!done);

    /* now, put all the links into the buffer... */

    rows = 1 + (count / cols);

    for (int r = 0; r < rows; r++)
    {
        put_spaces(start_off);
        for (int c = 0; c < cols; c++)
        {
            lnum = c*rows + r;

            if (first_link+lnum >= num_link)
                break;

            len = title[lnum].length();
            *curr++ = CMD_LINK;
            setint(curr,first_link+lnum);
            curr += 3*sizeof(int);
            memcpy(curr, title[lnum].c_str(), len);
            curr += len;
            *curr++ = CMD_LINK;

            if (c < cols-1)
                put_spaces(width-len);
        }
        *curr++ = '\n';
    }

    return (1);
}


void process_comment()
{
    int ch;

    while (1)
    {
        ch = read_char();

        if (ch == '~')
        {
            bool imbedded;
            char *ptr;

            ch = read_char();

            if (ch=='(')
                imbedded = true;
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
                        unread_char('(');
                    unread_char('~');
                }
                break;
            }
        }

        else if (ch == -1)
        {
            error(0,"Unexpected EOF in Comment");
            break;
        }
    }
}


void process_bininc()
{
    int  handle;
    long len;

    if ((handle=open(cmd+7, O_RDONLY|O_BINARY)) == -1)
    {
        error(0,"Unable to open \"%s\"", cmd+7);
        return ;
    }

    len = filelength(handle);

    if (len >= BUFFER_SIZE)
    {
        error(0,"File \"%s\" is too large to BinInc (%dK).", cmd+7, (int)(len>>10));
        close(handle);
        return ;
    }

    /*
     * Since we know len is less than BUFFER_SIZE (and therefore less then
     * 64K) we can treat it as an unsigned.
     */

    CHK_BUFFER((unsigned)len);

    read(handle, curr, (unsigned)len);

    curr += (unsigned)len;

    close(handle);
}


void start_topic(TOPIC *t, const char *title, int title_len)
{
    t->flags = 0;
    t->title_len = title_len;
    t->title = dupstr(title, title_len+1);
    t->title[title_len] = '\0';
    t->doc_page = -1;
    t->num_page = 0;
    curr = buffer;
}


void end_topic(TOPIC *t)
{
    alloc_topic_text(t, (unsigned)(curr - buffer));
    add_topic(t);
}


bool end_of_sentence(char *ptr)  /* true if ptr is at the end of a sentence */
{
    if (*ptr == ')')
        --ptr;

    if (*ptr == '\"')
        --ptr;

    return (*ptr=='.' || *ptr=='?' || *ptr=='!');
}


void add_blank_for_split()   /* add space at curr for merging two lines */
{
    if (!is_hyphen(curr-1))     /* no spaces if it's a hyphen */
    {
        if (end_of_sentence(curr-1))
            *curr++ = ' ';  /* two spaces at end of a sentence */
        *curr++ = ' ';
    }
}


void put_a_char(int ch, TOPIC *t)
{
    if (ch == '{' && !(t->flags & TF_DATA))    /* is it a hot-link? */
        parse_link();
    else
    {
        if ((ch&0xFF) <= MAX_CMD)
            *curr++ = CMD_LITERAL;
        *curr++ = ch;
    }
}


enum STATES   /* states for FSM's */
{
    S_Start,                 /* initial state, between paragraphs           */
    S_StartFirstLine,        /* spaces at start of first line               */
    S_FirstLine,             /* text on the first line                      */
    S_FirstLineSpaces,       /* spaces on the first line                    */
    S_StartSecondLine,       /* spaces at start of second line              */
    S_Line,                  /* text on lines after the first               */
    S_LineSpaces,            /* spaces on lines after the first             */
    S_StartLine,             /* spaces at start of lines after second       */
    S_FormatDisabled,        /* format automatically disabled for this line */
    S_FormatDisabledSpaces,  /* spaces in line which format is disabled     */
    S_Spaces
} ;


void check_command_length(int eoff, int len)
{
    if ((int) strlen(cmd) != len)
        error(eoff, "Invalid text after a command \"%s\"", cmd+len);
}


void read_src(char *fname)
{
    int    ch;
    char  *ptr;
    TOPIC  t;
    LABEL  lbl;
    char  *margin_pos = nullptr;
    bool in_topic = false;
    bool formatting = true;
    int    state      = S_Start;
    int    num_spaces = 0;
    int    margin     = 0;
    bool in_para = false;
    bool centering = false;
    int    lformat_exclude = format_exclude;

    xonline = false;
    xdoc = false;

    src_cfname = fname;

    if ((srcfile = fopen(fname, "rt")) == nullptr)
        fatal(0,"Unable to open \"%s\"", fname);

    msg("Compiling: %s", fname);

    in_topic = false;

    curr = buffer;

    while (1)
    {

        ch = read_char();

        if (ch == -1)     /* EOF? */
        {
            if (include_stack_top >= 0)
            {
                fclose(srcfile);
                src_cfname = include_stack[include_stack_top].fname;
                srcfile = include_stack[include_stack_top].file;
                srcline = include_stack[include_stack_top].line;
                srccol  = include_stack[include_stack_top].col;
                --include_stack_top;
                continue;
            }
            else
            {
                if (in_topic)  /* if we're in a topic, finish it */
                    end_topic(&t);
                if (num_topic == 0)
                    warn(0,".SRC file has no topics.");
                break;
            }
        }

        if (ch == '~')   /* is is a command? */
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
                eoff=0;
                unread_char(ch);
            }

            bool done = false;

            while (!done)
            {
                do
                    ch = read_char();
                while (ch == ' ');
                unread_char(ch);

                if (imbedded)
                    ptr = read_until(cmd, 128, ")\n,");
                else
                    ptr = read_until(cmd, 128, "\n,");

                if (*ptr == '\0')
                {
                    error(0,"Unexpected EOF in command.");
                    break;
                }

                if (*ptr == '\n')
                    ++eoff;

                if (imbedded && *ptr == '\n')
                    error(eoff,"Embedded command has no closing paren (\')\')");

                done = (*ptr != ',');   /* we done if it's not a comma */

                if (*ptr != '\n' && *ptr != ')' && *ptr != ',')
                {
                    error(0,"Command line too long.");
                    break;
                }

                *ptr = '\0';


                /* commands allowed anytime... */

                if (strnicmp(cmd, "Topic=", 6) == 0)
                {
                    if (in_topic)  /* if we're in a topic, finish it */
                        end_topic(&t);
                    else
                        in_topic = true;

                    if (cmd[6] == '\0')
                        warn(eoff,"Topic has no title.");

                    else if ((int)strlen(cmd+6) > 70)
                        error(eoff,"Topic title is too long.");

                    else if ((int)strlen(cmd+6) > 60)
                        warn(eoff,"Topic title is long.");

                    if (find_topic_title(cmd+6) != -1)
                        error(eoff,"Topic title already exists.");

                    start_topic(&t, cmd+6, (unsigned)(ptr-(cmd+6)));
                    formatting = true;
                    centering = false;
                    state = S_Start;
                    in_para = false;
                    num_spaces = 0;
                    xonline = false;
                    xdoc = false;
                    lformat_exclude = format_exclude;
                    compress_spaces = true;
                    continue;
                }

                else if (strnicmp(cmd, "Data=", 5) == 0)
                {
                    if (in_topic)  /* if we're in a topic, finish it */
                        end_topic(&t);
                    else
                        in_topic = true;

                    if (cmd[5] == '\0')
                        warn(eoff,"Data topic has no label.");

                    if (!validate_label_name(cmd+5))
                    {
                        error(eoff,"Label \"%s\" contains illegal characters.", cmd+5);
                        continue;
                    }

                    if (find_label(cmd+5) != nullptr)
                    {
                        error(eoff,"Label \"%s\" already exists", cmd+5);
                        continue;
                    }

                    if (cmd[5] == '@')
                        warn(eoff, "Data topic has a local label.");

                    start_topic(&t, "", 0);
                    t.flags |= TF_DATA;

                    if ((int)strlen(cmd+5) > 32)
                        warn(eoff,"Label name is long.");

                    lbl.name      = dupstr(cmd+5, 0);
                    lbl.topic_num = num_topic;
                    lbl.topic_off = 0;
                    lbl.doc_page  = -1;
                    add_label(&lbl);

                    formatting = false;
                    centering = false;
                    state = S_Start;
                    in_para = false;
                    num_spaces = 0;
                    xonline = false;
                    xdoc = false;
                    lformat_exclude = format_exclude;
                    compress_spaces = false;
                    continue;
                }

                else if (strnicmp(cmd, "DocContents", 11) == 0)
                {
                    check_command_length(eoff, 11);
                    if (in_topic)  /* if we're in a topic, finish it */
                        end_topic(&t);
                    if (!done)
                    {
                        if (imbedded)
                            unread_char('(');
                        unread_char('~');
                        done = true;
                    }
                    compress_spaces = true;
                    process_contents();
                    in_topic = false;
                    continue;
                }

                else if (stricmp(cmd, "Comment") == 0)
                {
                    process_comment();
                    continue;
                }

                else if (strnicmp(cmd, "FormatExclude", 13) == 0)
                {
                    if (cmd[13] == '-')
                    {
                        check_command_length(eoff, 14);
                        if (in_topic)
                        {
                            if (lformat_exclude > 0)
                                lformat_exclude = -lformat_exclude;
                            else
                                warn(eoff,"\"FormatExclude-\" is already in effect.");
                        }
                        else
                        {
                            if (format_exclude > 0)
                                format_exclude = -format_exclude;
                            else
                                warn(eoff,"\"FormatExclude-\" is already in effect.");
                        }
                    }
                    else if (cmd[13] == '+')
                    {
                        check_command_length(eoff,14);
                        if (in_topic)
                        {
                            if (lformat_exclude < 0)
                                lformat_exclude = -lformat_exclude;
                            else
                                warn(eoff,"\"FormatExclude+\" is already in effect.");
                        }
                        else
                        {
                            if (format_exclude < 0)
                                format_exclude = -format_exclude;
                            else
                                warn(eoff,"\"FormatExclude+\" is already in effect.");
                        }
                    }
                    else if (cmd[13] == '=')
                    {
                        if (cmd[14] == 'n' || cmd[14] == 'N')
                        {
                            check_command_length(eoff,15);
                            if (in_topic)
                                lformat_exclude = 0;
                            else
                                format_exclude = 0;
                        }
                        else if (cmd[14] == '\0')
                            lformat_exclude = format_exclude;
                        else
                        {
                            int n = ((in_topic ? lformat_exclude : format_exclude) < 0) ? -1 : 1;

                            lformat_exclude = atoi(cmd+14);

                            if (lformat_exclude <= 0)
                            {
                                error(eoff,"Invalid argument to FormatExclude=");
                                lformat_exclude = 0;
                            }

                            lformat_exclude *= n;

                            if (!in_topic)
                                format_exclude = lformat_exclude;
                        }
                    }
                    else
                        error(eoff,"Invalid format for FormatExclude");

                    continue;
                }

                else if (strnicmp(cmd, "Include ", 8) == 0)
                {
                    if (include_stack_top >= MAX_INCLUDE_STACK-1)
                        error(eoff, "Too many nested Includes.");
                    else
                    {
                        ++include_stack_top;
                        include_stack[include_stack_top].fname = src_cfname;
                        include_stack[include_stack_top].file = srcfile;
                        include_stack[include_stack_top].line = srcline;
                        include_stack[include_stack_top].col  = srccol;
                        strupr(cmd+8);
                        if ((srcfile = fopen(cmd+8, "rt")) == nullptr)
                        {
                            error(eoff, "Unable to open \"%s\"", cmd+8);
                            srcfile = include_stack[include_stack_top--].file;
                        }
                        src_cfname = dupstr(cmd+8,0);  /* never deallocate! */
                        srcline = 1;
                        srccol = 0;
                    }

                    continue;
                }


                /* commands allowed only before all topics... */

                if (!in_topic)
                {
                    if (strnicmp(cmd, "HdrFile=", 8) == 0)
                    {
                        if (hdr_fname[0] != '\0')
                            warn(eoff,"Header Filename has already been defined.");
                        strcpy(hdr_fname, cmd+8);
                        strupr(hdr_fname);
                    }

                    else if (strnicmp(cmd, "HlpFile=", 8) == 0)
                    {
                        if (hlp_fname[0] != '\0')
                            warn(eoff,"Help Filename has already been defined.");
                        strcpy(hlp_fname, cmd+8);
                        strupr(hlp_fname);
                    }

                    else if (strnicmp(cmd, "Version=", 8) == 0)
                    {
                        if (version != -1)   /* an unlikely value */
                            warn(eoff,"Help version has already been defined");
                        version = atoi(cmd+8);
                    }

                    else
                        error(eoff,"Bad or unexpected command \"%s\"", cmd);

                    continue;
                }


                /* commands allowed only in a topic... */

                else
                {
                    if (strnicmp(cmd, "FF", 2) == 0)
                    {
                        check_command_length(eoff,2);
                        if (in_para)
                            *curr++ = '\n';  /* finish off current paragraph */
                        *curr++ = CMD_FF;
                        state = S_Start;
                        in_para = false;
                        num_spaces = 0;
                    }

                    else if (strnicmp(cmd, "DocFF", 5) == 0)
                    {
                        check_command_length(eoff,5);
                        if (in_para)
                            *curr++ = '\n';  /* finish off current paragraph */
                        if (!xonline)
                            *curr++ = CMD_XONLINE;
                        *curr++ = CMD_FF;
                        if (!xonline)
                            *curr++ = CMD_XONLINE;
                        state = S_Start;
                        in_para = false;
                        num_spaces = 0;
                    }

                    else if (strnicmp(cmd, "OnlineFF", 8) == 0)
                    {
                        check_command_length(eoff,8);
                        if (in_para)
                            *curr++ = '\n';  /* finish off current paragraph */
                        if (!xdoc)
                            *curr++ = CMD_XDOC;
                        *curr++ = CMD_FF;
                        if (!xdoc)
                            *curr++ = CMD_XDOC;
                        state = S_Start;
                        in_para = false;
                        num_spaces = 0;
                    }

                    else if (strnicmp(cmd, "Label=", 6) == 0)
                    {
                        if ((int)strlen(cmd+6) <= 0)
                            error(eoff,"Label has no name.");

                        else if (!validate_label_name(cmd+6))
                            error(eoff,"Label \"%s\" contains illegal characters.", cmd+6);

                        else if (find_label(cmd+6) != nullptr)
                            error(eoff,"Label \"%s\" already exists", cmd+6);

                        else
                        {
                            if ((int)strlen(cmd+6) > 32)
                                warn(eoff,"Label name is long.");

                            if ((t.flags & TF_DATA) && cmd[6] == '@')
                                warn(eoff, "Data topic has a local label.");

                            lbl.name      = dupstr(cmd+6, 0);
                            lbl.topic_num = num_topic;
                            lbl.topic_off = (unsigned)(curr - buffer);
                            lbl.doc_page  = -1;
                            add_label(&lbl);
                        }
                    }

                    else if (strnicmp(cmd, "Table=", 6) == 0)
                    {
                        if (in_para)
                        {
                            *curr++ = '\n';  /* finish off current paragraph */
                            in_para = false;
                            num_spaces = 0;
                            state = S_Start;
                        }

                        if (!done)
                        {
                            if (imbedded)
                                unread_char('(');
                            unread_char('~');
                            done = true;
                        }

                        create_table();
                    }

                    else if (strnicmp(cmd, "FormatExclude", 12) == 0)
                    {
                        if (cmd[13] == '-')
                        {
                            check_command_length(eoff,14);
                            if (lformat_exclude > 0)
                                lformat_exclude = -lformat_exclude;
                            else
                                warn(0,"\"FormatExclude-\" is already in effect.");
                        }
                        else if (cmd[13] == '+')
                        {
                            check_command_length(eoff,14);
                            if (lformat_exclude < 0)
                                lformat_exclude = -lformat_exclude;
                            else
                                warn(0,"\"FormatExclude+\" is already in effect.");
                        }
                        else
                            error(eoff,"Unexpected or invalid argument to FormatExclude.");
                    }

                    else if (strnicmp(cmd, "Format", 6) == 0)
                    {
                        if (cmd[6] == '+')
                        {
                            check_command_length(eoff,7);
                            if (!formatting)
                            {
                                formatting = true;
                                in_para = false;
                                num_spaces = 0;
                                state = S_Start;
                            }
                            else
                                warn(eoff,"\"Format+\" is already in effect.");
                        }
                        else if (cmd[6] == '-')
                        {
                            check_command_length(eoff,7);
                            if (formatting)
                            {
                                if (in_para)
                                    *curr++ = '\n';  /* finish off current paragraph */
                                in_para = false;
                                formatting = false;
                                num_spaces = 0;
                                state = S_Start;
                            }
                            else
                                warn(eoff,"\"Format-\" is already in effect.");
                        }
                        else
                            error(eoff,"Invalid argument to Format.");
                    }

                    else if (strnicmp(cmd, "Online", 6) == 0)
                    {
                        if (cmd[6] == '+')
                        {
                            check_command_length(eoff,7);

                            if (xonline)
                            {
                                *curr++ = CMD_XONLINE;
                                xonline = false;
                            }
                            else
                                warn(eoff,"\"Online+\" already in effect.");
                        }
                        else if (cmd[6] == '-')
                        {
                            check_command_length(eoff,7);
                            if (!xonline)
                            {
                                *curr++ = CMD_XONLINE;
                                xonline = true;
                            }
                            else
                                warn(eoff,"\"Online-\" already in effect.");
                        }
                        else
                            error(eoff,"Invalid argument to Online.");
                    }

                    else if (strnicmp(cmd, "Doc", 3) == 0)
                    {
                        if (cmd[3] == '+')
                        {
                            check_command_length(eoff,4);
                            if (xdoc)
                            {
                                *curr++ = CMD_XDOC;
                                xdoc = false;
                            }
                            else
                                warn(eoff,"\"Doc+\" already in effect.");
                        }
                        else if (cmd[3] == '-')
                        {
                            check_command_length(eoff,4);
                            if (!xdoc)
                            {
                                *curr++ = CMD_XDOC;
                                xdoc = true;
                            }
                            else
                                warn(eoff,"\"Doc-\" already in effect.");
                        }
                        else
                            error(eoff,"Invalid argument to Doc.");
                    }

                    else if (strnicmp(cmd, "Center", 6) == 0)
                    {
                        if (cmd[6] == '+')
                        {
                            check_command_length(eoff,7);
                            if (!centering)
                            {
                                centering = true;
                                if (in_para)
                                {
                                    *curr++ = '\n';
                                    in_para = false;
                                }
                                state = S_Start;  /* for centering FSM */
                            }
                            else
                                warn(eoff,"\"Center+\" already in effect.");
                        }
                        else if (cmd[6] == '-')
                        {
                            check_command_length(eoff,7);
                            if (centering)
                            {
                                centering = false;
                                state = S_Start;  /* for centering FSM */
                            }
                            else
                                warn(eoff,"\"Center-\" already in effect.");
                        }
                        else
                            error(eoff,"Invalid argument to Center.");
                    }

                    else if (strnicmp(cmd, "CompressSpaces", 14) == 0)
                    {
                        check_command_length(eoff,15);

                        if (cmd[14] == '+')
                        {
                            if (compress_spaces)
                                warn(eoff,"\"CompressSpaces+\" is already in effect.");
                            else
                                compress_spaces = true;
                        }
                        else if (cmd[14] == '-')
                        {
                            if (!compress_spaces)
                                warn(eoff,"\"CompressSpaces-\" is already in effect.");
                            else
                                compress_spaces = false;
                        }
                        else
                            error(eoff,"Invalid argument to CompressSpaces.");
                    }

                    else if (strnicmp("BinInc ", cmd, 7) == 0)
                    {
                        if (!(t.flags & TF_DATA))
                            error(eoff,"BinInc allowed only in Data topics.");
                        else
                            process_bininc();
                    }

                    else
                        error(eoff,"Bad or unexpected command \"%s\".", cmd);
                } /* else */

            } /* while (!done) */

            continue;
        }

        if (!in_topic)
        {
            cmd[0] = ch;
            ptr = read_until(cmd+1, 127, "\n~");
            if (*ptr == '~')
                unread_char('~');
            *ptr = '\0';
            error(0,"Text outside of any topic \"%s\".", cmd);
            continue;
        }

        if (centering)
        {
            bool again;
            do
            {
                again = false;   /* default */

                switch (state)
                {
                case S_Start:
                    if (ch == ' ')
                        ; /* do nothing */
                    else if ((ch&0xFF) == '\n')
                        *curr++ = ch;  /* no need to center blank lines. */
                    else
                    {
                        *curr++ = CMD_CENTER;
                        state = S_Line;
                        again = true;
                    }
                    break;

                case S_Line:
                    put_a_char(ch, &t);
                    if ((ch&0xFF) == '\n')
                        state = S_Start;
                    break;
                } /* switch */
            }
            while (again);
        }

        else if (formatting)
        {
            bool again;

            do
            {
                again = false;   /* default */

                switch (state)
                {
                case S_Start:
                    if ((ch&0xFF) == '\n')
                        *curr++ = ch;
                    else
                    {
                        state = S_StartFirstLine;
                        num_spaces = 0;
                        again = true;
                    }
                    break;

                case S_StartFirstLine:
                    if (ch == ' ')
                        ++num_spaces;

                    else
                    {
                        if (lformat_exclude > 0 && num_spaces >= lformat_exclude)
                        {
                            put_spaces(num_spaces);
                            num_spaces = 0;
                            state = S_FormatDisabled;
                            again = true;
                        }
                        else
                        {
                            *curr++ = CMD_PARA;
                            *curr++ = (char)num_spaces;
                            *curr++ = (char)num_spaces;
                            margin_pos = curr - 1;
                            state = S_FirstLine;
                            again = true;
                            in_para = true;
                        }
                    }
                    break;

                case S_FirstLine:
                    if (ch == '\n')
                    {
                        state = S_StartSecondLine;
                        num_spaces = 0;
                    }
                    else if (ch == ('\n'|0x100))    /* force end of para ? */
                    {
                        *curr++ = '\n';
                        in_para = false;
                        state = S_Start;
                    }
                    else if (ch == ' ')
                    {
                        state = S_FirstLineSpaces;
                        num_spaces = 1;
                    }
                    else
                        put_a_char(ch, &t);
                    break;

                case S_FirstLineSpaces:
                    if (ch == ' ')
                        ++num_spaces;
                    else
                    {
                        put_spaces(num_spaces);
                        state = S_FirstLine;
                        again = true;
                    }
                    break;

                case S_StartSecondLine:
                    if (ch == ' ')
                        ++num_spaces;
                    else if ((ch&0xFF) == '\n') /* a blank line means end of a para */
                    {
                        *curr++ = '\n';   /* end the para */
                        *curr++ = '\n';   /* for the blank line */
                        in_para = false;
                        state = S_Start;
                    }
                    else
                    {
                        if (lformat_exclude > 0 && num_spaces >= lformat_exclude)
                        {
                            *curr++ = '\n';
                            in_para = false;
                            put_spaces(num_spaces);
                            num_spaces = 0;
                            state = S_FormatDisabled;
                            again = true;
                        }
                        else
                        {
                            add_blank_for_split();
                            margin = num_spaces;
                            *margin_pos = (char)num_spaces;
                            state = S_Line;
                            again = true;
                        }
                    }
                    break;

                case S_Line:   /* all lines after the first */
                    if (ch == '\n')
                    {
                        state = S_StartLine;
                        num_spaces = 0;
                    }
                    else if (ch == ('\n' | 0x100))    /* force end of para ? */
                    {
                        *curr++ = '\n';
                        in_para = false;
                        state = S_Start;
                    }
                    else if (ch == ' ')
                    {
                        state = S_LineSpaces;
                        num_spaces = 1;
                    }
                    else
                        put_a_char(ch, &t);
                    break;

                case S_LineSpaces:
                    if (ch == ' ')
                        ++num_spaces;
                    else
                    {
                        put_spaces(num_spaces);
                        state = S_Line;
                        again = true;
                    }
                    break;

                case S_StartLine:   /* for all lines after the second */
                    if (ch == ' ')
                        ++num_spaces;
                    else if ((ch&0xFF) == '\n') /* a blank line means end of a para */
                    {
                        *curr++ = '\n';   /* end the para */
                        *curr++ = '\n';   /* for the blank line */
                        in_para = false;
                        state = S_Start;
                    }
                    else
                    {
                        if (num_spaces != margin)
                        {
                            *curr++ = '\n';
                            in_para = false;
                            state = S_StartFirstLine;  /* with current num_spaces */
                        }
                        else
                        {
                            add_blank_for_split();
                            state = S_Line;
                        }
                        again = true;
                    }
                    break;

                case S_FormatDisabled:
                    if (ch == ' ')
                    {
                        state = S_FormatDisabledSpaces;
                        num_spaces = 1;
                    }
                    else
                    {
                        if ((ch&0xFF) == '\n')
                            state = S_Start;
                        put_a_char(ch, &t);
                    }
                    break;

                case S_FormatDisabledSpaces:
                    if (ch == ' ')
                        ++num_spaces;
                    else
                    {
                        put_spaces(num_spaces);
                        num_spaces = 0;    /* is this needed? */
                        state = S_FormatDisabled;
                        again = true;
                    }
                    break;

                } /* switch (state) */
            }
            while (again);
        }

        else
        {
            bool again;
            do
            {
                again = false;   /* default */

                switch (state)
                {
                case S_Start:
                    if (ch == ' ')
                    {
                        state = S_Spaces;
                        num_spaces = 1;
                    }
                    else
                        put_a_char(ch, &t);
                    break;

                case S_Spaces:
                    if (ch == ' ')
                        ++num_spaces;
                    else
                    {
                        put_spaces(num_spaces);
                        num_spaces = 0;     /* is this needed? */
                        state = S_Start;
                        again = true;
                    }
                    break;
                } /* switch */
            }
            while (again);
        }

        CHK_BUFFER(0);
    } /* while ( 1 ) */

    fclose(srcfile);

    srcline = -1;
}


/*
 * stuff to resolve hot-link references.
 */


void make_hot_links()
/*
 * calculate topic_num/topic_off for each link.
 */
{
    LINK    *l;
    LABEL   *lbl;
    int      t;

    msg("Making hot-links.");

    /*
     * Calculate topic_num for all entries in DocContents.  Also set
     * "TF_IN_DOC" flag for all topics included in the document.
     */
    CONTENT *c = contents;
    for (int lctr = 0; lctr < num_contents; lctr++, c++)
    {
        for (int ctr = 0; ctr < c->num_topic; ctr++)
        {
            if (c->is_label[ctr])
            {
                lbl = find_label(c->topic_name[ctr]);
                if (lbl == nullptr)
                {
                    src_cfname = c->srcfile;
                    srcline = c->srcline;
                    error(0,"Cannot find DocContent label \"%s\".", c->topic_name[ctr]);
                    srcline = -1;
                }
                else
                {
                    if (topic[lbl->topic_num].flags & TF_DATA)
                    {
                        src_cfname = c->srcfile;
                        srcline = c->srcline;
                        error(0,"Label \"%s\" is a data-only topic.", c->topic_name[ctr]);
                        srcline = -1;
                    }
                    else
                    {
                        c->topic_num[ctr] = lbl->topic_num;
                        if (topic[lbl->topic_num].flags & TF_IN_DOC)
                            warn(0,"Topic \"%s\" appears in document more than once.",
                                 topic[lbl->topic_num].title);
                        else
                            topic[lbl->topic_num].flags |= TF_IN_DOC;
                    }
                }

            }
            else
            {
                t = find_topic_title(c->topic_name[ctr]);

                if (t == -1)
                {
                    src_cfname = c->srcfile;
                    srcline = c->srcline;
                    error(0,"Cannot find DocContent topic \"%s\".", c->topic_name[ctr]);
                    srcline = -1;  /* back to reality */
                }
                else
                {
                    c->topic_num[ctr] = t;
                    if (topic[t].flags & TF_IN_DOC)
                        warn(0,"Topic \"%s\" appears in document more than once.",
                             topic[t].title);
                    else
                        topic[t].flags |= TF_IN_DOC;
                }
            }
        }
    }

    /*
     * Find topic_num and topic_off for all hot-links.  Also flag all hot-
     * links which will (probably) appear in the document.
     */
    l=a_link;
    for (int lctr = 0; lctr < num_link; l++, lctr++)
    {
        switch (l->type)
        {
        case 0:      /* name is the title of the topic */
            t = find_topic_title(l->name);
            if (t == -1)
            {
                src_cfname = l->srcfile;
                srcline = l->srcline; /* pretend we are still in the source... */
                error(0,"Cannot find implicit hot-link \"%s\".", l->name);
                srcline = -1;  /* back to reality */
            }
            else
            {
                l->topic_num = t;
                l->topic_off = 0;
                l->doc_page = (topic[t].flags & TF_IN_DOC) ? 0 : -1;
            }
            break;

        case 1:  /* name is the name of a label */
            lbl = find_label(l->name);
            if (lbl == nullptr)
            {
                src_cfname = l->srcfile;
                srcline = l->srcline; /* pretend again */
                error(0,"Cannot find explicit hot-link \"%s\".", l->name);
                srcline = -1;
            }
            else
            {
                if (topic[lbl->topic_num].flags & TF_DATA)
                {
                    src_cfname = l->srcfile;
                    srcline = l->srcline;
                    error(0,"Label \"%s\" is a data-only topic.", l->name);
                    srcline = -1;
                }
                else
                {
                    l->topic_num = lbl->topic_num;
                    l->topic_off = lbl->topic_off;
                    l->doc_page  = (topic[lbl->topic_num].flags & TF_IN_DOC) ? 0 : -1;
                }
            }
            break;

        case 2:   /* it's a "special" link; topic_off already has the value */
            break;
        }
    }

}


/*
 * online help pagination stuff
 */


void add_page_break(TOPIC *t, int margin, char *text, char *start, char *curr, int num_links)
{
    PAGE p;

    p.offset = (unsigned)(start - text);
    p.length = (unsigned)(curr - start);
    p.margin = margin;
    add_page(t, &p);

    if (max_links < num_links)
        max_links = num_links;
}


void paginate_online()    /* paginate the text for on-line help */
{   /* also calculates max_pages and max_links */
    int       lnum;
    char     *start;
    char     *curr;
    char     *text;
    unsigned  len;
    int       num_links;
    int       col;
    int       tok;
    int       size,
              width;
    int       start_margin;

    msg("Paginating online help.");

    TOPIC *t = topic;
    for (int tctr = 0; tctr < num_topic; t++, tctr++)
    {
        if (t->flags & TF_DATA)
            continue;    /* don't paginate data topics */

        text = get_topic_text(t);
        curr = text;
        len  = t->text_len;

        start = curr;
        bool skip_blanks = false;
        lnum = 0;
        num_links = 0;
        col = 0;
        start_margin = -1;

        while (len > 0)
        {
            tok = find_token_length(ONLINE, curr, len, &size, &width);

            switch (tok)
            {
            case TOK_PARA:
            {
                int indent,
                    margin;

                ++curr;

                indent = *curr++;
                margin = *curr++;

                len -= 3;

                col = indent;

                while (1)
                {
                    tok = find_token_length(ONLINE, curr, len, &size, &width);

                    if (tok == TOK_DONE || tok == TOK_NL || tok == TOK_FF)
                        break;

                    if (tok == TOK_PARA)
                    {
                        col = 0;   /* fake a nl */
                        ++lnum;
                        break;
                    }

                    if (tok == TOK_XONLINE || tok == TOK_XDOC)
                    {
                        curr += size;
                        len -= size;
                        continue;
                    }

                    /* now tok is TOK_SPACE or TOK_LINK or TOK_WORD */

                    if (col+width > SCREEN_WIDTH)
                    {   /* go to next line... */
                        if (++lnum >= SCREEN_DEPTH)
                        {   /* go to next page... */
                            add_page_break(t, start_margin, text, start, curr, num_links);
                            start = curr + ((tok == TOK_SPACE) ? size : 0);
                            start_margin = margin;
                            lnum = 0;
                            num_links = 0;
                        }
                        if (tok == TOK_SPACE)
                            width = 0;   /* skip spaces at start of a line */

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

            case TOK_NL:
                if (skip_blanks && col == 0)
                {
                    start += size;
                    break;
                }
                ++lnum;
                if (lnum >= SCREEN_DEPTH || (col == 0 && lnum==SCREEN_DEPTH-1))
                {
                    add_page_break(t, start_margin, text, start, curr, num_links);
                    start = curr + size;
                    start_margin = -1;
                    lnum = 0;
                    num_links = 0;
                    skip_blanks = true;
                }
                col = 0;
                break;

            case TOK_FF:
                col = 0;
                if (skip_blanks)
                {
                    start += size;
                    break;
                }
                add_page_break(t, start_margin, text, start, curr, num_links);
                start_margin = -1;
                start = curr + size;
                lnum = 0;
                num_links = 0;
                break;

            case TOK_DONE:
            case TOK_XONLINE:   /* skip */
            case TOK_XDOC:      /* ignore */
            case TOK_CENTER:    /* ignore */
                break;

            case TOK_LINK:
                ++num_links;

            /* fall-through */

            default:    /* TOK_SPACE, TOK_LINK, TOK_WORD */
                skip_blanks = false;
                break;

            } /* switch */

            curr += size;
            len  -= size;
            col  += width;
        } /* while */

        if (!skip_blanks)
            add_page_break(t, start_margin, text, start, curr, num_links);

        if (max_pages < t->num_page)
            max_pages = t->num_page;

        release_topic_text(t, 0);
    } /* for */
}


/*
 * paginate document stuff
 */


#define CNUM           0
#define TNUM           1
#define LINK_DEST_WARN 2


struct PAGINATE_DOC_INFO
{
    int      cnum,  /* must match above #defines so pd_get_info() will work */
             tnum,
             link_dest_warn;

    char *start;
    CONTENT  *c;
    LABEL    *lbl;

};


LABEL *find_next_label_by_topic(int t)
{
    LABEL *temp, *g, *p;
    p = nullptr;
    g = p;
    temp = label;
    for (int ctr = 0; ctr < num_label; ctr++, temp++)
        if (temp->topic_num == t && temp->doc_page == -1)
        {
            g = temp;
            break;
        }
        else if (temp->topic_num > t)
            break;

    temp=plabel;
    for (int ctr = 0; ctr < num_plabel; ctr++, temp++)
        if (temp->topic_num == t && temp->doc_page == -1)
        {
            p = temp;
            break;
        }
        else if (temp->topic_num > t)
            break;

    if (p == nullptr)
        return (g);

    else if (g == nullptr)
        return (p);

    else
        return ((g->topic_off < p->topic_off) ? g : p);
}


void set_hot_link_doc_page()
/*
 * Find doc_page for all hot-links.
 */
{
    LINK  *l;
    LABEL *lbl;
    int    t;

    l=a_link;
    for (int lctr = 0; lctr < num_link; l++, lctr++)
    {
        switch (l->type)
        {
        case 0:      /* name is the title of the topic */
            t = find_topic_title(l->name);
            if (t == -1)
            {
                src_cfname = l->srcfile;
                srcline = l->srcline; /* pretend we are still in the source... */
                error(0,"Cannot find implicit hot-link \"%s\".", l->name);
                srcline = -1;  /* back to reality */
            }
            else
                l->doc_page = topic[t].doc_page;
            break;

        case 1:  /* name is the name of a label */
            lbl = find_label(l->name);
            if (lbl == nullptr)
            {
                src_cfname = l->srcfile;
                srcline = l->srcline; /* pretend again */
                error(0,"Cannot find explicit hot-link \"%s\".", l->name);
                srcline = -1;
            }
            else
                l->doc_page = lbl->doc_page;
            break;

        case 2:   /* special topics don't appear in the document */
            break;
        }
    }
}


void set_content_doc_page()
/*
 * insert page #'s in the DocContents
 */
{
    TOPIC   *t;
    char    *base;
    int      tnum;
    char     buf[4];
    int      len;

    tnum = find_topic_title(DOCCONTENTS_TITLE);
    assert(tnum>=0);
    t = &topic[tnum];

    base = get_topic_text(t);

    CONTENT *c = contents+1;
    for (int ctr = 1; ctr < num_contents; ctr++, c++)
    {
        assert(c->doc_page>=1);
        sprintf(buf, "%d", c->doc_page);
        len = (int) strlen(buf);
        assert(len<=3);
        memcpy(base+c->page_num_pos+(3-len), buf, len);
    }

    release_topic_text(t, 1);
}


/* this funtion also used by print_document() */
bool pd_get_info(int cmd, PD_INFO *pd, void *context)
{
    int *info = static_cast<int *>(context);
    CONTENT *c;

    switch (cmd)
    {
    case PD_GET_CONTENT:
        if (++info[CNUM] >= num_contents)
            return false;
        c = &contents[info[CNUM]];
        info[TNUM] = -1;
        pd->id       = c->id;
        pd->title    = c->name;
        pd->new_page = (c->flags & CF_NEW_PAGE) ? 1 : 0;
        return true;

    case PD_GET_TOPIC:
        c = &contents[info[CNUM]];
        if (++info[TNUM] >= c->num_topic)
            return false;
        pd->curr = get_topic_text(&topic[c->topic_num[info[TNUM]]]);
        pd->len = topic[c->topic_num[info[TNUM]]].text_len;
        return true;

    case PD_GET_LINK_PAGE:
        if (a_link[getint(pd->s)].doc_page == -1)
        {
            if (info[LINK_DEST_WARN])
            {
                src_cfname = a_link[getint(pd->s)].srcfile;
                srcline    = a_link[getint(pd->s)].srcline;
                warn(0,"Hot-link destination is not in the document.");
                srcline = -1;
            }
            return false;
        }
        pd->i = a_link[getint(pd->s)].doc_page;
        return true;

    case PD_RELEASE_TOPIC:
        c = &contents[info[CNUM]];
        release_topic_text(&topic[c->topic_num[info[TNUM]]], 0);
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
        info->c = &contents[info->cnum];
        return true;

    case PD_START_TOPIC:
        info->start = pd->curr;
        info->lbl = find_next_label_by_topic(info->c->topic_num[info->tnum]);
        return true;

    case PD_SET_SECTION_PAGE:
        info->c->doc_page = pd->pnum;
        return true;

    case PD_SET_TOPIC_PAGE:
        topic[info->c->topic_num[info->tnum]].doc_page = pd->pnum;
        return true;

    case PD_PERIODIC:
        while (info->lbl != nullptr && (unsigned)(pd->curr - info->start) >= info->lbl->topic_off)
        {
            info->lbl->doc_page = pd->pnum;
            info->lbl = find_next_label_by_topic(info->c->topic_num[info->tnum]);
        }
        return true;

    default:
        return false;
    }
}


void paginate_document()
{
    PAGINATE_DOC_INFO info;

    if (num_contents == 0)
        return ;

    msg("Paginating document.");

    info.tnum = -1;
    info.cnum = info.tnum;
    info.link_dest_warn = 1;

    process_document(pd_get_info, paginate_doc_output, &info);

    set_hot_link_doc_page();
    set_content_doc_page();
}


/*
 * label sorting stuff
 */

int fcmp_LABEL(VOIDCONSTPTR a, VOIDCONSTPTR b)
{
    char *an = ((LABEL *)a)->name,
          *bn = ((LABEL *)b)->name;
    int   diff;

    /* compare the names, making sure that the index goes first */

    if ((diff=strcmp(an,bn)) == 0)
        return (0);

    if (strcmp(an, INDEX_LABEL) == 0)
        return (-1);

    if (strcmp(bn, INDEX_LABEL) == 0)
        return (1);

    return (diff);
}


void sort_labels()
{
    qsort(label,  num_label,  sizeof(LABEL), fcmp_LABEL);
    qsort(plabel, num_plabel, sizeof(LABEL), fcmp_LABEL);
}


/*
 * file write stuff.
 */


/* returns true if different */
bool compare_files(FILE *f1, FILE *f2)
{
    if (filelength(fileno(f1)) != filelength(fileno(f2)))
        return true;   /* different if sizes are not the same */

    while (!feof(f1) && !feof(f2))
        if (getc(f1) != getc(f2))
            return true;

    return (feof(f1) && feof(f2)) ? false : true;
}


void _write_hdr(char *fname, FILE *file)
{
    char nfile[MAXFILE],
         next[MAXEXT];

    FNSPLIT(fname, nullptr, nullptr, nfile, next);
    fprintf(file, "\n/*\n * %s%s\n", nfile, next);
    FNSPLIT(src_fname, nullptr, nullptr, nfile, next);
    fprintf(file, " *\n * Contains #defines for help.\n *\n");
    fprintf(file, " * Generated by HC from: %s%s\n *\n */\n\n\n", nfile, next);

    fprintf(file, "/* current help file version */\n");
    fprintf(file, "\n");
    fprintf(file, "#define %-32s %3d\n", "FIHELP_VERSION", version);
    fprintf(file, "\n\n");

    fprintf(file, "/* labels */\n\n");

    for (int ctr = 0; ctr < num_label; ctr++)
        if (label[ctr].name[0] != '@')  /* if it's not a local label... */
        {
            fprintf(file, "#define %-32s %3d", label[ctr].name, ctr);
            if (strcmp(label[ctr].name, INDEX_LABEL) == 0)
                fprintf(file, "        /* index */");
            fprintf(file, "\n");
        }

    fprintf(file, "\n\n");
}


void write_hdr(char *fname)
{
    FILE *temp,
         *hdr;

    hdr = fopen(fname, "rt");

    if (hdr == nullptr)
    {   /* if no prev. hdr file generate a new one */
        hdr = fopen(fname, "wt");
        if (hdr == nullptr)
            fatal(0,"Cannot create \"%s\".", fname);
        msg("Writing: %s", fname);
        _write_hdr(fname, hdr);
        fclose(hdr);
        notice("FRACTINT must be re-compiled.");
        return ;
    }

    msg("Comparing: %s", fname);

    temp = fopen(TEMP_FNAME, "wt");

    if (temp == nullptr)
        fatal(0,"Cannot create temporary file: \"%s\".", TEMP_FNAME);

    _write_hdr(fname, temp);

    fclose(temp);
    temp = fopen(TEMP_FNAME, "rt");

    if (temp == nullptr)
        fatal(0,"Cannot open temporary file: \"%s\".", TEMP_FNAME);

    if (compare_files(temp, hdr))     /* if they are different... */
    {
        msg("Updating: %s", fname);
        fclose(temp);
        fclose(hdr);
        unlink(fname);               /* delete the old hdr file */
        rename(TEMP_FNAME, fname);   /* rename the temp to the hdr file */
        notice("FRACTINT must be re-compiled.");
    }
    else
    {   /* if they are the same leave the original alone. */
        fclose(temp);
        fclose(hdr);
        unlink(TEMP_FNAME);      /* delete the temp */
    }
}


void calc_offsets()    /* calc file offset to each topic */
{
    /* NOTE: offsets do NOT include 6 bytes for signature & version! */
    long offset = sizeof(int) +      /* max_pages */
             sizeof(int) +           /* max_links */
             sizeof(int) +           /* num_topic */
             sizeof(int) +           /* num_label */
             sizeof(int) +           /* num_contents */
             sizeof(int) +           /* num_doc_pages */
             num_topic*sizeof(long) +/* offsets to each topic */
             num_label*2*sizeof(int);/* topic_num/topic_off for all public labels */

    CONTENT *cp = contents;
    for (int c = 0; c < num_contents; c++, cp++)
        offset += sizeof(int) +       /* flags */
                  1 +                 /* id length */
                  (int) strlen(cp->id) +    /* id text */
                  1 +                 /* name length */
                  (int) strlen(cp->name) +  /* name text */
                  1 +                 /* number of topics */
                  cp->num_topic*sizeof(int);    /* topic numbers */

    TOPIC *tp = topic;
    for (int t = 0; t<num_topic; t++, tp++)
    {
        tp->offset = offset;
        offset += (long)sizeof(int) + /* topic flags */
                  sizeof(int) +       /* number of pages */
                  tp->num_page*3*sizeof(int) +   /* page offset, length & starting margin */
                  1 +                 /* length of title */
                  tp->title_len +     /* title */
                  sizeof(int) +       /* length of text */
                  tp->text_len;       /* text */
    }

}


void insert_real_link_info(char *curr, unsigned len)
/*
 * Replaces link indexes in the help text with topic_num, topic_off and
 * doc_page info.
 */
{
    int       size;
    int       tok;
    LINK     *l;

    while (len > 0)
    {
        tok = find_token_length(0, curr, len, &size, nullptr);

        if (tok == TOK_LINK)
        {
            l = &a_link[ getint(curr+1) ];
            setint(curr+1,l->topic_num);
            setint(curr+1+sizeof(int),l->topic_off);
            setint(curr+1+2*sizeof(int),l->doc_page);
        }

        len -= size;
        curr += size;
    }
}


void _write_help(FILE *file)
{
    char                 *text;
    TOPIC                *tp;
    CONTENT              *cp;
    help_sig_info  hs;

    /* write the signature and version */

    hs.sig = HELP_SIG; /* Edit line 17 of helpcom.h if this is a syntax error */
    hs.version = version;

    fwrite(&hs, sizeof(long)+sizeof(int), 1, file);

    /* write max_pages & max_links */

    putw(max_pages, file);
    putw(max_links, file);

    /* write num_topic, num_label and num_contents */

    putw(num_topic, file);
    putw(num_label, file);
    putw(num_contents, file);

    /* write num_doc_page */

    putw(num_doc_pages, file);

    /* write the offsets to each topic */

    for (int t = 0; t < num_topic; t++)
        fwrite(&topic[t].offset, sizeof(long), 1, file);

    /* write all public labels */

    for (int l = 0; l < num_label; l++)
    {
        putw(label[l].topic_num, file);
        putw(label[l].topic_off, file);
    }

    /* write contents */
    cp = contents;
    for (int c = 0; c < num_contents; c++, cp++)
    {
        putw(cp->flags, file);

        int t = (int) strlen(cp->id);
        putc((BYTE)t, file);
        fwrite(cp->id, 1, t, file);

        t = (int) strlen(cp->name);
        putc((BYTE)t, file);
        fwrite(cp->name, 1, t, file);

        putc((BYTE)cp->num_topic, file);
        fwrite(cp->topic_num, sizeof(int), cp->num_topic, file);
    }

    /* write topics */
    tp = topic;
    for (int t = 0; t < num_topic; t++, tp++)
    {
        /* write the topics flags */

        putw(tp->flags, file);

        /* write offset, length and starting margin for each page */

        putw(tp->num_page, file);
        for (int p = 0; p < tp->num_page; p++)
        {
            putw(tp->page[p].offset, file);
            putw(tp->page[p].length, file);
            putw(tp->page[p].margin, file);
        }

        /* write the help title */

        putc((BYTE)tp->title_len, file);
        fwrite(tp->title, 1, tp->title_len, file);

        /* insert hot-link info & write the help text */

        text = get_topic_text(tp);

        if (!(tp->flags & TF_DATA))     /* don't process data topics... */
            insert_real_link_info(text, tp->text_len);

        putw(tp->text_len, file);
        fwrite(text, 1, tp->text_len, file);

        release_topic_text(tp, 0);  /* don't save the text even though        */
        /* insert_real_link_info() modified it    */
        /* because we don't access the info after */
        /* this.                                  */

    }
}


void write_help(char *fname)
{
    FILE *hlp;

    hlp = fopen(fname, "wb");

    if (hlp == nullptr)
        fatal(0,"Cannot create .HLP file: \"%s\".", fname);

    msg("Writing: %s", fname);

    _write_help(hlp);

    fclose(hlp);
}


/*
 * print document stuff.
 */


struct PRINT_DOC_INFO
{

    /*
     * Note: Don't move these first three or pd_get_info will work not
     *       correctly.
     */

    int      cnum;
    int      tnum;
    int      link_dest_warn;   /* = 0 */

    FILE    *file;
    int      margin;
    int      start_of_line;
    int      spaces;
};


void printerc(PRINT_DOC_INFO *info, int c, int n)
{
    while (n-- > 0)
    {
        if (c==' ')
            ++info->spaces;

        else if (c=='\n' || c=='\f')
        {
            info->start_of_line = 1;
            info->spaces = 0;   /* strip spaces before a new-line */
            putc(c, info->file);
        }

        else
        {
            if (info->start_of_line)
            {
                info->spaces += info->margin;
                info->start_of_line = 0;
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


void printers(PRINT_DOC_INFO *info, const char *s, int n)
{
    if (n > 0)
    {
        while (n-- > 0)
            printerc(info, *s++, 1);
    }
    else
    {
        while (*s != '\0')
            printerc(info, *s++, 1);
    }
}


bool print_doc_output(int cmd, PD_INFO *pd, void *context)
{
    PRINT_DOC_INFO *info = static_cast<PRINT_DOC_INFO *>(context);
    switch (cmd)
    {
    case PD_HEADING:
    {
        char buff[20];

        info->margin = 0;
        printers(info, "\n                     Fractint Version xx.xx                     Page ", 0);
        sprintf(buff, "%d\n\n", pd->pnum);
        printers(info, buff, 0);
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


void print_document(const char *fname)
{
    PRINT_DOC_INFO info;

    if (num_contents == 0)
        fatal(0,".SRC has no DocContents.");

    msg("Printing to: %s", fname);

    info.tnum = -1;
    info.cnum = info.tnum;
    info.link_dest_warn = 0;

    if ((info.file = fopen(fname, "wt")) == nullptr)
        fatal(0,"Couldn't create \"%s\"", fname);

    info.margin = PAGE_INDENT;
    info.start_of_line = 1;
    info.spaces = 0;

    process_document(pd_get_info, print_doc_output, &info);

    fclose(info.file);
}


/*
 * compiler status and memory usage report stuff.
 */


void report_memory()
{
    long bytes_in_strings = 0,  /* bytes in strings */
         text   = 0,   /* bytes in topic text (stored on disk) */
         data   = 0,   /* bytes in active data structure */
         dead   = 0;   /* bytes in unused data structure */

    for (int ctr = 0; ctr < num_topic; ctr++)
    {
        data   += sizeof(TOPIC);
        bytes_in_strings += topic[ctr].title_len;
        text   += topic[ctr].text_len;
        data   += topic[ctr].num_page * sizeof(PAGE);

        dead   += (PAGE_ALLOC_SIZE-(topic[ctr].num_page%PAGE_ALLOC_SIZE)) * sizeof(PAGE);
    }

    for (int ctr = 0; ctr < num_link; ctr++)
    {
        data += sizeof(LINK);
        bytes_in_strings += (long) strlen(a_link[ctr].name);
    }

    if (num_link > 0)
        dead += (LINK_ALLOC_SIZE-(num_link%LINK_ALLOC_SIZE)) * sizeof(LINK);

    for (int ctr = 0; ctr < num_label; ctr++)
    {
        data   += sizeof(LABEL);
        bytes_in_strings += (long) strlen(label[ctr].name) + 1;
    }

    if (num_label > 0)
        dead += (LABEL_ALLOC_SIZE-(num_label%LABEL_ALLOC_SIZE)) * sizeof(LABEL);

    for (int ctr = 0; ctr < num_plabel; ctr++)
    {
        data   += sizeof(LABEL);
        bytes_in_strings += (long) strlen(plabel[ctr].name) + 1;
    }

    if (num_plabel > 0)
        dead += (LABEL_ALLOC_SIZE-(num_plabel%LABEL_ALLOC_SIZE)) * sizeof(LABEL);

    for (int ctr = 0; ctr < num_contents; ctr++)
    {
        int t;

        t = (MAX_CONTENT_TOPIC - contents[ctr].num_topic) *
            (sizeof(contents[0].is_label[0])   +
             sizeof(contents[0].topic_name[0]) +
             sizeof(contents[0].topic_num[0]));
        data += sizeof(CONTENT) - t;
        dead += t;
        bytes_in_strings += (long) strlen(contents[ctr].id) + 1;
        bytes_in_strings += (long) strlen(contents[ctr].name) + 1;
        for (int ctr2 = 0; ctr2 < contents[ctr].num_topic; ctr2++)
            bytes_in_strings += (long) strlen(contents[ctr].topic_name[ctr2]) + 1;
    }

    dead += (CONTENTS_ALLOC_SIZE-(num_contents%CONTENTS_ALLOC_SIZE)) * sizeof(CONTENT);

    printf("\n");
    printf("Memory Usage:\n");
    printf("%8ld Bytes in buffers.\n", (long)BUFFER_SIZE);
    printf("%8ld Bytes in strings.\n", bytes_in_strings);
    printf("%8ld Bytes in data.\n", data);
    printf("%8ld Bytes in dead space.\n", dead);
    printf("--------\n");
    printf("%8ld Bytes total.\n", (long)BUFFER_SIZE+bytes_in_strings+data+dead);
    printf("\n");
    printf("Disk Usage:\n");
    printf("%8ld Bytes in topic text.\n", text);
}


void report_stats()
{
    int  pages = 0;

    for (int t = 0; t < num_topic; t++)
        pages += topic[t].num_page;

    printf("\n");
    printf("Statistics:\n");
    printf("%8d Topics\n", num_topic);
    printf("%8d Links\n", num_link);
    printf("%8d Labels\n", num_label);
    printf("%8d Private labels\n", num_plabel);
    printf("%8d Table of contents (DocContent) entries\n", num_contents);
    printf("%8d Online help pages\n", pages);
    printf("%8d Document pages\n", num_doc_pages);
}


/*
 * add/delete help from .EXE functions.
 */


void add_hlp_to_exe(const char *hlp_fname, const char *exe_fname)
{
    int                  exe,   /* handles */
                         hlp;
    long                 len;
    int                  size;
    help_sig_info hs;

    if ((exe=open(exe_fname, O_RDWR|O_BINARY)) == -1)
        fatal(0,"Unable to open \"%s\"", exe_fname);

    if ((hlp=open(hlp_fname, O_RDONLY|O_BINARY)) == -1)
        fatal(0,"Unable to open \"%s\"", hlp_fname);

    msg("Appending %s to %s", hlp_fname, exe_fname);

    /* first, check and see if any help is currently installed */

    lseek(exe, filelength(exe) - sizeof(help_sig_info), SEEK_SET);

    read(exe, (char *)&hs, 10);

    if (hs.sig == HELP_SIG)
        warn(0,"Overwriting previous help. (Version=%d)", hs.version);
    else
        hs.base = filelength(exe);

    /* now, let's see if their help file is for real (and get the version) */

    read(hlp, (char *)&hs, sizeof(long)+sizeof(int));

    if (hs.sig != HELP_SIG)
        fatal(0,"Help signature not found in %s", hlp_fname);

    msg("Help file %s Version=%d", hlp_fname, hs.version);

    /* append the help stuff, overwriting old help (if any) */

    lseek(exe, hs.base, SEEK_SET);

    len = filelength(hlp) - sizeof(long) - sizeof(int); /* adjust for the file signature & version */

    for (int count = 0; count < len;)
    {
        size = (int) std::min((long)BUFFER_SIZE, len-count);
        read(hlp, buffer, size);
        write(exe, buffer, size);
        count += size;
    }

    /* add on the signature, version and offset */

    write(exe, (char *)&hs, 10);

    chsize(exe, lseek(exe,0L,SEEK_CUR));/* truncate if old help was longer */

    close(exe);
    close(hlp);
}


void delete_hlp_from_exe(const char *exe_fname)
{
    int   exe;   /* file handle */
    help_sig_info hs;

    if ((exe=open(exe_fname, O_RDWR|O_BINARY)) == -1)
        fatal(0,"Unable to open \"%s\"", exe_fname);

    msg("Deleting help from %s", exe_fname);

    /* see if any help is currently installed */

#ifndef XFRACT
    lseek(exe, filelength(exe) - 10, SEEK_SET);
    read(exe, (char *)&hs, 10);
#else
    lseek(exe, filelength(exe) - 12, SEEK_SET);
    read(exe, (char *)&hs, 12);
#endif

    if (hs.sig == HELP_SIG)
    {
        chsize(exe, hs.base);   /* truncate at the start of the help */
        close(exe);
    }
    else
    {
        close(exe);
        fatal(0,"No help found in %s", exe_fname);
    }
}


/*
 * command-line parser, etc.
 */


#define MODE_COMPILE 1
#define MODE_PRINT   2
#define MODE_APPEND  3
#define MODE_DELETE  4


int main(int argc, char *argv[])
{
    bool show_stats = false;
    bool show_mem = false;
    int    mode       = 0;

    char **arg;
    char   fname1[81],
           fname2[81];
    char   swappath[81];

    swappath[0] = 0;
    fname2[0] = swappath[0];
    fname1[0] = fname2[0];

    printf("HC - FRACTINT Help Compiler.\n\n");

    buffer = static_cast<char *>(malloc(BUFFER_SIZE));

    if (buffer == nullptr)
        fatal(0,"Not enough memory to allocate buffer.");

    for (arg= &argv[1]; argc>1; argc--, arg++)
    {
        switch ((*arg)[0])
        {
        case '/':
        case '-':
            switch ((*arg)[1])
            {
            case 'c':
                if (mode == 0)
                    mode = MODE_COMPILE;
                else
                    fatal(0,"Cannot have /c with /a, /d or /p");
                break;

            case 'a':
                if (mode == 0)
                    mode = MODE_APPEND;
                else
                    fatal(0,"Cannot have /a with /c, /d or /p");
                break;

            case 'd':
                if (mode == 0)
                    mode = MODE_DELETE;
                else
                    fatal(0,"Cannot have /d with /c, /a or /p");
                break;

            case 'p':
                if (mode == 0)
                    mode = MODE_PRINT;
                else
                    fatal(0,"Cannot have /p with /c, /a or /d");
                break;

            case 'm':
                if (mode == MODE_COMPILE)
                    show_mem = true;
                else
                    fatal(0,"/m switch allowed only when compiling (/c)");
                break;

            case 's':
                if (mode == MODE_COMPILE)
                    show_stats = true;
                else
                    fatal(0,"/s switch allowed only when compiling (/c)");
                break;

            case 'r':
                if (mode == MODE_COMPILE || mode == MODE_PRINT)
                    strcpy(swappath, (*arg)+2);
                else
                    fatal(0,"/r switch allowed when compiling (/c) or printing (/p)");
                break;

            case 'q':
                quiet_mode = true;
                break;

            default:
                fatal(0,"Bad command-line switch /%c", (*arg)[1]);
                break;
            }
            break;

        default:   /* assume it is a fname */
            if (fname1[0] == '\0')
                strcpy(fname1, *arg);
            else if (fname2[0] == '\0')
                strcpy(fname2, *arg);
            else
                fatal(0,"Unexpected command-line argument \"%s\"", *arg);
            break;
        } /* switch */
    } /* for */

    strupr(fname1);
    strupr(fname2);
    strupr(swappath);

    switch (mode)
    {
    case 0:
        printf("To compile a .SRC file:\n");
        printf("      HC /c [/s] [/m] [/r[path]] [src_file]\n");
        printf("         /s       = report statistics.\n");
        printf("         /m       = report memory usage.\n");
        printf("         /r[path] = set swap file path.\n");
        printf("         src_file = .SRC file.  Default is \"%s\"\n", DEFAULT_SRC_FNAME);
        printf("To print a .SRC file:\n");
        printf("      HC /p [/r[path]] [src_file] [out_file]\n");
        printf("         /r[path] = set swap file path.\n");
        printf("         src_file = .SRC file.  Default is \"%s\"\n", DEFAULT_SRC_FNAME);
        printf("         out_file = Filename to print to. Default is \"%s\"\n",
               DEFAULT_DOC_FNAME);
        printf("To append a .HLP file to an .EXE file:\n");
        printf("      HC /a [hlp_file] [exe_file]\n");
        printf("         hlp_file = .HLP file.  Default is \"%s\"\n", DEFAULT_HLP_FNAME);
        printf("         exe_file = .EXE file.  Default is \"%s\"\n", DEFAULT_EXE_FNAME);
        printf("To delete help info from an .EXE file:\n");
        printf("      HC /d [exe_file]\n");
        printf("         exe_file = .EXE file.  Default is \"%s\"\n", DEFAULT_EXE_FNAME);
        printf("\n");
        printf("Use \"/q\" for quiet mode. (No status messages.)\n");
        break;

    case MODE_COMPILE:
        if (fname2[0] != '\0')
            fatal(0,"Unexpected command-line argument \"%s\"", fname2);

        strcpy(src_fname, (fname1[0]=='\0') ? DEFAULT_SRC_FNAME : fname1);

        strcat(swappath, SWAP_FNAME);

        if ((swapfile=fopen(swappath, "w+b")) == nullptr)
            fatal(0,"Cannot create swap file \"%s\"", swappath);
        swappos = 0;

        read_src(src_fname);

        if (hdr_fname[0] == '\0')
            error(0,"No .H file defined.  (Use \"~HdrFile=\")");
        if (hlp_fname[0] == '\0')
            error(0,"No .HLP file defined.  (Use \"~HlpFile=\")");
        if (version == -1)
            warn(0,"No help version has been defined.  (Use \"~Version=\")");

        /* order of these is very important... */

        make_hot_links();  /* do even if errors since it may report */
        /* more... */

        if (!errors)     paginate_online();
        if (!errors)     paginate_document();
        if (!errors)     calc_offsets();
        if (!errors)     sort_labels();
        if (!errors)     write_hdr(hdr_fname);
        if (!errors)     write_help(hlp_fname);

        if (show_stats)
            report_stats();

        if (show_mem)
            report_memory();

        if (errors || warnings)
            report_errors();

        fclose(swapfile);
        remove(swappath);

        break;

    case MODE_PRINT:
        strcpy(src_fname, (fname1[0]=='\0') ? DEFAULT_SRC_FNAME : fname1);

        strcat(swappath, SWAP_FNAME);

        if ((swapfile=fopen(swappath, "w+b")) == nullptr)
            fatal(0,"Cannot create swap file \"%s\"", swappath);
        swappos = 0;

        read_src(src_fname);

        make_hot_links();

        if (!errors)     paginate_document();
        if (!errors)     print_document((fname2[0]=='\0') ? DEFAULT_DOC_FNAME : fname2);

        if (errors || warnings)
            report_errors();

        fclose(swapfile);
        remove(swappath);

        break;

    case MODE_APPEND:
        add_hlp_to_exe((fname1[0]=='\0') ? DEFAULT_HLP_FNAME : fname1,
                       (fname2[0]=='\0') ? DEFAULT_EXE_FNAME : fname2);
        break;

    case MODE_DELETE:
        if (fname2[0] != '\0')
            fatal(0,"Unexpected argument \"%s\"", fname2);
        delete_hlp_from_exe((fname1[0]=='\0') ? DEFAULT_EXE_FNAME : fname1);
        break;
    }

    free(buffer);

    return (errors);     /* return the number of errors */
}

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4311)
#endif
void check_buffer(char *current, unsigned off, char *buffer)
{
    if ((unsigned)(curr + off - buffer) >= (BUFFER_SIZE-1024))
    {
        fatal(0, "Buffer overflowerd -- Help topic too large.");
    }
}
#if defined(_WIN32)
#pragma warning(pop)
#endif
