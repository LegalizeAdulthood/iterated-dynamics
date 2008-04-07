/*
 * help.cpp
 */
#include <cassert>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "strcpy.h"

#include "drivers.h"
#include "fihelp.h"
#include "filesystem.h"
#include "miscres.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"
#include "TextColors.h"

enum
{
	MAX_HIST = 16,					// number of pages we'll remember
	F_HIST = (1 << 0),				// flags for help_topic()
	F_INDEX = (1 << 1),
	MAX_PAGE_SIZE = (80*25),		// no page of text may be larger
	TEXT_START_ROW = 2,				// start print the help text here
	PRINT_BUFFER_SIZE = 32767,		// max. size of help topic in doc.
	MAX_NUM_TOPIC_SEC = 10			// max. number of topics under any single section (CONTENT)
};

struct LINK
{
	BYTE r, c;
	int           width;
	unsigned      offset;
	int           topic_num;
	unsigned      topic_off;
};

struct LABEL
{
	int      topic_num;
	unsigned topic_off;
};

struct PAGE
{
	unsigned      offset;
	unsigned      len;
	int           margin;
};

struct HIST
{
	int      topic_num;
	unsigned topic_off;
	int      link;
};

struct help_sig_info
{
	unsigned long sig;
	int           version;
	unsigned long base;     // only if added to exe
};

struct PRINT_DOC_INFO
{
	int       cnum;          // current CONTENT num
	int       tnum;          // current topic num

	long      content_pos;   // current CONTENT item offset in file
	int       num_page;      // total number of pages in document

	int       num_contents;  // total number of CONTENT entries
	int num_topic;			// number of topics in current CONTENT

	int       topic_num[MAX_NUM_TOPIC_SEC]; // topic_num[] for current CONTENT entry

	char buffer[PRINT_BUFFER_SIZE];        // text buffer

	char      id[81];        // s_buffer to store id in
	char      title[81];     // s_buffer to store title in

	int     (*msg_func)(int pnum, int num_page);
	int pnum;

	FILE     *file;          // file to sent output to
	int       margin;        // indent text by this much
	int       start_of_line; // are we at the beginning of a line?
	int       spaces;        // number of spaces in a row
};

class HelpSystem
{
public:
	HelpSystem();

	void Help(HelpAction action);
	int ReadTopic(int label_num, int off, VOIDPTR buf, int len);
	void PrintDocument(char const *outfname, int (*msg_func)(int, int), int save_extraseg);
	void Init();
	void End();
	void PushMode(int newMode);
	void PopMode();
	void SetMode(int newMode);
	int Mode() const { return mode_; }

private:
	typedef std::ifstream::off_type off_type;

	int mode_;
	std::ifstream stream_;
	off_type offset_;
	int maxLinks_;
	int maxPages_;
	int labelCount_;
	int topicCount_;
	int historyPosition_;

	std::vector<off_type> topicOffset_;		// 4*s_num_topic
	std::vector<LABEL> labels_;				// 4*s_num_label
	std::vector<HIST> history_;				// 6*MAX_HIST

	// these items alloc'ed only while help is active...
	std::vector<char> buffer_;				// MAX_PAGE_SIZE
	std::vector<LINK> linkTable_;			// 10*s_max_links
	std::vector<PAGE> pageTable_;			// 4*s_max_pages

	int  currentTopic_;
	long currentBase_;
	int  currentLength_;

	int s_help_mode_stack[10];
	int s_help_mode_stack_top;

	int ReadTopic(int topic, int off, int len, VOIDPTR buf);
	static void Instructions();
	static void DisplayParseText(char const *text, unsigned len, int start_margin, int *num_link, LINK *link);
	static void DisplayText(int row, int col, int color, char const *text, unsigned len);
	static void DisplayColoredCharacter(int row, int col, int color, char ch);
	HelpAction Topic(HIST *curr, HIST *next, int flags);
	void DisplayPage(char *title, char *text, unsigned text_len,
		int page, int num_pages, int start_margin,
		int *num_link, LINK *link);
	void Seek(off_type pos)
	{
		stream_.seekg(offset_ + pos, SEEK_SET);
	}
	int Get()
	{
		return stream_.get();
	}
	template <typename T>
	void Get(T &value)
	{
		stream_.read(reinterpret_cast<char *>(&value), sizeof(T));
	}
	template <typename T>
	void Get(T *base, int count)
	{
		stream_.read(reinterpret_cast<char *>(base), count*sizeof(T));
	}
	int GetInfo(int cmd, PD_INFO *pd, PRINT_DOC_INFO *info);
	static int GetInfoCallback(int cmd, PD_INFO *pd, VOIDPTR info);
	int Output(int cmd, PD_INFO *pd, PRINT_DOC_INFO *info);
	static int OutputCallback(int cmd, PD_INFO *pd, VOIDPTR info);
	void PrintCharacter(PRINT_DOC_INFO *info, int c, int n);
	void PrintString(PRINT_DOC_INFO *info, char *s, int n);
	bool MoveLink(LINK *link, int num_link, int *curr, int (*f)(LINK *, int, int, int), int val);
	void ColorLink(LINK *link, int color);
	static void PutKey(char *name, char *descrip);
	static void PrintInstructions();
	static int Overlap(int a, int a2, int b, int b2);
	static int PrintMessage(int pnum, int num_pages);
	static int FindLinkUpDown(LINK *link, int num_link, int curr_link, int up);
	static int FindLinkLeftRight(LINK *link, int num_link, int curr_link, int left);
	static int FindLinkKey(LINK *, int num_link, int curr_link, int key);
	bool OpenHelpFile();
};

