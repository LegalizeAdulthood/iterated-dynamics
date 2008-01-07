#if !defined(BROWSE_H)
#define BROWSE_H

class BrowseState
{
public:
	BrowseState()
		: _mask(),
		_name(),
		_browsing(false),
		_checkParameters(false),
		_checkType(false),
		_autoBrowse(false),
		_subImages(false),
		_crossHairBoxSize(3),
		_doubleCaution(false),
		_tooSmall(0.0f)
	{
	}
	~BrowseState()
	{
	}

	bool AutoBrowse() const				{ return _autoBrowse; }
	bool Browsing() const				{ return _browsing; }
	bool CheckParameters() const		{ return _checkParameters; }
	bool CheckType() const				{ return _checkType; }
	const std::string &Mask() const		{ return _mask; }
	const std::string &Name() const		{ return _name; }
	bool SubImages() const				{ return _subImages; }
	int CrossHairBoxSize() const		{ return _crossHairBoxSize; }
	bool DoubleCaution() const			{ return _doubleCaution; }
	float TooSmall() const				{ return _tooSmall; }

	void SetAutoBrowse(bool value)		{ _autoBrowse = value; }
	void SetBrowsing(bool value)		{ _browsing = value; }
	void SetCheckParameters(bool value) { _checkParameters = value; }
	void SetCheckType(bool value)		{ _checkType = value; }
	void SetMask(const char *value)	{ _mask = value; }
	void SetName(const std::string &value) { _name = value; }
	void SetSubImages(bool value)		{ _subImages = value; }
	void SetCrossHairBoxSize(int value) { _crossHairBoxSize = value; }
	void SetDoubleCaution(bool value)	{ _doubleCaution = value; }
	void SetTooSmall(float value)		{ _tooSmall = value; }

	void ExtractReadName();
	int GetParameters();
	void MakePath(const char *fname, const char *ext);
	void MergePathNames(char *read_name);
	void MergePathNames(std::string &read_name);
	void Restart();

private:
	std::string _mask;
	std::string _name;
	bool _browsing;
	bool _checkParameters;
	bool _checkType;
	bool _autoBrowse;
	bool _subImages;
	int _crossHairBoxSize;
	bool _doubleCaution;				/* confirm for deleting */
	float _tooSmall;
};

extern BrowseState g_browse_state;
extern std::string g_file_name_stack[16];

extern ApplicationStateType handle_look_for_files(bool &stacked);

#endif
