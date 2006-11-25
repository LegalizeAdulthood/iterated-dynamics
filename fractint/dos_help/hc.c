
/*
 * hc.c
 *
 * Stand-alone FRACTINT help compiler.  Compile in the COMPACT memory model.
 *
 * See HC.DOC for source file syntax.
 *
 *
 * Revision History:
 *
 *   02-26-91 EAN     Initial version.
 *
 *   03-21-91 EAN     Modified for automatic paragraph formatting.
 *                    Added several new commands:
 *                       Format[+/-]  Enable/disable paragraph formatting
 *                       Doc[+/-]     Enable/disable output to document.
 *                       Online[+/-]  Enable/disable output to online help.
 *                       Label=       Defines a label. Replaces ~(...)
 *                       FF           Forces a form-feed.  Replaces ~~
 *                       FormatExclude=val Exclude lines past val from
 *                                    formatting.  If before any topic sets
 *                                    global default, otherwise set local.
 *                       FormatExclude= Set to global default.
 *                       FormatExclude=n Disable exclusion. (global or local)
 *                       FormatExclude[+/-] Enable/disable format exclusion.
 *                       Center[+/-]  Enable/disable centering of text.
 *                       \ before nl  Forces the end of a paragraph
 *                    Support for commands embedded in text with new
 *                    ~(...) format.
 *                    Support for multiple commands on a line separated by
 *                    commas.
 *                    Support for implict links; explicit links must now
 *                    start with an equal sign.
 *   04-03-91 EAN     Added "include" command (works like #include)
 *   04-10-91 EAN     Added support for "data" topics.
 *                    Added Comment/EndComment commands for multi-line
 *                       comments.
 *                    Added CompressSpaces[+/-] command.
 *                    Added DocContents command for document printing.
 *                    Added BinInc command which includes a binary file
 *                       in a data topic.
 *                    Fixed tables to flow down instead of across the page.
 *                       Makes no allowances for page breaks within tables.
 *   11-03-94 TIW     Increased buffer size.
 *
 */


#define HC_C

#define INCLUDE_COMMON  /* tell helpcom.h to include common code */


#ifdef XFRACT
#define strupr strlwr
#else
#include <io.h>
#endif

#if defined(_WIN32)
#include <io.h>
#endif

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#ifdef __TURBOC__
#   include <dir.h>
#   define FNSPLIT fnsplit
#else
#   define MAXFILE _MAX_FNAME
#   define MAXEXT  _MAX_EXT
#   define FNSPLIT _splitpath
#endif


#include <assert.h>
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "helpcom.h"

#ifdef XFRACT
#ifndef HAVESTRI
extern int stricmp(char *, char *);
extern int strnicmp(char *, char *, int);
#endif
extern int filelength(int);
extern int _splitpath(char *,char *,char *,char *,char *);
#endif

/*
 * When defined, SHOW_ERROR_LINE will cause the line number in HC.C where
 * errors/warnings/messages are generated to be displayed at the start of
 * the line.
 *
 * Used when debugging HC.  Also useful for finding the line (in HC.C) that
 * generated a error or warning.
 */

#ifndef XFRACT
#define SHOW_ERROR_LINE
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

#define INDEX_LABEL       "HELP_INDEX"
#define DOCCONTENTS_TITLE "DocContent"



/* #define BUFFER_SIZE   (24*1024) */
#define BUFFER_SIZE   (30*1024)


typedef struct
   {
   int      type;            /* 0 = name is topic title, 1 = name is label, */
                             /*   2 = "special topic"; name is NULL and */
                             /*   topic_num/topic_off is valid */
   int      topic_num;       /* topic number to link to */
   unsigned topic_off;       /* offset into topic to link to */
   int      doc_page;        /* document page # to link to */
   char    *name;            /* name of label or title of topic to link to */
   char    *srcfile;         /* .SRC file link appears in */
   int      srcline;         /* .SRC file line # link appears in */
   } LINK;


typedef struct
   {
   unsigned offset;     /* offset from start of topic text */
   unsigned length;     /* length of page (in chars) */
   int      margin;     /* if > 0 then page starts in_para and text */
                        /* should be indented by this much */
   } PAGE;


/* values for TOPIC.flags */

#define TF_IN_DOC  (1)       /* 1 if topic is part of the printed document */
#define TF_DATA    (2)       /* 1 if it is a "data" topic */


typedef struct
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
   } TOPIC;


typedef struct
   {
   char    *name;            /* its name */
   int      topic_num;       /* topic number */
   unsigned topic_off;       /* offset of label in the topic's text */
   int      doc_page;
   } LABEL;


/* values for CONTENT.flags */

#define CF_NEW_PAGE  (1)     /* true if section starts on a new page */


#define MAX_CONTENT_TOPIC (10)


typedef struct
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
   } CONTENT;


struct help_sig_info
   {
   unsigned long sig;
   int           version;
   unsigned long base;
   } ;


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

int      quiet_mode       = 0;    /* true if "/Q" option used */

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
char    *src_cfname       = NULL; /* current .SRC filename */

int      format_exclude   = 0;    /* disable formatting at this col, 0 to */
                                  /*    never disable formatting */
FILE    *swapfile;
long     swappos;

char    *buffer;                  /* alloc'ed as BUFFER_SIZE bytes */
char    *curr;                    /* current position in the buffer */
char     cmd[128];                /* holds the current command */
int      compress_spaces;
int      xonline;
int      xdoc;

#define  MAX_INCLUDE_STACK (5)    /* allow 5 nested includes */

struct
   {
   char *fname;
   FILE *file;
   int   line;
   int   col;
   } include_stack[MAX_INCLUDE_STACK];
int include_stack_top = -1;

void check_buffer(char *current, unsigned off, char *buffer);

#define CHK_BUFFER(off) check_buffer(curr, off, buffer)

#ifdef __WATCOMC__
#define putw( x1, x2 )  fprintf( x2, "%c%c", x1&0xFF, x1>>8 );
#endif

#ifdef XFRACT
#define putw( x1, x2 )  fwrite( &(x1), 1, sizeof(int), x2);
#endif

/*
 * error/warning/message reporting functions.
 */


void report_errors(void)
   {
   printf("\n");
   printf("Compiler Status:\n");
   printf("%8d Error%c\n",       errors,   (errors==1)   ? ' ' : 's');
   printf("%8d Warning%c\n",     warnings, (warnings==1) ? ' ' : 's');
   }


void print_msg(char *type, int lnum, char *format, va_list arg)
   {
   if (type != NULL)
      {
      printf("   %s", type);
      if (lnum>0)
         printf(" %s %d", src_cfname, lnum);
      printf(": ");
      }
   vprintf(format, arg);
   printf("\n");
   }


#ifndef USE_VARARGS
void fatal(int diff, char *format, ...)
#else
void fatal(va_alist)
    va_dcl
#endif
   {
   va_list arg;

#ifndef USE_VARARGS
   va_start(arg, format);
#else
   int diff;
   char *format;
   va_start(arg);
   diff = va_arg(arg,int);
   format = va_arg(arg,char *);
#endif

   print_msg("Fatal", srcline-diff, format, arg);
   va_end(arg);

   if ( errors || warnings )
      report_errors();

   exit( errors + 1 );
   }


#ifndef USE_VARARGS
void error(int diff, char *format, ...)
#else
void error(va_alist)
    va_dcl
#endif
   {
   va_list arg;

#ifndef USE_VARARGS
   va_start(arg, format);
#else
   int diff;
   char *format;
   va_start(arg);
   diff = va_arg(arg,int);
   format = va_arg(arg,char *);
#endif
   print_msg("Error", srcline-diff, format, arg);
   va_end(arg);

   if (++errors >= MAX_ERRORS && MAX_ERRORS > 0)
      fatal(0,"Too many errors!");
   }


#ifndef USE_VARARGS
void warn(int diff, char *format, ...)
#else
void warn(va_alist)
   va_dcl
#endif
   {
   va_list arg;
#ifndef USE_VARARGS
   va_start(arg, format);
#else
   int diff;
   char *format;
   va_start(arg);
   diff = va_arg(arg, int);
   format = va_arg(arg, char *);
#endif
   print_msg("Warning", srcline-diff, format, arg);
   va_end(arg);

   if (++warnings >= MAX_WARNINGS && MAX_WARNINGS > 0)
      fatal(0,"Too many warnings!");
   }


#ifndef USE_VARARGS
void notice(char *format, ...)
#else
void notice(va_alist)
    va_dcl
