#if !defined(FRACTINT_HELP_H)
#define FRACTINT_HELP_H

#include <string>

enum HelpAction
{
	ACTION_CALL = 0,
	ACTION_PREV,
	ACTION_PREV2,
	ACTION_INDEX,
	ACTION_QUIT
};

extern void help(HelpAction action);
extern int read_help_topic(int, int, int, void *);
extern int makedoc_msg_func(int, int);
extern void print_document(const char *, int (*)(int, int), int);
extern void init_help();
extern void end_help();

extern void set_help_mode(int new_mode);
extern int get_help_mode();
extern int field_prompt_help(int help_mode,
							 std::string const &hdg, std::string const &instr,
							 char *fld, int len, int (*checkkey)(int) = 0);
extern int field_prompt_help(int help_mode, std::string const &hdg,
							 char *fld, int len, int (*check_keystroke)(int key) = 0);
extern long get_file_entry_help(int help_mode, int type,
	const char *title, char *fmask, char *filename, char *entryname);
long get_file_entry_help(int help_mode, int type,
	const char *title, char *fmask, std::string &filename, std::string &entryname);
extern int get_a_filename_help(int help_mode, char *hdg, char *file_template, char *flname);

class HelpModeSaver
{
public:
	HelpModeSaver(int new_mode) :
		m_old_mode(get_help_mode())
	{
		set_help_mode(new_mode);
	}
	~HelpModeSaver()
	{
		set_help_mode(m_old_mode);
	}

private:
	int m_old_mode;
};

#endif
