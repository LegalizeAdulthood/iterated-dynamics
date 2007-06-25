#if !defined(BROWSE_H)
#define BROWSE_H

class BrowseState
{
public:
	BrowseState()
		: m_auto_browse(false)
	{
	}
	bool auto_browse() const			{ return m_auto_browse; }
	bool browsing() const				{ return m_browsing; }
	bool check_parameters() const		{ return m_check_parameters; }
	bool check_type() const				{ return m_check_type; }
	const char *mask() const			{ return m_mask; }
	const char *name() const			{ return m_name; }

	void set_auto_browse(bool value)	{ m_auto_browse = value; }
	void set_browsing(bool value)		{ m_browsing = value; }
	void set_check_parameters(bool value) { m_check_parameters = value; }
	void set_check_type(bool value)		{ m_check_type = value; }
	void set_mask(const char *value)	{ ::strcpy(m_mask, value); }
	void set_name(const char *value)	{ ::strcpy(m_name, value); }
	void extract_read_name();
	void make_path(const char *fname, const char *ext);
	void merge_path_names(char *read_name);

private:
	char m_mask[FILE_MAX_FNAME];
	char m_name[FILE_MAX_FNAME];
	bool m_browsing;
	bool m_check_parameters;
	bool m_check_type;
	bool m_auto_browse;
};

extern BrowseState g_browse_state;

extern ApplicationStateType handle_look_for_files(bool &stacked);

#endif
