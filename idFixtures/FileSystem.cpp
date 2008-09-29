#include "stdafx.h"
#include <direct.h>
#include <errno.h>
#include <process.h>
#include <shlwapi.h>

#include <cstdlib>
#include <string>
#include <vector>

#include "FileSystem.h"

void FileSystem::DirectorySetCurrent(std::string const &path)
{
	if (-1 == _chdir(path.c_str()))
	{
		throw CStandardLibraryException(errno);
	}
}

std::string FileSystem::DirectoryGetCurrent()
{
	std::vector<char> buffer;
	buffer.resize(_MAX_PATH);
	char const *cwd = _getcwd(&buffer[0], buffer.size());
	while (0 == cwd && ERANGE == errno)
	{
		buffer.resize(buffer.size()*2);
		cwd = _getcwd(&buffer[0], buffer.size());
	}
	if (0 == cwd)
	{
		throw CStandardLibraryException(errno);
	}

	return std::string(&buffer[0]);
}

int FileSystem::ExecuteCommand(const std::string &command)
{
	int status = system(command.c_str());
	if (-1 == status)
	{
		throw CStandardLibraryException(errno);
	}
	return status;
}

bool FileSystem::Exists(std::string const &path)
{
	return TRUE == ::PathFileExistsA(path.c_str());
}