HelpSystem::HelpSystem()
	: mode_(0),
	stream_(),
	offset_(0),
	maxLinks_(0),
	maxPages_(0),
	labelCount_(0),
	topicCount_(0),
	historyPosition_(0),
	topicOffset_(),
	labels_(),
	history_(),
	buffer_(),
	linkTable_(),
	pageTable_(),
	currentTopic_(-1),
	currentBase_(0),
	currentLength_(0),
	s_help_mode_stack_top(0)
{
	std::fill(&s_help_mode_stack[0], &s_help_mode_stack[10], 0);
}

static HelpSystem s_helpSystem;

int HelpSystem::GetInfoCallback(int cmd, PD_INFO *pd, VOIDPTR info)
{
	return s_helpSystem.GetInfo(cmd, pd, static_cast<PRINT_DOC_INFO *>(info));
}

int HelpSystem::OutputCallback(int cmd, PD_INFO *pd, VOIDPTR info)
{
	return s_helpSystem.Output(cmd, pd, static_cast<PRINT_DOC_INFO *>(info));
}

void HelpSystem::DisplayColoredCharacter(int row, int col, int color, char ch)
{
	char const s[] = { ch, 0 };
	driver_put_string(row, col, color, s);
}

void HelpSystem::DisplayText(int row, int col, int color, char const *text, unsigned len)
{
	while (len-- != 0)
	{
		if (*text == CMD_LITERAL)
		{
			++text;
			--len;
		}
		DisplayColoredCharacter(row, col++, color, *text++);
	}
}

void HelpSystem::DisplayParseText(char const *text, unsigned len, int start_margin, int *num_link, LINK *link)
{
	g_text_cbase = SCREEN_INDENT;
	g_text_rbase = TEXT_START_ROW;

	char const *curr = text;
	int col = 0;
	int size = 0;
	int width = 0;
	int token = (start_margin >= 0) ? TOK_PARA : -1;
	while (true)
	{
		int row = 0;
		switch (token)
		{
		case TOK_PARA:
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
					token = find_token_length(ONLINE, curr, len, &size, &width);

					if (token == TOK_DONE || token == TOK_NL || token == TOK_FF)
					{
						break;
					}

					if (token == TOK_PARA)
					{
						col = 0;   // fake a new-line
						row++;
						break;
					}

					if (token == TOK_XONLINE || token == TOK_XDOC)
					{
						curr += size;
						len  -= size;
						continue;
					}

					// now tok is TOK_SPACE or TOK_LINK or TOK_WORD

					if (col + width > SCREEN_WIDTH)
					{          // go to next line...
						col = margin;
						++row;

						if (token == TOK_SPACE)
						{
							width = 0;   // skip spaces at start of a line
						}
					}

					if (token == TOK_LINK)
					{
						DisplayText(row, col, C_HELP_LINK, curr + 1 + 3*sizeof(int), width);
						if (num_link != 0)
						{
							link[*num_link].r         = (BYTE)row;
							link[*num_link].c         = (BYTE)col;
							link[*num_link].topic_num = getint(curr + 1);
							link[*num_link].topic_off = getint(curr + 1 + sizeof(int));
							link[*num_link].offset    = (unsigned) ((curr + 1 + 3*sizeof(int)) - text);
							link[*num_link].width     = width;
							++(*num_link);
						}
					}
					else if (token == TOK_WORD)
					{
						DisplayText(row, col, C_HELP_BODY, curr, width);
					}

					col += width;
					curr += size;
					len -= size;
				}

				width = 0;
				size = 0;
				break;
			}

		case TOK_CENTER:
			col = find_line_width(ONLINE, curr, len);
			col = (SCREEN_WIDTH - col)/2;
			if (col < 0)
			{
				col = 0;
			}
			break;

		case TOK_NL:
			col = 0;
			++row;
			break;

		case TOK_LINK:
			DisplayText(row, col, C_HELP_LINK, curr + 1 + 3*sizeof(int), width);
			if (num_link != 0)
			{
				link[*num_link].r         = (BYTE)row;
				link[*num_link].c         = (BYTE)col;
				link[*num_link].topic_num = getint(curr + 1);
				link[*num_link].topic_off = getint(curr + 1 + sizeof(int));
				link[*num_link].offset    = (unsigned) ((curr + 1 + 3*sizeof(int)) - text);
				link[*num_link].width     = width;
				++(*num_link);
			}
			break;

		case TOK_XONLINE:  // skip
		case TOK_FF:       // ignore
		case TOK_XDOC:     // ignore
		case TOK_DONE:
		case TOK_SPACE:
			break;

		case TOK_WORD:
			DisplayText(row, col, C_HELP_BODY, curr, width);
			break;
		} // switch

		curr += size;
		len  -= size;
		col  += width;

		if (len == 0)
		{
			break;
		}

		token = find_token_length(ONLINE, curr, len, &size, &width);
	} // while (true)

	g_text_cbase = 0;
	g_text_rbase = 0;
}

