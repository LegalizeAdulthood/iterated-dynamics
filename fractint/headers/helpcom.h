/*
 * helpcom.h
 *
 *
 * Common #defines, structures and code for HC.C and HELP.C
 *
 */

#ifndef HELPCOM_H
#define HELPCOM_H


/*
 * help file signature
 * If you get a syntax error, remove the LU from the end of the number.
 */

#define HELP_SIG           (0xAFBC1823LU)


/*
 * commands imbedded in the help text
 */
enum
{
	CMD_LITERAL       = 1,   // next char taken literally 
	CMD_PARA          = 2,   // paragraph start code 
	CMD_LINK          = 3,   // hot-link start/end code 
	CMD_FF            = 4,   // force a form-feed 
	CMD_XONLINE       = 5,   // exclude from online help on/off 
	CMD_XDOC          = 6,   // exclude from printed document on/off 
	CMD_CENTER        = 7,   // center this line 
	CMD_SPACE         = 8,   // next byte is count of spaces 
	MAX_CMD           = 8
};

/*
 * on-line help dimensions
 */
enum
{
	SCREEN_WIDTH      = 78,
	SCREEN_DEPTH      = 22,
	SCREEN_INDENT     = 1
};

/*
 * printed document dimensions
 */
enum
{
	PAGE_WIDTH         = 72,  // width of printed text 
	PAGE_INDENT        = 2,   // indent all text by this much 
	TITLE_INDENT       = 1,   // indent titles by this much 
	PAGE_RDEPTH        = 59,  // the total depth (inc. heading) 
	PAGE_HEADING_DEPTH = 3,   // depth of the heading 
	PAGE_DEPTH         = PAGE_RDEPTH-PAGE_HEADING_DEPTH, // depth of text 
};

/*
 * Document page-break macros.  Goto to next page if this close (or closer)
 * to end of page when starting a CONTENT, TOPIC, or at a BLANK line.
 */
enum
{
	CONTENT_BREAK = 7,  // start of a "DocContent" entry 
	TOPIC_BREAK   = 4,  // start of each topic under a DocContent entry 
	BLANK_BREAK   = 2  // a blank line 
};

/*
 * tokens returned by find_token_length
 */
enum
{
	TOK_DONE    = 0,   // len == 0             
	TOK_SPACE   = 1,   // a run of spaces      
	TOK_LINK    = 2,   // an entire link       
	TOK_PARA    = 3,   // a CMD_PARA           
	TOK_NL      = 4,   // a new-line ('\n',    
	TOK_FF      = 5,   // a form-feed (CMD_FF) 
	TOK_WORD    = 6,   // a word               
	TOK_XONLINE = 7,   // a CMD_XONLINE        
	TOK_XDOC    = 8,   // a CMD_XDOC           
	TOK_CENTER  = 9    // a CMD_CENTER         
};

/*
 * modes for find_token_length() and find_line_width()
 */
enum
{
	ONLINE = 1,
	DOC    = 2
};

/*
 * struct PD_INFO used by process_document()
 */
struct PD_INFO
{
	// used by process_document -- look but don't touch! 
	int pnum, lnum;

	// PD_GET_TOPIC is allowed to change these 
	char *curr;
	unsigned  len;

	// PD_GET_CONTENT is allowed to change these 
	char *id;
	char *title;
	int new_page;

	// general parameters 
	char *s;
	int       i;
};


/*
 * Commands passed to (*get_info)() and (*output)() by process_document()
 */

enum  PD_COMMANDS
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


typedef int (*PD_FUNC)(int cmd, PD_INFO *pd, VOIDPTR info);


int find_token_length(char const *curr, unsigned len, int *size, int *width);
int find_token_length(int mode, char const *curr, unsigned len, int *size, int *width);
int find_line_width(int mode, char const *curr, unsigned len);
int process_document(PD_FUNC get_info, PD_FUNC output, VOIDPTR info);


#ifndef XFRACT
inline int getint(void const *ptr) { return *static_cast<int const *>(ptr); }
inline void setint(void *ptr, int n) { *static_cast<int *>(ptr) = n; }
#else
/* Get an int from an unaligned pointer
 * This routine is needed because this program uses unaligned 2 byte
 * pointers all over the place.
 */
inline int getint(const char *ptr)
{
	int s;
	memcpy(&s, ptr, sizeof(int));
	return s;
}

// Set an int to an unaligned pointer 
void setint(char *ptr, int n)
{
	memcpy(ptr, &n, sizeof(int));
}
#endif

extern bool is_hyphen(const char *ptr);

#endif