#endif
   {
   va_list arg;
#ifndef USE_VARARGS
   va_start(arg, format);
#else
   char *format;

   va_start(arg);
   format = va_arg(arg,char *);
#endif
   print_msg("Note", srcline, format, arg);
   va_end(arg);
   }


#ifndef USE_VARARGS
void msg(char *format, ...)
#else
void msg(va_alist)
va_dcl
#endif
   {
   va_list arg;
#ifdef USE_VARARGS
   char *format;
#endif

   if (quiet_mode)
      return;
#ifndef USE_VARARGS
   va_start(arg, format);
#else
   va_start(arg);
   format = va_arg(arg,char *);
#endif
   print_msg(NULL, 0, format, arg);
   va_end(arg);
   }


#ifdef SHOW_ERROR_LINE
#   define fatal  (printf("[%04d] ", __LINE__), fatal)
#   define error  (printf("[%04d] ", __LINE__), error)
#   define warn   (printf("[%04d] ", __LINE__), warn)
#   define notice (printf("[%04d] ", __LINE__), notice)
#   define msg    (printf((quiet_mode)?"":"[%04d] ", __LINE__), msg)
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
   if ( save )
      {
      fseek(swapfile, t->text, SEEK_SET);
      fwrite(buffer, 1, t->text_len, swapfile);
      }
   }


/*
 * memory-allocation functions.
 */


#define new(item)    (item *)newx(sizeof(item))
#define delete(item) free(item)


VOIDPTR newx(unsigned size)
   {
   VOIDPTR ptr;

   ptr = malloc(size);

   if (ptr == NULL)
      fatal(0,"Out of memory!");

   return (ptr);
   }


VOIDPTR renewx(VOIDPTR ptr, unsigned size)
   {
   ptr = realloc(ptr, size);

   if (ptr == NULL)
      fatal(0,"Out of memory!");

   return (ptr);
   }


char *dupstr(char *s, unsigned len)
   {
   char *ptr;

   if (len == 0)
      len = (int) strlen(s) + 1;

   ptr = newx(len);

   memcpy(ptr, s, len);

   return (ptr);
   }


#define LINK_ALLOC_SIZE (16)


int add_link(LINK *l)
   {
   if (num_link == 0)
      a_link = newx( sizeof(LINK)*LINK_ALLOC_SIZE );

   else if (num_link%LINK_ALLOC_SIZE == 0)
      a_link = renewx(a_link, sizeof(LINK) * (num_link+LINK_ALLOC_SIZE) );

   a_link[num_link] = *l;

   return( num_link++ );
   }


#define PAGE_ALLOC_SIZE (4)


int add_page(TOPIC *t, PAGE *p)
   {
   if (t->num_page == 0)
      t->page = newx( sizeof(PAGE)*PAGE_ALLOC_SIZE );

   else if (t->num_page%PAGE_ALLOC_SIZE == 0)
      t->page = renewx(t->page, sizeof(PAGE) * (t->num_page+PAGE_ALLOC_SIZE) );

   t->page[t->num_page] = *p;

   return ( t->num_page++ );
   }


#define TOPIC_ALLOC_SIZE (16)


int add_topic(TOPIC *t)
   {
   if (num_topic == 0)
      topic = newx( sizeof(TOPIC)*TOPIC_ALLOC_SIZE );

   else if (num_topic%TOPIC_ALLOC_SIZE == 0)
      topic = renewx(topic, sizeof(TOPIC) * (num_topic+TOPIC_ALLOC_SIZE) );

   topic[num_topic] = *t;

   return ( num_topic++ );
   }


#define LABEL_ALLOC_SIZE (16)


int add_label(LABEL *l)
   {
   if (l->name[0] == '@')    /* if it's a private label... */
      {
      if (num_plabel == 0)
         plabel = newx( sizeof(LABEL)*LABEL_ALLOC_SIZE );

      else if (num_plabel%LABEL_ALLOC_SIZE == 0)
         plabel = renewx(plabel, sizeof(LABEL) * (num_plabel+LABEL_ALLOC_SIZE) );

      plabel[num_plabel] = *l;

      return ( num_plabel++ );
      }
   else
      {
      if (num_label == 0)
         label = newx( sizeof(LABEL)*LABEL_ALLOC_SIZE );

      else if (num_label%LABEL_ALLOC_SIZE == 0)
         label = renewx(label, sizeof(LABEL) * (num_label+LABEL_ALLOC_SIZE) );

      label[num_label] = *l;

      return ( num_label++ );
      }
   }


#define CONTENTS_ALLOC_SIZE (16)