void HelpSystem::ColorLink(LINK *link, int color)
{
	g_text_cbase = SCREEN_INDENT;
	g_text_rbase = TEXT_START_ROW;

	driver_set_attr(link->r, link->c, color, link->width);

	g_text_cbase = 0;
	g_text_rbase = 0;
}

void HelpSystem::PutKey(char *name, char *descrip)
{
	driver_put_string(-1, -1, C_HELP_INSTR, name);
	driver_put_string(-1, -1, C_HELP_INSTR, ":");
	driver_put_string(-1, -1, C_HELP_INSTR, descrip);
	driver_put_string(-1, -1, C_HELP_INSTR, "  ");
}

void HelpSystem::Instructions()
{
	for (int ctr = 0; ctr < 80; ctr++)
	{
		driver_put_string(24, ctr, C_HELP_INSTR, " ");
	}

	driver_move_cursor(24, 1);
	PutKey("F1",               "Index");
	PutKey("K J H L", "Select");
	PutKey("Enter",            "Go to");
	PutKey("Backspace",        "Last topic");
	PutKey("Escape",           "Exit help");
}

void HelpSystem::PrintInstructions()
{
	for (int ctr = 0; ctr < 80; ctr++)
	{
		driver_put_string(24, ctr, C_HELP_INSTR, " ");
	}

	driver_move_cursor(24, 1);
	PutKey("Escape", "Abort");
}

void HelpSystem::DisplayPage(char *title, char *text, unsigned text_len,
						int page, int num_pages, int start_margin,
						int *num_link, LINK *link)
{
	help_title();
	Instructions();
	driver_set_attr(2, 0, C_HELP_BODY, 80*22);
	put_string_center(1, 0, 80, C_HELP_HDG, title);

	// Some systems (Ultrix) mess up if you write to column 80
	driver_put_string(1, 78 - (6 + ((num_pages >= 10) ? 2 : 1)), C_HELP_INSTR,
		str(boost::format("%2d of %d") % (page + 1) % num_pages));

	if (text != 0)
	{
		DisplayParseText(text, text_len, start_margin, num_link, link);
	}

	driver_hide_text_cursor();
}

