#pragma once

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
	const std::string &_heading;
	std::string &_fileTemplate;
	std::string &_fileName;

	static int CheckSpecialKeys(int key, int);
	static int SpeedPrompt(int row, int col, int vid, char *speedstring, int speed_match);
};