int add_content(CONTENT *c)
   {
   if (num_contents == 0)
      contents = newx( sizeof(CONTENT)*CONTENTS_ALLOC_SIZE );

   else if (num_contents%CONTENTS_ALLOC_SIZE == 0)
      contents = renewx(contents, sizeof(CONTENT) * (num_contents+CONTENTS_ALLOC_SIZE) );

   contents[num_contents] = *c;

   return ( num_contents++ );
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


int eos(void)    /* end-of-source ? */
   {
   return ( !((read_char_sp==0) && (read_char_buff_pos==0) && feof(srcfile)) );
   }


int _read_char(void)
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
      return ( read_char_buff[read_char_buff_pos--] );
      }

   if (read_char_sp > 0)
      {
      --read_char_sp;
      return (' ');
      }

   if ( feof(srcfile) )
      return (-1);

   while (1)
      {
      ch = getc(srcfile);

      switch (ch)
         {
         case '\t':    /* expand a tab */
            {
            int diff = ( ( (srccol/8) + 1 ) * 8 ) - srccol;

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


int read_char(void)
   {
   int ch;

   ch = _read_char();

   while (ch == ';' && srccol==1)    /* skip over comments */
      {
      ch = _read_char();

      while (ch!='\n' && ch!=-1 )
         ch = _read_char();

      ch = _read_char();
      }

   if (ch == '\\')   /* process an escape code */
      {
      ch = _read_char();

      if (ch >= '0' && ch <= '9')
         {
         char buff[4];
         int  ctr;

         for (ctr=0; ; ctr++)
            {
            if ( ch<'0' || ch>'9' || ch==-1 || ctr>=3 )
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

   if ( (ch & 0xFF) == 0 )
      {
      error(0,"Null character (\'\\0\') not allowed!");
      ch = 0x1FF; /* since we've had an error the file will not be written; */
                  /*   the value we return doesn't really matter */
      }

   return(ch);
   }


/*
 * misc. search functions.
 */


LABEL *find_label(char *name)
   {
   int    l;
   LABEL *lp;

   if (*name == '@')
      {
      for (l=0, lp=plabel; l<num_plabel; l++, lp++)
         if ( strcmp(name, lp->name) == 0 )
            return (lp);
      }
   else
      {
      for (l=0, lp=label; l<num_label; l++, lp++)
         if ( strcmp(name, lp->name) == 0 )
            return (lp);
      }

   return (NULL);
   }


int find_topic_title(char *title)
   {
   int t;
   int len;

   while (*title == ' ')
      ++title;

   len = (int) strlen(title) - 1;
   while ( title[len] == ' ' && len > 0 )
      --len;

   ++len;

   if ( len > 2 && title[0] == '\"' && title[len-1] == '\"' )
      {
      ++title;
      len -= 2;
      }

   for (t=0; t<num_topic; t++)
      if ( (int) strlen(topic[t].title) == len &&
           strnicmp(title, topic[t].title, len) == 0 )
         return (t);

   return (-1);   /* not found */
   }


/*
 * .SRC file parser stuff
 */


int validate_label_name(char *name)
   {
   if ( !isalpha(*name) && *name!='@' && *name!='_' )
      return (0);  /* invalid */

   while (*(++name) != '\0')
      if ( !isalpha(*name) && !isdigit(*name) && *name!='_' )
         return(0);  /* invalid */

   return (1);  /* valid */
   }


char *read_until(char *buff, int len, char *stop_chars)
   {
   int ch;

   while ( --len > 0 )
      {
      ch = read_char();

      if ( ch == -1 )
         {
         *buff++ = '\0';
         break;
         }

      if ( (ch&0xFF) <= MAX_CMD )
         *buff++ = CMD_LITERAL;

      *buff++ = ch;

      if ( (ch&0x100)==0 && strchr(stop_chars, ch) != NULL )
         break;
      }

   return ( buff-1 );
   }


void skip_over(char *skip)
   {
   int ch;

   while (1)
      {
      ch = read_char();

      if ( ch == -1 )
         break;

      else if ( (ch&0x100) == 0 && strchr(skip, ch) == NULL )
         {
         unread_char(ch);
         break;
         }
      }
   }


char *pchar(int ch)
   {
   static char buff[16];

   if ( ch >= 0x20 && ch <= 0x7E )
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


int get_next_item(void)   /* used by parse_contents() */
   {
   int   last;
   char *ptr;

   skip_over(" \t\n");
   ptr = read_until(cmd, 128, ",}");
   last = (*ptr == '}');
   --ptr;
   while ( ptr >= cmd && strchr(" \t\n",*ptr) )   /* strip trailing spaces */
      --ptr;
   *(++ptr) = '\0';

   return (last);
   }


void process_contents(void)
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
         int last;

         c.flags = 0;
         c.num_topic = 0;
         c.doc_page = -1;
         c.srcfile = src_cfname;
         c.srcline = srcline;

         if ( get_next_item() )
            {
            error(0,"Unexpected end of DocContent entry.");
            continue;
            }
         c.id = dupstr(cmd,0);

         if ( get_next_item() )
            {
            error(0,"Unexpected end of DocContent entry.");
            continue;
            }
         indent = atoi(cmd);

         last = get_next_item();

         if ( cmd[0] == '\"' )
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
         while ( (ptr-curr) < PAGE_WIDTH-10 )
            *ptr++ = '.';
         c.page_num_pos = (unsigned) ( (ptr-3) - buffer );
         curr = ptr;

         while (!last)
            {
            last = get_next_item();

            if ( stricmp(cmd, "FF") == 0 )
               {
               if ( c.flags & CF_NEW_PAGE )
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

            if ( ++c.num_topic >= MAX_CONTENT_TOPIC )
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

   alloc_topic_text(&t, (unsigned) (curr - buffer) );
   add_topic(&t);
   }


int parse_link(void)   /* returns length of link or 0 on error */
   {
   char *ptr;
   char *end;
   int   bad = 0;
   int   len;
   LINK  l;
   int   lnum;
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

      if (ptr == NULL)
         ptr = end;
      else
         *ptr++ = '\0';

      len = (int) (end - ptr);

      if ( cmd[1] == '-' )
         {
         l.type      = 2;          /* type 2 = "special" */
         l.topic_num = atoi(cmd+1);
         l.topic_off = 0;
         l.name      = NULL;
         }
      else
         {
         l.type = 1;           /* type 1 = to a label */
         if ((int)strlen(cmd) > 32)
            warn(err_off, "Label is long.");
         if (cmd[1] == '\0')
            {
            error(err_off, "Explicit hot-link has no Label.");
            bad = 1;
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
      len = (int) (end - ptr);
      if (len == 0)
         {
         error(err_off, "Implicit hot-link has no title.");
         bad = 1;
         }
      l.name = dupstr(ptr,len+1);
      l.name[len] = '\0';
      }

   if ( !bad )
      {
      CHK_BUFFER(1+3*sizeof(int)+len+1);
      lnum = add_link(&l);
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


int create_table(void)
   {
   char  *ptr;
   int    width;
   int    cols;
   int    start_off;
   int    first_link;
   int    rows;
   int    r, c;
   int    ch;
   int    done;
   int    len;
   int    lnum;
   int    count;
   char  *title[MAX_TABLE_SIZE];
   char  *table_start;

   ptr = strchr(cmd, '=');

   if (ptr == NULL)
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

   done = 0;

   first_link = num_link;
   table_start = curr;
   count = 0;

   /* first, read all the links in the table */

   do
      {

      do
         ch = read_char();
      while ( ch=='\n' || ch == ' ' );

      if (done)
         break;

      switch (ch)
         {
         case -1:
            error(0,"Unexpected EOF in a Table.");
            return(0);

         case '{':
            if (count >= MAX_TABLE_SIZE)
               fatal(0,"Table is too large.");
            len = parse_link();
            curr = table_start;   /* reset to the start... */
            title[count] = dupstr(curr+3*sizeof(int)+1, len+1);
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
            int imbedded;

            ch = read_char();

            if (ch=='(')
               imbedded = 1;
            else
               {
               imbedded = 0;
               unread_char(ch);
               }

            ptr = read_until(cmd, 128, ")\n,");

            ch = *ptr;
            *ptr = '\0';

            if  ( stricmp(cmd, "EndTable") == 0 )
               done = 1;
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

   rows = 1 + ( count / cols );

   for (r=0; r<rows; r++)
      {
      put_spaces(start_off);
      for (c=0; c<cols; c++)
         {
         lnum = c*rows + r;

         if ( first_link+lnum >= num_link )
            break;

         len = (int) strlen(title[lnum]);
         *curr++ = CMD_LINK;
         setint(curr,first_link+lnum);
         curr += 3*sizeof(int);
         memcpy(curr, title[lnum], len);
         curr += len;
         *curr++ = CMD_LINK;

         delete(title[lnum]);

         if ( c < cols-1 )
            put_spaces( width-len );
         }
      *curr++ = '\n';
      }

   return (1);
   }


void process_comment(void)
   {
   int ch;

   while ( 1 )
      {
      ch = read_char();

      if (ch == '~')
         {
         int   imbedded;
         char *ptr;

         ch = read_char();

         if (ch=='(')
            imbedded = 1;
         else
            {
            imbedded = 0;
            unread_char(ch);
            }

         ptr = read_until(cmd, 128, ")\n,");

         ch = *ptr;
         *ptr = '\0';

         if  ( stricmp(cmd, "EndComment") == 0 )
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

      else if ( ch == -1 )
         {
         error(0,"Unexpected EOF in Comment");
         break;
         }
      }
   }


void process_bininc(void)
   {
   int  handle;
   long len;

   if ( (handle=open(cmd+7, O_RDONLY|O_BINARY)) == -1 )
      {
      error(0,"Unable to open \"%s\"", cmd+7);
      return ;
      }

   len = filelength(handle);

   if ( len >= BUFFER_SIZE )
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


void start_topic(TOPIC *t, char *title, int title_len)
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
   alloc_topic_text(t, (unsigned) (curr - buffer) );
   add_topic(t);
   }


int end_of_sentence(char *ptr)  /* true if ptr is at the end of a sentence */
   {
   if ( *ptr == ')')
      --ptr;

   if ( *ptr == '\"')
      --ptr;

   return ( *ptr=='.' || *ptr=='?' || *ptr=='!' );
   }


void add_blank_for_split(void)   /* add space at curr for merging two lines */
   {
   if ( !is_hyphen(curr-1) )   /* no spaces if it's a hyphen */
      {
      if ( end_of_sentence(curr-1) )
         *curr++ = ' ';  /* two spaces at end of a sentence */
      *curr++ = ' ';
      }
   }


void put_a_char(int ch, TOPIC *t)
   {
   if (ch == '{' && !(t->flags & TF_DATA) )   /* is it a hot-link? */
      parse_link();
   else
      {
      if ( (ch&0xFF) <= MAX_CMD)
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
   char  *margin_pos = NULL;
   int    in_topic   = 0,
          formatting = 1,
          state      = S_Start,
          num_spaces = 0,
          margin     = 0,
          in_para    = 0,
          centering  = 0,
          lformat_exclude = format_exclude,
          again;

   xonline = xdoc = 0;

   src_cfname = fname;

   if ( (srcfile = fopen(fname, "rt")) == NULL )
      fatal(0,"Unable to open \"%s\"", fname);

   msg("Compiling: %s", fname);

   in_topic = 0;

   curr = buffer;

   while ( 1 )
      {

      ch = read_char();

      if ( ch == -1 )   /* EOF? */
         {
         if ( include_stack_top >= 0)
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
         int imbedded;
         int eoff;
         int done;

         ch = read_char();
         if (ch == '(')
            {
            imbedded = 1;
            eoff = 0;
            }
         else
            {
            imbedded = 0;
            eoff=0;
            unread_char(ch);
            }

         done = 0;

         while ( !done )
            {
            do
               ch = read_char();
            while (ch == ' ');
            unread_char(ch);

            if (imbedded)
               ptr = read_until(cmd, 128, ")\n,");
            else
               ptr = read_until(cmd, 128, "\n,");

            done = 1;

            if ( *ptr == '\0' )
               {
               error(0,"Unexpected EOF in command.");
               break;
               }

            if (*ptr == '\n')
               ++eoff;

            if ( imbedded && *ptr == '\n' )
               error(eoff,"Imbedded command has no closing parend (\')\')");

            done = (*ptr != ',');   /* we done if it's not a comma */

            if ( *ptr != '\n' && *ptr != ')' && *ptr != ',' )
               {
               error(0,"Command line too long.");
               break;
               }

            *ptr = '\0';


            /* commands allowed anytime... */

            if ( strnicmp(cmd, "Topic=", 6) == 0 )
               {
               if (in_topic)  /* if we're in a topic, finish it */
                  end_topic(&t);
               else
                  in_topic = 1;

               if (cmd[6] == '\0')
                  warn(eoff,"Topic has no title.");

               else if ((int)strlen(cmd+6) > 70)
                  error(eoff,"Topic title is too long.");

               else if ((int)strlen(cmd+6) > 60)
                  warn(eoff,"Topic title is long.");

               if ( find_topic_title(cmd+6) != -1 )
                  error(eoff,"Topic title already exists.");

               start_topic(&t, cmd+6, (unsigned)(ptr-(cmd+6)));
               formatting = 1;
               centering = 0;
               state = S_Start;
               in_para = 0;
               num_spaces = 0;
               xonline = xdoc = 0;
               lformat_exclude = format_exclude;
               compress_spaces = 1;
               continue;
               }

            else if ( strnicmp(cmd, "Data=", 5) == 0 )
               {
               if (in_topic)  /* if we're in a topic, finish it */
                  end_topic(&t);
               else
                  in_topic = 1;

               if (cmd[5] == '\0')
                  warn(eoff,"Data topic has no label.");

               if ( !validate_label_name(cmd+5) )
                  {
                  error(eoff,"Label \"%s\" contains illegal characters.", cmd+5);
                  continue;
                  }

               if ( find_label(cmd+5) != NULL )
                  {
                  error(eoff,"Label \"%s\" already exists", cmd+5);
                  continue;
                  }

               if ( cmd[5] == '@' )
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

               formatting = 0;
               centering = 0;
               state = S_Start;
               in_para = 0;
               num_spaces = 0;
               xonline = xdoc = 0;
               lformat_exclude = format_exclude;
               compress_spaces = 0;
               continue;
               }

            else if ( strnicmp(cmd, "DocContents", 11) == 0 )
               {
               check_command_length(eoff, 11);
               if (in_topic)  /* if we're in a topic, finish it */
                  end_topic(&t);
               if (!done)
                  {
                  if (imbedded)
                     unread_char('(');
                  unread_char('~');
                  done = 1;
                  }
               compress_spaces = 1;
               process_contents();
               in_topic = 0;
               continue;
               }

            else if ( stricmp(cmd, "Comment") == 0 )
               {
               process_comment();
               continue;
               }

            else if ( strnicmp(cmd, "FormatExclude", 13) == 0 )
               {
               if (cmd[13] == '-')
                  {
                  check_command_length(eoff, 14);
                  if ( in_topic )
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
                  if ( in_topic )
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
                     int n = ( ( (in_topic) ? lformat_exclude : format_exclude) < 0 ) ? -1 : 1;

                     lformat_exclude = atoi(cmd+14);

                     if ( lformat_exclude <= 0 )
                        {
                        error(eoff,"Invalid argument to FormatExclude=");
                        lformat_exclude = 0;
                        }

                     lformat_exclude *= n;

                     if ( !in_topic )
                        format_exclude = lformat_exclude;
                     }
                  }
               else
                  error(eoff,"Invalid format for FormatExclude");

               continue;
               }

            else if ( strnicmp(cmd, "Include ", 8) == 0 )
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
                  if ( (srcfile = fopen(cmd+8, "rt")) == NULL )
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

            if ( !in_topic )
               {
               if ( strnicmp(cmd, "HdrFile=", 8) == 0 )
                  {
                  if (hdr_fname[0] != '\0')
                     warn(eoff,"Header Filename has already been defined.");
                  strcpy(hdr_fname, cmd+8);
                  strupr(hdr_fname);
                  }

               else if ( strnicmp(cmd, "HlpFile=", 8) == 0 )
                  {
                  if (hlp_fname[0] != '\0')
                     warn(eoff,"Help Filename has already been defined.");
                  strcpy(hlp_fname, cmd+8);
                  strupr(hlp_fname);
                  }

               else if ( strnicmp(cmd, "Version=", 8) == 0 )
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
               if (strnicmp(cmd, "FF", 2) == 0 )
                  {
                  check_command_length(eoff,2);
                  if ( in_para )
                     *curr++ = '\n';  /* finish off current paragraph */
                  *curr++ = CMD_FF;
                  state = S_Start;
                  in_para = 0;
                  num_spaces = 0;
                  }

               else if (strnicmp(cmd, "DocFF", 5) == 0 )
                  {
                  check_command_length(eoff,5);
                  if ( in_para )
                     *curr++ = '\n';  /* finish off current paragraph */
                  if (!xonline)
                     *curr++ = CMD_XONLINE;
                  *curr++ = CMD_FF;
                  if (!xonline)
                     *curr++ = CMD_XONLINE;
                  state = S_Start;
                  in_para = 0;
                  num_spaces = 0;
                  }

               else if (strnicmp(cmd, "OnlineFF", 8) == 0 )
                  {
                  check_command_length(eoff,8);
                  if ( in_para )
                     *curr++ = '\n';  /* finish off current paragraph */
                  if (!xdoc)
                     *curr++ = CMD_XDOC;
                  *curr++ = CMD_FF;
                  if (!xdoc)
                     *curr++ = CMD_XDOC;
                  state = S_Start;
                  in_para = 0;
                  num_spaces = 0;
                  }

               else if ( strnicmp(cmd, "Label=", 6) == 0 )
                  {
                  if ((int)strlen(cmd+6) <= 0)
                     error(eoff,"Label has no name.");

                  else if ( !validate_label_name(cmd+6) )
                     error(eoff,"Label \"%s\" contains illegal characters.", cmd+6);

                  else if ( find_label(cmd+6) != NULL )
                     error(eoff,"Label \"%s\" already exists", cmd+6);

                  else
                     {
                     if ((int)strlen(cmd+6) > 32)
                        warn(eoff,"Label name is long.");

                    if ( (t.flags & TF_DATA) && cmd[6] == '@' )
                       warn(eoff, "Data topic has a local label.");

                     lbl.name      = dupstr(cmd+6, 0);
                     lbl.topic_num = num_topic;
                     lbl.topic_off = (unsigned)(curr - buffer);
                     lbl.doc_page  = -1;
                     add_label(&lbl);
                     }
                  }

               else if ( strnicmp(cmd, "Table=", 6) == 0 )
                  {
                  if ( in_para )
                     {
                     *curr++ = '\n';  /* finish off current paragraph */
                     in_para = 0;
                     num_spaces = 0;
                     state = S_Start;
                     }

                  if (!done)
                     {
                     if (imbedded)
                        unread_char('(');
                     unread_char('~');
                     done = 1;
                     }

                  create_table();
                  }

               else if ( strnicmp(cmd, "FormatExclude", 12) == 0 )
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

               else if ( strnicmp(cmd, "Format", 6) == 0 )
                  {
                  if (cmd[6] == '+')
                     {
                     check_command_length(eoff,7);
                     if ( !formatting )
                        {
                        formatting = 1;
                        in_para = 0;
                        num_spaces = 0;
                        state = S_Start;
                        }
                     else
                        warn(eoff,"\"Format+\" is already in effect.");
                     }
                  else if (cmd[6] == '-')
                     {
                     check_command_length(eoff,7);
                     if ( formatting )
                        {
                        if ( in_para )
                           *curr++ = '\n';  /* finish off current paragraph */
                        state = S_Start;
                        in_para = 0;
                        formatting = 0;
                        num_spaces = 0;
                        state = S_Start;
                        }
                     else
                        warn(eoff,"\"Format-\" is already in effect.");
                     }
                  else
                     error(eoff,"Invalid argument to Format.");
                  }

               else if ( strnicmp(cmd, "Online", 6) == 0 )
                  {
                  if (cmd[6] == '+')
                     {
                     check_command_length(eoff,7);

                     if ( xonline )
                        {
                        *curr++ = CMD_XONLINE;
                        xonline = 0;
                        }
                     else
                        warn(eoff,"\"Online+\" already in effect.");
                     }
                  else if (cmd[6] == '-')
                     {
                     check_command_length(eoff,7);
                     if ( !xonline )
                        {
                        *curr++ = CMD_XONLINE;
                        xonline = 1;
                        }
                     else
                        warn(eoff,"\"Online-\" already in effect.");
                     }
                  else
                     error(eoff,"Invalid argument to Online.");
                  }

               else if ( strnicmp(cmd, "Doc", 3) == 0 )
                  {
                  if (cmd[3] == '+')
                     {
                     check_command_length(eoff,4);
                     if ( xdoc )
                        {
                        *curr++ = CMD_XDOC;
                        xdoc = 0;
                        }
                     else
                        warn(eoff,"\"Doc+\" already in effect.");
                     }
                  else if (cmd[3] == '-')
                     {
                     check_command_length(eoff,4);
                     if ( !xdoc )
                        {
                        *curr++ = CMD_XDOC;
                        xdoc = 1;
                        }
                     else
                        warn(eoff,"\"Doc-\" already in effect.");
                     }
                  else
                     error(eoff,"Invalid argument to Doc.");
                  }

               else if ( strnicmp(cmd, "Center", 6) == 0 )
                  {
                  if (cmd[6] == '+')
                     {
                     check_command_length(eoff,7);
                     if ( !centering )
                        {
                        centering = 1;
                        if ( in_para )
                           {
                           *curr++ = '\n';
                           in_para = 0;
                           }
                        state = S_Start;  /* for centering FSM */
                        }
                     else
                        warn(eoff,"\"Center+\" already in effect.");
                     }
                  else if (cmd[6] == '-')
                     {
                     check_command_length(eoff,7);
                     if ( centering )
                        {
                        centering = 0;
                        state = S_Start;  /* for centering FSM */
                        }
                     else
                        warn(eoff,"\"Center-\" already in effect.");
                     }
                  else
                     error(eoff,"Invalid argument to Center.");
                  }

               else if ( strnicmp(cmd, "CompressSpaces", 14) == 0 )
                  {
                  check_command_length(eoff,15);

                  if ( cmd[14] == '+' )
                     {
                     if ( compress_spaces )
                        warn(eoff,"\"CompressSpaces+\" is already in effect.");
                     else
                        compress_spaces = 1;
                     }
                  else if ( cmd[14] == '-' )
                     {
                     if ( !compress_spaces )
                        warn(eoff,"\"CompressSpaces-\" is already in effect.");
                     else
                        compress_spaces = 0;
                     }
                  else
                     error(eoff,"Invalid argument to CompressSpaces.");
                  }

               else if ( strnicmp("BinInc ", cmd, 7) == 0 )
                  {
                  if ( !(t.flags & TF_DATA) )
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

      if ( !in_topic )
         {
         cmd[0] = ch;
         ptr = read_until(cmd+1, 127, "\n~");
         if (*ptr == '~')
            unread_char('~');
         *ptr = '\0';
         error(0,"Text outside of any topic \"%s\".", cmd);
         continue;
         }

      if ( centering )
         {
         do
            {
            again = 0;   /* default */

            switch (state)
               {
               case S_Start:
                  if (ch == ' ')
                     ; /* do nothing */
                  else if ( (ch&0xFF) == '\n' )
                     *curr++ = ch;  /* no need to center blank lines. */
                  else
                     {
                     *curr++ = CMD_CENTER;
                     state = S_Line;
                     again = 1;
                     }
                  break;

               case S_Line:
                  put_a_char(ch, &t);
                  if ( (ch&0xFF) == '\n')
                     state = S_Start;
                  break;
               } /* switch */
            }
         while (again);
         }

      else if ( formatting )
         {
         int again;

         do
            {
            again = 0;   /* default */

            switch (state)
               {
               case S_Start:
                  if ( (ch&0xFF) == '\n' )
                     *curr++ = ch;
                  else
                     {
                     state = S_StartFirstLine;
                     num_spaces = 0;
                     again = 1;
                     }
                  break;

               case S_StartFirstLine:
                  if ( ch == ' ')
                     ++num_spaces;

                  else
                     {
                     if (lformat_exclude > 0 && num_spaces >= lformat_exclude )
                        {
                        put_spaces(num_spaces);
                        num_spaces = 0;
                        state = S_FormatDisabled;
                        again = 1;
                        }
                     else
                        {
                        *curr++ = CMD_PARA;
                        *curr++ = (char)num_spaces;
                        *curr++ = (char)num_spaces;
                        margin_pos = curr - 1;
                        state = S_FirstLine;
                        again = 1;
                        in_para = 1;
                        }
                     }
                  break;

               case S_FirstLine:
                  if (ch == '\n')
                     {
                     state = S_StartSecondLine;
                     num_spaces = 0;
                     }
                  else if (ch == ('\n'|0x100) )   /* force end of para ? */
                     {
                     *curr++ = '\n';
                     in_para = 0;
                     state = S_Start;
                     }
                  else if ( ch == ' ' )
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
                     again = 1;
                     }
                  break;

               case S_StartSecondLine:
                  if ( ch == ' ')
                     ++num_spaces;
                  else if ((ch&0xFF) == '\n') /* a blank line means end of a para */
                     {
                     *curr++ = '\n';   /* end the para */
                     *curr++ = '\n';   /* for the blank line */
                     in_para = 0;
                     state = S_Start;
                     }
                  else
                     {
                     if (lformat_exclude > 0 && num_spaces >= lformat_exclude )
                        {
                        *curr++ = '\n';
                        in_para = 0;
                        put_spaces(num_spaces);
                        num_spaces = 0;
                        state = S_FormatDisabled;
                        again = 1;
                        }
                     else
                        {
                        add_blank_for_split();
                        margin = num_spaces;
                        *margin_pos = (char)num_spaces;
                        state = S_Line;
                        again = 1;
                        }
                     }
                  break;

               case S_Line:   /* all lines after the first */
                  if (ch == '\n')
                     {
                     state = S_StartLine;
                     num_spaces = 0;
                     }
                  else if (ch == ('\n' | 0x100) )   /* force end of para ? */
                     {
                     *curr++ = '\n';
                     in_para = 0;
                     state = S_Start;
                     }
                  else if ( ch == ' ' )
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
                     again = 1;
                     }
                  break;

               case S_StartLine:   /* for all lines after the second */
                  if ( ch == ' ')
                     ++num_spaces;
                  else if ((ch&0xFF) == '\n') /* a blank line means end of a para */
                     {
                     *curr++ = '\n';   /* end the para */
                     *curr++ = '\n';   /* for the blank line */
                     in_para = 0;
                     state = S_Start;
                     }
                  else
                     {
                     if ( num_spaces != margin )
                        {
                        *curr++ = '\n';
                        in_para = 0;
                        state = S_StartFirstLine;  /* with current num_spaces */
                        again = 1;
                        }
                     else
                        {
                        add_blank_for_split();
                        state = S_Line;
                        again = 1;
                        }
                     }
                  break;

               case S_FormatDisabled:
                  if ( ch == ' ' )
                     {
                     state = S_FormatDisabledSpaces;
                     num_spaces = 1;
                     }
                  else
                     {
                     if ( (ch&0xFF) == '\n' )
                        state = S_Start;
                     put_a_char(ch, &t);
                     }
                  break;

               case S_FormatDisabledSpaces:
                  if ( ch == ' ' )
                     ++num_spaces;
                  else
                     {
                     put_spaces(num_spaces);
                     num_spaces = 0;    /* is this needed? */
                     state = S_FormatDisabled;
                     again = 1;
                     }
                  break;

               } /* switch (state) */
            }
         while (again);
         }

      else
         {
         do
            {
            again = 0;   /* default */

            switch (state)
               {
               case S_Start:
                  if ( ch == ' ' )
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
                     again = 1;
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


void make_hot_links(void)
   /*
    * calculate topic_num/topic_off for each link.
    */
   {
   LINK    *l;
   LABEL   *lbl;
   int      lctr;
   int      t;
   CONTENT *c;
   int      ctr;

   msg("Making hot-links.");

   /*
    * Calculate topic_num for all entries in DocContents.  Also set
    * "TF_IN_DOC" flag for all topics included in the document.
    */

   for (lctr=0, c=contents; lctr<num_contents; lctr++, c++)
      {
      for (ctr=0; ctr<c->num_topic; ctr++)
         {
         if ( c->is_label[ctr] )
            {
            lbl = find_label(c->topic_name[ctr]);
            if (lbl == NULL)
               {
               src_cfname = c->srcfile;
               srcline = c->srcline;
               error(0,"Cannot find DocContent label \"%s\".", c->topic_name[ctr]);
               srcline = -1;
               }
            else
               {
               if ( topic[lbl->topic_num].flags & TF_DATA )
                  {
                  src_cfname = c->srcfile;
                  srcline = c->srcline;
                  error(0,"Label \"%s\" is a data-only topic.", c->topic_name[ctr]);
                  srcline = -1;
                  }
               else
                  {
                  c->topic_num[ctr] = lbl->topic_num;
                  if ( topic[lbl->topic_num].flags & TF_IN_DOC )
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
               if ( topic[t].flags & TF_IN_DOC )
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

   for (lctr=0, l=a_link; lctr<num_link; l++, lctr++)
      {
      switch ( l->type )
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
            if (lbl == NULL)
               {
               src_cfname = l->srcfile;
               srcline = l->srcline; /* pretend again */
               error(0,"Cannot find explicit hot-link \"%s\".", l->name);
               srcline = -1;
               }
            else
               {
               if ( topic[lbl->topic_num].flags & TF_DATA )
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

   p.offset = (unsigned) (start - text);
   p.length = (unsigned) (curr - start);
   p.margin = margin;
   add_page(t, &p);

   if (max_links < num_links)
      max_links = num_links;
   }


void paginate_online(void)    /* paginate the text for on-line help */
   {                   /* also calculates max_pages and max_links */
   int       lnum;
   char     *start;
   char     *curr;
   char     *text;
   TOPIC    *t;
   int       tctr;
   unsigned  len;
   int       skip_blanks;
   int       num_links;
   int       col;
   int       tok;
   int       size,
             width;
   int       start_margin;

   msg("Paginating online help.");

   for (t=topic, tctr=0; tctr<num_topic; t++, tctr++)
      {
      if ( t->flags & TF_DATA )
         continue;    /* don't paginate data topics */

      text = get_topic_text(t);
      curr = text;
      len  = t->text_len;

      start = curr;
      skip_blanks = 0;
      lnum = 0;
      num_links = 0;
      col = 0;
      start_margin = -1;

      while (len > 0)
         {
         tok = find_token_length(ONLINE, curr, len, &size, &width);

         switch ( tok )
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

                  if (tok == TOK_DONE || tok == TOK_NL || tok == TOK_FF )
                     break;

                  if ( tok == TOK_PARA )
                     {
                     col = 0;   /* fake a nl */
                     ++lnum;
                     break;
                     }

                  if (tok == TOK_XONLINE || tok == TOK_XDOC )
                     {
                     curr += size;
                     len -= size;
                     continue;
                     }

                  /* now tok is TOK_SPACE or TOK_LINK or TOK_WORD */

                  if (col+width > SCREEN_WIDTH)
                     {          /* go to next line... */
                     if ( ++lnum >= SCREEN_DEPTH )
                        {           /* go to next page... */
                        add_page_break(t, start_margin, text, start, curr, num_links);
                        start = curr + ( (tok == TOK_SPACE) ? size : 0 );
                        start_margin = margin;
                        lnum = 0;
                        num_links = 0;
                        }
                     if ( tok == TOK_SPACE )
                        width = 0;   /* skip spaces at start of a line */

                     col = margin;
                     }

                  col += width;
                  curr += size;
                  len -= size;
                  }

               skip_blanks = 0;
               width = size = 0;
               break;
               }

            case TOK_NL:
               if (skip_blanks && col == 0)
                  {
                  start += size;
                  break;
                  }
               ++lnum;
               if ( lnum >= SCREEN_DEPTH || (col == 0 && lnum==SCREEN_DEPTH-1) )
                  {
                  add_page_break(t, start_margin, text, start, curr, num_links);
                  start = curr + size;
                  start_margin = -1;
                  lnum = 0;
                  num_links = 0;
                  skip_blanks = 1;
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
               skip_blanks = 0;
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


typedef struct
   {
   int      cnum,  /* must match above #defines so pd_get_info() will work */
            tnum,
            link_dest_warn;

   char far *start;
   CONTENT  *c;
   LABEL    *lbl;

   } PAGINATE_DOC_INFO;


LABEL *find_next_label_by_topic(int t)
   {
   LABEL *temp, *g, *p;
   int    ctr;

   g = p = NULL;

   for (temp=label, ctr=0; ctr<num_label; ctr++, temp++)
      if ( temp->topic_num == t && temp->doc_page == -1 )
         {
         g = temp;
         break;
         }
      else if (temp->topic_num > t)
         break;

   for (temp=plabel, ctr=0; ctr<num_plabel; ctr++, temp++)
      if ( temp->topic_num == t && temp->doc_page == -1 )
         {
         p = temp;
         break;
         }
      else if (temp->topic_num > t)
         break;

   if ( p == NULL )
      return (g);

   else if ( g == NULL )
      return (p);

   else
      return ( (g->topic_off < p->topic_off) ? g : p );
   }


void set_hot_link_doc_page(void)
   /*
    * Find doc_page for all hot-links.
    */
   {
   LINK  *l;
   LABEL *lbl;
   int    lctr;
   int    t;

   for (lctr=0, l=a_link; lctr<num_link; l++, lctr++)
      {
      switch ( l->type )
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
            if (lbl == NULL)
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


void set_content_doc_page(void)
   /*
    * insert page #'s in the DocContents
    */
   {
   CONTENT *c;
   TOPIC   *t;
   char    *base;
   int      tnum;
   int      ctr;
   char     buf[4];
   int      len;

   tnum = find_topic_title(DOCCONTENTS_TITLE);
   assert(tnum>=0);
   t = &topic[tnum];

   base = get_topic_text(t);

   for (ctr=1, c=contents+1; ctr<num_contents; ctr++, c++)
      {
      assert(c->doc_page>=1);
      sprintf(buf, "%d", c->doc_page);
      len = (int) strlen(buf);
      assert(len<=3);
      memcpy(base+c->page_num_pos+(3-len), buf, len);
      }

   release_topic_text(t, 1);
   }


int pd_get_info(int cmd, PD_INFO *pd, int *info)
   {             /* this funtion also used by print_document() */
   CONTENT *c;

   switch (cmd)
      {
      case PD_GET_CONTENT:
         if ( ++info[CNUM] >= num_contents )
            return (0);
         c = &contents[info[CNUM]];
         info[TNUM] = -1;
         pd->id       = c->id;
         pd->title    = c->name;
         pd->new_page = (c->flags & CF_NEW_PAGE) ? 1 : 0;
         return (1);

      case PD_GET_TOPIC:
         c = &contents[info[CNUM]];
         if ( ++info[TNUM] >= c->num_topic )
            return (0);
         pd->curr = get_topic_text( &topic[c->topic_num[info[TNUM]]] );
         pd->len = topic[c->topic_num[info[TNUM]]].text_len;
         return (1);

      case PD_GET_LINK_PAGE:
         if ( a_link[getint(pd->s)].doc_page == -1 )
            {
            if ( info[LINK_DEST_WARN] )
               {
               src_cfname = a_link[getint(pd->s)].srcfile;
               srcline    = a_link[getint(pd->s)].srcline;
               warn(0,"Hot-link destination is not in the document.");
               srcline = -1;
               }
            return (0);
            }
         pd->i = a_link[getint(pd->s)].doc_page;
         return (1);

      case PD_RELEASE_TOPIC:
         c = &contents[info[CNUM]];
         release_topic_text(&topic[c->topic_num[info[TNUM]]], 0);
         return (1);

      default:
         return (0);
      }
   }


int paginate_doc_output(int cmd, PD_INFO *pd, PAGINATE_DOC_INFO *info)
   {
   switch (cmd)
      {
      case PD_FOOTING:
      case PD_PRINT:
      case PD_PRINTN:
      case PD_PRINT_SEC:
         return (1);

      case PD_HEADING:
         ++num_doc_pages;
         return (1);

      case PD_START_SECTION:
         info->c = &contents[info->cnum];
         return (1);

      case PD_START_TOPIC:
         info->start = pd->curr;
         info->lbl = find_next_label_by_topic(info->c->topic_num[info->tnum]);
         return (1);

      case PD_SET_SECTION_PAGE:
         info->c->doc_page = pd->pnum;
         return (1);

      case PD_SET_TOPIC_PAGE:
         topic[info->c->topic_num[info->tnum]].doc_page = pd->pnum;
         return (1);

      case PD_PERIODIC:
         while ( info->lbl != NULL && (unsigned)(pd->curr - info->start) >= info->lbl->topic_off)
            {
            info->lbl->doc_page = pd->pnum;
            info->lbl = find_next_label_by_topic(info->c->topic_num[info->tnum]);
            }
         return (1);

      default:
         return (0);
      }
   }


void paginate_document(void)
   {
   PAGINATE_DOC_INFO info;

   if (num_contents == 0)
      return ;

   msg("Paginating document.");

   info.cnum = info.tnum = -1;
   info.link_dest_warn = 1;

   process_document((PD_FUNC)pd_get_info, (PD_FUNC)paginate_doc_output, &info);

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

   if ( (diff=strcmp(an,bn)) == 0 )
      return (0);

   if ( strcmp(an, INDEX_LABEL) == 0 )
      return (-1);

   if ( strcmp(bn, INDEX_LABEL) == 0 )
      return (1);

   return ( diff );
   }


void sort_labels(void)
   {
   qsort(label,  num_label,  sizeof(LABEL), fcmp_LABEL);
   qsort(plabel, num_plabel, sizeof(LABEL), fcmp_LABEL);
   }


/*
 * file write stuff.
 */


int compare_files(FILE *f1, FILE *f2) /* returns TRUE if different */
   {
   if ( filelength(fileno(f1)) != filelength(fileno(f2)) )
      return (1);   /* different if sizes are not the same */

   while ( !feof(f1) && !feof(f2) )
      if ( getc(f1) != getc(f2) )
         return (1);

   return ( ( feof(f1) && feof(f2) ) ? 0 : 1);
   }


void _write_hdr(char *fname, FILE *file)
   {
   int ctr;
   char nfile[MAXFILE],
        next[MAXEXT];

   FNSPLIT(fname, NULL, NULL, nfile, next);
   fprintf(file, "\n/*\n * %s%s\n", nfile, next);
   FNSPLIT(src_fname, NULL, NULL, nfile, next);
   fprintf(file, " *\n * Contains #defines for help.\n *\n");
   fprintf(file, " * Generated by HC from: %s%s\n *\n */\n\n\n", nfile, next);

   fprintf(file, "/* current help file version */\n");
   fprintf(file, "\n");
   fprintf(file, "#define %-32s %3d\n", "HELP_VERSION", version);
   fprintf(file, "\n\n");

   fprintf(file, "/* labels */\n\n");

   for (ctr=0; ctr<num_label; ctr++)
      if (label[ctr].name[0] != '@')  /* if it's not a local label... */
         {
         fprintf(file, "#define %-32s %3d", label[ctr].name, ctr);
         if ( strcmp(label[ctr].name, INDEX_LABEL) == 0 )
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

   if (hdr == NULL)
      {         /* if no prev. hdr file generate a new one */
      hdr = fopen(fname, "wt");
      if (hdr == NULL)
         fatal(0,"Cannot create \"%s\".", fname);
      msg("Writing: %s", fname);
      _write_hdr(fname, hdr);
      fclose(hdr);
      notice("FRACTINT must be re-compiled.");
      return ;
      }

   msg("Comparing: %s", fname);

   temp = fopen(TEMP_FNAME, "wt");

   if (temp == NULL)
      fatal(0,"Cannot create temporary file: \"%s\".", TEMP_FNAME);

   _write_hdr(fname, temp);

   fclose(temp);
   temp = fopen(TEMP_FNAME, "rt");

   if (temp == NULL)
      fatal(0,"Cannot open temporary file: \"%s\".", TEMP_FNAME);

   if ( compare_files(temp, hdr) )   /* if they are different... */
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


void calc_offsets(void)    /* calc file offset to each topic */
   {
   int      t;
   TOPIC   *tp;
   long     offset;
   CONTENT *cp;
   int      c;

   /* NOTE: offsets do NOT include 6 bytes for signature & version! */

   offset = sizeof(int) +           /* max_pages */
            sizeof(int) +           /* max_links */
            sizeof(int) +           /* num_topic */
            sizeof(int) +           /* num_label */
            sizeof(int) +           /* num_contents */
            sizeof(int) +           /* num_doc_pages */
            num_topic*sizeof(long) +/* offsets to each topic */
            num_label*2*sizeof(int);/* topic_num/topic_off for all public labels */

   for (c=0, cp=contents; c<num_contents; c++, cp++)
      offset += sizeof(int) +       /* flags */
                1 +                 /* id length */
                (int) strlen(cp->id) +    /* id text */
                1 +                 /* name length */
                (int) strlen(cp->name) +  /* name text */
                1 +                 /* number of topics */
                cp->num_topic*sizeof(int);    /* topic numbers */

   for (t=0, tp=topic; t<num_topic; t++, tp++)
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
      tok = find_token_length(0, curr, len, &size, NULL);

      if ( tok == TOK_LINK )
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
   int                   t, p, l, c;
   char                 *text;
   TOPIC                *tp;
   CONTENT              *cp;
   struct help_sig_info  hs;

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

   for (t=0; t<num_topic; t++)
      fwrite(&topic[t].offset, sizeof(long), 1, file);

   /* write all public labels */

   for (l=0; l<num_label; l++)
      {
      putw(label[l].topic_num, file);
      putw(label[l].topic_off, file);
      }

   /* write contents */

   for (c=0, cp=contents; c<num_contents; c++, cp++)
      {
      putw(cp->flags, file);

      t = (int) strlen(cp->id);
      putc((BYTE)t, file);
      fwrite(cp->id, 1, t, file);

      t = (int) strlen(cp->name);
      putc((BYTE)t, file);
      fwrite(cp->name, 1, t, file);

      putc((BYTE)cp->num_topic, file);
      fwrite(cp->topic_num, sizeof(int), cp->num_topic, file);
      }

   /* write topics */

   for (t=0, tp=topic; t<num_topic; t++, tp++)
      {
      /* write the topics flags */

      putw(tp->flags, file);

      /* write offset, length and starting margin for each page */

      putw(tp->num_page, file);
      for (p=0; p<tp->num_page; p++)
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

      if ( !(tp->flags & TF_DATA) )   /* don't process data topics... */
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

   if (hlp == NULL)
      fatal(0,"Cannot create .HLP file: \"%s\".", fname);

   msg("Writing: %s", fname);

   _write_help(hlp);

   fclose(hlp);
   }


/*
 * print document stuff.
 */


typedef struct
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
   } PRINT_DOC_INFO;


void printerc(PRINT_DOC_INFO *info, int c, int n)
   {
   while ( n-- > 0 )
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


void printers(PRINT_DOC_INFO *info, char far *s, int n)
   {
   if (n > 0)
      {
      while ( n-- > 0 )
         printerc(info, *s++, 1);
      }
   else
      {
      while ( *s != '\0' )
         printerc(info, *s++, 1);
      }
   }


int print_doc_output(int cmd, PD_INFO *pd, PRINT_DOC_INFO *info)
   {
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
         return (1);
         }

      case PD_FOOTING:
         info->margin = 0;
         printerc(info, '\f', 1);
         info->margin = PAGE_INDENT;
         return (1);

      case PD_PRINT:
         printers(info, pd->s, pd->i);
         return (1);

      case PD_PRINTN:
         printerc(info, *pd->s, pd->i);
         return (1);

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
         return (1);

      case PD_START_SECTION:
      case PD_START_TOPIC:
      case PD_SET_SECTION_PAGE:
      case PD_SET_TOPIC_PAGE:
      case PD_PERIODIC:
         return (1);

      default:
         return (0);
      }
   }


void print_document(char *fname)
   {
   PRINT_DOC_INFO info;

   if (num_contents == 0)
      fatal(0,".SRC has no DocContents.");

   msg("Printing to: %s", fname);

   info.cnum = info.tnum = -1;
   info.link_dest_warn = 0;

   if ( (info.file = fopen(fname, "wt")) == NULL )
      fatal(0,"Couldn't create \"%s\"", fname);

   info.margin = PAGE_INDENT;
   info.start_of_line = 1;
   info.spaces = 0;

   process_document((PD_FUNC)pd_get_info, (PD_FUNC)print_doc_output, &info);

   fclose(info.file);
   }


/*
 * compiler status and memory usage report stuff.
 */


void report_memory(void)
   {
   long string = 0,   /* bytes in strings */
        text   = 0,   /* bytes in topic text (stored on disk) */
        data   = 0,   /* bytes in active data structure */
        dead   = 0;   /* bytes in unused data structure */
   int  ctr, ctr2;

   for (ctr=0; ctr<num_topic; ctr++)
      {
      data   += sizeof(TOPIC);
      string += topic[ctr].title_len;
      text   += topic[ctr].text_len;
      data   += topic[ctr].num_page * sizeof(PAGE);

      dead   += (PAGE_ALLOC_SIZE-(topic[ctr].num_page%PAGE_ALLOC_SIZE)) * sizeof(PAGE);
      }

   for (ctr=0; ctr<num_link; ctr++)
      {
      data += sizeof(LINK);
      string += (long) strlen(a_link[ctr].name);
      }

   if (num_link > 0)
      dead += (LINK_ALLOC_SIZE-(num_link%LINK_ALLOC_SIZE)) * sizeof(LINK);

   for (ctr=0; ctr<num_label; ctr++)
      {
      data   += sizeof(LABEL);
      string += (long) strlen(label[ctr].name) + 1;
      }

   if (num_label > 0)
      dead += (LABEL_ALLOC_SIZE-(num_label%LABEL_ALLOC_SIZE)) * sizeof(LABEL);

   for (ctr=0; ctr<num_plabel; ctr++)
      {
      data   += sizeof(LABEL);
      string += (long) strlen(plabel[ctr].name) + 1;
      }

   if (num_plabel > 0)
      dead += (LABEL_ALLOC_SIZE-(num_plabel%LABEL_ALLOC_SIZE)) * sizeof(LABEL);

   for (ctr=0; ctr<num_contents; ctr++)
      {
      int t;

      t = ( MAX_CONTENT_TOPIC - contents[ctr].num_topic ) *
          ( sizeof(contents[0].is_label[0])   +
            sizeof(contents[0].topic_name[0]) +
            sizeof(contents[0].topic_num[0])     );
      data += sizeof(CONTENT) - t;
      dead += t;
      string += (long) strlen(contents[ctr].id) + 1;
      string += (long) strlen(contents[ctr].name) + 1;
      for (ctr2=0; ctr2<contents[ctr].num_topic; ctr2++)
         string += (long) strlen(contents[ctr].topic_name[ctr2]) + 1;
      }

   dead += (CONTENTS_ALLOC_SIZE-(num_contents%CONTENTS_ALLOC_SIZE)) * sizeof(CONTENT);

   printf("\n");
   printf("Memory Usage:\n");
   printf("%8ld Bytes in buffers.\n", (long)BUFFER_SIZE);
   printf("%8ld Bytes in strings.\n", string);
   printf("%8ld Bytes in data.\n", data);
   printf("%8ld Bytes in dead space.\n", dead);
   printf("--------\n");
   printf("%8ld Bytes total.\n", (long)BUFFER_SIZE+string+data+dead);
   printf("\n");
   printf("Disk Usage:\n");
   printf("%8ld Bytes in topic text.\n", text);
   }


void report_stats(void)
   {
   int  pages = 0;
   int      t;

   for (t=0; t<num_topic; t++)
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


void add_hlp_to_exe(char *hlp_fname, char *exe_fname)
   {
   int                  exe,   /* handles */
                        hlp;
   long                 len,
                        count;
   int                  size;
   struct help_sig_info hs;

   if ( (exe=open(exe_fname, O_RDWR|O_BINARY)) == -1 )
      fatal(0,"Unable to open \"%s\"", exe_fname);

   if ( (hlp=open(hlp_fname, O_RDONLY|O_BINARY)) == -1 )
      fatal(0,"Unable to open \"%s\"", hlp_fname);

   msg("Appending %s to %s", hlp_fname, exe_fname);

   /* first, check and see if any help is currently installed */

   lseek(exe, filelength(exe) - sizeof(struct help_sig_info), SEEK_SET);

   read(exe, (char *)&hs, 10);

   if ( hs.sig == HELP_SIG )
      warn(0,"Overwriting previous help. (Version=%d)", hs.version);
   else
      hs.base = filelength(exe);

   /* now, let's see if their help file is for real (and get the version) */

   read(hlp, (char *)&hs, sizeof(long)+sizeof(int));

   if (hs.sig != HELP_SIG )
      fatal(0,"Help signature not found in %s", hlp_fname);

   msg("Help file %s Version=%d", hlp_fname, hs.version);

   /* append the help stuff, overwriting old help (if any) */

   lseek(exe, hs.base, SEEK_SET);

   len = filelength(hlp) - sizeof(long) - sizeof(int); /* adjust for the file signature & version */

   for (count=0; count<len; )
      {
      size = (int) min((long)BUFFER_SIZE, len-count);
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


void delete_hlp_from_exe(char *exe_fname)
   {
   int   exe;   /* file handle */
   struct help_sig_info hs;

   if ( (exe=open(exe_fname, O_RDWR|O_BINARY)) == -1 )
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

   if ( hs.sig == HELP_SIG )
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
   int    show_stats = 0,
          show_mem   = 0;
   int    mode       = 0;

   char **arg;
   char   fname1[81],
          fname2[81];
   char   swappath[81];

   fname1[0] = fname2[0] = swappath[0] = 0;

   printf("HC - FRACTINT Help Compiler.\n\n");

   buffer = malloc(BUFFER_SIZE);

   if (buffer == NULL)
      fatal(0,"Not enough memory to allocate buffer.");

   for (arg= &argv[1]; argc>1; argc--, arg++)
      {
      switch ( (*arg)[0] )
         {
         case '/':
         case '-':
            switch ( (*arg)[1] )
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
                     show_mem = 1;
                  else
                     fatal(0,"/m switch allowed only when compiling (/c)");
                  break;

               case 's':
                  if (mode == MODE_COMPILE)
                     show_stats = 1;
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
                  quiet_mode = 1;
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
         printf( "To compile a .SRC file:\n");
         printf( "      HC /c [/s] [/m] [/r[path]] [src_file]\n");
         printf( "         /s       = report statistics.\n");
         printf( "         /m       = report memory usage.\n");
         printf( "         /r[path] = set swap file path.\n");
         printf( "         src_file = .SRC file.  Default is \"%s\"\n", DEFAULT_SRC_FNAME);
         printf( "To print a .SRC file:\n");
         printf( "      HC /p [/r[path]] [src_file] [out_file]\n");
         printf( "         /r[path] = set swap file path.\n");
         printf( "         src_file = .SRC file.  Default is \"%s\"\n", DEFAULT_SRC_FNAME);
         printf( "         out_file = Filename to print to. Default is \"%s\"\n",
         DEFAULT_DOC_FNAME);
         printf( "To append a .HLP file to an .EXE file:\n");
         printf( "      HC /a [hlp_file] [exe_file]\n");
         printf( "         hlp_file = .HLP file.  Default is \"%s\"\n", DEFAULT_HLP_FNAME);
         printf( "         exe_file = .EXE file.  Default is \"%s\"\n", DEFAULT_EXE_FNAME);
         printf( "To delete help info from an .EXE file:\n");
         printf( "      HC /d [exe_file]\n");
         printf( "         exe_file = .EXE file.  Default is \"%s\"\n", DEFAULT_EXE_FNAME);
         printf( "\n");
         printf( "Use \"/q\" for quiet mode. (No status messages.)\n");
         break;

      case MODE_COMPILE:
         if (fname2[0] != '\0')
            fatal(0,"Unexpected command-line argument \"%s\"", fname2);

         strcpy(src_fname, (fname1[0]=='\0') ? DEFAULT_SRC_FNAME : fname1);

         strcat(swappath, SWAP_FNAME);

         if ( (swapfile=fopen(swappath, "w+b")) == NULL )
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

         if ( !errors )     paginate_online();
         if ( !errors )     paginate_document();
         if ( !errors )     calc_offsets();
         if ( !errors )     sort_labels();
         if ( !errors )     write_hdr(hdr_fname);
         if ( !errors )     write_help(hlp_fname);

         if ( show_stats )
            report_stats();

         if ( show_mem )
            report_memory();

         if ( errors || warnings )
            report_errors();

         fclose(swapfile);
         remove(swappath);

         break;

      case MODE_PRINT:
         strcpy(src_fname, (fname1[0]=='\0') ? DEFAULT_SRC_FNAME : fname1);

         strcat(swappath, SWAP_FNAME);

         if ( (swapfile=fopen(swappath, "w+b")) == NULL )
            fatal(0,"Cannot create swap file \"%s\"", swappath);
         swappos = 0;

         read_src(src_fname);

         make_hot_links();

         if ( !errors )     paginate_document();
         if ( !errors )     print_document( (fname2[0]=='\0') ? DEFAULT_DOC_FNAME : fname2 );

         if ( errors || warnings )
            report_errors();

         fclose(swapfile);
         remove(swappath);

         break;

      case MODE_APPEND:
         add_hlp_to_exe( (fname1[0]=='\0') ? DEFAULT_HLP_FNAME : fname1,
                         (fname2[0]=='\0') ? DEFAULT_EXE_FNAME : fname2);
         break;

      case MODE_DELETE:
         if (fname2[0] != '\0')
            fatal(0,"Unexpected argument \"%s\"", fname2);
         delete_hlp_from_exe((fname1[0]=='\0') ? DEFAULT_EXE_FNAME : fname1);
         break;
      }

   free(buffer);

   return ( errors );   /* return the number of errors */
   }

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4311)
#endif
void check_buffer(char *current, unsigned off, char *buffer)
{
	if ((unsigned) curr + off - (unsigned) buffer >= (BUFFER_SIZE-1024))
	{
		fatal(0, "Buffer overflowerd -- Help topic too large.");
	}
}
#if defined(_WIN32)
#pragma warning(pop)
#endif