/*
 * int Overlap(int a, int a2, int b, int b2);
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

int HelpSystem::Overlap(int a, int a2, int b, int b2)
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

int HelpSystem::FindLinkUpDown(LINK *link, int num_link, int curr_link, int up)
{
	LINK *temp = link;
	LINK *curr = &link[curr_link];
	int best_overlap = 0;
	int temp_overlap;
	LINK *best = 0;
	int temp_dist;
	int curr_c2 = curr->c + curr->width - 1;
	for (int ctr = 0; ctr < num_link; ctr++, temp++)
	{
		if (ctr != curr_link &&
			((up && temp->r < curr->r) || (!up && temp->r > curr->r)))
		{
			temp_overlap = Overlap(curr->c, curr_c2, temp->c, temp->c + temp->width-1);
			// if >= 3 lines between, prioritize on vertical distance:
			temp_dist = std::abs(temp->r - curr->r);
			if (temp_dist >= 4)
			{
				temp_overlap -= temp_dist*100;
			}

			if (best != 0)
			{
				if (best_overlap >= 0 && temp_overlap >= 0)
				{     // if they're both under curr set to closest in y dir
					if (std::abs(best->r - curr->r) > temp_dist)
					{
						best = 0;
					}
				}
				else
				{
					if (best_overlap < temp_overlap)
					{
						best = 0;
					}
				}
			}

			if (best == 0)
			{
				best = temp;
				best_overlap = temp_overlap;
			}
		}
	}

	return (best == 0) ? -1 : int(best-link);
}

int HelpSystem::FindLinkLeftRight(LINK *link, int num_link, int curr_link, int left)
{
	LINK *curr = &link[curr_link];
	int best_c2 = 0;
	int best_dist = 0;
	int temp_dist;
	LINK *temp = link;
	LINK *best = 0;
	int curr_c2 = curr->c + curr->width - 1;
	for (int ctr = 0; ctr < num_link; ctr++, temp++)
	{
		int temp_c2 = temp->c + temp->width - 1;

		if (ctr != curr_link &&
			((left && temp_c2 < int(curr->c)) || (!left && int(temp->c) > curr_c2)))
		{
			temp_dist = std::abs(curr->r - temp->r);

			if (best != 0)
			{
				if (best_dist == 0 && temp_dist == 0)  // if both on curr's line...
				{
					if ((left && std::abs(curr->c - best_c2) > std::abs(curr->c - temp_c2)) ||
						(!left && std::abs(curr_c2 - best->c) > std::abs(curr_c2 - temp->c)))
					{
						best = 0;
					}
				}
				else if (best_dist >= temp_dist)   // if temp is closer...
				{
					best = 0;
				}
			}
			else
			{
				best      = temp;
				best_dist = temp_dist;
				best_c2   = temp_c2;
			}
		}
	}

	return (best == 0) ? -1 : int(best-link);
}

#ifdef __CLINT__
#   pragma argsused
#endif

int HelpSystem::FindLinkKey(LINK *, int num_link, int curr_link, int key)
{
	switch (key)
	{
	case IDK_TAB:		return (curr_link >= num_link-1) ? -1 : curr_link + 1;
	case IDK_SHF_TAB:	return (curr_link <= 0)          ? -1 : curr_link - 1;
	default:
		assert(!"bad key in find_link_key");
		return -1;
	}
}

bool HelpSystem::MoveLink(LINK *link, int num_link, int *curr, int (*f)(LINK *, int, int, int), int val)
{
	if (num_link > 1)
	{
		int t = (f == 0) ? val : f(link, num_link, *curr, val);
		if (t >= 0 && t != *curr)
		{
			ColorLink(&link[*curr], C_HELP_LINK);
			*curr = t;
			ColorLink(&link[*curr], C_HELP_CURLINK);
			return true;
		}
	}

	return false;
}

HelpAction HelpSystem::Topic(HIST *curr, HIST *next, int flags)
{
	off_type where_to = topicOffset_[curr->topic_num] + sizeof(int); // to skip flags
	int curr_link = curr->link;

	Seek(where_to);

	int num_pages;
	Get(num_pages);
	assert(num_pages > 0 && num_pages <= maxPages_);

	Get(&pageTable_[0], num_pages);

	BYTE ch = Get();
	int len = ch;
	assert(len < 81);
	char title[81];
	Get(title, len);
	title[len] = '\0';

	where_to += sizeof(int) + num_pages*3*sizeof(int) + 1 + len + sizeof(int);

	int page;
	for (page = 0; page < num_pages; page++)
	{
		if (curr->topic_off >= pageTable_[page].offset &&
			curr->topic_off <  pageTable_[page].offset + pageTable_[page].len)
		{
			break;
		}
	}
	assert(page < num_pages);

	HelpAction action = HelpAction(-1);
	enum DrawPageType
	{
		DRAWPAGE_SAME_PAGE = 0,
		DRAWPAGE_FIRST_LINK = 1,
		DRAWPAGE_SAME_LINK = 2,
		DRAWPAGE_LAST_LINK = 3
	};
	DrawPageType draw_page = DRAWPAGE_SAME_LINK;

	int num_link;
	do
	{
		if (draw_page)
		{
			Seek(where_to + pageTable_[page].offset);

			Get(&buffer_[0], pageTable_[page].len);

			num_link = 0;
			DisplayPage(title, &buffer_[0], pageTable_[page].len, page, num_pages,
				pageTable_[page].margin, &num_link, &linkTable_[0]);

			if (draw_page == DRAWPAGE_SAME_LINK)
			{
				assert(num_link <= 0 || (curr_link >= 0 && curr_link < num_link));
			}
			else if (draw_page == DRAWPAGE_LAST_LINK)
			{
				curr_link = num_link - 1;
			}
			else
			{
				curr_link = 0;
			}

			if (num_link > 0)
			{
				ColorLink(&linkTable_[curr_link], C_HELP_CURLINK);
			}

			draw_page = DRAWPAGE_SAME_PAGE;
		}

		int key = driver_get_key();

		switch (key)
		{
		case IDK_PAGE_DOWN:
			if (page < num_pages - 1)
			{
				page++;
				draw_page = DRAWPAGE_FIRST_LINK;
			}
			break;

		case IDK_PAGE_UP:
			if (page > 0)
			{
				page--;
				draw_page = DRAWPAGE_FIRST_LINK;
			}
			break;

		case IDK_HOME:
			if (page != 0)
			{
				page = 0;
				draw_page = DRAWPAGE_FIRST_LINK;
			}
			else
			{
				MoveLink(&linkTable_[0], num_link, &curr_link, 0, 0);
			}
			break;

		case IDK_END:
			if (page != num_pages - 1)
			{
				page = num_pages - 1;
				draw_page = DRAWPAGE_LAST_LINK;
			}
			else
			{
				MoveLink(&linkTable_[0], num_link, &curr_link, 0, num_link - 1);
			}
			break;

		case IDK_TAB:
			if (!MoveLink(&linkTable_[0], num_link, &curr_link, FindLinkKey, key) &&
				page < num_pages-1)
			{
				++page;
				draw_page = DRAWPAGE_FIRST_LINK;
			}
			break;

		case IDK_SHF_TAB:
			if (!MoveLink(&linkTable_[0], num_link, &curr_link, FindLinkKey, key) &&
				page > 0)
			{
				--page;
				draw_page = DRAWPAGE_LAST_LINK;
			}
			break;

		case IDK_DOWN_ARROW:
			if (!MoveLink(&linkTable_[0], num_link, &curr_link, FindLinkUpDown, 0) &&
				page < num_pages-1)
			{
				++page;
				draw_page = DRAWPAGE_FIRST_LINK;
			}
			break;

		case IDK_UP_ARROW:
			if (!MoveLink(&linkTable_[0], num_link, &curr_link, FindLinkUpDown, 1) &&
				page > 0)
			{
				--page;
				draw_page = DRAWPAGE_LAST_LINK;
			}
			break;

		case IDK_LEFT_ARROW:
			MoveLink(&linkTable_[0], num_link, &curr_link, FindLinkLeftRight, 1);
			break;

		case IDK_RIGHT_ARROW:
			MoveLink(&linkTable_[0], num_link, &curr_link, FindLinkLeftRight, 0);
			break;

		case IDK_ESC:         // exit help
			action = ACTION_QUIT;
			break;

		case IDK_BACKSPACE:   // prev topic
		case IDK_ALT_F1:
			if (flags & F_HIST)
			{
				action = ACTION_PREV;
			}
			break;

		case IDK_F1:    // help index
			if (!(flags & F_INDEX))
			{
				action = ACTION_INDEX;
			}
			break;

		case IDK_ENTER:
		case IDK_ENTER_2:
			if (num_link > 0)
			{
				next->topic_num = linkTable_[curr_link].topic_num;
				next->topic_off = linkTable_[curr_link].topic_off;
				action = ACTION_CALL;
			}
			break;
		} // switch
	}
	while (action == HelpAction(-1));

	curr->topic_off = pageTable_[page].offset;
	curr->link      = curr_link;

	return action;
}

void HelpSystem::Help(HelpAction action)
{
	if (mode_ == -1)   // is help disabled?
	{
		return;
	}

	if (!stream_.is_open())
	{
		driver_buzzer(BUZZER_ERROR);
		return;
	}

	buffer_.resize(MAX_PAGE_SIZE);
	linkTable_.resize(maxLinks_);
	pageTable_.resize(maxPages_);

	MouseModeSaver saved_mouse(LOOK_MOUSE_NONE);
	g_timer_start -= clock_ticks();
	ScreenStacker stacker;

	HIST next;
	if (mode_ >= 0)
	{
		next.topic_num = labels_[mode_].topic_num;
		next.topic_off = labels_[mode_].topic_off;
	}
	else
	{
		next.topic_num = mode_;
		next.topic_off = 0;
	}

	int savedMode_ = mode_;

	if (historyPosition_ <= 0)
	{
		action = ACTION_CALL;  // make sure it isn't ACTION_PREV!
	}

	HIST curr;
	do
	{
		switch (action)
		{
		case ACTION_PREV2:
			if (historyPosition_ > 0)
			{
				curr = history_[--historyPosition_];
			}
			// fall-through

		case ACTION_PREV:
			if (historyPosition_ > 0)
			{
				curr = history_[--historyPosition_];
			}
			break;

		case ACTION_QUIT:
			break;

		case ACTION_INDEX:
			next.topic_num = labels_[FIHELP_INDEX].topic_num;
			next.topic_off = labels_[FIHELP_INDEX].topic_off;
			// fall-through

		case ACTION_CALL:
			curr = next;
			curr.link = 0;
			break;

		default:
			assert(!"Bad help action");
			break;
		}

		int flags = 0;
		if (curr.topic_num == labels_[FIHELP_INDEX].topic_num)
		{
			flags |= F_INDEX;
		}
		if (historyPosition_ > 0)
		{
			flags |= F_HIST;
		}

		if (curr.topic_num >= 0)
		{
			action = Topic(&curr, &next, flags);
		}
		else
		{
			if (curr.topic_num == -100)
			{
				PrintDocument("id.doc", PrintMessage, 1);
				action = ACTION_PREV2;
			}
			else if (curr.topic_num == -101)
			{
				action = ACTION_PREV2;
			}
			else
			{
				DisplayPage("Unknown Help Topic", 0, 0, 0, 1, 0, 0, 0);
				action = HelpAction(-1);
				while (action == HelpAction(-1))
				{
					switch (driver_get_key())
					{
					case IDK_ESC:      action = ACTION_QUIT;  break;
					case IDK_ALT_F1:   action = ACTION_PREV;  break;
					case IDK_F1:       action = ACTION_INDEX; break;
					} // switch
				} // while
			}
		} // else

		if (action != ACTION_PREV && action != ACTION_PREV2)
		{
			if (historyPosition_ >= MAX_HIST)
			{
				int ctr;

				for (ctr = 0; ctr < MAX_HIST-1; ctr++)
				{
					history_[ctr] = history_[ctr + 1];
				}

				historyPosition_ = MAX_HIST-1;
			}
			history_[historyPosition_++] = curr;
		}
	}
	while (action != ACTION_QUIT);

	buffer_.clear();
	linkTable_.clear();
	pageTable_.clear();

	mode_ = savedMode_;
	g_timer_start += clock_ticks();
}

int HelpSystem::ReadTopic(int label_num, int off, VOIDPTR buf, int len)
{
	return ReadTopic(labels_[label_num].topic_num, labels_[label_num].topic_off + off, len, buf);
}

int HelpSystem::ReadTopic(int topic, int off, int len, VOIDPTR buf)
{
	if (topic != currentTopic_)
	{
		currentTopic_ = topic;

		currentBase_ = topicOffset_[topic];

		currentBase_ += sizeof(int);					// skip flags

		Seek(currentBase_);
		int t;
		Get(t);						// read num_pages
		currentBase_ += sizeof(int) + t*3*sizeof(int); // skip page info

		if (t > 0)
		{
			Seek(currentBase_);
		}
		// read title_len
		t = Get();
		currentBase_ += 1 + t;							// skip title

		if (t > 0)
		{
			Seek(currentBase_);
		}
		Get(currentLength_);				// read topic len
		currentBase_ += sizeof(int);
	}

	int read_len = (off + len > currentLength_) ? currentLength_ - off : len;
	if (read_len > 0)
	{
		Seek(currentBase_ + off);
		Get(static_cast<char *>(buf), read_len);
	}

	return currentLength_ - (off + len);
}

void HelpSystem::PrintCharacter(PRINT_DOC_INFO *info, int c, int n)
{
	while (n-- > 0)
	{
		if (c == ' ')
		{
			++info->spaces;
		}
		else if (c == '\n' || c == '\f')
		{
			info->start_of_line = 1;
			info->spaces = 0;   // strip spaces before a new-line
			fputc(c, info->file);
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

void HelpSystem::PrintString(PRINT_DOC_INFO *info, char *s, int n)
{
	if (n > 0)
	{
		while (n-- > 0)
		{
			PrintCharacter(info, *s++, 1);
		}
	}
	else
	{
		while (*s != '\0')
		{
			PrintCharacter(info, *s++, 1);
		}
	}
}

int HelpSystem::GetInfo(int cmd, PD_INFO *pd, PRINT_DOC_INFO *info)
{
	int tmp;
	int t;

	switch (cmd)
	{
	case PD_GET_CONTENT:
		if (++info->cnum >= info->num_contents)
		{
			return 0;
		}

		Seek(info->content_pos);

		Get(t);							// read flags
		info->content_pos += sizeof(int);
		pd->new_page = (t & 1) ? 1 : 0;

		t = Get();						// read id len
		if (t >= 80)
		{
			tmp = stream_.tellg();
		}
		assert(t < 80);
		Get(info->id, t);				// read the id
		info->content_pos += 1 + t;
		info->id[t] = '\0';

		t = Get();						// read title len
		assert(t < 80);
		Get(info->title, t);				// read the title
		info->content_pos += 1 + t;
		info->title[t] = '\0';

		t = Get();						// read num topics
		assert(t < MAX_NUM_TOPIC_SEC);
		Get(info->topic_num, t);			// read topic_num[]
		info->num_topic = t;
		info->content_pos += 1 + t*sizeof(int);

		info->tnum = -1;

		pd->id = info->id;
		pd->title = info->title;
		return 1;

	case PD_GET_TOPIC:
		if (++info->tnum >= info->num_topic)
		{
			return 0;
		}

		t = ReadTopic(info->topic_num[info->tnum], 0, PRINT_BUFFER_SIZE, info->buffer);

		assert(t <= 0);

		pd->curr = info->buffer;
		pd->len  = PRINT_BUFFER_SIZE + t;   // same as ...SIZE - abs(t)
		return 1;

	case PD_GET_LINK_PAGE:
		pd->i = getint(pd->s + sizeof(long));
		return (pd->i == -1) ? 0 : 1;

	case PD_RELEASE_TOPIC:
		return 1;

	default:
		return 0;
	}
}

int HelpSystem::Output(int cmd, PD_INFO *pd, PRINT_DOC_INFO *info)
{
	switch (cmd)
	{
	case PD_HEADING:
		{
			int keep_going = (info->msg_func != 0) ? info->msg_func(pd->pnum, info->num_page) : 1;

			info->margin = 0;

			char line[81];
			std::fill(line, line + 81, ' ');
			char buff[40];
			strcpy(buff, boost::format("Iterated Dynamics Version %d.%01d%c")
				% (g_release/100) % ((g_release % 100)/10)
				% ((g_release % 10) ? '0' + (g_release % 10) : ' '));
			int  width = PAGE_WIDTH + PAGE_INDENT;
			memmove(line + ((width - int(strlen(buff)))/2) - 4, buff, strlen(buff));

			strcpy(buff, boost::format("Page %d") % pd->pnum);
			memmove(line + (width - int(strlen(buff))), buff, strlen(buff));

			PrintCharacter(info, '\n', 1);
			PrintString(info, line, width);
			PrintCharacter(info, '\n', 2);

			info->margin = PAGE_INDENT;

			return keep_going;
		}

	case PD_FOOTING:
		info->margin = 0;
		PrintCharacter(info, '\f', 1);
		info->margin = PAGE_INDENT;
		return 1;

	case PD_PRINT:
		PrintString(info, pd->s, pd->i);
		return 1;

	case PD_PRINTN:
		PrintCharacter(info, *pd->s, pd->i);
		return 1;

	case PD_PRINT_SEC:
		info->margin = TITLE_INDENT;
		if (pd->id[0] != '\0')
		{
			PrintString(info, pd->id, 0);
			PrintCharacter(info, ' ', 1);
		}
		PrintString(info, pd->title, 0);
		PrintCharacter(info, '\n', 1);
		info->margin = PAGE_INDENT;
		return 1;

	case PD_START_SECTION:
	case PD_START_TOPIC:
	case PD_SET_SECTION_PAGE:
	case PD_SET_TOPIC_PAGE:
	case PD_PERIODIC:
		return 1;

	default:
		return 0;
	}
}

enum PrintDocumentStatus
{
	PRINTDOC_COMPLETE = -1,
	PRINTDOC_ABORTED = -2,
	PRINTDOC_INITIALIZATION = 0
};

int HelpSystem::PrintMessage(int pnum, int num_pages)
{
	int  key;

	if (pnum == PRINTDOC_COMPLETE)
	{
		driver_buzzer(BUZZER_COMPLETE);
		put_string_center(7, 0, 80, C_HELP_LINK, "Done -- Press any key");
		driver_get_key();
		return 0;
	}
	else if (pnum == PRINTDOC_ABORTED)
	{
		driver_buzzer(BUZZER_INTERRUPT);
		put_string_center(7, 0, 80, C_HELP_LINK, "Aborted -- Press any key");
		driver_get_key();
		return 0;
	}
	else if (pnum == PRINTDOC_INITIALIZATION)
	{
		help_title();
		PrintInstructions();
		driver_set_attr(2, 0, C_HELP_BODY, 80*22);
		put_string_center(1, 0, 80, C_HELP_HDG, "Generating id.doc");

		driver_put_string(7, 30, C_HELP_BODY, "Completed:");

		driver_hide_text_cursor();
	}

	driver_put_string(7, 41, C_HELP_LINK,
		str(boost::format("%d%%") % int((100.0/num_pages)*pnum)));

	while (driver_key_pressed())
	{
		key = driver_get_key();
		if (key == IDK_ESC)
		{
			return 0;    // user abort
		}
	}

	return 1;   // AOK -- continue
}

int makedoc_msg_func(int pnum, int num_pages)
{
	std::string message;
	int result = 0;

	if (pnum >= PRINTDOC_INITIALIZATION)
	{
		message = str(boost::format("\rcompleted %d%%" ) % int((100.0/num_pages)*pnum));
		result = 1;
	}
	else if (pnum == PRINTDOC_ABORTED)
	{
		message = "\n*** aborted\n";
	}
	stop_message(STOPMSG_NORMAL, message);
	return result;
}

void HelpSystem::PrintDocument(char const *outfname, int (*msg_func)(int, int), int save_extraseg)
{
	PRINT_DOC_INFO info;
	int success = 0;
	char *msg = 0;

	Seek(16L);
	Get(info.num_contents);
	Get(info.num_page);

	info.cnum = -1;
	info.tnum = -1;
	info.content_pos = 6*sizeof(int) + topicCount_*sizeof(long) + labelCount_*2*sizeof(int);
	info.msg_func = msg_func;

	if (msg_func != 0)
	{
		msg_func(PRINTDOC_INITIALIZATION, info.num_page);   // initialize
	}

	info.file = fopen(outfname, "wt");
	if (info.file == 0)
	{
		msg = "Unable to create output file.\n";
		goto ErrorAbort;
	}

	info.margin = PAGE_INDENT;
	info.start_of_line = 1;
	info.spaces = 0;

	success = process_document(GetInfoCallback, OutputCallback, &info);
	fclose(info.file);

ErrorAbort:
	if (msg != 0)
	{
		help_title();
		stop_message(STOPMSG_NO_STACK, msg);
	}
	else if (msg_func != 0)
	{
		msg_func(success ? PRINTDOC_COMPLETE : PRINTDOC_ABORTED, info.num_page);
	}
}

bool HelpSystem::OpenHelpFile()
{
	extern fs::path g_exe_path;
	fs::path helpFile = g_exe_path / "id.hlp";
	if (fs::exists(helpFile))
	{
		stream_.open(helpFile.string().c_str(), std::ios::in | std::ios::binary);
	}
	return stream_.is_open();
}

void HelpSystem::Init()
{
	help_sig_info hs = { 0 };
	if (!stream_.is_open())            // look for id.hlp
	{
		if (OpenHelpFile())
		{
			Get(hs.sig);
			Get(hs.version);

			if (hs.sig != HELP_SIG)
			{
				stream_.close();
				stop_message(STOPMSG_NO_STACK, "Invalid help signature in id.hlp!\n");
				return;
			}
			else if (hs.version != FIHELP_VERSION)
			{
				stream_.close();
				stop_message(STOPMSG_NO_STACK, "Wrong help version in id.hlp!\n");
				return;
			}
			else
			{
				offset_ = stream_.tellg();
			}
		}
	}

	if (!stream_)         // Can't find the help files anywhere!
	{
		stop_message(STOPMSG_NO_STACK,
			"Couldn't find id.hlp; set FRACTDIR to proper directory with setenv.\n");
		return;
	}

	Seek(0L);

	Get(maxPages_);
	Get(maxLinks_);
	Get(topicCount_);
	Get(labelCount_);
	Seek(6*sizeof(int));  // skip num_contents and num_doc_pages

	assert(maxPages_ > 0);
	assert(maxLinks_ >= 0);
	assert(topicCount_ > 0);
	assert(labelCount_ > 0);

	// allocate all three arrays
	topicOffset_.resize(topicCount_);
	labels_.resize(labelCount_);

	// read in the tables...
	Get(&topicOffset_[0], topicCount_);
	Get(&labels_[0], labelCount_);
}

void HelpSystem::End()
{
	if (stream_)
	{
		stream_.close();
		topicOffset_.clear();
		labels_.clear();
	}
}

void HelpSystem::PushMode(int newMode)
{
	assert(s_help_mode_stack_top < NUM_OF(s_help_mode_stack));
	s_help_mode_stack[s_help_mode_stack_top] = mode_;
	s_help_mode_stack_top++;
	mode_ = newMode;
}

void HelpSystem::PopMode()
{
	assert(s_help_mode_stack_top > 0);
	s_help_mode_stack_top--;
	mode_ = s_help_mode_stack[s_help_mode_stack_top];
}

void HelpSystem::SetMode(int new_mode)
{
	mode_ = new_mode;
}

void push_help_mode(int new_mode)
{
	s_helpSystem.PushMode(new_mode);
}

void pop_help_mode()
{
	s_helpSystem.PopMode();
}

void set_help_mode(int new_mode)
{
	s_helpSystem.SetMode(new_mode);
}

int get_help_mode()
{
	return s_helpSystem.Mode();
}

class HelpModePushPop
{
public:
	HelpModePushPop(int newMode)
	{
		s_helpSystem.PushMode(newMode);
	}
	~HelpModePushPop()
	{
		s_helpSystem.PopMode();
	}
};

int field_prompt_help(int help_mode,
					  std::string const &hdg, std::string const &instr,
					  char *fld, int len, int (*checkkey)(int key))
{
	HelpModePushPop pusher(help_mode);
	return field_prompt(hdg, instr, fld, len, checkkey);
}
int field_prompt_help(int help_mode,
					  std::string const &hdg,
					  char *fld, int len, int (*check_keystroke)(int key))
{
	return field_prompt_help(help_mode, hdg, "", fld, len, check_keystroke);
}


long get_file_entry_help(int help_mode, int type,
	const char *title, char *fmask, char *filename, char *entryname)
{
	HelpModePushPop pusher(help_mode);
	return get_file_entry(type, title, fmask, filename, entryname);
}

long get_file_entry_help(int help_mode, int type,
	const char *title, char *fmask, std::string &filename, std::string &entryname)
{
	char filename_buffer[FILE_MAX_PATH];
	char entryname_buffer[80];
	strcpy(filename_buffer, filename.c_str());
	strcpy(entryname_buffer, entryname.c_str());
	long result = get_file_entry_help(help_mode, type, title, fmask, filename_buffer, entryname_buffer);
	filename = filename_buffer;
	entryname = entryname_buffer;
	return result;
}

int get_a_filename_help(int help_mode, char *hdg, char *file_template, char *flname)
{
	HelpModePushPop pusher(help_mode);
	return get_a_filename(hdg, file_template, flname);
}

void help(HelpAction action)
{
	return s_helpSystem.Help(action);
}

// reads text from a help topic.  Returns number of bytes from (off + len)
// to end of topic.  On "EOF" returns a negative number representing
// number of bytes not read.
//
int read_help_topic(int label_num, int off, int len, VOIDPTR buf)
{
	return s_helpSystem.ReadTopic(label_num, off, buf, len);
}

void print_document(const char *outfname, int (*msg_func)(int, int), int save_extraseg)
{
	s_helpSystem.PrintDocument(outfname, msg_func, save_extraseg);
}

void init_help()
{
	s_helpSystem.Init();
}

void end_help()
{
	return s_helpSystem.End();
}

