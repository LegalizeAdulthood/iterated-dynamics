#if !defined(BROWSE_H)
#define BROWSE_H

class BrowseState
{
public:
	BrowseState()
		: m_auto_browse(false),
		m_browsing(false),
		m_check_parameters(false),
		m_check_type(false),
		m_sub_images(false)
	{
		m_mask[0] = 0;
		m_name[0] = 0;
	}
	~BrowseState()
	{
	}

	bool auto_browse() const			{ return m_auto_browse; }
	bool browsing() const				{ return m_browsing; }
	bool check_parameters() const		{ return m_check_parameters; }
	bool check_type() const				{ return m_check_type; }
	const std::string &mask() const		{ return m_mask; }
	const std::string &name() const		{ return m_name; }
	bool sub_images() const				{ return m_sub_images; }

	void set_auto_browse(bool value)	{ m_auto_browse = value; }
	void set_browsing(bool value)		{ m_browsing = value; }
	void set_check_parameters(bool value) { m_check_parameters = value; }
	void set_check_type(bool value)		{ m_check_type = value; }
	void set_mask(const char *value)	{ m_mask = value; }
	void set_name(const char *value)	{ m_name = value; }
	void set_sub_images(bool value)		{ m_sub_images = value; }

	void extract_read_name();
	void make_path(const char *fname, const char *ext);
	void merge_path_names(char *read_name);
	void merge_path_names(std::string &read_name);

private:
	std::string m_mask;
	std::string m_name;
	bool m_browsing;
	bool m_check_parameters;
	bool m_check_type;
	bool m_auto_browse;
	bool m_sub_images;
};

extern BrowseState g_browse_state;
extern std::string g_file_name_stack[16];

extern ApplicationStateType handle_look_for_files(bool &stacked);

#endif
