#if !defined(FILE_NAME_GETTER_H)
#define FILE_NAME_GETTER_H

#include <string>

class FileNameGetter
{
public:
	FileNameGetter(const std::string &heading, std::string &file_template, std::string &filename)
		: _heading(heading),
		_fileTemplate(file_template),
		_fileName(filename)
	{
	}
	~FileNameGetter()
	{
	}

	int Execute();

private:
	// speed key state values
	enum SpeedStateType
	{
		SPEEDSTATE_MATCHING = 0,		// string matches list - speed key mode
		SPEEDSTATE_TEMPLATE = -2,		// wild cards present - buiding template
		SPEEDSTATE_SEARCH_PATH = -3		// no match - building path search name
	};

	const std::string &_heading;
	std::string &_fileTemplate;
	std::string &_fileName;

	static SpeedStateType _speedState;
	static int CheckSpecialKeys(int key, int);
	static int SpeedPrompt(int row, int col, int vid, char *speedstring, int speed_match);
};

#endif
