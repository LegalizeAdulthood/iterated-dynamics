#if !defined(FRACTINT_HELP_H)
#define FRACTINT_HELP_H

extern void set_help_mode(int new_mode);
extern int get_help_mode();
extern int full_screen_prompt_help(int help_mode, const char *hdg, int numprompts, const char **prompts,
	struct full_screen_values *values, int fkeymask, char *extrainfo);
extern int field_prompt_help(int help_mode,
	char *hdg, char *instr, char *fld, int len, int (*checkkey)(int));
extern long get_file_entry_help(int help_mode,
	int type, char *title, char *fmask, char *filename, char *entryname);
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
