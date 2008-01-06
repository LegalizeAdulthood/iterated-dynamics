#if !defined(BROWSE_H)
#define BROWSE_H

class BrowseState
{
public:
	BrowseState()
		: m_mask(),
		m_name(),
		m_browsing(false),
		m_check_parameters(false),
		m_check_type(false),
		m_auto_browse(false),
		m_sub_images(false),
		_crossHairBoxSize(3),
		_doubleCaution(false),
		_tooSmall(0.0f)
	{
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
	int cross_hair_box_size() const		{ return _crossHairBoxSize; }
	bool double_caution() const			{ return _doubleCaution; }
	float too_small() const				{ return _tooSmall; }

	void set_auto_browse(bool value)	{ m_auto_browse = value; }
	void set_browsing(bool value)		{ m_browsing = value; }
	void set_check_parameters(bool value) { m_check_parameters = value; }
	void set_check_type(bool value)		{ m_check_type = value; }
	void set_mask(const char *value)	{ m_mask = value; }
	void set_name(const std::string &value) { m_name = value; }
	void set_sub_images(bool value)		{ m_sub_images = value; }
	void set_cross_hair_box_size(int value) { _crossHairBoxSize = value; }
	void set_double_caution(bool value)	{ _doubleCaution = value; }
	void set_too_small(float value)		{ _tooSmall = value; }

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
	int _crossHairBoxSize;
	bool _doubleCaution;				/* confirm for deleting */
	float _tooSmall;
};

extern BrowseState g_browse_state;
extern std::string g_file_name_stack[16];

extern ApplicationStateType handle_look_for_files(bool &stacked);

#endif
